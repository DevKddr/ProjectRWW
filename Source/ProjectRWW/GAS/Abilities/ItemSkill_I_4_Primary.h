#pragma once
#include "CoreMinimal.h"
#include "GAS/Abilities/MainChargeGameplayAbility.h"
#include "ItemSkill_I_4_Primary.generated.h"

class AItemSkill_I_4_Projectile;

// 충격 투사체(Charge): 누르고 있는 동안 충전하고, 뗄 때 충전량에 따라 속도가 달라지는 투사체를 던진다.
// 투사체가 무언가에 닿으면 짧은 지연(0.3초) 뒤 그 지점 중심으로 반경 안의 모든 플레이어(본인 포함)를
// 바깥쪽으로 밀어낸다. 충전량은 던지는 속도에만 영향을 주고, 밀어내는 세기는 충전과 무관하다.
// 마나(고정)/쿨다운/충전 시간(holdTime)은 items.json의 primarySkill... 값.
UCLASS()
class PROJECTRWW_API UGA_ItemSkill_I_4_Primary : public UMainChargeGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_ItemSkill_I_4_Primary();

protected:
	virtual void OnChargeReleased(float ChargeRatio) override;

	// 스폰할 투사체 BP. 어빌리티는 네이티브 클래스(이름으로 찾음)라 에디터에서 지정할 수 없어 생성자에서 경로로 지정한다.
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSoftClassPtr<AItemSkill_I_4_Projectile> ProjectileClassPath;

	// 밸런싱용 - 충전 0%일 때와 100%일 때의 투사체 처음 속도(cm/s). 실제 속도 = Lerp(Min, Max, ChargeRatio).
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float MinSpeed = 2400.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float MaxSpeed = 6600.0f;

	// 투사체를 카메라 앞 이만큼(cm) 떨어진 곳에서 시작한다 - 시전자 몸에 겹쳐 스폰되지 않게.
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float SpawnForwardOffset = 80.0f;
};
