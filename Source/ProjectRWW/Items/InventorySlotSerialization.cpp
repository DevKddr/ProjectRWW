// Copyright Epic Games, Inc. All Rights Reserved.

#include "InventorySlotSerialization.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

FString SerializeInventorySlots(const TArray<FInventorySlot>& Slots)
{
	TArray<TSharedPtr<FJsonValue>> JsonArray;
	for (const FInventorySlot& Slot : Slots)
	{
		JsonArray.Add(MakeShared<FJsonValueString>(Slot.ItemIndex.ToString()));
	}

	FString Result;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Result);
	FJsonSerializer::Serialize(JsonArray, Writer);
	return Result;
}

void DeserializeInventorySlots(const FString& Json, TArray<FInventorySlot>& OutSlots)
{
	OutSlots.Reset();

	TArray<TSharedPtr<FJsonValue>> JsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, JsonArray))
	{
		return;
	}

	OutSlots.SetNum(JsonArray.Num());
	for (int32 i = 0; i < JsonArray.Num(); ++i)
	{
		const FString ItemStr = JsonArray[i]->AsString();
		OutSlots[i].ItemIndex = ItemStr.IsEmpty() ? NAME_None : FName(*ItemStr);
	}
}
