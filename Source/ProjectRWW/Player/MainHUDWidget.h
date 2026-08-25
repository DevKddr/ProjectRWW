// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainHUDWidget.generated.h"

UCLASS()
class PROJECTRWW_API UMainHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	float CurrentHP = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	float MaxHP = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	float CurrentMana = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	float MaxMana = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	int32 CurrentAmmo = 0;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	int32 MagazineSize = 0;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	bool bIsReloading = false;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	float ReloadProgress = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	FName WeaponIndex;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	float CurrentSpreadDegrees = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	float MaxSpreadDegrees = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	bool bIsAiming = false;
};
