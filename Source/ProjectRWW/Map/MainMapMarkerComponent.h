// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MainMapMarkerComponent.generated.h"

UENUM(BlueprintType)
enum class EMainMapMarkerType : uint8
{
	Extraction,
	POI,
	Custom
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTRWW_API UMainMapMarkerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMainMapMarkerComponent();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Replicated, Category = "Map Marker")
	EMainMapMarkerType MarkerType = EMainMapMarkerType::POI;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Replicated, Category = "Map Marker")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Replicated, Category = "Map Marker")
	FLinearColor IconTint = FLinearColor::White;

	// 0이면 점으로, 0보다 크면 원형 영역으로 지도에 표시 (표시 방식은 위젯 쪽에서 처리)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Replicated, Category = "Map Marker")
	float Radius = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Replicated, Category = "Map Marker")
	bool bVisibleOnMap = true;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
