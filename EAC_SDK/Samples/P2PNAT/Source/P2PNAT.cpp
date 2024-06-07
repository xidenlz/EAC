// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "DebugLog.h"
#include "StringUtils.h"
#include "AccountHelpers.h"
#include "Game.h"
#include "GameEvent.h"
#include "Main.h"
#include "Platform.h"
#include "Users.h"
#include "Player.h"
#include "P2PNAT.h"
#include "P2PNATDialog.h"
#include "Menu.h"

#if !defined(_WIN32)
#define strncpy_s strncpy
#endif


FP2PNAT::FP2PNAT()
{
	
}

FP2PNAT::~FP2PNAT()
{

}

void FP2PNAT::Update()
{
	HandleReceivedMessages();
}

void FP2PNAT::RefreshNATType()
{
	EOS_HP2P P2PHandle = EOS_Platform_GetP2PInterface(FPlatform::GetPlatformHandle());

	EOS_P2P_QueryNATTypeOptions Options = {};
	Options.ApiVersion = EOS_P2P_QUERYNATTYPE_API_LATEST;

	EOS_P2P_QueryNATType(P2PHandle, &Options, nullptr, OnRefreshNATTypeFinished);
}

EOS_ENATType FP2PNAT::GetNATType() const
{
	EOS_ENATType NATType = EOS_ENATType::EOS_NAT_Unknown;
	EOS_HP2P P2PHandle = EOS_Platform_GetP2PInterface(FPlatform::GetPlatformHandle());

	EOS_P2P_GetNATTypeOptions Options = {};
	Options.ApiVersion = EOS_P2P_GETNATTYPE_API_LATEST;

	EOS_EResult Result = EOS_P2P_GetNATType(P2PHandle, &Options, &NATType);

	if (Result == EOS_EResult::EOS_NotFound)
	{
		//NAT type has not been queried yet. (Or query is not finished)		
		return EOS_ENATType::EOS_NAT_Unknown;
	}

	if (Result != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"EOS P2PNAT GetNatType: error while retrieving NAT Type: %ls", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
		return EOS_ENATType::EOS_NAT_Unknown;
	}

	return NATType;
}

void FP2PNAT::OnUserConnectLoggedIn(FProductUserId UserId)
{
	RefreshNATType();

	//subscribe to connection requests
	SubscribeToConnectionRequests();

	//subscribe to connection established
	SubscribeToConnectionEstablished();
}

void FP2PNAT::OnLoggedOut(FEpicAccountId UserId)
{
	UnsubscribeFromConnectionRequests();
	UnsubscribeToConnectionEstablished();
}

void EOS_CALL FP2PNAT::OnRefreshNATTypeFinished(const EOS_P2P_OnQueryNATTypeCompleteInfo* Data)
{
	if (Data)
	{
		if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
		{
			// Operation is retrying so it is not complete yet
			return;
		}

		if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"EOS P2PNAT QueryNATType callback: error code %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
	}
	else
	{
		FDebugLog::LogError(L"EOS P2PNAT QueryNATType callback: bad data returned");
	}
}

void FP2PNAT::OnGameEvent(const FGameEvent& Event)
{
	if (Event.GetType() == EGameEventType::UserConnectLoggedIn)
	{
		FProductUserId UserId = Event.GetProductUserId();
		OnUserConnectLoggedIn(UserId);
	}
	else if (Event.GetType() == EGameEventType::UserLoggedOut)
	{
		FEpicAccountId UserId = Event.GetUserId();
		OnLoggedOut(UserId);
	}
}

void FP2PNAT::SendMessage(FProductUserId FriendId, const std::wstring& Message)
{
	if (!FriendId.IsValid() || Message.empty())
	{
		FDebugLog::LogError(L"EOS P2PNAT SendMessage: bad input data (account id is wrong or message is empty).");
		return;
	}

	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player == nullptr)
	{
		FDebugLog::LogError(L"EOS P2PNAT SendMessage: error user not logged in.");
		return;
	}

	EOS_HP2P P2PHandle = EOS_Platform_GetP2PInterface(FPlatform::GetPlatformHandle());

	EOS_P2P_SocketId SocketId = {};
	SocketId.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
	strncpy_s(SocketId.SocketName, "CHAT", 5);

	EOS_P2P_SendPacketOptions Options = {};
	Options.ApiVersion = EOS_P2P_SENDPACKET_API_LATEST;
	Options.LocalUserId = Player->GetProductUserID();
	Options.RemoteUserId = FriendId;
	Options.SocketId = &SocketId;
	Options.bAllowDelayedDelivery = EOS_TRUE;
	Options.Channel = 0;
	Options.Reliability = EOS_EPacketReliability::EOS_PR_ReliableOrdered;
	Options.bDisableAutoAcceptConnection = EOS_FALSE;

	std::string MessageNarrow = FStringUtils::Narrow(Message);

	Options.DataLengthBytes = static_cast<uint32_t>(MessageNarrow.size());
	Options.Data = MessageNarrow.data();

	EOS_EResult Result = EOS_P2P_SendPacket(P2PHandle, &Options);
	if (Result != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"EOS P2PNAT SendMessage: error while sending data, code: %ls.", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
	}
}

