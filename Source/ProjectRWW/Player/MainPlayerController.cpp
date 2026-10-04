// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainPlayerController.h"
#include "MainDeathWidget.h"
#include "MainHUDWidget.h"
#include "InteractionHUDWidget.h"
#include "Core/MainNetworkSettings.h"
#include "Core/MainGameMode.h"
#include "Map/MainMapMarker.h"
#include "Map/MainMapMarkerComponent.h"
#include "Map/MainMapWidget.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Items/MainInventoryComponent.h"
#include "Items/MainInventoryScreenWidget.h"
#include "Items/MainSlotGridWidget.h"
#include "Net/UnrealNetwork.h"

AMainPlayerController::AMainPlayerController()
{
	InventoryComponent = CreateDefaultSubobject<UMainInventoryComponent>(TEXT("InventoryComponent"));
}

void AMainPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 원격 클라이언트는 폰 BeginPlay가 끝났다는 신호(Server_NotifyPawnReady)를 기다린다.
	// 리슨 서버 호스트는 기다릴 클라이언트가 따로 없으므로 바로 실행한다.
	// DB 로드/인벤토리 복원/스폰 보상/장착은 GameMode::HandlePlayerSpawned()가 처리한다.
	bSpawnInitPending = true;
	if (IsLocalController())
	{
		bSpawnInitPending = false;
		if (AMainGameMode* GameMode = GetWorld()->GetAuthGameMode<AMainGameMode>())
		{
			GameMode->HandlePlayerSpawned(this);
		}
	}
}

void AMainPlayerController::Server_NotifyPawnReady_Implementation(APawn* ReadyPawn)
{
	// 클라이언트가 보낸 값은 믿지 않는다 - 대기 중이고 지금 빙의한 폰일 때만 처리한다.
	if (!bSpawnInitPending || !ReadyPawn || ReadyPawn != GetPawn())
	{
		return;
	}
	bSpawnInitPending = false;

	if (AMainGameMode* GameMode = GetWorld()->GetAuthGameMode<AMainGameMode>())
	{
		GameMode->HandlePlayerSpawned(this);
	}
}

void AMainPlayerController::ClientRestart_Implementation(APawn* NewPawn)
{
	Super::ClientRestart_Implementation(NewPawn);

	// HUD는 소유 클라이언트에서만 띄운다 (서버/다른 클라이언트에서 실행되면 안 됨).
	// 최초 스폰이든 사망 후 리스폰이든 이 함수가 항상 다시 불리므로, HUD 생성/재표시를
	// 여기 한 곳에만 두면 BeginPlay에 따로 만들어둘 필요가 없다.
	if (IsLocalController() && HUDWidgetClass)
	{
		if (!HUDWidgetInstance)
		{
			HUDWidgetInstance = CreateWidget<UMainHUDWidget>(this, HUDWidgetClass);
		}

		if (HUDWidgetInstance && !HUDWidgetInstance->IsInViewport())
		{
			HUDWidgetInstance->AddToViewport();
		}
	}

	// 줍기 마커도 HUD와 생명주기를 같이한다. 나중에 추가된 위젯이 위에 그려지므로 HUD 위에,
	// 인벤토리/지도 창(사용자가 나중에 여는 것) 아래에 놓인다.
	if (IsLocalController() && InteractionHUDWidgetClass)
	{
		if (!InteractionHUDWidgetInstance)
		{
			InteractionHUDWidgetInstance = CreateWidget<UInteractionHUDWidget>(this, InteractionHUDWidgetClass);
		}

		if (InteractionHUDWidgetInstance && !InteractionHUDWidgetInstance->IsInViewport())
		{
			InteractionHUDWidgetInstance->AddToViewport();
		}
	}

	// 핫바는 HUD와 생명주기를 같이한다 - 사망/리스폰 때 같이 없어졌다 다시 생김.
	if (IsLocalController() && HotbarWidgetClass)
	{
		if (!HotbarWidgetInstance)
		{
			HotbarWidgetInstance = CreateWidget<UMainSlotGridWidget>(this, HotbarWidgetClass);
			HotbarWidgetInstance->SetContainerComponent(InventoryComponent);
		}

		if (HotbarWidgetInstance && !HotbarWidgetInstance->IsInViewport())
		{
			HotbarWidgetInstance->AddToViewport();
		}
	}
}

