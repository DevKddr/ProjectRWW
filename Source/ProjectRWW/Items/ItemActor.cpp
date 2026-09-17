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
		UE_LOG(LogTemp, Warning, TEXT("[ProjectRWW] PlayTimedMontage(%s): Montage가 None이라 재생 안 함"), *GetName());
		return;
	}

	USceneComponent* AttachParent = GetRootComponent()->GetAttachParent();
	USkeletalMeshComponent* ParentMesh = Cast<USkeletalMeshComponent>(AttachParent);
	if (!ParentMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ProjectRWW] PlayTimedMontage(%s): 부모 컴포넌트(%s, 클래스=%s)가 SkeletalMeshComponent가 아님"),
			*GetName(),
			AttachParent ? *AttachParent->GetName() : TEXT("None"),
			AttachParent ? *AttachParent->GetClass()->GetName() : TEXT("None"));
		return;
	}

	UAnimInstance* OuterAnimInstance = ParentMesh->GetAnimInstance();
	if (!OuterAnimInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ProjectRWW] PlayTimedMontage(%s): %s에 AnimInstance가 없음"), *GetName(), *ParentMesh->GetName());
		return;
	}

	// 바깥쪽(ABP_Unarmed)엔 이 몽타주를 받아줄 Slot이 없다 - 무기가 자기 링크드 인스턴스
	// (ABP_TacticalShooter_UE5)의 DefaultSlot에서 재생하는 것과 같은 패턴으로, "UpperBody"
	// 태그로 링크된 안쪽 인스턴스(아이템 장착 중이니 ABP_ItemUpperBody)를 찾아서 거기에 재생한다.
	UAnimInstance* TargetInstance = OuterAnimInstance->GetLinkedAnimGraphInstanceByTag(FName("UpperBody"));
	if (!TargetInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ProjectRWW] PlayTimedMontage(%s): \"UpperBody\" 태그로 링크된 인스턴스를 못 찾음"), *GetName());
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
	const float PlayedLength = TargetInstance->Montage_Play(Montage, PlayRate);

	UE_LOG(LogTemp, Warning, TEXT("[ProjectRWW] PlayTimedMontage(%s): TargetInstance=%s, Montage=%s, MontageLength=%.3f, DesiredDuration=%.3f, PlayRate=%.3f, Montage_Play 반환값=%.3f"),
		*GetName(), *TargetInstance->GetClass()->GetName(), *Montage->GetName(),
		Montage->GetPlayLength(), DesiredDuration, PlayRate, PlayedLength);
}
