// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NonCopyable.h"
#include "VoiceRequest.h"

/** Request to remote mute a user in a voice session */
class FVoiceRequestMuteUser : public FVoiceRequest, public FNonCopyable
{
public:
	FVoiceRequestMuteUser() = delete;
	FVoiceRequestMuteUser(EOS_HRTCAdmin InRTCAdminHandle, const std::string& InRoomName, const EOS_ProductUserId& InProductUserId, bool bMute);

	virtual EOS_EResult MakeRequest(EOS_HRTCAdmin RTCAdminHandle) override;
	std::future<EOS_EResult> GetFuture();

private:
	EOS_HRTCAdmin RTCAdminHandle = 0;
	std::string RoomName;
	EOS_ProductUserId ProductUserId;
	bool bMute = false;
	std::promise<EOS_EResult> ResultPromise;
};
