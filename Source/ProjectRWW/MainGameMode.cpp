#include "MainGameMode.h"
#include "MainGameState.h"
#include "MainPlayerController.h"
#include "MainPlayerState.h"
#include "MainCharacter.h"

AMainGameMode::AMainGameMode()
{
	GameStateClass = AMainGameState::StaticClass();
	PlayerControllerClass = AMainPlayerController::StaticClass();
	PlayerStateClass = AMainPlayerState::StaticClass();
	DefaultPawnClass = AMainCharacter::StaticClass();
}

void AMainGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] Player joined: %s"), *GetNameSafe(NewPlayer));
}

void AMainGameMode::Logout(AController* Exiting)
{
	UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] Player left: %s"), *GetNameSafe(Exiting));

	Super::Logout(Exiting);
}
