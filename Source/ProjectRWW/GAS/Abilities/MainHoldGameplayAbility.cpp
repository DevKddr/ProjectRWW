#include "MainHoldGameplayAbility.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "TimerManager.h"
#include "AbilitySystemComponent.h"
#include "GAS/MainGameplayTags.h"
#include "Player/MainCharacter.h"
#include "Player/MainPlayerController.h"

FGameplayTag UMainHoldGameplayAbility::GetHoldCueTag() const
{
	return SkillSlot == EMainAbilityInputID::Primary
		? MainGameplayTags::GameplayCue_ItemSkill_Hold_Primary.GetTag()
		: MainGameplayTags::GameplayCue_ItemSkill_Hold_Secondary.GetTag();
}

void UMainHoldGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// 쿨다운은 CanActivateAbility가 이미 확인했다. 마나 차감과 쿨다운 적용은 여기서 하지 않는다 -
	// 마나는 틱마다, 쿨다운은 끝날 때 적용한다.
	if (IsStillEquipping(ActorInfo))
	{
		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] 홀드 스킬 발동 실패: 장착 중 (%s)"), *GetClass()->GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (IsStillCasting(ActorInfo))
	{
		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] 홀드 스킬 발동 실패: 시전 중 (%s)"), *GetClass()->GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (!HasEnoughMana(ActorInfo))
	{
		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] 홀드 스킬 발동 실패: 마나 부족 (%s)"), *GetClass()->GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bHoldStarted = true;
	bEndedNormally = false;

	// 본인 화면에만 남은 유지 시간 링을 띄운다(표시용이라 서버/다른 클라이언트는 필요 없다).
	// holdTime이 0(무제한)이면 링이 차오를 기준이 없어 띄우지 않는다.
	const float RingDuration = GetCurrentSkillHoldTime(ActorInfo);
	const AMainCharacter* AvatarCharacter = ActorInfo ? Cast<AMainCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (RingDuration > 0.0f && AvatarCharacter && AvatarCharacter->IsLocallyControlled())
	{
		if (AMainPlayerController* PC = Cast<AMainPlayerController>(AvatarCharacter->GetController()))
		{
			PC->ShowSkillProgress(RingDuration);
		}
	}

	// 뗌 감지는 클라이언트/서버 모두 한다. 클라이언트가 뗐을 때 서버로 전달되는 것은 2단계 테스트로 확인했다.
	// bTestAlreadyReleased=true: 활성화 전에 이미 뗐다면 바로 뗌으로 처리한다.
	UAbilityTask_WaitInputRelease* ReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
	ReleaseTask->OnRelease.AddDynamic(this, &UMainHoldGameplayAbility::OnReleased);
	ReleaseTask->ReadyForActivation();

	// 연출 시작을 알린다. 서버만 부르면 큐에 "누른 본인 클라이언트의 예측 키"가 붙어서 복제되는데, 엔진은 그 키를 만든
	// 클라이언트가 이미 예측해서 재생했다고 보고 복제된 큐를 무시한다(본인에게만 연출이 안 보이던 원인). 그래서 서버와
	// 본인 클라이언트가 둘 다 부른다 - 클라이언트 호출은 엔진이 예측 추가(PredictiveAdd)로 처리하고 OnActive를 로컬에서 부른다.
	// 다른 클라이언트는 서버가 복제한 큐로 재생한다.
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		FGameplayCueParameters CueParams;
		CueParams.Instigator = ActorInfo->AvatarActor.Get();
		CueParams.RawMagnitude = 1.0f; // 재생 속도. 지금은 항상 1.0.
		ActorInfo->AbilitySystemComponent->AddGameplayCue(GetHoldCueTag(), CueParams);
	}

	// 틱과 최대 유지 시간은 서버에서만 돈다(효과 적용은 서버 권위, 클라이언트는 결과를 복제로 받는다).
	if (ActorInfo && ActorInfo->IsNetAuthority())
	{
		OnHoldStart();

		// 첫 틱은 누르자마자 한 번 돌린다 - 짧게 눌러도 효과가 최소 한 번 나가야 쿨다운이 걸리는 규칙과 맞는다.
		DoHoldTick();
		if (!IsActive() || !bHoldStarted)
		{
			return; // 첫 틱에서 마나 부족 등으로 이미 끝났다.
		}

		const float TickInterval = GetCurrentSkillTickInterval(ActorInfo);
		const float HoldTime = GetCurrentSkillHoldTime(ActorInfo);
		if (UWorld* World = GetWorld())
		{
			if (TickInterval > 0.0f)
			{
				World->GetTimerManager().SetTimer(TickTimerHandle, this, &UMainHoldGameplayAbility::DoHoldTick, TickInterval, true);
			}
			if (HoldTime > 0.0f)
			{
				World->GetTimerManager().SetTimer(MaxHoldTimerHandle, this, &UMainHoldGameplayAbility::OnMaxHoldReached, HoldTime, false);
			}
		}
	}
}