void FP2PNAT::HandleReceivedMessages()
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player == nullptr || !Player->GetProductUserID().IsValid())
	{
		return;
	}

	EOS_HP2P P2PHandle = EOS_Platform_GetP2PInterface(FPlatform::GetPlatformHandle());

	EOS_P2P_ReceivePacketOptions Options = {};
	Options.ApiVersion = EOS_P2P_RECEIVEPACKET_API_LATEST;
	Options.LocalUserId = Player->GetProductUserID();
	Options.MaxDataSizeBytes = 4096;
	Options.RequestedChannel = nullptr;

	//Packet params
	FProductUserId FriendId;

	EOS_P2P_SocketId SocketId = {};
	SocketId.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
	uint8_t Channel = 0;

	std::vector<char> MessageData;
	MessageData.resize(Options.MaxDataSizeBytes);
	uint32_t BytesWritten = 0;

	EOS_EResult Result = EOS_P2P_ReceivePacket(P2PHandle, &Options, &FriendId.AccountId, &SocketId, &Channel, MessageData.data(), &BytesWritten);
	if (Result == EOS_EResult::EOS_NotFound)
	{
		//no more packets, just end
		return;
	}
	else if (Result == EOS_EResult::EOS_Success)
	{
		std::shared_ptr<FP2PNATDialog> P2PDialog = static_cast<FMenu&>(*FGame::Get().GetMenu()).GetP2PNATDialog();
		if (P2PDialog)
		{
			std::wstring MessageWide;
			std::string MessageNarrow(MessageData.data(), BytesWritten);
			MessageWide = FStringUtils::Widen(MessageNarrow);
			P2PDialog->OnMessageReceived(MessageWide, FriendId);
		}
	}
	else
	{
		FDebugLog::LogError(L"EOS P2PNAT HandleReceivedMessages: error while reading data, code: %ls.", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
	}
}

void FP2PNAT::SubscribeToConnectionRequests()
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player == nullptr || !Player->GetProductUserID().IsValid())
	{
		FDebugLog::LogError(L"EOS P2PNAT SubscribeToConnectionRequests: error user not logged in.");
		return;
	}

	if (ConnectionNotificationId == EOS_INVALID_NOTIFICATIONID)
	{
		EOS_HP2P P2PHandle = EOS_Platform_GetP2PInterface(FPlatform::GetPlatformHandle());

		EOS_P2P_SocketId SocketId = {};
		SocketId.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
		strncpy_s(SocketId.SocketName, "CHAT", 5);

		EOS_P2P_AddNotifyPeerConnectionRequestOptions Options = {};
		Options.ApiVersion = EOS_P2P_ADDNOTIFYPEERCONNECTIONREQUEST_API_LATEST;
		Options.LocalUserId = Player->GetProductUserID();
		Options.SocketId = &SocketId;

		ConnectionNotificationId = EOS_P2P_AddNotifyPeerConnectionRequest(P2PHandle, &Options, nullptr, OnIncomingConnectionRequest);
		if (ConnectionNotificationId == EOS_INVALID_NOTIFICATIONID)
		{
			FDebugLog::LogError(L"EOS P2PNAT SubscribeToConnectionRequests: could not subscribe, bad notification id returned.");
		}
	}
}

void FP2PNAT::UnsubscribeFromConnectionRequests()
{
	EOS_HP2P P2PHandle = EOS_Platform_GetP2PInterface(FPlatform::GetPlatformHandle());
	EOS_P2P_RemoveNotifyPeerConnectionRequest(P2PHandle, ConnectionNotificationId);
	ConnectionNotificationId = EOS_INVALID_NOTIFICATIONID;
}

