// PlayerBaseStat.h
// 모든 플레이어가 공유하는 기본 스탯. Id/RowName으로 구분할 필요가 없으므로
// FTableRowBase를 상속하지 않는 순수 구조체다.
// PlayerBaseStat.json ( {"MaxHP":..,"HPRegen":..,"HPRegenDelay":..,"WalkSpeed":..,"RunSpeed":..,
// "MaxMana":..,"ManaRegen":..,"JumpPower":..} 형태의 단일 객체 )과 1:1로 매칭된다.
// 향후 퍽/특성 시스템은 이 구조체에 필드를 추가하는 게 아니라,
// 별도의 모디파이어 데이터를 두고 런타임에 이 값 위에 가감하는 방식으로 처리한다.

#pragma once

#include "CoreMinimal.h"
#include "PlayerBaseStat.generated.h"

USTRUCT(BlueprintType)
struct FPlayerBaseStat
{
	GENERATED_BODY()

public:
	// 최대 체력
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayerStat")
	float MaxHP = 0.f;

	// 초당 체력 회복량
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayerStat")
	float HPRegen = 0.f;

	// 피격 후 체력 회복이 다시 시작되기까지의 지연시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayerStat")
	float HPRegenDelay = 0.f;

	// 걷기 이동속도 (uu/s)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayerStat")
	float WalkSpeed = 0.f;

	// 달리기 이동속도 (uu/s)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayerStat")
	float RunSpeed = 0.f;

	// 최대 마나
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayerStat")
	float MaxMana = 0.f;

	// 초당 마나 회복량
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayerStat")
	float ManaRegen = 0.f;

	// 점프력 (JumpZVelocity)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayerStat")
	float JumpPower = 0.f;
};

