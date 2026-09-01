// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainPlayerController.h"
#include "MainDeathWidget.h"
#include "MainHUDWidget.h"
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
#include "Items/MainInventoryWidget.h"
#include "Items/MainHotbarWidget.h"
#include "Net/UnrealNetwork.h"

AMainPlayerController::AMainPlayerController()
{
	InventoryComponent = CreateDefaultSubobject<UMainInventoryComponent>(TEXT("InventoryComponent"));
}

void AMainPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 핫바 1번(슬롯 0)을 자동으로 든다. 비어있으면 EquipItem 내부에서 조용히 무시됨.
	if (InventoryComponent)
	{
		InventoryComponent->EquipItem(0);
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

	// 핫바는 HUD와 생명주기를 같이한다 - 사망/리스폰 때 같이 없어졌다 다시 생김.
	if (IsLocalController() && HotbarWidgetClass)
	{
		if (!HotbarWidgetInstance)
		{
			HotbarWidgetInstance = CreateWidget<UMainHotbarWidget>(this, HotbarWidgetClass);
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
	if (!MapWidgetClass)
	{
		return;
	}

	if (MapWidgetInstance && MapWidgetInstance->IsInViewport())
	{
		MapWidgetInstance->RemoveFromParent();
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
		return;
	}

	if (!MapWidgetInstance)
	{
		MapWidgetInstance = CreateWidget<UMainMapWidget>(this, MapWidgetClass);
	}

	MapWidgetInstance->AddToViewport();
	SetInputMode(FInputModeGameAndUI());
	SetShowMouseCursor(true);
}

void AMainPlayerController::OnToggleInventory(const FInputActionValue& Value)
{
	if (!InventoryWidgetClass)
	{
		return;
	}

	if (InventoryWidgetInstance && InventoryWidgetInstance->IsInViewport())
	{
		// 드래그 중에 인벤토리를 닫으면 입력 모드가 게임 전용으로 바뀌면서 Slate가
		// 마우스 업 이벤트를 못 받아 드래그 오퍼레이션이 끝나지 못하고 유령 아이콘이
		// 화면에 계속 남는다. 닫기 전에 진행 중인 드래그를 강제로 취소한다.
		UWidgetBlueprintLibrary::CancelDragDrop();

		InventoryWidgetInstance->RemoveFromParent();
		if (HotbarWidgetInstance)
		{
			HotbarWidgetInstance->AddToViewport();  // 인벤토리 닫으면 핫바 다시 보임
		}
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
		return;
	}

	if (!InventoryWidgetInstance)
	{
		InventoryWidgetInstance = CreateWidget<UMainInventoryWidget>(this, InventoryWidgetClass);
	}

	InventoryWidgetInstance->AddToViewport();
	if (HotbarWidgetInstance)
	{
		HotbarWidgetInstance->RemoveFromParent();  // 인벤토리 열면 핫바 숨김 (같은 슬롯이 겹쳐 보이지 않게)
	}
	SetInputMode(FInputModeGameAndUI());
	SetShowMouseCursor(true);
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

