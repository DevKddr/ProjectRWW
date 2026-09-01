// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainInventorySlotContainerWidget.h"
#include "MainInventorySlotWidget.h"
#include "MainSlotContainerComponent.h"

void UMainInventorySlotContainerWidget::SetContainerComponent(UMainSlotContainerComponent* InComponent)
{
	ContainerComponent = InComponent;

	// NativeConstruct가 이미 실행된 뒤라면(로비처럼 부모가 자식보다 늦게 컴포넌트를
	// 넘겨주는 경우), NativeConstruct가 놓친 구독/슬롯 생성/갱신을 지금 대신 해준다.
	// 아직 Construct 전이면(라이드처럼 AddToViewport 전에 미리 세팅하는 경우) 아무것도
	// 안 해도 된다 - 곧 실행될 NativeConstruct가 알아서 처리한다.
	if (bIsConstructed && ContainerComponent)
	{
		ContainerComponent->OnSlotsChanged.AddDynamic(this, &UMainInventorySlotContainerWidget::HandleInventoryChanged);

		if (SlotWidgets.Num() == 0)
		{
			BuildSlotWidgets();
		}

		RefreshAllSlots();
	}
}

void UMainInventorySlotContainerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	bIsConstructed = true;

	if (ContainerComponent)
	{
		ContainerComponent->OnSlotsChanged.AddDynamic(this, &UMainInventorySlotContainerWidget::HandleInventoryChanged);
	}

	// 슬롯 위젯 생성은 최초 한 번만 - AddToViewport()를 부를 때마다 NativeConstruct가
	// 재호출되므로, 매번 새로 만들면 슬롯이 중복 생성된다.
	if (SlotWidgets.Num() == 0)
	{
		BuildSlotWidgets();
	}

	RefreshAllSlots();
}

void UMainInventorySlotContainerWidget::NativeDestruct()
{
	if (ContainerComponent)
	{
		ContainerComponent->OnSlotsChanged.RemoveDynamic(this, &UMainInventorySlotContainerWidget::HandleInventoryChanged);
	}

	Super::NativeDestruct();
}

void UMainInventorySlotContainerWidget::RefreshAllSlots()
{
	if (!ContainerComponent)
	{
		return;
	}

	for (int32 i = 0; i < SlotWidgets.Num(); ++i)
	{
		if (ContainerComponent->Slots.IsValidIndex(i))
		{
			SlotWidgets[i]->SetSlotData(i, ContainerComponent->Slots[i]);
		}
	}
}

void UMainInventorySlotContainerWidget::HandleInventoryChanged()
{
	RefreshAllSlots();
}
