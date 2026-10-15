// Copyright Epic Games, Inc. All Rights Reserved.

#include "LootTableManager.h"
#include "Items/ItemDataManager.h"
#include "Engine/GameInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "JsonObjectConverter.h"

const FName ULootTableManager::DefaultRegionId = TEXT("Default");

// Content/Data/output/loot_tables.json
const FString ULootTableManager::LootTablesJsonRelativePath = TEXT("Data/output/loot_tables.json");

namespace
{
	// 가중치 배열에서 확률에 비례해 인덱스 하나를 고른다. 모든 가중치가 0 이하이면 INDEX_NONE.
	int32 PickWeightedIndex(const TArray<float>& Weights)
	{
		float Total = 0.f;
		for (const float W : Weights)
		{
			Total += FMath::Max(0.f, W);
		}
		if (Total <= 0.f)
		{
			return INDEX_NONE;
		}

		const float Roll = FMath::FRandRange(0.f, Total);
		float Upto = 0.f;
		for (int32 i = 0; i < Weights.Num(); ++i)
		{
			Upto += FMath::Max(0.f, Weights[i]);
			if (Upto >= Roll)
			{
				return i;
			}
		}
		return Weights.Num() - 1; // 부동소수 오차 대비
	}
}

void ULootTableManager::Initialize(FSubsystemCollectionBase& Collection)
{
	// ItemDataManager는 DrawItem에서 쓴다. 같은 GameInstance 서브시스템이라 먼저 만들어지도록 의존성 선언.
	Collection.InitializeDependency<UItemDataManager>();

	Super::Initialize(Collection);
	bDataLoaded = LoadLootTables();

	if (!bDataLoaded)
	{
		UE_LOG(LogTemp, Error, TEXT("[LootTableManager] 데이터 로드 실패"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[LootTableManager] 로드 완료: 테이블 %d개, 지역 %d개"), ResolvedTables.Num(), RawData.Regions.Num());
		ValidateData();
	}
}

void ULootTableManager::Deinitialize()
{
	RawData = FLootTablesFile();
	ResolvedTables.Empty();
	WarnedUnknownRegions.Empty();
	Super::Deinitialize();
}

bool ULootTableManager::LoadLootTables()
{
	const FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), LootTablesJsonRelativePath);
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FullPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[LootTableManager] 파일 없음: %s"), *FullPath);
		return false;
	}

	if (!FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &RawData, 0, 0))
	{
		UE_LOG(LogTemp, Error, TEXT("[LootTableManager] loot_tables.json 파싱 실패"));
		return false;
	}

	ResolvedTables.Empty();
	for (const TPair<FString, FLootTableDef>& Pair : RawData.Tables)
	{
		FResolvedLootTable Resolved;
		if (ResolveTable(Pair.Key, 0, Resolved))
		{
			ResolvedTables.Add(FName(*Pair.Key), MoveTemp(Resolved));
		}
	}
	return true;
}

bool ULootTableManager::ResolveTable(const FString& TableId, int32 Depth, FResolvedLootTable& Out) const
{
	// parent 순환(A->B->A) 방지. 정상적인 상속은 이 깊이를 넘지 않는다.
	constexpr int32 MaxDepth = 8;
	if (Depth > MaxDepth)
	{
		UE_LOG(LogTemp, Error, TEXT("[LootTableManager] 테이블 '%s': parent 체인이 너무 깊거나 순환함"), *TableId);
		return false;
	}

	const FLootTableDef* Def = RawData.Tables.Find(TableId);
	if (!Def)
	{
		UE_LOG(LogTemp, Error, TEXT("[LootTableManager] parent로 지정된 테이블 '%s'이(가) 없음"), *TableId);
		return false;
	}

	// 부모를 먼저 채운 뒤 자식이 적은 키만 덮어쓴다.
	if (!Def->Parent.IsEmpty() && !ResolveTable(Def->Parent, Depth + 1, Out))
	{
		return false;
	}

	for (const TPair<FString, float>& P : Def->RarityWeights) { Out.RarityWeights.Add(P.Key, P.Value); }
	for (const TPair<FString, float>& P : Def->CategoryWeights) { Out.CategoryWeights.Add(P.Key, P.Value); }
	for (const TPair<FString, float>& P : Def->ItemWeights) { Out.ItemWeights.Add(P.Key, P.Value); }
	for (const FString& Excluded : Def->ExcludedItems) { Out.ExcludedItems.Add(Excluded); }
	return true;
}

void ULootTableManager::ValidateData() const
{
	if (!RawData.Regions.Contains(DefaultRegionId.ToString()))
	{
		UE_LOG(LogTemp, Error, TEXT("[LootTableManager] loot_tables.json에 'Default' 지역이 없음 - RegionId가 없는 지역은 스폰되지 않는다"));
	}

	for (const TPair<FString, FLootRegionDef>& Pair : RawData.Regions)
	{
		if (!ResolvedTables.Contains(FName(*Pair.Value.Table)))
		{
			UE_LOG(LogTemp, Error, TEXT("[LootTableManager] 지역 '%s'이(가) 없는 테이블 '%s'을(를) 가리킴"), *Pair.Key, *Pair.Value.Table);
		}
	}
}

