// Copyright Epic Games, Inc. All Rights Reserved.

#include "ItemDataManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "JsonObjectConverter.h"

// 프로젝트 폴더 구조에 맞춰 조정: Content/Data/output/items.json
const FString UItemDataManager::ItemsJsonRelativePath = TEXT("Data/output/items.json");

void UItemDataManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bDataLoaded = LoadItems();

	if (!bDataLoaded)
	{
		UE_LOG(LogTemp, Error, TEXT("[ItemDataManager] 데이터 로드 실패"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[ItemDataManager] 로드 완료: 아이템 %d개"), ItemMap.Num());
	}
}

void UItemDataManager::Deinitialize()
{
	ItemMap.Empty();
	Super::Deinitialize();
}

bool UItemDataManager::LoadItems()
{
	const FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), ItemsJsonRelativePath);
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FullPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[ItemDataManager] 파일 없음: %s"), *FullPath);
		return false;
	}

	TArray<FItemData> Parsed;
	if (!FJsonObjectConverter::JsonArrayStringToUStruct(JsonString, &Parsed, 0, 0))
	{
		UE_LOG(LogTemp, Error, TEXT("[ItemDataManager] items.json 파싱 실패"));
		return false;
	}

	ItemMap.Empty();
	for (const FItemData& Entry : Parsed)
	{
		ItemMap.Add(Entry.Index, Entry);
	}
	return true;
}

bool UItemDataManager::GetItemData(FName Index, FItemData& OutData) const
{
	if (const FItemData* Found = ItemMap.Find(Index))
	{
		OutData = *Found;
		return true;
	}
	return false;
}
