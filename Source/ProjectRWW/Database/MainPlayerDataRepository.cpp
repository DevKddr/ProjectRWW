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
	Database.Execute(TEXT("PRAGMA journal_mode=WAL;"));

	Database.Execute(TEXT(
		"CREATE TABLE IF NOT EXISTS PlayerData ("
		"PlayerID TEXT PRIMARY KEY,"
		"PlayerName TEXT NOT NULL DEFAULT '',"
		"KillCount INTEGER NOT NULL DEFAULT 0,"
		"DeathCount INTEGER NOT NULL DEFAULT 0,"
		"Credit INTEGER NOT NULL DEFAULT 0,"
		"Items TEXT NOT NULL DEFAULT '[]',"
		"Storage TEXT NOT NULL DEFAULT '[]',"
		"StorageTier INTEGER NOT NULL DEFAULT 0)"));

	bool bFound = false;
	{
		// Statement를 이 블록 안에서만 살아있게 해서, 아래 Database.Close()를 부르기 전에
		// 먼저 정리(finalize)되도록 한다 — 안 그러면 살아있는 Statement 때문에 Close()가
		// 실제로는 못 닫고, 나중에 Database 소멸자가 크래시를 낸다.
		FSQLitePreparedStatement Statement = Database.PrepareStatement(
			TEXT("SELECT PlayerName, KillCount, DeathCount, Credit, Items, Storage, StorageTier FROM PlayerData WHERE PlayerID = ?1"));
		Statement.SetBindingValueByIndex(1, PlayerID);

		bFound = Statement.Execute([&Record](const FSQLitePreparedStatement& InStatement)
		{
			InStatement.GetColumnValueByName(TEXT("PlayerName"), Record.PlayerName);
			InStatement.GetColumnValueByName(TEXT("KillCount"), Record.KillCount);
			InStatement.GetColumnValueByName(TEXT("DeathCount"), Record.DeathCount);
			InStatement.GetColumnValueByName(TEXT("Credit"), Record.Credit);
			InStatement.GetColumnValueByName(TEXT("Items"), Record.Inventory);
			InStatement.GetColumnValueByName(TEXT("Storage"), Record.Storage);
			InStatement.GetColumnValueByName(TEXT("StorageTier"), Record.StorageTier);
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
	Database.Execute(TEXT("PRAGMA journal_mode=WAL;"));

	Database.Execute(TEXT(
		"CREATE TABLE IF NOT EXISTS PlayerData ("
		"PlayerID TEXT PRIMARY KEY,"
		"PlayerName TEXT NOT NULL DEFAULT '',"
		"KillCount INTEGER NOT NULL DEFAULT 0,"
		"DeathCount INTEGER NOT NULL DEFAULT 0,"
		"Credit INTEGER NOT NULL DEFAULT 0,"
		"Items TEXT NOT NULL DEFAULT '[]',"
		"Storage TEXT NOT NULL DEFAULT '[]',"
		"StorageTier INTEGER NOT NULL DEFAULT 0)"));

	bool bSuccess = false;
	{
		FSQLitePreparedStatement Statement = Database.PrepareStatement(
			TEXT("INSERT INTO PlayerData (PlayerID, PlayerName, KillCount, DeathCount, Credit, Items, Storage, StorageTier) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8) ")
			TEXT("ON CONFLICT(PlayerID) DO UPDATE SET PlayerName=excluded.PlayerName, KillCount=excluded.KillCount, DeathCount=excluded.DeathCount, Credit=excluded.Credit, Items=excluded.Items, Storage=excluded.Storage, StorageTier=excluded.StorageTier"));

		if (Statement.IsValid())
		{
			Statement.SetBindingValueByIndex(1, Record.PlayerID);
			Statement.SetBindingValueByIndex(2, Record.PlayerName);
			Statement.SetBindingValueByIndex(3, Record.KillCount);
			Statement.SetBindingValueByIndex(4, Record.DeathCount);
			Statement.SetBindingValueByIndex(5, Record.Credit);
			Statement.SetBindingValueByIndex(6, Record.Inventory);
			Statement.SetBindingValueByIndex(7, Record.Storage);
			Statement.SetBindingValueByIndex(8, Record.StorageTier);
			bSuccess = Statement.Execute();
		}
	}

	Database.Close();
	return bSuccess;
}

bool UMainPlayerDataRepository::AddCredit(const FString& PlayerID, int32 Amount)
{
	const FString DatabaseFilePath = GetDefaultDatabasePath();
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(DatabaseFilePath), true);

	FSQLiteDatabase Database;
	if (!Database.Open(*DatabaseFilePath, ESQLiteDatabaseOpenMode::ReadWriteCreate))
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerData.db 열기 실패: %s"), *Database.GetLastError());
		return false;
	}
	Database.Execute(TEXT("PRAGMA journal_mode=WAL;"));

	bool bSuccess = false;
	{
		// 읽고-더하고-쓰는 게 SQLite 안의 한 문장이라, 다른 프로세스가 동시에 같은 행을
		// 바꿔도 값이 덮어써지지 않고 합쳐진다. Statement는 Close() 전에 반드시 끝낸다.
		FSQLitePreparedStatement Statement = Database.PrepareStatement(
			TEXT("UPDATE PlayerData SET Credit = Credit + ?1 WHERE PlayerID = ?2"));
		if (Statement.IsValid())
		{
			Statement.SetBindingValueByIndex(1, Amount);
			Statement.SetBindingValueByIndex(2, PlayerID);
			bSuccess = Statement.Execute();
			if (!bSuccess)
			{
				// 실패 직후에 읽어야 오류 메시지가 남아 있다.
				UE_LOG(LogTemp, Warning, TEXT("[ProjectRWW] AddCredit 실행 실패: %s - %s"), *PlayerID, *Database.GetLastError());
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[ProjectRWW] AddCredit 쿼리 준비 실패: %s - %s"), *PlayerID, *Database.GetLastError());
		}
	}

	if (bSuccess)
	{
		// UPDATE는 대상 행이 없어도 성공이라, 실제로 바뀐 행 수를 같은 연결에서 확인한다.
		// changes()는 연결별 값이라 UPDATE를 한 바로 이 연결에서 물어봐야 한다.
		int32 ChangedRows = -1;
		{
			FSQLitePreparedStatement ChangesStatement = Database.PrepareStatement(TEXT("SELECT changes() AS ChangedRows"));
			if (ChangesStatement.IsValid())
			{
				ChangesStatement.Execute([&ChangedRows](const FSQLitePreparedStatement& InStatement)
				{
					InStatement.GetColumnValueByName(TEXT("ChangedRows"), ChangedRows);
					return ESQLitePreparedStatementExecuteRowResult::Stop;
				});
			}
		}

		// -1(질의 실패)은 행 없음으로 보지 않는다. 확실히 0일 때만 실패로 처리한다.
		if (ChangedRows == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ProjectRWW] AddCredit 대상 행 없음(정산 때 반영됨): %s"), *PlayerID);
			bSuccess = false;
		}
	}

	Database.Close();
	return bSuccess;
}
