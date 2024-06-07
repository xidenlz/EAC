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
#include "Achievements.h"
#include "HTTPClient.h"
#include <eos_sdk.h>
#include <eos_achievements.h>
#include <eos_stats.h>

#include <time.h>

void FPlayerAchievementData::InitFromSDKData(EOS_Achievements_PlayerAchievement* PlayerAchievement)
{
	if (PlayerAchievement->AchievementId)
	{
		AchievementId = FStringUtils::Widen(PlayerAchievement->AchievementId);
	}

	// Note: Undefined = EOS_ACHIEVEMENTS_ACHIEVEMENT_UNLOCKTIME_UNDEFINED
	UnlockTime = PlayerAchievement->UnlockTime;

	if (PlayerAchievement->StatInfoCount == 0)
	{
		// We have no stats so use overall progress
		Progress = PlayerAchievement->Progress;
	}
	else
	{
		double NumStatsCompleted = 0.0;

		for (int32_t StatIndex = 0; StatIndex < PlayerAchievement->StatInfoCount; ++StatIndex)
		{
			std::wstring StatName = FStringUtils::Widen(PlayerAchievement->StatInfo[StatIndex].Name);

			FStatProgress StatProgress;
			StatProgress.CurValue = PlayerAchievement->StatInfo[StatIndex].CurrentValue;
			StatProgress.ThresholdValue = PlayerAchievement->StatInfo[StatIndex].ThresholdValue;

			Stats.emplace(StatName, StatProgress);

			if (StatProgress.CurValue >= StatProgress.ThresholdValue)
			{
				NumStatsCompleted += 1.0;
			}
		}

		Progress = NumStatsCompleted * (1.0 / PlayerAchievement->StatInfoCount);
	}
}

FAchievements::FAchievements()
	: AchievementsUnlockedNotificationId(EOS_INVALID_NOTIFICATIONID)
{
	
}

FAchievements::~FAchievements()
{
}

void FAchievements::Init()
{
	if (FPlatform::GetPlatformHandle() != nullptr)
	{
		AchievementsHandle = EOS_Platform_GetAchievementsInterface(FPlatform::GetPlatformHandle());

		StatsHandle = EOS_Platform_GetStatsInterface(FPlatform::GetPlatformHandle());

		/*if (FPlatform::GetPlatformHandle())
		{
			DefaultLanguage = L"en";
			EOS_Platform_SetOverrideLocaleCode(FPlatform::GetPlatformHandle(), FStringUtils::Narrow(DefaultLanguage).c_str());
		}*/

		SubscribeToAchievementsUnlockedNotification();
	}
}

void FAchievements::OnShutdown()
{
	UnsubscribeFromAchievementsUnlockedNotification();
}

void FAchievements::Update()
{

}

void FAchievements::OnGameEvent(const FGameEvent& Event)
{
	if (Event.GetType() == EGameEventType::UserLoggedIn)
	{
		FEpicAccountId UserId = Event.GetUserId();
		OnLoggedIn(UserId);
	}
	else if (Event.GetType() == EGameEventType::UserLoggedOut)
	{
		FEpicAccountId UserId = Event.GetUserId();
		OnLoggedOut(UserId);
	}
	else if (!bDebugNotify && Event.GetType() == EGameEventType::UserConnectLoggedIn)
	{
		QueryDefinitions();
		QueryPlayerAchievements(Event.GetProductUserId());
		QueryStats();
	}
	else if (!bDebugNotify && Event.GetType() == EGameEventType::AchievementsUnlocked)
	{
		QueryPlayerAchievements(Event.GetProductUserId());
	}
	else if (!bDebugNotify && Event.GetType() == EGameEventType::StatsIngested)
	{
		QueryPlayerAchievements(Event.GetProductUserId());
	}
}

void FAchievements::OnLoggedIn(FEpicAccountId UserId)
{
	
}

void FAchievements::OnLoggedOut(FEpicAccountId UserId)
{
	if (FPlayerManager::Get().GetNumPlayers() == 0)
	{
		FGameEvent Event(EGameEventType::NoUserLoggedIn);
		FGame::Get().OnGameEvent(Event);
	}
}