void AMainPlayerController::CloseAllGameplayUI()
{
	// 드래그 중에 어떤 UI든 강제로 닫힐 수 있는 지점(사망, 나중엔 창고 닫기 등)이라
	// 맨 앞에서 한 번에 처리한다. 드래그 중이 아니면 아무 일도 안 하니 항상 호출해도 안전하다.
	UWidgetBlueprintLibrary::CancelDragDrop();

	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->RemoveFromParent();
	}

	if (InteractionHUDWidgetInstance)
	{
		InteractionHUDWidgetInstance->RemoveFromParent();
	}

	if (HotbarWidgetInstance)
	{
		HotbarWidgetInstance->RemoveFromParent();
	}

	if (InventoryWidgetInstance && InventoryWidgetInstance->IsInViewport())
	{
		InventoryWidgetInstance->RemoveFromParent();
	}

	if (MapWidgetInstance && MapWidgetInstance->IsInViewport())
	{
		MapWidgetInstance->RemoveFromParent();
	}

	// 사망 등으로 UI를 정리할 때 Esc 메뉴가 남아 있으면 같이 닫는다.
	// 입력 모드/커서는 호출한 쪽(사망 UI 등)이 직접 정하므로 여기서는 위젯만 제거한다.
	if (MenuWidgetInstance && MenuWidgetInstance->IsInViewport())
	{
		MenuWidgetInstance->RemoveFromParent();
	}

	// 위젯을 모두 직접 제거했으므로 "열린 패널" 상태도 비운다(리스폰 뒤 상태 불일치 방지).
	ActivePanel = EMainUIPanel::None;
}

void AMainPlayerController::Client_OnHitConfirmed_Implementation(bool bHeadshot)
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->ReceiveHitConfirmed(bHeadshot);
	}
}

void AMainPlayerController::Client_OnDamaged_Implementation(const FVector_NetQuantize& SourceLocation, float Damage, bool bHeadshot)
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->ReceiveDamaged(SourceLocation, Damage, bHeadshot);
	}
}

void AMainPlayerController::Client_OnKillConfirmed_Implementation()
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->ReceiveKillConfirmed();
	}
}

void AMainPlayerController::Client_OnPlayerDied_Implementation(const FMainPlayerRecord& Record, int32 FinalKillStreak)
{
	// 사망 시엔 사망 UI만 남기고 다른 목적으로 열려있던 UI는 전부 닫는다.
	CloseAllGameplayUI();

	if (DeathWidgetClass)
	{
		DeathWidgetInstance = CreateWidget<UMainDeathWidget>(this, DeathWidgetClass);
		if (DeathWidgetInstance)
		{
			DeathWidgetInstance->PlayerRecord = Record;
			DeathWidgetInstance->FinalKillStreak = FinalKillStreak;
			DeathWidgetInstance->AddToViewport();
			SetInputMode(FInputModeUIOnly());
			SetShowMouseCursor(true);
		}
	}
}

void AMainPlayerController::Server_RequestRespawn_Implementation()
{
	if (!bIsDead)
	{
		return;
	}

	bIsDead = false;

	if (AGameModeBase* GameMode = GetWorld()->GetAuthGameMode())
	{
		GameMode->RestartPlayer(this);
	}
}

void AMainPlayerController::Server_RequestReturnToLobby_Implementation()
{
	// 살아있는 채로 로비 복귀를 요청하면, 자진 이탈로 간주해 사망과 동일하게 정산한다.
	if (!bIsDead)
	{
		if (AMainGameMode* GameMode = GetWorld()->GetAuthGameMode<AMainGameMode>())
		{
			GameMode->HandlePlayerDeath(this, nullptr);
		}
	}

	const FString LobbyAddress = GetDefault<UMainNetworkSettings>()->LobbyAddress;
	const FString TravelURL = FString::Printf(TEXT("%s?PlayerID=%s"), *LobbyAddress, *PlayerRecord.PlayerID);
	ClientTravel(TravelURL, ETravelType::TRAVEL_Absolute);
}

void AMainPlayerController::RWW_SpawnExtractionMarker(float X, float Y, float Z)
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ProjectRWW] RWW_SpawnExtractionMarker ignored: not server authority"));
		return;
	}

	FActorSpawnParameters SpawnParams;
	AMainMapMarker* NewMarker = GetWorld()->SpawnActor<AMainMapMarker>(FVector(X, Y, Z), FRotator::ZeroRotator, SpawnParams);

	if (NewMarker && NewMarker->MarkerComponent)
	{
		NewMarker->MarkerComponent->MarkerType = EMainMapMarkerType::Extraction;
		NewMarker->MarkerComponent->DisplayName = FText::FromString(TEXT("Test Extraction Point"));
		NewMarker->MarkerComponent->IconTint = FLinearColor::Green;

		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] Spawned %s at (%.1f, %.1f, %.1f)"), *GetNameSafe(NewMarker), X, Y, Z);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[ProjectRWW] Failed to spawn AMainMapMarker at (%.1f, %.1f, %.1f)"), X, Y, Z);
	}
}

