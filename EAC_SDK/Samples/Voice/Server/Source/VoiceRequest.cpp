// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"

#include "VoiceRequest.h"
#include "VoiceSdk.h"

#include "Utils.h"

FVoiceRequestReceipt::~FVoiceRequestReceipt()
{
	if (VoiceSdk && VoiceRequestHandle)
	{
		VoiceSdk->ReleaseRequest(VoiceRequestHandle);
	}
}

void FVoiceRequestReceipt::WaitForResult()
{
	ResultFuture.wait();
}