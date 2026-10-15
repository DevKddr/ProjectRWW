// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LootSpawnSubsystem.generated.h"

class AItemPickupActor;
class ALootRegion;
class ALootSpawnPoint;

/**
 * 레벨의 ALootSpawnPoint마다 아이템 하나를 뽑아 AItemPickupActor로 스폰한다. 서버(및 스탠드얼론)에서만 동작한다.
 *
 * 흐름(월드 BeginPlay 때 한 번):
 *   1) 포인트가 어느 ALootRegion 안에 있는지 찾는다. 어느 지역에도 없거나 지역의 RegionId가 비어 있으면 Default.
 *   2) 지역 -> 테이블/스폰 확률을 ULootTableManager(loot_tables.json)에서 찾는다.
 *   3) 확률 판정에 통과하면 테이블로 아이템을 뽑아 픽업을 스폰한다.
 *
 * 틱이 없고 스폰은 레이드 시작 시 일괄 처리한다. 지역 판정은 박스 수학 검사라 비용이 작다(포인트 수 x 지역 수).
 * 콘솔: rww.Loot.Respawn (남은 픽업을 지우고 다시 스폰), rww.Loot.Dump (지역별 결과를 로그로).
 */
UCLASS()
class PROJECTRWW_API ULootSpawnSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// 이전에 이 시스템이 스폰한 픽업 중 남은 것을 지우고 처음부터 다시 스폰한다(디버그/재시작용).
	void RespawnAll();

	// 지역별 포인트 수/스폰 수를 로그로 남긴다.
	void DumpStats() const;

private:
	void SpawnAll();
	void ClearSpawned();

	// 이 포인트가 속한 지역의 RegionId를 찾는다. 우선순위가 높은 쪽, 같으면 부피가 작은 쪽. 없으면 None(=Default).
	FName FindRegionIdForPoint(const FVector& Location, const TArray<ALootRegion*>& Regions) const;

	// ItemIndex에 쓸 픽업 BP를 LootSettings에서 찾는다. 없으면 nullptr.
	UClass* ResolvePickupClass(FName ItemIndex);

	AItemPickupActor* SpawnPickup(UClass* PickupClass, FName ItemIndex, const FTransform& PointTransform);

	struct FRegionStats
	{
		int32 Points = 0;
		int32 Spawned = 0;
	};

	TMap<FName, FRegionStats> Stats;
	TArray<TWeakObjectPtr<AItemPickupActor>> SpawnedPickups;

	// 매핑이 없는 아이템 경고를 한 번만 찍기 위한 기록.
	TSet<FName> WarnedMissingPickupClass;
};
