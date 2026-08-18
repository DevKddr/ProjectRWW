// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainPlayerDataRepository.h"
#include "SQLiteDatabase.h"
#include "SQLitePreparedStatement.h"

FString UMainPlayerDataRepository::GetDefaultDatabasePath()
{
	return TEXT("D:/ProjectRWW_Data/PlayerData.db");
}

FMainPlayerRecord UMainPlayerDataRepository::LoadPlayerData(const FString& PlayerID)
{
	FMainPlayerRecord Record;
	Record.PlayerID = PlayerID;

	const FString DatabaseFilePath = GetDefaultDatabasePath();
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(DatabaseFilePath), true);

	FSQLiteDatabase Database;
	if (!Database.Open(*DatabaseFilePath, ESQLiteDatabaseOpenMode::ReadWriteCreate))
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerData.db 열기 실패: %s"), *Database.GetLastError());
		return Record;
	}

	Database.Execute(TEXT(
		"CREATE TABLE IF NOT EXISTS PlayerData ("
		"PlayerID TEXT PRIMARY KEY,"
		"KillCount INTEGER NOT NULL DEFAULT 0,"
		"DeathCount INTEGER NOT NULL DEFAULT 0,"
		"Currency INTEGER NOT NULL DEFAULT 0,"
		"Items TEXT NOT NULL DEFAULT '[]')"));

	bool bFound = false;
	{
		// Statement를 이 블록 안에서만 살아있게 해서, 아래 Database.Close()를 부르기 전에
		// 먼저 정리(finalize)되도록 한다 — 안 그러면 살아있는 Statement 때문에 Close()가
		// 실제로는 못 닫고, 나중에 Database 소멸자가 크래시를 낸다.
		FSQLitePreparedStatement Statement = Database.PrepareStatement(
			TEXT("SELECT KillCount, DeathCount, Currency, Items FROM PlayerData WHERE PlayerID = ?1"));
		Statement.SetBindingValueByIndex(1, PlayerID);

		bFound = Statement.Execute([&Record](const FSQLitePreparedStatement& InStatement)
		{
			InStatement.GetColumnValueByName(TEXT("KillCount"), Record.KillCount);
			InStatement.GetColumnValueByName(TEXT("DeathCount"), Record.DeathCount);
			InStatement.GetColumnValueByName(TEXT("Currency"), Record.Currency);
			InStatement.GetColumnValueByName(TEXT("Items"), Record.ItemsJson);
			return ESQLitePreparedStatementExecuteRowResult::Stop;
		}) > 0;
	}

	Database.Close();

	// 첫 접속(행이 없음): 기본값으로 새 레코드를 만들어 저장해둔다.
	// SavePlayerData가 자기 커넥션을 새로 열 것이므로, 위에서 이미 닫아둔 뒤에 호출한다.
	if (!bFound)
	{
		SavePlayerData(Record);
	}
	return Record;
}

bool UMainPlayerDataRepository::SavePlayerData(const FMainPlayerRecord& Record)
{
	const FString DatabaseFilePath = GetDefaultDatabasePath();
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(DatabaseFilePath), true);

	FSQLiteDatabase Database;
	if (!Database.Open(*DatabaseFilePath, ESQLiteDatabaseOpenMode::ReadWriteCreate))
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerData.db 열기 실패: %s"), *Database.GetLastError());
		return false;
	}

	Database.Execute(TEXT(
		"CREATE TABLE IF NOT EXISTS PlayerData ("
		"PlayerID TEXT PRIMARY KEY,"
		"KillCount INTEGER NOT NULL DEFAULT 0,"
		"DeathCount INTEGER NOT NULL DEFAULT 0,"
		"Currency INTEGER NOT NULL DEFAULT 0,"
		"Items TEXT NOT NULL DEFAULT '[]')"));

	bool bSuccess = false;
	{
		FSQLitePreparedStatement Statement = Database.PrepareStatement(
			TEXT("INSERT INTO PlayerData (PlayerID, KillCount, DeathCount, Currency, Items) VALUES (?1, ?2, ?3, ?4, ?5) ")
			TEXT("ON CONFLICT(PlayerID) DO UPDATE SET KillCount=excluded.KillCount, DeathCount=excluded.DeathCount, Currency=excluded.Currency, Items=excluded.Items"));

		if (Statement.IsValid())
		{
			Statement.SetBindingValueByIndex(1, Record.PlayerID);
			Statement.SetBindingValueByIndex(2, Record.KillCount);
			Statement.SetBindingValueByIndex(3, Record.DeathCount);
			Statement.SetBindingValueByIndex(4, Record.Currency);
			Statement.SetBindingValueByIndex(5, Record.ItemsJson);
			bSuccess = Statement.Execute();
		}
	}

	Database.Close();
	return bSuccess;
}
