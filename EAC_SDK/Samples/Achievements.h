// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "eos_sdk.h"
#include "eos_achievements.h"
#include "AccountHelpers.h"

/**
 * Stat info
 */
struct FStatInfo
{
	/** Stat name. */
	std::wstring Name;

	/** Stat threshold value. */
	int32_t ThresholdValue = 0;
};

/**
 * Structure to store all data related to achievements definitions.
 */
struct FAchievementsDefinitionData
{
	FAchievementsDefinitionData() = default;

	/** Achievement ID that can be used to uniquely identify the achievement. */
	std::wstring AchievementId;
	/** Localized display name for the achievement when it has been unlocked. */
	std::wstring UnlockedDisplayName;
	/** Localized description for the achievement when it has been unlocked. */
	std::wstring UnlockedDescription;
	/** Localized display name for the achievement when it is locked or hidden. */
	std::wstring LockedDisplayName;
	/** Localized description for the achievement when it is locked or hidden. */
	std::wstring LockedDescription;
	/** Localized flavor text that can be used by the game in an arbitrary manner. This may be null if there is no data configured in the dev portal */
	std::wstring FlavorText;
	/** URL of an icon to display for the achievement when it is unlocked. This may be null if there is no data configured in the dev portal */
	std::wstring UnlockedIconURL;
	/** URL of an icon to display for the achievement when it is locked or hidden. This may be null if there is no data configured in the dev portal */
	std::wstring LockedIconURL;
	/** True if achievement is hidden, false otherwise. */
	EOS_Bool bIsHidden = EOS_FALSE;
	/** Array of info for stats info. */
	std::vector<FStatInfo> StatInfo;
};

/**
 * Progress for a player stat
 */
struct FStatProgress
{
	/** Current value of player stat. */
	int32_t CurValue = 0;

	/** Threshold value of player stat. */
	int32_t ThresholdValue = 0;
};

/**
 * Structure to store all data related to player achievements.
 */
struct FPlayerAchievementData
{
	FPlayerAchievementData() = default;
	void InitFromSDKData(EOS_Achievements_PlayerAchievement* PlayerAchievement);

	/** Achievement ID that can be used to uniquely identify the achievement. */
	std::wstring AchievementId;
	/** Progress towards completing this achievement (as a percentage). */
	double Progress = 0.0;
	/** If not EOS_ACHIEVEMENTS_ACHIEVEMENT_UNLOCKTIME_UNDEFINED then this is the POSIX timestamp that the achievement was unlocked */
	int64_t UnlockTime = 0;
	/** Array of info for player stats progress. */
	std::map<std::wstring, FStatProgress> Stats;
};

/**
 * Structure to store all data related to stats.
 */
struct FStatData
{
	FStatData() = default;

	/** Stat name. */
	std::wstring Name;
	/** If not EOS_STATS_TIME_UNDEFINED then this is the POSIX timestamp for Start Time */
	int64_t StartTime = 0;
	/** If not EOS_STATS_TIME_UNDEFINED then this is the POSIX timestamp for End Time */
	int64_t EndTime = 0;
	/** Stat value */
	int Value = 0;
};

/**
 * A single stat to ingest
 */
struct FStatIngest
{
public:
	/** Name of Stat to ingest */
	std::wstring Name;
	/** Ingest amount */
	int Amount = 0;
};

/**
 * Manages achievements.
 */
class FAchievements
{
public:
	/**
	 * Constructor
	 */
	FAchievements() noexcept(false);

	/**
	 * No copying or copy assignment allowed for this class.
	 */
	FAchievements(FAchievements const&) = delete;
	FAchievements& operator=(FAchievements const&) = delete;

	/**
	 * Destructor
	 */
	virtual ~FAchievements();

	/**
	 * Initialization
	 */
	void Init();

	/**
	 * Called during shutdown to allow cleanup.
	 */
	void OnShutdown();

	/**
	 * Update
	 */
	void Update();

	/**
	 * Receives game event
	 *
	 * @param Event - Game event to act on
	 */
	void OnGameEvent(const FGameEvent& Event);

	/**
	 * Starts requesting achievement definitions
	 */
	void QueryDefinitions();

	/**
	 * Starts requesting player achievements
	 */
	void QueryPlayerAchievements(FProductUserId UserId);

	/**
	 * Starts requesting unlocking of achievements
	 *
	 * @param AchievementIds - Ids of achievements to unlock
	 */
	void UnlockAchievements(const std::vector<std::wstring>& AchievementIds);

	/**
	 * Called after achievement definitions have been received, stores definitions to show on UI
	 */
	void CacheAchievementsDefinitions();

	/**
	 * Called after player achievements have been received, stores achievements to show progress on UI
	 */
	void CachePlayerAchievements(const EOS_ProductUserId UserId);

	/**
	 * Called after stats have been queried
	 */
	void CacheStats(const EOS_ProductUserId UserId);

	/**
	 * Prints info about Cached Achievement Definitions
	 */
	void PrintAchievementDefinitions();

	/**
	 * Prints info about Cached Player Achievements
	 */
	void PrintPlayerAchievements(FProductUserId UserId);

	/**
	 * Prints info about Cached Stats
	 */
	void PrintStats(FProductUserId UserId);

	/**
	 * Sends notification for unlocked achievement
	 */
	void NotifyUnlockedAchievement(const std::wstring& AchievementId);

	/**
	 * Return the current state of the debug notify flag.
	 */
	bool GetDebugNotify() const { return bDebugNotify; }

	/**
	 * Toggle the debug notify flag then return it.
	 */
	bool ToggleDebugNotify() { bDebugNotify = !bDebugNotify; return bDebugNotify; }

	/**
	 * Gets stored achievement definitions
	 */
	std::vector<std::shared_ptr<FAchievementsDefinitionData>>& GetCachedDefinitions();

