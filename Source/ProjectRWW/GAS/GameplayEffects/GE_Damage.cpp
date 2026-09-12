#include "GE_Damage.h"
#include "GAS/MainAttributeSet.h"
#include "GAS/MainGameplayTags.h"

UGE_Damage::UGE_Damage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UMainAttributeSet::GetDamageAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat DamageMagnitude;
	DamageMagnitude.DataTag = MainGameplayTags::Data_Damage.GetTag();
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(DamageMagnitude);
	Modifiers.Add(Modifier);
}
