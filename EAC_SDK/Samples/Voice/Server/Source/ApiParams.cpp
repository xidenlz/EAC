// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"

#include "ApiParams.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/error/en.h"

FParseResult FCreateSessionParams::FromRequestBody(const std::string& Body, FCreateSessionParams& Out)
{	
	rapidjson::Document ReqDoc;	
	ReqDoc.Parse(Body.c_str());
	if (ReqDoc.HasParseError())
	{
		const rapidjson::ParseErrorCode ParseErr = ReqDoc.GetParseError();
		return RESULT_FAILED(rapidjson::GetParseError_En(ParseErr));
	}

	if (!ReqDoc.HasMember("puid") || !ReqDoc["puid"].IsString())
	{
		return RESULT_FAILED("Missing string parameter: puid");
	}

	// optional password
	if (ReqDoc.HasMember("password"))
	{
		if (!ReqDoc["password"].IsString())
		{
			return RESULT_FAILED("Invalid password, expected string");
		}
		Out.Password = ReqDoc["password"].GetString();
	}
	
	Out.Puid = ReqDoc["puid"].GetString();
	return RESULT_OK();
}

FParseResult FKickUserParams::FromRequestBody(const std::string& Body, FKickUserParams& Out)
{
	rapidjson::Document ReqDoc;
	ReqDoc.Parse(Body.c_str());
	if (ReqDoc.HasParseError())
	{
		const rapidjson::ParseErrorCode ParseErr = ReqDoc.GetParseError();
		return RESULT_FAILED(rapidjson::GetParseError_En(ParseErr));
	}

	if (!ReqDoc.HasMember("lock") || !ReqDoc["lock"].IsString())
	{
		return RESULT_FAILED("Missing string parameter: lock");
	}
	
	Out.Lock= ReqDoc["lock"].GetString();
	return RESULT_OK();
}


FParseResult FMuteUserParams::FromRequestBody(const std::string& Body, FMuteUserParams& Out)
{
	rapidjson::Document ReqDoc;
	ReqDoc.Parse(Body.c_str());
	if (ReqDoc.HasParseError())
	{
		const rapidjson::ParseErrorCode ParseErr = ReqDoc.GetParseError();
		return RESULT_FAILED(rapidjson::GetParseError_En(ParseErr));
	}

	if (!ReqDoc.HasMember("lock") || !ReqDoc["lock"].IsString())
	{
		return RESULT_FAILED("Missing string parameter: lock");
	}

	if (!ReqDoc.HasMember("mute") || !ReqDoc["mute"].IsBool())
	{
		return RESULT_FAILED("Missing bool parameter: mute");
	}
	
	Out.Lock = ReqDoc["lock"].GetString();
	Out.bMute = ReqDoc["mute"].GetBool();

	return RESULT_OK();
}

FParseResult FHeartbeatParams::FromRequestBody(const std::string& Body, FHeartbeatParams& Out)
{
	rapidjson::Document ReqDoc;
	ReqDoc.Parse(Body.c_str());

	if (ReqDoc.HasParseError())
	{
		const rapidjson::ParseErrorCode ParseErr = ReqDoc.GetParseError();
		return RESULT_FAILED(rapidjson::GetParseError_En(ParseErr));
	}

	if (!ReqDoc.HasMember("lock") || !ReqDoc["lock"].IsString())
	{
		return RESULT_FAILED("Missing string parameter: lock");
	}

	Out.Lock = ReqDoc["lock"].GetString();

	return RESULT_OK();
}