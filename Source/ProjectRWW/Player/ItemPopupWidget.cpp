// Copyright Epic Games, Inc. All Rights Reserved.

#include "ItemPopupWidget.h"
#include "Engine/GameInstance.h"
#include "Items/ItemDataManager.h"
#include "PopupRowWidget.h"

void UItemPopupWidget::AddItem(FName ItemIndex)
{
	if (!RowClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ProjectRWW] 아이템 팝업의 RowClass가 비어 있음"));
		return;
	}

	UPopupRowWidget* Row = CreateWidget<UPopupRowWidget>(this, RowClass);
	if (!Row)
	{
		return;
	}

	FItemPopupEntry Entry;
	const UItemDataManager* ItemMgr = GetGameInstance() ? GetGameInstance()->GetSubsystem<UItemDataManager>() : nullptr;
	FItemData ItemData;
	if (ItemMgr && ItemMgr->GetItemData(ItemIndex, ItemData))
	{
		Entry.Name = ItemMgr->GetItemNameText(ItemIndex);
		Entry.RarityId = ItemData.RarityId;
	}
	else
	{
		Entry.Name = FText::FromName(ItemIndex);
	}

	ReceiveRowCreated(Row, Entry);
	PushRow(Row);
}
