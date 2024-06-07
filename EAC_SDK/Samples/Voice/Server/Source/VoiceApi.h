// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NonCopyable.h"
#include "VoiceSdk.h"

#include "httplib/httplib.h"

using namespace httplib;

class UserActionParams;

/** Api hosting http endpoints and threadpool */
class FVoiceApi : public FNonCopyable
{
public:
	FVoiceApi(const FVoiceHostPtr& InVoiceHost, const FVoiceSdkPtr& InVoiceSDK);
	
	bool Listen(unsigned short Port);
	void Stop();

	/** Errors & constants */
	static const std::string ErrorSessionNotFound;
	static const std::string ErrorUnauthorized;
	static const std::string ErrorUserNotFound;
	static const std::string ErrorForbidden;
	static const std::string ErrorTimedOut;
	static const char* ContentTypeJson;

private:
	
	/** Note: The VoiceServer sample uses a simple http api to demonstrate communication between clients and the trusted server application.
	  * The http framework used here was chosen for its simplicity, not scalability. 
	  * Real-world applications should consider high-performance asynchronous frameworks or utilize their existing client-server network messaging (e.g. when using dedicated servers).
	*/
	Server Api;
	std::thread ApiThread;

	FVoiceHostPtr VoiceHost;
	FVoiceSdkPtr VoiceSdk;

	EOS_HRTCAdmin RTCAdminHandle = 0;
};
