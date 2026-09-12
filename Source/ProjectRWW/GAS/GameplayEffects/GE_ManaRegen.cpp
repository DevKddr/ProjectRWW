#include "GE_ManaRegen.h"
#include "GAS/MainAttributeSet.h"
#include "GAS/MainGameplayTags.h"

UGE_ManaRegen::UGE_ManaRegen()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = 1.0f;
	bExecutePeriodicEffectOnApplication = false;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UMainAttributeSet::GetManaAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat RegenMagnitude;
	RegenMagnitude.DataTag = MainGameplayTags::Data_ManaRegen.GetTag();
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(RegenMagnitude);
	Modifiers.Add(Modifier);

	// 마나는 기존 UMainManaComponent에 피격 지연 개념이 없었으므로 IgnoreTags를 걸지 않는다
	// (HP와 달리 항상 회복). 나중에 마나에도 지연이 필요해지면 여기에 같은 태그를 추가한다.
}
