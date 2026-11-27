// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PopupRowStackWidget.h"
#include "ItemPopupWidget.generated.h"

class UPopupRowWidget;

// 아이템 한 줄에 보여줄 값.
USTRUCT(BlueprintType)
struct FItemPopupEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "ItemPopup")
	FText Name;

	// items.json의 rarityId. 등급 색은 BP가 고른다.
	UPROPERTY(BlueprintReadOnly, Category = "ItemPopup")
	FName RarityId;
};

// 아이템 획득 팝업. 아이템마다 줄을 만들어 목록에 넣는다. 움직임은 부모 클래스가 맡는다.
UCLASS()
class PROJECTRWW_API UItemPopupWidget : public UPopupRowStackWidget
{
	GENERATED_BODY()

public:
	void AddItem(FName ItemIndex);

	// 줄 위젯 클래스. BP 기본값에서 WBP_ItemPopupRow를 고른다.
	UPROPERTY(EditDefaultsOnly, Category = "ItemPopup")
	TSubclassOf<UPopupRowWidget> RowClass;

	// 줄 위젯이 만들어졌을 때 호출된다. BP가 이름과 등급 색을 채운다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ItemPopup")
	void ReceiveRowCreated(UPopupRowWidget* Row, const FItemPopupEntry& Entry);
};
