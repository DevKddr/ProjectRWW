#include "MainCastGameplayAbility.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "AbilitySystemComponent.h"
#include "GAS/MainGameplayTags.h"
#include "Player/MainCharacter.h"
#include "Player/MainPlayerController.h"

FGameplayTag UMainCastGameplayAbility::GetCastCueTag() const
{
	return SkillSlot == EMainAbilityInputID::Primary
		? MainGameplayTags::GameplayCue_ItemSkill_Hold_Primary.GetTag()
		: MainGameplayTags::GameplayCue_ItemSkill_Hold_Secondary.GetTag();
}

void UMainCastGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// 마나와 쿨다운은 여기서 처리하지 않는다 - 발동하는 순간에 처리한다(취소하면 아무것도 안 든다).
	if (IsStillEquipping(ActorInfo))
	{
		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] 캐스트 스킬 발동 실패: 장착 중 (%s)"), *GetClass()->GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (IsStillCasting(ActorInfo))
	{
		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] 캐스트 스킬 발동 실패: 시전 중 (%s)"), *GetClass()->GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (!HasEnoughMana(ActorInfo))
	{
		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] 캐스트 스킬 발동 실패: 마나 부족 (%s)"), *GetClass()->GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bCastStarted = true;
	bReleased = false;
	ReleasedHeldSeconds = 0.0f;

	// 본인 화면에만 시전 진행도 링을 띄운다(표시용이라 서버/다른 클라이언트는 필요 없다).
	const AMainCharacter* AvatarCharacter = ActorInfo ? Cast<AMainCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (AvatarCharacter && AvatarCharacter->IsLocallyControlled())
	{
		if (AMainPlayerController* PC = Cast<AMainPlayerController>(AvatarCharacter->GetController()))
		{
			PC->ShowSkillProgress(GetCurrentSkillHoldTime(ActorInfo));
		}
	}

	// bTestAlreadyReleased=true: 활성화 전에 이미 뗐다면 바로 뗌으로 처리한다.
	UAbilityTask_WaitInputRelease* ReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
	ReleaseTask->OnRelease.AddDynamic(this, &UMainCastGameplayAbility::OnReleased);
	ReleaseTask->ReadyForActivation();

	// 연출 시작. 홀드형/충전형과 같은 이유로 서버와 본인 클라이언트가 둘 다 부른다(본인 클라이언트는 예측 추가로 로컬에서 바로 재생).
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		FGameplayCueParameters CueParams;
		CueParams.Instigator = ActorInfo->AvatarActor.Get();
		CueParams.RawMagnitude = 1.0f; // 재생 속도. 지금은 항상 1.0.
		ActorInfo->AbilitySystemComponent->AddGameplayCue(GetCastCueTag(), CueParams);
	}
}

void UMainCastGameplayAbility::OnReleased(float TimeHeld)
{
	bReleased = true;
	ReleasedHeldSeconds = TimeHeld; // 서버에서는 서버가 직접 잰 누른 시간이다(클라이언트 값을 믿지 않는다).

	// 클라이언트는 종료를 서버로 복제하지 않는다. 복제하면 서버가 자기 뗌 콜백보다 먼저 "그냥 끝난" 상태로
	// EndAbility에 들어가 bReleased가 false인 채 발동이 사라질 수 있다. 서버는 자기 뗌 감지로 끝낸다.
	const bool bReplicateEnd = CurrentActorInfo && CurrentActorInfo->IsNetAuthority();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, bReplicateEnd, false);
}

void UMainCastGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 시작 가드에서 막힌 경우(bCastStarted=false)엔 정리도 발동도 없다.
	if (bCastStarted)
	{
		bCastStarted = false;
		const bool bWasReleased = bReleased;
		const float HeldSeconds = ReleasedHeldSeconds;
		bReleased = false;
		ReleasedHeldSeconds = 0.0f;

		// 연출 종료. 큐가 제거되면 아이템 액터가 몽타주를 End 섹션으로 넘긴다(강제 종료는 액터 파괴 시 바로 멈춘다).
		if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
		{
			ActorInfo->AbilitySystemComponent->RemoveGameplayCue(GetCastCueTag());
		}

		// 진행도 링 끄기(본인 클라이언트). 아래 castTime 잠금에서도 같은 캐릭터를 쓴다.
		AMainCharacter* Character = ActorInfo ? Cast<AMainCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
		if (Character && Character->IsLocallyControlled())
		{
			if (AMainPlayerController* PC = Cast<AMainPlayerController>(Character->GetController()))
			{
				PC->HideSkillProgress();
			}
		}

		if (ActorInfo && ActorInfo->IsNetAuthority())
		{
			const float HoldTime = GetCurrentSkillHoldTime(ActorInfo);
			if (!bWasReleased)
			{
				UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] 캐스트 스킬 취소(발동 없음): %s"), *GetClass()->GetName());
			}
			else if (HeldSeconds + ReleaseToleranceSeconds < HoldTime)
			{
				UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] 캐스트 스킬 시전 시간 부족으로 취소: %s 누른 시간 %.2f초 / 필요 %.2f초"), *GetClass()->GetName(), HeldSeconds, HoldTime);
			}
			else if (!HasEnoughMana(ActorInfo))
			{
				// 시전 준비하는 사이 마나가 줄어든 경우. 발동도 쿨다운도 없다.
				UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] 캐스트 스킬 마나 부족으로 발동 실패: %s"), *GetClass()->GetName());
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] 캐스트 스킬 발동: %s 누른 시간 %.2f초"), *GetClass()->GetName(), HeldSeconds);

				ApplyManaCost(Handle, ActorInfo, ActivationInfo);
				ApplyCooldown(Handle, ActorInfo, ActivationInfo);
				OnCastCompleted();
			}
		}

		// castTime 잠금은 끝나는 순간부터 건다(서버/클라이언트 각자 로컬 값). 홀드형/충전형과 같은 규칙.
		if (Character && Character->WeaponComponent)
		{
			Character->WeaponComponent->SetSkillCastEndTimeSeconds(FPlatformTime::Seconds() + GetCurrentSkillCastTime(ActorInfo));
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
