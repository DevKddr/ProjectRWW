#include "MainGameState.h"
#include "Net/UnrealNetwork.h"

void AMainGameState::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		SessionStartServerTime = GetServerWorldTimeSeconds();
	}
}

void AMainGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMainGameState, SessionStartServerTime);
}
