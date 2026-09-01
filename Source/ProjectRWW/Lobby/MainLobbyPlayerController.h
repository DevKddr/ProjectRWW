// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Database/MainPlayerRecord.h"
#include "Database/MainSessionServerStatusRepository.h"
#include "MainLobbyPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSessionListUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerRecordUpdated);

UCLASS()
class PROJECTRWW_API AMainLobbyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMainLobbyPlayerController();

	void SetPlayerRecord(const FMainPlayerRecord& Record);

	UFUNCTION(BlueprintPure, Category = "Lobby")
	const FMainPlayerRecord& GetPlayerRecord() const { return PlayerRecord; }

	// 라이드용 36칸 인벤토리를 데이터 컨테이너로만 재사용한다. 로비엔 폰이 없어
	// MainInventoryComponent의 장착 로직은 자연히 아무 동작도 하지 않는다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lobby")
	TObjectPtr<class UMainInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lobby")
	TObjectPtr<class UMainStorageComponent> StorageComponent;

	// PlayerRecord가 리플리케이트되어 도착할 때마다 브로드캐스트한다(최초 로드, 닉네임 변경 등).
	// 로비 위젯은 여기 바인딩해서 값이 바뀔 때마다 자동으로 화면을 갱신하면 된다.
	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FOnPlayerRecordUpdated OnPlayerRecordUpdated;

	UFUNCTION(Exec)
	void RWW_SearchSession();

	UFUNCTION(Exec)
	void RWW_AddItem(const FString& ItemIndex);

	UFUNCTION(Server, Reliable)
	void Server_SearchSession();

	// 로비 UI의 "닉네임 변경"에서 호출한다. PlayerRecord.PlayerID는 건드리지 않는다 —
	// 세션 서버로 넘어갈 때 신원을 전달하는 통로라, 닉네임 변경과 절대 섞이면 안 된다.
	UFUNCTION(Server, Reliable)
	void Server_SetPlayerName(const FString& NewName);

	// 로비 UI의 "새로고침" 버튼에서만 호출한다 — 자동 폴링(Tick/타이머)으로 부르면
	// 호출할 때마다 DB 파일을 열고 닫아서 서버에 불필요한 부하를 준다.
	UFUNCTION(Server, Reliable)
	void Server_RequestSessionList();

	UFUNCTION(Client, Reliable)
	void Client_ReceiveSessionList(const TArray<FMainSessionServerStatus>& Servers);

	UFUNCTION(BlueprintPure, Category = "Lobby")
	const TArray<FMainSessionServerStatus>& GetCachedSessionList() const { return CachedSessionList; }

	// Client_ReceiveSessionList가 목록을 갱신할 때마다 브로드캐스트한다. 로비 위젯은
	// 여기에 바인딩해서 새 목록이 도착하면 리스트 UI를 다시 그리면 된다.
	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FOnSessionListUpdated OnSessionListUpdated;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_PlayerRecord();

	UPROPERTY(EditDefaultsOnly, Category = "Lobby")
	TSubclassOf<class UMainLobbyWidget> LobbyWidgetClass;

private:
	// 로비를 떠날 때(라이드 진입/접속 종료) 인벤토리+창고를 직렬화해서 DB에 저장한다.
	// 매번 이동할 때마다 저장하지 않는다 - 이벤트 기반 저장 정책.
	void SavePlayerRecord();

	// 본인 데이터라 다른 클라이언트한테는 안 보내도 된다(COND_OwnerOnly).
	UPROPERTY(ReplicatedUsing = OnRep_PlayerRecord)
	FMainPlayerRecord PlayerRecord;

	UPROPERTY()
	TArray<FMainSessionServerStatus> CachedSessionList;

	UPROPERTY()
	TObjectPtr<class UMainLobbyWidget> LobbyWidgetInstance;
};
