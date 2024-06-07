// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "DebugLog.h"
#include "StringUtils.h"
#include "Utils.h"
#include "AccountHelpers.h"
#include "Game.h"
#include "GameEvent.h"
#include "Main.h"
#include "Platform.h"
#include "Users.h"
#include "Player.h"
#include "Leaderboard.h"
#include <eos_leaderboards.h>
#include <eos_stats.h>

FLeaderboard::FLeaderboard()
{
}

FLeaderboard::~FLeaderboard()
{

}

void FLeaderboard::Init()
{
	if (FPlatform::GetPlatformHandle() != nullptr)
	{
		LeaderboardsHandle = EOS_Platform_GetLeaderboardsInterface(FPlatform::GetPlatformHandle());

		StatsHandle = EOS_Platform_GetStatsInterface(FPlatform::GetPlatformHandle());
	}
}


void FLeaderboard::Update()
{

}

void FLeaderboard::OnLoggedIn(FProductUserId UserId)
{
	QueryDefinitions();
}

void FLeaderboard::OnLoggedOut(FProductUserId UserId)
{
	if (FPlayerManager::Get().GetNumPlayers() == 0)
	{
		FGameEvent Event(EGameEventType::NoUserLoggedIn);
		FGame::Get().OnGameEvent(Event);
	}

	CachedLeaderboardsDefinitionData.clear();
	CachedLeaderboardsRecordData.clear();
	CachedLeaderboardsUserScoresData.clear();
}

void FLeaderboard::OnGameEvent(const FGameEvent& Event)
{
	if (Event.GetType() == EGameEventType::UserConnectLoggedIn)
	{
		FProductUserId UserId = Event.GetProductUserId();
		OnLoggedIn(UserId);
	}
	else if (Event.GetType() == EGameEventType::UserConnectAuthExpiration)
	{
		FProductUserId UserId = Event.GetProductUserId();
		OnLoggedOut(UserId);
	}
}

void FLeaderboard::QueryDefinitions()
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player == nullptr)
	{
		FDebugLog::LogError(L"Leaderboard - QueryDefinitions: Current player is invalid!");
		return;
	}

	EOS_Leaderboards_QueryLeaderboardDefinitionsOptions QueryDefinitionsOptions = { 0 };
	QueryDefinitionsOptions.ApiVersion = EOS_LEADERBOARDS_QUERYLEADERBOARDDEFINITIONS_API_LATEST;
	QueryDefinitionsOptions.StartTime = EOS_LEADERBOARDS_TIME_UNDEFINED;
	QueryDefinitionsOptions.EndTime = EOS_LEADERBOARDS_TIME_UNDEFINED;
	QueryDefinitionsOptions.LocalUserId = Player->GetProductUserID();

	EOS_Leaderboards_QueryLeaderboardDefinitions(LeaderboardsHandle, &QueryDefinitionsOptions, nullptr, LeaderboardDefinitionsReceivedCallbackFn);
}

