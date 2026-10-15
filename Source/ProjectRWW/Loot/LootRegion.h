// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LootRegion.generated.h"

class UBoxComponent;

// 레벨에 박스로 배치하는 "루트 지역". 안에 있는 ALootSpawnPoint가 어떤 루트 테이블을 쓸지를 결정한다.
// 테이블 이름은 여기에 없다 - RegionId만 정하고, RegionId -> 테이블 매핑은 loot_tables.json에 있다.
//
// 서버에서 레이드 시작 시 한 번만 쓰이는 데이터 액터다: 콜리전/틱/리플리케이션이 전부 꺼져 있고,
// 클라이언트에는 로드조차 되지 않는다(bNetLoadOnClient=false).
UCLASS()
class PROJECTRWW_API ALootRegion : public AActor
{
	GENERATED_BODY()

public:
	ALootRegion();

	// 비워 두면 Default 지역으로 취급한다(loot_tables.json의 Default).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	FName RegionId;

	// 지역이 겹쳤을 때 값이 큰 쪽이 이긴다. 같으면 부피가 작은(더 구체적인) 쪽이 이긴다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	int32 Priority = 0;

	// 비어 있으면 Default로 바꿔서 돌려준다.
	FName GetEffectiveRegionId() const;

	// 월드 좌표 한 점이 이 지역 박스 안에 있는지(회전/스케일 반영). 순수 수학 검사라 콜리전이 필요 없다.
	bool ContainsPoint(const FVector& WorldPoint) const;

	// 겹침 판정 동률 처리용: 스케일이 반영된 박스 부피.
	double GetVolume() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UBoxComponent> Volume;
};
