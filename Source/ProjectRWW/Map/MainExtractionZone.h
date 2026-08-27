// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MainExtractionZone.generated.h"

UCLASS()
class PROJECTRWW_API AMainExtractionZone : public AActor
{
	GENERATED_BODY()

public:
	AMainExtractionZone();

protected:
	// 눈에 보이는 메시이자 동시에 콜리전 판정 영역. EditAnywhere라 레벨에 배치한
	// 인스턴스마다 각자 다른 메시로 바꿀 수 있다.
	UPROPERTY(EditAnywhere, Category = "Extraction")
	TObjectPtr<class UStaticMeshComponent> CollisionMesh;

	UPROPERTY(VisibleAnywhere, Category = "Extraction")
	TObjectPtr<class UMainMapMarkerComponent> MarkerComponent;

	// 존 안에 얼마나 머물러야 탈출 완료되는지(초). 액터마다 개별 조정 가능.
	UPROPERTY(EditAnywhere, Category = "Extraction")
	float ExtractionDuration = 10.0f;

	UFUNCTION()
	void OnZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	// 플레이어별 독립 타이머 - 여러 명이 동시에 존 안에 있을 수 있으므로.
	TMap<TWeakObjectPtr<class AMainPlayerController>, FTimerHandle> ActiveTimers;

	void CompleteExtraction(TWeakObjectPtr<class AMainPlayerController> WeakController);
};
