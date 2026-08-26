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
		"PlayerName TEXT NOT NULL DEFAULT '',"
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
			TEXT("SELECT PlayerName, KillCount, DeathCount, Currency, Items FROM PlayerData WHERE PlayerID = ?1"));
		Statement.SetBindingValueByIndex(1, PlayerID);

		bFound = Statement.Execute([&Record](const FSQLitePreparedStatement& InStatement)
		{
			InStatement.GetColumnValueByName(TEXT("PlayerName"), Record.PlayerName);
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
		Record.PlayerName = PlayerID; // 닉네임을 아직 안 정했으면 PlayerID를 임시 표시 이름으로.
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
		"PlayerName TEXT NOT NULL DEFAULT '',"
		"KillCount INTEGER NOT NULL DEFAULT 0,"
		"DeathCount INTEGER NOT NULL DEFAULT 0,"
		"Currency INTEGER NOT NULL DEFAULT 0,"
		"Items TEXT NOT NULL DEFAULT '[]')"));

	bool bSuccess = false;
	{
		FSQLitePreparedStatement Statement = Database.PrepareStatement(
			TEXT("INSERT INTO PlayerData (PlayerID, PlayerName, KillCount, DeathCount, Currency, Items) VALUES (?1, ?2, ?3, ?4, ?5, ?6) ")
			TEXT("ON CONFLICT(PlayerID) DO UPDATE SET PlayerName=excluded.PlayerName, KillCount=excluded.KillCount, DeathCount=excluded.DeathCount, Currency=excluded.Currency, Items=excluded.Items"));

		if (Statement.IsValid())
		{
			Statement.SetBindingValueByIndex(1, Record.PlayerID);
			Statement.SetBindingValueByIndex(2, Record.PlayerName);
			Statement.SetBindingValueByIndex(3, Record.KillCount);
			Statement.SetBindingValueByIndex(4, Record.DeathCount);
			Statement.SetBindingValueByIndex(5, Record.Currency);
			Statement.SetBindingValueByIndex(6, Record.ItemsJson);
			bSuccess = Statement.Execute();
		}
	}

	Database.Close();
	return bSuccess;
}
