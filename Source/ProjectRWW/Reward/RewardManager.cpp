// Copyright Epic Games, Inc. All Rights Reserved.

#include "RewardManager.h"
#include "Database/MainPlayerDataRepository.h"
#include "Engine/GameInstance.h"
#include "Items/ItemDataManager.h"
#include "Items/MainInventoryComponent.h"
#include "JsonObjectConverter.h"
#include "Misc/CoreMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Player/MainPlayerController.h"
#include "RandomBox/GachaManager.h"

bool URewardManager::ShouldCreateSubsystem(UObject* Outer) const
{
	// 보상은 서버만 지급한다. 클라이언트 전용 실행에서는 만들지 않는다.
	return !IsRunningClientOnly() && Super::ShouldCreateSubsystem(Outer);
}

void URewardManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	PlayerDataRepository = NewObject<UMainPlayerDataRepository>(this);
	LoadRewardTable();
}

void URewardManager::LoadRewardTable()
{
	const FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/output/rewards.json"));
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FullPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[ProjectRWW] rewards.json 로드 실패: %s"), *FullPath);
		return;
	}

	if (!FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &Table, 0, 0))
	{
		UE_LOG(LogTemp, Error, TEXT("[ProjectRWW] rewards.json 파싱 실패"));
		Table = FRewardTable();
		return;
	}

	// 손으로 편집하는 파일이라, 잘못된 항목은 경고를 남기고 뺀다.
	for (auto It = Table.Rewards.CreateIterator(); It; ++It)
	{
		const FGameReward& Reward = It.Value();
		const bool bEmptyReward = Reward.Credit == 0 && Reward.Items.IsEmpty() && Reward.GachaBoxIds.IsEmpty();
		if (Reward.Credit < 0 || bEmptyReward)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ProjectRWW] 보상 항목 무시(빈 보상 또는 음수 Credit): %s"), *It.Key());
			It.RemoveCurrent();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] 보상 표 로드: %d개"), Table.Rewards.Num());
}

bool URewardManager::HasReward(FName RewardId) const
{
	return Table.Rewards.Contains(RewardId.ToString());
}

FGameReward URewardManager::GrantReward(AMainPlayerController* Player, const TArray<FName>& RewardIds)
{
	FGameReward Granted;
	if (!Player || !Player->HasAuthority())
	{
		return Granted;
	}

	// 1) 조회해서 한 묶음으로 합친다.
	FGameReward Reward;
	for (const FName& RewardId : RewardIds)
	{
		if (const FGameReward* Found = Table.Rewards.Find(RewardId.ToString()))
		{
			Reward.Append(*Found);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[ProjectRWW] 보상 표에 없는 RewardId: %s"), *RewardId.ToString());
		}
	}

	// 2) Credit은 사망/탈출과 상관없이 유지된다. 메모리와 DB를 같이 올린다.
	// DB 쪽이 실패해도 이후 사망/탈출 정산이 메모리의 절대값을 저장해 따라잡는다.
	if (Reward.Credit > 0)
	{
		Player->PlayerRecord.Credit += Reward.Credit;
		Granted.Credit = Reward.Credit;
		if (PlayerDataRepository && !Player->PlayerRecord.PlayerID.IsEmpty()
			&& !PlayerDataRepository->AddCredit(Player->PlayerRecord.PlayerID, Reward.Credit))
		{
			UE_LOG(LogTemp, Warning, TEXT("[ProjectRWW] 보상 Credit 즉시 저장 실패(정산 때 반영됨): %s"), *Player->PlayerRecord.PlayerID);
		}
		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] 보상: %s Credit +%d (합계 %d)"),
			*Player->PlayerRecord.PlayerID, Reward.Credit, Player->PlayerRecord.Credit);
	}

	// 3) 상자는 추첨해서 아이템 목록에 합친다. 이후 지급 경로는 하나다.
	TArray<FName> ItemsToAdd = Reward.Items;
	const UGachaManager* GachaMgr = GetGameInstance()->GetSubsystem<UGachaManager>();
	for (const FName& BoxId : Reward.GachaBoxIds)
	{
		FName Category, ItemIndex, RarityId;
		if (GachaMgr && GachaMgr->DrawFromBox(BoxId, Category, ItemIndex, RarityId))
		{
			ItemsToAdd.Add(ItemIndex);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[ProjectRWW][Gacha] 추첨 실패: BoxId=%s, Target=%s"), *BoxId.ToString(), *Player->PlayerRecord.PlayerID);
		}
	}

	// 4) 아이템 지급. 실제로 들어간 것만 결과에 담는다.
	const UItemDataManager* ItemMgr = GetGameInstance()->GetSubsystem<UItemDataManager>();
	for (const FName& ItemIndex : ItemsToAdd)
	{
		// AddItem은 없는 Index도 그대로 넣기 때문에 여기서 먼저 거른다.
		FItemData ItemData;
		if (!ItemMgr || !ItemMgr->GetItemData(ItemIndex, ItemData))
		{
			UE_LOG(LogTemp, Warning, TEXT("[ProjectRWW] 보상에 없는 아이템 Index: %s"), *ItemIndex.ToString());
			continue;
		}
		if (!Player->InventoryComponent || Player->InventoryComponent->AddItem(ItemIndex) == INDEX_NONE)
		{
			UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] 인벤토리 가득 참 - 보상 유실: %s, Item=%s"), *Player->PlayerRecord.PlayerID, *ItemIndex.ToString());
			continue;
		}
		Granted.Items.Add(ItemIndex);
	}

	return Granted;
}
