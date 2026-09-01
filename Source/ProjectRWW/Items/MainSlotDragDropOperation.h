// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "MainSlotDragDropOperation.generated.h"

// 슬롯 드래그 시작 시 담기는 정보. 드롭받는 쪽이 이걸로 "어느 컨테이너의 몇 번 슬롯에서
// 왔는지"를 판단한다. 컴포넌트가 드래그 도중 파괴될 가능성에 대비해 약한 참조로 담는다.
UCLASS()
class PROJECTRWW_API UMainSlotDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	TWeakObjectPtr<class UMainSlotContainerComponent> SourceComponent;

	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	int32 SourceSlotIndex = -1;
};
