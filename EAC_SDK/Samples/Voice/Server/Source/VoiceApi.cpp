// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"

#define RAPIDJSON_HAS_STDSTRING 1
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/error/en.h"

#include "ApiParams.h"
#include "VoiceApi.h"
#include "VoiceHost.h"
#include "VoiceUser.h"
#include "VoiceRequestKickUser.h"
#include "VoiceRequestMuteUser.h"

#include "DebugLog.h"
#include "StringUtils.h"
#include "Utils.h"

#include "eos_common.h"

namespace
{
	std::string JsonDocToString(const rapidjson::Document& Document)
	{
		rapidjson::StringBuffer Buffer;
		Buffer.Clear();

		rapidjson::Writer<rapidjson::StringBuffer> Writer(Buffer);
		Document.Accept(Writer);

		return std::string(Buffer.GetString());
	}

	std::string FormatBadRequest(const std::string& Message)
	{
		rapidjson::Document Doc;
		Doc.SetObject();
		Doc.AddMember("error", "bad request", Doc.GetAllocator());
		Doc.AddMember("description", Message, Doc.GetAllocator());
		return JsonDocToString(Doc);
	}
}


const std::string FVoiceApi::ErrorSessionNotFound = "{\"error\" : \"session not found\" }";
const std::string FVoiceApi::ErrorUserNotFound = "{\"error\" : \"user not found\" }";
const std::string FVoiceApi::ErrorForbidden = "{\"error\" : \"invalid lock\" }";
const std::string FVoiceApi::ErrorUnauthorized = "{\"error\" : \"unauthorized\" }";
const std::string FVoiceApi::ErrorTimedOut = "{\"error\" : \"timed out\" }";

const char* FVoiceApi::ContentTypeJson = "application/json";

