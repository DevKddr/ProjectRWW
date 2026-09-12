#pragma once
#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GAS/Abilities/MainGameplayAbility.h"
#include "ItemSkill_I_2_Primary.generated.h"

// UGA_ItemSkill_I_2_Primary 하나에서만 쓰이는 1:1 전용 이펙트라 같은 파일에 둔다. Duration,
// WalkSpeed/RunSpeed에 SetByCaller 배율만큼 Multiplicative(곱하기).
UCLASS()
class PROJECTRWW_API UGE_ItemSkill_I_2_Effect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_ItemSkill_I_2_Effect();
};

UCLASS()
class PROJECTRWW_API UGA_ItemSkill_I_2_Primary : public UMainGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_ItemSkill_I_2_Primary();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	// "이동속도 2배, 3초간" - Additive가 아니라 Multiplicative라서 WalkSpeed/RunSpeed
	// 현재값에 그대로 곱해진다(기본 이동속도가 나중에 바뀌어도 "2배"라는 의미가 안 깨짐).
	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	float SpeedMultiplier = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	float EffectDuration = 3.0f;
};
