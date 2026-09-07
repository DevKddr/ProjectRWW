// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainInventoryScreenWidget.generated.h"

// 인벤토리 창 전체. 핫바 구역(0~8번)과 나머지 구역을 각자 그리드 위젯으로 나눠
// 배치해두고, 같은 InventoryComponent를 두 그리드 모두에게 넘겨주는 역할만 한다 -
// MainLobbyStorageScreenWidget과 같은 패턴.
UCLASS()
class PROJECTRWW_API UMainInventoryScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 생성 직후 밖에서 호출한다. 호출 시점(Construct 전/후)과 무관하게 안전하다 -
	// HotbarSection/BackpackSection이 각자 자기 bIsConstructed로 판단해서 처리한다.
	void SetContainerComponent(class UMainSlotContainerComponent* InComponent);

protected:
	// 디자이너에서 이름이 정확히 "HotbarSection"인 자식 위젯과 연결된다.
	// StartIndex=0, MaxSlotsToShow=9로 설정해서 0~8번만 그린다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UMainSlotGridWidget> HotbarSection;

	// 디자이너에서 이름이 정확히 "BackpackSection"인 자식 위젯과 연결된다.
	// StartIndex=9, MaxSlotsToShow=0(제한 없음)로 설정해서 나머지 전부를 그린다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UMainSlotGridWidget> BackpackSection;

private:
	UPROPERTY()
	TObjectPtr<class UMainSlotContainerComponent> ContainerComponent;
};
