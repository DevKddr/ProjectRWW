// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MainMapMarker.generated.h"

class UMainMapMarkerComponent;

UCLASS()
class PROJECTRWW_API AMainMapMarker : public AActor
{
	GENERATED_BODY()

public:
	AMainMapMarker();

	UPROPERTY(VisibleAnywhere, Category = "Map Marker")
	TObjectPtr<UMainMapMarkerComponent> MarkerComponent;
};
