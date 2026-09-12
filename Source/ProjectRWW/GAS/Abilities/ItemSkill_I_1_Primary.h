#pragma once
#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GAS/Abilities/MainGameplayAbility.h"
#include "ItemSkill_I_1_Primary.generated.h"

// UGA_ItemSkill_I_1_Primary 하나에서만 쓰이는 1:1 전용 이펙트라 같은 파일에 둔다. Instant, Health에
// SetByCaller 값만큼 Additive.
UCLASS()
class PROJECTRWW_API UGE_ItemSkill_I_1_Effect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_ItemSkill_I_1_Effect();
};

UCLASS()
class PROJECTRWW_API UGA_ItemSkill_I_1_Primary : public UMainGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_ItemSkill_I_1_Primary();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	// 밸런싱용 - 나중에 값만 조정하면 된다.
	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	float HealAmount = 30.0f;
};
