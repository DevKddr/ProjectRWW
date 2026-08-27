// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainInventorySlotContainerWidget.generated.h"

// MainInventoryWidget과 MainHotbarWidget이 공유하는 부모 클래스.
// 두 위젯 모두 "InventoryComponent 구독 -> 슬롯 위젯 최초 1회 생성 -> 데이터 갱신"
// 흐름이 똑같고, 배치 방식(그리드 vs 가로 나열)만 다르다. 그 배치 로직만
// BuildSlotWidgets()로 분리해서 자식 클래스가 각자 구현하게 한다.
UCLASS(Abstract)
class PROJECTRWW_API UMainInventorySlotContainerWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<class UMainInventorySlotWidget> SlotWidgetClass;

	UPROPERTY()
	TArray<TObjectPtr<class UMainInventorySlotWidget>> SlotWidgets;

	class UMainInventoryComponent* GetInventoryComponent() const;

	void RefreshAllSlots();

	// 자식 클래스가 자신의 레이아웃(그리드/가로 박스 등)에 맞게 SlotWidgets를
	// 채워 넣는다. NativeConstruct()가 슬롯이 비어있을 때 딱 한 번만 호출한다.
	virtual void BuildSlotWidgets() PURE_VIRTUAL(UMainInventorySlotContainerWidget::BuildSlotWidgets, );

private:
	UFUNCTION()
	void HandleInventoryChanged();
};
