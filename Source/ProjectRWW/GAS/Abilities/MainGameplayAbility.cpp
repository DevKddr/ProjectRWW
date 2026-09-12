#include "MainGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "GAS/GameplayEffects/GE_Cooldown.h"
#include "GAS/GameplayEffects/GE_ManaCost.h"
#include "GAS/MainAttributeSet.h"
#include "GAS/MainGameplayTags.h"
#include "Player/MainCharacter.h"
#include "Items/ItemData.h"
#include "Items/ItemDataManager.h"

void UMainGameplayAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const float CooldownDuration = GetCurrentSkillCooldownDuration(ActorInfo);
	if (!CooldownTag.IsValid() || CooldownDuration <= 0.0f)
	{
		return;
	}

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(UGE_Cooldown::StaticClass(), GetAbilityLevel(Handle, ActorInfo));
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(MainGameplayTags::Data_Cooldown_Duration.GetTag(), CooldownDuration);
		// 어떤 스킬의 쿨다운인지는 이 동적 태그로만 구분한다 - UGE_Cooldown 자체는 태그를
		// 하나도 하드코딩하지 않은 완전 범용 클래스라 스킬마다 새로 안 만들어도 된다.
		SpecHandle.Data->DynamicGrantedTags.AddTag(CooldownTag);
		FActiveGameplayEffectHandle ActiveHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);

		// 쿨다운이 자연 만료되는 순간(시간이 다 되어 GAS가 알아서 이 활성 이펙트를 제거하는
		// 시점)에 로그를 남긴다. 이 베이스 클래스에 한 번만 짜두면 I_1/I_2를 포함해 앞으로
		// 만들 모든 스킬이 자동으로 이 로그를 갖게 된다.
		if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
		{
			if (FOnActiveGameplayEffectRemoved_Info* RemovedDelegate = ASC->OnGameplayEffectRemoved_InfoDelegate(ActiveHandle))
			{
				const FGameplayTag TagForLog = CooldownTag;
				RemovedDelegate->AddLambda([TagForLog](const FGameplayEffectRemovalInfo& RemovalInfo)
				{
					UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] 쿨다운 종료: %s"), *TagForLog.ToString());
				});
			}
		}
	}
}

bool UMainGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	FGameplayTagContainer FailureTags;
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, &FailureTags))
	{
		// ActivateAbility()는 여기서 이미 거부되면 아예 호출되지 않는다 - 쿨다운/코스트/
		// 태그 요구사항 등 GAS 자체의 발동 전제조건 실패는 전부 여기서 잡힌다.
		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] 아이템 스킬 발동 실패(CanActivateAbility): %s, 사유 태그: %s"),
			*GetClass()->GetName(), *FailureTags.ToStringSimple());
		return false;
	}
	return true;
}

const FGameplayTagContainer* UMainGameplayAbility::GetCooldownTags() const
{
	CachedCooldownTags.Reset();
	if (CooldownTag.IsValid())
	{
		CachedCooldownTags.AddTag(CooldownTag);
	}
	return &CachedCooldownTags;
}

bool UMainGameplayAbility::GetOwnerItemData(const FGameplayAbilityActorInfo* ActorInfo, FItemData& OutItemData) const
{
	const AMainCharacter* Character = ActorInfo ? Cast<AMainCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	const UMainWeaponComponent* WeaponComp = Character ? Character->WeaponComponent : nullptr;
	const UGameInstance* GameInstance = WeaponComp && WeaponComp->GetWorld() ? WeaponComp->GetWorld()->GetGameInstance() : nullptr;
	UItemDataManager* ItemDataManager = GameInstance ? GameInstance->GetSubsystem<UItemDataManager>() : nullptr;
	return ItemDataManager && WeaponComp && ItemDataManager->GetItemData(WeaponComp->GetActiveItemIndex(), OutItemData);
}

float UMainGameplayAbility::GetCurrentSkillCooldownDuration(const FGameplayAbilityActorInfo* ActorInfo) const
{
	FItemData ItemData;
	if (!GetOwnerItemData(ActorInfo, ItemData))
	{
		return 0.0f;
	}
	return SkillSlot == EMainAbilityInputID::Primary ? ItemData.PrimarySkillCooldown : ItemData.SecondarySkillCooldown;
}

float UMainGameplayAbility::GetCurrentSkillManaCost(const FGameplayAbilityActorInfo* ActorInfo) const
{
	FItemData ItemData;
	if (!GetOwnerItemData(ActorInfo, ItemData))
	{
		return 0.0f;
	}
	return SkillSlot == EMainAbilityInputID::Primary ? ItemData.PrimarySkillManaCost : ItemData.SecondarySkillManaCost;
}

bool UMainGameplayAbility::HasEnoughMana(const FGameplayAbilityActorInfo* ActorInfo) const
{
	const float ManaCost = GetCurrentSkillManaCost(ActorInfo);
	if (ManaCost <= 0.0f)
	{
		return true;
	}

	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const UMainAttributeSet* AttrSet = ASC ? ASC->GetSet<UMainAttributeSet>() : nullptr;
	return ASC && AttrSet && AttrSet->GetMana() >= ManaCost;
}

void UMainGameplayAbility::ApplyManaCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const float ManaCost = GetCurrentSkillManaCost(ActorInfo);
	if (ManaCost <= 0.0f)
	{
		return;
	}

	// ApplyCooldown()과 완전히 같은 패턴 - MakeOutgoingGameplayEffectSpec()으로 스펙을
	// 만들고 ApplyGameplayEffectSpecToOwner()로 적용해서 예측 키가 자동으로 붙게 한다.
	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(UGE_ManaCost::StaticClass(), GetAbilityLevel(Handle, ActorInfo));
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(MainGameplayTags::Data_ManaCost.GetTag(), -ManaCost);
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}
}

bool UMainGameplayAbility::IsStillEquipping(const FGameplayAbilityActorInfo* ActorInfo) const
{
	const AMainCharacter* Character = ActorInfo ? Cast<AMainCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	const UMainWeaponComponent* WeaponComp = Character ? Character->WeaponComponent : nullptr;
	if (!WeaponComp)
	{
		return false;
	}

	return FPlatformTime::Seconds() - WeaponComp->GetEquippedTimeSeconds() < WeaponComp->GetEquipTime();
}

bool UMainGameplayAbility::TryCommitSkillActivation(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (IsStillEquipping(ActorInfo))
	{
		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] 아이템 스킬 발동 실패: 장착 중 (%s)"), *GetClass()->GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return false;
	}

	if (!HasEnoughMana(ActorInfo))
	{
		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] 아이템 스킬 발동 실패: 마나 부족 (%s)"), *GetClass()->GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return false;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] 아이템 스킬 발동 실패: 쿨다운 중 (%s)"), *GetClass()->GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] 아이템 스킬 발동: %s"), *GetClass()->GetName());
	ApplyManaCost(Handle, ActorInfo, ActivationInfo);
	return true;
}
