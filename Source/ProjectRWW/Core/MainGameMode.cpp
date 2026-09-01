#include "MainGameMode.h"
#include "MainGameState.h"
#include "Player/MainPlayerController.h"
#include "Player/MainPlayerState.h"
#include "Player/MainCharacter.h"
#include "Database/MainSessionServerStatusRepository.h"
#include "Database/MainPlayerDataRepository.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Items/MainInventoryComponent.h"
#include "Items/InventorySlotSerialization.h"
#include "Core/MainNetworkSettings.h"

AMainGameMode::AMainGameMode()
{
	GameStateClass = AMainGameState::StaticClass();
	PlayerControllerClass = AMainPlayerController::StaticClass();
	PlayerStateClass = AMainPlayerState::StaticClass();
	DefaultPawnClass = AMainCharacter::StaticClass();
}

void AMainGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	FParse::Value(FCommandLine::Get(), TEXT("maxplayers="), CurrentMaxPlayers);

	int32 Port = 0;
	FParse::Value(FCommandLine::Get(), TEXT("port="), Port);

	// -serveraddress=를 안 주면 127.0.0.1:포트로 기본 동작 (로컬 테스트 편의용).
	FString ServerAddress = FString::Printf(TEXT("127.0.0.1:%d"), Port);
	FParse::Value(FCommandLine::Get(), TEXT("serveraddress="), ServerAddress);
	MySessionServerAddress = ServerAddress;

	StatusRepository = NewObject<UMainSessionServerStatusRepository>(this);
	PlayerDataRepository = NewObject<UMainPlayerDataRepository>(this);

	// 시작하자마자 DB에 자기 존재를 등록해둔다 — 로비가 이 서버를 배정 후보로 인식하려면
	// 첫 플레이어가 들어오기 전부터 DB에 행이 있어야 하기 때문 (인원 0으로 등록).
	StatusRepository->UpdatePlayerCount(MySessionServerAddress, 0, CurrentMaxPlayers);
}

FString AMainGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
	// Super를 먼저 호출한다 — 내부적으로 Options의 "Name="을 파싱해서 ChangeName을 부르는데,
	// 이걸 우리보다 먼저 실행되게 해야 우리가 마지막에 덮어쓴 값이 살아남는다.
	// (로비 GameMode에서 겪었던 것과 동일한 이유)
	const FString ErrorMessage = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);

	// 로비에서 넘겨준 "PlayerID" 옵션을 읽어서, Super가 끝난 뒤 마지막으로 이름을 설정한다.
	const FString ParsedPlayerID = UGameplayStatics::ParseOption(Options, TEXT("PlayerID"));
	if (!ParsedPlayerID.IsEmpty())
	{
		ChangeName(NewPlayerController, ParsedPlayerID, false);
	}

	return ErrorMessage;
}

void AMainGameMode::UpdatePlayerCount()
{
	if (StatusRepository && GameState)
	{
		const bool bSuccess = StatusRepository->UpdatePlayerCount(MySessionServerAddress, GameState->PlayerArray.Num(), CurrentMaxPlayers);
		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] 인원수 갱신 %s - 주소 %s, 인원 %d/%d"),
			bSuccess ? TEXT("성공") : TEXT("실패"),
			*MySessionServerAddress,
			GameState->PlayerArray.Num(),
			CurrentMaxPlayers);
	}
}

void AMainGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	// 정원 초과 시 접속 거부 — 동적 서버 생성 도입 전까지 비활성화.
	// 다시 켜려면 아래 두 줄만 주석 해제하면 됨.
	// if (GameState && GameState->PlayerArray.Num() >= CurrentMaxPlayers)
	// {
	// 	ErrorMessage = TEXT("서버 정원이 가득 찼습니다.");
	// }
}

void AMainGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] Player joined: %s"), *GetNameSafe(NewPlayer));
	UpdatePlayerCount();

	// 로비에서 로드했던 걸 넘겨받는 게 아니라, 세션 서버가 같은 PlayerID로 독립적으로 다시 로드한다.
	AMainPlayerController* MainPC = Cast<AMainPlayerController>(NewPlayer);
	if (MainPC && PlayerDataRepository)
	{
		const FString PlayerID = NewPlayer->PlayerState ? NewPlayer->PlayerState->GetPlayerName() : FString();
		MainPC->PlayerRecord = PlayerDataRepository->LoadPlayerData(PlayerID);
		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] 세션 접속: %s - KillCount %d, DeathCount %d"), *PlayerID, MainPC->PlayerRecord.KillCount, MainPC->PlayerRecord.DeathCount);

		// OnPossess()는 이 로드보다 먼저 실행돼서 그때는 배열이 0칸이었을 것 - 여기서
		// 저장된 데이터를 실제로 채운 뒤 한 번 더 장착을 시도한다(리스폰 때는 이미
		// 채워져 있어 무해함).
		if (MainPC->InventoryComponent)
		{
			// 저장된 데이터를 배열로 되돌린다. 최초 접속(Inventory="[]")이면 빈 배열이 나온다.
			DeserializeInventorySlots(MainPC->PlayerRecord.Inventory, MainPC->InventoryComponent->Slots);

			// 방어적 처리: 저장된 배열 길이가 36과 다르면(최초 접속의 빈 배열 포함) 36칸으로
			// 맞춘다 - 부족한 칸은 기본값(빈 슬롯)으로 채워지고, 넘치는 칸은 잘린다.
			MainPC->InventoryComponent->Slots.SetNum(UMainInventoryComponent::InventorySlotCount);
			MainPC->InventoryComponent->EquipItem(0);
		}
	}
}

void AMainGameMode::HandlePlayerDeath(APlayerController* Victim, AController* Killer)
{
	AMainPlayerController* VictimController = Cast<AMainPlayerController>(Victim);
	if (!VictimController || VictimController->bIsDead || VictimController->bHasExtracted)
	{
		return;
	}

	VictimController->PlayerRecord.DeathCount += 1;

	// 사망 시 인벤토리 전부 손실(익스트랙션 실패 페널티). 월드 드랍은 별도 과제.
	if (VictimController->InventoryComponent)
	{
		VictimController->InventoryComponent->Slots.Init(FInventorySlot(), UMainInventoryComponent::InventorySlotCount);
		VictimController->PlayerRecord.Inventory = SerializeInventorySlots(VictimController->InventoryComponent->Slots);
	}

	// 존 안에서 죽었을 수도 있으니, 남아있는 탈출 타이머가 나중에 발동해도
	// (리스폰으로 bIsDead가 다시 false가 되더라도) 무해하도록 꺼둔다.
	VictimController->bIsExtracting = false;

	VictimController->bIsDead = true;

	// 가해자가 있고 자기 자신이 아닐 때만 킬 카운트 반영 (자살/자진 이탈 등은 제외)
	AMainPlayerController* KillerController = Cast<AMainPlayerController>(Killer);
	if (KillerController && KillerController != VictimController)
	{
		KillerController->PlayerRecord.KillCount += 1;
		KillerController->KillStreak += 1;
	}

	// TODO: 재화/아이템 획득 로직이 생기면, 여기서 저장하기 전에
	// 세션 중 획득분을 되돌리는 처리를 추가해야 함 (사망 시 손실 규칙)
	if (PlayerDataRepository)
	{
		const bool bSaveSuccess = PlayerDataRepository->SavePlayerData(VictimController->PlayerRecord);
		if (!bSaveSuccess)
		{
			UE_LOG(LogTemp, Error, TEXT("[ProjectRWW] 사망 정산 저장 실패: %s"), *VictimController->PlayerRecord.PlayerID);
		}
	}

	// 죽은 폰을 파괴한다 — 안 그러면 리스폰 시 RestartPlayer가 이미 폰을 가진
	// 컨트롤러를 만나게 되고, 기존 시체 액터도 맵에 영원히 남는다.
	if (APawn* VictimPawn = VictimController->GetPawn())
	{
		VictimController->UnPossess();
		VictimPawn->Destroy();
	}

	VictimController->Client_OnPlayerDied(VictimController->PlayerRecord, VictimController->KillStreak);
	VictimController->KillStreak = 0;
}

