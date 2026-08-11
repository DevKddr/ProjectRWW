// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MainMapMarkerSubsystem.generated.h"

class UMainMapMarkerComponent;

UCLASS()
class PROJECTRWW_API UMainMapMarkerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void AddMarker(UMainMapMarkerComponent* Marker);
	void RemoveMarker(UMainMapMarkerComponent* Marker);

	const TArray<TWeakObjectPtr<UMainMapMarkerComponent>>& GetMarkers() const { return Markers; }

private:
	TArray<TWeakObjectPtr<UMainMapMarkerComponent>> Markers;
};
