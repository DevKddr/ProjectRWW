// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainHealthComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

UMainHealthComponent::UMainHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UMainHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

	// 서버에서만 델리게이트를 등록한다. 클라이언트에서 등록하면 클라이언트 로컬에서
	// 체력이 깎이는 경로가 생겨 서버 권위 원칙이 깨진다.
	if (AActor* Owner = GetOwner())
	{
		if (Owner->HasAuthority())
		{
			Owner->OnTakeAnyDamage.AddDynamic(this, &UMainHealthComponent::OnTakeAnyDamage);
		}
	}
}

void UMainHealthComponent::OnTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.0f || IsDead())
	{
		return;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth - Damage, 0.0f, MaxHealth);

	// 가해자/피해자 이름은 PlayerState의 PlayerName(=Title 화면에서 입력한 PlayerID)을 쓴다.
	// 가해자는 InstigatedBy(컨트롤러)에서 바로 접근 가능하지만, 피해자는 DamagedActor(폰)만
	// 넘어오므로 폰 -> 컨트롤러 -> PlayerState 순서로 한 단계씩 거쳐야 한다.
	const FString KillerName = (InstigatedBy && InstigatedBy->PlayerState)
		? InstigatedBy->PlayerState->GetPlayerName()
		: TEXT("Unknown");

	const APawn* VictimPawn = Cast<APawn>(DamagedActor);
	const AController* VictimController = VictimPawn ? VictimPawn->GetController() : nullptr;
	const FString VictimName = (VictimController && VictimController->PlayerState)
		? VictimController->PlayerState->GetPlayerName()
		: GetNameSafe(DamagedActor);

	UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] %s -> %s: %.1f damage, health now %.1f"),
		*KillerName, *VictimName, Damage, CurrentHealth);

	if (IsDead())
	{
		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] %s died (killed by %s)"), *VictimName, *KillerName);
		OnDeath.Broadcast(InstigatedBy);
	}
}

void UMainHealthComponent::OnRep_CurrentHealth()
{
	// TODO: 체력바 UI 갱신 등, 클라이언트 반응 로직이 필요해지면 여기에 추가
}

void UMainHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UMainHealthComponent, CurrentHealth);
}
