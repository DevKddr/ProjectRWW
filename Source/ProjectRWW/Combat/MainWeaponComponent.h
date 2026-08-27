// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Weapons/WeaponData.h"
#include "TimerManager.h"
#include "MainWeaponComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTRWW_API UMainWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMainWeaponComponent();

	// 캐릭터(소유자)가 발사 입력을 받으면 이 함수를 호출한다.
	void StartFire();

	// 발사 입력을 뗐을 때 호출한다. FullAuto의 반복 발사를 멈추는 용도.
	void StopFire();

	// 재장전 입력을 받으면 호출한다.
	void RequestReload();

	// weapons.json에서 전체 스탯을 채우고 탄약을 세팅한다. SavedAmmo가 음수면 탄창을 가득
	// 채운 채로 시작한다(최초 장착용). 인벤토리 등 외부에서 무기를 바꿔 낄 때도 이 함수를
	// 그대로 재사용한다. 서버에서만 호출되어야 한다.
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipWeapon(FName NewWeaponIndex, int32 SavedAmmo);

	// 손에서 무기를 완전히 내린다(빈손 상태). 인벤토리에서 아이템을 다른 슬롯으로
	// 옮기거나 새 무기로 교체하기 직전에 호출된다.
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void UnequipWeapon();

	// 빈손이면 발사/재장전/조준 요청을 무시하기 위한 가드용.
	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool HasWeaponEquipped() const { return WeaponIndex != NAME_None; }

	// FirstPersonMesh가 블루프린트(BP_MainCharacter) 쪽에서 만들어진 컴포넌트라 C++이
	// 직접 만들거나 이름으로 찾을 수 없어서, 이 참조를 블루프린트가 채워주게 한다.
	// BP_MainCharacter의 컨스트럭션 스크립트에서 "Set Weapon Mesh Component"로
	// FirstPersonMesh를 지정해줘야 한다 - 안 하면 EquipWeapon()이 메쉬를 못 바꾼다.
	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	TObjectPtr<class USkeletalMeshComponent> WeaponMeshComponent;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetCurrentAmmo() const { return CurrentAmmo; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetMagazineSize() const { return MagazineSize; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	FName GetWeaponIndex() const { return WeaponIndex; }

	// 조준(ADS) 입력을 받으면 호출한다.
	void StartADS();

	// 조준 해제 입력을 받으면 호출한다.
	void StopADS();

	// 지금 조준 중인지. 블루프린트에서 조준 시 카메라 정렬 연출을 트리거하는 데 쓴다.
	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsAiming() const { return bIsAiming; }

	// 조준 완료까지 걸리는 시간(초). 이름은 Speed지만 weapons.json 값은 시간(초) 단위로 쓰인다.
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetADSSpeed() const { return ADSSpeed; }

	// 지금 재장전 중인지. HUD에서 재장전 UI 표시 여부에 쓴다.
	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsReloading() const { return bIsReloading; }

	// 재장전 진행률(0.0~1.0). HUD 프로그레스 바에 그대로 연결해서 쓸 수 있다.
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetReloadProgress() const;

	// 지금 이 순간 기준으로 예상되는 산포각(도 단위). 실제 상태(CurrentSpreadDegrees)는
	// 발사할 때만 갱신되므로, 이 함수는 그 사이 시간에 대한 회복분을 매번 다시 계산해서
	// 보여준다(크로스헤어가 쏘지 않는 동안에도 서서히 좁아지는 것처럼 보이게 하기 위함).
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetCurrentSpreadDegrees() const;

	// 지금 조준 여부 기준으로 낼 수 있는 최대 산포각(기준 산포 + 최대 블룸).
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetMaxSpreadDegrees() const;

protected:
	// 이번 발의 산포각을 계산하고, 다음 발을 위해 블룸을 누적한다. 서버(FireShot)와
	// 원격 클라이언트(RequestFire) 각자 자기 상태로 이 함수를 호출한다.
	float UpdateSpread();

	// Stats의 각 필드를 로컬 멤버 변수에 그대로 반영한다. EquipWeapon()에서 실제 무기
	// 데이터로, UnequipWeapon()에서는 빈 FWeaponStats()로 호출해서 초기화 용도로도 쓴다.
	void ApplyWeaponStats(const FWeaponStats& Stats);

	virtual void BeginPlay() override;

	// ReloadTime초 뒤에 실제로 탄창을 채우는 타이머 콜백.
	void CompleteReload();

	// bIsReloading이 복제되어 도착하면 호출된다. 서버 시각을 그대로 믿지 않고,
	// 클라이언트가 소식을 받은 그 순간을 자기 시계로 다시 찍어서 기준으로 삼는다.
	UFUNCTION()
	void OnRep_IsReloading();

	// 조준 정보를 캡처해서 서버에 발사 요청(RPC)을 보낸다. FullAuto 반복 타이머의 콜백으로도 쓰인다.
	void RequestFire();

	// 실제로 총알 한 발을 처리한다(레이트레이스, 데미지, 이펙트 통보). 모드와 무관하게 공통으로 쓰인다.
	void FireShot(const FVector_NetQuantize& TraceStart, const FVector_NetQuantizeNormal& TraceDirection);

	// 버스트 모드에서 첫 발 이후 나머지 탄을 쏘는 타이머 콜백.
	void FireBurstShot();

	// 클라이언트 -> 서버 요청: "이 방향으로 쐈다". 실제 판정은 서버가 한다.
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFire(const FVector_NetQuantize& TraceStart, const FVector_NetQuantizeNormal& TraceDirection);

	// 서버 -> 전체 클라이언트: 판정 결과에 따른 이펙트만 통보.
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayFireEffects(const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& TraceEnd, bool bHit);

	// 클라이언트 -> 서버 요청: 재장전.
	UFUNCTION(Server, Reliable)
	void Server_Reload();

	// 클라이언트 -> 서버 요청: 조준 상태 변경. Sprint와 같은 패턴.
	UFUNCTION(Server, Reliable)
	void Server_SetAiming(bool bNewAiming);

	UFUNCTION()
	void OnRep_CurrentAmmo();

	// WeaponIndex가 복제되어 도착하면 클라이언트가 스스로 EquipWeapon()을 다시 실행해
	// 로컬 상태(스탯, FullAuto/Burst 타이머 정리)를 서버와 맞춘다.
	UFUNCTION()
	void OnRep_WeaponIndex();

	// weapons.json의 "index"와 매칭되는 키.
	UPROPERTY(ReplicatedUsing = OnRep_WeaponIndex, EditDefaultsOnly, Category = "Weapon")
	FName WeaponIndex;

	// --- FWeaponItem 최상위 필드 ---
	// Rarity/Name/Description은 인벤토리 서브프로젝트의 ItemDataManager로 표시 책임이
	// 옮겨가서(같은 Index로 조회) 여기선 더 이상 들고 있지 않는다. 아무도 읽어가는 곳이
	// 없었던 걸 확인하고 제거함.
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName WeaponType;

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

	// FullAuto 반복 발사용 타이머 핸들.
	FTimerHandle AutoFireTimerHandle;

	// 버스트 나머지 탄 발사용 타이머 핸들.
	FTimerHandle BurstTimerHandle;

	// 버스트 진행 중 남은 탄 수.
	int32 PendingBurstShotsRemaining = 0;

	// --- 산포(Spread) 런타임 상태 --- (서버/클라이언트 각자 로컬로 관리, 복제 안 함)
	float CurrentSpreadDegrees = 0.0f;
	double LastSpreadUpdateTimeSeconds = 0.0;

	// 조준 중인지. 서버는 산포(Spread) 계산에, 클라이언트는 블루프린트의 조준 연출에 사용한다.
	// 양쪽이 각자 독립적으로 값을 들고 있어서 복제가 필요 없다(Sprint의 MaxWalkSpeed와 같은 패턴).
	bool bIsAiming = false;

	// 지금 재장전 중인지. 클라이언트 UI 표시용으로 써야 하므로 복제한다.
	UPROPERTY(ReplicatedUsing = OnRep_IsReloading)
	bool bIsReloading = false;

	// 재장전을 시작한 시각. 서버/클라이언트 시계가 정확히 동기화되어 있지 않아서
	// 복제하지 않고, 각자 자기 시계로 이 값을 따로 찍는다(LastFireTimeSeconds와 같은 정책).
	float ReloadStartTimeSeconds = 0.0f;

	// ReloadTime초 뒤 CompleteReload를 호출하는 타이머 핸들.
	FTimerHandle ReloadTimerHandle;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
