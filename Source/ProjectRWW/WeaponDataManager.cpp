#include "WeaponDataManager.h"
#include "LocalizationManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "JsonObjectConverter.h"
#include "Engine/GameInstance.h"

const FString UWeaponDataManager::WeaponJsonRelativePath = TEXT("Data/output/Weapon.json");
const FString UWeaponDataManager::WeaponTypeJsonRelativePath = TEXT("Data/output/WeaponTypeData.json");
const FString UWeaponDataManager::RarityJsonRelativePath = TEXT("Data/output/Rarity.json");

void UWeaponDataManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const bool bTypeOk = LoadWeaponTypeData();
	const bool bRarityOk = LoadRarityData();
	const bool bWeaponOk = LoadWeaponData();
	bDataLoaded = bTypeOk && bRarityOk && bWeaponOk;

	if (!bDataLoaded)
	{
		UE_LOG(LogTemp, Error, TEXT("[WeaponDataManager] 데이터 로드 실패"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[WeaponDataManager] 로드 완료: 타입 %d, 등급 %d, 무기 %d"),
			WeaponTypeMap.Num(), RarityMap.Num(), WeaponMap.Num());
	}
}

void UWeaponDataManager::Deinitialize()
{
	WeaponMap.Empty();
	WeaponTypeMap.Empty();
	RarityMap.Empty();
	Super::Deinitialize();
}

bool UWeaponDataManager::LoadWeaponTypeData()
{
	const FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), WeaponTypeJsonRelativePath);
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FullPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[WeaponDataManager] 파일 없음: %s"), *FullPath);
		return false;
	}
	TArray<FWeaponTypeData> Parsed;
	if (!FJsonObjectConverter::JsonArrayStringToUStruct(JsonString, &Parsed, 0, 0))
	{
		UE_LOG(LogTemp, Error, TEXT("[WeaponDataManager] WeaponTypeData 파싱 실패: %s"), *FullPath);
		return false;
	}
	WeaponTypeMap.Empty();
	for (const FWeaponTypeData& Entry : Parsed)
	{
		WeaponTypeMap.Add(Entry.TypeID, Entry);
	}
	return true;
}

bool UWeaponDataManager::LoadRarityData()
{
	const FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), RarityJsonRelativePath);
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FullPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[WeaponDataManager] 파일 없음: %s"), *FullPath);
		return false;
	}
	TArray<FRarityData> Parsed;
	if (!FJsonObjectConverter::JsonArrayStringToUStruct(JsonString, &Parsed, 0, 0))
	{
		UE_LOG(LogTemp, Error, TEXT("[WeaponDataManager] Rarity 파싱 실패: %s"), *FullPath);
		return false;
	}
	RarityMap.Empty();
	for (const FRarityData& Entry : Parsed)
	{
		RarityMap.Add(Entry.RarityID, Entry);
	}
	return true;
}

bool UWeaponDataManager::LoadWeaponData()
{
	const FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), WeaponJsonRelativePath);
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FullPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[WeaponDataManager] 파일 없음: %s"), *FullPath);
		return false;
	}
	TArray<FWeaponData> Parsed;
	if (!FJsonObjectConverter::JsonArrayStringToUStruct(JsonString, &Parsed, 0, 0))
	{
		UE_LOG(LogTemp, Error, TEXT("[WeaponDataManager] Weapon 파싱 실패: %s"), *FullPath);
		return false;
	}

	WeaponMap.Empty();
	for (const FWeaponData& Entry : Parsed)
	{
		if (!WeaponTypeMap.Contains(Entry.WeaponType))
		{
			UE_LOG(LogTemp, Warning, TEXT("[WeaponDataManager] '%s'의 WeaponType '%s' 미등록"),
				*Entry.Index.ToString(), *Entry.WeaponType.ToString());
		}
		if (!RarityMap.Contains(Entry.Rarity))
		{
			UE_LOG(LogTemp, Warning, TEXT("[WeaponDataManager] '%s'의 Rarity '%s' 미등록"),
				*Entry.Index.ToString(), *Entry.Rarity.ToString());
		}
		WeaponMap.Add(Entry.Index, Entry);
	}
	return true;
}

bool UWeaponDataManager::GetWeaponData(FName WeaponIndex, FWeaponData& OutData) const
{
	if (const FWeaponData* Found = WeaponMap.Find(WeaponIndex)) { OutData = *Found; return true; }
	return false;
}

bool UWeaponDataManager::GetWeaponTypeData(FName TypeID, FWeaponTypeData& OutData) const
{
	if (const FWeaponTypeData* Found = WeaponTypeMap.Find(TypeID)) { OutData = *Found; return true; }
	return false;
}

bool UWeaponDataManager::GetRarityData(FName RarityID, FRarityData& OutData) const
{
	if (const FRarityData* Found = RarityMap.Find(RarityID)) { OutData = *Found; return true; }
	return false;
}

bool UWeaponDataManager::GetWeaponSellValue(FName WeaponIndex, int32& OutSellValue) const
{
	const FWeaponData* Weapon = WeaponMap.Find(WeaponIndex);
	if (!Weapon) return false;
	const FRarityData* Rarity = RarityMap.Find(Weapon->Rarity);
	if (!Rarity) return false;
	OutSellValue = Rarity->SellValue;
	return true;
}

bool UWeaponDataManager::GetWeaponMaxTotalDamage(FName WeaponIndex, float& OutTotalDamage) const
{
	const FWeaponData* Weapon = WeaponMap.Find(WeaponIndex);
	if (!Weapon) return false;
	// 모든 펠릿(투사체)이 다 맞았을 때 기준. 부분 히트(예: 샷건 8발 중 5발)는
	// 호출부에서 실제로 맞은 펠릿 수를 따로 세어 Damage와 곱해야 한다.
	OutTotalDamage = Weapon->Damage * static_cast<float>(Weapon->PelletCount);
	return true;
}

FText UWeaponDataManager::GetWeaponDisplayName(FName WeaponIndex) const
{
	const FWeaponData* Weapon = WeaponMap.Find(WeaponIndex);
	if (!Weapon)
	{
		return FText::FromString(FString::Printf(TEXT("[Unknown Weapon: %s]"), *WeaponIndex.ToString()));
	}
	if (const ULocalizationManager* Loc = GetGameInstance()->GetSubsystem<ULocalizationManager>())
	{
		return Loc->GetLocalizedText(Weapon->NameKey);
	}
	return FText::FromName(Weapon->NameKey);
}

FText UWeaponDataManager::GetWeaponDescription(FName WeaponIndex) const
{
	const FWeaponData* Weapon = WeaponMap.Find(WeaponIndex);
	if (!Weapon)
	{
		return FText::GetEmpty();
	}
	if (const ULocalizationManager* Loc = GetGameInstance()->GetSubsystem<ULocalizationManager>())
	{
		return Loc->GetLocalizedText(Weapon->DescKey);
	}
	return FText::FromName(Weapon->DescKey);
}

TArray<FWeaponData> UWeaponDataManager::GetAllWeapons() const
{
	TArray<FWeaponData> Result;
	WeaponMap.GenerateValueArray(Result);
	return Result;
}

TArray<FWeaponData> UWeaponDataManager::GetWeaponsByType(FName TypeID) const
{
	TArray<FWeaponData> Result;
	for (const auto& Pair : WeaponMap)
		if (Pair.Value.WeaponType == TypeID) Result.Add(Pair.Value);
	return Result;
}
