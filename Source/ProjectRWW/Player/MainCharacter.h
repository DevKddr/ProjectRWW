// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MainCharacter.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

// 팩의 Gait 값과 동일 규칙(0=Idle, 1=Walk, 2=Sprint). ReceiveMovementChange()에
// 넘길 때만 float으로 변환한다.
UENUM(BlueprintType)
enum class EMovementStatus : uint8
{
	Idle,
	Walk,
	Sprint
};

UCLASS()
class PROJECTRWW_API AMainCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMainCharacter();

	// PlayerState의 AttributeSet에서 이동 스탯을 가져온다. 이동/HP/Mana 등 모든 스탯이
	// GAS로 통합되면서, 캐릭터 자신은 더 이상 이 값들을 로컬로 들고 있지 않는다.
	UFUNCTION(BlueprintPure, Category = "GAS")
	class UMainAttributeSet* GetMainAttributeSet() const;

	// 지금 달리기(Sprint) 중인지. MaxWalkSpeed로 판단 - 별도 상태 변수 없이
	// 이미 있는 값을 재사용한다. 구현은 cpp에 있다 - UMainAttributeSet의 멤버 함수를
	// 호출하려면 완전한 타입 정의가 필요한데, 헤더에서는 전방선언만 해뒀기 때문이다.
	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsSprinting() const;

	// 다른 클라이언트에게 "지금 이 캐릭터가 Idle/Walk/Sprint 중 뭘 보여줘야 하는지"를
	// 전달하는 리플리케이트 값. bSprintRequested(입력 의도)와 실제 이동 여부를
	// 서버가 합쳐서 계산한다 - 나중에 이동속도에 배율이 붙어도 이 판단 로직은 안 바뀐다.
	UPROPERTY(ReplicatedUsing = OnRep_MovementChange)
	EMovementStatus MovementStatus = EMovementStatus::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<class UMainWeaponComponent> WeaponComponent;

	// 아래 이벤트들은 Kinemation Tactical Shooter Pack 애니메이션 컴포넌트를 실제로
	// 구동하는 지점이다. C++은 "언제 뭐가 바뀌었는지"만 알려주고, 그 값으로 서드파티
	// BP 컴포넌트(AC_RecoilAnimation 등)의 함수를 부르는 건 BP_MainCharacter 쪽
	// 이벤트 구현부가 담당한다 - C++이 서드파티 BP 전용 클래스 타입을 몰라도 되게 하기 위함.
	// 게임플레이 로직에는 전혀 관여하지 않는 순수 표시(애니메이션) 전용 훅이라, 이
	// 이벤트들의 구현부를 전부 비워놔도 게임 자체는 원래대로 동작한다.

	// 총알 한 발이 실제로 나갈 때마다 호출된다 (버튼 누름이 아니라 실제 발사 시점).
	// RequestFire()가 쏘는 클라이언트 본인 컴퓨터에서 로컬로 실행하므로, 이 이벤트도
	// 본인 화면에서만 실행된다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
	void ReceiveWeaponFire();

	// 발사 입력을 뗐을 때(연사 중단) 호출된다. 본인 클라이언트에서만 실행된다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
	void ReceiveWeaponFireStopped();

	// 조준(ADS) 시작/종료 시 호출된다. 본인 클라이언트에서만 실행된다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
	void ReceiveADSChange(bool bIsAiming);

	// 무기를 장착해서 발사모드가 확정될 때 호출된다. weapons.json의 FireMode 값
	// ("FullAuto"/"Burst"/"Semi" 등)을 그대로 전달한다. EquipWeapon()이 서버와
	// 모든 클라이언트에서 각자 실행되므로, 이 이벤트도 서버+전체 클라이언트에서 호출된다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
	void ReceiveFireModeChange(FName NewFireMode);

	// 이동 상태가 바뀔 때 호출된다. 0=Idle, 1=Walk, 2=Sprint (팩의 Gait 값과 동일 규칙).
	UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
	void ReceiveMovementChange(float Gait);

	// 무기를 새로 장착했을 때 호출된다. BP 쪽에서 이 값으로 무기별 리코일/IK 데이터
	// 애셋을 찾아 AC_RecoilAnimation::Init() 등에 넘겨준다. ReceiveFireModeChange와
	// 같은 이유로 서버+전체 클라이언트에서 호출된다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
	void ReceiveWeaponEquip(FName ItemIndex);

	// 무기가 아닌 아이템(빈손 포함, index "UNARMED")을 장착했을 때 호출된다.
	// ReceiveWeaponEquip과 대칭 구조 - BP 쪽에서 아이템 Actor를 스폰/부착하고
	// 그 Actor의 Draw()를 부르는 역할을 한다. ReceiveWeaponEquip과 같은 이유로
	// 서버+전체 클라이언트에서 호출된다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
	void ReceiveItemEquip(FName ItemIndex);

	// 재장전 입력을 받으면 호출한다. 재장전이 시작되는 시점에 알려주는 역할.
	// 본인 클라이언트는 RequestReload()에서 즉시(서버 응답을 기다리지 않고) 로컬
	// 예측으로 호출되고, 다른 클라이언트는 bIsReloading 리플리케이션
	// (OnRep_IsReloading)을 통해 호출된다 - ReceiveADSChange와 같은 패턴.
	UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
	void ReceiveReloadStart();

	// 지금 손에 부착되어 있는 비무기 아이템 Actor(BP_Unarmed 등)를 반환한다.
	// WeaponComponent의 ActiveHandActor(무기/아이템 공용 슬롯)를, 무기가 장착 중이
	// 아닐 때만 반환한다 - GetMainWeapon()과 대칭되는 필터링. ActiveHandActor는
	// 원래 MainInventoryComponent(PlayerController 위)에 있었지만, PlayerController는
	// 오너 클라이언트에게만 리플리케이트되어 다른 플레이어가 이 값을 못 봐서
	// MainWeaponComponent(캐릭터 위) 쪽으로 옮겼다.
	UFUNCTION(BlueprintPure, Category = "Item")
	AActor* GetMainItem() const;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

	// PlayerState의 ASC에 자신을 Avatar로 연결하고, 사망/이동속도 동기화 델리게이트를 건다.
	// PossessedBy(서버)/OnRep_PlayerState(클라이언트) 양쪽에서 호출된다 - GAS 공식 패턴.
	void InitAbilitySystem();

	void SyncMovementSpeedFromAttributes(bool bSprinting);

	// UMainAttributeSet::OnDeath에 바인딩되는 콜백. 실제 처리는 GameMode에 위임한다.
	UFUNCTION()
	void HandleAttributeDeath(AActor* Avatar, AController* Killer);

	// UMainAttributeSet::OnMovementAttributesChanged에 바인딩되는 콜백. 클라이언트에서
	// 이동 Attribute(WalkSpeed/RunSpeed/JumpPower)가 늦게 리플리케이트되어 도착해도
	// 이 콜백이 재동기화를 트리거해 스스로 회복시킨다.
	UFUNCTION()
	void HandleMovementAttributesChanged();

	// AActor::OnTakeAnyDamage에 바인딩되는 콜백. GE_Damage/GE_DamagedTag를 적용해
	// GAS 쪽 Health를 깎는다. 이름에 _GAS를 붙인 이유는 AActor의 OnTakeAnyDamage
	// 델리게이트 멤버와 이름이 겹치면 이름 가리기(name hiding)로 컴파일이 깨지기 때문이다.
	UFUNCTION()
	void OnTakeAnyDamage_GAS(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	// IA_Move가 들어오는 동안 계속 호출
	void OnMove(const FInputActionValue& Value);
	// IA_Look이 들어오는 동안 계속 호출
	void OnLook(const FInputActionValue& Value);

	void OnSprintStart(const FInputActionValue& Value);
	void OnSprintStop(const FInputActionValue& Value);

	// 이동 입력이 끊겼을 때(IA_Move가 완전히 손을 뗀 순간) 호출된다.
	void OnMoveStopped(const FInputActionValue& Value);

	// 지금 이동 입력이 들어오고 있는지. OnMove/OnMoveStopped가 관리한다.
	bool bIsMoving = false;

	// 스프린트 키를 누르고 있으려는 "입력 의도". 실제 결과 속도(MaxWalkSpeed)와
	// 별개로 유지해서, 나중에 조준/디버프 등으로 속도에 배율이 붙어도 이 값은
	// 오염되지 않는다 - MovementStatus 계산의 유일한 판단 기준이 된다.
	bool bSprintRequested = false;

	// MovementStatus가 리플리케이트되어 도착하면 호출된다. 본인은 이미 로컬 예측으로
	// 재생했으니 원격 클라이언트일 때만 ReceiveMovementChange()를 호출한다
	// (OnRep_IsReloading과 같은 패턴).
	UFUNCTION()
	void OnRep_MovementChange();

	virtual void Tick(float DeltaSeconds) override;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

	// 클라이언트 요청을 받아 서버 쪽 이동 속도를 실제로 바꾸는 함수. 속도 변경은 항상 이 함수를 거처야 한다.
	UFUNCTION(Server, Reliable)
	void ServerSetSprinting(bool bNewSprinting);

	void OnFire(const FInputActionValue& Value);
	void OnStopFire(const FInputActionValue& Value);
	void OnReload(const FInputActionValue& Value);
	void OnADSStart(const FInputActionValue& Value);
	void OnADSStop(const FInputActionValue& Value);

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ReloadAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ADSAction;
};
