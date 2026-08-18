#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainGameMode.generated.h"

UCLASS()
class PROJECTRWW_API AMainGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMainGameMode();

protected:
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal = TEXT("")) override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// 접속/퇴장 이벤트가 있을 때마다 지금 인원수를 DB에 갱신한다.
	void UpdatePlayerCount();

	// -maxplayers= 커맨드라인 인자로 받는다. 안 주면 기본값(20)을 쓴다.
	int32 CurrentMaxPlayers = 20;

	UPROPERTY()
	TObjectPtr<class UMainSessionServerStatusRepository> StatusRepository;

	// -serveraddress= 인자로 받는 "이 서버의 주소". 안 주어지면 127.0.0.1:포트로 기본 동작(로컬 테스트용).
	FString MySessionServerAddress;
};
