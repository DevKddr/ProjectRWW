#include "MainGameMode.h"
#include "MainGameState.h"
#include "Player/MainPlayerController.h"
#include "Player/MainPlayerState.h"
#include "Player/MainCharacter.h"
#include "Database/MainSessionServerStatusRepository.h"
#include "Kismet/GameplayStatics.h"

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
}

void AMainGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] Player left: %s"), *GetNameSafe(Exiting));
	UpdatePlayerCount();
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
