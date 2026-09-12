#include "ItemSkill_I_2_Primary.h"
#include "AbilitySystemComponent.h"
#include "GAS/MainAttributeSet.h"
#include "NativeGameplayTags.h"

// 이 파일 안에서만 쓰는 태그 - MainGameplayTags에 안 넣는다 (다른 파일에서 참조할 일이 없음).
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Cooldown_ItemSkill_I_2_Primary, "Cooldown.ItemSkill.I_2.Primary");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Data_ItemSkill_I_2_SpeedMultiplier, "Data.ItemSkill.I_2.SpeedMultiplier");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Data_ItemSkill_I_2_Duration, "Data.ItemSkill.I_2.Duration");

UGE_ItemSkill_I_2_Effect::UGE_ItemSkill_I_2_Effect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat EffectDurationMagnitude;
	EffectDurationMagnitude.DataTag = TAG_Data_ItemSkill_I_2_Duration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(EffectDurationMagnitude);

	FSetByCallerFloat SpeedMagnitude;
	SpeedMagnitude.DataTag = TAG_Data_ItemSkill_I_2_SpeedMultiplier;

	FGameplayModifierInfo WalkModifier;
	WalkModifier.Attribute = UMainAttributeSet::GetWalkSpeedAttribute();
	WalkModifier.ModifierOp = EGameplayModOp::Multiplicitive;
	WalkModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SpeedMagnitude);
	Modifiers.Add(WalkModifier);

	FGameplayModifierInfo RunModifier = WalkModifier;
	RunModifier.Attribute = UMainAttributeSet::GetRunSpeedAttribute();
	Modifiers.Add(RunModifier);
}

UGA_ItemSkill_I_2_Primary::UGA_ItemSkill_I_2_Primary()
{
	CooldownTag = TAG_Cooldown_ItemSkill_I_2_Primary;
	SkillSlot = EMainAbilityInputID::Primary;
}

void UGA_ItemSkill_I_2_Primary::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!TryCommitSkillActivation(Handle, ActorInfo, ActivationInfo))
	{
		return;
	}

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(UGE_ItemSkill_I_2_Effect::StaticClass(), GetAbilityLevel(Handle, ActorInfo));
	SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_ItemSkill_I_2_SpeedMultiplier, SpeedMultiplier);
	SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_ItemSkill_I_2_Duration, EffectDuration);
	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);

	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}
