#include "GE_HealthRegen.h"
#include "GAS/MainAttributeSet.h"
#include "GAS/MainGameplayTags.h"

UGE_HealthRegen::UGE_HealthRegen()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = 1.0f;
	bExecutePeriodicEffectOnApplication = false;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UMainAttributeSet::GetHealthAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat RegenMagnitude;
	RegenMagnitude.DataTag = MainGameplayTags::Data_HPRegen.GetTag();
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(RegenMagnitude);
	Modifiers.Add(Modifier);

	OngoingTagRequirements.IgnoreTags.AddTag(MainGameplayTags::State_Damaged);
}
