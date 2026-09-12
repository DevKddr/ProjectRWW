#include "GE_ManaCost.h"
#include "GAS/MainAttributeSet.h"
#include "GAS/MainGameplayTags.h"

UGE_ManaCost::UGE_ManaCost()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UMainAttributeSet::GetManaAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat CostMagnitude;
	CostMagnitude.DataTag = MainGameplayTags::Data_ManaCost.GetTag();
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(CostMagnitude);
	Modifiers.Add(Modifier);
}
