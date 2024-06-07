// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#define WITHOUT_SDL
#include "SDL_net.h"

#include <vector>
#include <functional>

class FTCPClient final
{
public:
	/**
	 * No default constructor for this class
	 */
	FTCPClient() = delete;

	/**
	 * Constructor
	 */
	FTCPClient(int MaxSockets, size_t MaxBufferSizeBytes);

	/**
	 * No copying or copy assignment allowed for this class.
	 */
	FTCPClient(FTCPClient const&) = delete;
	FTCPClient& operator=(FTCPClient const&) = delete;

	/**
	 * Destructor
	 */
	virtual ~FTCPClient();

	void Open(uint16_t Port);
	void Send(void* To, const void* Data, size_t Length);

	using FOnBufferReceivedCallback = std::function<void(void*, char*, int)>;
	void SetOnBufferReceivedCallback(FOnBufferReceivedCallback Callback);

	using FOnClientDisconnectedCallback = std::function<void(void*)>;
	void SetOnClientDisconnectedCallback(FOnClientDisconnectedCallback Callback);

	void Update();

private:
	friend class FAntiCheatNetworkTransport;
	void OpenNewClientConnection();
	void CloseClientConnection(TCPsocket ClientSocket);

	void CloseServerConnection() const;

private:
	TCPsocket ServerSocket = nullptr;
	std::vector<TCPsocket> ClientsSockets;

	SDLNet_SocketSet SocketSet;

	std::vector<char> Buffer;

	FOnBufferReceivedCallback OnBufferReceivedCallback;
	FOnClientDisconnectedCallback OnClientDisconnectedCallback;
};

