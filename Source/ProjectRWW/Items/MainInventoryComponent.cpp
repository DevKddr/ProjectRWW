// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainInventoryComponent.h"
#include "GameFramework/PlayerController.h"
#include "Player/MainCharacter.h"
#include "Combat/MainWeaponComponent.h"
#include "ItemDataManager.h"
#include "Net/UnrealNetwork.h"
#include "Abilities/GameplayAbility.h"

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

		// 스킬 클래스는 데이터 필드가 아니라 아이템 코드로 직접 조합해서 찾는다 -
		// "아이템 코드와 스킬 클래스명은 항상 1:1로 일치시킨다"는 규칙 확정에 따름.
		// 스킬 없는 슬롯(UNARMED, 스킬 하나뿐인 아이템의 나머지 슬롯 등)은 이 이름의
		// 클래스가 애초에 존재하지 않아 FindObject가 nullptr을 돌려주고,
		// GrantItemSkillAbility()가 그 슬롯만 조용히 비워둔다. FindObject를 쓰는
		// 이유: 이 클래스들은 전부 네이티브 C++라 게임 모듈 로드 시점에 이미
		// 메모리에 있다 - LoadClass처럼 없는 클래스마다 경고 로그를 남기지 않고
		// 조용히 nullptr을 반환한다.
		const FString PrimaryClassPath = FString::Printf(TEXT("/Script/ProjectRWW.GA_ItemSkill_%s_Primary"), *ItemData.Index.ToString());
		WeaponComponent->GrantItemSkillAbility(FindObject<UClass>(nullptr, *PrimaryClassPath), EMainAbilityInputID::Primary);

		const FString SecondaryClassPath = FString::Printf(TEXT("/Script/ProjectRWW.GA_ItemSkill_%s_Secondary"), *ItemData.Index.ToString());
		WeaponComponent->GrantItemSkillAbility(FindObject<UClass>(nullptr, *SecondaryClassPath), EMainAbilityInputID::Secondary);
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
		// UnequipWeapon() 하나로 충분하다 - ActiveHandActor가 무기/아이템 공용
		// 슬롯이 된 뒤로는, WeaponIndex가 None이 되면서 GetItemData(None)이 실패해
		// 지금 슬롯에 있는 게 무기든 비무기 아이템이든 상관없이 이미 지워진다.
		// 예전엔 ActiveWeaponActor/ActiveItemActor가 분리돼 있어서 아이템 쪽을
		// EquipItemVisual(NAME_None)으로 따로 지워줘야 했지만, 지금 그 호출을 남겨두면
		// ActiveItemIndex가 실제로 값이 바뀌어서(예: "UNARMED" -> NAME_None) 원격
		// 클라이언트에 OnRep_ActiveItemIndex가 별도로 하나 더 발동한다 - 이게
		// OnRep_ReplicationSequence(무기 쪽)와 서로 다른 순서로 도착할 수 있어서,
		// "무기가 먼저 스폰된 뒤 아이템 쪽이 뒤늦게 도착해 방금 스폰된 무기를
		// 지워버리는" 레이스의 원인이었다.
		WeaponComponent->UnequipWeapon();

		// EquipItem()이 매번 맨 먼저 UnequipItem()을 부르므로, 여기 한 줄이 아이템→무기/
		// 아이템→다른 아이템/완전 해제 모든 경로를 커버한다.
		WeaponComponent->RevokeItemSkillAbilities();
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
