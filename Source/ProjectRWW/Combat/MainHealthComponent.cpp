// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainHealthComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

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

	UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] %s took %.1f damage, health now %.1f"), *GetNameSafe(DamagedActor), Damage, CurrentHealth);
}

void UMainHealthComponent::OnRep_CurrentHealth()
{
	UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] (client) %s health replicated: %.1f"), *GetNameSafe(GetOwner()), CurrentHealth);
}

void UMainHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UMainHealthComponent, CurrentHealth);
}
