#pragma once
#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_DamagedTag.generated.h"

// Duration 이펙트. Attribute는 안 건드리고 State.Damaged 태그만 부여한다.
// Duration은 SetByCaller(Data.HPRegenDelay)로 매번 주입 - PlayerBaseStat.json의
// HPRegenDelay 값을 그대로 쓰기 위함(하드코딩 안 함).
UCLASS()
class PROJECTRWW_API UGE_DamagedTag : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_DamagedTag();
};
