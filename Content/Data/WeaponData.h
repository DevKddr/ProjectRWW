#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "WeaponData.generated.h"

// Weapon.json 한 항목과 1:1 대응. 모든 무기 타입이 이 구조체 하나를 공유한다.
USTRUCT(BlueprintType)
struct FWeaponData : public FTableRowBase
{
	GENERATED_BODY()

	// ---------- 공통 필드 ----------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName Index;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName WeaponType;

	// Rarity 마스터(FRarityData)의 RarityID를 참조. 판매가는 여기서 유도됨(개별 값 없음).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName Rarity;

	// 실제 텍스트가 아니라 Localization_{lang}.json을 참조하는 키.
	// ULocalizationManager::GetLocalizedText(NameKey)로 실제 표시 문자열을 얻는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName NameKey;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName DescKey;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FString Category;

	// 발당(펠릿당) 데미지. 총 데미지 = Damage x PelletCount.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float Damage = 0.f;

	// 한 번 발사에 판정되는 펠릿(투사체) 수. 일반 무기는 1, ShotGun만 1보다 큼.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 PelletCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float HeadshotMultiplier = 1.f;

	// 초당 발사 횟수(RPS). 발사 간격(초) = 1 / FireRate_RPS.
	// FireMode=Burst인 경우: 버스트와 버스트 사이 재사격 간격을 의미 (버스트 내부 간격은 BurstShotInterval).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float FireRate_RPS = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 MagazineSize = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float ReloadTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float MaxRange = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float DamageFalloffStart = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float DamageFalloffEnd = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float DamageFalloffMin = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool IsHitscan = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float ProjectileSpeed = 0.f;

	// 조준(ADS) 가능 여부. FALSE면 이 무기는 조준 자체가 불가능 (예: 추후 추가될 일부 근접무기).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool CanADS = true;

	// 조준경/가늠자 배율. 조준경이 없는 무기는 1.0(배율 없음).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float ScopeZoomLevel = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float SpreadHipfire = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float SpreadADS = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float SpreadIncreasePerShot = 0.f;

	// 무조준 상태에서 탄퍼짐이 도달할 수 있는 최대치.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float MaxSpreadBloomHipfire = 0.f;

	// 조준 상태에서 탄퍼짐이 도달할 수 있는 최대치 (보통 Hipfire보다 작음).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float MaxSpreadBloomADS = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float SpreadRecoveryDelay = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float SpreadRecoveryRate = 0.f;

	// 예약 필드 (현재 미사용). 추후 카메라/에임 이동형 반동 전환 시 사용.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Reserved")
	float RecoilVertical = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Reserved")
	float RecoilHorizontal = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float ADSSpeed = 0.f;

	// 조준(ADS) 중 이동속도 배율. MoveSpeedMultiplier(장착 중 기본 이동속도)와 별개로 조준 시 추가 적용.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float ADSMoveSpeedMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float MoveSpeedMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float EquipTime = 0.f;

	// 발사 방식: Single / Burst / FullAuto
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName FireMode;

	// FireMode=Burst일 때만 의미 있는 값. 그 외 FireMode에서는 0.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 BurstCount = 0;

	// Burst 내부에서 한 발과 다음 발 사이의 간격(초). FireRate_RPS(버스트간 간격)와 별개. 그 외에는 0.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float BurstShotInterval = 0.f;

	// ---------- 타입 전용 필드 (해당 타입이 아니면 기본값) ----------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|ShotGun")
	float PelletSpreadAngle = 0.f;
};

// WeaponTypeData.json 한 항목과 1:1 대응 (타입 메타데이터)
USTRUCT(BlueprintType)
struct FWeaponTypeData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponType")
	FName TypeID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponType")
	FString Category;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponType")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponType")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponType")
	FString IconPath;
};

// Rarity.json 한 항목과 1:1 대응 (등급 메타데이터, 판매가의 유일한 출처)
USTRUCT(BlueprintType)
struct FRarityData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity")
	FName RarityID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity")
	FString Color;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity")
	float DropWeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity")
	int32 SellValue = 0;
};