	/**
	 * Gets Ids for all stored achievement definitions
	 */
	std::vector<std::wstring> GetDefinitionIds();

	/**
	 * Retrieves achievement definition matching the achievement Id given
	 *
	 * @return True if definition is found, false otherwise
	 */
	bool GetDefinitionFromId(const std::wstring& AchievementId, std::shared_ptr<FAchievementsDefinitionData>& OutDef);

	/**
	 * Retrieves achievement definition matching the index for the achievement based on the order the definitions were stored
	 *
	 * @return True if definition is found, false otherwise
	 */
	bool GetDefinitionFromIndex(int InIndex, std::shared_ptr<FAchievementsDefinitionData>& OutDef);

	/**
	 * Retrieves stored player achievements data for a specified user
	 *
	 * @return True if achievements are found, false otherwise
	 */
	bool GetCachedPlayerAchievements(FProductUserId UserId, std::vector<std::shared_ptr<FPlayerAchievementData>>& OutAchievements);

	/**
	 * Retrieves stored player achievement data based on achievement Id
	 *
	 * @return True if achievement is found, false otherwise
	 */
	bool GetPlayerAchievementFromId(const std::wstring& AchievementId, std::shared_ptr<FPlayerAchievementData>& OutAchievement);

	/** Sets the default language for achievement info display. */
	void SetDefaultLanguage();

	/** Gets default language to use for achievement info display. */
	std::wstring GetDefaultLanguage();

	/**
	 * Starts requesting stats ingest
	 *
	 * @param Stats - Stats to ingest
	 */
	void IngestStats(const std::vector<FStatIngest>& Stats);

	/**
	 * Starts requesting stats ingest for single stat
	 *
	 * @param StatName - Name of Stat to ingest
	 * @param Amount - Amount to ingest
	 */
	void IngestStat(const std::wstring& StatName, int Amount);

	/**
	 * Starts requesting stats ingest for single stat
	 *
	 * @param StatName - Name of Stat to ingest
	 * @param Amount - Amount to ingest
	 * @param TargetUserId - User ID for the target user whose stat we're ingesting
	 */
	void IngestStat(const std::wstring& StatName, int Amount, FProductUserId TargetUserId);

	/**
	 * Starts querying stats for current user
	 */
	void QueryStats();

	/**
	 * Subscribe to achievements unlocked notifications
	 */
	void SubscribeToAchievementsUnlockedNotification();

	/**
	 * Unsubscribe from achievements unlocked notifications
	 */
	void UnsubscribeFromAchievementsUnlockedNotification();

	/** 
	 * Get Icon data for specific achievement. Returns true when data is retrieved successfully.
	 */
	bool GetIconData(const std::wstring& AchievementId, std::vector<char>& OutData, bool bLocked = false);

private:
	/**
	 * Called after a user has logged in
	 */
	void OnLoggedIn(FEpicAccountId UserId);

	/**
	 * Called after a user has logged out
	 */
	void OnLoggedOut(FEpicAccountId UserId);

	/** 
	 * Check and trigger download for icon data.
	 */
	bool CheckAndTriggerDownloadForIconData(const std::wstring& URL);

	/**
	 * Callback called after achievement definitions have been retrieved
	 */
	static void EOS_CALL AchievementDefinitionsReceivedCallbackFn(const EOS_Achievements_OnQueryDefinitionsCompleteCallbackInfo* Data);

	/**
	 * Callback called after player achievements data has been retrieved
	 */
	static void EOS_CALL PlayerAchievementsReceivedCallbackFn(const EOS_Achievements_OnQueryPlayerAchievementsCompleteCallbackInfo* Data);

	/**
	 * Callback called after achievements have been unlocked
	 */
	static void EOS_CALL UnlockAchievementsReceivedCallbackFn(const EOS_Achievements_OnUnlockAchievementsCompleteCallbackInfo* Data);

	/**
	 * Callback called after stats have been ingested
	 */
	static void EOS_CALL StatsIngestCallbackFn(const EOS_Stats_IngestStatCompleteCallbackInfo* Data);

	/**
	 * Callback called after stats have been queried
	 */
	static void EOS_CALL StatsQueryCallbackFn(const EOS_Stats_OnQueryStatsCompleteCallbackInfo* Data);

	/**
	 * Callback called after achievement has been unlocked
	 */
	static void EOS_CALL AchievementsUnlockedReceivedCallbackFn(const EOS_Achievements_OnAchievementsUnlockedCallbackV2Info* Data);

	/** Default Language */
	std::wstring DefaultLanguage;

	/** Handle to EOS SDK achievements system */
	EOS_HAchievements AchievementsHandle = nullptr;

	/** Handle to EOS SDK stats system */
	EOS_HStats StatsHandle = nullptr;

	/** Notification Id for achievement unlocked */
	EOS_NotificationId AchievementsUnlockedNotificationId = EOS_INVALID_NOTIFICATIONID;

	/** Cached data with user achievement definitions */
	std::vector<std::shared_ptr<FAchievementsDefinitionData>> CachedAchievementsDefinitionData;

	/** Cached data with player achievements */
	std::map<EOS_ProductUserId, std::vector<std::shared_ptr<FPlayerAchievementData>>> CachedPlayerAchievements;

	/** Cached data with stats */
	std::map<EOS_ProductUserId, std::vector<std::shared_ptr<FStatData>>> CachedStats;

	/** Downloaded and cached data for achievement icons */
	std::unordered_map<std::wstring, std::vector<char>> IconsData;

	/**
	 * When true then queries from this class will be disabled and the app will
	 * rely on the automatic queries performed by the SDK for the Social Overlay.
	 */
	bool bDebugNotify = false;
};
