// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Weapons/WeaponData.h"
#include "TimerManager.h"
#include "MainWeaponComponent.generated.h"

class AMainCharacter;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTRWW_API UMainWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMainWeaponComponent();

	// 캐릭터(소유자)가 발사 입력을 받으면 이 함수를 호출한다.
	void StartFire();

	// 발사 입력을 뗐을 때 호출한다. FullAuto의 반복 발사를 멈추는 용도.
	// bCancelBurst: true면 진행 중인 버스트의 남은 발까지(ClientBurstTimerHandle) 강제로
	// 끊는다 - 재장전/무기교체처럼 진행 중이던 걸 확실히 다 끊어야 하는 경우용. 단순히
	// 발사 버튼을 뗀 경우(OnStopFire)엔 false로 불러야 한다 - Burst는 트리거를 일찍
	// 떼도 이미 시작된 버스트가 끝까지 나가야 하는데, 여기서 무조건 끊으면 서버(실제
	// 발사)는 계속 이어지는데 본인 화면의 나머지 발 애니메이션만 끊기는 불일치가 생긴다.
	void StopFire(bool bCancelBurst = true);

	// 재장전 입력을 받으면 호출한다.
	void RequestReload();

	// weapons.json에서 전체 스탯을 채우고 탄약을 세팅한다. SavedAmmo가 음수면 탄창을 가득
	// 채운 채로 시작한다(최초 장착용). 인벤토리 등 외부에서 무기를 바꿔 낄 때도 이 함수를
	// 그대로 재사용한다. 서버에서만 호출되어야 한다.
	// SlotIndex: 이 무기가 인벤토리 몇 번 슬롯에서 왔는지. 같은 무기를 다른 슬롯으로
	// 바꿔 낄 때도 이 값이 바뀌므로, 리모트 클라이언트에게 "장착이 다시 일어났다"는
	// 걸 정확히 알리는 용도로 쓴다(WeaponIndex만으로는 같은 무기면 리플리케이션이
	// "값 안 바뀜"으로 판단해서 스킵해버린다).
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipWeapon(FName NewWeaponIndex, int32 SavedAmmo, int32 SlotIndex = -1);

	// 손에서 무기를 완전히 내린다(빈손 상태). 인벤토리에서 아이템을 다른 슬롯으로
	// 옮기거나 새 무기로 교체하기 직전에 호출된다.
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void UnequipWeapon();

	// 서버 -> 전체 클라이언트: 각자 자기 로컬 무기 Actor를 지금 당장 지우라고 알린다.
	// 폰 파괴 직전(사망 등)처럼 프로퍼티 리플리케이션(WeaponIndex/EquippedSlotIndex)이
	// 전송될 시간도 없이 폰이 사라지는 상황을 위한 것 - Reliable이라 반드시 도착한다.
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_DestroyWeaponActor();

	// 클라이언트 -> 서버 요청: 발사 버튼을 뗐다. 원격 클라이언트에게 반동 애니메이션
	// 정지를 알리기 위한 용도(실제 발사 중단 자체는 서버가 ServerFire RPC가 더 이상
	// 안 오는 것으로 이미 알고 있음).
	UFUNCTION(Server, Reliable)
	void Server_StopFire();

	// 서버 -> 전체 클라이언트: 발사가 멈췄음을 통보. 원격 클라이언트일 때만
	// ReceiveWeaponFireStopped()를 호출한다(본인은 이미 로컬에서 처리함).
	UFUNCTION(NetMulticast, Reliable)
	void MulticastWeaponFireStopped();

	// 빈손이면 발사/재장전/조준 요청을 무시하기 위한 가드용.
	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool HasWeaponEquipped() const { return WeaponIndex != NAME_None; }

	// FirstPersonMesh가 블루프린트(BP_MainCharacter) 쪽에서 만들어진 컴포넌트라 C++이
	// 직접 만들거나 이름으로 찾을 수 없어서, 이 참조를 블루프린트가 채워주게 한다.
	// BP_MainCharacter의 컨스트럭션 스크립트에서 "Set Weapon Mesh Component"로
	// FirstPersonMesh를 지정해줘야 한다 - 안 하면 EquipWeapon()이 메쉬를 못 바꾼다.
	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	TObjectPtr<class USkeletalMeshComponent> WeaponMeshComponent;

	// 지금 캐릭터 손에 부착되어 있는 Actor - 무기든 비무기 아이템이든 상관없이 이
	// 슬롯 하나로 관리한다. 둘은 항상 배타적이라(동시에 존재하지 않음) 별도 변수로
	// 나눠뒀던 게 "한쪽만 갱신되고 반대쪽이 안 지워지는" 불일치 버그의 원인이었다 -
	// 슬롯을 하나로 합치면 EquipVisual()이 항상 "이 슬롯에 이미 있던 걸 지우고 새 걸
	// 넣는" 방식이라 그런 불일치 자체가 구조적으로 불가능해진다. 지금 무기인지
	// 아이템인지는 WeaponIndex/ActiveItemIndex 중 뭐가 NAME_None이 아닌지로 판단한다.
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<AActor> ActiveHandActor;

	// BP의 GetMainWeapon()/GetPrimaryWeapon()이 예전엔 ActiveWeaponActor 프로퍼티를
	// 자동 게터로 직접 읽었는데, 그 프로퍼티가 없어졌으니 같은 이름의 함수로 대체한다.
	// 무기가 실제로 장착 중일 때만 반환 - ActiveHandActor가 지금 아이템 용도로 쓰이고
	// 있을 수도 있어서 WeaponIndex로 먼저 걸러야 한다.
	UFUNCTION(BlueprintPure, Category = "Weapon")
	AActor* GetActiveWeaponActor() const { return HasWeaponEquipped() ? ActiveHandActor.Get() : nullptr; }

	// 비무기 아이템을 스폰/부착하고 ReceiveItemEquip()을 호출한다. EquipWeapon()과
	// 완전히 같은 패턴 - 이 컴포넌트가 Character 위에 있어서 ActiveItemIndex가
	// 리플리케이트되면 모든 클라이언트(오너 포함 다른 리모트 클라이언트까지)에서
	// 이 함수가 재실행된다. 서버에서만 호출되어야 한다.
	UFUNCTION(BlueprintCallable, Category = "Item")
	void EquipItemVisual(FName ItemIndex);

	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetCurrentAmmo() const { return CurrentAmmo; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetMagazineSize() const { return MagazineSize; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	FName GetWeaponIndex() const { return WeaponIndex; }

	// weapons.json의 FireMode 값("Single"/"Burst"/"FullAuto")을 그대로 반환한다.
	// GetFireMode()는 이 값을 E_FireMode(Kinemation 블루프린트 이넘)로 변환하는 역할을 한다.
	UFUNCTION(BlueprintPure, Category = "Weapon")
	FName GetFireMode() const { return FireMode; }

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

	// 클라이언트 예측용: 서버의 FireBurstShot()과 같은 타이밍으로 Burst 나머지 발의
	// 애니메이션만 재생한다(실제 판정은 이미 서버가 처리하므로 여기선 손대지 않는다).
	void PlayClientBurstShot();

	// 무기/아이템 공용: ActorClass를 스폰해서 손(WeaponMeshComponent 부착점)에 붙이고,
	// 이전에 붙어있던 액터는 지운다. EquipWeapon()과 EquipItemVisual()이 완전히 같은
	// 스폰/부착/파괴 로직을 갖고 있어서 공용으로 뺐다 - 둘 다 같은 슬롯(ActiveHandActor)을
	// OutActiveActor로 넘기므로, 어느 쪽이 실행되든 이전에 있던 게 무기든 아이템이든
	// 상관없이 항상 정리된다.
	void EquipVisual(UClass* ActorClass, TObjectPtr<AActor>& OutActiveActor);

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

	// bIsAiming이 리플리케이트되어 도착하면 호출된다. 본인은 이미 로컬 예측으로
	// 재생했으니 원격 클라이언트일 때만 ReceiveADSChange()를 호출한다.
	UFUNCTION()
	void OnRep_IsAiming();

	// ReplicationSequence가 복제되어 도착하면 클라이언트가 스스로 EquipWeapon() 또는
	// EquipItemVisual()을 재실행해 로컬 상태를 서버와 맞춘다. 무기/아이템 둘 다 이
	// 하나의 트리거를 공유한다 - 각자 따로 트리거(OnRep_ActiveItemIndex 등)를 두면,
	// 무기 전환과 아이템 전환이 같은 프레임에 겹칠 때(예: 아이템<->무기 전환은 항상
	// WeaponIndex와 ActiveItemIndex가 같이 바뀐다) 원격 클라이언트에서 두 OnRep이
	// 서로 다른 순서로 도착해 방금 스폰된 액터를 반대쪽이 지워버리는 레이스가 있었다.
	// 하나로 합치면 HasWeaponEquipped()로 "지금 최종적으로 뭐가 맞는지"만 판단해서
	// 그쪽 함수 하나만 실행하므로 그런 레이스 자체가 불가능해진다.
	UFUNCTION()
	void OnRep_ReplicationSequence();

	// weapons.json의 "index"와 매칭되는 키. 리플리케이트는 되지만, 값 변경 알림은
	// ReplicationSequence가 대신 담당한다(같은 무기 재장착 시에도 놓치지 않기 위해).
	UPROPERTY(Replicated, EditDefaultsOnly, Category = "Weapon")
	FName WeaponIndex;

	// 이 무기가 인벤토리 몇 번 슬롯에서 왔는지. -1이면 빈손. 트리거 역할은 더 이상
	// 이 값이 하지 않는다 - 아래 ReplicationSequence 참고.
	UPROPERTY(Replicated)
	int32 EquippedSlotIndex = -1;

	// EquipWeapon()이 호출될 때마다 무조건 증가한다. 같은 슬롯에 다른 무기를 낄 때처럼
	// EquippedSlotIndex가 한 번의 서버 RPC 안에서 왕복해서(예: 3 -> -1 -> 3) 최종적으로
	// 시작값과 같아지면, 프로퍼티 리플리케이션이 "값 변화 없음"으로 판단해 OnRep이
	// 원격 클라이언트에서 아예 안 뜨는 버그가 있었다 - WeaponIndex/CurrentAmmo는 각자
	// 독자적으로 리플리케이트되어 정상 반영되는데 무기 메쉬 교체/Draw 애니메이션만 안
	// 나오는 증상으로 나타났다. 이 값은 절대 왕복하지 않으므로(항상 증가만 함) 그런
	// 경우에도 리플리케이션이 반드시 변화를 감지한다.
	UPROPERTY(ReplicatedUsing = OnRep_ReplicationSequence)
	int32 ReplicationSequence = 0;

	// 지금 손에 든 비무기 아이템의 Index. NAME_None이면 무기가 장착된 상태(또는 아직
	// 아무것도 장착 안 된 초기 상태). 트리거 역할은 위 ReplicationSequence가 공용으로
	// 담당한다.
	UPROPERTY(Replicated)
	FName ActiveItemIndex;

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

	// "마지막으로 쏜 시각"이 아니라 "다음으로 발사가 허용되는 정확한 시각"을 저장한다.
	// 발사할 때마다 Now가 아니라 이 값에 간격을 더해서 갱신해야, 프레임 지터로 체크가
	// 한 번 밀려도 오차가 누적되지 않는다(자세한 이유는 ServerFire_Implementation 참고).
	double NextAllowedFireTimeSeconds = 0.0;

	// 서버 권위용 NextAllowedFireTimeSeconds와 별개로, 클라이언트 로컬 예측(애니메이션)에만
	// 쓰는 값. RequestFire()는 원격 클라이언트가 아니라 각자 자기 화면에서
	// 실행되므로, 서버가 RPC 안에서 갱신하는 NextAllowedFireTimeSeconds가 이 클라이언트
	// 인스턴스에는 반영되지 않는다 - 그래서 예측 전용으로 따로 하나 둔다.
	double NextAllowedPredictedFireTimeSeconds = 0.0;

	// 탄약이 0이 됐을 때 ReceiveWeaponFireStopped()를 한 번만 보내기 위한 플래그.
	// 트리거를 계속 누르고 있으면 RequestFire()가 매 틱 호출되는데, 그때마다 다시
	// 보내면 낭비이니 탄약이 다시 채워질 때(장착/재장전 완료)까지 한 번으로 제한한다.
	bool bAmmoEmptyNotified = false;

	double EquippedTimeSeconds = 0.0;

	// FullAuto 반복 발사용 타이머 핸들.
	FTimerHandle AutoFireTimerHandle;

	// 버스트 나머지 탄 발사용 타이머 핸들.
	FTimerHandle BurstTimerHandle;

	// 클라이언트 예측용 Burst 애니메이션 반복 타이머(위 BurstTimerHandle의 클라이언트 쪽 짝).
	FTimerHandle ClientBurstTimerHandle;

	// 위 타이머로 재생해야 할, 남은 예측 애니메이션 재생 횟수.
	int32 ClientBurstShotsRemaining = 0;

	// 버스트 진행 중 남은 탄 수.
	int32 PendingBurstShotsRemaining = 0;

	// --- 산포(Spread) 런타임 상태 --- (서버/클라이언트 각자 로컬로 관리, 복제 안 함)
	float CurrentSpreadDegrees = 0.0f;
	double LastSpreadUpdateTimeSeconds = 0.0;

	// 조준 중인지. 서버 권위 값이며, 리플리케이트되어 다른 클라이언트의 조준 애니메이션에도
	// 쓰인다(OnRep_IsAiming). 클라이언트는 StartADS()/StopADS()에서 로컬 예측으로 먼저
	// 바꿔두고, 나중에 서버 값이 도착하면 그걸로 덮어써진다.
	UPROPERTY(ReplicatedUsing = OnRep_IsAiming)
	bool bIsAiming = false;

	// 지금 재장전 중인지. 클라이언트 UI 표시용으로 써야 하므로 복제한다.
	UPROPERTY(ReplicatedUsing = OnRep_IsReloading)
	bool bIsReloading = false;

	// 재장전을 시작한 시각. 서버/클라이언트 시계가 정확히 동기화되어 있지 않아서
	// 복제하지 않고, 각자 자기 시계로 이 값을 따로 찍는다(NextAllowedFireTimeSeconds와 같은 정책).
	float ReloadStartTimeSeconds = 0.0f;

	// ReloadTime초 뒤 CompleteReload를 호출하는 타이머 핸들.
	FTimerHandle ReloadTimerHandle;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
