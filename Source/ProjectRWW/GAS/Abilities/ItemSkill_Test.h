#pragma once
#include "CoreMinimal.h"
#include "GAS/Abilities/MainGameplayAbility.h"
#include "ItemSkill_Test.generated.h"

// [임시 테스트] 홀드형 스킬 설계 2단계 검증 전용. 효과 없이 로그만 남긴다 -
// 클라이언트가 뗐을 때 서버에서도 WaitInputRelease가 호출되는지, 서버가 끝낸 어빌리티(아이템 교체/사망)가
// 오너 클라이언트에서도 EndAbility로 끝나는지 확인한 뒤, 3단계에서 I_3~I_5의 실제 스킬로 교체하고
// 이 파일 두 개는 삭제한다.
// 클래스 이름이 곧 연결 규칙이다: 아이템 코드로 "GA_ItemSkill_<코드>_Primary"를 찾아 부여한다.
UCLASS(Abstract)
class PROJECTRWW_API UGA_ItemSkillTestBase : public UMainGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_ItemSkillTestBase();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UFUNCTION()
	void OnReleased(float TimeHeld);

	// 로그에 서버/클라이언트와 오너 여부를 같이 찍기 위한 헬퍼.
	FString RoleString() const;
};

UCLASS()
class PROJECTRWW_API UGA_ItemSkill_I_5_Primary : public UGA_ItemSkillTestBase
{
	GENERATED_BODY()
};
