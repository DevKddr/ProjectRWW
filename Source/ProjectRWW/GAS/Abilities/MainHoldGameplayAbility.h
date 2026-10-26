#pragma once
#include "CoreMinimal.h"
#include "GAS/Abilities/MainGameplayAbility.h"
#include "MainHoldGameplayAbility.generated.h"

// 키를 누르고 있는 동안 이어지는 스킬(채널링/상태 유지)의 공통 배관. 서브클래스는 훅만 구현한다.
//  - 시작 가드: 장착 중/시전 중/마나 부족(한 틱 비용 미만)이면 시작하지 않는다.
//  - 누르자마자 첫 틱을 한 번 돌리고, 이후 tickInterval마다 틱(0이면 틱 없이 시작/종료만).
//  - 마나는 틱마다 manaCost만큼 차감하고, 모자라면 종료한다.
//  - 뗌, holdTime 경과(0이면 무제한), 마나 부족, 아이템 교체/사망(어빌리티 회수) 어느 경로로 끝나든
//    EndAbility 한 곳에서 정리한다. 쿨다운은 끝날 때 적용하고, castTime 잠금도 끝나는 순간부터 건다.
//  - 효과 적용(훅 호출, 마나, 쿨다운)은 서버에서만 한다. 클라이언트는 활성화/뗌 감지/종료만 하고 결과는 복제로 받는다.
UCLASS(Abstract)
class PROJECTRWW_API UMainHoldGameplayAbility : public UMainGameplayAbility
{
	GENERATED_BODY()

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// 아래 훅은 서버에서만 호출된다.
	virtual void OnHoldStart() {}
	virtual void OnHoldTick() {}
	// bNormal: 뗌/시간 경과/마나 부족으로 정상 종료했으면 true. 아이템 교체/사망으로 어빌리티가 회수돼 끝난
	// 강제 종료면 false(ClearAbility는 bWasCancelled=false로 끝내므로 그 플래그로는 구분할 수 없다).
	virtual void OnHoldEnd(bool bNormal) {}

private:
	// 슬롯에 따른 홀드 연출 큐 태그.
	FGameplayTag GetHoldCueTag() const;

	// 정상 종료 경로는 이 함수로 끝낸다 - bEndedNormally를 세운 뒤 EndAbility를 부른다.
	void EndNormally();

	UFUNCTION()
	void OnReleased(float TimeHeld);

	void DoHoldTick();
	void OnMaxHoldReached();

	FTimerHandle TickTimerHandle;
	FTimerHandle MaxHoldTimerHandle;

	// 시작 가드를 통과해 실제로 홀드가 시작됐는지. 시작도 못 한 경우엔 쿨다운/잠금을 걸지 않고,
	// EndAbility가 두 번 불려도(뗌과 타이머가 겹칠 때) 정리가 한 번만 되게 하는 용도도 겸한다.
	bool bHoldStarted = false;

	// 정상 종료 경로(EndNormally)에서만 true로 세운다. EndAbility가 읽고 곧바로 리셋한다.
	bool bEndedNormally = false;
};
