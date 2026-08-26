// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainTitleWidget.h"
#include "Core/MainNetworkSettings.h"
#include "GameFramework/PlayerController.h"

namespace
{
	// PlayerID가 URL 옵션 문자열에 그대로 삽입되므로, "?"/"&"/공백처럼 URL 파서가
	// 특별하게 해석하는 문자를 걸러낸다. 영숫자/언더스코어/하이픈만 허용.
	bool IsValidPlayerID(const FString& Value)
	{
		if (Value.IsEmpty())
		{
			return false;
		}

		for (const TCHAR Char : Value)
		{
			const bool bIsAllowed = FChar::IsAlnum(Char) || Char == TEXT('_') || Char == TEXT('-');
			if (!bIsAllowed)
			{
				return false;
			}
		}
		return true;
	}
}

void UMainTitleWidget::OnConnectClicked(const FString& EnteredPlayerID)
{
	if (!IsValidPlayerID(EnteredPlayerID))
	{
		// TODO: 실패 사유를 UI에 표시(예: "영문/숫자/-/_만 사용 가능합니다")
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
