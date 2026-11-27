// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PopupRowWidget.generated.h"

// 팝업 목록의 한 줄. C++ 목록은 상황만 알려주고, 어떻게 보이고 움직일지는 BP가 정한다.
UCLASS()
class PROJECTRWW_API UPopupRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 목록에 들어왔다.
	UFUNCTION(BlueprintImplementableEvent, Category = "PopupRow")
	void ReceiveEnter();

	// 아래에서 몇 번째 줄인지(0이 맨 아래) 정해지거나 바뀌었다.
	UFUNCTION(BlueprintImplementableEvent, Category = "PopupRow")
	void ReceiveSlotChanged(int32 SlotIndex);

	// 더 새로운 줄이 들어와 이전 줄이 됐다.
	UFUNCTION(BlueprintImplementableEvent, Category = "PopupRow")
	void ReceiveAged();

	// 목록에서 나간다. LeaveSeconds 뒤에 목록이 이 줄을 없앤다.
	UFUNCTION(BlueprintImplementableEvent, Category = "PopupRow")
	void ReceiveLeave();

	// 나가는 연출에 걸리는 시간(초). BP의 Leave 애니메이션 길이에 맞춘다.
	UPROPERTY(EditDefaultsOnly, Category = "PopupRow")
	float LeaveSeconds = 0.4f;
};
