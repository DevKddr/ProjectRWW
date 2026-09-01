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

public:
	// 이 위젯이 어느 컨테이너(라이드 인벤토리, 로비 인벤토리, 창고 등)를 보여줄지
	// 생성 직후 밖에서 지정해준다. AddToViewport() 전에 호출해야 한다 - Construct가
	// 그 시점에 실행되면서 이 값을 바로 사용하기 때문이다. 이렇게 하면 이 위젯은
	// 자신을 만든 컨트롤러가 어떤 클래스인지 전혀 몰라도 된다.
	void SetContainerComponent(class UMainSlotContainerComponent* InComponent);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<class UMainInventorySlotWidget> SlotWidgetClass;

	UPROPERTY()
	TArray<TObjectPtr<class UMainInventorySlotWidget>> SlotWidgets;

	UPROPERTY()
	TObjectPtr<class UMainSlotContainerComponent> ContainerComponent;

	// NativeConstruct가 이미 한 번 실행됐는지. SetContainerComponent가 Construct 이후에
	// 호출되는 경우(로비처럼 부모가 자식보다 늦게 컴포넌트를 넘겨줄 때)를 구분하는 데 쓴다.
	bool bIsConstructed = false;

	void RefreshAllSlots();

	// 자식 클래스가 자신의 레이아웃(그리드/가로 박스 등)에 맞게 SlotWidgets를
	// 채워 넣는다. NativeConstruct()가 슬롯이 비어있을 때 딱 한 번만 호출한다.
	virtual void BuildSlotWidgets() PURE_VIRTUAL(UMainInventorySlotContainerWidget::BuildSlotWidgets, );

private:
	UFUNCTION()
	void HandleInventoryChanged();
};
