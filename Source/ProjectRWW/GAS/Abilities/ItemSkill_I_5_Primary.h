#pragma once
#include "CoreMinimal.h"
#include "GAS/Abilities/MainGameplayAbility.h"
#include "ItemSkill_I_5_Primary.generated.h"

class AItemSkill_I_5_Zone;

// 장판 생성(Instant): 누르는 즉시 시전자 발밑에 장판 액터를 소환한다. 장판은 일정 시간 뒤 스스로 사라지고,
// 그동안 원기둥 안의 모든 플레이어(적 포함)에게 이동속도/연사속도 상승 버프를 준다.
// 마나/쿨다운/시전 시간(castTime)은 items.json의 primarySkill... 값.
UCLASS()
class PROJECTRWW_API UGA_ItemSkill_I_5_Primary : public UMainGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_ItemSkill_I_5_Primary();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	// 스폰할 장판 BP. 어빌리티는 네이티브 클래스(이름으로 찾음)라 에디터에서 지정할 수 없어 생성자에서 경로로 지정한다.
	UPROPERTY(EditDefaultsOnly, Category = "Zone")
	TSoftClassPtr<AItemSkill_I_5_Zone> ZoneClassPath;
};
