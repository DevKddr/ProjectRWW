#pragma once
#include "CoreMinimal.h"
#include "GAS/Abilities/MainGameplayAbility.h"
#include "MainChargeGameplayAbility.generated.h"

// 키를 누르고 있는 동안 충전하고, 뗄 때 충전량(holdTime 상한)에 비례한 위력으로 한 번 발동하는 스킬의 공통 배관.
//  - 시작 가드: 장착 중/시전 중/마나 부족(manaCost 미만)이면 시작하지 않는다.
//  - 충전량은 서버가 잰 누른 시간 / holdTime (0~1). 가득 차도 자동 발동하지 않고 뗄 때까지 가득 찬 채 대기한다.
//  - 마나는 뗄 때 manaCost만큼 한 번 차감하고, 쿨다운도 그때 적용한다.
//  - 뗌이 아닌 경로(아이템 교체/사망으로 어빌리티가 회수됨)로 끝나면 발동도 마나도 쿨다운도 없다.
//  - 효과 적용(훅 호출, 마나, 쿨다운)은 서버에서만 한다. 클라이언트는 활성화/뗌 감지/종료만 하고 결과는 복제로 받는다.
UCLASS(Abstract)
class PROJECTRWW_API UMainChargeGameplayAbility : public UMainGameplayAbility
{
	GENERATED_BODY()

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// 서버에서만 호출된다. ChargeRatio: 0~1 (holdTime이 0이면 항상 1).
	virtual void OnChargeReleased(float ChargeRatio) {}

private:
	// 슬롯에 따른 연출 큐 태그. 홀드형과 같은 큐를 쓴다(Start/Loop 재생, 큐가 제거되면 End 섹션).
	FGameplayTag GetChargeCueTag() const;

	UFUNCTION()
	void OnReleased(float TimeHeld);

	// 시작 가드를 통과해 실제로 충전이 시작됐는지. EndAbility가 두 번 불려도 정리가 한 번만 되게 하는 용도도 겸한다.
	bool bChargeStarted = false;

	// 뗌으로 끝났는지(true면 발동). 아이템 교체/사망으로 회수돼 끝나면 false로 남는다.
	bool bReleased = false;
	float ReleasedHeldSeconds = 0.0f;
};
