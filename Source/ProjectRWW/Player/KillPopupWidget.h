// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "PopupRowStackWidget.h"
#include "Reward/KillNotice.h"
#include "KillPopupWidget.generated.h"

class UPopupRowWidget;

// 처치 팝업. 상대 이름은 부모의 줄 목록에 쌓고, 연속 킬 수와 Credit 합계를 BP에 알린다.
// 마지막 처치 후 RowSeconds가 지나면 끝난다. 순수 로컬 UI다.
UCLASS()
class PROJECTRWW_API UKillPopupWidget : public UPopupRowStackWidget
{
	GENERATED_BODY()

public:
	void AddKill(const FKillNotice& Notice);

	// 즉시 비운다(사망).
	void ClearPopup();

	// 이름 줄 위젯 클래스. BP 기본값에서 고른다.
	UPROPERTY(EditDefaultsOnly, Category = "KillPopup")
	TSubclassOf<UPopupRowWidget> NameRowClass;

	// 이름 줄이 만들어졌을 때. BP가 이름을 채운다.
	UFUNCTION(BlueprintImplementableEvent, Category = "KillPopup")
	void ReceiveNameRowCreated(UPopupRowWidget* Row, const FString& VictimName);

	// 처치 반영. TotalCredit은 팝업이 떠 있는 동안의 합계, bFirstKill은 팝업이 새로 뜨는 경우다.
	UFUNCTION(BlueprintImplementableEvent, Category = "KillPopup")
	void ReceiveKill(int32 KillStreak, int32 TotalCredit, bool bFirstKill);

	// 표시 시간이 끝났다. BP가 사라지는 연출을 한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "KillPopup")
	void ReceiveExpired();

	// 즉시 숨겨야 한다(사망).
	UFUNCTION(BlueprintImplementableEvent, Category = "KillPopup")
	void ReceiveCleared();

private:
	void HandleExpired();

	bool bShowing = false;
	int32 ShownTotalCredit = 0;
	FTimerHandle ExpireTimerHandle;
};
