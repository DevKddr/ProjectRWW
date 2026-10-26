// Copyright Epic Games, Inc. All Rights Reserved.

#include "ItemActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Player/MainCharacter.h"
#include "Combat/MainWeaponComponent.h"

AItemActor::AItemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	ItemMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMeshComponent"));
	RootComponent = ItemMeshComponent;
}

void AItemActor::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<AMainCharacter>(GetOwner());
}

FName AItemActor::GetAttachSocketName() const
{
	if (const UItemVisualData* VisualData = ItemVisualDataPath.LoadSynchronous())
	{
		return VisualData->AttachSocketName;
	}
	return TEXT("VB ik_hand_gun_pivot");
}

FVector AItemActor::GetAttachOffsetLocation() const
{
	if (const UItemVisualData* VisualData = ItemVisualDataPath.LoadSynchronous())
	{
		return VisualData->AttachOffsetLocation;
	}
	return FVector::ZeroVector;
}

FRotator AItemActor::GetAttachOffsetRotation() const
{
	if (const UItemVisualData* VisualData = ItemVisualDataPath.LoadSynchronous())
	{
		return VisualData->AttachOffsetRotation;
	}
	return FRotator::ZeroRotator;
}

UAnimSequence* AItemActor::GetIdleAnimation() const
{
	if (const UItemVisualData* VisualData = ItemVisualDataPath.LoadSynchronous())
	{
		return VisualData->IdleAnimation.LoadSynchronous();
	}
	return nullptr;
}

UAnimSequence* AItemActor::GetWalkAnimation() const
{
	if (const UItemVisualData* VisualData = ItemVisualDataPath.LoadSynchronous())
	{
		return VisualData->WalkAnimation.LoadSynchronous();
	}
	return nullptr;
}

UAnimSequence* AItemActor::GetSprintAnimation() const
{
	if (const UItemVisualData* VisualData = ItemVisualDataPath.LoadSynchronous())
	{
		return VisualData->SprintAnimation.LoadSynchronous();
	}
	return nullptr;
}

UAnimSequence* AItemActor::GetJumpStartAnimation() const
{
	if (const UItemVisualData* VisualData = ItemVisualDataPath.LoadSynchronous())
	{
		return VisualData->JumpStartAnimation.LoadSynchronous();
	}
	return nullptr;
}

UAnimSequence* AItemActor::GetJumpLoopAnimation() const
{
	if (const UItemVisualData* VisualData = ItemVisualDataPath.LoadSynchronous())
	{
		return VisualData->JumpLoopAnimation.LoadSynchronous();
	}
	return nullptr;
}

UAnimSequence* AItemActor::GetJumpEndAnimation() const
{
	if (const UItemVisualData* VisualData = ItemVisualDataPath.LoadSynchronous())
	{
		return VisualData->JumpEndAnimation.LoadSynchronous();
	}
	return nullptr;
}

void AItemActor::PlayEquipAnimation(float EquipTime)
{
	if (const UItemVisualData* VisualData = ItemVisualDataPath.LoadSynchronous())
	{
		PlayTimedMontage(VisualData->EquipMontage.LoadSynchronous(), EquipTime);
	}
}

void AItemActor::UsePrimary(float CastTime)
{
	if (const UItemVisualData* VisualData = ItemVisualDataPath.LoadSynchronous())
	{
		PlayTimedMontage(VisualData->UsePrimaryMontage.LoadSynchronous(), CastTime);
	}
}

void AItemActor::UseSecondary(float CastTime)
{
	if (const UItemVisualData* VisualData = ItemVisualDataPath.LoadSynchronous())
	{
		PlayTimedMontage(VisualData->UseSecondaryMontage.LoadSynchronous(), CastTime);
	}
}

