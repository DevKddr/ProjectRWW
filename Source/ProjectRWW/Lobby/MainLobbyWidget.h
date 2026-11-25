// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Database/MainSessionServerStatusRepository.h"
#include "MainLobbyPlayerController.h"
#include "MainLobbyWidget.generated.h"

UCLASS()
class PROJECTRWW_API UMainLobbyWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandlePlayerRecordUpdated();

	UFUNCTION()
	void HandleSessionListUpdated();

public:
	// 블루프린트에서 "닉네임 변경" 버튼 클릭 시, 입력창 텍스트를 넘겨서 호출한다.
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void OnSetNameClicked(const FString& NewName);

	// 블루프린트에서 "접속하기" 버튼 클릭 시 호출한다.
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void OnSearchSessionClicked();

	// 블루프린트에서 "새로고침" 버튼 클릭 시에만 호출한다 — Tick이나 타이머로 자동 호출하지
	// 말 것 (호출할 때마다 서버가 DB 파일을 열고 닫는다).
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void OnRefreshSessionListClicked();

	// PlayerController의 OnPlayerRecordUpdated가 발생할 때만 갱신된다(매 프레임 갱신 아님).
	// 사망 UI(UMainDeathWidget)와 같은 방식 - 레코드를 통째로 들고 있고, BP는 필드를
	// (PlayerID, PlayerName, Credit, KillCount ...) 꺼내서 텍스트에 바인딩한다.
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	FMainPlayerRecord PlayerRecord;

	// 새 세션 목록이 도착하면 자동으로 채워진다. 블루프린트는 캐스트/바인딩 없이
	// 이 배열만 보면 된다.
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	TArray<FMainSessionServerStatus> SessionList;

	// 블루프린트가 여기 바인딩해서 "목록이 갱신됐으니 UI 다시 그려라" 신호를 받는다.
	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FOnSessionListUpdated OnSessionListRefreshed;
};