bool FAchievements::CheckAndTriggerDownloadForIconData(const std::wstring& URL)
{
	auto Iter = IconsData.find(URL);
	if (Iter == IconsData.end())
	{
		FHTTPClient::GetInstance().PerformHTTPRequest(FStringUtils::Narrow(URL), FHTTPClient::EHttpRequestMethod::GET, std::string(), [URL](FHTTPClient::HTTPErrorCode ErrorCode, const std::vector<char>& Data)
		{
			if (ErrorCode == 200)
			{
				FGame::Get().GetAchievements()->IconsData[URL] = Data;
			}
			else
			{
				std::string ErrorString(Data.data(), Data.size());
				FDebugLog::LogError(L"Could not load icon data. HTTP Request failed: %ls", FStringUtils::Widen(ErrorString).c_str());
				FGame::Get().GetAchievements()->IconsData.erase(URL);
			}
		}
		);
		IconsData[URL] = std::vector<char>();
		return true;
	}
	else
	{
		return false;
	}
}

void FAchievements::QueryDefinitions()
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player == nullptr)
	{
		FDebugLog::LogError(L"FAchievements - QueryDefinitions: Current player is invalid!");
		return;
	}

	EOS_Achievements_QueryDefinitionsOptions QueryDefinitionsOptions = {};
	QueryDefinitionsOptions.ApiVersion = EOS_ACHIEVEMENTS_QUERYDEFINITIONS_API_LATEST;
	QueryDefinitionsOptions.LocalUserId = Player->GetProductUserID();

	EOS_Achievements_QueryDefinitions(AchievementsHandle, &QueryDefinitionsOptions, nullptr, AchievementDefinitionsReceivedCallbackFn);
}

void FAchievements::QueryPlayerAchievements(FProductUserId UserId)
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player == nullptr)
	{
		FDebugLog::LogError(L"Achievements - QueryPlayerAchievements: Current player is invalid!");
		return;
	}

	if (!UserId.IsValid())
	{
		// Use local player's id
		UserId = Player->GetProductUserID();
	}

	EOS_Achievements_QueryPlayerAchievementsOptions QueryPlayerAchievementsOptions = {};
	QueryPlayerAchievementsOptions.ApiVersion = EOS_ACHIEVEMENTS_QUERYPLAYERACHIEVEMENTS_API_LATEST;
	QueryPlayerAchievementsOptions.LocalUserId = Player->GetProductUserID();
	QueryPlayerAchievementsOptions.TargetUserId = UserId;

	EOS_Achievements_QueryPlayerAchievements(AchievementsHandle, &QueryPlayerAchievementsOptions, nullptr, PlayerAchievementsReceivedCallbackFn);
}

void FAchievements::UnlockAchievements(const std::vector<std::wstring>& AchievementIds)
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player == nullptr)
	{
		FDebugLog::LogError(L"Achievements - UnlockAchievements: Current player is invalid!");
		return;
	}

	EOS_Achievements_UnlockAchievementsOptions UnlockAchievementsOptions = {};
	UnlockAchievementsOptions.ApiVersion = EOS_ACHIEVEMENTS_UNLOCKACHIEVEMENTS_API_LATEST;
	UnlockAchievementsOptions.UserId = Player->GetProductUserID();
	UnlockAchievementsOptions.AchievementsCount = (uint32_t)AchievementIds.size();
	UnlockAchievementsOptions.AchievementIds = new const char*[AchievementIds.size()];
	std::vector<std::string> NarrowIDs;
	for (std::wstring AchievementId : AchievementIds)
	{
		NarrowIDs.emplace_back(FStringUtils::Narrow(AchievementId));
	}
	for (uint32_t Index = 0; Index < AchievementIds.size(); ++Index)
	{
		UnlockAchievementsOptions.AchievementIds[Index] = NarrowIDs[Index].c_str();
	}

	EOS_Achievements_UnlockAchievements(AchievementsHandle, &UnlockAchievementsOptions, nullptr, UnlockAchievementsReceivedCallbackFn);

	delete[] UnlockAchievementsOptions.AchievementIds;
}

void FAchievements::SubscribeToAchievementsUnlockedNotification()
{
	UnsubscribeFromAchievementsUnlockedNotification();

	EOS_Achievements_AddNotifyAchievementsUnlockedV2Options AchievementsUnlockedNotifyOptions = {};
	AchievementsUnlockedNotifyOptions.ApiVersion = EOS_ACHIEVEMENTS_ADDNOTIFYACHIEVEMENTSUNLOCKEDV2_API_LATEST;
	AchievementsUnlockedNotificationId = EOS_Achievements_AddNotifyAchievementsUnlockedV2(AchievementsHandle, &AchievementsUnlockedNotifyOptions, nullptr, AchievementsUnlockedReceivedCallbackFn);
}

void FAchievements::UnsubscribeFromAchievementsUnlockedNotification()
{
	if (AchievementsUnlockedNotificationId != EOS_INVALID_NOTIFICATIONID)
	{
		EOS_Achievements_RemoveNotifyAchievementsUnlocked(AchievementsHandle, AchievementsUnlockedNotificationId);
		AchievementsUnlockedNotificationId = EOS_INVALID_NOTIFICATIONID;
	}
}

