// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Database/MainPlayerRecord.h"
#include "MainDeathWidget.generated.h"

UCLASS()
class PROJECTRWW_API UMainDeathWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Death")
	FMainPlayerRecord PlayerRecord;

	// DB에 저장되지 않는 값 — 이번 목숨 한정 기록이라 PlayerRecord와 분리해서 둔다.
	UPROPERTY(BlueprintReadOnly, Category = "Death")
	int32 FinalKillStreak = 0;

	UFUNCTION(BlueprintCallable, Category = "Death")
	void RequestRespawn();

	UFUNCTION(BlueprintCallable, Category = "Death")
	void RequestReturnToLobby();
};
