// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"

#include "VoiceHost.h"

bool FVoiceHost::AddSession(const FVoiceSessionPtr& Session)
{
	FScopedLock Lock(SessionMutex);

	Sessions.push_back(Session);
	return true;
}

bool FVoiceHost::RemoveSession(const std::string& Id)
{
	FScopedLock Lock(SessionMutex);
	return erase_if(Sessions, [&Id](const FVoiceSessionPtr& VoiceSession) { return VoiceSession->GetId() == Id; }) != 0;
}

FVoiceSessionPtr FVoiceHost::FindSession(const std::string& Id)
{
	for (std::vector<FVoiceSessionPtr>::iterator Itr = Sessions.begin(); Itr != Sessions.end(); ++Itr)
	{
		if ((*Itr)->GetId() == Id)
		{
			return *Itr;
		}
	}

	return FVoiceSessionPtr(nullptr);
}

size_t FVoiceHost::RemoveExpiredSessions()
{	
	// Removes all sessions that have expired due to lack of heartbeat.
	// The sample leaves at that and future calls to e.g. join the session will result in Http 404 once expired and removed.
	// Applications may want to include a notification pushed from the server to all room participants or have the client deal with it accordingly.

	const auto Now = std::chrono::steady_clock::now();
	FScopedLock Lock(SessionMutex);
	return erase_if(Sessions, [&Now](const FVoiceSessionPtr& VoiceSession) { return VoiceSession->IsExpired(Now); });
}
