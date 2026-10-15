// Copyright Epic Games, Inc. All Rights Reserved.

#include "ItemDataManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "JsonObjectConverter.h"

// 프로젝트 폴더 구조에 맞춰 조정: Content/Data/output/items.json
const FString UItemDataManager::ItemsJsonRelativePath = TEXT("Data/output/items.json");

// StringTable 에셋 경로. FText::FromStringTable()이 에셋 경로 ID를 받으면 필요할 때 에셋을 로드해서 쓴다.
// 주의: /Game/Localization 폴더에 두면 안 된다(Dashboard가 수집에서 자동 제외). Dashboard 에셋 수집 경로(Content/Core/*, Content/UI/*)에 포함된 폴더여야 한다.
const FName UItemDataManager::ItemStringTableId = TEXT("/Game/Core/Text/ST_Items.ST_Items");

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

void UItemDataManager::GetAllItems(TArray<FItemData>& OutItems) const
{
	ItemMap.GenerateValueArray(OutItems);
}

FText UItemDataManager::GetItemNameText(FName Index) const
{
	const FItemData* Found = ItemMap.Find(Index);
	if (!Found || Found->NameKey.IsNone())
	{
		return FText::GetEmpty();
	}
	// FindOrFullyLoad: 기본 정책은 StringTable 에셋을 비동기로 로드하고 그 전엔 "<MISSING STRING TABLE ENTRY>"를
	// 돌려준다. UI가 매 프레임 새 FText를 만들어도 처음부터 올바른 문장이 나오도록 동기 로드한다.
	return FText::FromStringTable(ItemStringTableId, Found->NameKey.ToString(), EStringTableLoadingPolicy::FindOrFullyLoad);
}

FText UItemDataManager::GetItemDescriptionText(FName Index) const
{
	const FItemData* Found = ItemMap.Find(Index);
	if (!Found || Found->DescriptionKey.IsNone())
	{
		return FText::GetEmpty();
	}
	return FText::FromStringTable(ItemStringTableId, Found->DescriptionKey.ToString(), EStringTableLoadingPolicy::FindOrFullyLoad);
}
