#pragma once
#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_Damage.generated.h"

// Instant 이펙트. SetByCaller(Data.Damage) 값을 UMainAttributeSet::Damage(메타)에
// Additive로 넣는다 - 실제 Health 반영은 UMainAttributeSet::PostGameplayEffectExecute가 처리.
UCLASS()
class PROJECTRWW_API UGE_Damage : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Damage();
};
