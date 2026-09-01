// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MainInventorySlotContainerWidget.h"
#include "MainInventoryWidget.generated.h"

// 인벤토리 창. 36칸을 그리드로 배치한다.
UCLASS()
class PROJECTRWW_API UMainInventoryWidget : public UMainInventorySlotContainerWidget
{
	GENERATED_BODY()

protected:
	virtual void BuildSlotWidgets() override;

	// 블루프린트에서 UniformGridPanel로 바인딩.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UUniformGridPanel> SlotGrid;

	// 한 줄에 몇 칸을 배치할지. 블루프린트 클래스 디폴트에서 조정 가능 -
	// 코드 재컴파일 없이 레이아웃을 바꿀 수 있게.
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	int32 SlotsPerRow = 9;
};
