#include "ItemSkill_I_3_Primary.h"
#include "AbilitySystemComponent.h"
#include "GAS/MainAttributeSet.h"
#include "NativeGameplayTags.h"

// 이 파일 안에서만 쓰는 태그 - MainGameplayTags에 안 넣는다 (다른 파일에서 참조할 일이 없음).
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Cooldown_ItemSkill_I_3_Primary, "Cooldown.ItemSkill.I_3.Primary");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Data_ItemSkill_I_3_HealAmount, "Data.ItemSkill.I_3.HealAmount");

UGE_ItemSkill_I_3_Effect::UGE_ItemSkill_I_3_Effect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UMainAttributeSet::GetHealthAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat HealMagnitude;
	HealMagnitude.DataTag = TAG_Data_ItemSkill_I_3_HealAmount;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(HealMagnitude);
	Modifiers.Add(Modifier);
}

UGA_ItemSkill_I_3_Primary::UGA_ItemSkill_I_3_Primary()
{
	CooldownTag = TAG_Cooldown_ItemSkill_I_3_Primary;
	SkillSlot = EMainAbilityInputID::Primary;
}

void UGA_ItemSkill_I_3_Primary::OnHoldTick()
{
	// 서버에서만 호출된다(UMainHoldGameplayAbility). 마나 차감은 베이스가 이미 했다.
	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(UGE_ItemSkill_I_3_Effect::StaticClass(), GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo));
	SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_ItemSkill_I_3_HealAmount, HealPerTick);
	ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, SpecHandle);
}
