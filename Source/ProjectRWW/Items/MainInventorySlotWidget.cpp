// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainInventorySlotWidget.h"
#include "MainSlotContainerComponent.h"
#include "MainSlotDragDropOperation.h"
#include "ItemDataManager.h"
#include "Engine/Texture2D.h"
#include "Components/Image.h"

void UMainInventorySlotWidget::SetSlotData(int32 InSlotIndex, const FInventorySlot& SlotData)
{
	SlotIndex = InSlotIndex;
	CachedSlotData = SlotData;
	SetRenderOpacity(1.0f);

	UTexture2D* Icon = nullptr;
	if (!SlotData.IsEmpty())
	{
		UItemDataManager* ItemDataManager = GetGameInstance() ? GetGameInstance()->GetSubsystem<UItemDataManager>() : nullptr;
		FItemData ItemData;
		if (ItemDataManager && ItemDataManager->GetItemData(SlotData.ItemIndex, ItemData))
		{
			Icon = Cast<UTexture2D>(ItemData.IconPath.TryLoad());
		}
	}

	if (IconImage)
	{
		IconImage->SetBrushFromTexture(Icon);
		IconImage->SetVisibility(Icon ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

FReply UMainInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}

	if (!CachedSlotData.IsEmpty())
	{
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}

	// 빈 슬롯이어도 클릭 자체는 UI가 소비해야 한다 - 안 그러면 이 마우스 입력이 뒤쪽
	// 게임 화면까지 새어나가서 카메라가 돌아가는 문제가 생긴다.
	return FReply::Handled();
}

void UMainInventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	UMainSlotDragDropOperation* DragOp = NewObject<UMainSlotDragDropOperation>(this);
	DragOp->SourceComponent = OwningComponent;
	DragOp->SourceSlotIndex = SlotIndex;

	// 커서를 따라다닐 비주얼 - 자기 자신과 같은 클래스로 하나 더 만들어서 같은 데이터를 보여준다.
	if (UMainInventorySlotWidget* Visual = CreateWidget<UMainInventorySlotWidget>(GetOwningPlayer(), GetClass()))
	{
		// bIsDragPreview는 TakeWidget()보다 먼저 세팅해야 한다 - Event Construct 그래프가
		// 이 값을 읽어서 테두리를 숨길지 결정하기 때문에, Construct가 실행되는 시점엔
		// 이미 최종 값이 들어가 있어야 한다.
		Visual->bIsDragPreview = true;

		// TakeWidget()을 먼저 호출해야 Event Construct(IconImage 연결)가 실행된다.
		// CreateWidget 직후에는 아직 Slate 위젯이 안 만들어져서 Construct가 지연되고,
		// 그 상태에서 SetSlotData를 부르면 IconImage가 아직 null이라 아이콘 설정이 무시된다.
		Visual->TakeWidget();
		Visual->SetSlotData(SlotIndex, CachedSlotData);
		DragOp->DefaultDragVisual = Visual;
		DragOp->Pivot = EDragPivot::MouseDown;
	}

	// 원본 슬롯은 드래그 중임을 알 수 있게 반투명 처리.
	SetRenderOpacity(DragOpacity);

	OutOperation = DragOp;
}

bool UMainInventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	const UMainSlotDragDropOperation* SlotOp = Cast<UMainSlotDragDropOperation>(InOperation);
	UMainSlotContainerComponent* Source = SlotOp ? SlotOp->SourceComponent.Get() : nullptr;
	UMainSlotContainerComponent* Dest = OwningComponent.Get();

	if (!Source || !Dest)
	{
		return false;
	}

	if (Source == Dest)
	{
		Dest->Server_MoveSlot(SlotOp->SourceSlotIndex, SlotIndex);
	}
	else
	{
		Source->Server_MoveSlot(SlotOp->SourceSlotIndex, SlotIndex, Dest);
	}

	return true;
}

void UMainInventorySlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	SetRenderOpacity(1.0f);
}
