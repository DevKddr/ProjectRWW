// Copyright Epic Games, Inc. All Rights Reserved.

#include "PlayerMovementComponent.h"
#include "GameFramework/Character.h"
#include "Player/MainCharacter.h"
#include "Combat/MainWeaponComponent.h"
#include "GAS/MainAttributeSet.h"

UPlayerMovementComponent::UPlayerMovementComponent()
{
	bWantsToSprint = false;
	bWantsToAim = false;
}

bool UPlayerMovementComponent::IsSprintingEffective() const
{
	return bWantsToSprint;
}

bool UPlayerMovementComponent::IsAimingEffective() const
{
	const AMainCharacter* Character = Cast<AMainCharacter>(CharacterOwner);
	const UMainWeaponComponent* Weapon = Character ? Character->WeaponComponent : nullptr;
	return bWantsToAim && !bWantsToSprint && Weapon && Weapon->HasWeaponEquipped() && !Weapon->IsReloading();
}

float UPlayerMovementComponent::GetMaxSpeed() const
{
	// 걷기/낙하일 때만 우리 계산을 쓴다(수영/비행/웅크리기 등은 엔진 기본값).
	if ((MovementMode != MOVE_Walking && MovementMode != MOVE_NavWalking && MovementMode != MOVE_Falling) || IsCrouching())
	{
		return Super::GetMaxSpeed();
	}

	const AMainCharacter* Character = Cast<AMainCharacter>(CharacterOwner);
	const UMainAttributeSet* Attr = Character ? Character->GetMainAttributeSet() : nullptr;
	if (!Attr)
	{
		return Super::GetMaxSpeed();
	}

	// 기본 속도는 GAS 어트리뷰트 - JSON 기본값에 I_2 같은 버프 GE가 이미 곱해진 값이다.
	float Speed = IsSprintingEffective() ? Attr->GetRunSpeed() : Attr->GetWalkSpeed();

	// 무기 배율 x 조준 배율(모두 곱셈). 무기를 안 들면 두 값이 1.0(FWeaponStats 기본값)이라 결과가 변하지 않는다.
	if (const UMainWeaponComponent* Weapon = Character->WeaponComponent)
	{
		Speed *= Weapon->GetMoveSpeedMultiplier();
		if (IsAimingEffective())
		{
			Speed *= Weapon->GetADSMoveSpeedMultiplier();
		}
	}
	return Speed;
}

void UPlayerMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	// 서버가 클라이언트의 이동 패킷을 받을 때 호출된다.
	bWantsToSprint = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
	bWantsToAim = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;
}

bool UPlayerMovementComponent::ClientUpdatePositionAfterServerUpdate()
{
	// 서버 교정을 받으면 저장된 이동들을 재생하는데, 그 과정에서 PrepMoveFor가 플래그를 과거 값으로 덮어쓴다.
	// 엔진은 자기 플래그(bWantsToCrouch)만 원상 복구하므로, 우리 플래그는 여기서 직접 복구한다.
	const bool bRealSprint = bWantsToSprint;
	const bool bRealAim = bWantsToAim;
	const bool bResult = Super::ClientUpdatePositionAfterServerUpdate();
	bWantsToSprint = bRealSprint;
	bWantsToAim = bRealAim;
	return bResult;
}

FNetworkPredictionData_Client* UPlayerMovementComponent::GetPredictionData_Client() const
{
	if (ClientPredictionData == nullptr)
	{
		UPlayerMovementComponent* MutableThis = const_cast<UPlayerMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Player(*this);
	}
	return ClientPredictionData;
}

void FSavedMove_Player::Clear()
{
	Super::Clear();
	bSavedWantsToSprint = false;
	bSavedWantsToAim = false;
}

uint8 FSavedMove_Player::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();
	if (bSavedWantsToSprint)
	{
		Result |= FLAG_Custom_0;
	}
	if (bSavedWantsToAim)
	{
		Result |= FLAG_Custom_1;
	}
	return Result;
}

bool FSavedMove_Player::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
	// 상태가 다른 두 이동은 합치지 않는다 - 합치면 그 사이의 상태 변화가 서버에 전달되지 않는다.
	const FSavedMove_Player* NewPlayerMove = static_cast<const FSavedMove_Player*>(NewMove.Get());
	if (bSavedWantsToSprint != NewPlayerMove->bSavedWantsToSprint || bSavedWantsToAim != NewPlayerMove->bSavedWantsToAim)
	{
		return false;
	}
	return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void FSavedMove_Player::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	if (const UPlayerMovementComponent* Movement = Cast<UPlayerMovementComponent>(C->GetCharacterMovement()))
	{
		bSavedWantsToSprint = Movement->bWantsToSprint;
		bSavedWantsToAim = Movement->bWantsToAim;
	}
}

void FSavedMove_Player::PrepMoveFor(ACharacter* C)
{
	Super::PrepMoveFor(C);

	if (UPlayerMovementComponent* Movement = Cast<UPlayerMovementComponent>(C->GetCharacterMovement()))
	{
		Movement->bWantsToSprint = bSavedWantsToSprint;
		Movement->bWantsToAim = bSavedWantsToAim;
	}
}

FSavedMovePtr FNetworkPredictionData_Client_Player::AllocateNewMove()
{
	return MakeShared<FSavedMove_Player>();
}
