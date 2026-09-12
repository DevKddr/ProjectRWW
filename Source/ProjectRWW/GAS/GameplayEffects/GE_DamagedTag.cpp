#include "GE_DamagedTag.h"
#include "GAS/MainGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_DamagedTag::UGE_DamagedTag()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat DelayMagnitude;
	DelayMagnitude.DataTag = MainGameplayTags::Data_HPRegenDelay.GetTag();
	DurationMagnitude = FGameplayEffectModifierMagnitude(DelayMagnitude);
}

void UGE_DamagedTag::PostInitProperties()
{
	Super::PostInitProperties();

	// FindOrAddComponent()는 내부적으로 NewObject(this, NAME_None, ...)를 호출하는데,
	// 이걸 생성자 안에서 부르면 "생성자 안에서 이름 없는 NewObject 금지"라는 엔진 규칙에
	// 걸려 인스턴스 생성 시점(에디터 시작 시 CDO 생성 포함)에 크래시가 난다. Epic 엔진
	// 자신도 이 계열 함수를 생성자가 아니라 PostLoad()에서만 부른다 - 우리는 네이티브
	// 클래스라 PostLoad 대신 생성자 체인이 끝난 직후인 PostInitProperties()를 쓴다.
	UTargetTagsGameplayEffectComponent& TargetTagsComponent = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer TagChanges;
	TagChanges.AddTag(MainGameplayTags::State_Damaged);
	TagChanges.AddTag(MainGameplayTags::Effect_ClearOnDeath);
	TargetTagsComponent.SetAndApplyTargetTagChanges(TagChanges);
}
