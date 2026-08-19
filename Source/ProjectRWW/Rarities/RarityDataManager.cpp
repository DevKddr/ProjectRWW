#include "RarityDataManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "JsonObjectConverter.h"

// 프로젝트 폴더 구조에 맞춰 조정: Content/Data/output/rarities.json
const FString URarityDataManager::RaritiesJsonRelativePath = TEXT("Data/output/rarities.json");

void URarityDataManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bDataLoaded = LoadRarities();

	if (!bDataLoaded)
	{
		UE_LOG(LogTemp, Error, TEXT("[RarityDataManager] 데이터 로드 실패"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[RarityDataManager] 로드 완료: 등급 %d개"), RarityMap.Num());
	}
}

void URarityDataManager::Deinitialize()
{
	RarityMap.Empty();
	Super::Deinitialize();
}

bool URarityDataManager::LoadRarities()
{
	const FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), RaritiesJsonRelativePath);
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FullPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[RarityDataManager] 파일 없음: %s"), *FullPath);
		return false;
	}

	TArray<FRarityData> Parsed;
	if (!FJsonObjectConverter::JsonArrayStringToUStruct(JsonString, &Parsed, 0, 0))
	{
		UE_LOG(LogTemp, Error, TEXT("[RarityDataManager] rarities.json 파싱 실패"));
		return false;
	}

	RarityMap.Empty();
	for (const FRarityData& Entry : Parsed)
	{
		RarityMap.Add(Entry.RarityId, Entry);
	}
	return true;
}

bool URarityDataManager::GetRarityData(FName RarityId, FRarityData& OutData) const
{
	if (const FRarityData* Found = RarityMap.Find(RarityId))
	{
		OutData = *Found;
		return true;
	}
	return false;
}

TArray<FRarityData> URarityDataManager::GetAllRarities() const
{
	TArray<FRarityData> Result;
	RarityMap.GenerateValueArray(Result);
	return Result;
}
