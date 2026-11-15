#pragma once
#include "CoreMinimal.h"
#include "GAS/Abilities/MainGameplayAbility.h"
#include "MainCastGameplayAbility.generated.h"

// 키를 holdTime 이상 눌러 시전이 준비된 뒤 뗄 때 발동하는 스킬의 공통 배관.
//  - 시작 가드: 장착 중/시전 중/마나 부족(manaCost 미만)이면 시작하지 않는다.
//  - 서버가 잰 누른 시간이 holdTime 이상이면(허용 오차 포함) 뗄 때 발동한다. 가득 차도 자동 발동하지 않고 뗄 때까지 가득 찬 채 대기한다.
//  - holdTime 전에 떼면 취소. holdTime이 0이면 누르고 뗀 것만으로 발동한다.
//  - 마나와 쿨다운은 발동하는 순간 한 번 적용한다.
//  - 뗌이 아닌 경로(아이템 교체/사망으로 어빌리티가 회수됨)로 끝나면 발동도 마나도 쿨다운도 없다.
//  - 효과 적용(훅 호출, 마나, 쿨다운)은 서버에서만 한다. 클라이언트는 활성화/뗌 감지/종료만 하고 결과는 복제로 받는다.
UCLASS(Abstract)
class PROJECTRWW_API UMainCastGameplayAbility : public UMainGameplayAbility
{
	GENERATED_BODY()

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// 서버에서만 호출된다. 시전이 성립했고 마나와 쿨다운 처리도 이미 끝난 뒤에 불린다.
	virtual void OnCastCompleted() {}

	// 서버가 잰 누른 시간이 holdTime보다 이만큼(초)까지 모자라도 성립으로 봐준다. 클라이언트와 서버가 시작/뗌을 각각
	// 지연만큼 늦게 받아서, 클라이언트가 정확히 holdTime에 뗀 경우에도 서버는 약간 짧게 잴 수 있기 때문이다.
	UPROPERTY(EditDefaultsOnly, Category = "Cast", meta = (ClampMin = "0.0"))
	float ReleaseToleranceSeconds = 0.15f;

private:
	// 슬롯에 따른 연출 큐 태그. 홀드형/충전형과 같은 큐를 쓴다(Start/Loop 재생, 큐가 제거되면 End 섹션).
	FGameplayTag GetCastCueTag() const;

	UFUNCTION()
	void OnReleased(float TimeHeld);

	// 시작 가드를 통과해 실제로 시전 준비가 시작됐는지. EndAbility가 두 번 불려도 정리가 한 번만 되게 하는 용도도 겸한다.
	bool bCastStarted = false;

	// 뗌으로 끝났는지. 아이템 교체/사망으로 회수돼 끝나면 false로 남는다.
	bool bReleased = false;
	float ReleasedHeldSeconds = 0.0f;
};
