// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include <eos_sdk.h>

struct FUserData;

/**
 * Leaderboard aggregation (used to sort leaderboard user scores).
 */
enum class ELeaderboardAggregation : unsigned int
{
	/** Minimum */
	Min,
	/** Maximum */
	Max,
	/** Sum */
	Sum,
	/** Latest */
	Latest
};

/**
 * Structure to store all data related to leaderboard definitions.
 */
struct FLeaderboardsDefinitionData
{
	FLeaderboardsDefinitionData() = default;

	/** Leaderboard ID that can be used to uniquely identify the leaderboard. */
	std::wstring LeaderboardId;
	/** Name of stat to rank scores by. */
	std::wstring StatName;
	/** Start Time. */
	int64_t StartTime;
	/** End Time. */
	int64_t EndTime;
	/** Aggregation. */
	ELeaderboardAggregation Aggregation;
};

/**
 * Structure to store all data related to leaderboard ranking records.
 */
struct FLeaderboardsRecordData
{
	FLeaderboardsRecordData() = default;

	/** Product User ID. */
	EOS_ProductUserId UserId;
	/** User name */
	std::wstring DisplayName;
	/** Sorted position on leaderboard. */
	uint32_t Rank;
	/** Leaderboard score. */
	int32_t Score;
};

/**
 * Structure to store all data related to leaderboard user scores.
 */
struct FLeaderboardsUserScoreData
{
	FLeaderboardsUserScoreData() = default;

	/** User ID. */
	EOS_ProductUserId UserId;
	/** Leaderboard score. */
	int32_t Score;
};

/**
* Manages leaderboard
*/
class FLeaderboard
{
public:
	/**
	* Constructor
	*/
	FLeaderboard() noexcept(false);

	/**
	* No copying or copy assignment allowed for this class.
	*/
	FLeaderboard(FLeaderboard const&) = delete;
	FLeaderboard& operator=(FLeaderboard const&) = delete;

	/**
	* Destructor
	*/
	virtual ~FLeaderboard();

	/**
	 * Initialization
	 */
	void Init();

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
	 * Starts requesting leaderboard definitions
	 */
	void QueryDefinitions();

	/**
	 * Called after leaderboard definitions have been received, stores leaderboard definitions
	 */
	void CacheLeaderboardDefinitions();

	/**
	 * Prints info about Cached Leaderboard Definitions
	 */
	void PrintLeaderboardDefinitions();

	/**
	 * Gets stored leaderboard definitions
	 */
	std::vector<std::shared_ptr<FLeaderboardsDefinitionData>>& GetCachedDefinitions();

	/**
	 * Gets Ids for all stored leaderboard definitions
	 */
	std::vector<std::wstring> GetDefinitionIds();

	/**
	 * Retrieves leaderboard definition matching the leaderboard Id given
	 *
	 * @return True if definition is found, false otherwise
	 */
	bool GetDefinitionFromId(std::wstring LeaderboardId, std::shared_ptr<FLeaderboardsDefinitionData>& OutDef);

	/**
	 * Retrieves leaderboard definition matching the index for the leaderboard based on the order the definitions were stored
	 *
	 * @return True if definition is found, false otherwise
	 */
	bool GetDefinitionFromIndex(int InIndex, std::shared_ptr<FLeaderboardsDefinitionData>& OutDef);

	/**
	 * Starts requesting leaderboard ranks
	 */
	void QueryRanks(std::wstring LeaderboardId);

	/**
	 * Called after leaderboard ranks have been received, stores leaderboard records
	 */
	void CacheLeaderboardRecords();

	/**
	 * Prints info about cached leaderboard records
	 */
	void PrintLeaderboardRecords();

	/**
	 * Gets stored leaderboard records
	 */
	std::vector<std::shared_ptr<FLeaderboardsRecordData>>& GetCachedRecords();

	/**
	 * Starts requesting leaderboard user scores
	 */
	void QueryUserScores(std::vector<std::wstring> LeaderboardIds, std::vector<EOS_ProductUserId>& UserIds);

	/**
	 * Called after leaderboard user scores have been received, stores leaderboard user scores
	 */
	void CacheLeaderboardUserScores();

	/**
	 * Prints info about cached leaderboard user scores
	 */
	void PrintLeaderboardUserScores();

	/**
	 * Gets all stored leaderboard user scores
	 */
	const std::map<std::wstring, std::vector<std::shared_ptr<FLeaderboardsUserScoreData>>>& GetCachedUserScores() const;

	/**
	 * Gets stored leaderboard user scores for a given leaderboard
	 */
	bool GetCachedUserScoresFromLeaderboardId(std::wstring LeaderboardId, const std::vector<std::shared_ptr<FLeaderboardsUserScoreData>>** OutUserScores) const;

	/**
	 * Starts requesting stats ingest for single stat for logged in user
	 *
	 * @param StatName - Name of Stat to ingest
	 * @param Amount - Amount to ingest
	 */
	void IngestStat(const std::wstring& StatName, int Amount);
	
private:

	/**
	 * Called when a user has logged in
	 */
	void OnLoggedIn(FProductUserId UserId);

	/**
	 * Called when a user has logged out
	 */
	void OnLoggedOut(FProductUserId UserId);

	/**
	 * Callback called after leaderboard definitions have been retrieved
	 */
	static void EOS_CALL LeaderboardDefinitionsReceivedCallbackFn(const EOS_Leaderboards_OnQueryLeaderboardDefinitionsCompleteCallbackInfo* Data);

	/**
	 * Callback called after leaderboard ranks have been retrieved
	 */
	static void EOS_CALL LeaderboardRanksReceivedCallbackFn(const EOS_Leaderboards_OnQueryLeaderboardRanksCompleteCallbackInfo* Data);

	/**
	 * Callback called after leaderboard user scores have been retrieved
	 */
	static void EOS_CALL LeaderboardUserScoresReceivedCallbackFn(const EOS_Leaderboards_OnQueryLeaderboardUserScoresCompleteCallbackInfo* Data);

	/**
	 * Callback called after stats have been ingested
	 */
	static void EOS_CALL StatsIngestCallbackFn(const EOS_Stats_IngestStatCompleteCallbackInfo* Data);

	/** Handle to EOS SDK Leaderboards system */
	EOS_HLeaderboards LeaderboardsHandle;

	/** Handle to EOS SDK stats system */
	EOS_HStats StatsHandle;

	/** Cached data with leaderboard definitions */
	std::vector<std::shared_ptr<FLeaderboardsDefinitionData>> CachedLeaderboardsDefinitionData;

	/** Cached data with leaderboard ranking records */
	std::vector<std::shared_ptr<FLeaderboardsRecordData>> CachedLeaderboardsRecordData;

	/** Cached data with mapping from leaderboard ID to leaderboard user scores */
	std::map<std::wstring, std::vector<std::shared_ptr<FLeaderboardsUserScoreData>>> CachedLeaderboardsUserScoresData;

	/** How many user info queries need to wait before finishing leaderboard records query */
	size_t NumUserInfosLeft = 0;
};
