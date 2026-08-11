// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainMapMarker.h"
#include "MainMapMarkerComponent.h"
#include "Components/SceneComponent.h"

AMainMapMarker::AMainMapMarker()
{
	bReplicates = true;
	// 지도 마커는 거리와 상관없이 항상 모든 클라이언트에 리플리케이트되어야 한다
	// (관련성 컬링으로 멀리 있는 마커가 지도에서 사라지는 걸 방지).
	bAlwaysRelevant = true;

	// 액터가 월드 위치(Transform)를 가지려면 SceneComponent 타입의 루트가 반드시 필요하다.
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	MarkerComponent = CreateDefaultSubobject<UMainMapMarkerComponent>(TEXT("MarkerComponent"));
}