void AMainGameMode::HandleExtraction(APlayerController* Player)
{
	AMainPlayerController* MainPC = Cast<AMainPlayerController>(Player);
	if (!MainPC || MainPC->bIsDead || MainPC->bHasExtracted)
	{
		return;
	}

	MainPC->bHasExtracted = true;
	MainPC->bIsExtracting = false;

	// 사망과 달리 인벤토리를 그대로 유지한 채 직렬화해서 저장한다.
	if (MainPC->InventoryComponent)
	{
		MainPC->PlayerRecord.Inventory = SerializeInventorySlots(MainPC->InventoryComponent->Slots);
	}

	if (PlayerDataRepository)
	{
		const bool bSaveSuccess = PlayerDataRepository->SavePlayerData(MainPC->PlayerRecord);
		if (!bSaveSuccess)
		{
			UE_LOG(LogTemp, Error, TEXT("[ProjectRWW] 탈출 정산 저장 실패: %s"), *MainPC->PlayerRecord.PlayerID);
		}
	}

	// 지금은 UI 없이 바로 로비로 이동. 나중에 Client_OnPlayerExtracted RPC를
	// 추가하면 이 지점에서 같이 호출하면 된다.
	const FString LobbyAddress = GetDefault<UMainNetworkSettings>()->LobbyAddress;
	const FString TravelURL = FString::Printf(TEXT("%s?PlayerID=%s"), *LobbyAddress, *MainPC->PlayerRecord.PlayerID);
	MainPC->ClientTravel(TravelURL, ETravelType::TRAVEL_Absolute);

	// ClientTravel은 즉시 접속을 끊지 않으므로, 실제 이동이 지연되는 동안 폰이 서버에
	// 남아 데미지를 받으면 bHasExtracted 가드 때문에 사망 처리가 영구히 막혀버린다.
	// 폰을 여기서 바로 파괴해 그 가능성 자체를 없앤다(HandlePlayerDeath와 동일 패턴).
	if (APawn* Pawn = MainPC->GetPawn())
	{
		MainPC->UnPossess();
		Pawn->Destroy();
	}
}

void AMainGameMode::Logout(AController* Exiting)
{
	// 살아있는 채로 접속이 끊기면(비정상 종료 포함) 사망과 동일하게 정산 처리한다.
	// bIsDead가 이미 true면 HandlePlayerDeath 내부에서 조기 리턴되어 중복 저장 안 됨.
	if (AMainPlayerController* ExitingController = Cast<AMainPlayerController>(Exiting))
	{
		if (!ExitingController->bIsDead && !ExitingController->bHasExtracted)
		{
			HandlePlayerDeath(ExitingController, nullptr);
		}
	}

	Super::Logout(Exiting);

	UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] Player left: %s"), *GetNameSafe(Exiting));

	// Logout() 시점엔 Exiting이 아직 GameState->PlayerArray에서 안 빠진 상태라(PlayerState
	// 파괴 이후에나 빠짐), 공용 UpdatePlayerCount()를 그대로 쓰면 실제보다 1명 많게 집계된다.
	// 여기서만 예외적으로 하나 빼서 계산한다.
	if (StatusRepository && GameState)
	{
		const int32 PlayerCount = FMath::Max(GameState->PlayerArray.Num() - 1, 0);
		const bool bSuccess = StatusRepository->UpdatePlayerCount(MySessionServerAddress, PlayerCount, CurrentMaxPlayers);
		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] 인원수 갱신(퇴장) %s - 주소 %s, 인원 %d/%d"),
			bSuccess ? TEXT("성공") : TEXT("실패"), *MySessionServerAddress, PlayerCount, CurrentMaxPlayers);
	}
}

void AMainGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 서버가 정상적으로 꺼지는 경우, 로비가 죽은 서버로 플레이어를 계속 보내지 않도록
	// DB에서 자기 등록을 지운다. (크래시 같은 비정상 종료는 이걸로 못 잡음 — 별도 과제)
	if (StatusRepository)
	{
		StatusRepository->RemoveServer(MySessionServerAddress);
	}

	Super::EndPlay(EndPlayReason);
}
