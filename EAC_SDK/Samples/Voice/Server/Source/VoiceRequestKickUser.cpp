// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"

#include "VoiceRequestKickUser.h"
#include "VoiceSdk.h"

#include "DebugLog.h"
#include "StringUtils.h"

FVoiceRequestKickUser::FVoiceRequestKickUser(EOS_HRTCAdmin InRTCAdminHandle, const std::string& InRoomName, const EOS_ProductUserId& InProductUserId) :
	RTCAdminHandle(InRTCAdminHandle),
	RoomName(InRoomName),
	ProductUserId(InProductUserId)
{
}

EOS_EResult FVoiceRequestKickUser::MakeRequest(EOS_HRTCAdmin RTCAdminHandle)
{
	FDebugLog::Log(L"FVoiceRequestKickUser::MakeRequest (%x)", this);

	EOS_RTCAdmin_KickOptions Options = {};
	Options.ApiVersion = EOS_RTCADMIN_KICK_API_LATEST;
	Options.RoomName = RoomName.c_str();
	Options.TargetUserId = ProductUserId;

	EOS_RTCAdmin_Kick(RTCAdminHandle, &Options, this, [](const EOS_RTCAdmin_KickCompleteCallbackInfo* Data) {
		FVoiceRequestKickUser* Request = reinterpret_cast<FVoiceRequestKickUser*>(Data->ClientData);
		
		// Wait for completion, SDK may retry
		// The lifetime of this callback and its request must be guaranteed until EOS_EResult_IsOperationComplete is true.
		// In this case, we fulfill the ResultPromise which unblocks the wait in VoiceApi. 
		// As the receipt is destructed it releases the request from ActiveRequests in VoiceSdk.
		if (EOS_EResult_IsOperationComplete(Data->ResultCode))
		{
			if (Data->ResultCode == EOS_EResult::EOS_Success)
			{
				FDebugLog::Log(L"FVoiceRequestKickUser (%x) - Completed successfully", Request);				
			}
			else
			{
				FDebugLog::LogError(L"FVoiceRequestKickUser (%x) failed: %ls", Request, FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
			}

			Request->ResultPromise.set_value(Data->ResultCode);
		}
		else if (Data->ResultCode == EOS_EResult::EOS_OperationWillRetry)
		{
			FDebugLog::LogWarning(L"FVoiceRequestKickUser (%x) will retry", Request);
		}
	});

	return EOS_EResult::EOS_Success;
}

std::future<EOS_EResult> FVoiceRequestKickUser::GetFuture()
{
	return ResultPromise.get_future();
}
