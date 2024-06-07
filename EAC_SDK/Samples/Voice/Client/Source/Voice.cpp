// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "Game.h"
#include "GameEvent.h"
#include "Main.h"
#include "Platform.h"
#include "Users.h"
#include "Player.h"
#include "Voice.h"

#include "HTTPClient.h"
#include "DebugLog.h"
#include "StringUtils.h"
#include "AccountHelpers.h"
#include "SampleConstants.h"
#include "CommandLine.h"
#include "Utils.h"

#define RAPIDJSON_HAS_STDSTRING 1
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/error/en.h"

#include <eos_presence.h>
#include <eos_ui.h>
#include <eos_rtc.h>

#include <regex>

constexpr const char* SampleJoinInfoFormat = R"({"RoomName": "%s"})";
constexpr const char* SampleJoinInfoRegex = R"(\{\"RoomName\"\s*:\s*\"(.*)\"\})";

/** Password used to create and join a room session, set to nullptr to disable requirement for password */
constexpr const char* SessionPassword = "a682473a335a4449ac235d86eb961526";

constexpr char SampleConstants::ServerURL[];
constexpr int SampleConstants::ServerPort;

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
}

FVoice::FVoice()
{
	
}

FVoice::~FVoice()
{

}

void FVoice::Init()
{
	if (!FPlatform::IsInitialized())
	{
		return;
	}

	if (FPlatform::GetPlatformHandle() != nullptr)
	{
		RTCHandle = EOS_Platform_GetRTCInterface(FPlatform::GetPlatformHandle());
		RTCAdminHandle = EOS_Platform_GetRTCAdminInterface(FPlatform::GetPlatformHandle());
		RTCAudioHandle = EOS_RTC_GetAudioInterface(RTCHandle);
	}

	std::string ServerURLStr = SampleConstants::ServerURL;
	TrustedServerURL = FStringUtils::Widen(ServerURLStr);
	if (FCommandLine::Get().HasParam(CommandLineConstants::ServerURL))
	{
		TrustedServerURL = FCommandLine::Get().GetParamValue(CommandLineConstants::ServerURL);
	}

	TrustedServerPort = std::to_wstring(SampleConstants::ServerPort);
	if (FCommandLine::Get().HasParam(CommandLineConstants::ServerPort))
	{
		TrustedServerPort = FCommandLine::Get().GetParamValue(CommandLineConstants::ServerPort);
	}

	FullTrustedServerURL = TrustedServerURL + L":" + TrustedServerPort;

	SubscribeToNotifications();
	
	UpdateAudioInputDevices();
	UpdateAudioOutputDevices();
}

void FVoice::OnShutdown()
{
	ClearRoomMembers();

	UnsubscribeFromNotifications();
}

void FVoice::Update()
{
	// heartbeat the session
	if (!CurrentRoomName.empty() && !OwnerLock.empty())
	{
		if (std::chrono::steady_clock::now() > NextHeartbeat)
		{
			if (LocalProductUserId.IsValid())
			{
				HeartbeatVoiceSession(LocalProductUserId, OwnerLock);
			}
		}
	}
}

void FVoice::OnLoggedIn(FEpicAccountId UserId)
{
	LocalEpicUserId = UserId;

	SetJoinInfo("");
}

void FVoice::OnLoggedOut(FEpicAccountId UserId)
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(UserId);
	if (Player)
	{
		QueryLeaveRoom(Player->GetProductUserID(), CurrentRoomName);
	}

	SetJoinInfo("");

	UnsubscribeFromRoomNotifications();

	ClearRoomMembers();

	ClearOwnerLock();

	CachedUserRoomTokenInfo.clear();

	if (LocalEpicUserId == UserId)
	{
		LocalEpicUserId = FEpicAccountId();
		LocalProductUserId = FProductUserId();
	}

	FGameEvent Event(EGameEventType::NoUserLoggedIn);
	FGame::Get().OnGameEvent(Event);
}

void FVoice::OnUserConnectLoggedIn(FProductUserId ProductUserId)
{
	LocalProductUserId = ProductUserId;

	// After login cache devices is empty
	UpdateAudioInputDevices();
	UpdateAudioOutputDevices();
}

void FVoice::OnGameEvent(const FGameEvent& Event)
{
	if (Event.GetType() == EGameEventType::UserLoggedIn)
	{
		FEpicAccountId UserId = Event.GetUserId();
		OnLoggedIn(UserId);
	}
	else if (Event.GetType() == EGameEventType::UserLoggedOut)
	{
		FEpicAccountId UserId = Event.GetUserId();
		OnLoggedOut(UserId);
	}
	else if (Event.GetType() == EGameEventType::UserConnectLoggedIn)
	{
		FProductUserId ProductUserId = Event.GetProductUserId();
		OnUserConnectLoggedIn(ProductUserId);
	}
	else if (Event.GetType() == EGameEventType::JoinRoom)
	{
		RequestJoinRoom(Event.GetProductUserId(), Event.GetFirstStr());
	}
	else if (Event.GetType() == EGameEventType::LeaveRoom)
	{
		QueryLeaveRoom(Event.GetProductUserId(), CurrentRoomName);
	}
	else if (Event.GetType() == EGameEventType::StartChatWithFriend)
	{
		JoinRoomWithFriend(Event.GetUserId(), Event.GetProductUserId(), Event.GetFirstStr());
	}
	else if (Event.GetType() == EGameEventType::EpicAccountsMappingRetrieved)
	{
		OnEpicAccountsMappingRetrieved();
	}
	else if (Event.GetType() == EGameEventType::EpicAccountDisplayNameRetrieved)
	{
		OnEpicAccountDisplayNameRetrieved(Event.GetUserId(), Event.GetFirstStr());
	}
	else if (Event.GetType() == EGameEventType::Kick)
	{
		KickMember(Event.GetProductUserId());
	}
	else if (Event.GetType() == EGameEventType::RemoteMute)
	{
		RemoteMuteMember(Event.GetProductUserId(), Event.GetFirstExtendedType() == 1);
	}
	else if (Event.GetType() == EGameEventType::UpdateReceivingVolume)
	{
		UpdateReceivingVolume(Event.GetFirstStr());
	}
	else if (Event.GetType() == EGameEventType::UpdateParticipantVolume)
	{
		UpdateParticipantVolume(Event.GetProductUserId(), Event.GetFirstStr());
	}
}

bool FVoice::IsMember(FProductUserId ProductUserId)
{
	auto Itr = RoomMembers.find(ProductUserId);
	if (Itr != RoomMembers.end())
	{
		return true;
	}
	return false;
}

void FVoice::ClearRoomMembers()
{
	for (std::map<EOS_ProductUserId, FVoiceRoomMember>::iterator Itr = RoomMembers.begin(); Itr != RoomMembers.end(); ++Itr)
	{
		if (Itr->second.Player != nullptr)
		{
			FPlayerManager::Get().Remove(Itr->second.Player->GetUserID());
		}
	}

	RoomMembers.clear();
}

