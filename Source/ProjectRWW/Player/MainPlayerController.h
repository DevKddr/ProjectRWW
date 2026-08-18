// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainPlayerController.generated.h"

UCLASS()
class PROJECTRWW_API AMainPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// 디버그용: "RWW_SpawnExtractionMarker 100 200 0"처럼 X Y Z 좌표를 받아 마커를 스폰한다.
	UFUNCTION(Exec)
	void RWW_SpawnExtractionMarker(float X, float Y, float Z);

	// 디버그용: 지금까지 스폰한 모든 AMainMapMarker를 제거한다.
	UFUNCTION(Exec)
	void RWW_ClearMapMarkers();

protected:
	virtual void SetupInputComponent() override;

	void OnToggleMap(const struct FInputActionValue& Value);

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<class UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<class UInputAction> ToggleMapAction;

	UPROPERTY(EditDefaultsOnly, Category = "Map")
	TSubclassOf<class UMainMapWidget> MapWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<UMainMapWidget> MapWidgetInstance;
};
