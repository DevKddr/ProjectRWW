// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainSessionServerStatusRepository.h"
#include "SQLiteDatabase.h"
#include "SQLitePreparedStatement.h"

FString UMainSessionServerStatusRepository::GetDefaultDatabasePath()
{
	return TEXT("D:/ProjectRWW_Data/SessionServerStatus.db");
}

bool UMainSessionServerStatusRepository::Open(const FString& DatabaseFilePath)
{
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(DatabaseFilePath), true);

	Database = MakePimpl<FSQLiteDatabase>();
	if (!Database->Open(*DatabaseFilePath, ESQLiteDatabaseOpenMode::ReadWriteCreate))
	{
		UE_LOG(LogTemp, Error, TEXT("SessionServerStatus.db 열기 실패: %s"), *Database->GetLastError());
		return false;
	}

	return Database->Execute(TEXT(
		"CREATE TABLE IF NOT EXISTS SessionServerStatus ("
		"Address TEXT PRIMARY KEY,"
		"PlayerCount INTEGER NOT NULL DEFAULT 0,"
		"LastUpdateTime TEXT NOT NULL)"));
}

void UMainSessionServerStatusRepository::Close()
{
	if (Database)
	{
		Database->Close();
		Database.Reset();
	}
}

bool UMainSessionServerStatusRepository::ReportHeartbeat(const FString& Address, int32 PlayerCount)
{
	FSQLitePreparedStatement Statement = Database->PrepareStatement(
		TEXT("INSERT INTO SessionServerStatus (Address, PlayerCount, LastUpdateTime) VALUES (?1, ?2, ?3) ")
		TEXT("ON CONFLICT(Address) DO UPDATE SET PlayerCount=excluded.PlayerCount, LastUpdateTime=excluded.LastUpdateTime"));

	Statement.SetBindingValueByIndex(1, Address);
	Statement.SetBindingValueByIndex(2, PlayerCount);
	Statement.SetBindingValueByIndex(3, FDateTime::UtcNow().ToString());

	return Statement.Execute();
}

int32 UMainSessionServerStatusRepository::GetPlayerCount(const FString& Address)
{
	FSQLitePreparedStatement Statement = Database->PrepareStatement(
		TEXT("SELECT PlayerCount FROM SessionServerStatus WHERE Address = ?1"));
	Statement.SetBindingValueByIndex(1, Address);

	int32 PlayerCount = 0;
	Statement.Execute([&PlayerCount](const FSQLitePreparedStatement& InStatement)
	{
		InStatement.GetColumnValueByName(TEXT("PlayerCount"), PlayerCount);
		return ESQLitePreparedStatementExecuteRowResult::Stop;
	});

	return PlayerCount;
}
