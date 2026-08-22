// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Weapons/WeaponData.h"
#include "MainWeaponComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTRWW_API UMainWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMainWeaponComponent();

	// 캐릭터(소유자)가 발사 입력을 받으면 이 함수를 호출한다.
	void StartFire();

	// 재장전 입력을 받으면 호출한다.
	void RequestReload();

	// weapons.json에서 전체 스탯을 채우고 탄약을 세팅한다. SavedAmmo가 음수면 탄창을 가득
	// 채운 채로 시작한다(최초 장착용). 인벤토리 등 외부에서 무기를 바꿔 낄 때도 이 함수를
	// 그대로 재사용한다. 서버에서만 호출되어야 한다.
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipWeapon(FName NewWeaponIndex, int32 SavedAmmo);

	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetCurrentAmmo() const { return CurrentAmmo; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetMagazineSize() const { return MagazineSize; }

protected:
	virtual void BeginPlay() override;

	// 클라이언트 -> 서버 요청: "이 방향으로 쐈다". 실제 판정은 서버가 한다.
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFire(const FVector_NetQuantize& TraceStart, const FVector_NetQuantizeNormal& TraceDirection);

	// 서버 -> 전체 클라이언트: 판정 결과에 따른 이펙트만 통보.
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayFireEffects(const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& TraceEnd, bool bHit);

	// 클라이언트 -> 서버 요청: 재장전.
	UFUNCTION(Server, Reliable)
	void Server_Reload();

	UFUNCTION()
	void OnRep_CurrentAmmo();

	// weapons.json의 "index"와 매칭되는 키.
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName WeaponIndex;

	// --- FWeaponItem 최상위 필드 ---
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName WeaponType;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName Rarity;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FLocalizedPair Name;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FLocalizedPair Description;

	// --- FWeaponStats 필드 (선언 순서 그대로) ---
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float Damage = 20.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	int32 PelletCount = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float HeadshotMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float FireRate_RPS = 6.67f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName FireMode;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	int32 BurstCount = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float BurstShotInterval = 0.0f;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Weapon")
	int32 MagazineSize = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float ReloadTime = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	bool CanReload = true;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float WeaponReqMana = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float ManaPerAmmo = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float MaxRange = 10000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float DamageFalloffStart = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float DamageFalloffEnd = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float DamageFalloffMin = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	bool IsHitscan = true;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float ProjectileSpeed = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	bool CanADS = true;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float ScopeZoomLevel = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float SpreadHipfire = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float SpreadADS = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float SpreadIncreasePerShot = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float MaxSpreadBloomHipfire = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float MaxSpreadBloomADS = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float SpreadRecoveryDelay = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float SpreadRecoveryRate = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float RecoilVertical = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float RecoilHorizontal = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float ADSSpeed = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float ADSMoveSpeedMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float MoveSpeedMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float EquipTime = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float PelletSpreadAngle = 0.0f;

	// --- 런타임 상태 ---
	UPROPERTY(ReplicatedUsing = OnRep_CurrentAmmo, VisibleAnywhere, Category = "Weapon")
	int32 CurrentAmmo = 0;

	double LastFireTimeSeconds = 0.0;
	double EquippedTimeSeconds = 0.0;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
