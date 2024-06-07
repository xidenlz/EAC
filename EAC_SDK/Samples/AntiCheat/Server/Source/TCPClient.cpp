// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "TCPClient.h"
#include "DebugLog.h"

FTCPClient::FTCPClient(int MaxSockets, size_t MaxBufferSizeBytes)
{
	SDLNet_Init();

	ClientsSockets.reserve(MaxSockets);
	SocketSet = SDLNet_AllocSocketSet(MaxSockets + 1); // +1 for the Server socket

	Buffer.resize(MaxBufferSizeBytes);
}

FTCPClient::~FTCPClient()
{
	CloseServerConnection();
	for (TCPsocket ClientSocket : ClientsSockets)
	{
		CloseClientConnection(ClientSocket);
	}

	SDLNet_FreeSocketSet(SocketSet);
	SDLNet_Quit();
}

void FTCPClient::Open(uint16_t Port)
{
	IPaddress IP;
	SDLNet_ResolveHost(&IP, nullptr, Port);
	
	ServerSocket = SDLNet_TCP_Open(&IP);
	SDLNet_TCP_AddSocket(SocketSet, ServerSocket);
}

void FTCPClient::Send(void* To, const void* Data, size_t Length)
{
	SDLNet_TCP_Send(reinterpret_cast<TCPsocket>(To), Data, static_cast<int>(Length));
}

void FTCPClient::SetOnClientDisconnectedCallback(FOnClientDisconnectedCallback Callback)
{
	OnClientDisconnectedCallback = std::move(Callback);
}

void FTCPClient::SetOnBufferReceivedCallback(FOnBufferReceivedCallback Callback)
{
	OnBufferReceivedCallback = std::move(Callback);
}

void FTCPClient::OpenNewClientConnection()
{
	TCPsocket NewlyConnectedClientSocket = SDLNet_TCP_Accept(ServerSocket);

	SDLNet_TCP_AddSocket(SocketSet, NewlyConnectedClientSocket);
	ClientsSockets.push_back(NewlyConnectedClientSocket);
}

void FTCPClient::CloseClientConnection(TCPsocket Socket)
{
	OnClientDisconnectedCallback(Socket);

	ClientsSockets.erase(std::find(ClientsSockets.begin(), ClientsSockets.end(), Socket));
	SDLNet_TCP_DelSocket(SocketSet, Socket);

	SDLNet_TCP_Close(Socket);
}

void FTCPClient::CloseServerConnection() const
{
	if (ServerSocket)
	{
		SDLNet_TCP_DelSocket(SocketSet, ServerSocket);
		SDLNet_TCP_Close(ServerSocket);
	}
}

void FTCPClient::Update()
{
	SDLNet_CheckSockets(SocketSet, 0);

	for (TCPsocket ClientSocket : ClientsSockets)
	{
		if (SDLNet_SocketReady(ClientSocket))
		{
			const int ReceivedBufferLength = SDLNet_TCP_Recv(ClientSocket, &Buffer[0], static_cast<int>(Buffer.size()));
			if (ReceivedBufferLength > 0)
			{
				OnBufferReceivedCallback(ClientSocket, &Buffer[0], static_cast<size_t>(ReceivedBufferLength));
			}
			else
			{
				CloseClientConnection(ClientSocket);
			}
		}
	}
	if (SDLNet_SocketReady(ServerSocket))
	{
		OpenNewClientConnection();
	}
	
}
