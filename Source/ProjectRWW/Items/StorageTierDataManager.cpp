// Copyright Epic Games, Inc. All Rights Reserved.

#include "StorageTierDataManager.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

const FString UStorageTierDataManager::StorageTiersJsonRelativePath = TEXT("Data/output/storage_tiers.json");

void UStorageTierDataManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadTiers();
}

bool UStorageTierDataManager::LoadTiers()
{
	const FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), StorageTiersJsonRelativePath);
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FullPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[ProjectRWW] storage_tiers.json 로드 실패: %s"), *FullPath);
		return false;
	}

	TArray<FStorageTierData> Tiers;
	if (!FJsonObjectConverter::JsonArrayStringToUStruct(JsonString, &Tiers, 0, 0))
	{
		UE_LOG(LogTemp, Error, TEXT("[ProjectRWW] storage_tiers.json 파싱 실패"));
		return false;
	}

	for (const FStorageTierData& TierData : Tiers)
	{
		TierMap.Add(TierData.Tier, TierData);
	}
	return true;
}

bool UStorageTierDataManager::GetTierData(int32 Tier, FStorageTierData& OutData) const
{
	if (const FStorageTierData* Found = TierMap.Find(Tier))
	{
		OutData = *Found;
		return true;
	}
	return false;
}