bool FAchievements::GetIconData(const std::wstring& AchievementId, std::vector<char>& OutData, bool bLocked /*= false*/)
{
	std::shared_ptr<FAchievementsDefinitionData> AchievementDefinition;
	if (!GetDefinitionFromId(AchievementId, AchievementDefinition) || !AchievementDefinition)
	{
		return false;
	}

	const std::wstring IconURL = (bLocked) ? AchievementDefinition->LockedIconURL : AchievementDefinition->UnlockedIconURL;
	if (IconsData.find(IconURL) != IconsData.end())
	{
		OutData = IconsData[IconURL];
		return !OutData.empty();
	}
	else
	{
		return false;
	}
}

void EOS_CALL FAchievements::AchievementDefinitionsReceivedCallbackFn(const EOS_Achievements_OnQueryDefinitionsCompleteCallbackInfo* Data)
{
	assert(Data != NULL);

	if (Data->ResultCode != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"[EOS SDK] Achievements - Get Definitions Error: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		return;
	}

	FDebugLog::Log(L"[EOS SDK] Achievements - Get Definitions Complete");

	FGame::Get().GetAchievements()->CacheAchievementsDefinitions();
}

void EOS_CALL FAchievements::PlayerAchievementsReceivedCallbackFn(const EOS_Achievements_OnQueryPlayerAchievementsCompleteCallbackInfo* Data)
{
	assert(Data != NULL);

	if (Data->ResultCode != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"[EOS SDK] Achievements - Get Player Achievements Error: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		return;
	}

	FDebugLog::Log(L"[EOS SDK] Achievements - Get Player Achievements Completed - LocalUserId: %ls, TargetUserId: %ls", FStringUtils::Widen(FAccountHelpers::ProductUserIDToString(Data->LocalUserId)).c_str(), FStringUtils::Widen(FAccountHelpers::ProductUserIDToString(Data->TargetUserId)).c_str());

	FGame::Get().GetAchievements()->CachePlayerAchievements(Data->TargetUserId);

	FGameEvent Event(EGameEventType::PlayerAchievementsReceived, Data->TargetUserId);
	FGame::Get().OnGameEvent(Event);
}

void EOS_CALL FAchievements::UnlockAchievementsReceivedCallbackFn(const EOS_Achievements_OnUnlockAchievementsCompleteCallbackInfo* Data)
{
	assert(Data != NULL);

	if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
	{
		// Operation is retrying so it is not complete yet
		return;
	}

	if (Data->ResultCode != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"[EOS SDK] Achievements - Unlock Achievements Error: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		return;
	}

	FDebugLog::Log(L"[EOS SDK] Achievements - Unlock Achievement Completed - User ID: %ls", FStringUtils::Widen(FAccountHelpers::ProductUserIDToString(Data->UserId)).c_str());
}

void EOS_CALL FAchievements::StatsIngestCallbackFn(const EOS_Stats_IngestStatCompleteCallbackInfo* Data)
{
	assert(Data != NULL);

	if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
	{
		// Operation is retrying so it is not complete yet
		return;
	}

	if (Data->ResultCode != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"[EOS SDK] Achievements - Ingest Stats Error: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		return;
	}

	FDebugLog::Log(L"[EOS SDK] Achievements - Stats Ingest Complete");

	FGameEvent Event(EGameEventType::StatsIngested, Data->TargetUserId);
	FGame::Get().OnGameEvent(Event);
}

void EOS_CALL FAchievements::StatsQueryCallbackFn(const EOS_Stats_OnQueryStatsCompleteCallbackInfo* Data)
{
	assert(Data != NULL);

	if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
	{
		// Operation is retrying so it is not complete yet
		return;
	}

	if (Data->ResultCode != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"[EOS SDK] Achievements - Query Stats Error: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		return;
	}

	FGame::Get().GetAchievements()->CacheStats(Data->TargetUserId);

	FDebugLog::Log(L"[EOS SDK] Achievements - Stats Query Complete");

	FGameEvent Event(EGameEventType::StatsQueried, Data->TargetUserId);
	FGame::Get().OnGameEvent(Event);
}

void EOS_CALL FAchievements::AchievementsUnlockedReceivedCallbackFn(const EOS_Achievements_OnAchievementsUnlockedCallbackV2Info* Data)
{
	const std::wstring WideAchievementId = FStringUtils::Widen(Data->AchievementId);
	FDebugLog::Log(L"Achievements Unlocked: %ls", WideAchievementId.c_str());
	FGame::Get().GetAchievements()->NotifyUnlockedAchievement(WideAchievementId);

	FGameEvent Event(EGameEventType::AchievementsUnlocked, Data->UserId);
	FGame::Get().OnGameEvent(Event);
}

