#pragma once
#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GAS/Abilities/MainChargeGameplayAbility.h"
#include "ItemSkill_I_4_Primary.generated.h"

// UGA_ItemSkill_I_4_Primary 하나에서만 쓰이는 1:1 전용 이펙트라 같은 파일에 둔다. Instant, Health에
// SetByCaller 값만큼 Additive.
UCLASS()
class PROJECTRWW_API UGE_ItemSkill_I_4_Effect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_ItemSkill_I_4_Effect();
};

// 충전 회복(Charge): 누르고 있는 동안 충전하고, 뗄 때 충전량에 비례해 HP를 한 번 회복한다.
// 마나(고정)/쿨다운/충전 시간(holdTime)은 items.json의 primarySkill... 값.
UCLASS()
class PROJECTRWW_API UGA_ItemSkill_I_4_Primary : public UMainChargeGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_ItemSkill_I_4_Primary();

protected:
	virtual void OnChargeReleased(float ChargeRatio) override;

	// 밸런싱용 - 가득 충전했을 때의 회복량. 실제 회복량 = MaxHeal * ChargeRatio.
	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	float MaxHeal = 30.0f;
};
