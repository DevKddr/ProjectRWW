// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Database/MainPlayerRecord.h"
#include "MainPlayerController.generated.h"

UCLASS()
class PROJECTRWW_API AMainPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// 서버 전용 데이터. non-replicated라 클라이언트로는 절대 전송되지 않는다.
	FMainPlayerRecord PlayerRecord;

	// 죽으면 0으로 리셋되는 순수 런타임 값. DB에 저장 안 함.
	int32 KillStreak = 0;

	// 서버가 "죽었다"고 판단한 상태. 리스폰/로비복귀 RPC의 유효성 검증에 사용 —
	// 이게 없으면 살아있는 클라이언트가 임의로 RPC를 호출해 악용할 수 있다.
	bool bIsDead = false;

	UFUNCTION(Client, Reliable)
	void Client_OnPlayerDied(const FMainPlayerRecord& Record, int32 FinalKillStreak);

	UFUNCTION(Server, Reliable)
	void Server_RequestRespawn();

	UFUNCTION(Server, Reliable)
	void Server_RequestReturnToLobby();

	// 디버그용: "RWW_SpawnExtractionMarker 100 200 0"처럼 X Y Z 좌표를 받아 마커를 스폰한다.
	UFUNCTION(Exec)
	void RWW_SpawnExtractionMarker(float X, float Y, float Z);

	// 디버그용: 지금까지 스폰한 모든 AMainMapMarker를 제거한다.
	UFUNCTION(Exec)
	void RWW_ClearMapMarkers();

protected:
	virtual void SetupInputComponent() override;

	void OnToggleMap(const struct FInputActionValue& Value);

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<class UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<class UInputAction> ToggleMapAction;

	UPROPERTY(EditDefaultsOnly, Category = "Map")
	TSubclassOf<class UMainMapWidget> MapWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Death")
	TSubclassOf<class UMainDeathWidget> DeathWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<UMainMapWidget> MapWidgetInstance;

	UPROPERTY()
	TObjectPtr<UMainDeathWidget> DeathWidgetInstance;
};