void FVoice::SubscribeToRoomNotifications(std::wstring InRoomName)
{
	FDebugLog::Log(L"Voice (SubscribeToRoomNotifications)");

	if (LocalProductUserId.IsValid())
	{
		std::string RoomNameStr = FStringUtils::Narrow(InRoomName);

		// Local user is disconnected
		if (DisconnectedNotification == EOS_INVALID_NOTIFICATIONID)
		{
			EOS_RTC_AddNotifyDisconnectedOptions DisconnectedOptions = {};
			DisconnectedOptions.ApiVersion = EOS_RTC_ADDNOTIFYDISCONNECTED_API_LATEST;
			DisconnectedOptions.LocalUserId = LocalProductUserId;
			DisconnectedOptions.RoomName = RoomNameStr.c_str();
			DisconnectedNotification = EOS_RTC_AddNotifyDisconnected(RTCHandle, &DisconnectedOptions, nullptr, OnDisconnectedCb);
		}

		// Participant Status Changed
		if (ParticipantStatusChangedNotification == EOS_INVALID_NOTIFICATIONID)
		{
			EOS_RTC_AddNotifyParticipantStatusChangedOptions ParticipantStatusChangedOptions = {};
			ParticipantStatusChangedOptions.ApiVersion = EOS_RTC_ADDNOTIFYPARTICIPANTSTATUSCHANGED_API_LATEST;
			ParticipantStatusChangedOptions.LocalUserId = LocalProductUserId;
			ParticipantStatusChangedOptions.RoomName = RoomNameStr.c_str();
			ParticipantStatusChangedNotification = EOS_RTC_AddNotifyParticipantStatusChanged(RTCHandle, &ParticipantStatusChangedOptions, nullptr, OnParticipantStatusChangedCb);
		}

		// Participant Updated
		if (ParticipantAudioUpdatedNotification == EOS_INVALID_NOTIFICATIONID)
		{
			EOS_RTCAudio_AddNotifyParticipantUpdatedOptions ParticipantAudioUpdatedOptions = {};
			ParticipantAudioUpdatedOptions.ApiVersion = EOS_RTCAUDIO_ADDNOTIFYPARTICIPANTUPDATED_API_LATEST;
			ParticipantAudioUpdatedOptions.LocalUserId = LocalProductUserId;
			ParticipantAudioUpdatedOptions.RoomName = RoomNameStr.c_str();
			ParticipantAudioUpdatedNotification = EOS_RTCAudio_AddNotifyParticipantUpdated(RTCAudioHandle, &ParticipantAudioUpdatedOptions, nullptr, OnParticipantAudioUpdatedCb);
		}
	}
}

void FVoice::UnsubscribeFromRoomNotifications()
{
	FDebugLog::Log(L"Voice (UnsubscribeFromRoomNotifications)");

	if (DisconnectedNotification != EOS_INVALID_NOTIFICATIONID)
	{
		EOS_RTC_RemoveNotifyDisconnected(RTCHandle, DisconnectedNotification);
		DisconnectedNotification = EOS_INVALID_NOTIFICATIONID;
	}
	if (ParticipantStatusChangedNotification != EOS_INVALID_NOTIFICATIONID)
	{
		EOS_RTC_RemoveNotifyParticipantStatusChanged(RTCHandle, ParticipantStatusChangedNotification);
		ParticipantStatusChangedNotification = EOS_INVALID_NOTIFICATIONID;
	}
	if (ParticipantAudioUpdatedNotification != EOS_INVALID_NOTIFICATIONID)
	{
		EOS_RTCAudio_RemoveNotifyParticipantUpdated(RTCAudioHandle, ParticipantAudioUpdatedNotification);
		ParticipantAudioUpdatedNotification = EOS_INVALID_NOTIFICATIONID;
	}
}

void FVoice::SubscribeToNotifications()
{
	// Audio devices changed
	if (AudioDevicesChangedNotification == EOS_INVALID_NOTIFICATIONID)
	{
		EOS_RTCAudio_AddNotifyAudioDevicesChangedOptions AudioDevicesChangedOptions = {};
		AudioDevicesChangedOptions.ApiVersion = EOS_RTCAUDIO_ADDNOTIFYAUDIODEVICESCHANGED_API_LATEST;
		AudioDevicesChangedNotification = EOS_RTCAudio_AddNotifyAudioDevicesChanged(RTCAudioHandle, &AudioDevicesChangedOptions, nullptr, OnAudioDevicesChangedCb);
	}
}

void FVoice::UnsubscribeFromNotifications()
{
	if (AudioDevicesChangedNotification != EOS_INVALID_NOTIFICATIONID)
	{
		EOS_RTCAudio_RemoveNotifyAudioDevicesChanged(RTCAudioHandle, AudioDevicesChangedNotification);
		AudioDevicesChangedNotification = EOS_INVALID_NOTIFICATIONID;
	}
}

void FVoice::LocalToggleMuteMember(FProductUserId ProductUserId)
{
	if (ProductUserId.IsValid())
	{
		auto Itr = RoomMembers.find(ProductUserId);
		if (Itr != RoomMembers.end())
		{
			// Don't toggle mute if we're muted remotely
			if (!Itr->second.bIsRemoteMuted)
			{
				Itr->second.bIsMuted = !Itr->second.bIsMuted;

				FDebugLog::Log(L"Local Muting - Id: %ls - %ls", ProductUserId.ToString().c_str(), Itr->second.bIsMuted ? L"Muted" : L"Unmuted");

				FGameEvent Event(EGameEventType::RoomDataUpdated);
				FGame::Get().OnGameEvent(Event);

				std::string RoomNameStr = FStringUtils::Narrow(CurrentRoomName);

				if (LocalProductUserId == ProductUserId)
				{
					EOS_RTCAudio_UpdateSendingOptions UpdateSendingOptions = { 0 };
					UpdateSendingOptions.ApiVersion = EOS_RTCAUDIO_UPDATESENDING_API_LATEST;
					UpdateSendingOptions.LocalUserId = LocalProductUserId;
					UpdateSendingOptions.RoomName = RoomNameStr.c_str();
					UpdateSendingOptions.AudioStatus = Itr->second.bIsMuted ? EOS_ERTCAudioStatus::EOS_RTCAS_Disabled : EOS_ERTCAudioStatus::EOS_RTCAS_Enabled;
					EOS_RTCAudio_UpdateSending(RTCAudioHandle, &UpdateSendingOptions, nullptr, OnAudioUpdateSendingCb);
				}
				else
				{
					EOS_RTCAudio_UpdateReceivingOptions UpdateReceivingOptions = { 0 };
					UpdateReceivingOptions.ApiVersion = EOS_RTCAUDIO_UPDATERECEIVING_API_LATEST;
					UpdateReceivingOptions.LocalUserId = LocalProductUserId;
					UpdateReceivingOptions.RoomName = RoomNameStr.c_str();
					UpdateReceivingOptions.ParticipantId = ProductUserId;
					UpdateReceivingOptions.bAudioEnabled = Itr->second.bIsMuted ? EOS_FALSE : EOS_TRUE;
					EOS_RTCAudio_UpdateReceiving(RTCAudioHandle, &UpdateReceivingOptions, nullptr, OnAudioUpdateReceivingCb);
				}
			}
		}
	}
}