void FLeaderboard::CacheLeaderboardDefinitions()
{
	EOS_HLeaderboards LeaderboardsHandle = EOS_Platform_GetLeaderboardsInterface(FPlatform::GetPlatformHandle());

	EOS_Leaderboards_GetLeaderboardDefinitionCountOptions LeaderboardDefinitionsCountOptions = { 0 };
	LeaderboardDefinitionsCountOptions.ApiVersion = EOS_LEADERBOARDS_GETLEADERBOARDDEFINITIONCOUNT_API_LATEST;

	uint32_t LeaderboardDefinitionsCount = EOS_Leaderboards_GetLeaderboardDefinitionCount(LeaderboardsHandle, &LeaderboardDefinitionsCountOptions);

	EOS_Leaderboards_CopyLeaderboardDefinitionByIndexOptions CopyOptions = { 0 };
	CopyOptions.ApiVersion = EOS_LEADERBOARDS_COPYLEADERBOARDDEFINITIONBYINDEX_API_LATEST;

	// Clear definitions
	CachedLeaderboardsDefinitionData.clear();

	for (CopyOptions.LeaderboardIndex = 0; CopyOptions.LeaderboardIndex < LeaderboardDefinitionsCount; ++CopyOptions.LeaderboardIndex)
	{
		EOS_Leaderboards_Definition* LeaderboardDef = NULL;
		EOS_EResult CopyLeaderboardDefinitionsResult = EOS_Leaderboards_CopyLeaderboardDefinitionByIndex(LeaderboardsHandle, &CopyOptions, &LeaderboardDef);
		if (CopyLeaderboardDefinitionsResult != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"CopyLeaderboardDefinitions Failure!");
			break;
		}

		std::shared_ptr<FLeaderboardsDefinitionData> LeaderboardsDefinition = std::make_shared<FLeaderboardsDefinitionData>();

		if (LeaderboardDef->LeaderboardId)
		{
			LeaderboardsDefinition->LeaderboardId = FStringUtils::Widen(LeaderboardDef->LeaderboardId);
		}

		if (LeaderboardDef->StatName)
		{
			LeaderboardsDefinition->StatName = FStringUtils::Widen(LeaderboardDef->StatName);
		}

		LeaderboardsDefinition->StartTime = LeaderboardDef->StartTime;

		LeaderboardsDefinition->EndTime = LeaderboardDef->EndTime;

		switch (LeaderboardDef->Aggregation)
		{
			case EOS_ELeaderboardAggregation::EOS_LA_Min:
				LeaderboardsDefinition->Aggregation = ELeaderboardAggregation::Min;
				break;
			case EOS_ELeaderboardAggregation::EOS_LA_Max:
				LeaderboardsDefinition->Aggregation = ELeaderboardAggregation::Max;
				break;
			case EOS_ELeaderboardAggregation::EOS_LA_Sum:
				LeaderboardsDefinition->Aggregation = ELeaderboardAggregation::Sum;
				break;
			case EOS_ELeaderboardAggregation::EOS_LA_Latest:
				LeaderboardsDefinition->Aggregation = ELeaderboardAggregation::Latest;
				break;
		}

		CachedLeaderboardsDefinitionData.emplace_back(LeaderboardsDefinition);

		// Release Leaderboard Definition
		EOS_Leaderboards_Definition_Release(LeaderboardDef);
	}

	PrintLeaderboardDefinitions();

	FGameEvent Event(EGameEventType::DefinitionsReceived);
	FGame::Get().OnGameEvent(Event);
}

void FLeaderboard::PrintLeaderboardDefinitions()
{
	// Print info
	FDebugLog::Log(L"%d Leaderboards:", CachedLeaderboardsDefinitionData.size());

	for (uint32_t LeaderboardIndex = 0; LeaderboardIndex < CachedLeaderboardsDefinitionData.size(); ++LeaderboardIndex)
	{
		FDebugLog::Log(L"-------------");
		FDebugLog::Log(L"Leaderboard: %d", LeaderboardIndex + 1);

		std::shared_ptr<FLeaderboardsDefinitionData> LeaderboardDef = CachedLeaderboardsDefinitionData[LeaderboardIndex];

		if (!LeaderboardDef->LeaderboardId.empty())
		{
			FDebugLog::Log(L"Leaderboard ID: %ls", LeaderboardDef->LeaderboardId.c_str());
		}

		if (!LeaderboardDef->StatName.empty())
		{
			FDebugLog::Log(L"Stat Name: %ls", LeaderboardDef->StatName.c_str());
		}

		if (LeaderboardDef->StartTime == EOS_LEADERBOARDS_TIME_UNDEFINED)
		{
			FDebugLog::Log(L"Start Time: Undefined");
		}
		else
		{
			std::wstring DateTimeStrW = FUtils::ConvertUnixTimestampToUTCString(LeaderboardDef->StartTime);
			FDebugLog::Log(L"Start Time: %ls", DateTimeStrW.c_str());
		}

		if (LeaderboardDef->EndTime == EOS_LEADERBOARDS_TIME_UNDEFINED)
		{
			FDebugLog::Log(L"End Time: Undefined");
		}
		else
		{
			std::wstring DateTimeStrW = FUtils::ConvertUnixTimestampToUTCString(LeaderboardDef->EndTime);
			FDebugLog::Log(L"End Time: %ls", DateTimeStrW.c_str());
		}

		switch (LeaderboardDef->Aggregation)
		{
			case ELeaderboardAggregation::Min:
				FDebugLog::Log(L"Aggregation: Minimum");
				break;
			case ELeaderboardAggregation::Max:
				FDebugLog::Log(L"Aggregation: Maximum");
				break;
			case ELeaderboardAggregation::Sum:
				FDebugLog::Log(L"Aggregation: Sum");
				break;
			case ELeaderboardAggregation::Latest:
				FDebugLog::Log(L"Aggregation: Latest");
				break;
		}

		FDebugLog::Log(L"-------------");
	}
}

