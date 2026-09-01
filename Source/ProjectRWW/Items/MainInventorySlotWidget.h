// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventorySlot.h"
#include "MainInventorySlotWidget.generated.h"

// 인벤토리 창/핫바 양쪽에서 재사용하는 슬롯 하나짜리 위젯. 드래그 앤 드랍 감지/처리를
// 이 클래스가 직접 담당한다 - 어느 컨테이너(인벤토리/창고 등) 소속인지는 OwningComponent
// 하나만 알면 되고, 그 외엔 컴포넌트 타입을 전혀 몰라도 된다.
UCLASS()
class PROJECTRWW_API UMainInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 이 위젯이 Slots 배열의 몇 번을 나타내는지.
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 SlotIndex = -1;

	// 이 슬롯이 속한 컨테이너 컴포넌트. 컨테이너 위젯이 슬롯 생성 시 설정해준다.
	TWeakObjectPtr<class UMainSlotContainerComponent> OwningComponent;

	// 데이터가 바뀔 때마다 부모가 이 함수를 호출한다. 아이콘 조회/로드까지 여기서 끝내고,
	// 결과(로드된 텍스처, 없으면 nullptr)를 OnIconChanged로 블루프린트에 넘긴다.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetSlotData(int32 InSlotIndex, const FInventorySlot& SlotData);

	// 아이콘을 표시할 이미지 위젯. Details 패널에는 노출하지 않는다 - 거기서 값을 지정하면
	// 클래스 디폴트(CDO) 값 하나를 모든 인스턴스가 공유해버리는 문제가 있다. 대신 BP의
	// Event Construct 그래프에서 디자이너의 실제 Image 위젯을 드래그해 Set 노드로
	// 연결해야 한다 (BindWidget 대신 이 방식을 씀).
	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	TObjectPtr<class UImage> IconImage;

	// 드래그 중일 때 원본 슬롯에 적용할 투명도. BP 클래스 디폴트에서 조정 가능.
	UPROPERTY(EditAnywhere, Category = "Inventory")
	float DragOpacity = 0.5f;

	// 이 인스턴스가 커서를 따라다니는 드래그 미리보기용으로 만들어진 것인지. Details
	// 패널에는 노출하지 않는다 - NativeOnDragDetected에서 C++이 직접 세팅해주는 값이라
	// 사람이 편집할 일이 없다. BP의 Event Construct 그래프가 이 값을 읽어서 true일 때
	// 테두리 위젯을 Collapsed로 숨기는 데 쓴다.
	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	bool bIsDragPreview = false;

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	// 드래그 시작 시 커서를 따라다닐 비주얼을 만들 때 재사용하기 위해, 마지막으로
	// 받은 슬롯 데이터를 기억해둔다.
	FInventorySlot CachedSlotData;
};
