// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "VoiceUser.h"

class FParseResult
{
public:
	FParseResult() : bIsSuccess(true) {}
	explicit FParseResult(const std::string& Error) : bIsSuccess(false), ErrorMsg(Error) {}

	bool IsOk() const { return bIsSuccess; }
	const std::string& GetError() const { return ErrorMsg; }

private:
	bool bIsSuccess = false;
	std::string ErrorMsg;
};

#define RESULT_OK() FParseResult();
#define RESULT_FAILED(x) FParseResult(x);

class FCreateSessionParams final
{
public:
	FCreateSessionParams() {}
	static FParseResult FromRequestBody(const std::string& Body, FCreateSessionParams& Out);

	const std::string& GetPuid() const { return Puid; }
	const std::string& GetPassword() const { return Password; }
	const bool RequiresPassword() const { return Password != ""; }

private:	
	std::string Puid;
	std::string Password;
};

class FKickUserParams final
{
public:
	FKickUserParams() {}
	static FParseResult FromRequestBody(const std::string& Body, FKickUserParams& Out);
	const std::string& GetLock() const { return Lock; }

private:
	std::string Lock;
};

class FMuteUserParams final
{
public:
	FMuteUserParams() {}
	static FParseResult FromRequestBody(const std::string& Body, FMuteUserParams& Out);
	
	const std::string& GetLock() const { return Lock; }
	bool ShouldMute() const { return bMute; }

private:	
	std::string Lock;
	bool bMute = false;
};

class FHeartbeatParams final
{
public:
	FHeartbeatParams() {}
	static FParseResult FromRequestBody(const std::string& Body, FHeartbeatParams& Out);
	const std::string& GetLock() const { return Lock; }	

private:
	std::string Lock;
};


