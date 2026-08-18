// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainTitleGameMode.h"
#include "MainTitlePlayerController.h"

AMainTitleGameMode::AMainTitleGameMode()
{
	PlayerControllerClass = AMainTitlePlayerController::StaticClass();
}
