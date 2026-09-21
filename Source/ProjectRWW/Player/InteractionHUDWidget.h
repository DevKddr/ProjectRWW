// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InteractionHUDWidget.generated.h"

class AItemPickupActor;
class UInteractionComponent;

// 바라보는 줍기 대상 위에 마커(흰 동그라미)와 프롬프트를 그리는 전용 뷰포트 위젯. 기본
// HUD(MainHUDWidget)와 분리해서, 매 프레임 움직이는 마커가 HUD 전체의 캐시(Invalidation)를
// 깨지 않게 하고 마커 로직이 HP/탄약 같은 다른 HUD 로직과 섞이지 않게 한다.
UCLASS()
class PROJECTRWW_API UInteractionHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 바라보는 줍기 대상이 바뀔 때(잡힘/놓침/교체/가득 참 여부 변화)만 호출된다. BP에서
	// 마커와 프롬프트의 표시 여부(Visibility 토글)와 색(가득 참이면 빨강)을 처리한다.
	// 프로퍼티 바인딩 대신 이벤트로 밀어주는 이유: 바인딩은 매 프레임 폴링이라 비싸다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnInteractTargetChanged(bool bShow, bool bBlocked);

	// 마커가 보이는 동안, 화면 좌표가 실제로 바뀔 때만 호출된다. BP에서 마커의
	// Render Translation으로 옮긴다(레이아웃 슬롯 위치는 더 비싸다).
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnInteractMarkerMoved(FVector2D ScreenPosition);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UFUNCTION()
	void OnInteractFocusChanged(AItemPickupActor* Pickup, bool bBlocked);

	// 마커 화면 좌표를 계산해 BP에 알린다. 임계값을 두지 않고 호출될 때마다 그대로 보낸다 -
	// 임계값이 있으면 좌표가 계단식으로 갱신돼 점이 뚝뚝 끊겨 보인다.
	void UpdateMarkerPosition();

	// 폰이 바뀌면(리스폰) 컴포넌트도 바뀌므로 매 틱 포인터만 비교해 필요할 때 재구독한다.
	TWeakObjectPtr<UInteractionComponent> BoundInteraction;

	TWeakObjectPtr<AItemPickupActor> MarkerTarget;
};
