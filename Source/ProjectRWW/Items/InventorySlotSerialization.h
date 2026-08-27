// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InventorySlot.h"

// TArray<FInventorySlot>을 위치 기반 JSON 문자열 배열로 직렬화한다.
// 슬롯 번호는 배열 인덱스로 표현되므로 별도 표기가 없다. 예: ["P_1","","AR_1"]
// 탄약(CurrentAmmo)은 포함하지 않는다 - 세션이 끝난 뒤엔 의미가 없으므로.
// 순수 변환 함수 - DB나 파일 접근 없음. 실제 저장은 호출부가 PlayerDataRepository로 처리한다.
PROJECTRWW_API FString SerializeInventorySlots(const TArray<FInventorySlot>& Slots);

// JSON 문자열을 TArray<FInventorySlot>으로 되돌린다. 파싱 실패 시 빈 배열을 반환한다.
PROJECTRWW_API void DeserializeInventorySlots(const FString& Json, TArray<FInventorySlot>& OutSlots);
