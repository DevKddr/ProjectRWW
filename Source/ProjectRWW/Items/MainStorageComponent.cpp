// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainStorageComponent.h"
#include "Net/UnrealNetwork.h"
#include "StorageTierDataManager.h"

void UMainStorageComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UMainStorageComponent, StorageTier, COND_OwnerOnly);
}

bool UMainStorageComponent::GetTierData(int32 Tier, FStorageTierData& OutData) const
{
	const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const UStorageTierDataManager* TierManager = GameInstance ? GameInstance->GetSubsystem<UStorageTierDataManager>() : nullptr;
	return TierManager && TierManager->GetTierData(Tier, OutData);
}

void UMainStorageComponent::ApplyTier(int32 NewTier)
{
	StorageTier = NewTier;

	FStorageTierData TierData;
	const int32 SlotCount = GetTierData(NewTier, TierData) ? TierData.Rows * TierData.Cols : Slots.Num();
	Slots.SetNum(SlotCount);
}

int32 UMainStorageComponent::GetColumnCount() const
{
	FStorageTierData TierData;
	if (GetTierData(StorageTier, TierData) && TierData.Cols > 0)
	{
		return TierData.Cols;
	}
	return Slots.Num(); // 조회 실패 시 한 줄로라도 표시되게.
}
