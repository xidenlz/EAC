// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NonCopyable.h"

#include <eos_rtc_admin.h>

class FVoiceRequest
{
public:
	virtual ~FVoiceRequest() { }
	virtual EOS_EResult MakeRequest(EOS_HRTCAdmin RTCAdminHandle) = 0;
};

using FVoiceRequestPtr = std::unique_ptr<FVoiceRequest>;
using FVoiceRequestHandle = FVoiceRequest*;

class FVoiceRequestReceipt : public FNonCopyable
{
public:
	FVoiceRequestReceipt(std::future<EOS_EResult>&& InResultFuture, FVoiceRequestHandle InRequestHandle, FVoiceSdk* InVoiceSdk) :
		ResultFuture(std::move(InResultFuture)),
		VoiceRequestHandle(InRequestHandle),
		VoiceSdk(InVoiceSdk)
	{
	}

	virtual ~FVoiceRequestReceipt();

	void WaitForResult();
	EOS_EResult GetResult() { return ResultFuture.get(); }

private:
	std::future<EOS_EResult> ResultFuture;
	FVoiceRequestHandle VoiceRequestHandle = nullptr;
	FVoiceSdk* VoiceSdk = nullptr;
};
using FVoiceRequestReceiptPtr = std::unique_ptr<FVoiceRequestReceipt>;