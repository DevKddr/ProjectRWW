// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MainInventorySlotContainerWidget.h"
#include "MainHotbarWidget.generated.h"

// 인벤토리 창과 별개로 항상 화면에 떠 있는 HUD. 슬롯 0~8(핫바)만 표시한다.
// 게임플레이 중엔 게임 전용 입력 모드(FInputModeGameOnly)라 마우스로 클릭할 수 없으므로
// 순수 표시 전용이다 - 실제 장착은 숫자키 입력(AMainPlayerController 쪽)으로만 이뤄진다.
UCLASS()
class PROJECTRWW_API UMainHotbarWidget : public UMainInventorySlotContainerWidget
{
	GENERATED_BODY()

protected:
	virtual void BuildSlotWidgets() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UHorizontalBox> SlotBar;

	// 핫바가 다루는 슬롯 개수(0번부터 이 개수만큼). 블루프린트에서 조정 가능.
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	int32 HotbarSlotCount = 9;
};
