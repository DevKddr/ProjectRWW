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
}

void AMainPlayerController::CloseAllGameplayUI()
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->RemoveFromParent();
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
