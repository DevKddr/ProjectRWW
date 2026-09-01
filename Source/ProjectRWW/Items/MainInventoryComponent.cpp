// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainInventoryComponent.h"
#include "GameFramework/PlayerController.h"
#include "Player/MainCharacter.h"
#include "Combat/MainWeaponComponent.h"
#include "ItemDataManager.h"

UMainWeaponComponent* UMainInventoryComponent::GetWeaponComponent() const
{
	if (const APlayerController* PC = Cast<APlayerController>(GetOwner()))
	{
		if (const AMainCharacter* Character = Cast<AMainCharacter>(PC->GetPawn()))
		{
			return Character->WeaponComponent;
		}
	}
	return nullptr;
}

int32 UMainInventoryComponent::AddItem(FName ItemIndex)
{
	const int32 FilledSlot = Super::AddItem(ItemIndex);

	// 지금 선택된 슬롯(핫바)에 새 아이템이 막 들어왔으면, 즉시 장착까지 반영한다 -
	// 안 그러면 빈손으로 선택돼있던 슬롯에 아이템이 들어와도 다시 핫키를 눌러야만
	// 실제로 장착된다.
	if (FilledSlot != INDEX_NONE && FilledSlot == EquippedSlotIndex)
	{
		EquipItem(EquippedSlotIndex);
	}
	return FilledSlot;
}

FInventorySlot UMainInventoryComponent::TakeSlot(int32 SlotIndex)
{
	if (SlotIndex == EquippedSlotIndex)
	{
		// 먼저 손에서 내려서 탄약을 슬롯에 정확히 기록한 다음 꺼낸다.
		UnequipItem();
	}
	return Super::TakeSlot(SlotIndex);
}

bool UMainInventoryComponent::PlaceSlot(int32 SlotIndex, const FInventorySlot& SlotData)
{
	const bool bSuccess = Super::PlaceSlot(SlotIndex, SlotData);
	if (bSuccess && SlotIndex == EquippedSlotIndex)
	{
		// 채워 넣은 자리가 지금 선택된 슬롯이면 바로 장착한다.
		EquipItem(SlotIndex);
	}
	return bSuccess;
}

void UMainInventoryComponent::EquipItem(int32 SlotIndex)
{
	if (!Slots.IsValidIndex(SlotIndex))
	{
		return;
	}

	// 먼저 손에 든 걸 내리고, 이 슬롯을 "지금 선택된 슬롯"으로 기록한다. 슬롯이
	// 비어있어도(빈손 상태로) 선택 자체는 유효하다 - 마인크래프트 핫바처럼 빈 칸을
	// 선택하면 실제로 빈손이 된다.
	UnequipItem();
	EquippedSlotIndex = SlotIndex;

	if (Slots[SlotIndex].IsEmpty())
	{
		return;
	}

	UItemDataManager* ItemDataManager = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<UItemDataManager>()
		: nullptr;
	UMainWeaponComponent* WeaponComponent = GetWeaponComponent();
	if (!ItemDataManager || !WeaponComponent)
	{
		return;
	}

	FItemData ItemData;
	if (!ItemDataManager->GetItemData(Slots[SlotIndex].ItemIndex, ItemData))
	{
		return;
	}

	if (ItemData.Category == TEXT("Weapon"))
	{
		WeaponComponent->EquipWeapon(ItemData.Index, Slots[SlotIndex].CurrentAmmo);
	}
	// else: 다른 카테고리는 아직 "장착" 로직 없음 (빈손 취급, EquippedSlotIndex는 이미 설정됨)
}

void UMainInventoryComponent::UnequipItem()
{
	if (!Slots.IsValidIndex(EquippedSlotIndex))
	{
		return;  // 애초에 장착 중인 게 없음
	}

	if (UMainWeaponComponent* WeaponComponent = GetWeaponComponent())
	{
		// 실제로 뭔가 장착돼 있었을 때만 탄약을 저장한다. 그렇지 않으면(빈손인 채로
		// EquippedSlotIndex만 유효한 경우) WeaponComponent의 탄약 0을 그 슬롯에 덮어써서,
		// 그 슬롯에 막 들어온 새 아이템의 "-1(=미장착, 다음엔 가득 채움)" 값을 오염시킨다.
		if (WeaponComponent->HasWeaponEquipped())
		{
			Slots[EquippedSlotIndex].CurrentAmmo = WeaponComponent->GetCurrentAmmo();
		}
		WeaponComponent->UnequipWeapon();
	}

	// EquippedSlotIndex는 여기서 리셋하지 않는다 - TakeSlot()이 "지금 선택된 슬롯을
	// 꺼낸 뒤에도 어디가 선택돼있었는지"를 계속 알아야 PlaceSlot()이 재장착할 자리를
	// 정확히 판단할 수 있다. EquipItem()이 새 슬롯을 선택할 때 어차피 새 값으로 덮어쓴다.
}

void UMainInventoryComponent::Server_EquipItem_Implementation(int32 SlotIndex)
{
	EquipItem(SlotIndex);
}
