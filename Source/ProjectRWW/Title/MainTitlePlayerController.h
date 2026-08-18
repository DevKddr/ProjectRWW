// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainTitlePlayerController.generated.h"

UCLASS()
class PROJECTRWW_API AMainTitlePlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

	// 블루프린트(BP_MainTitlePlayerController)에서 WBP_MainTitleWidget으로 지정
	UPROPERTY(EditDefaultsOnly, Category = "Title")
	TSubclassOf<class UMainTitleWidget> TitleWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<class UMainTitleWidget> TitleWidgetInstance;
};