void FVoice::QueryJoinRoomToken(FProductUserId ProductUserId, std::wstring InRoomName)
{
	// [Trusted Server]
	// Send message to trusted server with join room info

	std::wstring URL = FullTrustedServerURL;
	URL.append(L"/session");

	if (InRoomName.empty())
	{
		// Create and join session (with json request body)
		rapidjson::Document Doc;
		Doc.SetObject();
		Doc.AddMember("puid", FStringUtils::Narrow(ProductUserId.ToString()), Doc.GetAllocator());
		if (SessionPassword != nullptr)
		{
			// Add optional password
			const std::string Password = SessionPassword;
			Doc.AddMember("password", Password, Doc.GetAllocator());
		}
		QueryJoinRoomTokenRequestBody = JsonDocToString(Doc);
		FDebugLog::Log(L"Query join token Body: %ls", FStringUtils::Widen(QueryJoinRoomTokenRequestBody).c_str());
	}
	else
	{
		// Join session
		URL.append(L"/");
		URL.append(InRoomName);
		URL.append(L"/join/");
		URL.append(ProductUserId.ToString());
		if (SessionPassword != nullptr)
		{
			// Add optional password
			const std::string Password = SessionPassword;
			URL.append(L"?password=");
			URL.append(FStringUtils::Widen(Password));
		}
		// No body required
		QueryJoinRoomTokenRequestBody = std::string();
	}
	FDebugLog::Log(L"Query join token URL: %ls", URL.c_str());

	FHTTPClient::GetInstance().PerformHTTPRequest(FStringUtils::Narrow(URL), FHTTPClient::EHttpRequestMethod::POST, QueryJoinRoomTokenRequestBody,
		[ProductUserId, InRoomName](FHTTPClient::HTTPErrorCode ErrorCode, const std::vector<char>& Data)
	{
		if (ErrorCode == 200)
		{
			std::string ResponseString(Data.data(), Data.size());

			if (ResponseString.find("error") != std::string::npos)
			{
				FDebugLog::LogError(L"Query join token failed, Response: %ls", FStringUtils::Widen(ResponseString).c_str());
			}
			else
			{
				FDebugLog::Log(L"Query join token success, Response: %ls", FStringUtils::Widen(ResponseString).c_str());

				// Parse Json data from response
				rapidjson::Document ResponseDoc;
				ResponseDoc.Parse(ResponseString.c_str());
				if (ResponseDoc.HasParseError())
				{
					const rapidjson::ParseErrorCode ParseErr = ResponseDoc.GetParseError();
					std::string ParseErrorStr = rapidjson::GetParseError_En(ParseErr);
					FDebugLog::LogError(L"Query join token failed, Json response parse error: %ls", FStringUtils::Widen(ParseErrorStr).c_str());
					return;
				}

				// SessionId (Room Name)
				if (!ResponseDoc.HasMember("sessionId") || !ResponseDoc["sessionId"].IsString())
				{
					FDebugLog::LogError(L"Query join token failed, Json response missing sessionId");
					return;
				}
				std::string SessionIdStr = ResponseDoc["sessionId"].GetString();

				// Owner Lock
				std::string OwnerLockStr;
				if (ResponseDoc.HasMember("ownerLock") && ResponseDoc["ownerLock"].IsString())
				{
					OwnerLockStr = ResponseDoc["ownerLock"].GetString();
				}

				// Client Base URL
				std::string ClientBaseUrlStr;
				if (!ResponseDoc.HasMember("clientBaseUrl") || !ResponseDoc["clientBaseUrl"].IsString())
				{
					FDebugLog::LogError(L"Query join token failed, Json response missing clientBaseUrl");
					return;
				}
				ClientBaseUrlStr = ResponseDoc["clientBaseUrl"].GetString();

				// Join Tokens
				if (ResponseDoc.HasMember("joinTokens") && ResponseDoc["joinTokens"].IsArray())
				{
					FDebugLog::LogError(L"Query join token failed, Json response missing joinTokens");
					return;
				}
				const rapidjson::Value& JoinTokens = ResponseDoc["joinTokens"];
				if (JoinTokens.MemberCount() != 1)
				{
					FDebugLog::LogError(L"Query join token failed, joinTokens array should have one member - count: %d", JoinTokens.MemberCount());
					return;
				}
				std::string PuidStr;
				std::string TokenStr;
				for (rapidjson::Value::ConstMemberIterator JoinTokensItr = JoinTokens.MemberBegin(); JoinTokensItr != JoinTokens.MemberEnd(); ++JoinTokensItr)
				{
					PuidStr = JoinTokensItr->name.GetString();
					TokenStr = JoinTokensItr->value.GetString();
				}
				EOS_ProductUserId JoinTokenProductUserId = EOS_ProductUserId_FromString(PuidStr.c_str());
				if (JoinTokenProductUserId != ProductUserId)
				{
					FDebugLog::LogError(L"Query join token failed, joinTokens ProductUserId mismatch - Request ID: %ls, Response ID: %ls",
						ProductUserId.ToString().c_str(),
						FStringUtils::Widen(PuidStr).c_str());
					return;
				}

				// Cache room token
				std::shared_ptr<FVoiceUserRoomTokenData> UserRoomToken = std::make_shared<FVoiceUserRoomTokenData>();
				UserRoomToken->ProductUserId = JoinTokenProductUserId;
				UserRoomToken->Token = TokenStr;
				FGame::Get().GetVoice()->AddLocalUserRoomTokenData(UserRoomToken);

				// Join Room
				std::wstring WideRoomName = FStringUtils::Widen(SessionIdStr);
				std::wstring WideClientBaseUrl = FStringUtils::Widen(ClientBaseUrlStr);
				FGame::Get().GetVoice()->JoinFromCachedRoomTokenData(WideRoomName, WideClientBaseUrl);

				// Set owner lock (empty if we are not the owner)
				FGame::Get().GetVoice()->SetOwnerLock(OwnerLockStr);
			}
		}
		else
		{
			std::string ErrorString(Data.data(), Data.size());
			FDebugLog::LogError(L"Query join token failed, HTTP Request failed, Code: %d, Error %ls", ErrorCode, FStringUtils::Widen(ErrorString).c_str());
		}
	});
}

void FVoice::RemoteToggleMuteMember(FProductUserId ProductUserId)
{
	FDebugLog::Log(L"Remote Mute Member - Id: %ls", ProductUserId.ToString().c_str());

	// [Trusted Server]
	// Ask trusted server to remote mute user

	auto Itr = RoomMembers.find(ProductUserId);
	if (Itr != RoomMembers.end())
	{
		FDebugLog::Log(L"Remote Muting - Id: %ls - %ls", ProductUserId.ToString().c_str(), !Itr->second.bIsRemoteMuted ? L"Mute" : L"Unmute");

		RemoteMuteMember(ProductUserId, !Itr->second.bIsRemoteMuted);
	}
}

void FVoice::RemoteMuteMember(FProductUserId ProductUserId, bool bIsMuted)
{
	FDebugLog::Log(L"Remote Mute Member - Id: %ls", ProductUserId.ToString().c_str());

	// [Trusted Server]
	// Ask trusted server to remote mute user

	if (OwnerLock.empty())
	{
		FDebugLog::LogError(L"Mute failed, Owner lock is invalid");
		return;
	}

	std::wstring URL = FullTrustedServerURL;
	URL.append(L"/session/");
	URL.append(CurrentRoomName);
	URL.append(L"/mute/");
	URL.append(ProductUserId.ToString());
	FDebugLog::Log(L"Mute URL: %ls", URL.c_str());

	// Add owner lock and mute to json request body
	rapidjson::Document Doc;
	Doc.SetObject();
	Doc.AddMember("lock", OwnerLock, Doc.GetAllocator());
	Doc.AddMember("mute", bIsMuted, Doc.GetAllocator());
	MuteRequestBody = JsonDocToString(Doc);
	FDebugLog::Log(L"Mute Body: %ls", FStringUtils::Widen(MuteRequestBody).c_str());

	FHTTPClient::GetInstance().PerformHTTPRequest(FStringUtils::Narrow(URL), FHTTPClient::EHttpRequestMethod::POST, MuteRequestBody,
		[ProductUserId, bIsMuted](FHTTPClient::HTTPErrorCode ErrorCode, const std::vector<char>& Data)
		{
			if (ErrorCode == 200)
			{
				std::string ResponseString(Data.data(), Data.size());

				if (ResponseString.find("error") != std::string::npos)
				{
					FDebugLog::LogError(L"Mute failed, Response: %ls", FStringUtils::Widen(ResponseString).c_str());
				}
				else
				{
					FDebugLog::Log(L"Mute success, User Id: %ls", ProductUserId.ToString().c_str());

					FGame::Get().GetVoice()->SetMemberRemoteMuteState(ProductUserId, bIsMuted);
				}
			}
			else
			{
				std::string ErrorString(Data.data(), Data.size());
				FDebugLog::LogError(L"Mute failed, HTTP Request failed, Code: %d, Error %ls", ErrorCode, FStringUtils::Widen(ErrorString).c_str());
			}
		});
}

void FVoice::KickMember(FProductUserId ProductUserId)
{
	FDebugLog::Log(L"Kick Member - Id: %ls", ProductUserId.ToString().c_str());

	// [Trusted Server]
	// Ask trusted server to kick user

	if (OwnerLock.empty())
	{
		FDebugLog::LogError(L"Kick failed, Owner lock is invalid");
		return;
	}

	std::wstring URL = FullTrustedServerURL;
	URL.append(L"/session/");
	URL.append(CurrentRoomName);
	URL.append(L"/kick/");
	URL.append(ProductUserId.ToString());
	FDebugLog::Log(L"Kick URL: %ls", URL.c_str());

	// Add owner lock to json request body
	rapidjson::Document Doc;
	Doc.SetObject();
	Doc.AddMember("lock", OwnerLock, Doc.GetAllocator());
	KickRequestBody = JsonDocToString(Doc);
	FDebugLog::Log(L"Kick Body: %ls", FStringUtils::Widen(KickRequestBody).c_str());

	FHTTPClient::GetInstance().PerformHTTPRequest(FStringUtils::Narrow(URL), FHTTPClient::EHttpRequestMethod::POST, KickRequestBody,
		[ProductUserId](FHTTPClient::HTTPErrorCode ErrorCode, const std::vector<char>& Data)
	{
		if (ErrorCode == 200)
		{
			std::string ResponseString(Data.data(), Data.size());

			if (ResponseString.find("error") != std::string::npos)
			{
				FDebugLog::LogError(L"Kick failed, Response: %ls", FStringUtils::Widen(ResponseString).c_str());
			}
			else
			{
				FDebugLog::Log(L"Kick success, User Id: %ls", ProductUserId.ToString().c_str());
			}
		}
		else
		{
			std::string ErrorString(Data.data(), Data.size());
			FDebugLog::LogError(L"Kick failed, HTTP Request failed, Code: %d, Error %ls", ErrorCode, FStringUtils::Widen(ErrorString).c_str());
		}
	});
}