void FAchievements::CacheAchievementsDefinitions()
{
	EOS_HAchievements AchievementsHandle = EOS_Platform_GetAchievementsInterface(FPlatform::GetPlatformHandle());

	EOS_Achievements_GetAchievementDefinitionCountOptions AchievementDefinitionsCountOptions = {};
	AchievementDefinitionsCountOptions.ApiVersion = EOS_ACHIEVEMENTS_GETACHIEVEMENTDEFINITIONCOUNT_API_LATEST;

	uint32_t AchievementDefinitionsCount = EOS_Achievements_GetAchievementDefinitionCount(AchievementsHandle, &AchievementDefinitionsCountOptions);

	EOS_Achievements_CopyAchievementDefinitionV2ByIndexOptions CopyOptions = {};
	CopyOptions.ApiVersion = EOS_ACHIEVEMENTS_COPYDEFINITIONV2BYACHIEVEMENTID_API_LATEST;

	// Clear definitions
	CachedAchievementsDefinitionData.clear();

	for (CopyOptions.AchievementIndex = 0; CopyOptions.AchievementIndex < AchievementDefinitionsCount; ++CopyOptions.AchievementIndex)
	{
		EOS_Achievements_DefinitionV2* AchievementDef = NULL;
		EOS_EResult CopyAchievementDefinitionsResult = EOS_Achievements_CopyAchievementDefinitionV2ByIndex(AchievementsHandle, &CopyOptions, &AchievementDef);
		if (CopyAchievementDefinitionsResult != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"CopyAchievementDefinitions Failure!");
			break;
		}

		std::shared_ptr<FAchievementsDefinitionData> AchievementsDefinition = std::make_shared<FAchievementsDefinitionData>();

		AchievementsDefinition->bIsHidden = AchievementDef->bIsHidden;

		AchievementsDefinition->AchievementId = FStringUtils::Widen(AchievementDef->AchievementId);

		if (AchievementDef->UnlockedDisplayName)
		{
			AchievementsDefinition->UnlockedDisplayName = FStringUtils::Widen(AchievementDef->UnlockedDisplayName);
		}

		if (AchievementDef->UnlockedDescription)
		{
			AchievementsDefinition->UnlockedDescription = FStringUtils::Widen(AchievementDef->UnlockedDescription);
		}

		if (AchievementDef->LockedDisplayName)
		{
			AchievementsDefinition->LockedDisplayName = FStringUtils::Widen(AchievementDef->LockedDisplayName);
		}

		if (AchievementDef->LockedDescription)
		{
			AchievementsDefinition->LockedDescription = FStringUtils::Widen(AchievementDef->LockedDescription);
		}

		if (AchievementDef->FlavorText)
		{
			AchievementsDefinition->FlavorText = FStringUtils::Widen(AchievementDef->FlavorText);
		}

		if (AchievementDef->UnlockedIconURL)
		{
			AchievementsDefinition->UnlockedIconURL = FStringUtils::Widen(AchievementDef->UnlockedIconURL);
			CheckAndTriggerDownloadForIconData(AchievementsDefinition->UnlockedIconURL);
		}

		if (AchievementDef->LockedIconURL)
		{
			AchievementsDefinition->LockedIconURL = FStringUtils::Widen(AchievementDef->LockedIconURL);
			CheckAndTriggerDownloadForIconData(AchievementsDefinition->LockedIconURL);
		}

		for (uint32_t StatIndex = 0; StatIndex < AchievementDef->StatThresholdsCount; ++StatIndex)
		{
			FStatInfo StatInfo;
			StatInfo.Name = FStringUtils::Widen(AchievementDef->StatThresholds[StatIndex].Name);
			StatInfo.ThresholdValue = AchievementDef->StatThresholds[StatIndex].Threshold;
			AchievementsDefinition->StatInfo.emplace_back(StatInfo);
		}

		CachedAchievementsDefinitionData.emplace_back(AchievementsDefinition);

		// Release Achievement Definition
		EOS_Achievements_DefinitionV2_Release(AchievementDef);
	}

	PrintAchievementDefinitions();

	FGameEvent Event(EGameEventType::DefinitionsReceived);
	FGame::Get().OnGameEvent(Event);
}

