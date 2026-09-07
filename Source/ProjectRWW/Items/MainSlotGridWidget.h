// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainSlotGridWidget.generated.h"

// 인벤토리/창고 등 그리드형 슬롯 컨테이너 위젯의 공용 부모이자 기본 구현.
// "InventoryComponent 구독 -> 슬롯 위젯 최초 1회 생성 -> 데이터 갱신" 흐름과
// "ContainerComponent->GetColumnCount()/Slots.Num()만으로 그리드를 채우는" 배치
// 로직까지 여기서 전부 처리한다 - 인벤토리든 창고든 나중에 생길 상자든, 컴포넌트의
// GetColumnCount()만 알맞게 구현하면 이 클래스를 그대로 부모로 쓸 수 있다.
// 핫바처럼 가로 나열 + 일부 슬롯만 보여줘야 하는 특수한 배치가 필요하면
// BuildSlotWidgets()를 오버라이드해서 대체한다.
UCLASS()
class PROJECTRWW_API UMainSlotGridWidget : public UUserWidget
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

	// 블루프린트에서 UniformGridPanel로 바인딩. Optional인 이유는 핫바처럼
	// BuildSlotWidgets()를 오버라이드해서 이 패널 자체를 안 쓰는 자식도 있기 때문이다.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UUniformGridPanel> SlotGrid;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<class UMainInventorySlotWidget> SlotWidgetClass;

	// ContainerComponent->Slots의 몇 번부터 보여줄지. EditAnywhere라서, 같은 클래스의
	// 위젯 인스턴스 여러 개를 한 화면에 배치하고(예: 핫바 구역 + 나머지 구역) 각자
	// 다른 범위를 보여주게 할 수 있다.
	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 StartIndex = 0;

	// 몇 개까지만 보여줄지. 0이면 제한 없이 StartIndex부터 끝까지 전부 보여준다.
	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 MaxSlotsToShow = 0;

	UPROPERTY()
	TArray<TObjectPtr<class UMainInventorySlotWidget>> SlotWidgets;

	UPROPERTY()
	TObjectPtr<class UMainSlotContainerComponent> ContainerComponent;

	// NativeConstruct가 이미 한 번 실행됐는지. SetContainerComponent가 Construct 이후에
	// 호출되는 경우(로비처럼 부모가 자식보다 늦게 컴포넌트를 넘겨줄 때)를 구분하는 데 쓴다.
	bool bIsConstructed = false;

	void RefreshAllSlots();

	// SlotGrid + ContainerComponent->GetColumnCount()/Slots.Num()만으로 그리드를 채운다.
	// 세로 줄 수는 저장하지 않고 여기서 매번 올림 나눗셈으로 계산한다. 가로 나열 등
	// 다른 배치가 필요한 자식 클래스(핫바)는 이 함수를 오버라이드해서 대체한다.
	virtual void BuildSlotWidgets();

private:
	UFUNCTION()
	void HandleInventoryChanged();
};
