// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LootTablesData.generated.h"

// loot_tables.json은 배열이 아니라 객체 하나이므로 JsonObjectStringToUStruct로 파싱한다.
// 필드 이름은 JSON 키와 대소문자 무시로 매칭된다(rarityWeights -> RarityWeights).

// 루트 테이블 하나. "이 테이블이 쓰이는 지역에서 어떤 아이템이 얼마나 나오는가"만 정의한다.
// 아이템 자체의 등급/카테고리는 items.json이 갖고 있으므로 여기서 복제하지 않는다 -
// 아이템을 추가하면 가중치에 해당 등급/카테고리가 있는 한 자동으로 후보가 된다.
USTRUCT()
struct FLootTableDef
{
	GENERATED_BODY()

	// 비어 있지 않으면 그 테이블의 값을 먼저 가져온 뒤, 이 테이블에 적힌 키만 덮어쓴다.
	UPROPERTY()
	FString Parent;

	// 1단계(등급) 가중치. Key = RarityId. 여기 없거나 0 이하인 등급의 아이템은 나오지 않는다.
	UPROPERTY()
	TMap<FString, float> RarityWeights;

	// 2단계(카테고리) 가중치. Key = Category(Weapon/Item...). 맵이 비어 있으면 모든 카테고리를 1로 본다.
	// 맵이 있는데 카테고리가 빠져 있거나 0 이하이면 그 카테고리의 아이템은 나오지 않는다.
	UPROPERTY()
	TMap<FString, float> CategoryWeights;

	// 3단계(아이템) 가중치 배수. Key = 아이템 Index. 없으면 1, 0 이하이면 이 테이블에서 제외.
	UPROPERTY()
	TMap<FString, float> ItemWeights;

	// 이 테이블에서 절대 나오면 안 되는 아이템 Index(예: UNARMED). 부모의 제외 목록과 합쳐진다.
	UPROPERTY()
	TArray<FString> ExcludedItems;
};

// 지역(RegionId)이 어떤 테이블을 쓰는지. 레벨의 ALootRegion은 RegionId만 갖고 테이블은 모른다 -
// 지역 -> 테이블 매핑은 전부 이 데이터에 있어서 밸런스를 레벨을 열지 않고 바꿀 수 있다.
USTRUCT()
struct FLootRegionDef
{
	GENERATED_BODY()

	UPROPERTY()
	FString Table;

	// 이 지역의 스폰 포인트 하나가 실제로 아이템을 만들 확률(0~1).
	UPROPERTY()
	float SpawnChance = 1.0f;
};

USTRUCT()
struct FLootTablesFile
{
	GENERATED_BODY()

	// Key = TableId
	UPROPERTY()
	TMap<FString, FLootTableDef> Tables;

	// Key = RegionId. "Default"는 반드시 있어야 한다(RegionId가 없거나 모르는 지역의 기본값).
	UPROPERTY()
	TMap<FString, FLootRegionDef> Regions;
};