std::vector<std::shared_ptr<FLeaderboardsDefinitionData>>& FLeaderboard::GetCachedDefinitions()
{
	return CachedLeaderboardsDefinitionData;
}

std::vector<std::wstring> FLeaderboard::GetDefinitionIds()
{
	std::vector<std::wstring> DefinitionIds;

	for (std::shared_ptr<FLeaderboardsDefinitionData>& NextDefinition : CachedLeaderboardsDefinitionData)
	{
		if (!NextDefinition->LeaderboardId.empty())
		{
			DefinitionIds.emplace_back(NextDefinition->LeaderboardId);
		}
	}

	return DefinitionIds;
}

bool FLeaderboard::GetDefinitionFromId(std::wstring LeaderboardId, std::shared_ptr<FLeaderboardsDefinitionData>& OutDef)
{
	for (std::shared_ptr<FLeaderboardsDefinitionData> NextDefinition : CachedLeaderboardsDefinitionData)
	{
		if (NextDefinition->LeaderboardId == LeaderboardId)
		{
			OutDef = NextDefinition;
			return true;
		}
	}

	return false;
}

bool FLeaderboard::GetDefinitionFromIndex(int InIndex, std::shared_ptr<FLeaderboardsDefinitionData>& OutDef)
{
	int Index = 0;
	for (std::shared_ptr<FLeaderboardsDefinitionData> NextDefinition : CachedLeaderboardsDefinitionData)
	{
		if (Index == InIndex)
		{
			OutDef = NextDefinition;
			return true;
		}
		++Index;
	}

	return false;
}

void EOS_CALL FLeaderboard::LeaderboardDefinitionsReceivedCallbackFn(const EOS_Leaderboards_OnQueryLeaderboardDefinitionsCompleteCallbackInfo* Data)
{
	assert(Data != NULL);

	if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
	{
		// Operation is retrying so it is not complete yet
		return;
	}

	if (Data->ResultCode != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"[EOS SDK] Leaderboards - Query Definitions Error: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		return;
	}

	FDebugLog::Log(L"[EOS SDK] Leaderboards - Query Definitions Complete");

	FGame::Get().GetLeaderboard()->CacheLeaderboardDefinitions();
}

void FLeaderboard::QueryRanks(std::wstring LeaderboardId)
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player == nullptr)
	{
		FDebugLog::LogError(L"Leaderboard - QueryRanks: Current player is invalid!");
		return;
	}

	std::string LeaderboardIdStr = FStringUtils::Narrow(LeaderboardId);

	EOS_Leaderboards_QueryLeaderboardRanksOptions QueryRanksOptions = { 0 };
	QueryRanksOptions.ApiVersion = EOS_LEADERBOARDS_QUERYLEADERBOARDRANKS_API_LATEST;
	QueryRanksOptions.LeaderboardId = LeaderboardIdStr.c_str();
	QueryRanksOptions.LocalUserId = Player->GetProductUserID();

	EOS_Leaderboards_QueryLeaderboardRanks(LeaderboardsHandle, &QueryRanksOptions, nullptr, LeaderboardRanksReceivedCallbackFn);
}

