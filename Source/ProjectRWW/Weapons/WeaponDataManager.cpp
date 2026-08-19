#include "WeaponDataManager.h"
#include "Rarities/RarityDataManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "JsonObjectConverter.h"
#include "Engine/GameInstance.h"

// 프로젝트 폴더 구조에 맞춰 조정: Content/Data/output/weapons.json
const FString UWeaponDataManager::WeaponsJsonRelativePath = TEXT("Data/output/weapons.json");

void UWeaponDataManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bDataLoaded = LoadWeapons();

	if (!bDataLoaded)
	{
		UE_LOG(LogTemp, Error, TEXT("[WeaponDataManager] 데이터 로드 실패"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[WeaponDataManager] 로드 완료: 무기 %d개"), WeaponMap.Num());
	}
}

void UWeaponDataManager::Deinitialize()
{
	WeaponMap.Empty();
	Super::Deinitialize();
}

bool UWeaponDataManager::LoadWeapons()
{
	const FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), WeaponsJsonRelativePath);
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FullPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[WeaponDataManager] 파일 없음: %s"), *FullPath);
		return false;
	}

	TArray<FWeaponItem> Parsed;
	if (!FJsonObjectConverter::JsonArrayStringToUStruct(JsonString, &Parsed, 0, 0))
	{
		UE_LOG(LogTemp, Error, TEXT("[WeaponDataManager] weapons.json 파싱 실패"));
		return false;
	}

	WeaponMap.Empty();
	for (const FWeaponItem& Entry : Parsed)
	{
		WeaponMap.Add(Entry.Index, Entry);
	}
	return true;
}

bool UWeaponDataManager::GetWeaponData(FName WeaponIndex, FWeaponItem& OutData) const
{
	if (const FWeaponItem* Found = WeaponMap.Find(WeaponIndex))
	{
		OutData = *Found;
		return true;
	}
	return false;
}

void UWeaponDataManager::SetLanguage(const FString& LanguageCode)
{
	if (LanguageCode == TEXT("ko") || LanguageCode == TEXT("en"))
	{
		CurrentLanguage = LanguageCode;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponDataManager] 지원하지 않는 언어 코드: %s (ko/en만 지원)"), *LanguageCode);
	}
}

FText UWeaponDataManager::GetWeaponDisplayName(FName WeaponIndex) const
{
	const FWeaponItem* Weapon = WeaponMap.Find(WeaponIndex);
	if (!Weapon)
	{
		return FText::FromString(FString::Printf(TEXT("[Unknown Weapon: %s]"), *WeaponIndex.ToString()));
	}
	const FString& Text = (CurrentLanguage == TEXT("en")) ? Weapon->Name.En : Weapon->Name.Ko;
	return FText::FromString(Text);
}

FText UWeaponDataManager::GetWeaponDescription(FName WeaponIndex) const
{
	const FWeaponItem* Weapon = WeaponMap.Find(WeaponIndex);
	if (!Weapon)
	{
		return FText::GetEmpty();
	}
	const FString& Text = (CurrentLanguage == TEXT("en")) ? Weapon->Description.En : Weapon->Description.Ko;
	return FText::FromString(Text);
}

bool UWeaponDataManager::GetWeaponSellValue(FName WeaponIndex, int32& OutSellValue) const
{
	const FWeaponItem* Weapon = WeaponMap.Find(WeaponIndex);
	if (!Weapon) return false;

	// 판매가는 이 매니저가 계산하지 않는다 - RarityDataManager가 유일한 출처.
	const URarityDataManager* RarityMgr = GetGameInstance()->GetSubsystem<URarityDataManager>();
	if (!RarityMgr) return false;

	FRarityData Rarity;
	if (!RarityMgr->GetRarityData(Weapon->Rarity, Rarity)) return false;

	OutSellValue = Rarity.SellValue;
	return true;
}

bool UWeaponDataManager::GetWeaponMaxTotalDamage(FName WeaponIndex, float& OutTotalDamage) const
{
	const FWeaponItem* Weapon = WeaponMap.Find(WeaponIndex);
	if (!Weapon) return false;
	OutTotalDamage = Weapon->Stats.Damage * static_cast<float>(Weapon->Stats.PelletCount);
	return true;
}

TArray<FWeaponItem> UWeaponDataManager::GetAllWeapons() const
{
	TArray<FWeaponItem> Result;
	WeaponMap.GenerateValueArray(Result);
	return Result;
}

TArray<FWeaponItem> UWeaponDataManager::GetWeaponsByType(FName WeaponType) const
{
	TArray<FWeaponItem> Result;
	for (const auto& Pair : WeaponMap)
	{
		if (Pair.Value.WeaponType == WeaponType)
		{
			Result.Add(Pair.Value);
		}
	}
	return Result;
}
