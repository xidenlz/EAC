// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "VoiceSession.h"

/** Container for multiple voice sessions, managing synchronization */
class FVoiceHost
{
public:
	FVoiceHost() {}

	bool AddSession(const FVoiceSessionPtr& Session);	
	bool RemoveSession(const std::string& Id);
	
	FVoiceSessionPtr FindSession(const std::string& Id);

	/** Compares expiration timestamps of sessions and removes expired ones, clients heartbeat to keep sessions alive. */
	size_t RemoveExpiredSessions();


private:
	std::mutex SessionMutex;
	std::vector<FVoiceSessionPtr> Sessions;
};