void FLeaderboard::CacheLeaderboardRecords()
{
	EOS_HLeaderboards LeaderboardsHandle = EOS_Platform_GetLeaderboardsInterface(FPlatform::GetPlatformHandle());

	EOS_Leaderboards_GetLeaderboardRecordCountOptions LeaderboardsRecordsCountOptions = { 0 };
	LeaderboardsRecordsCountOptions.ApiVersion = EOS_LEADERBOARDS_GETLEADERBOARDRECORDCOUNT_API_LATEST;

	uint32_t LeaderboardRecordsCount = EOS_Leaderboards_GetLeaderboardRecordCount(LeaderboardsHandle, &LeaderboardsRecordsCountOptions);

	EOS_Leaderboards_CopyLeaderboardRecordByIndexOptions CopyOptions = { 0 };
	CopyOptions.ApiVersion = EOS_LEADERBOARDS_COPYLEADERBOARDRECORDBYINDEX_API_LATEST;

	// Clear records
	CachedLeaderboardsRecordData.clear();

	for (CopyOptions.LeaderboardRecordIndex = 0; CopyOptions.LeaderboardRecordIndex < LeaderboardRecordsCount; ++CopyOptions.LeaderboardRecordIndex)
	{
		EOS_Leaderboards_LeaderboardRecord* LeaderboardRecord = NULL;
		EOS_EResult CopyLeaderboardRecordResult = EOS_Leaderboards_CopyLeaderboardRecordByIndex(LeaderboardsHandle, &CopyOptions, &LeaderboardRecord);
		if (CopyLeaderboardRecordResult != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"CopyLeaderboardRecords Failure!");
			break;
		}

		std::shared_ptr<FLeaderboardsRecordData> LeaderboardRecordData = std::make_shared<FLeaderboardsRecordData>();

		LeaderboardRecordData->UserId = LeaderboardRecord->UserId;

		LeaderboardRecordData->Rank = LeaderboardRecord->Rank;

		LeaderboardRecordData->Score = LeaderboardRecord->Score;

		LeaderboardRecordData->DisplayName = FStringUtils::Widen(LeaderboardRecord->UserDisplayName);

		CachedLeaderboardsRecordData.emplace_back(LeaderboardRecordData);

		// Release Leaderboard Record
		EOS_Leaderboards_LeaderboardRecord_Release(LeaderboardRecord);
	}

	PrintLeaderboardRecords();

	FGameEvent Event(EGameEventType::LeaderboardRecordsReceived);
	FGame::Get().OnGameEvent(Event);
}

void FLeaderboard::PrintLeaderboardRecords()
{
	// Print info
	FDebugLog::Log(L"%d Leaderboard Records:", CachedLeaderboardsRecordData.size());

	for (uint32_t LeaderboardRecordIndex = 0; LeaderboardRecordIndex < CachedLeaderboardsRecordData.size(); ++LeaderboardRecordIndex)
	{
		std::shared_ptr<FLeaderboardsRecordData> LeaderboardRecord = CachedLeaderboardsRecordData[LeaderboardRecordIndex];

		FDebugLog::Log(L"[%d] User ID: %ls, User Name: %ls, Rank: %d, Score: %d",
			LeaderboardRecordIndex + 1,
			FStringUtils::Widen(FAccountHelpers::ProductUserIDToString(LeaderboardRecord->UserId)).c_str(),
			LeaderboardRecord->DisplayName.c_str(),
			LeaderboardRecord->Rank,
			LeaderboardRecord->Score);
	}
}

std::vector<std::shared_ptr<FLeaderboardsRecordData>>& FLeaderboard::GetCachedRecords()
{
	return CachedLeaderboardsRecordData;
}

void EOS_CALL FLeaderboard::LeaderboardRanksReceivedCallbackFn(const EOS_Leaderboards_OnQueryLeaderboardRanksCompleteCallbackInfo* Data)
{
	assert(Data != NULL);

	if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
	{
		// Operation is retrying so it is not complete yet
		return;
	}

	if (Data->ResultCode != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"[EOS SDK] Leaderboards - Query Ranks Error: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		return;
	}

	FDebugLog::Log(L"[EOS SDK] Leaderboards - Query Ranks Complete, LeaderboardId=[%ls]", FStringUtils::Widen(Data->LeaderboardId).c_str());

	FGame::Get().GetLeaderboard()->CacheLeaderboardRecords();
}

