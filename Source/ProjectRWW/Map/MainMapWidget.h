// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMapMarkerComponent.h"
#include "MainMapWidget.generated.h"

// 위젯이 화면에 그릴 마커 한 개 분량의 정보. 컴포넌트를 직접 넘기지 않고 필요한 값만 복사해서
// 블루프린트에 넘긴다 (블루프린트가 컴포넌트 내부를 직접 건드릴 필요가 없게 하기 위함).
USTRUCT(BlueprintType)
struct FMainMapMarkerViewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FVector2D MapPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	EMainMapMarkerType MarkerType = EMainMapMarkerType::POI;

	UPROPERTY(BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly)
	FLinearColor IconTint = FLinearColor::White;
};

UCLASS()
class PROJECTRWW_API UMainMapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 지도 텍스처의 네 모서리가 대응하는 월드 좌표 범위. TestMap 기준으로 값을 맞춘다.
	UPROPERTY(EditDefaultsOnly, Category = "Map")
	float WorldMinX = -5000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Map")
	float WorldMaxX = 5000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Map")
	float WorldMinY = -5000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Map")
	float WorldMaxY = 5000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Map")
	float MarkerRefreshIntervalSeconds = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	FVector PlayerWorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	FVector2D PlayerMapPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	float PlayerMapRotationDegrees = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	TArray<FMainMapMarkerViewData> Markers;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 월드 좌표를 지도 텍스처 위 0~1 UV 좌표로 변환한다.
	FVector2D WorldToMapUV(const FVector& WorldLocation) const;

	void RefreshMarkers();

	// 블루프린트(WBP_MainMap)가 이 이벤트를 구현해서 실제 화면 배치를 한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Map")
	void OnMapDataRefreshed();

private:
	FTimerHandle MarkerRefreshTimerHandle;
};
