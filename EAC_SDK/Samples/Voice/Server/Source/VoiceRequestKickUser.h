// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NonCopyable.h"
#include "VoiceRequest.h"

/** Request to kick a ProductUserId from a session */
class FVoiceRequestKickUser : public FVoiceRequest, public FNonCopyable
{
public:
	FVoiceRequestKickUser() = delete;
	FVoiceRequestKickUser(EOS_HRTCAdmin InRTCAdminHandle, const std::string& InRoomName, const EOS_ProductUserId& InProductUserId);

	virtual EOS_EResult MakeRequest(EOS_HRTCAdmin RTCAdminHandle) override;	
	std::future<EOS_EResult> GetFuture();

private:
	EOS_HRTCAdmin RTCAdminHandle = 0;
	std::string RoomName;
	EOS_ProductUserId ProductUserId;
	std::promise<EOS_EResult> ResultPromise;
};