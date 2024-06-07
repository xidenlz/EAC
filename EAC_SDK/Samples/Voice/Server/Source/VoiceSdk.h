// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NonCopyable.h"
#include "VoiceRequestJoin.h"

#include <eos_sdk.h>

/** Server VoiceSdk Wrapper to manage multiple requests originating from different threads. */
class FVoiceSdk : public FNonCopyable
{
	friend class FJoinRoomReceipt;
	friend class FVoiceRequestReceipt;

public:
	FVoiceSdk() noexcept(false) { }
	virtual ~FVoiceSdk();

	EOS_Bool LoadAndInitSdk();
	EOS_Bool Shutdown();
	
	/** Queues up a request for a joinRoom token for each of the provided users */
	FJoinRoomReceiptPtr CreateJoinRoomTokens(const char* RoomId, const std::vector<FVoiceUser>& Users);

	/** Queues up a request to kick a user from the session */
	FVoiceRequestReceiptPtr KickUser(const char* RoomId, const EOS_ProductUserId& ProductUserId);

	/** Queues up a request to remote mute a user in the session */
	FVoiceRequestReceiptPtr MuteUser(const char* RoomId, const EOS_ProductUserId& ProductUserId, bool bMute);

	/** Processes queued requests */
	void Tick();

private:
	/** called by Receipt friend classes */
	void ReleaseRequest(FVoiceRequestHandle VoiceRequestHandle);

	/** Handle to EOS SDK Platform */
	EOS_HPlatform PlatformHandle = 0;

	/** Handle to EOS SDK RTC Admin system */
	EOS_HRTCAdmin RTCAdminHandle = 0;

	/** mutex for accessing NewRequests & ActiveRequests */
	std::mutex RequestsMutex;
	
	/** new requests that need to be kicked off */
	std::queue<FVoiceRequestPtr> NewRequests;

	/** active requests that are in flight */
	std::vector<FVoiceRequestPtr> ActiveRequests;
};
