// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "eos_common.h"

/** Represents a single user in a voiceSession */
class FVoiceUser
{
public:
	FVoiceUser(const std::string& InPuid, const std::string& InIPAddress)
	: IPAddress(InIPAddress)
	{
		Puid = EOS_ProductUserId_FromString(InPuid.c_str());
	}
	inline const EOS_ProductUserId& GetPuid() const { return Puid; }
	inline const std::string& GetIPAddress() const { return IPAddress; }

	bool operator==(const FVoiceUser& Rhs) const { return Puid == Rhs.Puid && IPAddress == Rhs.IPAddress; }

private:
	EOS_ProductUserId Puid;
	std::string IPAddress;
};