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
	AMainPlayerController();

	// 서버 전용 데이터. non-replicated라 클라이언트로는 절대 전송되지 않는다.
	FMainPlayerRecord PlayerRecord;

	// 죽으면 0으로 리셋되는 순수 런타임 값. DB에 저장 안 함.
	int32 KillStreak = 0;

	// 서버가 "죽었다"고 판단한 상태. 리스폰/로비복귀 RPC의 유효성 검증에 사용 —
	// 이게 없으면 살아있는 클라이언트가 임의로 RPC를 호출해 악용할 수 있다.
	bool bIsDead = false;

	// 인벤토리 데이터/동작은 별도 컴포넌트로 분리했다 - PlayerRecord와 달리
	// 리플리케이션과 여러 동작을 갖는 시스템이라 MainWeaponComponent 등과 같은 패턴.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<class UMainInventoryComponent> InventoryComponent;

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

	// 디버그용: "RWW_AddItem AR_1"처럼 아이템 Index를 받아서 인벤토리에 넣는다.
	UFUNCTION(Exec)
	void RWW_AddItem(const FString& ItemIndex);

protected:
	virtual void SetupInputComponent() override;

	// 서버에서 이 컨트롤러가 새 폰을 빙의할 때마다(최초 스폰+리스폰 모두) 호출된다.
	// 핫바 1번(슬롯 0)을 자동으로 장착시키는 용도.
	virtual void OnPossess(APawn* InPawn) override;

	// 서버가 이 컨트롤러에 새 Pawn을 Possess시킬 때마다(최초 스폰 + 리스폰 모두) 호출된다.
	// HUD 생성/재표시를 여기 한 곳에서만 관리한다.
	virtual void ClientRestart_Implementation(APawn* NewPawn) override;

	// 사망 등, 게임플레이 중 열려있던 UI를 전부 정리해야 하는 상황에서 호출한다.
	// 새 UI(인벤토리 등)가 생기면 여기에 한 줄만 추가하면 된다.
	void CloseAllGameplayUI();

	void OnToggleMap(const struct FInputActionValue& Value);

	// 인벤토리 창을 열고 닫는다. 열 때 핫바 HUD를 숨기고, 닫을 때 다시 보여준다
	// (같은 슬롯 0~8이 두 군데(창+핫바)에 동시에 겹쳐 보이지 않게 하기 위함).
	void OnToggleInventory(const struct FInputActionValue& Value);

	// 핫키가 눌리면 호출된다. SlotIndex는 SetupInputComponent에서 바인딩할 때 미리 정해둔 값.
	void OnHotbarKeyPressed(int32 SlotIndex);

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<class UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<class UInputAction> ToggleMapAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<class UInputAction> ToggleInventoryAction;

	// 인덱스 0~8이 각각 1~9번 핫키에 대응한다. 에디터에서 IA_Hotbar1~9를 순서대로 채워야 함.
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TArray<TObjectPtr<class UInputAction>> HotbarSlotActions;

	UPROPERTY(EditDefaultsOnly, Category = "Map")
	TSubclassOf<class UMainMapWidget> MapWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Death")
	TSubclassOf<class UMainDeathWidget> DeathWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSubclassOf<class UMainHUDWidget> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<class UMainInventoryWidget> InventoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<class UMainHotbarWidget> HotbarWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<UMainMapWidget> MapWidgetInstance;

	UPROPERTY()
	TObjectPtr<UMainDeathWidget> DeathWidgetInstance;

	UPROPERTY()
	TObjectPtr<class UMainHUDWidget> HUDWidgetInstance;

	UPROPERTY()
	TObjectPtr<class UMainInventoryWidget> InventoryWidgetInstance;

	UPROPERTY()
	TObjectPtr<class UMainHotbarWidget> HotbarWidgetInstance;
};
