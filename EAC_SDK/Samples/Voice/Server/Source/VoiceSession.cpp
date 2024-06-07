// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"

#include "VoiceSession.h"
#include "VoiceUser.h"

const uint32_t FVoiceSession::kSessionHeartbeatTimeout = 70;

FVoiceSession::FVoiceSession(const std::string& InSessionId, const std::string& InSessionLock, const std::string& InSessionPassword, const std::initializer_list<FVoiceUser>& InSessionMembers) :
	Expiration(std::chrono::steady_clock::now()),
	SessionId(InSessionId),
	SessionLock(InSessionLock),
	SessionPassword(InSessionPassword),
	SessionMembers(InSessionMembers)
{
	ResetHeartbeat();
}

const std::string& FVoiceSession::GetId() const
{
	return SessionId;
}

const std::string& FVoiceSession::GetLock() const
{
	return SessionLock;
}

bool FVoiceSession::AddUser(const FVoiceUser& InUser)
{	
	ResetHeartbeat();

	FScopedLock Lock(SessionMemberMutex);

	const auto& ExistingUserItr = std::find_if(
		SessionMembers.begin(),
		SessionMembers.end(),
		[&](const FVoiceUser& VoiceUser) { return VoiceUser.GetPuid() == InUser.GetPuid(); }
	);

	if (ExistingUserItr != SessionMembers.end())
	{
		return false;
	}

	SessionMembers.push_back(InUser);
	return true;
}

bool FVoiceSession::RemoveUser(const FVoiceUser& InUser)
{
	ResetHeartbeat();

	FScopedLock Lock(SessionMemberMutex);
	return erase_if(SessionMembers, [&](const FVoiceUser& VoiceUser) { return VoiceUser == InUser; }) != 0;
}

bool FVoiceSession::MatchesPassword(const std::string& InPassword) const
{
	return (SessionPassword.empty() || SessionPassword == InPassword);
}

bool FVoiceSession::BanUser(const FVoiceUser& InUser)
{
	ResetHeartbeat();

	FScopedLock Lock(BanListMutex);
	return PuidBanList.emplace(InUser.GetPuid()).second;
}

bool FVoiceSession::IsUserBanned(const FVoiceUser& User) const
{	
	return PuidBanList.find(User.GetPuid()) != PuidBanList.end();
}

void FVoiceSession::ResetHeartbeat()
{
	Expiration = std::chrono::steady_clock::now() + std::chrono::seconds(FVoiceSession::kSessionHeartbeatTimeout);
}

bool FVoiceSession::IsExpired(const ServerTimePoint& Now) const
{
	return Now > Expiration;
}