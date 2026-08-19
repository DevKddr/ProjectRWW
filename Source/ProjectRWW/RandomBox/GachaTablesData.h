#pragma once

#include "CoreMinimal.h"
#include "GachaTablesData.generated.h"

// gacha_tables.json 은 배열이 아니라 객체 하나이므로
// JsonObjectStringToUStruct로 파싱한다 (JsonArrayStringToUStruct 아님에 유의).

USTRUCT(BlueprintType)
struct FGachaRarityInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gacha")
	FName RarityId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gacha")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gacha")
	FString Color;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gacha")
	float DropWeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gacha")
	int32 SellValue = 0;
};

USTRUCT(BlueprintType)
struct FGachaPoolItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gacha")
	FName Category;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gacha")
	FName Index;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gacha")
	float Weight = 0.f;
};

USTRUCT(BlueprintType)
struct FGachaRarityPool
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gacha")
	float RarityDropWeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gacha")
	TArray<FGachaPoolItem> Items;
};

USTRUCT(BlueprintType)
struct FGachaBox
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gacha")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gacha")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gacha")
	bool IsActive = true;

	// Key = RarityID (Common/Rare/Epic)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gacha")
	TMap<FString, FGachaRarityPool> Pools;
};

USTRUCT(BlueprintType)
struct FGachaTables
{
	GENERATED_BODY()

	// Key = RarityID (gacha_tables.json에 자체 내장된 등급 사본 - 확률 계산 전용, RarityDataManager와는 별개)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gacha")
	TMap<FString, FGachaRarityInfo> Rarities;

	// Key = BoxID
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gacha")
	TMap<FString, FGachaBox> Boxes;
};
