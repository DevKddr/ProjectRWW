// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Database/MainPlayerRecord.h"
#include "MainPlayerController.generated.h"

// 인게임에서 키로 여닫는 UI 패널 종류. 한 번에 하나만 열린다.
// 새 패널(창고 등)이 생기면 여기에 값을 추가하고, .cpp의 HasPanelWidgetClass / OpenPanelWidget /
// ClosePanelWidget에 case를 하나씩 추가한다. (입력 모드/커서/상호 배제는 공통 코드가 처리한다)
UENUM()
enum class EMainUIPanel : uint8
{
	None,
	Inventory,
	Map,
	Menu
};

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

	// 탈출 성공으로 정산이 끝났음을 나타낸다. 사망(bIsDead)과 별개의 플래그다 -
	// Logout()이 탈출 성공 뒤의 접속 종료를 다시 사망으로 처리해 인벤토리를
	// 덮어쓰지 않도록 막는 가드로 쓰인다.
	bool bHasExtracted = false;

	// 지금 탈출 지점 콜리전 안에서 타이머가 도는 중인지. OnRep에서 시작 시각을
	// 자기 시계로 다시 찍는다(재장전의 bIsReloading/ReloadStartTimeSeconds와 동일 정책).
	UPROPERTY(ReplicatedUsing = OnRep_IsExtracting)
	bool bIsExtracting = false;

	// 지금 진행 중인 탈출의 총 소요 시간(초). AMainExtractionZone의 값이 그대로 복제된다.
	UPROPERTY(Replicated)
	float ExtractionDuration = 0.0f;

	// bIsExtracting이 true로 바뀐 순간을 각자(서버/클라)의 시계로 다시 찍는다.
	// 복제되는 값이 아니라 각자 로컬에서 찍는 값이다. MainHUDWidget::NativeTick()이
	// 이 값을 그대로 가져다 쓴다(재장전의 ReloadStartTimeSeconds와 동일 패턴).
	float ExtractionStartTimeSeconds = 0.0f;

	// 인벤토리 데이터/동작은 별도 컴포넌트로 분리했다 - PlayerRecord와 달리
	// 리플리케이션과 여러 동작을 갖는 시스템이라 MainWeaponComponent 등과 같은 패턴.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<class UMainInventoryComponent> InventoryComponent;

	UFUNCTION(Client, Reliable)
	void Client_OnPlayerDied(const FMainPlayerRecord& Record, int32 FinalKillStreak);

	// 서버 -> 쏜 사람: "네 총알이 캐릭터에 맞았다". 화면 표시용이라 유실돼도 게임에 영향이 없어 Unreliable로 보낸다.
	UFUNCTION(Client, Unreliable)
	void Client_OnHitConfirmed(bool bHeadshot);

	// 서버 -> 맞은 사람: "누가 어느 위치에서 이만큼 맞혔다".
	UFUNCTION(Client, Unreliable)
	void Client_OnDamaged(const FVector_NetQuantize& SourceLocation, float Damage, bool bHeadshot);

	// 서버 -> 킬한 사람: "네가 죽였다". 화면 표시용이라 유실돼도 게임에 영향이 없어 Unreliable로 보낸다.
	UFUNCTION(Client, Unreliable)
	void Client_OnKillConfirmed();

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

	UFUNCTION()
	void OnRep_IsExtracting();

	// 소유 클라이언트가 폰의 BeginPlay를 마친 뒤 서버에 보내는 신호.
	// 서버는 이걸 받은 뒤에야 스폰 초기화(보상/장착)를 실행한다 - 그 전에 장착하면
	// 클라이언트의 장착 BP 이벤트가 BeginPlay보다 먼저 돌아 애니메이션이 꼬인다.
	UFUNCTION(Server, Reliable)
	void Server_NotifyPawnReady(APawn* ReadyPawn);

	// Esc 메뉴를 닫는다. 키로 닫는 것이 기본이고, 이 함수는 BP에서 메뉴를 닫아야 할 때를 위해 남겨 둔 것이다.
	// RemoveFromParent만 하면 입력 모드/마우스 커서가 복구되지 않으므로 BP에서도 반드시 이 함수로 닫는다.
	// 서버 권위 대상이 아닌 순수 로컬 UI 동작이다(RPC/복제 없음).
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void CloseMenu();

