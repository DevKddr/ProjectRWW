#pragma once
#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_ManaCost.generated.h"

// Instant 이펙트. SetByCaller(Data.ManaCost)를 Mana에 Additive로 넣는다 - 호출부가
// 음수 값을 넘기면 차감, 양수 값을 넘기면 지급이 되는 범용 구조.
UCLASS()
class PROJECTRWW_API UGE_ManaCost : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_ManaCost();
};
