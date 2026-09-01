// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainSlotContainerComponent.h"
#include "Net/UnrealNetwork.h"

UMainSlotContainerComponent::UMainSlotContainerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UMainSlotContainerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UMainSlotContainerComponent, Slots, COND_OwnerOnly);
}

void UMainSlotContainerComponent::OnRep_Slots()
{
	OnSlotsChanged.Broadcast();
}

int32 UMainSlotContainerComponent::AddItem(FName ItemIndex)
{
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		if (Slots[i].IsEmpty())
		{
			Slots[i].ItemIndex = ItemIndex;
			Slots[i].CurrentAmmo = -1;

			// 리슨 서버 호스트는 자기 자신에게는 리플리케이션(OnRep)이 안 오므로,
			// 여기서 직접 알려줘야 본인 화면도 즉시 갱신된다.
			OnSlotsChanged.Broadcast();
			return i;
		}
	}
	return INDEX_NONE;
}

FInventorySlot UMainSlotContainerComponent::TakeSlot(int32 SlotIndex)
{
	if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty())
	{
		return FInventorySlot();
	}

	const FInventorySlot Result = Slots[SlotIndex];
	Slots[SlotIndex] = FInventorySlot();
	return Result;
}

bool UMainSlotContainerComponent::PlaceSlot(int32 SlotIndex, const FInventorySlot& SlotData)
{
	if (!Slots.IsValidIndex(SlotIndex) || !Slots[SlotIndex].IsEmpty())
	{
		return false;
	}

	Slots[SlotIndex] = SlotData;
	return true;
}

void UMainSlotContainerComponent::Server_MoveSlot_Implementation(int32 FromSlot, int32 ToSlot, UMainSlotContainerComponent* DestComponent)
{
	if (!DestComponent)
	{
		DestComponent = this;
	}

	// 서로 다른 플레이어 소유 컴포넌트끼리는 거부 - 악의적인 클라이언트가 남의
	// 인벤토리/창고에 접근하는 것을 막는다.
	if (DestComponent->GetOwner() != GetOwner())
	{
		return;
	}

	// FromSlot은 내 배열, ToSlot은 목적지 배열 기준으로 각각 범위를 검사한다. 이 검사가
	// 없으면 TakeSlot(FromSlot)으로 아이템을 이미 꺼낸 뒤 PlaceSlot(ToSlot, ...)이
	// 잘못된 인덱스라 조용히 실패해서, 그 아이템이 그대로 사라지는 버그가 생긴다.
	if (!Slots.IsValidIndex(FromSlot) || !DestComponent->Slots.IsValidIndex(ToSlot))
	{
		return;
	}

	if (DestComponent == this && FromSlot == ToSlot)
	{
		return;
	}

	const FInventorySlot A = TakeSlot(FromSlot);
	if (A.IsEmpty())
	{
		// 출발지가 애초에 비어있었다 - 옮길 게 없으니 여기서 중단한다.
		return;
	}

	const FInventorySlot B = DestComponent->TakeSlot(ToSlot);
	DestComponent->PlaceSlot(ToSlot, A);
	PlaceSlot(FromSlot, B);

	DestComponent->OnSlotsChanged.Broadcast();
	if (DestComponent != this)
	{
		OnSlotsChanged.Broadcast();
	}
}

void UMainSlotContainerComponent::Server_AddItem_Implementation(FName ItemIndex)
{
	AddItem(ItemIndex);
}
