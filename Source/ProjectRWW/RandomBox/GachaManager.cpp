#include "GachaManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "JsonObjectConverter.h"

// 프로젝트 폴더 구조에 맞춰 조정: Content/Data/output/gacha_tables.json
const FString UGachaManager::GachaTablesJsonRelativePath = TEXT("Data/output/gacha_tables.json");

void UGachaManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bDataLoaded = LoadGachaTables();

	if (!bDataLoaded)
	{
		UE_LOG(LogTemp, Error, TEXT("[GachaManager] 데이터 로드 실패"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[GachaManager] 로드 완료: 박스 %d개"), GachaTables.Boxes.Num());
	}
}

void UGachaManager::Deinitialize()
{
	GachaTables = FGachaTables();
	Super::Deinitialize();
}

bool UGachaManager::LoadGachaTables()
{
	// gacha_tables.json은 배열이 아니라 객체 하나이므로 JsonObjectStringToUStruct 사용.
	const FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), GachaTablesJsonRelativePath);
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FullPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[GachaManager] 파일 없음: %s"), *FullPath);
		return false;
	}

	if (!FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &GachaTables, 0, 0))
	{
		UE_LOG(LogTemp, Error, TEXT("[GachaManager] gacha_tables.json 파싱 실패"));
		return false;
	}
	return true;
}

bool UGachaManager::DrawFromBox(FName BoxId, FName& OutCategory, FName& OutItemIndex, FName& OutRarityId) const
{
	const FGachaBox* Box = GachaTables.Boxes.Find(BoxId.ToString());
	if (!Box)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GachaManager] BoxID를 찾을 수 없음: %s"), *BoxId.ToString());
		return false;
	}

	// ---------- 1단계: 등급 추첨 (RarityDropWeight, 아이템이 있는 등급만) ----------
	float RarityTotal = 0.f;
	TArray<TPair<FString, float>> RarityPairs;
	for (const auto& PoolPair : Box->Pools)
	{
		if (PoolPair.Value.Items.Num() > 0)
		{
			RarityPairs.Add(TPair<FString, float>(PoolPair.Key, PoolPair.Value.RarityDropWeight));
			RarityTotal += PoolPair.Value.RarityDropWeight;
		}
	}
	if (RarityPairs.Num() == 0 || RarityTotal <= 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GachaManager] BoxID '%s'에 뽑을 수 있는 등급이 없음"), *BoxId.ToString());
		return false;
	}

	float R = FMath::FRandRange(0.f, RarityTotal);
	float Upto = 0.f;
	FString ChosenRarity = RarityPairs.Last().Key; // 부동소수 오차 대비 fallback
	for (const auto& Pair : RarityPairs)
	{
		Upto += Pair.Value;
		if (Upto >= R)
		{
			ChosenRarity = Pair.Key;
			break;
		}
	}

	// ---------- 2단계: 아이템 추첨 (선택된 등급 내 ItemDropWeight) ----------
	const FGachaRarityPool* Pool = Box->Pools.Find(ChosenRarity);
	if (!Pool || Pool->Items.Num() == 0)
	{
		return false;
	}

	float ItemTotal = 0.f;
	for (const FGachaPoolItem& Item : Pool->Items)
	{
		ItemTotal += Item.Weight;
	}
	if (ItemTotal <= 0.f)
	{
		return false;
	}

	float R2 = FMath::FRandRange(0.f, ItemTotal);
	float Upto2 = 0.f;
	const FGachaPoolItem* ChosenItem = &Pool->Items.Last(); // fallback
	for (const FGachaPoolItem& Item : Pool->Items)
	{
		Upto2 += Item.Weight;
		if (Upto2 >= R2)
		{
			ChosenItem = &Item;
			break;
		}
	}

	OutCategory = ChosenItem->Category;
	OutItemIndex = ChosenItem->Index;
	OutRarityId = FName(*ChosenRarity);
	return true;
}
