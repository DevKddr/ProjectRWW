// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LootTablesData.h"
#include "LootTableManager.generated.h"

/**
 * loot_tables.json 전용 매니저. "이 지역(RegionId)에서는 어떤 테이블로 아이템 하나를 뽑는가"만 책임진다.
 * 뽑힌 아이템을 월드에 만드는 건 이 매니저의 일이 아니다 - ULootSpawnSubsystem이 한다.
 *
 * 추첨은 3단계 가중치다: 등급(RarityWeights) -> 카테고리(CategoryWeights) -> 아이템(ItemWeights 배수).
 * 후보 아이템은 UItemDataManager(items.json)에서 가져오므로, 아이템을 추가해도 이 파일은 안 건드려도 된다.
 */
UCLASS()
class PROJECTRWW_API ULootTableManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// RegionId가 비어 있거나 loot_tables.json에 없는 지역일 때 쓰는 지역 이름.
	static const FName DefaultRegionId;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	bool IsDataLoaded() const { return bDataLoaded; }

	bool HasTable(FName TableId) const { return ResolvedTables.Contains(TableId); }

	/**
	 * RegionId -> 실제로 쓸 지역/테이블/스폰 확률. RegionId가 None이거나 데이터에 없는 지역이면
	 * Default 지역으로 대체한다(없는 지역은 로그로 한 번만 경고). Default조차 없으면 false.
	 */
	bool ResolveRegion(FName RegionId, FName& OutRegionUsed, FName& OutTableId, float& OutSpawnChance) const;

	/** 테이블 하나로 아이템 Index를 하나 뽑는다. 뽑을 수 있는 아이템이 없으면 false. */
	bool DrawItem(FName TableId, FName& OutItemIndex) const;

private:
	// 부모 테이블을 이미 합쳐 둔(상속이 풀린) 테이블.
	struct FResolvedLootTable
	{
		TMap<FString, float> RarityWeights;
		TMap<FString, float> CategoryWeights;
		TMap<FString, float> ItemWeights;
		TSet<FString> ExcludedItems;
	};

	bool LoadLootTables();
	bool ResolveTable(const FString& TableId, int32 Depth, FResolvedLootTable& Out) const;
	void ValidateData() const;

	UPROPERTY()
	bool bDataLoaded = false;

	FLootTablesFile RawData;
	TMap<FName, FResolvedLootTable> ResolvedTables;

	// 모르는 RegionId 경고를 같은 이름당 한 번만 찍기 위한 기록(스폰 포인트마다 호출되므로).
	mutable TSet<FName> WarnedUnknownRegions;

	static const FString LootTablesJsonRelativePath;
};
