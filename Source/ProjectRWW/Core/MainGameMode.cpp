#include "MainGameMode.h"
#include "MainGameState.h"
#include "Player/MainPlayerController.h"
#include "Player/MainPlayerState.h"
#include "Player/MainCharacter.h"
#include "Database/MainSessionServerStatusRepository.h"

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

	CurrentMaxPlayers = MapMaxPlayers.Contains(MapName) ? MapMaxPlayers[MapName] : DefaultMaxPlayers;

	int32 Port = 0;
	FParse::Value(FCommandLine::Get(), TEXT("port="), Port);

	// -serveraddress=를 안 주면 127.0.0.1:포트로 기본 동작 (로컬 테스트 편의용).
	FString ServerAddress = FString::Printf(TEXT("127.0.0.1:%d"), Port);
	FParse::Value(FCommandLine::Get(), TEXT("serveraddress="), ServerAddress);
	MySessionServerAddress = ServerAddress;

	StatusRepository = NewObject<UMainSessionServerStatusRepository>(this);
	if (!StatusRepository->Open(UMainSessionServerStatusRepository::GetDefaultDatabasePath()))
	{
		UE_LOG(LogTemp, Error, TEXT("[ProjectRWW] SessionServerStatus.db 열기 실패"));
	}

	GetWorldTimerManager().SetTimer(HeartbeatTimerHandle, this, &AMainGameMode::ReportHeartbeat, 5.0f, true);
}

void AMainGameMode::ReportHeartbeat()
{
	if (StatusRepository && GameState)
	{
		StatusRepository->ReportHeartbeat(MySessionServerAddress, GameState->PlayerArray.Num());
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
}

void AMainGameMode::Logout(AController* Exiting)
{
	UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] Player left: %s"), *GetNameSafe(Exiting));

	Super::Logout(Exiting);
}

void AMainGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (StatusRepository)
	{
		StatusRepository->Close();
	}

	Super::EndPlay(EndPlayReason);
}
