// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MainNetworkSettings.generated.h"

// 로비 주소처럼 "코드 재컴파일 없이 바꾸고 싶은 값"을 모아두는 설정 클래스.
// Project Settings 메뉴에도 자동으로 노출되어 에디터에서 편집 가능.
// 세션 서버 목록은 더 이상 여기 하드코딩하지 않는다 — 각 세션 서버가 시작할 때
// SessionServerStatus.db에 스스로 등록하고, 로비는 그 DB를 조회해서 목록을 얻는다.
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "ProjectRWW Network Settings"))
class PROJECTRWW_API UMainNetworkSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// 코드 기본값은 누구나 안전하게 쓸 수 있는 로컬 주소로 둔다.
	// 개인 실제 테스트용 IP는 이 값을 직접 고치지 말고, Saved/Config(git 무시됨)에서 개인적으로 덮어쓴다.
	UPROPERTY(Config, EditAnywhere, Category = "Connection")
	FString LobbyAddress = TEXT("127.0.0.1:7777");
};