void FAchievements::CachePlayerAchievements(const EOS_ProductUserId UserId)
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player == nullptr)
	{
		FDebugLog::LogError(L"CachePlayerAchievements - Local user is invalid");
		return;
	}

	EOS_HAchievements AchievementsHandle = EOS_Platform_GetAchievementsInterface(FPlatform::GetPlatformHandle());

	EOS_Achievements_GetPlayerAchievementCountOptions AchievementsCountOptions = {};
	AchievementsCountOptions.ApiVersion = EOS_ACHIEVEMENTS_GETPLAYERACHIEVEMENTCOUNT_API_LATEST;
	AchievementsCountOptions.UserId = UserId;

	uint32_t AchievementsCount = EOS_Achievements_GetPlayerAchievementCount(AchievementsHandle, &AchievementsCountOptions);

	EOS_Achievements_CopyPlayerAchievementByIndexOptions CopyOptions = {};
	CopyOptions.ApiVersion = EOS_ACHIEVEMENTS_COPYPLAYERACHIEVEMENTBYINDEX_API_LATEST;
	CopyOptions.TargetUserId = UserId;
	CopyOptions.LocalUserId = Player->GetProductUserID();

	// Clear user achievements
	CachedPlayerAchievements.erase(UserId);

	std::vector<std::shared_ptr<FPlayerAchievementData>> PlayerAchievements;
	std::vector<std::shared_ptr<FPlayerAchievementData>> UnlockedAchievements;

	for (CopyOptions.AchievementIndex = 0; CopyOptions.AchievementIndex < AchievementsCount; ++CopyOptions.AchievementIndex)
	{
		EOS_Achievements_PlayerAchievement* Achievement = NULL;
		EOS_EResult CopyAchievementResult = EOS_Achievements_CopyPlayerAchievementByIndex(AchievementsHandle, &CopyOptions, &Achievement);
		if (CopyAchievementResult != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"CopyPlayerAchievement Failure!");
			break;
		}

		std::shared_ptr<FPlayerAchievementData> PlayerAchievement = std::make_shared<FPlayerAchievementData>();
		PlayerAchievement->InitFromSDKData(Achievement);
		PlayerAchievements.push_back(PlayerAchievement);

		if (PlayerAchievement->UnlockTime != EOS_ACHIEVEMENTS_ACHIEVEMENT_UNLOCKTIME_UNDEFINED)
		{
			UnlockedAchievements.push_back(PlayerAchievement);
		}

		// Release
		EOS_Achievements_PlayerAchievement_Release(Achievement);
	}

	CachedPlayerAchievements.emplace(UserId, PlayerAchievements);

	PrintPlayerAchievements(UserId);
}

void FAchievements::CacheStats(const EOS_ProductUserId UserId)
{
	EOS_HStats StatsHandle = EOS_Platform_GetStatsInterface(FPlatform::GetPlatformHandle());

	EOS_Stats_GetStatCountOptions StatCountOptions = {};
	StatCountOptions.ApiVersion = EOS_STATS_GETSTATCOUNT_API_LATEST;
	StatCountOptions.TargetUserId = UserId;

	uint32_t StatsCount = EOS_Stats_GetStatsCount(StatsHandle, &StatCountOptions);

	EOS_Stats_Stat* Stat = NULL;
	EOS_Stats_CopyStatByIndexOptions CopyByIndexOptions = {};
	CopyByIndexOptions.ApiVersion = EOS_STATS_COPYSTATBYINDEX_API_LATEST;
	CopyByIndexOptions.TargetUserId = UserId;

	// Clear user stats
	CachedStats.erase(UserId);

	std::vector<std::shared_ptr<FStatData>> Stats;

	for (CopyByIndexOptions.StatIndex = 0; CopyByIndexOptions.StatIndex < StatsCount; ++CopyByIndexOptions.StatIndex)
	{
		EOS_EResult CopyStatResult = EOS_Stats_CopyStatByIndex(StatsHandle, &CopyByIndexOptions, &Stat);
		if (CopyStatResult != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Copy Stat failure");
			break;
		}

		std::shared_ptr<FStatData> StatData = std::make_shared<FStatData>();

		if (Stat->Name)
		{
			StatData->Name = FStringUtils::Widen(Stat->Name);
		}

		// Note: Undefined = EOS_STATS_QUERYSTATS_API_LATEST
		StatData->StartTime = Stat->StartTime;
		StatData->EndTime = Stat->EndTime;

		StatData->Value = Stat->Value;

		Stats.emplace_back(StatData);

		// Release
		EOS_Stats_Stat_Release(Stat);
	}

	CachedStats.emplace(UserId, Stats);

	PrintStats(UserId);
}

