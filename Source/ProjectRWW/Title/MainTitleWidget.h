// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainTitleWidget.generated.h"

UCLASS()
class PROJECTRWW_API UMainTitleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 블루프린트에서 "접속" 버튼 OnClicked 이벤트에 이 함수를 바인딩하고,
	// 입력창(EditableTextBox)의 현재 텍스트를 인자로 넘겨서 호출한다.
	UFUNCTION(BlueprintCallable, Category = "Title")
	void OnConnectClicked(const FString& EnteredPlayerID);
};
