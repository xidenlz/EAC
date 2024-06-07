// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "TCPClient.h"
#include "DebugLog.h"

FTCPClient::FTCPClient(size_t MaxBufferSizeBytes)
{
	SDLNet_Init();
	SocketSet = SDLNet_AllocSocketSet(1);

	Buffer.resize(MaxBufferSizeBytes);
}

FTCPClient::~FTCPClient()
{
	Disconnect();
	SDLNet_Quit();
}

bool FTCPClient::Connect(const char* Host, uint16_t Port)
{
	IPaddress IP;
	SDLNet_ResolveHost(&IP, Host, Port);
	
	Client = SDLNet_TCP_Open(&IP);
	if (!Client)
	{
		FDebugLog::LogError(L"Connect to server failed");
		return false;
	}
	SDLNet_TCP_AddSocket(SocketSet, Client);

	bIsConnected = true;
	return bIsConnected;
}

void FTCPClient::Disconnect()
{
	if (!bIsConnected)
	{
		return;
	}

	SDLNet_TCP_DelSocket(SocketSet, Client);
	SDLNet_TCP_Close(Client);

	Client = nullptr;
	bIsConnected = false;
	FDebugLog::LogWarning(L"Disconnected from server");
}

void FTCPClient::Send(const void* Data, size_t Length)
{
	if (!bIsConnected)
	{
		return;
	}

	SDLNet_TCP_Send(Client, Data, static_cast<int>(Length));
}

void FTCPClient::SetOnBufferReceivedCallback(std::function<void(char*, int)> Callback)
{
	OnBufferReceivedCallback = std::move(Callback);
}

void FTCPClient::Update()
{
	if (!bIsConnected)
	{
		return;
	}
	
	SDLNet_CheckSockets(SocketSet, 0);
	
	if (SDLNet_SocketReady(Client))
	{
		const int ReceivedBufferLength = SDLNet_TCP_Recv(Client, &Buffer[0], static_cast<int>(Buffer.size()));
		if (ReceivedBufferLength > 0)
		{
			OnBufferReceivedCallback(&Buffer[0], ReceivedBufferLength);
		}
		else
		{
			Disconnect();
		}
	}
}