void FAchievements::PrintAchievementDefinitions()
{
	// Print info
	FDebugLog::Log(L"%d Achievements:", CachedAchievementsDefinitionData.size());

	for (uint32_t AchievementIndex = 0; AchievementIndex < CachedAchievementsDefinitionData.size(); ++AchievementIndex)
	{
		FDebugLog::Log(L"-------------");
		FDebugLog::Log(L"Achievement: %d", AchievementIndex + 1);

		std::shared_ptr<FAchievementsDefinitionData> AchievementDef = CachedAchievementsDefinitionData[AchievementIndex];

		FDebugLog::Log(L"Is Hidden: %ls", AchievementDef->bIsHidden ? L"true" : L"false");

		if (!AchievementDef->AchievementId.empty())
		{
			FDebugLog::Log(L"Achievement ID: %ls", AchievementDef->AchievementId.c_str());
		}
		if (!AchievementDef->UnlockedDisplayName.empty())
		{
			FDebugLog::Log(L"Display Name: %ls", AchievementDef->UnlockedDisplayName.c_str());
		}
		if (!AchievementDef->UnlockedDescription.empty())
		{
			FDebugLog::Log(L"Description: %ls", AchievementDef->UnlockedDescription.c_str());
		}
		if (!AchievementDef->LockedDisplayName.empty())
		{
			FDebugLog::Log(L"Locked Display Name: %ls", AchievementDef->LockedDisplayName.c_str());
		}
		if (!AchievementDef->LockedDescription.empty())
		{
			FDebugLog::Log(L"Locked Description: %ls", AchievementDef->LockedDescription.c_str());
		}
		if (!AchievementDef->UnlockedIconURL.empty())
		{
			FDebugLog::Log(L"Unlocked Icon: %ls", AchievementDef->UnlockedIconURL.c_str());
		}
		if (!AchievementDef->LockedIconURL.empty())
		{
			FDebugLog::Log(L"Locked Icon: %ls", AchievementDef->LockedIconURL.c_str());
		}
		if (!AchievementDef->FlavorText.empty())
		{
			FDebugLog::Log(L"Flavor text: %ls", AchievementDef->FlavorText.c_str());
		}
		if (!AchievementDef)
		{
			FDebugLog::Log(L"Flavor text: %ls", AchievementDef->FlavorText.c_str());
		}

		FDebugLog::Log(L"-------------");
	}
}

void FAchievements::PrintPlayerAchievements(FProductUserId UserId)
{
	auto it = CachedPlayerAchievements.find(UserId);
	if (it != CachedPlayerAchievements.end())
	{
		int AchievementIndex = 0;
		std::vector<std::shared_ptr<FPlayerAchievementData>> AchData = it->second;

		FDebugLog::Log(L"%d Achievements for UserId: %ls", AchData.size(), UserId.ToString().c_str());

		for (std::shared_ptr<FPlayerAchievementData> NextAchievement : AchData)
		{
			FDebugLog::Log(L"-------------");
			FDebugLog::Log(L"Achievement: %d", AchievementIndex + 1);

			if (!NextAchievement->AchievementId.empty())
			{
				FDebugLog::Log(L"Achievement ID: %ls", NextAchievement->AchievementId.c_str());
			}
			FDebugLog::Log(L"Progress: %f", NextAchievement->Progress);
			if (NextAchievement->UnlockTime == EOS_ACHIEVEMENTS_ACHIEVEMENT_UNLOCKTIME_UNDEFINED)
			{
				FDebugLog::Log(L"Unlock Time: Undefined");
			}
			else
			{
				std::wstring DateTimeStrW = FUtils::ConvertUnixTimestampToUTCString(NextAchievement->UnlockTime);
				FDebugLog::Log(L"Unlock Time: %ls", DateTimeStrW.c_str());
			}

			FDebugLog::Log(L"Stat Info: %d", NextAchievement->Stats.size());
			for (auto Itr = NextAchievement->Stats.begin(); Itr != NextAchievement->Stats.end(); ++Itr)
			{
				FDebugLog::Log(L"  Name: %ls, Current: %d, Threshold: %d",
					Itr->first.c_str(),
					Itr->second.CurValue,
					Itr->second.ThresholdValue);
			}

			FDebugLog::Log(L"-------------");

			++AchievementIndex;
		}
	}
}

