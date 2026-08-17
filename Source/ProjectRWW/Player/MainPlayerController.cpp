// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainPlayerController.h"
#include "Map/MainMapMarker.h"
#include "Map/MainMapMarkerComponent.h"
#include "Map/MainMapWidget.h"
#include "Database/MainPlayerDataRepository.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Blueprint/UserWidget.h"

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

// [테스트 코드] Task 2 검증용 — UMainPlayerDataRepository의 SQLite 저장/조회가 실제로 동작하는지 확인.
// 검증이 끝나면 삭제해도 된다.
void AMainPlayerController::RWW_TestPlayerData()
{
	UMainPlayerDataRepository* Repository = NewObject<UMainPlayerDataRepository>();
	Repository->Open(UMainPlayerDataRepository::GetDefaultDatabasePath());

	FMainPlayerRecord Record = Repository->LoadPlayerData(TEXT("TestPlayer"));
	UE_LOG(LogTemp, Warning, TEXT("로드됨 - KillCount: %d, DeathCount: %d, Currency: %d"), Record.KillCount, Record.DeathCount, Record.Currency);

	Record.KillCount += 1;
	Repository->SavePlayerData(Record);
	UE_LOG(LogTemp, Warning, TEXT("저장 완료 - KillCount: %d"), Record.KillCount);

	Repository->Close();
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
