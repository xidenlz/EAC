// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"

#include "VoiceRequestJoin.h"
#include "VoiceSdk.h"
#include "VoiceUser.h"

#include "DebugLog.h"
#include "Utils.h"
#include "StringUtils.h"

FJoinRoomReceipt::~FJoinRoomReceipt()
{
	if (VoiceSdk && VoiceRequestHandle)
	{
		VoiceSdk->ReleaseRequest(VoiceRequestHandle);
	}
}

void FJoinRoomReceipt::WaitForResult()
{
	ResultFuture.wait();
}

FVoiceRequestJoin::FVoiceRequestJoin(EOS_HRTCAdmin InRTCAdminHandle, const std::string& InRoomName, const std::vector<FVoiceUser>& InVoiceUsers) :
	RTCAdminHandle(InRTCAdminHandle),
	RoomName(InRoomName),
	VoiceUsers(InVoiceUsers)
{
}

std::future<FJoinRoomResult> FVoiceRequestJoin::GetFuture()
{
	return ResultPromise.get_future();
}

EOS_EResult FVoiceRequestJoin::MakeRequest(EOS_HRTCAdmin RTCAdminHandle)
{
	FDebugLog::Log(L"FVoiceRequestJoin::MakeRequest (%x)", this);

	EOS_RTCAdmin_QueryJoinRoomTokenOptions Options = {};
	Options.ApiVersion = EOS_RTCADMIN_QUERYJOINROOMTOKEN_API_LATEST;
	Options.RoomName = RoomName.c_str();

	std::vector<EOS_ProductUserId> TargetUserIds;
	std::vector<const char*> TargetUserIpAddresses;
	for (const FVoiceUser& VoiceUser : VoiceUsers)
	{
		TargetUserIds.emplace_back(VoiceUser.GetPuid());
		TargetUserIpAddresses.emplace_back(VoiceUser.GetIPAddress().c_str());
	}
	Options.TargetUserIds = TargetUserIds.data();
	Options.TargetUserIdsCount = static_cast<uint32_t>(TargetUserIds.size());
	Options.TargetUserIpAddresses = TargetUserIpAddresses.data();

	EOS_RTCAdmin_QueryJoinRoomToken(RTCAdminHandle, &Options, this, [](const EOS_RTCAdmin_QueryJoinRoomTokenCompleteCallbackInfo* Data) {
		FVoiceRequestJoin* Request = reinterpret_cast<FVoiceRequestJoin*>(Data->ClientData);

		// Wait for completion, SDK may retry
		// The lifetime of this callback and its request must be guaranteed until EOS_EResult_IsOperationComplete is true.
		// In this case, we fulfill the ResultPromise which unblocks the wait in VoiceApi. 
		// As the receipt is destructed it releases the request from ActiveRequests in VoiceSdk.
		if (EOS_EResult_IsOperationComplete(Data->ResultCode))
		{
			if (Data->ResultCode == EOS_EResult::EOS_Success)
			{
				FDebugLog::Log(L"FVoiceRequestJoin (%x) - Completed successfully for room %ls", Request, FStringUtils::Widen(Data->RoomName).c_str());

				FJoinRoomResult Result{ };
				Result.Result = Data->ResultCode;
				Result.RoomName = Data->RoomName;
				Result.ClientBaseUrl = Data->ClientBaseUrl;

				EOS_RTCAdmin_CopyUserTokenByIndexOptions CopyOptions = {};
				CopyOptions.ApiVersion = EOS_RTCADMIN_COPYUSERTOKENBYINDEX_API_LATEST;
				CopyOptions.QueryId = Data->QueryId;

				for (CopyOptions.UserTokenIndex = 0; CopyOptions.UserTokenIndex < Data->TokenCount; ++CopyOptions.UserTokenIndex)
				{
					EOS_RTCAdmin_UserToken* UserToken = NULL;
					EOS_EResult CopyResult = EOS_RTCAdmin_CopyUserTokenByIndex(Request->RTCAdminHandle, &CopyOptions, &UserToken);
					if (CopyResult == EOS_EResult::EOS_Success)
					{
						// only return tokens for puids that were requested
						if (Request->ContainsPuid(UserToken->ProductUserId))
						{
							static char PuidBuffer[EOS_PRODUCTUSERID_MAX_LENGTH + 1] = { 0 };
							int32_t PuidBufferSize = sizeof(PuidBuffer);
							EOS_EResult ToStringResult = EOS_ProductUserId_ToString(UserToken->ProductUserId, PuidBuffer, &PuidBufferSize);

							if (ToStringResult == EOS_EResult::EOS_Success)
							{
								Result.Tokens.push_back(FPuidToken(PuidBuffer, UserToken->Token));
							}
							else
							{
								FDebugLog::LogError(L"EOS_ProductUserId_ToString Failure!");
							}
						}

						EOS_RTCAdmin_UserToken_Release(UserToken);
					}
					else
					{
						FDebugLog::LogError(L"EOS_RTCAdmin_CopyUserTokenByIndex Failure!");
					}
				}

				// setting the promise value readies the future in the receipt.
				Request->ResultPromise.set_value(Result);
			}
			else
			{
				FDebugLog::LogError(L"FVoiceRequestJoin (%x) failed: %ls", Request, FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
				FJoinRoomResult Result{ };
				Result.Result = Data->ResultCode;
				Request->ResultPromise.set_value(Result);
			}
		}
		else if (Data->ResultCode == EOS_EResult::EOS_OperationWillRetry)
		{
			FDebugLog::Log(L"FVoiceRequestJoin (%x) will retry", Request);
		}
	});

	return EOS_EResult::EOS_Success;
}

bool FVoiceRequestJoin::ContainsPuid(const EOS_ProductUserId& ProductUserId)
{	
	for (const FVoiceUser& VoiceUser : VoiceUsers)
	{
		if (VoiceUser.GetPuid() == ProductUserId)
		{
			return true;
		}
	}
	return false;
}
