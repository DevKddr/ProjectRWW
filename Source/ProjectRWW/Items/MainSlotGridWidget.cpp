// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainSlotGridWidget.h"
#include "MainInventorySlotWidget.h"
#include "MainSlotContainerComponent.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"

void UMainSlotGridWidget::SetContainerComponent(UMainSlotContainerComponent* InComponent)
{
	if (ContainerComponent == InComponent)
	{
		return;  // 이미 같은 컴포넌트 - 중복 구독/재빌드 방지
	}

	// 다른 컴포넌트로 바뀌는 거라면, 예전 컴포넌트의 구독부터 반드시 해제한다 -
	// 안 그러면 이 위젯이 더 이상 안 보여주는 컴포넌트의 변경 알림까지 계속 받게 되고,
	// 그 컴포넌트가 이 위젯을 계속 참조로 붙잡고 있게 된다.
	if (bIsConstructed && ContainerComponent)
	{
		ContainerComponent->OnSlotsChanged.RemoveDynamic(this, &UMainSlotGridWidget::HandleInventoryChanged);
	}

	ContainerComponent = InComponent;

	// NativeConstruct가 이미 실행된 뒤라면(로비처럼 부모가 자식보다 늦게 컴포넌트를
	// 넘겨주는 경우), NativeConstruct가 놓친 구독/슬롯 생성/갱신을 지금 대신 해준다.
	// 아직 Construct 전이면(라이드처럼 AddToViewport 전에 미리 세팅하는 경우) 아무것도
	// 안 해도 된다 - 곧 실행될 NativeConstruct가 알아서 처리한다.
	if (bIsConstructed && ContainerComponent)
	{
		ContainerComponent->OnSlotsChanged.AddDynamic(this, &UMainSlotGridWidget::HandleInventoryChanged);

		if (SlotWidgets.Num() == 0)
		{
			BuildSlotWidgets();
		}

		RefreshAllSlots();
	}
}

void UMainSlotGridWidget::NativeConstruct()
{
	Super::NativeConstruct();

	bIsConstructed = true;

	if (ContainerComponent)
	{
		ContainerComponent->OnSlotsChanged.AddDynamic(this, &UMainSlotGridWidget::HandleInventoryChanged);
	}

	// 슬롯 위젯 생성은 최초 한 번만 - AddToViewport()를 부를 때마다 NativeConstruct가
	// 재호출되므로, 매번 새로 만들면 슬롯이 중복 생성된다.
	if (SlotWidgets.Num() == 0)
	{
		BuildSlotWidgets();
	}

	RefreshAllSlots();
}

void UMainSlotGridWidget::NativeDestruct()
{
	if (ContainerComponent)
	{
		ContainerComponent->OnSlotsChanged.RemoveDynamic(this, &UMainSlotGridWidget::HandleInventoryChanged);
	}

	Super::NativeDestruct();
}

void UMainSlotGridWidget::RefreshAllSlots()
{
	if (!ContainerComponent)
	{
		return;
	}

	for (int32 i = 0; i < SlotWidgets.Num(); ++i)
	{
		const int32 SourceIndex = StartIndex + i;
		if (ContainerComponent->Slots.IsValidIndex(SourceIndex))
		{
			SlotWidgets[i]->SetSlotData(SourceIndex, ContainerComponent->Slots[SourceIndex]);
		}
	}
}

void UMainSlotGridWidget::HandleInventoryChanged()
{
	// Slots가 리플리케이트로 늦게 도착해서 최초 BuildSlotWidgets()가 실행됐을 때
	// Slots.Num()이 아직 0이라 슬롯을 하나도 못 지었을 수 있다(ClientRestart의 UI
	// 생성 타이밍과 Slots 리플리케이션 도착 순서는 서로 다른 액터에 걸쳐있어서
	// 보장되지 않는다). 데이터가 실제로 도착한 지금(OnSlotsChanged가 불렸다는 건
	// Slots가 갱신됐다는 뜻) 아직 못 지었으면 다시 지어본다 - NativeConstruct/
	// SetContainerComponent와 같은 가드(Num()==0)를 그대로 재사용한다.
	if (SlotWidgets.Num() == 0)
	{
		BuildSlotWidgets();
	}

	RefreshAllSlots();
}

void UMainSlotGridWidget::BuildSlotWidgets()
{
	if (!SlotGrid || !SlotWidgetClass || !ContainerComponent)
	{
		return;
	}

	// 가로 칸 수는 컴포넌트에게 물어본다 - 고정 크기 컨테이너는 자기 Columns 필드를,
	// 창고처럼 조건에 따라 계산이 필요한 컨테이너는 오버라이드된 값을 돌려준다.
	// 세로 줄 수는 저장된 값이 없으므로 여기서 매번 올림 나눗셈으로 계산한다.
	const int32 Columns = FMath::Max(1, ContainerComponent->GetColumnCount());
	int32 TotalSlots = FMath::Max(0, ContainerComponent->Slots.Num() - StartIndex);
	if (MaxSlotsToShow > 0)
	{
		TotalSlots = FMath::Min(TotalSlots, MaxSlotsToShow);
	}

	for (int32 i = 0; i < TotalSlots; ++i)
	{
		UMainInventorySlotWidget* SlotWidget = CreateWidget<UMainInventorySlotWidget>(this, SlotWidgetClass);
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->OwningComponent = ContainerComponent.Get();

		const int32 Row = i / Columns;
		const int32 Column = i % Columns;
		SlotGrid->AddChildToUniformGrid(SlotWidget, Row, Column);

		SlotWidgets.Add(SlotWidget);
	}
}
