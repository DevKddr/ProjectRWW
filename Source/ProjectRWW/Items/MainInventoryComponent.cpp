// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainInventoryComponent.h"
#include "GameFramework/PlayerController.h"
#include "Player/MainCharacter.h"
#include "Combat/MainWeaponComponent.h"
#include "ItemDataManager.h"
#include "Net/UnrealNetwork.h"

UMainWeaponComponent* UMainInventoryComponent::GetWeaponComponent() const
{
	const AMainCharacter* Character = GetMainCharacter();
	return Character ? Character->WeaponComponent : nullptr;
}

AMainCharacter* UMainInventoryComponent::GetMainCharacter() const
{
	if (const APlayerController* PC = Cast<APlayerController>(GetOwner()))
	{
		return Cast<AMainCharacter>(PC->GetPawn());
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

	AMainCharacter* OwningCharacter = GetMainCharacter();

	// 빈 슬롯도 "UNARMED"라는 진짜 아이템으로 취급한다 - items.json에 실제 등록된
	// Index라서, 아래 조회+스폰 경로를 무기가 아닌 다른 아이템과 완전히 동일하게
	// 태워야 BP_Unarmed Actor가 스폰되고 GetMainItem()이 정확한 값을 가리키게
	// 된다. 여기서 조기 return하면 스폰 로직을 건너뛰게 된다.
	const FName ItemIndexToEquip = Slots[SlotIndex].IsEmpty() ? FName(TEXT("UNARMED")) : Slots[SlotIndex].ItemIndex;

	UItemDataManager* ItemDataManager = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<UItemDataManager>()
		: nullptr;
	UMainWeaponComponent* WeaponComponent = GetWeaponComponent();
	if (!ItemDataManager || !WeaponComponent || !OwningCharacter)
	{
		return;
	}

	FItemData ItemData;
	if (!ItemDataManager->GetItemData(ItemIndexToEquip, ItemData))
	{
		return;
	}

	if (ItemData.Category == TEXT("Weapon"))
	{
		WeaponComponent->EquipWeapon(ItemData.Index, Slots[SlotIndex].CurrentAmmo, SlotIndex);
	}
	else
	{
		// 비무기 아이템의 스폰/부착/ReceiveItemEquip()은 이제 MainWeaponComponent가
		// 담당한다(EquipItemVisual()) - 이 컴포넌트는 Character 위에 있어서
		// ActiveItemIndex가 모든 클라이언트(다른 리모트 클라이언트 포함)에 정상적으로
		// 리플리케이트되기 때문이다. MainInventoryComponent(PlayerController 위)는
		// 소유 클라이언트에게만 리플리케이트되어 이 용도로 못 쓴다.
		WeaponComponent->EquipItemVisual(ItemData.Index);
	}
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

		// 비무기 아이템도 여기서 같이 내린다 - 무기로 바꾸는 경우 EquipItem()의
		// Weapon 분기는 아이템 쪽을 건드리지 않아서, 여기서 안 지우면 이전에 들고
		// 있던 비무기 아이템(예: I_1의 큐브)이 새 무기와 함께 계속 손에 남아있게 된다.
		// EquipItemVisual(NAME_None)은 GetItemData(NAME_None)이 실패하는 걸 이용해
		// "현재 아이템을 그냥 지운다"는 뜻으로 쓴다 - EquipWeapon(NAME_None, ...)과 같은 패턴.
		WeaponComponent->EquipItemVisual(NAME_None);
	}

	// EquippedSlotIndex는 여기서 리셋하지 않는다 - TakeSlot()이 "지금 선택된 슬롯을
	// 꺼낸 뒤에도 어디가 선택돼있었는지"를 계속 알아야 PlaceSlot()이 재장착할 자리를
	// 정확히 판단할 수 있다. EquipItem()이 새 슬롯을 선택할 때 어차피 새 값으로 덮어쓴다.
}

void UMainInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UMainInventoryComponent, EquippedSlotIndex);
}

void UMainInventoryComponent::Server_EquipItem_Implementation(int32 SlotIndex)
{
	// 이미 장착 중인 슬롯을 또 누른 거면 아무 것도 안 한다 - 안 그러면 무기 Actor를
	// 불필요하게 다시 스폰하고 Draw 애니메이션도 다시 재생하게 된다. AddItem()/
	// PlaceSlot()이 내부적으로 부르는 EquipItem()은 이 가드를 거치지 않으므로,
	// "빈 슬롯에 아이템이 막 도착해서 자동 장착"하는 경우는 영향받지 않는다.
	if (SlotIndex == EquippedSlotIndex)
	{
		return;
	}

	EquipItem(SlotIndex);
}
