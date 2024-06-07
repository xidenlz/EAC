// Copyright Epic Games, Inc. All Rights Reserved.



#include "Game.h"
#include "GameEvent.h"
#include "Platform.h"
#include "AntiCheatNetworkTransport.h"
#include "AntiCheatClient.h"

#include "DebugLog.h"
#include "AccountHelpers.h"

#include <eos_anticheatclient.h>

FAntiCheatClient::FAntiCheatClient()
{
	FGame::Get().GetAntiCheatNetworkTransport()->SetOnNewMessageCallback([this](const void* Data, uint32_t Length)
	{
		OnMessageFromServerReceived(Data, Length);
	});
	FGame::Get().GetAntiCheatNetworkTransport()->SetOnClientActionRequiredCallback([this](EOS_AntiCheatCommon_OnClientActionRequiredCallbackInfo Message)
	{
		FDebugLog::Log(L"OnClientActionRequired: ClientAction=%d, ClientActionReasonCode=%d, ClientActionReasonDetails=%ls", Message.ClientAction, Message.ActionReasonCode, FStringUtils::Widen(Message.ActionReasonDetailsString).c_str());
		FGameEvent Event(EGameEventType::AntiCheatKicked);
		FGame::Get().OnGameEvent(Event);
	});
}

FAntiCheatClient::~FAntiCheatClient()
{

}

void FAntiCheatClient::Init()
{
	AntiCheatClientHandle = EOS_Platform_GetAntiCheatClientInterface(FPlatform::GetPlatformHandle());
	if (!AntiCheatClientHandle)
	{
		FDebugLog::LogWarning(L"Can't get a handle to the Anti-Cheat Client Interface. Please, start the game via Anti-Cheat bootstrapper.");
		return;
	}
}

bool FAntiCheatClient::Start(const std::string& Host, int Port, const FProductUserId& LocalUserId, const std::string& EOSConnectIdTokenJWT)
{
	if (!ConnectToAntiCheatServer(Host, Port))
	{
		FDebugLog::LogError(L"ConnectToAntiCheatServer Error");
		return false;
	}

	if (!AddNotifyMessageToServerCallback())
	{
		DisconnectFromAntiCheatServer();
		FDebugLog::LogError(L"AddNotifyMessageToServerCallback Error");
		return false;
	}

	if (!BeginSession(LocalUserId))
	{
		RemoveNotifyMessageToServerCallback();
		DisconnectFromAntiCheatServer();
		FDebugLog::LogError(L"BeginSession Error");
		return false;
	}

	SendRegistrationInfoToAntiCheatServer(LocalUserId, EOSConnectIdTokenJWT);
	bConnected = true;

	return true;
}

void FAntiCheatClient::Stop()
{
	if (bConnected)
	{
		RemoveNotifyMessageToServerCallback();
		EndSession();
		DisconnectFromAntiCheatServer();
		bConnected = false;
	}
}

void FAntiCheatClient::EndSession()
{
	EOS_AntiCheatClient_EndSessionOptions EndSessionOptions{};
	EndSessionOptions.ApiVersion = EOS_ANTICHEATCLIENT_ENDSESSION_API_LATEST;
	EOS_AntiCheatClient_EndSession(AntiCheatClientHandle, &EndSessionOptions);
}

void FAntiCheatClient::PollStatus()
{
	EOS_AntiCheatClient_PollStatusOptions Options = {};
	Options.ApiVersion = EOS_ANTICHEATCLIENT_POLLSTATUS_API_LATEST;
	Options.OutMessageLength = 256;

	EOS_EAntiCheatClientViolationType Type = {};
	std::array<char, 256> Message = {};

	const EOS_EResult Result = EOS_AntiCheatClient_PollStatus(AntiCheatClientHandle, &Options, &Type, &Message[0]);

	if (Result == EOS_EResult::EOS_NotFound)
	{
		FDebugLog::Log(L"EOS_AntiCheatClient_PollStatus: No violations found.");
	}
	else
	{
		FDebugLog::Log(L"EOS_AntiCheatClient_PollStatus: ViolationType=%d, Message=%ls", Type,  FStringUtils::Widen(Message.data()).c_str());
	}
}

void FAntiCheatClient::OnShutdown()
{
	Stop();
}

void FAntiCheatClient::OnMessageFromServerReceived(const void* Data, uint32_t DataLengthBytes) const
{
	EOS_AntiCheatClient_ReceiveMessageFromServerOptions ReceiveMessageFromServerOptions = {};

	ReceiveMessageFromServerOptions.ApiVersion = EOS_ANTICHEATCLIENT_RECEIVEMESSAGEFROMSERVER_API_LATEST;
	ReceiveMessageFromServerOptions.Data = Data;
	ReceiveMessageFromServerOptions.DataLengthBytes = DataLengthBytes;

	EOS_AntiCheatClient_ReceiveMessageFromServer(AntiCheatClientHandle, &ReceiveMessageFromServerOptions);
}

bool FAntiCheatClient::AddNotifyMessageToServerCallback()
{
	EOS_AntiCheatClient_AddNotifyMessageToServerOptions Options = {};
	Options.ApiVersion = EOS_ANTICHEATCLIENT_ADDNOTIFYMESSAGETOSERVER_API_LATEST;
	NotificationId = EOS_AntiCheatClient_AddNotifyMessageToServer(AntiCheatClientHandle, &Options, nullptr, OnMessageToServerCallback);

	return NotificationId != EOS_INVALID_NOTIFICATIONID;
}

void FAntiCheatClient::RemoveNotifyMessageToServerCallback()
{
	EOS_AntiCheatClient_RemoveNotifyMessageToServer(AntiCheatClientHandle, NotificationId);
}

bool FAntiCheatClient::BeginSession(const FProductUserId& LocalUserId)
{
	EOS_AntiCheatClient_BeginSessionOptions Options = {};
	Options.ApiVersion = EOS_ANTICHEATCLIENT_BEGINSESSION_API_LATEST;
	Options.LocalUserId = LocalUserId;
	Options.Mode = EOS_EAntiCheatClientMode::EOS_ACCM_ClientServer;

	const EOS_EResult Result = EOS_AntiCheatClient_BeginSession(AntiCheatClientHandle, &Options);

	return Result == EOS_EResult::EOS_Success;
}

bool FAntiCheatClient::ConnectToAntiCheatServer(const std::string& Host, int Port)
{
	const bool bIsConnected = FGame::Get().GetAntiCheatNetworkTransport()->Connect(Host.c_str(), Port);
	return bIsConnected;
}

void FAntiCheatClient::DisconnectFromAntiCheatServer()
{
	FGame::Get().GetAntiCheatNetworkTransport()->Disconnect();
}

void FAntiCheatClient::SendRegistrationInfoToAntiCheatServer(const FProductUserId& LocalUserId, const std::string& EOSConnectIdTokenJWT)
{
	FAntiCheatNetworkTransport::FRegistrationInfoMessage Message;

	Message.ProductUserId = FStringUtils::Narrow(LocalUserId.ToString());
	Message.EOSConnectIdTokenJWT = EOSConnectIdTokenJWT;
	Message.ClientPlatform = EOS_EAntiCheatCommonClientPlatform::EOS_ACCCP_Windows;

	FGame::Get().GetAntiCheatNetworkTransport()->Send(Message);
}

void FAntiCheatClient::OnMessageToServerCallback(const EOS_AntiCheatClient_OnMessageToServerCallbackInfo* Message)
{
	FGame::Get().GetAntiCheatNetworkTransport()->Send(Message);
}
