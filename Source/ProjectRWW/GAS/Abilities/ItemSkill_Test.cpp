#include "ItemSkill_Test.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"

UGA_ItemSkillTestBase::UGA_ItemSkillTestBase()
{
	SkillSlot = EMainAbilityInputID::Primary;
}

FString UGA_ItemSkillTestBase::RoleString() const
{
	if (!CurrentActorInfo)
	{
		return TEXT("ActorInfo없음");
	}
	return FString::Printf(TEXT("%s/%s"),
		CurrentActorInfo->IsNetAuthority() ? TEXT("서버") : TEXT("클라"),
		CurrentActorInfo->IsLocallyControlled() ? TEXT("오너") : TEXT("비오너"));
}

void UGA_ItemSkillTestBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UE_LOG(LogTemp, Log, TEXT("[ProjectRWW][테스트] 활성화 (%s) %s"), *RoleString(), *GetClass()->GetName());

	// bTestAlreadyReleased=true: 활성화되기 전에 이미 키를 뗐다면 바로 뗌으로 처리한다(빠른 연타 대비).
	UAbilityTask_WaitInputRelease* ReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
	ReleaseTask->OnRelease.AddDynamic(this, &UGA_ItemSkillTestBase::OnReleased);
	ReleaseTask->ReadyForActivation();
}

void UGA_ItemSkillTestBase::OnReleased(float TimeHeld)
{
	UE_LOG(LogTemp, Log, TEXT("[ProjectRWW][테스트] 뗌 감지 (%s) 홀드 %.2f초 %s"), *RoleString(), TimeHeld, *GetClass()->GetName());
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_ItemSkillTestBase::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UE_LOG(LogTemp, Log, TEXT("[ProjectRWW][테스트] 종료 (%s) cancelled=%s %s"), *RoleString(), bWasCancelled ? TEXT("true") : TEXT("false"), *GetClass()->GetName());
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
