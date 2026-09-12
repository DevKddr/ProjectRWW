#pragma once
#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_Cooldown.generated.h"

// Duration만 SetByCaller로 받는 완전 범용 쿨다운 GE. 태그는 하드코딩하지 않는다 - 이걸
// 적용하는 어빌리티(UMainGameplayAbility::ApplyCooldown)가 DynamicGrantedTags로 매번
// 다른 태그를 실어서 붙인다. 그래서 스킬이 몇 개로 늘어나도 이 클래스는 하나면 된다.
UCLASS()
class PROJECTRWW_API UGE_Cooldown : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Cooldown();
};
