// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainCrosshairWidget.generated.h"

UCLASS()
class PROJECTRWW_API UMainCrosshairWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	UPROPERTY(BlueprintReadOnly, Category = "Crosshair")
	float CurrentSpreadDegrees = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Crosshair")
	float MaxSpreadDegrees = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Crosshair")
	bool bIsAiming = false;

	UPROPERTY(BlueprintReadOnly, Category = "Crosshair")
	float OffsetX = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Crosshair")
	float OffsetY = 0.0f;
};
