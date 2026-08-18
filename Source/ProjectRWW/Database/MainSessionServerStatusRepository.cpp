// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainSessionServerStatusRepository.h"
#include "SQLiteDatabase.h"
#include "SQLitePreparedStatement.h"

FString UMainSessionServerStatusRepository::GetDefaultDatabasePath()
{
	return TEXT("D:/ProjectRWW_Data/SessionServerStatus.db");
}

bool UMainSessionServerStatusRepository::UpdatePlayerCount(const FString& Address, int32 PlayerCount, int32 MaxPlayers)
{
	const FString DatabaseFilePath = GetDefaultDatabasePath();
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(DatabaseFilePath), true);

	FSQLiteDatabase Database;
	if (!Database.Open(*DatabaseFilePath, ESQLiteDatabaseOpenMode::ReadWriteCreate))
	{
		UE_LOG(LogTemp, Error, TEXT("SessionServerStatus.db 열기 실패: %s"), *Database.GetLastError());
		return false;
	}

	Database.Execute(TEXT("PRAGMA journal_mode=WAL;"));
	Database.Execute(TEXT(
		"CREATE TABLE IF NOT EXISTS SessionServerStatus ("
		"Address TEXT PRIMARY KEY,"
		"PlayerCount INTEGER NOT NULL DEFAULT 0,"
		"MaxPlayers INTEGER NOT NULL DEFAULT 0,"
		"LastUpdateTime TEXT NOT NULL)"));

	bool bSuccess = false;
	{
		// Statement를 이 블록 안에서만 살아있게 해서, 아래 Database.Close()를 부르기 전에
		// 먼저 finalize되도록 한다. 안 그러면 Close()가 SQLITE_BUSY로 실패하고
		// Database 포인터가 안 지워져서, 나중에 소멸자가 크래시를 낸다.
		FSQLitePreparedStatement Statement = Database.PrepareStatement(
			TEXT("INSERT INTO SessionServerStatus (Address, PlayerCount, MaxPlayers, LastUpdateTime) VALUES (?1, ?2, ?3, ?4) ")
			TEXT("ON CONFLICT(Address) DO UPDATE SET PlayerCount=excluded.PlayerCount, MaxPlayers=excluded.MaxPlayers, LastUpdateTime=excluded.LastUpdateTime"));

		if (Statement.IsValid())
		{
			Statement.SetBindingValueByIndex(1, Address);
			Statement.SetBindingValueByIndex(2, PlayerCount);
			Statement.SetBindingValueByIndex(3, MaxPlayers);
			Statement.SetBindingValueByIndex(4, FDateTime::UtcNow().ToString());
			bSuccess = Statement.Execute();
		}
	}

	Database.Close();
	return bSuccess;
}

TArray<FMainSessionServerStatus> UMainSessionServerStatusRepository::GetAllServerStatuses()
{
	TArray<FMainSessionServerStatus> Results;

	const FString DatabaseFilePath = GetDefaultDatabasePath();
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(DatabaseFilePath), true);

	FSQLiteDatabase Database;
	if (!Database.Open(*DatabaseFilePath, ESQLiteDatabaseOpenMode::ReadWriteCreate))
	{
		UE_LOG(LogTemp, Error, TEXT("SessionServerStatus.db 열기 실패: %s"), *Database.GetLastError());
		return Results;
	}
	Database.Execute(TEXT("PRAGMA journal_mode=WAL;"));

	{
		// Statement가 이 블록 안에서 끝나야, 그 다음 Database.Close()가 성공한다.
		FSQLitePreparedStatement Statement = Database.PrepareStatement(
			TEXT("SELECT Address, PlayerCount, MaxPlayers FROM SessionServerStatus"));

		if (Statement.IsValid())
		{
			Statement.Execute([&Results](const FSQLitePreparedStatement& InStatement)
			{
				FMainSessionServerStatus Status;
				InStatement.GetColumnValueByName(TEXT("Address"), Status.Address);
				InStatement.GetColumnValueByName(TEXT("PlayerCount"), Status.PlayerCount);
				InStatement.GetColumnValueByName(TEXT("MaxPlayers"), Status.MaxPlayers);
				Results.Add(Status);
				return ESQLitePreparedStatementExecuteRowResult::Continue;
			});
		}
	}

	Database.Close();
	return Results;
}

bool UMainSessionServerStatusRepository::RemoveServer(const FString& Address)
{
	const FString DatabaseFilePath = GetDefaultDatabasePath();
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(DatabaseFilePath), true);

	FSQLiteDatabase Database;
	if (!Database.Open(*DatabaseFilePath, ESQLiteDatabaseOpenMode::ReadWriteCreate))
	{
		UE_LOG(LogTemp, Error, TEXT("SessionServerStatus.db 열기 실패: %s"), *Database.GetLastError());
		return false;
	}
	Database.Execute(TEXT("PRAGMA journal_mode=WAL;"));

	bool bSuccess = false;
	{
		FSQLitePreparedStatement Statement = Database.PrepareStatement(
			TEXT("DELETE FROM SessionServerStatus WHERE Address = ?1"));

		if (Statement.IsValid())
		{
			Statement.SetBindingValueByIndex(1, Address);
			bSuccess = Statement.Execute();
		}
	}

	Database.Close();
	return bSuccess;
}
