// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainLobbyPlayerController.h"
#include "MainLobbyGameMode.h"
#include "MainLobbyWidget.h"
#include "Database/MainPlayerDataRepository.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Items/MainInventoryComponent.h"
#include "Items/MainStorageComponent.h"
#include "Items/InventorySlotSerialization.h"

namespace
{
	// PlayerID와 달리 화면 표시용이라 공백/한글 등은 허용하되, 제어 문자와 과도한
	// 길이만 막는다. URL에는 안 실리므로 PlayerID만큼 엄격할 필요는 없다.
	bool IsValidPlayerName(const FString& Value)
	{
		if (Value.IsEmpty() || Value.Len() > 20)
		{
			return false;
		}

		for (const TCHAR Char : Value)
		{
			if (FChar::IsControl(Char))
			{
				return false;
			}
		}
		return true;
	}
}

AMainLobbyPlayerController::AMainLobbyPlayerController()
{
	InventoryComponent = CreateDefaultSubobject<UMainInventoryComponent>(TEXT("InventoryComponent"));
	StorageComponent = CreateDefaultSubobject<UMainStorageComponent>(TEXT("StorageComponent"));
}

void AMainLobbyPlayerController::SetPlayerRecord(const FMainPlayerRecord& Record)
{
	PlayerRecord = Record;

	if (InventoryComponent)
	{
		DeserializeInventorySlots(PlayerRecord.Inventory, InventoryComponent->Slots);
		InventoryComponent->Slots.SetNum(UMainInventoryComponent::InventorySlotCount);
	}

	if (StorageComponent)
	{
		DeserializeInventorySlots(PlayerRecord.Storage, StorageComponent->Slots);
		StorageComponent->ApplyTier(PlayerRecord.StorageTier);
	}
}

void AMainLobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 로비 UI는 소유 클라이언트에서만 띄운다 (MainPlayerController의 HUD 패턴과 동일).
	if (IsLocalController() && LobbyWidgetClass)
	{
		LobbyWidgetInstance = CreateWidget<UMainLobbyWidget>(this, LobbyWidgetClass);
		if (LobbyWidgetInstance)
		{
			LobbyWidgetInstance->AddToViewport();
			SetInputMode(FInputModeGameAndUI());
			SetShowMouseCursor(true);
		}
	}
}

void AMainLobbyPlayerController::RWW_SearchSession()
{
	Server_SearchSession();
}

void AMainLobbyPlayerController::RWW_AddItem(const FString& ItemIndex)
{
	if (InventoryComponent)
	{
		// AddItem()을 직접 부르지 않고 RPC를 거친다 - 클라이언트에서 이 명령어를
		// 입력해도 항상 서버의 진짜 데이터에 반영되게 하기 위함.
		InventoryComponent->Server_AddItem(FName(*ItemIndex));
	}
}

void AMainLobbyPlayerController::Server_SearchSession_Implementation()
{
	AMainLobbyGameMode* LobbyGameMode = GetWorld()->GetAuthGameMode<AMainLobbyGameMode>();
	const FString Address = LobbyGameMode ? LobbyGameMode->AssignSessionServer() : FString();
	if (!Address.IsEmpty())
	{
		SavePlayerRecord();

		// PlayerState->GetPlayerName()이 아니라 PlayerRecord.PlayerID를 직접 쓴다.
		// Server_SetPlayerName으로 닉네임이 바뀌어도 이 값은 영향받지 않아야 하기 때문.
		const FString TravelURL = FString::Printf(TEXT("%s?PlayerID=%s"), *Address, *PlayerRecord.PlayerID);
		ClientTravel(TravelURL, ETravelType::TRAVEL_Absolute);
	}
}

void AMainLobbyPlayerController::Server_SetPlayerName_Implementation(const FString& NewName)
{
	if (!IsValidPlayerName(NewName))
	{
		return;
	}

	// PlayerState의 표시 이름은 절대 안 건드린다 — 그건 PlayerID를 담는 통로로 고정해두고,
	// 닉네임은 PlayerRecord.PlayerName 쪽에서만 관리한다.
	PlayerRecord.PlayerName = NewName;
	SavePlayerRecord();
}

void AMainLobbyPlayerController::Server_RequestSessionList_Implementation()
{
	if (AMainLobbyGameMode* LobbyGameMode = GetWorld()->GetAuthGameMode<AMainLobbyGameMode>())
	{
		if (UMainSessionServerStatusRepository* Repository = LobbyGameMode->GetStatusRepository())
		{
			Client_ReceiveSessionList(Repository->GetAllServerStatuses());
		}
	}
}

void AMainLobbyPlayerController::Client_ReceiveSessionList_Implementation(const TArray<FMainSessionServerStatus>& Servers)
{
	CachedSessionList = Servers;
	OnSessionListUpdated.Broadcast();
}

void AMainLobbyPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AMainLobbyPlayerController, PlayerRecord, COND_OwnerOnly);
}

void AMainLobbyPlayerController::OnRep_PlayerRecord()
{
	OnPlayerRecordUpdated.Broadcast();
}

void AMainLobbyPlayerController::SavePlayerRecord()
{
	if (InventoryComponent)
	{
		PlayerRecord.Inventory = SerializeInventorySlots(InventoryComponent->Slots);
	}

	if (StorageComponent)
	{
		PlayerRecord.Storage = SerializeInventorySlots(StorageComponent->Slots);
		PlayerRecord.StorageTier = StorageComponent->StorageTier;
	}

	if (AMainLobbyGameMode* LobbyGameMode = GetWorld()->GetAuthGameMode<AMainLobbyGameMode>())
	{
		if (UMainPlayerDataRepository* Repository = LobbyGameMode->GetPlayerDataRepository())
		{
			if (!Repository->SavePlayerData(PlayerRecord))
			{
				UE_LOG(LogTemp, Error, TEXT("[ProjectRWW] 로비 데이터 저장 실패: %s"), *PlayerRecord.PlayerID);
			}
		}
	}
}

void AMainLobbyPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		SavePlayerRecord();
	}

	Super::EndPlay(EndPlayReason);
}
