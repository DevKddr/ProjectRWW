// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainPlayerDataRepository.h"
#include "SQLiteDatabase.h"
#include "SQLitePreparedStatement.h"

FString UMainPlayerDataRepository::GetDefaultDatabasePath()
{
	return TEXT("D:/ProjectRWW_Data/PlayerData.db");
}

bool UMainPlayerDataRepository::Open(const FString& DatabaseFilePath)
{
	// SQLite는 파일은 알아서 만들어주지만 폴더까지는 안 만들어주므로 미리 생성해둔다.
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(DatabaseFilePath), true);

	Database = MakePimpl<FSQLiteDatabase>();
	if (!Database->Open(*DatabaseFilePath, ESQLiteDatabaseOpenMode::ReadWriteCreate))
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerData.db 열기 실패: %s"), *Database->GetLastError());
		return false;
	}

	return Database->Execute(TEXT(
		"CREATE TABLE IF NOT EXISTS PlayerData ("
		"PlayerID TEXT PRIMARY KEY,"
		"KillCount INTEGER NOT NULL DEFAULT 0,"
		"DeathCount INTEGER NOT NULL DEFAULT 0,"
		"Currency INTEGER NOT NULL DEFAULT 0,"
		"Items TEXT NOT NULL DEFAULT '[]')"));
}

void UMainPlayerDataRepository::Close()
{
	if (Database)
	{
		Database->Close();
		Database.Reset();
	}
}

FMainPlayerRecord UMainPlayerDataRepository::LoadPlayerData(const FString& PlayerID)
{
	FMainPlayerRecord Record;
	Record.PlayerID = PlayerID;

	FSQLitePreparedStatement Statement = Database->PrepareStatement(
		TEXT("SELECT KillCount, DeathCount, Currency, Items FROM PlayerData WHERE PlayerID = ?1"));
	Statement.SetBindingValueByIndex(1, PlayerID);

	const bool bFound = Statement.Execute([&Record](const FSQLitePreparedStatement& InStatement)
	{
		InStatement.GetColumnValueByName(TEXT("KillCount"), Record.KillCount);
		InStatement.GetColumnValueByName(TEXT("DeathCount"), Record.DeathCount);
		InStatement.GetColumnValueByName(TEXT("Currency"), Record.Currency);
		InStatement.GetColumnValueByName(TEXT("Items"), Record.ItemsJson);
		return ESQLitePreparedStatementExecuteRowResult::Stop;
	}) > 0;

	// 첫 접속(행이 없음): 기본값으로 새 레코드를 만들어 저장해둔다.
	if (!bFound)
	{
		SavePlayerData(Record);
	}
	return Record;
}

bool UMainPlayerDataRepository::SavePlayerData(const FMainPlayerRecord& Record)
{
	FSQLitePreparedStatement Statement = Database->PrepareStatement(
		TEXT("INSERT INTO PlayerData (PlayerID, KillCount, DeathCount, Currency, Items) VALUES (?1, ?2, ?3, ?4, ?5) ")
		TEXT("ON CONFLICT(PlayerID) DO UPDATE SET KillCount=excluded.KillCount, DeathCount=excluded.DeathCount, Currency=excluded.Currency, Items=excluded.Items"));

	Statement.SetBindingValueByIndex(1, Record.PlayerID);
	Statement.SetBindingValueByIndex(2, Record.KillCount);
	Statement.SetBindingValueByIndex(3, Record.DeathCount);
	Statement.SetBindingValueByIndex(4, Record.Currency);
	Statement.SetBindingValueByIndex(5, Record.ItemsJson);

	return Statement.Execute();
}
