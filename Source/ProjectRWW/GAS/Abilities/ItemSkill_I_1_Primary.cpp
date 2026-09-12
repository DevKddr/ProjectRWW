#include "ItemSkill_I_1_Primary.h"
#include "AbilitySystemComponent.h"
#include "GAS/MainAttributeSet.h"
#include "NativeGameplayTags.h"

// 이 파일 안에서만 쓰는 태그 - MainGameplayTags에 안 넣는다 (다른 파일에서 참조할 일이 없음).
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Cooldown_ItemSkill_I_1_Primary, "Cooldown.ItemSkill.I_1.Primary");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Data_ItemSkill_I_1_HealAmount, "Data.ItemSkill.I_1.HealAmount");

UGE_ItemSkill_I_1_Effect::UGE_ItemSkill_I_1_Effect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UMainAttributeSet::GetHealthAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat HealMagnitude;
	HealMagnitude.DataTag = TAG_Data_ItemSkill_I_1_HealAmount;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(HealMagnitude);
	Modifiers.Add(Modifier);
}

UGA_ItemSkill_I_1_Primary::UGA_ItemSkill_I_1_Primary()
{
	CooldownTag = TAG_Cooldown_ItemSkill_I_1_Primary;
	SkillSlot = EMainAbilityInputID::Primary;
}

void UGA_ItemSkill_I_1_Primary::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!TryCommitSkillActivation(Handle, ActorInfo, ActivationInfo))
	{
		return;
	}

	// CommitAbility()가 내부적으로 ApplyCooldown()(부모 클래스 오버라이드)을 이미 호출해서
	// 쿨다운은 여기서 신경 안 써도 된다 - 실제 효과(회복)만 별도로 적용한다.
	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(UGE_ItemSkill_I_1_Effect::StaticClass(), GetAbilityLevel(Handle, ActorInfo));
	SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_ItemSkill_I_1_HealAmount, HealAmount);
	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);

	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}
