// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NonCopyable.h"
#include "VoiceRequest.h"
#include "VoiceUser.h"

/** A (ProductUserId, JoinRoomToken) pair */
class FPuidToken final
{
public:
	FPuidToken(const std::string& InProductUserId, const std::string& InToken) : ProductUserId(InProductUserId), Token(InToken) {}
	FPuidToken() = delete;

	std::string ProductUserId;
	std::string Token;
};

/** Result of a JoinRoom request, contains result, roomName, url to connect to and tokens for all requested ProductUserIds */
class FJoinRoomResult final
{
public:
	EOS_EResult Result;
	std::string RoomName;
	std::string ClientBaseUrl;
	std::vector<FPuidToken> Tokens;
};

/** Receipt for FVoiceRequestJoin */
class FJoinRoomReceipt : public FNonCopyable
{
public:
	FJoinRoomReceipt(std::future<FJoinRoomResult>&& InResultFuture, FVoiceRequestHandle InRequestHandle, FVoiceSdk* InVoiceSdk) :
		ResultFuture(std::move(InResultFuture)),
		VoiceRequestHandle(InRequestHandle),
		VoiceSdk(InVoiceSdk)
	{
	}

	~FJoinRoomReceipt();	
	
	void WaitForResult();
	FJoinRoomResult GetResult() { return ResultFuture.get(); }

private:
	std::future<FJoinRoomResult> ResultFuture;
	FVoiceRequestHandle VoiceRequestHandle = nullptr;
	FVoiceSdk* VoiceSdk = nullptr;
};
using FJoinRoomReceiptPtr = std::unique_ptr<FJoinRoomReceipt>;

/** A request for joinRoom tokens for a RoomId and a set of VoiceUsers */
class FVoiceRequestJoin : public FVoiceRequest, public FNonCopyable
{
public:
	FVoiceRequestJoin() = delete;	
	FVoiceRequestJoin(EOS_HRTCAdmin InRTCAdminHandle, const std::string& InRoomName, const std::vector<FVoiceUser>& InVoiceUsers);

	~FVoiceRequestJoin() {}

	virtual EOS_EResult MakeRequest(EOS_HRTCAdmin RTCAdminHandle) override;

	bool ContainsPuid(const EOS_ProductUserId& ProductUserId);
	std::future<FJoinRoomResult> GetFuture();

private:
	EOS_HRTCAdmin RTCAdminHandle = 0;
	std::string RoomName;
	std::vector<FVoiceUser> VoiceUsers;
	std::promise<FJoinRoomResult> ResultPromise;
};