void FAchievements::PrintStats(FProductUserId UserId)
{
	auto it = CachedStats.find(UserId);
	if (it != CachedStats.end())
	{
		int StatIndex = 0;
		std::vector<std::shared_ptr<FStatData>> StatData = it->second;

		FDebugLog::Log(L"%d Stat for UserId: %ls", StatData.size(), UserId.ToString().c_str());

		for (std::shared_ptr<FStatData> NextStat : StatData)
		{
			FDebugLog::Log(L"-------------");
			FDebugLog::Log(L"Stat: %d", StatIndex + 1);

			FDebugLog::Log(L"Name: %ls", NextStat->Name.c_str());
			if (NextStat->StartTime == EOS_STATS_TIME_UNDEFINED)
			{
				FDebugLog::Log(L"Start Time: Undefined");
			}
			else
			{
				std::wstring StartTimeStrW = FUtils::ConvertUnixTimestampToUTCString(NextStat->StartTime);
				FDebugLog::Log(L"Start Time: %ls", StartTimeStrW.c_str());
			}
			if (NextStat->EndTime == EOS_STATS_TIME_UNDEFINED)
			{
				FDebugLog::Log(L"End Time: Undefined");
			}
			else
			{
				std::wstring EndTimeStrW = FUtils::ConvertUnixTimestampToUTCString(NextStat->EndTime);
				FDebugLog::Log(L"End Time: %ls", EndTimeStrW.c_str());
			}
			FDebugLog::Log(L"Value: %d", NextStat->Value);

			FDebugLog::Log(L"-------------");

			++StatIndex;
		}
	}
}

void FAchievements::NotifyUnlockedAchievement(const std::wstring& AchievementId)
{
	std::wstring Msg = L"Achievement Unlocked: " + AchievementId;
	FGameEvent Event(EGameEventType::AddNotification, Msg);
	FGame::Get().OnGameEvent(Event);
}

std::vector<std::shared_ptr<FAchievementsDefinitionData>>& FAchievements::GetCachedDefinitions()
{
	return CachedAchievementsDefinitionData;
}

std::vector<std::wstring> FAchievements::GetDefinitionIds()
{
	std::vector<std::wstring> DefinitionIds;

	for (std::shared_ptr<FAchievementsDefinitionData>& NextDefinition : CachedAchievementsDefinitionData)
	{
		if (!NextDefinition->AchievementId.empty())
		{
			DefinitionIds.emplace_back(NextDefinition->AchievementId);
		}
	}

	return DefinitionIds;
}

bool FAchievements::GetDefinitionFromId(const std::wstring& AchievementId, std::shared_ptr<FAchievementsDefinitionData>& OutDef)
{
	for (std::shared_ptr<FAchievementsDefinitionData> NextDefinition : CachedAchievementsDefinitionData)
	{
		if (NextDefinition->AchievementId == AchievementId)
		{
			OutDef = NextDefinition;
			return true;
		}
	}

	return false;
}

bool FAchievements::GetDefinitionFromIndex(int InIndex, std::shared_ptr<FAchievementsDefinitionData>& OutDef)
{
	int Index = 0;
	for (std::shared_ptr<FAchievementsDefinitionData> NextDefinition : CachedAchievementsDefinitionData)
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

bool FAchievements::GetCachedPlayerAchievements(FProductUserId UserId, std::vector<std::shared_ptr<FPlayerAchievementData>>& OutAchievements)
{
	auto it = CachedPlayerAchievements.find(UserId.AccountId);
	if (it != CachedPlayerAchievements.end())
	{
		OutAchievements = it->second;
		return true;
	}

	return false;
}

bool FAchievements::GetPlayerAchievementFromId(const std::wstring& AchievementId, std::shared_ptr<FPlayerAchievementData>& OutAchievement)
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player == nullptr)
	{
		return false;
	}

	auto it = CachedPlayerAchievements.find(Player->GetProductUserID());
	if (it != CachedPlayerAchievements.end())
	{
		std::vector< std::shared_ptr<FPlayerAchievementData>> AchData = it->second;
		for (std::shared_ptr<FPlayerAchievementData> NextAchievement : AchData)
		{
			if (NextAchievement->AchievementId == AchievementId)
			{
				OutAchievement = NextAchievement;
				return true;
			}
		}
	}

	return false;
}

void FAchievements::SetDefaultLanguage()
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player != nullptr)
	{
		DefaultLanguage = FStringUtils::Widen(Player->GetLocale());
	}
	else
	{
		char Buffer[EOS_LOCALECODE_MAX_BUFFER_LEN];
		int32_t BufferLen = sizeof(Buffer);
		if (EOS_Platform_GetOverrideLocaleCode(FPlatform::GetPlatformHandle(), Buffer, &BufferLen) == EOS_EResult::EOS_Success)
		{
			DefaultLanguage = FStringUtils::Widen(Buffer);
		}
		else
		{
			DefaultLanguage = L"en";
		}
	}
}

std::wstring FAchievements::GetDefaultLanguage()
{
	return DefaultLanguage;
}

