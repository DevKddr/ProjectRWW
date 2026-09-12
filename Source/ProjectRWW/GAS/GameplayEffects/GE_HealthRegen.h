#pragma once
#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_HealthRegen.generated.h"

// Infinite + Period 1초 이펙트. State.Damaged 태그가 있는 동안은 OngoingTagRequirements가
// 적용을 막는다 - 기존 UMainHPComponent::OnEffectTick의 "피격 후 지연시간 동안 회복 안 함"과
// 동일한 결과를 GAS 표준 기능으로 재현한 것.
UCLASS()
class PROJECTRWW_API UGE_HealthRegen : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_HealthRegen();
};
