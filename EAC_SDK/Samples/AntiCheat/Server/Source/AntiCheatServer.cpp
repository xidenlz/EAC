// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "AntiCheatServer.h"

#include "AntiCheatNetworkTransport.h"
#include "DebugLog.h"
#include "eos_connect.h"
#include "eos_anticheatserver.h"
#include "eos_anticheatserver_types.h"
#include "eos_sdk.h"
#include "TCPClient.h"

void FAntiCheatServer::Init(EOS_HPlatform Platform)
{
	ConnectHandle = EOS_Platform_GetConnectInterface(Platform);
	AntiCheatServerHandle = EOS_Platform_GetAntiCheatServerInterface(Platform);
}

void FAntiCheatServer::BeginSession()
{
	{
		EOS_AntiCheatServer_AddNotifyMessageToClientOptions Options = {};
		Options.ApiVersion = EOS_ANTICHEATSERVER_ADDNOTIFYMESSAGETOCLIENT_API_LATEST;
		MessageToClientId = EOS_AntiCheatServer_AddNotifyMessageToClient(AntiCheatServerHandle, &Options, this, OnMessageToClientCb);
	}

	{
		EOS_AntiCheatServer_AddNotifyClientActionRequiredOptions Options = {};
		Options.ApiVersion = EOS_ANTICHEATSERVER_ADDNOTIFYCLIENTACTIONREQUIRED_API_LATEST;
		ClientActionRequiredId = EOS_AntiCheatServer_AddNotifyClientActionRequired(AntiCheatServerHandle, &Options, this, OnClientActionRequiredCb);
	}

	{
		EOS_AntiCheatServer_BeginSessionOptions Options = {};
		Options.ApiVersion = EOS_ANTICHEATSERVER_BEGINSESSION_API_LATEST;
		Options.RegisterTimeoutSeconds = EOS_ANTICHEATSERVER_BEGINSESSION_MAX_REGISTERTIMEOUT;
		Options.ServerName = "Anti-Cheat Samples Server";
		Options.bEnableGameplayData = EOS_FALSE;
		Options.LocalUserId = nullptr;
		EOS_AntiCheatServer_BeginSession(AntiCheatServerHandle, &Options);
	}
}

void FAntiCheatServer::VerifyIdToken(EOS_ProductUserId ExpectedUserId, const char* ConnectIdTokenJWT, void* ClientData, const EOS_Connect_OnVerifyIdTokenCallback Callback)
{
	EOS_Connect_IdToken Token = {};
	Token.ApiVersion = EOS_CONNECT_IDTOKEN_API_LATEST;
	Token.ProductUserId = ExpectedUserId;
	Token.JsonWebToken = ConnectIdTokenJWT;

	EOS_Connect_VerifyIdTokenOptions Options = {};
	Options.ApiVersion = EOS_CONNECT_VERIFYIDTOKEN_API_LATEST;
	Options.IdToken = &Token;

	EOS_Connect_VerifyIdToken(ConnectHandle, &Options, ClientData, Callback);
}

void FAntiCheatServer::RegisterClient(void* ClientHandle, FAntiCheatNetworkTransport::FRegistrationInfoMessage Message)
{
	EOS_AntiCheatServer_RegisterClientOptions Options = {};
	Options.ApiVersion = EOS_ANTICHEATSERVER_REGISTERCLIENT_API_LATEST;
	Options.ClientHandle = ClientHandle;

	// The ClientPlatform value was validated using an EOS Connect ID token before this function is called.
	Options.ClientPlatform = Message.ClientPlatform;
	if (Message.ClientPlatform == EOS_EAntiCheatCommonClientPlatform::EOS_ACCCP_Windows ||
		Message.ClientPlatform == EOS_EAntiCheatCommonClientPlatform::EOS_ACCCP_Mac ||
		Message.ClientPlatform == EOS_EAntiCheatCommonClientPlatform::EOS_ACCCP_Linux)
	{
		Options.ClientType = EOS_EAntiCheatCommonClientType::EOS_ACCCT_ProtectedClient;
	}
	else
	{
		Options.ClientType = EOS_EAntiCheatCommonClientType::EOS_ACCCT_UnprotectedClient;
	}
	
	// The ProductUserId value was validated using an EOS Connect ID token before this function is called.
	Options.UserId = EOS_ProductUserId_FromString(Message.ProductUserId);

	EOS_AntiCheatServer_RegisterClient(AntiCheatServerHandle, &Options);
}

void FAntiCheatServer::UnregisterClient(void* ClientHandle)
{
	EOS_AntiCheatServer_UnregisterClientOptions Options = {};
	Options.ApiVersion = EOS_ANTICHEATSERVER_UNREGISTERCLIENT_API_LATEST;
	Options.ClientHandle = ClientHandle;

	EOS_AntiCheatServer_UnregisterClient(AntiCheatServerHandle, &Options);
}

void FAntiCheatServer::EndSession()
{
	EOS_AntiCheatServer_RemoveNotifyMessageToClient(AntiCheatServerHandle, MessageToClientId);
	EOS_AntiCheatServer_RemoveNotifyClientActionRequired(AntiCheatServerHandle, ClientActionRequiredId);

	EOS_AntiCheatServer_EndSessionOptions Options = {};
	Options.ApiVersion = EOS_ANTICHEATSERVER_ENDSESSION_API_LATEST;
	EOS_AntiCheatServer_EndSession(AntiCheatServerHandle, &Options);
}

void FAntiCheatServer::OnMessageFromClientReceived(void* ClientHandle, const void* Data, uint32_t DataLengthBytes)
{
	EOS_AntiCheatServer_ReceiveMessageFromClientOptions Options = {};

	Options.ApiVersion = EOS_ANTICHEATSERVER_RECEIVEMESSAGEFROMCLIENT_API_LATEST;
	Options.ClientHandle = ClientHandle;
	Options.Data = Data;
	Options.DataLengthBytes = DataLengthBytes;

	EOS_AntiCheatServer_ReceiveMessageFromClient(AntiCheatServerHandle, &Options);
}

void FAntiCheatServer::OnMessageToClientCb(const EOS_AntiCheatCommon_OnMessageToClientCallbackInfo* Message)
{
	FAntiCheatNetworkTransport::GetInstance().Send(Message);
}

void FAntiCheatServer::OnClientActionRequiredCb(const EOS_AntiCheatCommon_OnClientActionRequiredCallbackInfo* Message)
{
	FAntiCheatNetworkTransport::GetInstance().Send(Message);
	FAntiCheatNetworkTransport::GetInstance().CloseClientConnection(Message->ClientHandle);
}