void AMainPlayerController::RWW_ClearMapMarkers()
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ProjectRWW] RWW_ClearMapMarkers ignored: not server authority"));
		return;
	}

	TArray<AActor*> FoundMarkers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMainMapMarker::StaticClass(), FoundMarkers);

	for (AActor* Marker : FoundMarkers)
	{
		Marker->Destroy();
	}

	UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] Cleared %d map marker(s)"), FoundMarkers.Num());
}

void AMainPlayerController::RWW_AddItem(const FString& ItemIndex)
{
	if (InventoryComponent)
	{
		// AddItem()을 직접 부르지 않고 RPC를 거친다 - 클라이언트에서 이 명령어를
		// 입력해도 항상 서버의 진짜 데이터에 반영되게 하기 위함.
		InventoryComponent->Server_AddItem(FName(*ItemIndex));
	}
}

void AMainPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (ToggleMapAction)
		{
			EnhancedInput->BindAction(ToggleMapAction, ETriggerEvent::Started, this, &AMainPlayerController::OnToggleMap);
		}

		if (ToggleInventoryAction)
		{
			EnhancedInput->BindAction(ToggleInventoryAction, ETriggerEvent::Started, this, &AMainPlayerController::OnToggleInventory);
		}

		if (ToggleMenuAction)
		{
			EnhancedInput->BindAction(ToggleMenuAction, ETriggerEvent::Started, this, &AMainPlayerController::OnToggleMenu);
		}

		for (int32 i = 0; i < HotbarSlotActions.Num(); ++i)
		{
			if (HotbarSlotActions[i])
			{
				EnhancedInput->BindAction(HotbarSlotActions[i], ETriggerEvent::Started, this, &AMainPlayerController::OnHotbarKeyPressed, i);
			}
		}
	}
}

void AMainPlayerController::OnToggleMap(const FInputActionValue& Value)
{
	TogglePanel(EMainUIPanel::Map);
}

void AMainPlayerController::OnToggleInventory(const FInputActionValue& Value)
{
	TogglePanel(EMainUIPanel::Inventory);
}

void AMainPlayerController::OnToggleMenu(const FInputActionValue& Value)
{
	TogglePanel(EMainUIPanel::Menu);
}

void AMainPlayerController::CloseMenu()
{
	ClosePanel(EMainUIPanel::Menu);
}

bool AMainPlayerController::HasPanelWidgetClass(EMainUIPanel Panel) const
{
	switch (Panel)
	{
	case EMainUIPanel::Inventory:
		return InventoryWidgetClass.Get() != nullptr;
	case EMainUIPanel::Map:
		return MapWidgetClass.Get() != nullptr;
	case EMainUIPanel::Menu:
		return MenuWidgetClass.Get() != nullptr;
	default:
		return false;
	}
}

void AMainPlayerController::TogglePanel(EMainUIPanel Panel)
{
	if (Panel != EMainUIPanel::None && ActivePanel == Panel)
	{
		ClosePanel(Panel);
	}
	else
	{
		OpenPanel(Panel);
	}
}

void AMainPlayerController::OpenPanel(EMainUIPanel Panel)
{
	if (Panel == EMainUIPanel::None || ActivePanel == Panel)
	{
		return;
	}

	// 사망 UI가 떠 있는 동안에는 어떤 패널도 열지 않는다(사망 시 HUD를 포함한 모든 위젯이 닫히고 사망 위젯만 남는다).
	if (DeathWidgetInstance && DeathWidgetInstance->IsInViewport())
	{
		return;
	}

	// 위젯 클래스가 지정되지 않은 패널은 열 수 없다. 다른 패널을 닫지도 않는다.
	if (!HasPanelWidgetClass(Panel))
	{
		return;
	}

	// 메뉴는 인벤토리/지도보다 우선한다: 메뉴가 열려 있는 동안은 다른 패널을 열 수 없다.
	if (ActivePanel == EMainUIPanel::Menu && Panel != EMainUIPanel::Menu)
	{
		return;
	}

	// 패널은 한 번에 하나만 열린다: 열려 있는 다른 패널은 먼저 닫는다(나중에 연 것이 우선).
	if (ActivePanel != EMainUIPanel::None)
	{
		ClosePanelWidget(ActivePanel);
		ActivePanel = EMainUIPanel::None;
	}

	if (OpenPanelWidget(Panel))
	{
		ActivePanel = Panel;
	}

	ApplyInputModeForActivePanel();
}

