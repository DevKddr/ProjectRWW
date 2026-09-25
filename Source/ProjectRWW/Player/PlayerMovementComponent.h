// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PlayerMovementComponent.generated.h"

// 스프린트/조준 의도를 이동 패킷(saved move)의 압축 플래그에 실어 서버로 보낸다 - 서버가 같은 이동을
// 재현할 때 같은 프레임의 상태로 속도를 계산해서, 클라이언트/서버 속도 불일치(보정)를 없앤다.
// 속도 = 기본 속도(GAS 어트리뷰트, 버프 GE 포함) x 무기 배율 x (조준 중이면 조준 배율).
UCLASS()
class PROJECTRWW_API UPlayerMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UPlayerMovementComponent();

	// 클라이언트 입력이 세우는 의도. 실제로 적용되는 상태는 아래 Effective 함수가 판단한다.
	void SetWantsToSprint(bool bNewWantsToSprint) { bWantsToSprint = bNewWantsToSprint; }
	void SetWantsToAim(bool bNewWantsToAim) { bWantsToAim = bNewWantsToAim; }

	// 스프린트와 조준은 배타적이고 스프린트가 이긴다(스프린트 시작이 조준을 푸는 기존 규칙).
	bool IsSprintingEffective() const;
	// 서버가 확실히 아는 조건(무기 장착, 재장전 아님)을 AND한다 - 서버는 조준 RPC(Server_SetAiming)가
	// 이동 패킷보다 늦게 도착해도 bIsAiming을 기다리지 않고 이 조건으로 판단한다.
	bool IsAimingEffective() const;

	virtual float GetMaxSpeed() const override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual bool ClientUpdatePositionAfterServerUpdate() override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	uint8 bWantsToSprint : 1;
	uint8 bWantsToAim : 1;
};

// 이동 한 프레임의 입력/상태 스냅샷. 클라이언트가 서버로 보내고, 서버 교정 후 재생에도 쓰인다.
class FSavedMove_Player : public FSavedMove_Character
{
	typedef FSavedMove_Character Super;

public:
	virtual void Clear() override;
	virtual uint8 GetCompressedFlags() const override;
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
	virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;
	virtual void PrepMoveFor(ACharacter* C) override;

	uint8 bSavedWantsToSprint : 1;
	uint8 bSavedWantsToAim : 1;
};

class FNetworkPredictionData_Client_Player : public FNetworkPredictionData_Client_Character
{
	typedef FNetworkPredictionData_Client_Character Super;

public:
	FNetworkPredictionData_Client_Player(const UCharacterMovementComponent& ClientMovement) : Super(ClientMovement) {}

	virtual FSavedMovePtr AllocateNewMove() override;
};
