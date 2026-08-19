#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "RarityData.generated.h"

// rarities.json 한 항목과 1:1 대응.
USTRUCT(BlueprintType)
struct FRarityData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity")
	FName RarityId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity")
	FString Color;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity")
	float DropWeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity")
	int32 SellValue = 0;
};