void FAchievements::IngestStats(const std::vector<FStatIngest>& Stats)
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player == nullptr)
	{
		FDebugLog::LogError(L"Achievements - IngestStats: Current player is invalid!");
		return;
	}

	EOS_Stats_IngestStatOptions StatsIngestOptions = {};
	StatsIngestOptions.ApiVersion = EOS_STATS_INGESTSTAT_API_LATEST;
	StatsIngestOptions.LocalUserId = Player->GetProductUserID();
	StatsIngestOptions.TargetUserId = Player->GetProductUserID();
	StatsIngestOptions.StatsCount = (uint32_t)Stats.size();

	EOS_Stats_IngestData* IngestData = new EOS_Stats_IngestData[Stats.size()];

	std::vector<std::string> NarrowStatNames;
	for (FStatIngest Stat : Stats)
	{
		NarrowStatNames.emplace_back(FStringUtils::Narrow(Stat.Name));
	}

	for (uint32_t Index = 0; Index < Stats.size(); ++Index)
	{
		IngestData[Index].ApiVersion = EOS_STATS_INGESTDATA_API_LATEST;
		IngestData[Index].StatName = NarrowStatNames[Index].c_str();
		IngestData[Index].IngestAmount = Stats[Index].Amount;
	}

	StatsIngestOptions.Stats = IngestData;

	EOS_Stats_IngestStat(StatsHandle, &StatsIngestOptions, nullptr, StatsIngestCallbackFn);

	delete[] IngestData;
}

void FAchievements::IngestStat(const std::wstring& StatName, int Amount)
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player == nullptr)
	{
		FDebugLog::LogError(L"Achievements - IngestStat: Current player is invalid!");
		return;
	}

	if (StatName.empty())
	{
		FDebugLog::LogError(L"Achievements - IngestStat: Stat Name is empty");
		return;
	}

	if (Amount <= 0)
	{
		FDebugLog::LogError(L"Achievements - IngestStat: Amount is invalid");
		return;
	}

	EOS_Stats_IngestStatOptions StatsIngestOptions = {};
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

void FAchievements::IngestStat(const std::wstring& StatName, int Amount, FProductUserId TargetUserId)
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player == nullptr)
	{
		FDebugLog::LogError(L"Achievements - IngestStat: Current player is invalid!");
		return;
	}

	if (!TargetUserId.IsValid())
	{
		FDebugLog::LogError(L"Achievements - IngestStat: Target player is invalid!");
		return;
	}

	if (StatName.empty())
	{
		FDebugLog::LogError(L"Achievements - IngestStat: Stat Name is empty");
		return;
	}

	if (Amount <= 0)
	{
		FDebugLog::LogError(L"Achievements - IngestStat: Amount is invalid");
		return;
	}

	EOS_Stats_IngestStatOptions StatsIngestOptions = {};
	StatsIngestOptions.ApiVersion = EOS_STATS_INGESTSTAT_API_LATEST;
	StatsIngestOptions.LocalUserId = Player->GetProductUserID();
	StatsIngestOptions.TargetUserId = TargetUserId;
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

void FAchievements::QueryStats()
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player == nullptr)
	{
		FDebugLog::LogError(L"Achievements - QueryStats: Current player is invalid!");
		return;
	}

	// Query Player Stats
	EOS_Stats_QueryStatsOptions StatsQueryOptions = {};
	StatsQueryOptions.ApiVersion = EOS_STATS_QUERYSTATS_API_LATEST;
	StatsQueryOptions.LocalUserId = Player->GetProductUserID();
	StatsQueryOptions.TargetUserId = Player->GetProductUserID();

	// Optional params
	StatsQueryOptions.StartTime = EOS_STATS_TIME_UNDEFINED;
	StatsQueryOptions.EndTime = EOS_STATS_TIME_UNDEFINED;
	StatsQueryOptions.StatNamesCount = 0;
	StatsQueryOptions.StatNames = NULL;

	//StatsQueryOptions.StartTime = 1575417600; // 12/04/2019 @ 12:00am (UTC)
	//StatsQueryOptions.EndTime = 1575590400; // 12/06/2019 @ 12:00am (UTC)
	//StatsQueryOptions.StatNamesCount = 2;
	//StatsQueryOptions.StatNames = new const char*[StatsQueryOptions.StatNamesCount];
	//StatsQueryOptions.StatNames[0] = "test_sum_stat_1";
	//StatsQueryOptions.StatNames[1] = "test_sum_stat_2";

	EOS_Stats_QueryStats(StatsHandle, &StatsQueryOptions, nullptr, StatsQueryCallbackFn);

	//delete[] StatsQueryOptions.StatNames;
}
