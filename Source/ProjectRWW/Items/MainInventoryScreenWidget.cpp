// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainInventoryScreenWidget.h"
#include "MainSlotGridWidget.h"

void UMainInventoryScreenWidget::SetContainerComponent(UMainSlotContainerComponent* InComponent)
{
	ContainerComponent = InComponent;

	// 여기서 한 번만 전달한다 - HotbarSection/BackpackSection(UMainSlotGridWidget)이
	// 자기 bIsConstructed 플래그로 "지금 당장 델리게이트를 바인딩할지, 곧 실행될
	// 자기 자신의 NativeConstruct가 대신 처리하게 둘지"를 알아서 판단한다. 이 함수를
	// 우리 NativeConstruct에서 또 호출하면(예전에 그랬음) 자식이 이미 바인딩을 마친
	// 뒤에 같은 델리게이트를 중복 등록하게 되어 엔진 ensure가 뜬다.
	if (HotbarSection && BackpackSection)
	{
		HotbarSection->SetContainerComponent(ContainerComponent);
		BackpackSection->SetContainerComponent(ContainerComponent);
	}
}