FVoiceApi::FVoiceApi(const FVoiceHostPtr& InVoiceHost, const FVoiceSdkPtr& InVoiceSDK) :
	VoiceHost(InVoiceHost),
	VoiceSdk(InVoiceSDK)
{
	assert(VoiceHost != nullptr);
	assert(VoiceSdk != nullptr);

	// setup logging
	Api.set_logger([](const Request& Req, const Response& Res) {
		FDebugLog::Log(L"%d | %ls | %ls (%ls)",
			Res.status,
			FStringUtils::Widen(Req.method).c_str(),
			FStringUtils::Widen(Req.path).c_str(),
			FStringUtils::Widen(Req.remote_addr).c_str());
	});

	// create voice session
	Api.Post("/session", [&](const Request& Req, Response &Res) {
		FCreateSessionParams Params;
		FParseResult ParseResult = FCreateSessionParams::FromRequestBody(Req.body, Params);
		if (ParseResult.IsOk())
		{
			// create a random roomId and request a roomToken
			const std::string RoomId = FUtils::GenerateRandomId(16);

			// This http request callback is called on one of the http threadpool threads.
			// All EOS SDK calls must originate from the same thread.
			// To handle this we enqueue a request in the VoiceSdk and wait for its promised result on this thread
			// The main loop will process all EOS SDK requests on the main thread.
			FJoinRoomReceiptPtr Receipt = VoiceSdk->CreateJoinRoomTokens(RoomId.c_str(), { FVoiceUser(Params.GetPuid(), Req.remote_addr) });

			// blocks until the request has been completed
			Receipt->WaitForResult();
			
			const FJoinRoomResult TokenResult = Receipt->GetResult();
			if (TokenResult.Result == EOS_EResult::EOS_Success)
			{
				// generate a lock key that is required for owner-level operations, such as kick or mute.
				const std::string OwnerLock = FUtils::GenerateRandomId(8);

				// add the session and create the json response
				FVoiceSessionPtr Session = FVoiceSessionPtr(new FVoiceSession(RoomId, OwnerLock, Params.GetPassword(), { FVoiceUser(Params.GetPuid(), Req.remote_addr) }));
				VoiceHost->AddSession(Session);

				rapidjson::Document Doc;
				Doc.SetObject();

				Doc.AddMember("sessionId", RoomId, Doc.GetAllocator());
				Doc.AddMember("ownerLock", OwnerLock, Doc.GetAllocator());
				Doc.AddMember("clientBaseUrl", TokenResult.ClientBaseUrl, Doc.GetAllocator());

				rapidjson::Document TokenObj;
				TokenObj.SetObject();
				for (const auto& TokenPair : TokenResult.Tokens)
				{
					TokenObj.AddMember(rapidjson::StringRef(TokenPair.ProductUserId), TokenPair.Token, TokenObj.GetAllocator());
				}
				Doc.AddMember("joinTokens", TokenObj, Doc.GetAllocator());

				Res.status = 200;
				Res.set_content(JsonDocToString(Doc).c_str(), FVoiceApi::ContentTypeJson);

				FDebugLog::Log(L"Created session %ls", FStringUtils::Widen(RoomId).c_str());
			}
			else if (TokenResult.Result == EOS_EResult::EOS_TimedOut)
			{
				Res.status = 408;
				Res.set_content(FVoiceApi::ErrorTimedOut, FVoiceApi::ContentTypeJson);
			}
			else
			{
				Res.status = 500;
			}
		}
		else
		{
			Res.status = 400;
			Res.set_content(FormatBadRequest(ParseResult.GetError()).c_str(), FVoiceApi::ContentTypeJson);
		}
	});

	// join session
	Api.Post(R"(/session/([a-zA-Z0-9\-]+)/join/([a-zA-Z0-9\-]+))", [&](const Request& Req, Response& Res) {
		const std::string& SessionId = Req.matches[1];
		const std::string& Puid = Req.matches[2];

		FVoiceSessionPtr Session = VoiceHost->FindSession(SessionId);
		if (Session.get() != nullptr)
		{
			const FVoiceUser NewUser(Puid, Req.remote_addr);

			// authenticate optional password and check banned list
			const std::string Password = Req.has_param("password") ? Req.get_param_value("password") : "";
			if (!Session->MatchesPassword(Password) || Session->IsUserBanned(NewUser))
			{
				Res.status = 403;
				Res.set_content(FVoiceApi::ErrorUnauthorized, FVoiceApi::ContentTypeJson);
			}
			else
			{
				// request token to join the session
				FJoinRoomReceiptPtr Receipt = VoiceSdk->CreateJoinRoomTokens(SessionId.c_str(), { NewUser });

				// blocks until the request has been completed
				Receipt->WaitForResult();
				
				const FJoinRoomResult Result = Receipt->GetResult();
				if (Result.Result == EOS_EResult::EOS_TimedOut)
				{
					Res.status = 408;
					Res.set_content(FVoiceApi::ErrorTimedOut, FVoiceApi::ContentTypeJson);
				}
				else
				{
					Session->AddUser(FVoiceUser{ Puid, Req.remote_addr });

					rapidjson::Document Doc;
					Doc.SetObject();
					Doc.AddMember("sessionId", SessionId, Doc.GetAllocator());
					Doc.AddMember("clientBaseUrl", Result.ClientBaseUrl, Doc.GetAllocator());

					rapidjson::Document TokenObj;
					TokenObj.SetObject();
					for (const auto& TokenPair : Result.Tokens)
					{
						TokenObj.AddMember(rapidjson::StringRef(TokenPair.ProductUserId), TokenPair.Token, TokenObj.GetAllocator());
					}
					Doc.AddMember("joinTokens", TokenObj, Doc.GetAllocator());

					Res.status = 200;
					Res.set_content(JsonDocToString(Doc).c_str(), FVoiceApi::ContentTypeJson);
				}
			}
		}
		else
		{
			Res.status = 404;
			Res.set_content(FVoiceApi::ErrorSessionNotFound, FVoiceApi::ContentTypeJson);
		}
	});

	// kickUser
	Api.Post(R"(/session/([a-zA-Z0-9\-]+)/kick/([a-zA-Z0-9\-]+))", [&](const Request& Req, Response& Res) {
		const std::string RoomId = Req.matches[1];
		const std::string UserId = Req.matches[2];

		FVoiceSessionPtr Session = VoiceHost->FindSession(RoomId);
		if (Session.get() != nullptr)
		{
			FKickUserParams Params;
			FParseResult Result = FKickUserParams::FromRequestBody(Req.body, Params);
			if (Result.IsOk())
			{
				// check lock
				if (Session->GetLock() != Params.GetLock())
				{
					Res.status = 403;
					Res.set_content(FVoiceApi::ErrorForbidden, FVoiceApi::ContentTypeJson);
				}
				else
				{
					FVoiceUser User{ UserId, std::string() };

					// kick user
					FVoiceRequestReceiptPtr Receipt = VoiceSdk->KickUser(RoomId.c_str(), EOS_ProductUserId_FromString(UserId.c_str()));

					// blocks until the request has been completed
					Receipt->WaitForResult();
					EOS_EResult KickResult = Receipt->GetResult();

					if (KickResult == EOS_EResult::EOS_Success)
					{
						// remove user and ban from rejoining
						Session->RemoveUser(User);
						Session->BanUser(User);
						Res.status = 204;
					}
					else if (KickResult == EOS_EResult::EOS_TimedOut)
					{
						Res.status = 408;
						Res.set_content(FVoiceApi::ErrorTimedOut, FVoiceApi::ContentTypeJson);
					}					
					else
					{
						Res.status = 500;
					}
				}
			}
			else
			{
				Res.status = 400;
				Res.set_content(FormatBadRequest(Result.GetError()).c_str(), FVoiceApi::ContentTypeJson);
			}
		}
		else
		{
			Res.status = 404;
			Res.set_content(FVoiceApi::ErrorSessionNotFound, FVoiceApi::ContentTypeJson);
		}
	});

	// muteUser
	Api.Post(R"(/session/([a-zA-Z0-9\-]+)/mute/([a-zA-Z0-9\-]+))", [&](const Request& Req, Response& Res) {
		const std::string RoomId = Req.matches[1];
		const std::string UserId = Req.matches[2];

		FVoiceSessionPtr Session = VoiceHost->FindSession(RoomId);
		if (Session.get() != nullptr)
		{
			FMuteUserParams Params;
			FParseResult Result = FMuteUserParams::FromRequestBody(Req.body, Params);
			if (Result.IsOk())
			{
				// check lock
				if (Session->GetLock() != Params.GetLock())
				{
					Res.status = 403;
					Res.set_content(FVoiceApi::ErrorForbidden, FVoiceApi::ContentTypeJson);
				}
				else
				{
					// hard mute/unmute user
					FVoiceRequestReceiptPtr Receipt = VoiceSdk->MuteUser(RoomId.c_str(), EOS_ProductUserId_FromString(UserId.c_str()) , Params.ShouldMute());

					// blocks until the request has been completed
					Receipt->WaitForResult();
					EOS_EResult MuteResult = Receipt->GetResult();

					if (MuteResult == EOS_EResult::EOS_Success)
					{
						Res.status = 204;
					}
					else if (MuteResult == EOS_EResult::EOS_TimedOut)
					{
						Res.status = 408;
						Res.set_content("Request Timeout", FVoiceApi::ContentTypeJson);
					}
					else
					{
						Res.status = 500;
					}
					
				}
			}
			else
			{
				Res.status = 400;
				Res.set_content(FormatBadRequest(Result.GetError()).c_str(), FVoiceApi::ContentTypeJson);
			}
		}
		else
		{
			Res.status = 404;
			Res.set_content(FVoiceApi::ErrorSessionNotFound, FVoiceApi::ContentTypeJson);
		}
	});

	// heartbeat session
	Api.Post(R"(/session/([a-zA-Z0-9\-]+)/heartbeat)", [&](const Request& Req, Response& Res) {
		const std::string RoomId = Req.matches[1];

		FVoiceSessionPtr Session = VoiceHost->FindSession(RoomId);
		if (Session.get() != nullptr)
		{
			FHeartbeatParams Params;
			FParseResult Result = FHeartbeatParams::FromRequestBody(Req.body, Params);
			if (Result.IsOk())
			{
				// Heartbeating requires the session lock from the session owner
				// This sample only uses the heartbeat to expire old session from the VoiceHost.
				// A real world application may want to include heartbeats from all participants to remove players from sessions in a similar fashion.
				if (Params.GetLock() == Session->GetLock())
				{
					Session->ResetHeartbeat();
					Res.status = 204;
				}
				else
				{
					Res.status = 403;
					Res.set_content(FVoiceApi::ErrorForbidden, FVoiceApi::ContentTypeJson);
				}
			}
		}
		else
		{
			Res.status = 404;
			Res.set_content(FVoiceApi::ErrorSessionNotFound, FVoiceApi::ContentTypeJson);
		}
	});
}

bool FVoiceApi::Listen(unsigned short Port)
{	
	// start api on a new thread to not block main thread
	ApiThread = std::thread{ [this, Port]() { Api.listen("0.0.0.0", Port); } };
	return true;
}

void FVoiceApi::Stop()
{
	// stop the Api thread and wait for it to complete
	if (ApiThread.joinable())
	{
		Api.stop();
		ApiThread.join();
	}
}