void UMainHoldGameplayAbility::EndNormally()
{
	bEndedNormally = true;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UMainHoldGameplayAbility::OnReleased(float TimeHeld)
{
	EndNormally();
}

void UMainHoldGameplayAbility::OnMaxHoldReached()
{
	UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] 홀드 스킬 최대 유지 시간 도달: %s"), *GetClass()->GetName());
	EndNormally();
}

void UMainHoldGameplayAbility::DoHoldTick()
{
	if (!CurrentActorInfo || !IsActive() || !bHoldStarted)
	{
		return;
	}

	// 마나는 틱마다 manaCost만큼 차감한다. 모자라면 효과를 내지 않고 끝낸다.
	if (!HasEnoughMana(CurrentActorInfo))
	{
		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] 홀드 스킬 마나 부족으로 종료: %s"), *GetClass()->GetName());
		EndNormally();
		return;
	}

	ApplyManaCost(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);
	OnHoldTick();
}

void UMainHoldGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 시작 가드에서 막힌 경우(bHoldStarted=false)엔 정리도 쿨다운도 없다.
	if (bHoldStarted)
	{
		bHoldStarted = false;
		const bool bNormal = bEndedNormally;
		bEndedNormally = false;

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(TickTimerHandle);
			World->GetTimerManager().ClearTimer(MaxHoldTimerHandle);
		}

		// 연출 종료를 알린다. 시작과 마찬가지로 서버와 본인 클라이언트가 둘 다 부른다 - 클라이언트가 예측 키 범위 안에서
		// (뗌 콜백 등) 부르면 엔진이 예측 제거(PredictiveRemove)로 Removed를 로컬에서 바로 부르고, 서버가 끝낸 경우처럼 범위 밖이면
		// 클라이언트 호출은 무시되고 서버가 제거한 큐가 복제돼 와서 Removed가 불린다. 정상 종료는 노티파이가 End 섹션으로
		// 넘기고, 강제 종료(교체/사망)는 아이템 액터가 파괴될 때 몽타주를 바로 멈춘다.
		if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
		{
			ActorInfo->AbilitySystemComponent->RemoveGameplayCue(GetHoldCueTag());
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

		// 서버에서만: 서브클래스 정리 훅(상태 효과 제거 등)과 쿨다운. 쿨다운은 뗌/시간 경과/마나 부족/
		// 아이템 교체/사망 어느 경로로 끝나든 적용한다 - 안 걸면 홀드 중 교체했다 다시 들어 쿨다운 없이 재사용하는 꼼수가 생긴다.
		if (ActorInfo && ActorInfo->IsNetAuthority())
		{
			OnHoldEnd(bNormal);
			ApplyCooldown(Handle, ActorInfo, ActivationInfo);
		}

		// castTime 잠금은 끝나는 순간부터 건다(서버/클라이언트 각자 로컬 값). 위에서 쓰는 아이템 데이터는
		// 교체 경로에서도 아직 유효하다(UnequipWeapon이 ActiveItemIndex를 바꾸지 않는다).
		if (Character && Character->WeaponComponent)
		{
			Character->WeaponComponent->SetSkillCastEndTimeSeconds(FPlatformTime::Seconds() + GetCurrentSkillCastTime(ActorInfo));
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