void FLeaderboard::QueryUserScores(std::vector<std::wstring> LeaderboardIds, std::vector<EOS_ProductUserId>& UserIds)
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player == nullptr)
	{
		FDebugLog::LogError(L"Leaderboard - QueryUserScores: Current player is invalid!");
		return;
	}

	std::vector<std::wstring> StatNames;
	std::vector<EOS_ELeaderboardAggregation> StatAggregation;
	for (std::wstring LeaderboardId : LeaderboardIds)
	{
		std::shared_ptr<FLeaderboardsDefinitionData> DefData;
		if (GetDefinitionFromId(LeaderboardId, DefData))
		{
			StatNames.emplace_back(DefData->StatName);

			EOS_ELeaderboardAggregation Aggregation;
			switch (DefData->Aggregation)
			{
				case ELeaderboardAggregation::Min:
					Aggregation = EOS_ELeaderboardAggregation::EOS_LA_Min;
					break;
				case ELeaderboardAggregation::Max:
					Aggregation = EOS_ELeaderboardAggregation::EOS_LA_Max;
					break;
				case ELeaderboardAggregation::Sum:
					Aggregation = EOS_ELeaderboardAggregation::EOS_LA_Sum;
					break;
				case ELeaderboardAggregation::Latest:
					Aggregation = EOS_ELeaderboardAggregation::EOS_LA_Latest;
					break;
			}
			StatAggregation.emplace_back(Aggregation);
		}
	}

	// Query User Scores
	EOS_Leaderboards_QueryLeaderboardUserScoresOptions QueryUserScoresOptions = { 0 };
	QueryUserScoresOptions.ApiVersion = EOS_LEADERBOARDS_QUERYLEADERBOARDUSERSCORES_API_LATEST;
	QueryUserScoresOptions.UserIdsCount = (uint32_t)UserIds.size();
	EOS_ProductUserId* UserData = new EOS_ProductUserId[QueryUserScoresOptions.UserIdsCount];
	for (uint32_t UserIndex = 0; UserIndex < UserIds.size(); ++UserIndex)
	{
		UserData[UserIndex] = UserIds[UserIndex];
	}
	QueryUserScoresOptions.UserIds = UserData;
	QueryUserScoresOptions.StatInfoCount = (uint32_t)StatNames.size();
	EOS_Leaderboards_UserScoresQueryStatInfo* StatInfoData = new EOS_Leaderboards_UserScoresQueryStatInfo[QueryUserScoresOptions.StatInfoCount];
	std::vector<std::string> NarrowStatNames;
	for (std::wstring StatName : StatNames)
	{
		NarrowStatNames.emplace_back(FStringUtils::Narrow(StatName));
	}
	for (uint32_t StatIndex = 0; StatIndex < StatNames.size(); ++StatIndex)
	{
		StatInfoData[StatIndex].StatName = NarrowStatNames[StatIndex].c_str();
		StatInfoData[StatIndex].Aggregation = StatAggregation[StatIndex];
	}
	QueryUserScoresOptions.StatInfo = StatInfoData;
	QueryUserScoresOptions.StartTime = EOS_LEADERBOARDS_TIME_UNDEFINED;
	QueryUserScoresOptions.EndTime = EOS_LEADERBOARDS_TIME_UNDEFINED;
	QueryUserScoresOptions.LocalUserId = Player->GetProductUserID();

	EOS_Leaderboards_QueryLeaderboardUserScores(LeaderboardsHandle, &QueryUserScoresOptions, nullptr, LeaderboardUserScoresReceivedCallbackFn);

	delete[] UserData;
	delete[] StatInfoData;
}

