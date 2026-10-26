#pragma once
#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GAS/Abilities/MainHoldGameplayAbility.h"
#include "ItemSkill_I_3_Primary.generated.h"

// UGA_ItemSkill_I_3_Primary 하나에서만 쓰이는 1:1 전용 이펙트라 같은 파일에 둔다. Instant, Health에
// SetByCaller 값만큼 Additive. 틱마다 한 번씩 적용된다.
UCLASS()
class PROJECTRWW_API UGE_ItemSkill_I_3_Effect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_ItemSkill_I_3_Effect();
};

// 응급 치료(Hold): 누르고 있는 동안 tickInterval마다 HP를 회복한다. 틱당 마나 소모는 items.json의 primarySkillManaCost.
UCLASS()
class PROJECTRWW_API UGA_ItemSkill_I_3_Primary : public UMainHoldGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_ItemSkill_I_3_Primary();

protected:
	virtual void OnHoldTick() override;

	// 밸런싱용 - 틱당 회복량(tickInterval 0.1초 기준 초당 10).
	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	float HealPerTick = 1.0f;
};
