// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainHPComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "PlayerBaseStat/PlayerStatManager.h"

UMainHPComponent::UMainHPComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UMainHPComponent::BeginPlay()
{
	Super::BeginPlay();

	// 서버에서만 계산한다 — MaxHP/CurrentHP 둘 다 복제되므로 클라이언트는 서버 값을 받기만 하면 된다.
	// 로드가 실패했으면 MaxHP가 0으로 남아 눈에 띄게 망가지는 쪽을 택했다(조용한 폴백 없음).
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
		{
			if (UPlayerStatManager* StatManager = GameInstance->GetSubsystem<UPlayerStatManager>())
			{
				const FPlayerBaseStat& Stat = StatManager->GetBaseStat();
				MaxHP = Stat.MaxHP;
				HPRegen = Stat.HPRegen;
				HPRegenDelay = Stat.HPRegenDelay;
			}
		}

		CurrentHP = MaxHP;
	}

	// 서버에서만 델리게이트를 등록한다. 클라이언트에서 등록하면 클라이언트 로컬에서
	// 체력이 깎이는 경로가 생겨 서버 권위 원칙이 깨진다.
	if (AActor* Owner = GetOwner())
	{
		if (Owner->HasAuthority())
		{
			Owner->OnTakeAnyDamage.AddDynamic(this, &UMainHPComponent::OnTakeAnyDamage);
		}
	}
}

void UMainHPComponent::OnEffectTick()
{
	if (IsDead() || CurrentHP >= MaxHP)
	{
		return;
	}

	if (GetWorld()->GetTimeSeconds() - LastDamageTimeSeconds < HPRegenDelay)
	{
		return;
	}

	CurrentHP = FMath::Min(CurrentHP + HPRegen, MaxHP);
}

void UMainHPComponent::OnTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.0f || IsDead())
	{
		return;
	}

	CurrentHP = FMath::Clamp(CurrentHP - Damage, 0.0f, MaxHP);
	LastDamageTimeSeconds = GetWorld()->GetTimeSeconds();

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

	UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] %s -> %s: %.1f damage, hp now %.1f"),
		*KillerName, *VictimName, Damage, CurrentHP);

	if (IsDead())
	{
		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] %s died (killed by %s)"), *VictimName, *KillerName);
		OnDeath.Broadcast(InstigatedBy);
	}
}

void UMainHPComponent::OnRep_CurrentHP()
{
	// TODO: 체력바 UI 갱신 등, 클라이언트 반응 로직이 필요해지면 여기에 추가
}

void UMainHPComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UMainHPComponent, CurrentHP);
	DOREPLIFETIME(UMainHPComponent, MaxHP);
}