bool ULootTableManager::ResolveRegion(FName RegionId, FName& OutRegionUsed, FName& OutTableId, float& OutSpawnChance) const
{
	const FLootRegionDef* Region = RegionId.IsNone() ? nullptr : RawData.Regions.Find(RegionId.ToString());
	OutRegionUsed = RegionId;

	if (!Region)
	{
		// 비워 둔 RegionId는 의도된 Default라서 경고하지 않는다. 이름은 있는데 데이터에 없으면 오타일 가능성이 크다.
		if (!RegionId.IsNone() && RegionId != DefaultRegionId && !WarnedUnknownRegions.Contains(RegionId))
		{
			WarnedUnknownRegions.Add(RegionId);
			UE_LOG(LogTemp, Warning, TEXT("[LootTableManager] loot_tables.json에 없는 RegionId '%s' - Default로 대체"), *RegionId.ToString());
		}

		OutRegionUsed = DefaultRegionId;
		Region = RawData.Regions.Find(DefaultRegionId.ToString());
	}

	if (!Region)
	{
		return false;
	}

	OutTableId = FName(*Region->Table);
	OutSpawnChance = FMath::Clamp(Region->SpawnChance, 0.f, 1.f);
	return true;
}

bool ULootTableManager::DrawItem(FName TableId, FName& OutItemIndex) const
{
	const FResolvedLootTable* Table = ResolvedTables.Find(TableId);
	const UGameInstance* GameInstance = GetGameInstance();
	const UItemDataManager* ItemMgr = GameInstance ? GameInstance->GetSubsystem<UItemDataManager>() : nullptr;
	if (!Table || !ItemMgr)
	{
		return false;
	}

	// 등급 -> 카테고리 -> (아이템, 가중치 배수) 후보. 테이블 규칙을 통과한 아이템만 들어간다.
	TMap<FString, TMap<FString, TArray<TPair<FName, float>>>> Buckets;

	TArray<FItemData> AllItems;
	ItemMgr->GetAllItems(AllItems);
	for (const FItemData& Item : AllItems)
	{
		const FString ItemKey = Item.Index.ToString();
		if (Table->ExcludedItems.Contains(ItemKey))
		{
			continue;
		}

		const FString RarityKey = Item.RarityId.ToString();
		const float* RarityWeight = Table->RarityWeights.Find(RarityKey);
		if (!RarityWeight || *RarityWeight <= 0.f)
		{
			continue;
		}

		const FString CategoryKey = Item.Category.ToString();
		if (Table->CategoryWeights.Num() > 0)
		{
			const float* CategoryWeight = Table->CategoryWeights.Find(CategoryKey);
			if (!CategoryWeight || *CategoryWeight <= 0.f)
			{
				continue;
			}
		}

		float ItemWeight = 1.f;
		if (const float* Override = Table->ItemWeights.Find(ItemKey))
		{
			if (*Override <= 0.f)
			{
				continue;
			}
			ItemWeight = *Override;
		}

		Buckets.FindOrAdd(RarityKey).FindOrAdd(CategoryKey).Emplace(Item.Index, ItemWeight);
	}

	if (Buckets.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LootTableManager] 테이블 '%s'에서 뽑을 수 있는 아이템이 없음"), *TableId.ToString());
		return false;
	}

	// ---------- 1단계: 등급 ----------
	TArray<FString> RarityKeys;
	TArray<float> RarityWeights;
	for (const TPair<FString, TMap<FString, TArray<TPair<FName, float>>>>& Pair : Buckets)
	{
		RarityKeys.Add(Pair.Key);
		RarityWeights.Add(Table->RarityWeights.FindRef(Pair.Key));
	}
	const int32 RarityPick = PickWeightedIndex(RarityWeights);
	if (RarityPick == INDEX_NONE)
	{
		return false;
	}
	const TMap<FString, TArray<TPair<FName, float>>>& CategoryBuckets = Buckets[RarityKeys[RarityPick]];

	// ---------- 2단계: 카테고리 ----------
	TArray<const TArray<TPair<FName, float>>*> ItemLists;
	TArray<float> CategoryWeights;
	for (const TPair<FString, TArray<TPair<FName, float>>>& Pair : CategoryBuckets)
	{
		ItemLists.Add(&Pair.Value);
		// 카테고리 가중치 맵이 비어 있으면 모든 카테고리를 같은 값(1)으로 본다.
		CategoryWeights.Add(Table->CategoryWeights.Num() > 0 ? Table->CategoryWeights.FindRef(Pair.Key) : 1.f);
	}
	const int32 CategoryPick = PickWeightedIndex(CategoryWeights);
	if (CategoryPick == INDEX_NONE)
	{
		return false;
	}
	const TArray<TPair<FName, float>>& Candidates = *ItemLists[CategoryPick];

	// ---------- 3단계: 아이템 ----------
	TArray<float> ItemWeights;
	for (const TPair<FName, float>& Candidate : Candidates)
	{
		ItemWeights.Add(Candidate.Value);
	}
	const int32 ItemPick = PickWeightedIndex(ItemWeights);
	if (ItemPick == INDEX_NONE)
	{
		return false;
	}

	OutItemIndex = Candidates[ItemPick].Key;
	return true;
}
