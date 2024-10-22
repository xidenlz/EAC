// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "eos_common.h"

/** A session with an optional password, private owner lock and list of members. */
class FVoiceSession
{
public:	
	FVoiceSession(const std::string& InSessionId, const std::string& InSessionLock, const std::string& InSessionPassword, const std::initializer_list<FVoiceUser>& InSessionMembers);
	
	const std::string& GetId() const;
	const std::string& GetLock() const;
	bool MatchesPassword(const std::string& InPassword) const;

	bool AddUser(const FVoiceUser& InUser);
	bool RemoveUser(const FVoiceUser& InUser);

	bool BanUser(const FVoiceUser& InUser);
	bool IsUserBanned(const FVoiceUser& InUser) const;

	void ResetHeartbeat();
	bool IsExpired(const ServerTimePoint& Now) const;

private:
	/** The unique identifier of the session, generated by the voiceServer. */
	std::string SessionId;

	/** Expiration time of the session, heartbeat the session to keep it alive. Used to remove unused sessions */	  
	ServerTimePoint Expiration;

	/** The Lock represents a private key, initially only shared with the creator of the session to perform owner-level operations such as kick or remoteMute. */
	std::string SessionLock;

	/** An optional password for the session. */
	std::string SessionPassword;

	std::mutex SessionMemberMutex;
	std::vector<FVoiceUser> SessionMembers;

	/** Once members get kicked, they land on the ban list to prevent them from rejoining using previously issued tokens. */
	std::mutex BanListMutex;
	std::unordered_set<EOS_ProductUserId> PuidBanList;

	/** Sessions expire after N seconds without any activity or heartbeat */
	static const uint32_t kSessionHeartbeatTimeout;
};