protected:
	virtual void SetupInputComponent() override;

	// 서버에서 이 컨트롤러가 새 폰을 빙의할 때마다(최초 스폰+리스폰 모두) 호출된다.
	// 스폰 초기화(인벤토리 복원/스폰 보상/1번 슬롯 장착)는 GameMode::HandlePlayerSpawned()에 위임한다.
	virtual void OnPossess(APawn* InPawn) override;

	// 서버 전용: 새 폰을 빙의했지만 클라이언트 준비 신호를 아직 못 받은 상태.
	bool bSpawnInitPending = false;

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

	// 메뉴 키 입력. 메뉴를 열고 닫는다(열려 있는 인벤토리/지도는 자동으로 닫힌다).
	void OnToggleMenu(const struct FInputActionValue& Value);

	// --- 패널 관리 ---
	// 모든 패널(인벤토리/지도/메뉴)의 열기·닫기는 아래 함수를 거친다. 이 함수들은 순수 로컬 UI 상태만
	// 다루며 RPC/복제와 무관하다. 규칙:
	//  - 패널은 한 번에 하나만 열린다. 새 패널을 열면 열려 있던 패널은 먼저 닫힌다(나중에 연 것이 우선).
	//  - 메뉴는 항상 우선한다: 메뉴가 열려 있는 동안은 인벤토리/지도를 열 수 없다.
	//  - 같은 패널의 키를 다시 누르면 닫힌다.
	//  - 입력 모드와 마우스 커서는 ApplyInputModeForActivePanel()에서만 정한다.
	void TogglePanel(EMainUIPanel Panel);
	void OpenPanel(EMainUIPanel Panel);
	void ClosePanel(EMainUIPanel Panel);

	// 패널 종류별 실제 위젯 열기/닫기. 입력 모드는 건드리지 않는다. 새 패널은 여기에 case를 추가한다.
	bool OpenPanelWidget(EMainUIPanel Panel);
	void ClosePanelWidget(EMainUIPanel Panel);
	bool HasPanelWidgetClass(EMainUIPanel Panel) const;

	// ActivePanel에 맞춰 입력 모드(GameOnly / GameAndUI)와 마우스 커서를 한 곳에서 적용한다.
	void ApplyInputModeForActivePanel();

	// 핫키가 눌리면 호출된다. SlotIndex는 SetupInputComponent에서 바인딩할 때 미리 정해둔 값.
	void OnHotbarKeyPressed(int32 SlotIndex);

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<class UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<class UInputAction> ToggleMapAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<class UInputAction> ToggleInventoryAction;

	// Esc 메뉴 토글용 입력 액션(IA_Menu). 에디터에서 지정하고, IMC에 Escape 키를 매핑한다.
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<class UInputAction> ToggleMenuAction;

	// 인덱스 0~8이 각각 1~9번 핫키에 대응한다. 에디터에서 IA_Hotbar1~9를 순서대로 채워야 함.
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TArray<TObjectPtr<class UInputAction>> HotbarSlotActions;

	UPROPERTY(EditDefaultsOnly, Category = "Map")
	TSubclassOf<class UMainMapWidget> MapWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Death")
	TSubclassOf<class UMainDeathWidget> DeathWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSubclassOf<class UMainHUDWidget> HUDWidgetClass;

	// 바라보는 줍기 대상 위 마커/프롬프트 전용 뷰포트 위젯. HUD와 생명주기를 같이한다.
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSubclassOf<class UInteractionHUDWidget> InteractionHUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<class UMainInventoryScreenWidget> InventoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<class UMainSlotGridWidget> HotbarWidgetClass;

	// Esc로 여는 메뉴 위젯 클래스. 처음엔 설정 위젯(WBP_Settings)을 지정하고,
	// 나중에 일시정지 메뉴(계속하기/설정/나가기)로 교체할 수 있다.
	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	TSubclassOf<class UUserWidget> MenuWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<UMainMapWidget> MapWidgetInstance;

	UPROPERTY()
	TObjectPtr<UMainDeathWidget> DeathWidgetInstance;

	UPROPERTY()
	TObjectPtr<class UMainHUDWidget> HUDWidgetInstance;

	UPROPERTY()
	TObjectPtr<class UInteractionHUDWidget> InteractionHUDWidgetInstance;

	UPROPERTY()
	TObjectPtr<class UMainInventoryScreenWidget> InventoryWidgetInstance;

	UPROPERTY()
	TObjectPtr<class UMainSlotGridWidget> HotbarWidgetInstance;

	UPROPERTY()
	TObjectPtr<class UUserWidget> MenuWidgetInstance;

	// 지금 열려 있는 패널. 로컬 UI 상태라 복제하지 않는다(서버와 무관).
	EMainUIPanel ActivePanel = EMainUIPanel::None;
};
