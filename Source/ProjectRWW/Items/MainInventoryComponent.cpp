// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainInventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerController.h"
#include "Player/MainCharacter.h"
#include "Combat/MainWeaponComponent.h"
#include "ItemDataManager.h"

UMainInventoryComponent::UMainInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UMainInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UMainInventoryComponent, InventorySlots, COND_OwnerOnly);
}

void UMainInventoryComponent::OnRep_InventorySlots()
{
	OnInventoryChanged.Broadcast();
}

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

bool UMainInventoryComponent::AddItem(FName ItemIndex)
{
	for (int32 i = 0; i < InventorySlots.Num(); ++i)
	{
		if (InventorySlots[i].IsEmpty())
		{
			InventorySlots[i].ItemIndex = ItemIndex;
			InventorySlots[i].CurrentAmmo = -1;

			// 리슨 서버 호스트는 자기 자신에게는 리플리케이션(OnRep)이 안 오므로,
			// 여기서 직접 알려줘야 본인 화면도 즉시 갱신된다. 진짜 원격 클라이언트는
			// 이 함수를 직접 실행하지 않으니(서버에서만 실행됨) 중복되지 않는다.
			OnInventoryChanged.Broadcast();

			// 지금 선택된 슬롯(핫바)에 새 아이템이 막 들어왔으면, MoveItem()과 같은
			// 규칙으로 즉시 장착까지 반영한다 - 안 그러면 빈손으로 선택돼있던 슬롯에
			// 아이템이 들어와도 다시 핫키를 눌러야만 실제로 장착된다.
			if (EquippedSlotIndex == i)
			{
				EquipItem(i);
			}
			return true;
		}
	}
	return false;
}

bool UMainInventoryComponent::MoveItem(int32 FromSlot, int32 ToSlot)
{
	if (!InventorySlots.IsValidIndex(FromSlot) || !InventorySlots.IsValidIndex(ToSlot) || FromSlot == ToSlot)
	{
		return false;
	}

	if (EquippedSlotIndex == FromSlot || EquippedSlotIndex == ToSlot)
	{
		// 장착 중이던 "슬롯 번호 자체"를 기억해둔다 - 스왑 후 그 자리에 새로 들어온
		// 아이템을 다시 장착해서, 핫바의 그 칸이 계속 활성 상태를 유지하게 한다.
		const int32 SlotToReequip = EquippedSlotIndex;

		UnequipItem();
		Swap(InventorySlots[FromSlot], InventorySlots[ToSlot]);
		EquipItem(SlotToReequip);
		OnInventoryChanged.Broadcast();  // 리슨 서버 호스트 본인 화면 갱신용 (AddItem과 동일한 이유)
		return true;
	}

	Swap(InventorySlots[FromSlot], InventorySlots[ToSlot]);
	OnInventoryChanged.Broadcast();
	return true;
}

void UMainInventoryComponent::EquipItem(int32 SlotIndex)
{
	if (!InventorySlots.IsValidIndex(SlotIndex))
	{
		return;
	}

	// 먼저 손에 든 걸 내리고, 이 슬롯을 "지금 선택된 슬롯"으로 기록한다. 슬롯이
	// 비어있어도(빈손 상태로) 선택 자체는 유효하다 - 마인크래프트 핫바처럼 빈 칸을
	// 선택하면 실제로 빈손이 된다. 이렇게 하면 사망 등으로 인벤토리가 통째로
	// 비워져도 다음 EquipItem() 호출 때 EquippedSlotIndex가 항상 최신 상태를 따라간다.
	UnequipItem();
	EquippedSlotIndex = SlotIndex;

	if (InventorySlots[SlotIndex].IsEmpty())
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
	if (!ItemDataManager->GetItemData(InventorySlots[SlotIndex].ItemIndex, ItemData))
	{
		return;
	}

	if (ItemData.Category == TEXT("Weapon"))
	{
		WeaponComponent->EquipWeapon(ItemData.Index, InventorySlots[SlotIndex].CurrentAmmo);
	}
	// else: 다른 카테고리는 아직 "장착" 로직 없음 (빈손 취급, EquippedSlotIndex는 이미 설정됨)
}

void UMainInventoryComponent::UnequipItem()
{
	if (!InventorySlots.IsValidIndex(EquippedSlotIndex))
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
			InventorySlots[EquippedSlotIndex].CurrentAmmo = WeaponComponent->GetCurrentAmmo();
		}
		WeaponComponent->UnequipWeapon();
	}

	EquippedSlotIndex = -1;
}

void UMainInventoryComponent::Server_EquipItem_Implementation(int32 SlotIndex)
{
	EquipItem(SlotIndex);
}

void UMainInventoryComponent::Server_MoveItem_Implementation(int32 FromSlot, int32 ToSlot)
{
	MoveItem(FromSlot, ToSlot);
}

void UMainInventoryComponent::Server_AddItem_Implementation(FName ItemIndex)
{
	AddItem(ItemIndex);
}