void FVoice::HeartbeatVoiceSession(FProductUserId ProductUserId, const std::string& OwnerLock)
{
	FDebugLog::Log(L"Heartbeat VoiceSession - Id: %ls", ProductUserId.ToString().c_str());

	if (OwnerLock.empty())
	{
		FDebugLog::LogError(L"HeartbeatVoiceSession failed, Owner lock is invalid");
		return;
	}

	std::wstring URL = FullTrustedServerURL;
	URL.append(L"/session/");
	URL.append(CurrentRoomName);
	URL.append(L"/heartbeat");
	FDebugLog::Log(L"Heartbeat URL: %ls", URL.c_str());

	// Add owner lock to json request body
	rapidjson::Document Doc;
	Doc.SetObject();
	Doc.AddMember("lock", OwnerLock, Doc.GetAllocator());
	HeartbeatRequestBody = JsonDocToString(Doc);
	FDebugLog::Log(L"Heartbeat Body: %ls", FStringUtils::Widen(HeartbeatRequestBody).c_str());

	// Immediately push out timer, but may get corrected down in case of request failure to prevent multiple attempts in case of timeouts
	NextHeartbeat = std::chrono::steady_clock::now() + std::chrono::seconds(30);

	FHTTPClient::GetInstance().PerformHTTPRequest(FStringUtils::Narrow(URL), FHTTPClient::EHttpRequestMethod::POST, HeartbeatRequestBody,
		[ProductUserId, this](FHTTPClient::HTTPErrorCode ErrorCode, const std::vector<char>& Data)
	{
		if (ErrorCode == 200)
		{
			std::string ResponseString(Data.data(), Data.size());

			if (ResponseString.find("error") != std::string::npos)
			{
				// A failing heartbeat could mean either an invalid lock (403) or the session has disappeared on the backend (404).
				// The sample application doesn't handle this any further, but applications should handle these cases by recreating a new session.
				FDebugLog::LogError(L"Heartbeat failed, Response: %ls", FStringUtils::Widen(ResponseString).c_str());
			}
			else
			{
				FDebugLog::Log(L"Heartbeat success, User Id: %ls", ProductUserId.ToString().c_str());
			}
		}
		else
		{
			std::string ErrorString(Data.data(), Data.size());
			FDebugLog::LogError(L"Heartbeat failed, HTTP Request failed, Code: %d, Error %ls", ErrorCode, FStringUtils::Widen(ErrorString).c_str());

			// Try heartbeat again in a few seconds
			NextHeartbeat = std::chrono::steady_clock::now() + std::chrono::seconds(5);
		}
	});
}

void FVoice::UpdateAudioInputDevices()
{
	EOS_RTCAudio_QueryInputDevicesInformationOptions Options = {};
	Options.ApiVersion = EOS_RTCAUDIO_QUERYINPUTDEVICESINFORMATION_API_LATEST;
	EOS_RTCAudio_QueryInputDevicesInformation(RTCAudioHandle, &Options, NULL, OnInputDevicesInformationCb);
}

void FVoice::GetAudioInputDevices()
{
	AudioInputDeviceNames.clear();
	AudioInputDeviceIds.clear();

	EOS_RTCAudio_GetInputDevicesCountOptions Options = {};
	Options.ApiVersion = EOS_RTCAUDIO_GETINPUTDEVICESCOUNT_API_LATEST;
	uint32_t Count = EOS_RTCAudio_GetInputDevicesCount(RTCAudioHandle, &Options);

	std::wstring DefaultDeviceName;
	for (uint32_t Index = 0; Index < Count; Index++)
	{
		EOS_RTCAudio_InputDeviceInformation* AudioDeviceInfo;
		EOS_RTCAudio_CopyInputDeviceInformationByIndexOptions CopyByIndexOptions = {};
		CopyByIndexOptions.ApiVersion = EOS_RTCAUDIO_COPYINPUTDEVICEINFORMATIONBYINDEX_API_LATEST;
		CopyByIndexOptions.DeviceIndex = Index;

		if (EOS_RTCAudio_CopyInputDeviceInformationByIndex(RTCAudioHandle, &CopyByIndexOptions, &AudioDeviceInfo) == EOS_EResult::EOS_Success)
		{
			std::string DeviceName = AudioDeviceInfo->DeviceName;
			AudioInputDeviceNames.emplace_back(FStringUtils::Widen(DeviceName));

			std::string DeviceId = AudioDeviceInfo->DeviceId;
			AudioInputDeviceIds.emplace_back(FStringUtils::Widen(DeviceId));

			if (AudioDeviceInfo->bDefaultDevice == EOS_TRUE)
			{
				DefaultDeviceName = FStringUtils::Widen(DeviceName);
			}

			EOS_RTCAudio_InputDeviceInformation_Release(AudioDeviceInfo);
		}
	}

	FGameEvent Event(EGameEventType::AudioInputDevicesUpdated, DefaultDeviceName);
	FGame::Get().OnGameEvent(Event);
}

void FVoice::UpdateAudioOutputDevices()
{
	EOS_RTCAudio_QueryOutputDevicesInformationOptions Options = {};
	Options.ApiVersion = EOS_RTCAUDIO_QUERYOUTPUTDEVICESINFORMATION_API_LATEST;
	EOS_RTCAudio_QueryOutputDevicesInformation(RTCAudioHandle, &Options, NULL, OnOutputDevicesInformationCb);
}

void FVoice::GetAudioOutputDevices()
{
	AudioOutputDeviceNames.clear();
	AudioOutputDeviceIds.clear();

	EOS_RTCAudio_GetOutputDevicesCountOptions Options = {};
	Options.ApiVersion = EOS_RTCAUDIO_GETOUTPUTDEVICESCOUNT_API_LATEST;
	uint32_t Count = EOS_RTCAudio_GetOutputDevicesCount(RTCAudioHandle, &Options);

	std::wstring DefaultDeviceName;
	for (uint32_t Index = 0; Index < Count; Index++)
	{
		EOS_RTCAudio_CopyOutputDeviceInformationByIndexOptions CopyByIndexOptions = {};
		CopyByIndexOptions.ApiVersion = EOS_RTCAUDIO_COPYOUTPUTDEVICEINFORMATIONBYINDEX_API_LATEST;
		CopyByIndexOptions.DeviceIndex = Index;
		EOS_RTCAudio_OutputDeviceInformation* AudioDeviceInfo;
		if (EOS_RTCAudio_CopyOutputDeviceInformationByIndex(RTCAudioHandle, &CopyByIndexOptions, &AudioDeviceInfo) == EOS_EResult::EOS_Success)
		{
			std::string DeviceName = AudioDeviceInfo->DeviceName;
			AudioOutputDeviceNames.emplace_back(FStringUtils::Widen(DeviceName));

			std::string DeviceId = AudioDeviceInfo->DeviceId;
			AudioOutputDeviceIds.emplace_back(FStringUtils::Widen(DeviceId));

			if (AudioDeviceInfo->bDefaultDevice == EOS_TRUE)
			{
				DefaultDeviceName = FStringUtils::Widen(DeviceName);
			}

			EOS_RTCAudio_OutputDeviceInformation_Release(AudioDeviceInfo);
		}
	}

	FGameEvent Event(EGameEventType::AudioOutputDevicesUpdated, DefaultDeviceName);
	FGame::Get().OnGameEvent(Event);
}

