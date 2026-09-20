#include "WeaponDataManager.h"
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

	// 콘솔 명령어는 프로세스 전역 자원이라, 여러 GameInstance(PIE 멀티플레이어 테스트 등)가
	// 동시에 떠 있어도 첫 인스턴스가 생길 때 한 번만 등록해야 한다.
	++ActiveInstanceCount;
	if (ActiveInstanceCount == 1)
	{
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
	// Name은 weapons.json에서 빠지고 items.json(ItemDataManager) 쪽으로 옮겨갔다.
	// ItemDataManager 연동은 별도 작업 범위라, 지금은 무기 존재 여부만 확인하고
	// Index를 그대로 보여주는 자리표시자로 남겨둔다.
	if (!WeaponMap.Contains(WeaponIndex))
	{
		return FText::FromString(FString::Printf(TEXT("[Unknown Weapon: %s]"), *WeaponIndex.ToString()));
	}
	return FText::FromString(WeaponIndex.ToString());
}

FText UWeaponDataManager::GetWeaponDescription(FName WeaponIndex) const
{
	// Description도 Name과 마찬가지로 items.json 쪽으로 옮겨갔다 (ItemDataManager 연동은 별도 작업).
	if (!WeaponMap.Contains(WeaponIndex))
	{
		return FText::GetEmpty();
	}
	return FText::GetEmpty();
}

bool UWeaponDataManager::GetWeaponSellValue(FName WeaponIndex, int32& OutSellValue) const
{
	// Rarity가 weapons.json에서 빠지고 items.json 쪽으로 옮겨갔다. 판매가 계산은
	// ItemDataManager(RarityId 조회) + RarityDataManager 연동이 필요한데, 그건 별도
	// 작업 범위라 지금은 계산 불가로 처리한다 - ItemDataManager 연동 시 이 함수만 고치면 된다.
	if (!WeaponMap.Contains(WeaponIndex))
	{
		return false;
	}
	UE_LOG(LogTemp, Warning, TEXT("[WeaponDataManager] GetWeaponSellValue: Rarity가 items.json으로 이전되어 아직 계산할 수 없습니다 (ItemDataManager 연동 대기 중)."));
	return false;
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
	OutRequiredMana = Weapon->Stats.WeaponReqMana + static_cast<float>(MissingAmmo) * Weapon->Stats.ManaPerAmmo;
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
			TEXT("[%s] Type=%s | Damage=%.1f PelletCount=%d FireRate=%.2f FireMode=%s BurstCount=%d "
				 "| IsHitscan=%s CanADS=%s ScopeZoom=%.1f | MagSize=%d ReloadTime=%.2f ReloadType=%s "
				 "| CanReload=%s WeaponReqMana=%.1f ManaPerAmmo=%.1f "
				 "| Recoil V[%.2f,%.2f] H[%.2f,%.2f]"),
			*W.Index.ToString(), *W.WeaponType.ToString(),
			W.Stats.Damage, W.Stats.PelletCount, W.Stats.FireRate_RPS, *W.Stats.FireMode.ToString(), W.Stats.BurstCount,
			W.Stats.IsHitscan ? TEXT("true") : TEXT("false"),
			W.Stats.CanADS ? TEXT("true") : TEXT("false"),
			W.Stats.ScopeZoomLevel,
			W.Stats.MagazineSize, W.Stats.ReloadTime, *W.Stats.ReloadType,
			W.Stats.CanReload ? TEXT("true") : TEXT("false"),
			W.Stats.WeaponReqMana, W.Stats.ManaPerAmmo,
			W.Stats.VerticalRecoilMin, W.Stats.VerticalRecoilMax,
			W.Stats.HorizontalRecoilMin, W.Stats.HorizontalRecoilMax);
	}
	UE_LOG(LogTemp, Log, TEXT("========================================================"));
}
