#pragma once

#include "CoreMinimal.h"
#include "Items/ItemData.h"
#include "GachaTablesData.generated.h"

// gacha_tables.json은 배열이 아니라 객체 하나이므로
// JsonObjectStringToUStruct로 파싱한다 (JsonArrayStringToUStruct 아님에 유의).
//
// 예전 버전과 달리 등급 자체의 정의(DisplayName/Color/SellValue)를 여기서 복제해서
// 들고 있지 않는다 - RandomBoxConverter.py가 이미 items.json을 참조해서 등급/카테고리를
// 검증한 뒤 만든 결과물이라, 이 파일은 오직 "확률(가중치)"만 책임진다.

USTRUCT(BlueprintType)
struct FGachaPoolItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gacha")
	FName Category;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gacha")
	FName Index;

	// 2단계 추첨 가중치. 같은 등급 풀 안의 다른 아이템들과 비교되는 값.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gacha")
	float Weight = 0.f;
};

USTRUCT(BlueprintType)
struct FGachaRarityPool
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gacha")
	TArray<FGachaPoolItem> Items;
};

USTRUCT(BlueprintType)
struct FGachaBox
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gacha")
	FLocalizedPair DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gacha")
	FLocalizedPair Description;

	// 1단계 추첨(등급) 가중치. Key = RarityId. rarities.json/RarityDataManager는
	// 폐기되었으므로 등급 확률은 오직 이 박스 전용 값만 존재한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gacha")
	TMap<FString, float> RarityWeights;

	// 2단계 추첨(아이템) 대상. Key = RarityId.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gacha")
	TMap<FString, FGachaRarityPool> Pools;
};

USTRUCT(BlueprintType)
struct FGachaTables
{
	GENERATED_BODY()

	// Key = BoxId
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gacha")
	TMap<FString, FGachaBox> Boxes;
};
