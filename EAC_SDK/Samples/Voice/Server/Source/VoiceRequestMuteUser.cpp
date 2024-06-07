// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"

#include "VoiceRequestMuteUser.h"

#include "DebugLog.h"
#include "StringUtils.h"

FVoiceRequestMuteUser::FVoiceRequestMuteUser(EOS_HRTCAdmin InRTCAdminHandle, const std::string& InRoomName, const EOS_ProductUserId& InProductUserId, bool bInMute) :
	RTCAdminHandle(InRTCAdminHandle),
	RoomName(InRoomName),
	ProductUserId(InProductUserId),
	bMute(bInMute)
{
}

EOS_EResult FVoiceRequestMuteUser::MakeRequest(EOS_HRTCAdmin RTCAdminHandle)
{
	EOS_RTCAdmin_SetParticipantHardMuteOptions Options = {};
	Options.ApiVersion = EOS_RTCADMIN_SETPARTICIPANTHARDMUTE_API_LATEST;
	Options.RoomName = RoomName.c_str();
	Options.TargetUserId = ProductUserId;
	Options.bMute = bMute;

	EOS_RTCAdmin_SetParticipantHardMute(RTCAdminHandle, &Options, this, [](const EOS_RTCAdmin_SetParticipantHardMuteCompleteCallbackInfo* Data) {
		FVoiceRequestMuteUser* Request = reinterpret_cast<FVoiceRequestMuteUser*>(Data->ClientData);

		// Wait for completion, SDK may retry
		// The lifetime of this callback and its request must be guaranteed until EOS_EResult_IsOperationComplete is true.
		// In this case, we fulfill the ResultPromise which unblocks the wait in VoiceApi. 
		// As the receipt is destructed it releases the request from ActiveRequests in VoiceSdk.
		if (EOS_EResult_IsOperationComplete(Data->ResultCode))
		{
			if (Data->ResultCode == EOS_EResult::EOS_Success)
			{
				FDebugLog::Log(L"FVoiceRequestMuteUser (%x) - Completed successfully", Request);
			}
			else
			{
				FDebugLog::LogError(L"FVoiceRequestMuteUser (%x) failed: %ls", Request, FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
			}

			Request->ResultPromise.set_value(Data->ResultCode);
		}
		else if (Data->ResultCode == EOS_EResult::EOS_OperationWillRetry)
		{
			FDebugLog::LogWarning(L"FVoiceRequestMuteUser (%x) will retry", Request);
		}
	});

	return EOS_EResult::EOS_Success;
}

std::future<EOS_EResult> FVoiceRequestMuteUser::GetFuture()
{
	return ResultPromise.get_future();
}