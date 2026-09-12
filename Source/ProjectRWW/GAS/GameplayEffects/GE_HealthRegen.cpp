#include "GE_HealthRegen.h"
#include "GAS/MainAttributeSet.h"
#include "GAS/MainGameplayTags.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"

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
}

void UGE_HealthRegen::PostInitProperties()
{
	Super::PostInitProperties();

	// GE_DamagedTag와 동일한 이유로 생성자 대신 PostInitProperties()에서 호출한다
	// (FindOrAddComponent 내부의 NewObject(NAME_None)이 생성자 안에서는 크래시를 냄).
	FindOrAddComponent<UTargetTagRequirementsGameplayEffectComponent>().OngoingTagRequirements.IgnoreTags.AddTag(MainGameplayTags::State_Damaged);
}