void FVoice::QueryJoinRoom(FProductUserId ProductUserId, std::wstring InRoomName, std::wstring InClientBaseUrl, std::string InToken)
{
	std::string RoomNameStr = FStringUtils::Narrow(InRoomName);
	std::string ClientBaseUrlStr = FStringUtils::Narrow(InClientBaseUrl);

	EOS_RTC_JoinRoomOptions Options = {};
	Options.ApiVersion = EOS_RTC_JOINROOM_API_LATEST;
	Options.LocalUserId = ProductUserId;
	Options.RoomName = RoomNameStr.c_str();
	Options.ClientBaseUrl = ClientBaseUrlStr.c_str();
	Options.ParticipantToken = InToken.c_str();

	EOS_RTC_JoinRoom(RTCHandle, &Options, nullptr, OnJoinRoomCb);
}

void FVoice::QueryLeaveRoom(FProductUserId ProductUserId, std::wstring InRoomName)
{
	std::string RoomNameStr = FStringUtils::Narrow(InRoomName);

	EOS_RTC_LeaveRoomOptions Options = {};
	Options.ApiVersion = EOS_RTC_LEAVEROOM_API_LATEST;
	Options.LocalUserId = ProductUserId;
	Options.RoomName = RoomNameStr.c_str();

	EOS_RTC_LeaveRoom(RTCHandle, &Options, nullptr, OnLeaveRoomCb);
}

void FVoice::RequestJoinRoom(FProductUserId ProductUserId, std::wstring InRoomName)
{
	QueryJoinRoomToken(ProductUserId, InRoomName);
}

void FVoice::StartJoinRoom(FProductUserId ProductUserId, std::wstring InRoomName, std::wstring InClientBaseUrl, std::string InToken)
{
	QueryJoinRoom(ProductUserId, InRoomName, InClientBaseUrl, InToken);
}

void FVoice::JoinRoom(FProductUserId ProductUserId, std::wstring InRoomName)
{
	if (LocalProductUserId.IsValid())
	{
		auto Itr = RoomMembers.find(ProductUserId);
		if (Itr == RoomMembers.end())
		{
			PlayerPtr OtherPlayer = FPlayerManager::Get().GetPlayer(ProductUserId);
			if (OtherPlayer != nullptr)
			{
				FinalizeJoinRoom(OtherPlayer, ProductUserId, InRoomName);
			}
			else
			{
				// We need to get FEpicAccountId and DisplayName for new player before adding
				// them to the list of players in this room
				QueryPlayerInfo(LocalProductUserId, ProductUserId);
			}
		}
		else
		{
			FDebugLog::LogError(L"Trying to add duplicate player to room - Id: %ls", ProductUserId.ToString().c_str());
		}
	}
}

void FVoice::FinalizeJoinRoom(PlayerPtr Player, FProductUserId ProductUserId, std::wstring InRoomName)
{
	if (LocalProductUserId.IsValid())
	{
		FVoiceRoomMember VoiceRoomMember;
		VoiceRoomMember.Player = Player;
		VoiceRoomMember.bIsOwner = !OwnerLock.empty(); // Local client is the owner if they have the owner lock
		VoiceRoomMember.bIsLocal = LocalProductUserId == ProductUserId;
		VoiceRoomMember.Volume = 50.0f;
		RoomMembers.insert({ ProductUserId, VoiceRoomMember });

		FGameEvent Event(EGameEventType::RoomJoined, ProductUserId, InRoomName);
		FGame::Get().OnGameEvent(Event);

		if (VoiceRoomMember.bIsLocal)
		{
			CurrentRoomName = InRoomName;

			// Update Presence for local user
			std::string RoomNameStr = FStringUtils::Narrow(InRoomName);
			SetJoinInfo(RoomNameStr);

			SubscribeToRoomNotifications(InRoomName);

			NextHeartbeat = std::chrono::steady_clock::now();
		}

		FDebugLog::Log(L"Player joined room - Id: %ls, Room: %ls", ProductUserId.ToString().c_str(), InRoomName.c_str());
	}
}

void FVoice::QueryPlayerInfo(FProductUserId LocalUserId, FProductUserId OtherUserId)
{
	// Check to see if we already have info for the other player
	FEpicAccountId EpicUserId = FGame::Get().GetUsers()->GetAccountMapping(OtherUserId);
	if (EpicUserId.IsValid())
	{
		// Use latest display name we have for other player
		std::wstring DisplayName = FGame::Get().GetUsers()->GetDisplayName(EpicUserId);
		if (!DisplayName.empty())
		{
			FDebugLog::Log(L"QueryPlayerInfo - Display name: %ls, Id: %ls", DisplayName.c_str(), OtherUserId.ToString().c_str());

			PlayerPtr OtherPlayer = FPlayerManager::Get().GetPlayer(EpicUserId);
			if (OtherPlayer == nullptr)
			{
				// Add new player
				FPlayerManager::Get().Add(EpicUserId);
				FPlayerManager::Get().SetProductUserID(EpicUserId, OtherUserId);
			}

			// Join Room
			OtherPlayer = FPlayerManager::Get().GetPlayer(EpicUserId);
			FPlayerManager::Get().SetDisplayName(EpicUserId, DisplayName);
			FinalizeJoinRoom(OtherPlayer, OtherUserId, CurrentRoomName);
			return;
		}
	}

	FDebugLog::Log(L"QueryPlayerInfo - Querying for display name - Id: %ls", OtherUserId.ToString().c_str());

	// Query for display name since we don't have it yet
	ProductUserIdsToQuery.emplace_back(OtherUserId);
	FGame::Get().GetUsers()->QueryAccountMappings(LocalUserId, ProductUserIdsToQuery);
}

void FVoice::OnEpicAccountsMappingRetrieved()
{
	for (FProductUserId ProductUserId : ProductUserIdsToQuery)
	{
		FEpicAccountId EpicUserId = FGame::Get().GetUsers()->GetAccountMapping(ProductUserId);
		if (EpicUserId.IsValid())
		{
			PlayerPtr OtherPlayer = FPlayerManager::Get().GetPlayer(EpicUserId);
			if (OtherPlayer == nullptr)
			{
				// Add new player
				FPlayerManager::Get().Add(EpicUserId);
				FPlayerManager::Get().SetProductUserID(EpicUserId, ProductUserId);
			}

			// Join Room
			OtherPlayer = FPlayerManager::Get().GetPlayer(EpicUserId);
			FinalizeJoinRoom(OtherPlayer, ProductUserId, CurrentRoomName);

			// Query for display name which will get set after query
			FGame::Get().GetUsers()->QueryDisplayName(EpicUserId);

			auto Itr = std::find(ProductUserIdsToQuery.begin(), ProductUserIdsToQuery.end(), ProductUserId);
			if (Itr != ProductUserIdsToQuery.end())
			{
				ProductUserIdsToQuery.erase(Itr);
			}
		}
	}
}

void FVoice::OnEpicAccountDisplayNameRetrieved(FEpicAccountId EpicUserId, std::wstring DisplayName)
{
	FPlayerManager::Get().SetDisplayName(EpicUserId, DisplayName);

	FGameEvent Event(EGameEventType::RoomDataUpdated);
	FGame::Get().OnGameEvent(Event);
}

void FVoice::AddLocalUserRoomTokenData(std::shared_ptr<FVoiceUserRoomTokenData> UserRoomToken)
{
	CachedUserRoomTokenInfo.emplace_back(UserRoomToken);
}

void FVoice::JoinFromCachedRoomTokenData(std::wstring InRoomName, std::wstring ClientBaseUrl)
{
	for (std::shared_ptr<FVoiceUserRoomTokenData>& NextUserToken : CachedUserRoomTokenInfo)
	{
		StartJoinRoom(NextUserToken->ProductUserId, InRoomName, ClientBaseUrl, NextUserToken->Token);
	}
}

