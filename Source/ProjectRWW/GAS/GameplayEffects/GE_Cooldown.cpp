#include "GE_Cooldown.h"
#include "GAS/MainGameplayTags.h"

UGE_Cooldown::UGE_Cooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	// 변수명을 부모(UGameplayEffect)의 DurationMagnitude 멤버와 겹치지 않게 짓는다 -
	// 겹치면 이름 가리기(shadowing)로 아래 대입이 로컬 변수 자기 자신을 가리키게 돼서
	// 타입이 안 맞아 컴파일이 깨진다.
	FSetByCallerFloat CooldownDurationMagnitude;
	CooldownDurationMagnitude.DataTag = MainGameplayTags::Data_Cooldown_Duration.GetTag();
	DurationMagnitude = FGameplayEffectModifierMagnitude(CooldownDurationMagnitude);
}
