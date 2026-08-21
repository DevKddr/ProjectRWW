#include "WeaponDataManager.h"
#include "Rarities/RarityDataManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "JsonObjectConverter.h"
#include "Engine/GameInstance.h"
#include "HAL/IConsoleManager.h"

// 프로젝트 폴더 구조에 맞춰 조정: Content/Data/output/weapons.json
const FString UWeaponDataManager::WeaponsJsonRelativePath = TEXT("Data/output/weapons.json");
IConsoleCommand* UWeaponDataManager::DebugPrintWeaponsCommand = nullptr;
int32 UWeaponDataManager::ActiveInstanceCount = 0;

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
		DebugPrintAllWeapons();
	}

	// 여러 GameInstance(PIE 멀티플레이어 등)가 동시에 존재해도 딱 한 번만 등록한다.
	++ActiveInstanceCount;
	if (ActiveInstanceCount == 1)
	{
		// 콘솔에서 "WeaponData.PrintAll" 입력 시 DebugPrintAllWeapons()가 실행되도록 등록.
		DebugPrintWeaponsCommand = IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("WeaponData.PrintAll"),
			TEXT("모든 무기의 파싱된 스탯을 로그에 출력 (파싱 검증용)."),
			FConsoleCommandDelegate::CreateUObject(this, &UWeaponDataManager::DebugPrintAllWeapons),
			ECVF_Default
		);
	}
}

void UWeaponDataManager::Deinitialize()
{
	// 마지막 인스턴스가 사라질 때만 실제로 해제한다 (다른 인스턴스가 아직 쓰고 있을 수 있음).
	--ActiveInstanceCount;
	if (ActiveInstanceCount == 0 && DebugPrintWeaponsCommand)
	{
		IConsoleManager::Get().UnregisterConsoleObject(DebugPrintWeaponsCommand);
		DebugPrintWeaponsCommand = nullptr;
	}
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

bool UWeaponDataManager::GetRequiredReloadMana(FName WeaponIndex, int32 CurAmmo, float& OutRequiredMana) const
{
	const FWeaponItem* Weapon = WeaponMap.Find(WeaponIndex);
	if (!Weapon) return false;

	if (!Weapon->Stats.CanReload)
	{
		// 장전 자체가 불가능한 무기 - 마나 계산이 의미가 없으므로 실패 처리.
		return false;
	}

	const int32 MaxAmmo = Weapon->Stats.MagazineSize;
	const int32 MissingAmmo = FMath::Max(MaxAmmo - CurAmmo, 0);
	OutRequiredMana = Weapon->Stats.WeaponReqMana - static_cast<float>(MissingAmmo) * Weapon->Stats.ManaPerAmmo;
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

void UWeaponDataManager::DebugPrintAllWeapons() const
{
	UE_LOG(LogTemp, Log, TEXT("========== [WeaponDataManager] 파싱 검증 (무기 %d개) =========="), WeaponMap.Num());
	for (const auto& Pair : WeaponMap)
	{
		const FWeaponItem& W = Pair.Value;
		UE_LOG(LogTemp, Log,
			TEXT("[%s] Type=%s Rarity=%s Name(ko)=%s | Damage=%.1f PelletCount=%d FireRate=%.2f FireMode=%s BurstCount=%d "
				 "| IsHitscan=%s CanADS=%s ScopeZoom=%.1f | MagSize=%d ReloadTime=%.2f "
				 "| CanReload=%s WeaponReqMana=%.1f ManaPerAmmo=%.1f"),
			*W.Index.ToString(), *W.WeaponType.ToString(), *W.Rarity.ToString(), *W.Name.Ko,
			W.Stats.Damage, W.Stats.PelletCount, W.Stats.FireRate_RPS, *W.Stats.FireMode.ToString(), W.Stats.BurstCount,
			W.Stats.IsHitscan ? TEXT("true") : TEXT("false"),
			W.Stats.CanADS ? TEXT("true") : TEXT("false"),
			W.Stats.ScopeZoomLevel,
			W.Stats.MagazineSize, W.Stats.ReloadTime,
			W.Stats.CanReload ? TEXT("true") : TEXT("false"),
			W.Stats.WeaponReqMana, W.Stats.ManaPerAmmo);
	}
	UE_LOG(LogTemp, Log, TEXT("========================================================"));
}
