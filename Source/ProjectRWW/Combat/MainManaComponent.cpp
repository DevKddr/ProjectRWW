// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainManaComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"
#include "PlayerBaseStat/PlayerStatManager.h"

UMainManaComponent::UMainManaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UMainManaComponent::BeginPlay()
{
	Super::BeginPlay();

	// 서버에서만 계산한다 — MaxMana/CurrentMana 둘 다 복제되므로 클라이언트는 서버 값을
	// 받기만 하면 된다. 로드가 실패했으면 0으로 남아 눈에 띄게 망가지는 쪽을 택했다.
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
		{
			if (UPlayerStatManager* StatManager = GameInstance->GetSubsystem<UPlayerStatManager>())
			{
				const FPlayerBaseStat& Stat = StatManager->GetBaseStat();
				MaxMana = static_cast<float>(Stat.MaxMana);
				ManaRegen = Stat.ManaRegen;
			}
		}

		CurrentMana = MaxMana;
	}
}

void UMainManaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 회복 계산은 서버에서만 — 클라이언트가 계산하면 그 값을 신뢰할 수 없다.
	if (GetOwner() && GetOwner()->HasAuthority() && CurrentMana < MaxMana)
	{
		CurrentMana = FMath::Min(CurrentMana + ManaRegen * DeltaTime, MaxMana);
	}
}

bool UMainManaComponent::ConsumeMana(float Amount)
{
	if (CurrentMana < Amount)
	{
		return false;
	}

	CurrentMana -= Amount;
	return true;
}

void UMainManaComponent::OnRep_CurrentMana()
{
	// TODO: 마나바 UI 갱신 등, 클라이언트 반응 로직이 필요해지면 여기에 추가
}

void UMainManaComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UMainManaComponent, CurrentMana);
	DOREPLIFETIME(UMainManaComponent, MaxMana);
}
