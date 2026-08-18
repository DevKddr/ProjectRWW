// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainTitlePlayerController.h"
#include "MainTitleWidget.h"
#include "Blueprint/UserWidget.h"

void AMainTitlePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!TitleWidgetClass)
	{
		return;
	}

	TitleWidgetInstance = CreateWidget<UMainTitleWidget>(this, TitleWidgetClass);
	if (!TitleWidgetInstance)
	{
		return;
	}

	TitleWidgetInstance->AddToViewport();

	// 타이틀 화면은 마우스로 입력창/버튼을 눌러야 하니 UI 전용 입력 모드로 전환한다.
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(TitleWidgetInstance->TakeWidget());
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}