void FLeaderboard::CacheLeaderboardUserScores()
{
	CachedLeaderboardsUserScoresData.clear();

	EOS_HLeaderboards LeaderboardsHandle = EOS_Platform_GetLeaderboardsInterface(FPlatform::GetPlatformHandle());

	for (uint32_t LeaderboardIndex = 0; LeaderboardIndex < CachedLeaderboardsDefinitionData.size(); ++LeaderboardIndex)
	{
		std::vector<std::shared_ptr<FLeaderboardsUserScoreData>> CachedUserScores;

		std::shared_ptr<FLeaderboardsDefinitionData> LeaderboardDef = CachedLeaderboardsDefinitionData[LeaderboardIndex];

		if (!LeaderboardDef->LeaderboardId.empty() && !LeaderboardDef->StatName.empty())
		{
			std::string StatName = FStringUtils::Narrow(LeaderboardDef->StatName);

			EOS_Leaderboards_GetLeaderboardUserScoreCountOptions LeaderboardUserScoresCountOptions = { 0 };
			LeaderboardUserScoresCountOptions.ApiVersion = EOS_LEADERBOARDS_GETLEADERBOARDUSERSCORECOUNT_API_LATEST;
			LeaderboardUserScoresCountOptions.StatName = StatName.c_str();

			uint32_t LeaderboardUserScoresCount = EOS_Leaderboards_GetLeaderboardUserScoreCount(LeaderboardsHandle, &LeaderboardUserScoresCountOptions);

			EOS_Leaderboards_CopyLeaderboardUserScoreByIndexOptions CopyOptions = { 0 };
			CopyOptions.ApiVersion = EOS_LEADERBOARDS_COPYLEADERBOARDUSERSCOREBYINDEX_API_LATEST;
			CopyOptions.StatName = StatName.c_str();

			for (CopyOptions.LeaderboardUserScoreIndex = 0; CopyOptions.LeaderboardUserScoreIndex < LeaderboardUserScoresCount; ++CopyOptions.LeaderboardUserScoreIndex)
			{
				EOS_Leaderboards_LeaderboardUserScore* LeaderboardUserScore = NULL;
				EOS_EResult CopyLeaderboardUserScoreResult = EOS_Leaderboards_CopyLeaderboardUserScoreByIndex(LeaderboardsHandle, &CopyOptions, &LeaderboardUserScore);

				if (CopyLeaderboardUserScoreResult != EOS_EResult::EOS_Success)
				{
					FDebugLog::LogError(L"CopyLeaderboardRecords Failure!");
					break;
				}

				std::shared_ptr<FLeaderboardsUserScoreData> LeaderboardUserScoreData = std::make_shared<FLeaderboardsUserScoreData>();

				LeaderboardUserScoreData->UserId = LeaderboardUserScore->UserId;

				LeaderboardUserScoreData->Score = LeaderboardUserScore->Score;

				CachedUserScores.emplace_back(LeaderboardUserScoreData);

				// Release Leaderboard User Score
				EOS_Leaderboards_LeaderboardUserScore_Release(LeaderboardUserScore);
			}
		}

		CachedLeaderboardsUserScoresData.emplace(LeaderboardDef->LeaderboardId, CachedUserScores);
	}

	PrintLeaderboardUserScores();

	FGameEvent Event(EGameEventType::LeaderboardRecordsReceived);
	FGame::Get().OnGameEvent(Event);
}

void FLeaderboard::PrintLeaderboardUserScores()
{
	// Print info
	for (uint32_t LeaderboardIndex = 0; LeaderboardIndex < CachedLeaderboardsDefinitionData.size(); ++LeaderboardIndex)
	{
		std::shared_ptr<FLeaderboardsDefinitionData> LeaderboardDef = CachedLeaderboardsDefinitionData[LeaderboardIndex];

		if (!LeaderboardDef->LeaderboardId.empty())
		{
			const std::vector<std::shared_ptr<FLeaderboardsUserScoreData>>* CachedUserScores;
			if (GetCachedUserScoresFromLeaderboardId(LeaderboardDef->LeaderboardId, &CachedUserScores))
			{
				FDebugLog::Log(L"-------------");

				FDebugLog::Log(L"%d Leaderboard User Scores for Leaderboard ID: %ls", CachedUserScores->size(), LeaderboardDef->LeaderboardId.c_str());

				for (uint32_t LeaderboardUserScoreIndex = 0; LeaderboardUserScoreIndex < CachedUserScores->size(); ++LeaderboardUserScoreIndex)
				{
					std::shared_ptr<FLeaderboardsUserScoreData> LeaderboardUserScore = (*CachedUserScores)[LeaderboardUserScoreIndex];

					FDebugLog::Log(L"[%d] User ID: %ls, Score: %d",
						LeaderboardUserScoreIndex + 1,
						FStringUtils::Widen(FAccountHelpers::ProductUserIDToString(LeaderboardUserScore->UserId)).c_str(),
						LeaderboardUserScore->Score);
				}

				FDebugLog::Log(L"-------------");
			}
		}
	}
}

