#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "WeaponData.generated.h"

// weapons.json 각 항목의 "name"/"description" ({"ko":.., "en":..}) 과 1:1 대응.
USTRUCT(BlueprintType)
struct FLocalizedPair
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Localization")
	FString Ko;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Localization")
	FString En;
};

// weapons.json 각 항목의 "stats" 객체와 1:1 대응.
USTRUCT(BlueprintType)
struct FWeaponStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	FName WeaponType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	FName Rarity;

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

	// 예약 필드 (현재 미사용).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|Reserved")
	float RecoilVertical = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|Reserved")
	float RecoilHorizontal = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float ADSSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float ADSMoveSpeedMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float MoveSpeedMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float EquipTime = 0.f;

	// ShotGun 전용 (그 외 무기는 0)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|ShotGun")
	float PelletSpreadAngle = 0.f;
};

// weapons.json 배열의 각 항목과 1:1 대응.
USTRUCT(BlueprintType)
struct FWeaponItem : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName Index;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName WeaponType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName Rarity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FLocalizedPair Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FLocalizedPair Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FWeaponStats Stats;
};
