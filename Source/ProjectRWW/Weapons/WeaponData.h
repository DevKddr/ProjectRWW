#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "WeaponData.generated.h"

// weapons.json 각 항목의 "stats" 객체와 1:1 대응.
USTRUCT(BlueprintType)
struct FWeaponStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float Damage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 PelletCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float HeadshotMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float FireRate_RPS = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	FName FireMode;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 BurstCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float BurstShotInterval = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 MagazineSize = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float ReloadTime = 0.f;

	// 탄창이 완전히 비었을 때의 재장전 소요 시간(초). 일반 ReloadTime과 별도로 관리 -
	// 슬라이드/볼트 릴리즈 등 추가 동작이 붙어 보통 더 길다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|ReloadAnim")
	float ReloadTime_Empty = 0.f;

	// 재장전 시작 애니메이션 출력 시간(초).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|ReloadAnim")
	float ReloadTime_Start = 0.f;

	// ReloadType=Single(한 발씩 장전)일 때, 한 발 삽입 애니메이션의 루프 시간(초).
	// Magazine 재장전 무기는 반복 동작이 없어 0.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|ReloadAnim")
	float ReloadTime_Loop = 0.f;

	// 재장전 완료 후 마무리 애니메이션 출력 시간(초).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|ReloadAnim")
	float ReloadTime_End = 0.f;

	// 장전 가능 여부. FALSE면 장전 자체가 불가능 (탄창 교체 없음).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	bool CanReload = true;

	// 장전 시 필요한 마나(기본 요구치). CanReload=FALSE여도 값은 유지(추후 변경 대비).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float WeaponReqMana = 0.f;

	// 탄 1발당 요구 마나 감소분.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float ManaPerAmmo = 0.f;

	// 장전 방식: "Single"(한 발씩) 또는 "Magazine"(탄창째로 한번에). 값 타입은 요청대로 문자열.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	FString ReloadType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float MaxRange = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float DamageFalloffStart = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float DamageFalloffEnd = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float DamageFalloffMin = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	bool IsHitscan = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float ProjectileSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	bool CanADS = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float ScopeZoomLevel = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float SpreadHipfire = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float SpreadADS = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float SpreadIncreasePerShot = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float MaxSpreadBloomHipfire = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float MaxSpreadBloomADS = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float SpreadRecoveryDelay = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float SpreadRecoveryRate = 0.f;

	// 사격 1회당 수직/수평 반동을 [Min, Max] 범위에서 랜덤하게 뽑아 적용한다.
	// 이전엔 RecoilVertical/RecoilHorizontal 단일값(미사용)이었으나, 사격마다 편차를
	// 주기 위해 최소/최대 범위로 대체했다. Horizontal은 음수=좌측, 양수=우측.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|Recoil")
	float VerticalRecoilMin = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|Recoil")
	float VerticalRecoilMax = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|Recoil")
	float HorizontalRecoilMin = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|Recoil")
	float HorizontalRecoilMax = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float ADSSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float ADSMoveSpeedMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float MoveSpeedMultiplier = 1.f;

	// ShotGun 전용 (그 외 무기는 0)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|ShotGun")
	float PelletSpreadAngle = 0.f;
};

// weapons.json 배열의 각 항목과 1:1 대응.
// Rarity/Name/Description은 items.json(ItemDataManager)으로 표시 책임이 옮겨가서
// 여기선 더 이상 들고 있지 않는다 - 같은 Index로 ItemDataManager를 조회해야 한다.
USTRUCT(BlueprintType)
struct FWeaponItem : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName Index;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName WeaponType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FWeaponStats Stats;
};