void FP2PNAT::SubscribeToConnectionEstablished()
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player == nullptr || !Player->GetProductUserID().IsValid())
	{
		FDebugLog::LogError(L"EOS P2PNAT SubscribeToConnectionEstablished: error user not logged in.");
		return;
	}

	if (ConnectionEstablishedNotificationId == EOS_INVALID_NOTIFICATIONID)
	{
		EOS_HP2P P2PHandle = EOS_Platform_GetP2PInterface(FPlatform::GetPlatformHandle());

		EOS_P2P_SocketId SocketId = {};
		SocketId.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
		strncpy_s(SocketId.SocketName, "CHAT", 5);

		EOS_P2P_AddNotifyPeerConnectionEstablishedOptions Options = {};
		Options.ApiVersion = EOS_P2P_ADDNOTIFYPEERCONNECTIONESTABLISHED_API_LATEST;
		Options.LocalUserId = Player->GetProductUserID();
		Options.SocketId = &SocketId;

		ConnectionEstablishedNotificationId = EOS_P2P_AddNotifyPeerConnectionEstablished(P2PHandle, &Options, nullptr, OnIncomingConnectionEstablished);
		if (ConnectionEstablishedNotificationId == EOS_INVALID_NOTIFICATIONID)
		{
			FDebugLog::LogError(L"EOS P2PNAT SubscribeToConnectionEstablished: could not subscribe, bad notification id returned.");
		}
	}
}

void FP2PNAT::UnsubscribeToConnectionEstablished()
{
	EOS_HP2P P2PHandle = EOS_Platform_GetP2PInterface(FPlatform::GetPlatformHandle());
	EOS_P2P_RemoveNotifyPeerConnectionEstablished(P2PHandle, ConnectionEstablishedNotificationId);
	ConnectionEstablishedNotificationId = EOS_INVALID_NOTIFICATIONID;
}

void EOS_CALL FP2PNAT::OnIncomingConnectionRequest(const EOS_P2P_OnIncomingConnectionRequestInfo* Data)
{
	if (Data)
	{
		std::string SocketName = Data->SocketId->SocketName;
		if (SocketName != "CHAT")
		{
			FDebugLog::LogError(L"EOS P2PNAT OnIncomingConnectionRequest: bad socket id.");
			return;
		}

		PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
		if (Player == nullptr || !Player->GetProductUserID().IsValid())
		{
			FDebugLog::LogError(L"EOS P2PNAT OnIncomingConnectionRequest: error user not logged in.");
			return;
		}
		
		EOS_HP2P P2PHandle = EOS_Platform_GetP2PInterface(FPlatform::GetPlatformHandle());
		EOS_P2P_AcceptConnectionOptions Options = {};
		Options.ApiVersion = EOS_P2P_ACCEPTCONNECTION_API_LATEST;
		Options.LocalUserId = Player->GetProductUserID();
		Options.RemoteUserId = Data->RemoteUserId;

		EOS_P2P_SocketId SocketId = {};
		SocketId.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
		strncpy_s(SocketId.SocketName, "CHAT", 5);
		Options.SocketId = &SocketId;

		EOS_EResult Result = EOS_P2P_AcceptConnection(P2PHandle, &Options);
		if (Result != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"EOS P2PNAT OnIncomingConnectionRequest: error while accepting connection, code: %ls.", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
		}
	}
	else
	{
		FDebugLog::LogError(L"EOS P2PNAT SubscribeToConnectionRequests: EOS_P2P_OnIncomingConnectionRequestInfo is NULL.");
	}
}

void EOS_CALL FP2PNAT::OnIncomingConnectionEstablished(const EOS_P2P_OnPeerConnectionEstablishedInfo* Data)
{
	if (Data)
	{
		std::string SocketName = Data->SocketId->SocketName;
		if (SocketName != "CHAT")
		{
			FDebugLog::LogError(L"EOS P2PNAT OnIncomingConnectionEstablished: bad socket id.");
			return;
		}

		FDebugLog::Log(L"EOS P2PNAT OnIncomingConnectionEstablished: connection established. ConnectionType=[%ls]", Data->ConnectionType == EOS_EConnectionEstablishedType::EOS_CET_NewConnection ? L"NewConnection" : L"Reconnection");
	}
	else
	{
		FDebugLog::LogError(L"EOS P2PNAT SubscribeToConnectionEstablished: EOS_P2P_OnPeerConnectionEstablishedInfo is NULL.");
	}
}
