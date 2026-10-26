// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemVisualData.h"
#include "ItemActor.generated.h"

class AMainCharacter;
class UStaticMeshComponent;

// 아이템(맨손 포함) 전용 액터. BP_TacticalShooterWeapon(Kinemation 무기 액터)을 더 이상
// 상속하지 않는다 - Fire/Reload/ADS 등 무기 전용 인터페이스가 하나도 없다. 유일한 C++
// 클래스이고, 아이템마다(BP_I_1, BP_I_2, BP_Unarmed 등) 이 클래스를 상속한 Blueprint를
// 만들어 메시와 ItemVisualDataPath만 Class Defaults에서 다르게 채운다.
//
UCLASS(BlueprintType, Blueprintable)
class PROJECTRWW_API AItemActor : public AActor
{
	GENERATED_BODY()

public:
	AItemActor();

	// 이 아이템의 시각적 표현(Idle/Walk/Sprint/Equip/스킬 몽타주/부착 소켓)을 담은
	// Data Asset 경로. 아이템마다 파생 Blueprint의 Class Defaults에서 지정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TSoftObjectPtr<UItemVisualData> ItemVisualDataPath;

	// MainWeaponComponent::EquipVisual()이 부착 직후 이 값들을 읽어서 소켓/오프셋을 적용한다.
	// ItemVisualDataPath가 아직 로드 안 됐거나 비어있으면 기존 하드코딩 기본값을 반환해서
	// 무기 쪽 동작(항상 "VB ik_hand_gun_pivot", 오프셋 0)과 동일하게 유지된다.
	UFUNCTION(BlueprintPure, Category = "Item")
	FName GetAttachSocketName() const;

	UFUNCTION(BlueprintPure, Category = "Item")
	FVector GetAttachOffsetLocation() const;

	UFUNCTION(BlueprintPure, Category = "Item")
	FRotator GetAttachOffsetRotation() const;

	// BP_MainCharacter의 Event ReceiveItemEquip이 ABP_ItemUpperBody의 링크드 인스턴스에
	// 매 장착마다 값을 채워넣을 때 쓴다. ItemVisualDataPath가 없거나 로드 실패하면 nullptr을
	// 반환한다.
	UFUNCTION(BlueprintPure, Category = "Item")
	UAnimSequence* GetIdleAnimation() const;

	UFUNCTION(BlueprintPure, Category = "Item")
	UAnimSequence* GetWalkAnimation() const;

	UFUNCTION(BlueprintPure, Category = "Item")
	UAnimSequence* GetSprintAnimation() const;

	UFUNCTION(BlueprintPure, Category = "Item")
	UAnimSequence* GetJumpStartAnimation() const;

	UFUNCTION(BlueprintPure, Category = "Item")
	UAnimSequence* GetJumpLoopAnimation() const;

	UFUNCTION(BlueprintPure, Category = "Item")
	UAnimSequence* GetJumpEndAnimation() const;

	// 장착(Equip) 몽타주를 재생한다. EquipTime은 MainWeaponComponent::GetEquipTime()이
	// 반환하는 값을 그대로 전달받아, 재생 시간을 정확히 EquipTime에 맞춘다.
	UFUNCTION(BlueprintCallable, Category = "Item")
	void PlayEquipAnimation(float EquipTime);

	// 스킬 사용 몽타주를 재생한다. GetPlayerMesh() 같은 BP 전용 함수 없이, EquipVisual()이
	// 이미 만들어둔 부착 관계(이 액터의 부모 컴포넌트)를 그대로 타고 올라가서 AnimInstance를
	// 얻는다. CastTime은 GC_ItemSkillUse_Primary/Secondary가 RawMagnitude로 실어보낸 값을
	// 그대로 전달받아, 재생 시간을 정확히 CastTime에 맞춘다.
	UFUNCTION(BlueprintCallable, Category = "Item")
	void UsePrimary(float CastTime);

	UFUNCTION(BlueprintCallable, Category = "Item")
	void UseSecondary(float CastTime);

	// 홀드형 스킬 연출. DA의 UsePrimaryMontage/UseSecondaryMontage 하나에 Start/Loop/End 섹션이 있다고 가정한다.
	// PlayHoldStart는 몽타주를 재생해 Start→Loop로 이어지게 하고, StopHold는 다음 섹션을 End로 바꿔 정상 종료시킨다.
	// 강제 종료(아이템 교체/사망)는 이 액터가 파괴될 때 EndPlay가 몽타주를 바로 멈춘다.
	UFUNCTION(BlueprintCallable, Category = "Item")
	void PlayHoldStart(bool bPrimary, float PlayRate = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Item")
	void StopHold();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 아이템 3D 모델. RootComponent로 사용한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UStaticMeshComponent> ItemMeshComponent;

	// GetOwner()를 캐스팅해서 채운다.
	UPROPERTY(BlueprintReadOnly, Category = "Item")
	TObjectPtr<AMainCharacter> OwnerCharacter;

private:
	// PlayEquipAnimation()/UsePrimary()/UseSecondary() 공용 재생 로직. Montage가 없으면
	// (에셋 미설정) 조용히 무시한다. DesiredDuration이 0 이하면(비정상 값) 기본 속도(1.0)로
	// 재생한다.
	void PlayTimedMontage(UAnimMontage* Montage, float DesiredDuration) const;

	// 위쪽 몸 애니메이션 인스턴스("UpperBody" 태그로 링크된 것). PlayTimedMontage와 같은 경로로 얻는다.
	UAnimInstance* GetUpperBodyAnimInstance() const;

	// 지금 재생 중인 홀드 몽타주와 그것이 재생되는 인스턴스. StopHold 뒤에도 유지해서, 그 직후 액터가 파괴되면
	// EndPlay가 End 재생 중인 몽타주를 바로 멈출 수 있게 한다.
	TWeakObjectPtr<UAnimMontage> ActiveHoldMontage;
	TWeakObjectPtr<UAnimInstance> ActiveHoldInstance;
};
