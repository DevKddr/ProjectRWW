// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PopupRowStackWidget.generated.h"

class UOverlay;
class UPopupRowWidget;

// 목록에 있는 줄 하나의 상태.
USTRUCT()
struct FPopupRow
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UPopupRowWidget> Widget;

	double ExpireTime = 0.0;       // 나가기 시작할 시각
	double RemoveTime = 0.0;       // 없앨 시각(나가는 중일 때)
	int32 SlotIndex = INDEX_NONE;  // 마지막으로 알려준 줄 번호
	bool bAged = false;
};

// 팝업 줄 목록의 규칙: 최대 줄 수, 유지 시간, 오래된 줄부터 내보내기.
// 모양과 움직임은 줄 위젯(UPopupRowWidget의 BP)이 맡는다. 순수 로컬 UI다.
UCLASS()
class PROJECTRWW_API UPopupRowStackWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 새 줄을 맨 아래(0번)에 넣는다.
	void PushRow(UPopupRowWidget* Row);

	// 모든 줄을 즉시 없앤다(사망).
	void ClearRows();

	// 줄을 담을 Overlay. BP가 Construct에서 넣어 준다.
	UPROPERTY(BlueprintReadWrite, Category = "PopupRow")
	TObjectPtr<UOverlay> RowPanel;

	UPROPERTY(EditDefaultsOnly, Category = "PopupRow")
	int32 MaxRows = 3;

	// 각 줄이 유지되는 시간(초).
	UPROPERTY(EditDefaultsOnly, Category = "PopupRow")
	float RowSeconds = 3.0f;

	// 한꺼번에 들어온 줄들이 하나씩 나가는 최소 간격(초).
	UPROPERTY(EditDefaultsOnly, Category = "PopupRow")
	float RemoveIntervalSeconds = 0.25f;

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	// Rows[Index]를 나가는 목록으로 옮긴다.
	void StartLeaving(int32 Index);

	// 번호가 바뀐 줄에만 알려준다.
	void NotifySlotChanges();

	// 0번이 최신(맨 아래).
	UPROPERTY(Transient)
	TArray<FPopupRow> Rows;

	UPROPERTY(Transient)
	TArray<FPopupRow> LeavingRows;

	double Clock = 0.0;
	uint64 LastPushFrame = 0;
};
