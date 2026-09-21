// Copyright Epic Games, Inc. All Rights Reserved.

#include "InteractionHUDWidget.h"
#include "Items/InteractionComponent.h"
#include "Items/ItemPickupActor.h"
#include "Player/MainCharacter.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void UInteractionHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 상호작용 컴포넌트 구독. 폰이 바뀌면(리스폰) 컴포넌트가 새것이라 재구독한다 -
	// 매 틱 하는 일은 포인터 비교 하나뿐이다.
	const AMainCharacter* OwningCharacter = Cast<AMainCharacter>(GetOwningPlayerPawn());
	UInteractionComponent* Interaction = OwningCharacter ? OwningCharacter->InteractionComponent : nullptr;
	if (BoundInteraction.Get() != Interaction)
	{
		if (UInteractionComponent* Old = BoundInteraction.Get())
		{
			Old->OnFocusChanged.RemoveDynamic(this, &UInteractionHUDWidget::OnInteractFocusChanged);
		}
		BoundInteraction = Interaction;

		// 이전 폰의 마커 상태를 지우고, 새 컴포넌트가 이미 대상을 잡고 있으면 그 상태로
		// 바로 맞춘다. 델리게이트는 "바뀔 때"만 오므로 구독 전에 일어난 변화는 놓친다.
		MarkerTarget.Reset();
		if (Interaction)
		{
			Interaction->OnFocusChanged.AddDynamic(this, &UInteractionHUDWidget::OnInteractFocusChanged);
			OnInteractFocusChanged(Interaction->GetFocusedPickup(), Interaction->IsFocusBlocked());
		}
		else
		{
			OnInteractTargetChanged(false, false);
		}
	}

	// 마커가 보이는 동안에만 좌표를 계산한다. 대상이 없으면 여기서 아무 일도 안 한다.
	UpdateMarkerPosition();
}

void UInteractionHUDWidget::OnInteractFocusChanged(AItemPickupActor* Pickup, bool bBlocked)
{
	MarkerTarget = Pickup;
	if (Pickup)
	{
		// 마커가 켜지기 전에 위치를 먼저 보내서, 한 프레임 동안 옛 위치에 보이는 걸 막는다.
		UpdateMarkerPosition();
	}
	OnInteractTargetChanged(Pickup != nullptr, bBlocked);
}

void UInteractionHUDWidget::UpdateMarkerPosition()
{
	const AItemPickupActor* Target = MarkerTarget.Get();
	if (!Target)
	{
		return;
	}

	// 마커는 판정 구의 중심에 뜬다 - 구 위치를 옮기면 표시 위치도 같이 조절된다.
	FVector2D ScreenPosition;
	if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(GetOwningPlayer(), Target->GetPickupCenter(), ScreenPosition, false))
	{
		return;  // 화면 밖이면 갱신하지 않는다(조준 중인 대상이라 사실상 항상 화면 안)
	}

	// 임계값 없이 그대로 보낸다 - 서브픽셀 이동까지 전달해서 계단식 끊김을 없앤다.
	OnInteractMarkerMoved(ScreenPosition);
}
