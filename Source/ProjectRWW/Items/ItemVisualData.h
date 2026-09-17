// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"
#include "ItemVisualData.generated.h"

// 아이템(맨손 포함)의 시각적 표현(Idle/Walk/Sprint/Equip/스킬 몽타주/부착 소켓)을 담는다.
// Kinemation Tactical Shooter Pack 전용 타입(DA_TacticalShooterViewSettings)을 무기가
// 아닌 아이템에도 억지로 씌우던 것을 그만두기 위해 이 프로젝트 소유로 새로 만들었다 -
// 무기 전용 필드(sway, ADS, 리로드 등)가 하나도 없다. 아이템마다 인스턴스 하나
// (DA_ItemVisual_I_1 등)를 만들고, FItemData::ItemVisualDataPath가 그 인스턴스를
// 가리킨다.
UCLASS(BlueprintType)
class PROJECTRWW_API UItemVisualData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 상체 Idle/Walk/Sprint 포즈. 방향별 블렌드(BlendSpace) 없이 단일 시퀀스 재생 -
	// 이동 방향 블렌딩은 하체(ABP_Unarmed)가 이미 담당한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Visual")
	TSoftObjectPtr<UAnimSequence> IdleAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Visual")
	TSoftObjectPtr<UAnimSequence> WalkAnimation;

	// Sprint 전용 에셋이 아직 없어 당분간 WalkAnimation과 같은 에셋을 가리키게 채운다 -
	// 나중에 전용 에셋이 생기면 이 인스턴스의 값만 바꾸면 된다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Visual")
	TSoftObjectPtr<UAnimSequence> SprintAnimation;

	// 점프 시퀀스: 발구름(In) → 공중 루프(Loop) → 착지(Out) 3단 구성.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Visual")
	TSoftObjectPtr<UAnimSequence> JumpStartAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Visual")
	TSoftObjectPtr<UAnimSequence> JumpLoopAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Visual")
	TSoftObjectPtr<UAnimSequence> JumpEndAnimation;

	// 장착(Equip) 모션.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Visual")
	TSoftObjectPtr<UAnimMontage> EquipMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Visual")
	TSoftObjectPtr<UAnimMontage> UsePrimaryMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Visual")
	TSoftObjectPtr<UAnimMontage> UseSecondaryMontage;

	// 손 부착 소켓 이름. 지금까지 무기/아이템 공용으로 MainWeaponComponent::EquipVisual()에
	// "VB ik_hand_gun_pivot"이 하드코딩되어 있던 것을 아이템별로 오버라이드 가능하게 한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Visual")
	FName AttachSocketName = TEXT("VB ik_hand_gun_pivot");

	// 소켓 기준 위치/회전 오프셋. 기본값(0)이면 소켓에 정확히 스냅된 것과 동일하게 동작한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Visual")
	FVector AttachOffsetLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Visual")
	FRotator AttachOffsetRotation = FRotator::ZeroRotator;
};