const std::map<std::wstring, std::vector<std::shared_ptr<FLeaderboardsUserScoreData>>>& FLeaderboard::GetCachedUserScores() const
{
	return CachedLeaderboardsUserScoresData;
}

bool FLeaderboard::GetCachedUserScoresFromLeaderboardId(std::wstring LeaderboardId, const std::vector<std::shared_ptr<FLeaderboardsUserScoreData>>** OutUserScores) const
{
	if (!OutUserScores)
	{
		return false;
	}

	auto it = CachedLeaderboardsUserScoresData.find(LeaderboardId);
	if (it != CachedLeaderboardsUserScoresData.end())
	{
		*OutUserScores = &(it->second);
		return true;
	}

	*OutUserScores = nullptr;
	return false;
}

void EOS_CALL FLeaderboard::LeaderboardUserScoresReceivedCallbackFn(const EOS_Leaderboards_OnQueryLeaderboardUserScoresCompleteCallbackInfo* Data)
{
	assert(Data != NULL);

	if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
	{
		// Operation is retrying so it is not complete yet
		return;
	}

	if (Data->ResultCode != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"[EOS SDK] Leaderboards - Query User Scores Error: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		return;
	}

	FDebugLog::Log(L"[EOS SDK] Leaderboards - Query User Scores Complete");

	FGame::Get().GetLeaderboard()->CacheLeaderboardUserScores();
}

void FLeaderboard::IngestStat(const std::wstring& StatName, int Amount)
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player == nullptr)
	{
		FDebugLog::LogError(L"Leaderboard - IngestStats: Current player is invalid!");
		return;
	}

	EOS_Stats_IngestStatOptions StatsIngestOptions = { 0 };
	StatsIngestOptions.ApiVersion = EOS_STATS_INGESTSTAT_API_LATEST;
	StatsIngestOptions.LocalUserId = Player->GetProductUserID();
	StatsIngestOptions.TargetUserId = Player->GetProductUserID();
	StatsIngestOptions.StatsCount = 1;

	EOS_Stats_IngestData* IngestData = new EOS_Stats_IngestData[StatsIngestOptions.StatsCount];

	IngestData[0].ApiVersion = EOS_STATS_INGESTDATA_API_LATEST;
	std::string Name = FStringUtils::Narrow(StatName);
	IngestData[0].StatName = Name.c_str();
	IngestData[0].IngestAmount = Amount;

	StatsIngestOptions.Stats = IngestData;

	EOS_Stats_IngestStat(StatsHandle, &StatsIngestOptions, nullptr, StatsIngestCallbackFn);

	delete[] IngestData;
}

void EOS_CALL FLeaderboard::StatsIngestCallbackFn(const EOS_Stats_IngestStatCompleteCallbackInfo* Data)
{
	assert(Data != NULL);

	if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
	{
		// Operation is retrying so it is not complete yet
		return;
	}

	if (Data->ResultCode != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"[EOS SDK] FLeaderboard - Ingest Stats Error: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		return;
	}

	FDebugLog::Log(L"[EOS SDK] FLeaderboard - Stats Ingest Complete");

	FGameEvent Event(EGameEventType::StatsIngested, Data->TargetUserId);
	FGame::Get().OnGameEvent(Event);
}