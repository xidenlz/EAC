// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#define WITHOUT_SDL
#include "SDL_net.h"

class FTCPClient
{
public:
	/**
	 * No default constructor for this class
	 */
	FTCPClient() = delete;

	/**
	 * Constructor
	 */
	FTCPClient(size_t MaxBufferSizeBytes);

	/**
	 * No copying or copy assignment allowed for this class.
	 */
	FTCPClient(FTCPClient const&) = delete;
	FTCPClient& operator=(FTCPClient const&) = delete;

	/**
	 * Destructor
	 */
	virtual ~FTCPClient();

	bool Connect(const char* Host, uint16_t Port);
	void Disconnect();

	void Send(const void* Data, size_t Length);
	void SetOnBufferReceivedCallback(std::function<void(char*, int)> Callback);

	void Update();

private:
	bool bIsConnected = false;

	TCPsocket Client = nullptr;
	SDLNet_SocketSet SocketSet;

	std::vector<char> Buffer;

	std::function<void(char*, int)> OnBufferReceivedCallback;
};

