// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainLobbyStorageScreenWidget.generated.h"

// 로비 창고 화면 전체. 인벤토리 그리드 + 창고 그리드를 각자 위젯으로 배치해두고,
// 로비 컨트롤러의 두 컴포넌트를 각각 연결해주는 역할만 한다.
UCLASS()
class PROJECTRWW_API UMainLobbyStorageScreenWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	// 디자이너에서 이름이 정확히 "InventoryWidget"인 자식 위젯과 연결된다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UMainInventoryWidget> InventoryWidget;

	// 디자이너에서 이름이 정확히 "StorageWidget"인 자식 위젯과 연결된다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UMainStorageWidget> StorageWidget;
};
