// Copyright Epic Games, Inc. All Rights Reserved.
#include "MainAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "MainGameplayTags.h"

UMainAttributeSet::UMainAttributeSet()
{
}

void UMainAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UMainAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMainAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMainAttributeSet, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMainAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMainAttributeSet, WalkSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMainAttributeSet, RunSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMainAttributeSet, JumpPower, COND_None, REPNOTIFY_Always);
}

void UMainAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		const float LocalDamage = GetDamage();
		SetDamage(0.0f);

		if (LocalDamage > 0.0f)
		{
			const float NewHealth = FMath::Clamp(GetHealth() - LocalDamage, 0.0f, GetMaxHealth());
			SetHealth(NewHealth);

			// 체력이 바뀌면 소유자(PlayerState)를 바로 복제 대상으로 올린다 - 복제 빈도 설정을 기다리지 않는다.
			if (AActor* OwnerActor = Data.Target.GetOwnerActor())
			{
				OwnerActor->ForceNetUpdate();
			}

			if (NewHealth <= 0.0f)
			{
				const FGameplayEffectContextHandle Context = Data.EffectSpec.GetEffectContext();
				AController* Killer = Cast<AController>(Context.GetInstigator());
				AActor* Avatar = GetOwningAbilitySystemComponent() ? GetOwningAbilitySystemComponent()->GetAvatarActor() : nullptr;

				// 사망 시점에 남아있는 임시 효과(회복 지연 태그, 향후 버프/디버프 등)를
				// 전부 정리한다 - 태그 자체로 "정리 대상"만 표시하므로, 새 임시 효과를
				// 추가할 때 이 코드를 다시 건드릴 필요가 없다.
				if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
				{
					ASC->RemoveActiveEffectsWithTags(FGameplayTagContainer(MainGameplayTags::Effect_ClearOnDeath.GetTag()));
				}

				OnDeath.Broadcast(Avatar, Killer);
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));

		// 회복(자연 회복, 아이템 스킬 등 GameplayEffect로 체력이 오르는 모든 경로)도 여기를 지나므로
		// 헬퍼 함수 없이 이 한 곳에서 바로 복제 대상으로 올린다.
		if (AActor* OwnerActor = Data.Target.GetOwnerActor())
		{
			OwnerActor->ForceNetUpdate();
		}
	}
	else if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(), 0.0f, GetMaxMana()));

		// 마나도 같은 이유 - 홀드형 스킬이 틱마다 마나를 깎아도 HUD가 바로 따라오게 한다.
		if (AActor* OwnerActor = Data.Target.GetOwnerActor())
		{
			OwnerActor->ForceNetUpdate();
		}
	}
}

void UMainAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMainAttributeSet, Health, OldValue);
}
void UMainAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMainAttributeSet, MaxHealth, OldValue);
}
void UMainAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMainAttributeSet, Mana, OldValue);
}
void UMainAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMainAttributeSet, MaxMana, OldValue);
}
void UMainAttributeSet::OnRep_WalkSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMainAttributeSet, WalkSpeed, OldValue);
	OnMovementAttributesChanged.Broadcast();
}
void UMainAttributeSet::OnRep_RunSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMainAttributeSet, RunSpeed, OldValue);
	OnMovementAttributesChanged.Broadcast();
}
void UMainAttributeSet::OnRep_JumpPower(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMainAttributeSet, JumpPower, OldValue);
	OnMovementAttributesChanged.Broadcast();
}