void AItemActor::PlayTimedMontage(UAnimMontage* Montage, float DesiredDuration) const
{
	if (!Montage)
	{
		return;
	}

	USceneComponent* AttachParent = GetRootComponent()->GetAttachParent();
	USkeletalMeshComponent* ParentMesh = Cast<USkeletalMeshComponent>(AttachParent);
	if (!ParentMesh)
	{
		return;
	}

	UAnimInstance* OuterAnimInstance = ParentMesh->GetAnimInstance();
	if (!OuterAnimInstance)
	{
		return;
	}

	// 바깥쪽(ABP_Unarmed)엔 이 몽타주를 받아줄 Slot이 없다 - 무기가 자기 링크드 인스턴스
	// (ABP_TacticalShooter_UE5)의 DefaultSlot에서 재생하는 것과 같은 패턴으로, "UpperBody"
	// 태그로 링크된 안쪽 인스턴스(아이템 장착 중이니 ABP_ItemUpperBody)를 찾아서 거기에 재생한다.
	UAnimInstance* TargetInstance = OuterAnimInstance->GetLinkedAnimGraphInstanceByTag(FName("UpperBody"));
	if (!TargetInstance)
	{
		return;
	}

	// 이전에 이 파이프라인(Equip/스킬)이 재생을 시작해둔 몽타주가 아직 남아있으면
	// 그것만 콕 집어서 끊는다 - 턴인플레이스 등 무관한 몽타주는 건드리지 않는다.
	if (OwnerCharacter && OwnerCharacter->WeaponComponent)
	{
		if (UAnimMontage* PreviousMontage = OwnerCharacter->WeaponComponent->GetCurrentItemActionMontage())
		{
			TargetInstance->Montage_Stop(0.0f, PreviousMontage);
		}
		OwnerCharacter->WeaponComponent->SetCurrentItemActionMontage(Montage);
	}

	const float PlayRate = DesiredDuration > 0.f ? Montage->GetPlayLength() / DesiredDuration : 1.f;
	TargetInstance->Montage_Play(Montage, PlayRate);
}

UAnimInstance* AItemActor::GetUpperBodyAnimInstance() const
{
	const USkeletalMeshComponent* ParentMesh = GetRootComponent() ? Cast<USkeletalMeshComponent>(GetRootComponent()->GetAttachParent()) : nullptr;
	UAnimInstance* OuterAnimInstance = ParentMesh ? ParentMesh->GetAnimInstance() : nullptr;
	return OuterAnimInstance ? OuterAnimInstance->GetLinkedAnimGraphInstanceByTag(FName("UpperBody")) : nullptr;
}

void AItemActor::PlayHoldStart(bool bPrimary, float PlayRate)
{
	const UItemVisualData* VisualData = ItemVisualDataPath.LoadSynchronous();
	UAnimMontage* Montage = VisualData ? (bPrimary ? VisualData->UsePrimaryMontage : VisualData->UseSecondaryMontage).LoadSynchronous() : nullptr;
	UAnimInstance* Instance = GetUpperBodyAnimInstance();
	if (!Montage || !Instance)
	{
		return;
	}

	// 이미 같은 몽타주를 재생 중이면 다시 시작하지 않는다(OnActive와 WhileActive가 둘 다 올 수 있다).
	if (ActiveHoldMontage.Get() == Montage && Instance->Montage_IsPlaying(Montage))
	{
		return;
	}

	// 이전 아이템 액션 몽타주가 남아 있으면 끊고, 이번 것을 새로 기록한다(PlayTimedMontage와 같은 방식).
	if (OwnerCharacter && OwnerCharacter->WeaponComponent)
	{
		if (UAnimMontage* PreviousMontage = OwnerCharacter->WeaponComponent->GetCurrentItemActionMontage())
		{
			Instance->Montage_Stop(0.0f, PreviousMontage);
		}
		OwnerCharacter->WeaponComponent->SetCurrentItemActionMontage(Montage);
	}

	Instance->Montage_Play(Montage, PlayRate > 0.0f ? PlayRate : 1.0f);
	ActiveHoldMontage = Montage;
	ActiveHoldInstance = Instance;
}

void AItemActor::StopHold()
{
	UAnimInstance* Instance = ActiveHoldInstance.Get();
	UAnimMontage* Montage = ActiveHoldMontage.Get();
	if (!Instance || !Montage || !Instance->Montage_IsPlaying(Montage))
	{
		return;
	}

	// 현재 섹션(Start 또는 Loop)이 끝난 뒤 End로 넘어가게 한다. 이미 End면 그대로 끝까지 재생한다.
	const FName CurrentSection = Instance->Montage_GetCurrentSection(Montage);
	if (CurrentSection != FName(TEXT("End")))
	{
		Instance->Montage_SetNextSection(CurrentSection, FName(TEXT("End")), Montage);
	}
}

void AItemActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 홀드 중에 아이템이 사라지면(아이템 교체/사망) End 없이 몽타주를 바로 멈춘다.
	UAnimInstance* Instance = ActiveHoldInstance.Get();
	UAnimMontage* Montage = ActiveHoldMontage.Get();
	if (Instance && Montage && Instance->Montage_IsPlaying(Montage))
	{
		Instance->Montage_Stop(0.1f, Montage);
	}

	Super::EndPlay(EndPlayReason);
}