void AMainPlayerController::ClosePanel(EMainUIPanel Panel)
{
	// 지금 열려 있는 패널이 아니면 아무것도 하지 않는다 - 다른 패널이 설정한 입력 모드를 건드리지 않기 위함.
	if (Panel == EMainUIPanel::None || ActivePanel != Panel)
	{
		return;
	}

	ClosePanelWidget(Panel);
	ActivePanel = EMainUIPanel::None;
	ApplyInputModeForActivePanel();
}

bool AMainPlayerController::OpenPanelWidget(EMainUIPanel Panel)
{
	switch (Panel)
	{
	case EMainUIPanel::Inventory:
		if (!InventoryWidgetInstance)
		{
			InventoryWidgetInstance = CreateWidget<UMainInventoryScreenWidget>(this, InventoryWidgetClass);
			if (InventoryWidgetInstance)
			{
				InventoryWidgetInstance->SetContainerComponent(InventoryComponent);
			}
		}
		if (!InventoryWidgetInstance)
		{
			return false;
		}
		InventoryWidgetInstance->AddToViewport();
		if (HotbarWidgetInstance)
		{
			HotbarWidgetInstance->RemoveFromParent();  // 인벤토리 열면 핫바 숨김 (같은 슬롯이 겹쳐 보이지 않게)
		}
		return true;

	case EMainUIPanel::Map:
		if (!MapWidgetInstance)
		{
			MapWidgetInstance = CreateWidget<UMainMapWidget>(this, MapWidgetClass);
		}
		if (!MapWidgetInstance)
		{
			return false;
		}
		MapWidgetInstance->AddToViewport();
		return true;

	case EMainUIPanel::Menu:
		if (!MenuWidgetInstance)
		{
			MenuWidgetInstance = CreateWidget<UUserWidget>(this, MenuWidgetClass);
		}
		if (!MenuWidgetInstance)
		{
			return false;
		}
		MenuWidgetInstance->AddToViewport();
		return true;

	default:
		return false;
	}
}

void AMainPlayerController::ClosePanelWidget(EMainUIPanel Panel)
{
	switch (Panel)
	{
	case EMainUIPanel::Inventory:
		// 드래그 중에 인벤토리를 닫으면 입력 모드가 게임 전용으로 바뀌면서 Slate가
		// 마우스 업 이벤트를 못 받아 드래그 오퍼레이션이 끝나지 못하고 유령 아이콘이
		// 화면에 계속 남는다. 닫기 전에 진행 중인 드래그를 강제로 취소한다.
		UWidgetBlueprintLibrary::CancelDragDrop();

		if (InventoryWidgetInstance && InventoryWidgetInstance->IsInViewport())
		{
			InventoryWidgetInstance->RemoveFromParent();
		}
		if (HotbarWidgetInstance && !HotbarWidgetInstance->IsInViewport())
		{
			HotbarWidgetInstance->AddToViewport();  // 인벤토리 닫으면 핫바 다시 보임
		}
		break;

	case EMainUIPanel::Map:
		if (MapWidgetInstance && MapWidgetInstance->IsInViewport())
		{
			MapWidgetInstance->RemoveFromParent();
		}
		break;

	case EMainUIPanel::Menu:
		if (MenuWidgetInstance && MenuWidgetInstance->IsInViewport())
		{
			MenuWidgetInstance->RemoveFromParent();
		}
		break;

	default:
		break;
	}
}

void AMainPlayerController::ApplyInputModeForActivePanel()
{
	// 입력 모드와 마우스 커서는 여기서만 정한다. 패널이 하나라도 열려 있으면 UI 조작이 가능해야 하고,
	// 없으면 게임 입력으로 복귀한다.
	// UIOnly로 하면 컨트롤러의 입력 바인딩이 동작하지 않아 키로 다시 닫을 수 없으므로 GameAndUI를 쓴다.
	// 게임은 멈추지 않는다(멀티플레이에서 일시정지 금지). 서버/복제와 무관한 로컬 상태다.
	if (ActivePanel == EMainUIPanel::None)
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
	}
	else
	{
		SetInputMode(FInputModeGameAndUI());
		SetShowMouseCursor(true);
	}
}

void AMainPlayerController::OnHotbarKeyPressed(int32 SlotIndex)
{
	if (InventoryComponent)
	{
		InventoryComponent->Server_EquipItem(SlotIndex);
	}
}

void AMainPlayerController::OnRep_IsExtracting()
{
	if (bIsExtracting)
	{
		ExtractionStartTimeSeconds = GetWorld()->GetTimeSeconds();
	}
}

void AMainPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AMainPlayerController, bIsExtracting, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AMainPlayerController, ExtractionDuration, COND_OwnerOnly);
}

