#include "GE_DamagedTag.h"
#include "GAS/MainGameplayTags.h"

UGE_DamagedTag::UGE_DamagedTag()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat DelayMagnitude;
	DelayMagnitude.DataTag = MainGameplayTags::Data_HPRegenDelay.GetTag();
	DurationMagnitude = FGameplayEffectModifierMagnitude(DelayMagnitude);

	InheritableOwnedTagsContainer.Added.AddTag(MainGameplayTags::State_Damaged);
	InheritableOwnedTagsContainer.Added.AddTag(MainGameplayTags::Effect_ClearOnDeath);
}
