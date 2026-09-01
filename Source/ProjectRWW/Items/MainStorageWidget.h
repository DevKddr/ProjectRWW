// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MainInventorySlotContainerWidget.h"
#include "MainStorageWidget.generated.h"

// 창고를 그리드로 보여준다. 창고는 등급에 따라 칸 수가 달라지므로, 한 줄에 몇 칸인지를
// 고정값이 아니라 UMainStorageComponent::GetColumnCount()로 매번 물어본다.
UCLASS()
class PROJECTRWW_API UMainStorageWidget : public UMainInventorySlotContainerWidget
{
	GENERATED_BODY()

protected:
	virtual void BuildSlotWidgets() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UUniformGridPanel> SlotGrid;
};
