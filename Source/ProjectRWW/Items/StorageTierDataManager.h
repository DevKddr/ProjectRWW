// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StorageTierData.h"
#include "StorageTierDataManager.generated.h"

// storage_tiers.json 전용 매니저. 등급 번호로 가로/세로 칸 수를 조회한다.
UCLASS()
class PROJECTRWW_API UStorageTierDataManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "Storage")
	bool GetTierData(int32 Tier, FStorageTierData& OutData) const;

private:
	bool LoadTiers();

	TMap<int32, FStorageTierData> TierMap;

	static const FString StorageTiersJsonRelativePath;
};