void FVoice::JoinRoomWithFriend(FEpicAccountId EpicUserId, FProductUserId ProductUserId, std::wstring DisplayName)
{
	if (LocalEpicUserId.IsValid() && LocalProductUserId.IsValid())
	{
		if (EpicUserId.IsValid() && ProductUserId.IsValid() && !DisplayName.empty())
		{
			PlayerPtr OtherPlayer = FPlayerManager::Get().GetPlayer(EpicUserId);
			if (OtherPlayer == nullptr)
			{
				// Add new player
				FPlayerManager::Get().Add(EpicUserId);
				FPlayerManager::Get().SetProductUserID(EpicUserId, ProductUserId);
				FPlayerManager::Get().SetDisplayName(EpicUserId, DisplayName);
				OtherPlayer = FPlayerManager::Get().GetPlayer(EpicUserId);
			}

			EOS_Presence_GetJoinInfoOptions GetJoinInfoOptions = {};
			GetJoinInfoOptions.ApiVersion = EOS_PRESENCE_GETJOININFO_API_LATEST;
			GetJoinInfoOptions.LocalUserId = LocalEpicUserId;
			GetJoinInfoOptions.TargetUserId = EpicUserId;

			char JoinInfoBuffer[EOS_PRESENCEMODIFICATION_JOININFO_MAX_LENGTH + 1];
			int32_t BufferLength = EOS_PRESENCEMODIFICATION_JOININFO_MAX_LENGTH;

			EOS_HPresence PresenceHandle = EOS_Platform_GetPresenceInterface(FPlatform::GetPlatformHandle());

			EOS_EResult GetJoinInfoResult = EOS_Presence_GetJoinInfo(PresenceHandle, &GetJoinInfoOptions, JoinInfoBuffer, &BufferLength);
			if (GetJoinInfoResult == EOS_EResult::EOS_Success)
			{
				std::string JoinInfoStr = JoinInfoBuffer;

				std::regex JoinInfoRegex(SampleJoinInfoRegex);
				std::smatch JoinInfoMatch;
				if (std::regex_match(JoinInfoStr, JoinInfoMatch, JoinInfoRegex))
				{
					if (JoinInfoMatch.size() == 2)
					{
						std::string RoomNameStr = JoinInfoMatch[1].str();
						if (!RoomNameStr.empty())
						{
							RequestJoinRoom(LocalProductUserId, FStringUtils::Widen(RoomNameStr));
						}
						else
						{
							FDebugLog::LogError(L"Voice (JoinRoomWithFriend): Invalid Room Id!");
						}
					}
				}
			}
			else
			{
				FDebugLog::LogError(L"Voice (JoinRoomWithFriend): error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(GetJoinInfoResult)).c_str());
			}
		}
	}
}

void FVoice::SetJoinInfo(const std::string& InRoomName)
{
	// Local buffer to hold the `JoinInfo` string if its used.
	char JoinInfoBuffer[EOS_PRESENCEMODIFICATION_JOININFO_MAX_LENGTH + 1];

	if (!LocalEpicUserId.IsValid())
	{
		FDebugLog::LogError(L"Voice - SetJoinInfo: Local user is invalid!");
		return;
	}

	EOS_HPresence PresenceHandle = EOS_Platform_GetPresenceInterface(FPlatform::GetPlatformHandle());

	EOS_Presence_CreatePresenceModificationOptions CreateModOpt = {};
	CreateModOpt.ApiVersion = EOS_PRESENCE_CREATEPRESENCEMODIFICATION_API_LATEST;
	CreateModOpt.LocalUserId = LocalEpicUserId;

	EOS_HPresenceModification PresenceModification;
	EOS_EResult Result = EOS_Presence_CreatePresenceModification(PresenceHandle, &CreateModOpt, &PresenceModification);
	if (Result != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"[EOS SDK] Create presence modification failed: %ls.", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
		return;
	}

	EOS_PresenceModification_SetJoinInfoOptions JoinOptions = {};
	JoinOptions.ApiVersion = EOS_PRESENCEMODIFICATION_SETJOININFO_API_LATEST;
	if (InRoomName.length() == 0)
	{
		// Clear the JoinInfo string if there is no local InRoomName.
		JoinOptions.JoinInfo = nullptr;
	}
	else
	{
		// Use the local InRoomName to build a JoinInfo string to share with friends.
		snprintf(JoinInfoBuffer, EOS_PRESENCEMODIFICATION_JOININFO_MAX_LENGTH, SampleJoinInfoFormat, InRoomName.c_str());
		JoinOptions.JoinInfo = JoinInfoBuffer;
	}
	EOS_PresenceModification_SetJoinInfo(PresenceModification, &JoinOptions);

	EOS_Presence_SetPresenceOptions SetOpt = {};
	SetOpt.ApiVersion = EOS_PRESENCE_SETPRESENCE_API_LATEST;
	SetOpt.LocalUserId = LocalEpicUserId;
	SetOpt.PresenceModificationHandle = PresenceModification;
	EOS_Presence_SetPresence(PresenceHandle, &SetOpt, nullptr, FVoice::OnSetPresenceCb);

	EOS_PresenceModification_Release(PresenceModification);
}

void FVoice::LeaveRoom(FProductUserId ProductUserId, std::wstring InRoomName)
{
	if (InRoomName == CurrentRoomName)
	{
		auto Itr = RoomMembers.find(ProductUserId);
		if (Itr != RoomMembers.end())
		{
			RoomMembers.erase(ProductUserId);

			if (LocalProductUserId == ProductUserId)
			{
				// Local player has left so clear out room
				UnsubscribeFromRoomNotifications();

				ClearRoomMembers();

				// Reset name & lock
				CurrentRoomName = L"";
				OwnerLock = "";

				CachedUserRoomTokenInfo.clear();
			}

			FGameEvent Event(EGameEventType::RoomLeft, ProductUserId);
			FGame::Get().OnGameEvent(Event);

			FDebugLog::Log(L"LeaveRoom - Player removed from room - Id: %ls", ProductUserId.ToString().c_str());
		}
	}
	else
	{
		FDebugLog::LogError(L"LeaveRoom - Room name not recognized: %ls", InRoomName.c_str());
	}
}

void FVoice::SetMemberSpeakingState(FProductUserId ProductUserId, bool bIsSpeaking)
{
	auto Itr = RoomMembers.find(ProductUserId);
	if (Itr != RoomMembers.end())
	{
		FDebugLog::Log(L"SetMemberSpeakingState - User: %ls, Speaking: %ld", ProductUserId.ToString().c_str(), bIsSpeaking ? 1 : 0);

		Itr->second.bIsSpeaking = bIsSpeaking;

		FGameEvent Event(EGameEventType::RoomDataUpdated);
		FGame::Get().OnGameEvent(Event);
	}
}

void FVoice::SetMemberMuteState(FProductUserId ProductUserId, bool bIsMuted)
{
	auto Itr = RoomMembers.find(ProductUserId);
	if (Itr != RoomMembers.end())
	{
		FDebugLog::Log(L"SetMemberMuteState - User: %ls, Mute: %ld", ProductUserId.ToString().c_str(), bIsMuted ? 1 : 0);

		Itr->second.bIsMuted = bIsMuted;

		FGameEvent Event(EGameEventType::RoomDataUpdated);
		FGame::Get().OnGameEvent(Event);
	}
}

void FVoice::SetMemberRemoteMuteState(FProductUserId ProductUserId, bool bIsMuted)
{
	auto Itr = RoomMembers.find(ProductUserId);
	if (Itr != RoomMembers.end())
	{
		FDebugLog::Log(L"SetMemberRemoteMuteState - User: %ls, Mute: %ld", ProductUserId.ToString().c_str(), bIsMuted ? 1 : 0);

		Itr->second.bIsRemoteMuted = bIsMuted;

		FGameEvent Event(EGameEventType::RoomDataUpdated);
		FGame::Get().OnGameEvent(Event);
	}
}

void FVoice::SetMemberVolume(FProductUserId ProductUserId, float Volume)
{
	auto Itr = RoomMembers.find(ProductUserId);
	if (Itr != RoomMembers.end())
	{
		FDebugLog::Log(L"SetMemberVolume - User: %ls, Volume: %f", ProductUserId.ToString().c_str(), Volume);

		Itr->second.Volume = Volume;

		FGameEvent Event(EGameEventType::RoomDataUpdated);
		FGame::Get().OnGameEvent(Event);
	}
}

void FVoice::SetAudioInputDeviceFromName(std::wstring DeviceName)
{
	for (uint32_t Index = 0; Index < static_cast<uint32_t>(AudioInputDeviceNames.size()); Index++)
	{
		if (AudioInputDeviceNames[Index] == DeviceName)
		{
			const std::wstring DeviceId = AudioInputDeviceIds[Index];
			FDebugLog::Log(L"Voice: setting input device to %ls : %ls", DeviceName.c_str(), DeviceId.c_str());
			SetAudioInputDevice(DeviceId);
			return;
		}
	}
}

void FVoice::SetAudioOutputDeviceFromName(std::wstring DeviceName)
{
	for (uint32_t Index = 0; Index < static_cast<uint32_t>(AudioOutputDeviceNames.size()); Index++)
	{
		if (AudioOutputDeviceNames[Index] == DeviceName)
		{
			const std::wstring DeviceId = AudioOutputDeviceIds[Index];
			FDebugLog::Log(L"Voice: setting output device to %ls : %ls", DeviceName.c_str(), DeviceId.c_str());
			SetAudioOutputDevice(DeviceId);
			return;
		}
	}
}

void FVoice::SetAudioInputDevice(const std::wstring& DeviceId)
{
	if (LocalProductUserId.IsValid())
	{
		std::string NarrowDeviceId = FStringUtils::Narrow(DeviceId);

		EOS_RTCAudio_SetInputDeviceSettingsOptions InputDeviceSettingsOptions = {};
		InputDeviceSettingsOptions.ApiVersion = EOS_RTCAUDIO_SETINPUTDEVICESETTINGS_API_LATEST;
		InputDeviceSettingsOptions.LocalUserId = LocalProductUserId;
		InputDeviceSettingsOptions.RealDeviceId = NarrowDeviceId.c_str();
		InputDeviceSettingsOptions.bPlatformAEC = EOS_FALSE;

		EOS_RTCAudio_SetInputDeviceSettings(RTCAudioHandle, &InputDeviceSettingsOptions, NULL, OnSetAudioInputDeviceCb);
	}
}

void FVoice::SetAudioOutputDevice(const std::wstring& DeviceId)
{
	if (LocalProductUserId.IsValid())
	{
		std::string NarrowDeviceId = FStringUtils::Narrow(DeviceId);

		EOS_RTCAudio_SetOutputDeviceSettingsOptions AudioOutputSettingsOptions = {};
		AudioOutputSettingsOptions.ApiVersion = EOS_RTCAUDIO_SETOUTPUTDEVICESETTINGS_API_LATEST;
		AudioOutputSettingsOptions.LocalUserId = LocalProductUserId;
		AudioOutputSettingsOptions.RealDeviceId = NarrowDeviceId.c_str();

		EOS_RTCAudio_SetOutputDeviceSettings(RTCAudioHandle, &AudioOutputSettingsOptions, NULL, OnSetAudioOutputDeviceCb);
	}
}

void FVoice::UpdateReceivingVolume(const std::wstring& InVolume)
{
	float Volume = 50;
	try
	{
		Volume = std::stof(InVolume);
	}
	catch (...)
	{
		return; // wrong format, out of range, etc
	}
	
	std::string NarrowRoomName = FStringUtils::Narrow(CurrentRoomName);

	EOS_RTCAudio_UpdateReceivingVolumeOptions ReceivingVolumeOptions{};

	ReceivingVolumeOptions.ApiVersion = EOS_RTCAUDIO_UPDATERECEIVINGVOLUME_API_LATEST;
	ReceivingVolumeOptions.LocalUserId = LocalProductUserId;
	ReceivingVolumeOptions.RoomName = NarrowRoomName.c_str();
	ReceivingVolumeOptions.Volume = Volume;

	EOS_RTCAudio_UpdateReceivingVolume(RTCAudioHandle, &ReceivingVolumeOptions, nullptr, OnUpdateReceivingVolumeCb);
}

void FVoice::UpdateParticipantVolume(FProductUserId ProductUserId, const std::wstring& InVolume)
{
	float Volume = 50;
	try
	{
		Volume = std::stof(InVolume);
	}
	catch (...)
	{
		return; // wrong format, out of range, etc
	}

	std::string NarrowRoomName = FStringUtils::Narrow(CurrentRoomName);

	EOS_RTCAudio_UpdateParticipantVolumeOptions ParticipantVolumeOptions{};

	ParticipantVolumeOptions.ApiVersion = EOS_RTCAUDIO_UPDATEPARTICIPANTVOLUME_API_LATEST;
	ParticipantVolumeOptions.LocalUserId = LocalProductUserId;
	ParticipantVolumeOptions.ParticipantId = ProductUserId;
	ParticipantVolumeOptions.RoomName = NarrowRoomName.c_str();
	ParticipantVolumeOptions.Volume = Volume;

	EOS_RTCAudio_UpdateParticipantVolume(RTCAudioHandle, &ParticipantVolumeOptions, nullptr, OnUpdateParticipantVolumeCb);
}


void FVoice::OnDisconnected(FProductUserId LocalProductUserId, std::wstring InRoomName)
{
	LeaveRoom(LocalProductUserId, InRoomName);
}

void FVoice::OnParticipantJoined(FProductUserId ProductUserId, std::wstring InRoomName)
{
	JoinRoom(ProductUserId, InRoomName);
}

void FVoice::OnParticipantLeft(FProductUserId ProductUserId, std::wstring InRoomName)
{
	LeaveRoom(ProductUserId, InRoomName);
}

void FVoice::OnParticipantAudioUpdated(FProductUserId ProductUserId, std::wstring InRoomName, EOS_ERTCAudioStatus InAudioStatus, bool bIsSpeaking)
{
	if (InRoomName == CurrentRoomName)
	{
		SetMemberSpeakingState(ProductUserId, bIsSpeaking);
		SetMemberMuteState(ProductUserId, InAudioStatus != EOS_ERTCAudioStatus::EOS_RTCAS_Enabled);
		SetMemberRemoteMuteState(ProductUserId, InAudioStatus == EOS_ERTCAudioStatus::EOS_RTCAS_AdminDisabled);
	}
}

void FVoice::OnAudioDevicesChanged()
{
	UpdateAudioInputDevices();
	UpdateAudioOutputDevices();
}

void EOS_CALL FVoice::OnJoinRoomCb(const EOS_RTC_JoinRoomCallbackInfo* Data)
{
	assert(Data != NULL);

	if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
	{
		// Operation is retrying so it is not complete yet
		return;
	}

	if (Data->ResultCode != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"[EOS SDK] Voice - Join Room Error: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		return;
	}

	std::string RoomNameStr = Data->RoomName;
	std::wstring WideRoomName = FStringUtils::Widen(RoomNameStr);

	FGame::Get().GetVoice()->JoinRoom(Data->LocalUserId, WideRoomName);
}

void EOS_CALL FVoice::OnLeaveRoomCb(const EOS_RTC_LeaveRoomCallbackInfo* Data)
{
	assert(Data != NULL);

	if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
	{
		// Operation is retrying so it is not complete yet
		return;
	}

	if (Data->ResultCode != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"[EOS SDK] Voice - Leave Room Error: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		return;
	}

	std::string RoomNameStr = Data->RoomName;
	std::wstring WideRoomName = FStringUtils::Widen(RoomNameStr);

	FGame::Get().GetVoice()->LeaveRoom(Data->LocalUserId, WideRoomName);
}

void EOS_CALL FVoice::OnSetPresenceCb(const EOS_Presence_SetPresenceCallbackInfo* Data)
{
	if (!Data)
	{
		FDebugLog::LogError(L"Voice (OnSetPresenceCallback): EOS_Presence_SetPresenceCallbackInfo is null");
	}
	else if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
	{
		// Operation is retrying so it is not complete yet
		return;
	}
	else if (Data->ResultCode != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"Voice (OnSetPresenceCallback): error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
	}
	else
	{
		FDebugLog::Log(L"Voice: set presence successfully.");
	}
}

void EOS_CALL FVoice::OnDisconnectedCb(const EOS_RTC_DisconnectedCallbackInfo* Data)
{
	if (Data)
	{
		FDebugLog::Log(L"Voice (OnDisconnectedCallback)");

		std::string RoomNameStr = Data->RoomName;

		FGame::Get().GetVoice()->OnDisconnected(Data->LocalUserId, FStringUtils::Widen(RoomNameStr));
	}
	else
	{
		FDebugLog::LogError(L"Voice (OnDisconnected): EOS_RTC_DisconnectedCallbackInfo is null");
	}
}

void EOS_CALL FVoice::OnParticipantStatusChangedCb(const EOS_RTC_ParticipantStatusChangedCallbackInfo* Data)
{
	if (Data)
	{
		FDebugLog::Log(L"Voice (OnParticipantStatusChangedCb)");

		std::string RoomNameStr = Data->RoomName;

		if (Data->ParticipantStatus == EOS_ERTCParticipantStatus::EOS_RTCPS_Joined)
		{
			FGame::Get().GetVoice()->OnParticipantJoined(Data->ParticipantId, FStringUtils::Widen(RoomNameStr));
		}
		else if (Data->ParticipantStatus == EOS_ERTCParticipantStatus::EOS_RTCPS_Left)
		{
			FGame::Get().GetVoice()->OnParticipantLeft(Data->ParticipantId, FStringUtils::Widen(RoomNameStr));
		}
		else
		{
			FDebugLog::LogError(L"Voice (OnParticipantStatusChangedCb): ParticipantStatus is invalid");
			assert(false);
		}
	}
	else
	{
		FDebugLog::LogError(L"Voice (OnParticipantStatusChangedCb): EOS_RTC_ParticipantStatusChangedCallbackInfo is null");
	}
}

void EOS_CALL FVoice::OnParticipantAudioUpdatedCb(const EOS_RTCAudio_ParticipantUpdatedCallbackInfo* Data)
{
	if (Data)
	{
		FDebugLog::Log(L"Voice (OnParticipantAudioUpdatedCb)");

		std::string RoomNameStr = Data->RoomName;

		FGame::Get().GetVoice()->OnParticipantAudioUpdated(Data->ParticipantId,
			FStringUtils::Widen(RoomNameStr),
			Data->AudioStatus,
			Data->bSpeaking == EOS_TRUE);
	}
	else
	{
		FDebugLog::LogError(L"Voice (OnParticipantAudioUpdatedCb): EOS_RTCAudio_ParticipantUpdatedCallbackInfo is null");
	}
}

void EOS_CALL FVoice::OnAudioDevicesChangedCb(const EOS_RTCAudio_AudioDevicesChangedCallbackInfo* Data)
{
	if (Data)
	{
		FDebugLog::Log(L"Voice (OnAudioDevicesChangedCb)");

		FGame::Get().GetVoice()->OnAudioDevicesChanged();
	}
	else
	{
		FDebugLog::LogError(L"Voice (OnAudioDevicesChangedCb): EOS_RTCAudio_AudioDevicesChangedCallbackInfo is null");
	}
}

void EOS_CALL FVoice::OnAudioUpdateSendingCb(const EOS_RTCAudio_UpdateSendingCallbackInfo* Data)
{
	if (Data)
	{
		FDebugLog::Log(L"Voice (OnAudioUpdateSendingCb)");
	}
	else
	{
		FDebugLog::LogError(L"Voice (OnAudioUpdateSendingCb): EOS_RTCAudio_UpdateSendingCallbackInfo is null");
	}
}

void EOS_CALL FVoice::OnAudioUpdateReceivingCb(const EOS_RTCAudio_UpdateReceivingCallbackInfo* Data)
{
	if (Data)
	{
		FDebugLog::Log(L"Voice (OnAudioUpdateReceivingCb)");
	}
	else
	{
		FDebugLog::LogError(L"Voice (OnAudioUpdateReceivingCb): EOS_RTCAudio_UpdateReceivingCallbackInfo is null");
	}
}

void EOS_CALL FVoice::OnUpdateReceivingVolumeCb(const EOS_RTCAudio_UpdateReceivingVolumeCallbackInfo* Data)
{
	if (Data)
	{
		if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
		{
			// Operation is retrying so it is not complete yet
			return;
		}
		else if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Voice (OnUpdateReceivingVolumeCb) - Error: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else
		{
			FDebugLog::Log(L"Voice (OnUpdateReceivingVolumeCb) Volume: %f", Data->Volume);
		}
	}
	else
	{
		FDebugLog::LogError(L"Voice (OnUpdateReceivingVolumeCb): EOS_RTCAudio_UpdateReceivingVolumeCallbackInfo is null");
	}
}

void EOS_CALL FVoice::OnUpdateParticipantVolumeCb(const EOS_RTCAudio_UpdateParticipantVolumeCallbackInfo* Data)
{
	if (Data)
	{
		if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
		{
			// Operation is retrying so it is not complete yet
			return;
		}
		else if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Voice (OnUpdateParticipantVolumeCb) - Error: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else
		{
			FDebugLog::Log(L"Voice (OnUpdateParticipantVolumeCb) Volume: %f", Data->Volume);
			FGame::Get().GetVoice()->SetMemberVolume(Data->ParticipantId, Data->Volume);
		}
	}
	else
	{
		FDebugLog::LogError(L"Voice (OnUpdateParticipantVolumeCb): EOS_RTCAudio_UpdateParticipantVolumeCallbackInfo is null");
	}
}

void EOS_CALL FVoice::OnInputDevicesInformationCb(const EOS_RTCAudio_OnQueryInputDevicesInformationCallbackInfo* Data)
{
	if (Data)
	{
		if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
		{
			// Operation is retrying so it is not complete yet
			return;
		}
		else if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Voice (OnInputDevicesInformationCb) - Error: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else
		{
			FGame::Get().GetVoice()->GetAudioInputDevices();
		}
	}
	else
	{
		FDebugLog::LogError(L"Voice (OnInputDevicesInformationCb): EOS_RTCAudio_OnQueryInputDevicesInformationCallbackInfo is null");
	}
}

void EOS_CALL FVoice::OnOutputDevicesInformationCb(const EOS_RTCAudio_OnQueryOutputDevicesInformationCallbackInfo* Data)
{
	if (Data)
	{
		if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
		{
			// Operation is retrying so it is not complete yet
			return;
		}
		else if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Voice (OnOutputDevicesInformationCb) - Error: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else
		{
			FGame::Get().GetVoice()->GetAudioOutputDevices();
		}
	}
	else
	{
		FDebugLog::LogError(L"Voice (OnOutputDevicesInformationCb): EOS_RTCAudio_OnQueryOutputDevicesInformationCallbackInfo is null");
	}
}

void EOS_CALL FVoice::OnSetAudioInputDeviceCb(const EOS_RTCAudio_OnSetInputDeviceSettingsCallbackInfo* Data)
{
	if (Data)
	{
		if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
		{
			// Operation is retrying so it is not complete yet
			return;
		}
		else if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"[EOS SDK] Voice - Failed to update audio input - Error: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else
		{
			FDebugLog::Log(L"Voice (OnSetAudioInputDeviceCb) - RealDeviceId: %ls", FStringUtils::Widen(Data->RealDeviceId).c_str());
		}
	}
	else
	{
		FDebugLog::LogError(L"Voice (OnSetAudioInputDeviceCb): EOS_RTCAudio_OnSetInputDeviceSettingsCallbackInfo is null");
	}
}

void EOS_CALL FVoice::OnSetAudioOutputDeviceCb(const EOS_RTCAudio_OnSetOutputDeviceSettingsCallbackInfo* Data)
{
	if (Data)
	{
		if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
		{
			// Operation is retrying so it is not complete yet
			return;
		}
		else if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"[EOS SDK] Voice - Failed to update audio output - Error: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else
		{
			FDebugLog::Log(L"Voice (OnSetAudioOutputDeviceCb) - RealDeviceId: %ls", FStringUtils::Widen(Data->RealDeviceId).c_str());
		}
	}
	else
	{
		FDebugLog::LogError(L"Voice (OnSetAudioOutputDeviceCb): EOS_RTCAudio_OnSetOutputDeviceSettingsCallbackInfo is null");
	}
}

