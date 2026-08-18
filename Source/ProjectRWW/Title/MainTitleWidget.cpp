// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainTitleWidget.h"
#include "Core/MainNetworkSettings.h"
#include "GameFramework/PlayerController.h"

void UMainTitleWidget::OnConnectClicked(const FString& EnteredPlayerID)
{
	if (EnteredPlayerID.IsEmpty())
	{
		return;
	}

	if (APlayerController* PC = GetOwningPlayer())
	{
		const FString LobbyAddress = GetDefault<UMainNetworkSettings>()->LobbyAddress;

		// 엔진 예약 키인 "Name"은 OSS(Online Subsystem)가 자체 닉네임으로 덮어써서 무시된다.
		// 그래서 우리만의 커스텀 키 "PlayerID"로 전달하고, 서버 쪽에서 직접 읽어 이름을 설정한다.
		const FString Command = FString::Printf(TEXT("open %s?PlayerID=%s"), *LobbyAddress, *EnteredPlayerID);

		PC->ConsoleCommand(Command);
	}
}
