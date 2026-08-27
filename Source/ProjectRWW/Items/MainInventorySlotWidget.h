// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventorySlot.h"
#include "MainInventorySlotWidget.generated.h"

// 인벤토리 창/핫바 양쪽에서 재사용하는 슬롯 하나짜리 위젯.
UCLASS()
class PROJECTRWW_API UMainInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 이 위젯이 InventorySlots 배열의 몇 번을 나타내는지. 클릭 알림에 실어보내는 용도.
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 SlotIndex = -1;

	// 데이터가 바뀔 때마다 부모가 이 함수를 호출한다. 아이콘 조회/로드까지 여기서 끝내고,
	// 결과(로드된 텍스처, 없으면 nullptr)를 OnIconChanged로 블루프린트에 넘긴다.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetSlotData(int32 InSlotIndex, const FInventorySlot& SlotData);

	// 클릭됐다는 사실 + 내 슬롯 번호만 위로 올려보낸다. "클릭하면 뭘 할지"는
	// 이 위젯이 아니라 부모(인벤토리 창)가 정한다.
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotClicked, int32, ClickedSlotIndex);
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnSlotClicked OnSlotClicked;

	// 블루프린트의 버튼 OnClicked에 바인딩해서 쓴다.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void HandleClicked();

	// 아이콘 로드까지 C++이 끝내고, 실제로 화면 위젯에 적용하는 마지막 한 단계만
	// 블루프린트에 맡긴다. Icon이 nullptr이면 숨겨야 한다는 뜻.
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void OnIconChanged(class UTexture2D* Icon);
};
