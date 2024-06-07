// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "eos_anticheatserver_types.h"
#include "TCPClient.h"

#include <cstring>
#include <type_traits>

class FAntiCheatNetworkTransport
{
public:
	struct FRegistrationInfoMessage
	{
		const char* ProductUserId = nullptr;
		const char* EOSConnectIdTokenJWT = nullptr;
		EOS_EAntiCheatCommonClientPlatform ClientPlatform = EOS_EAntiCheatCommonClientPlatform::EOS_ACCCP_Unknown;
	};

	static FAntiCheatNetworkTransport& GetInstance();

	using FOnNewMessageCallback = std::function<void(void*, const void*, uint32_t)>;
	void SetOnNewMessageCallback(FOnNewMessageCallback Callback);

	using FOnNewClientCallback = std::function<void(void*, FRegistrationInfoMessage)>;
	void SetOnNewClientCallback(FOnNewClientCallback Callback);

	using FOnClientDisconnectedCallback = std::function<void(void*)>;
	void SetOnClientDisconnectedCallback(FOnClientDisconnectedCallback Callback);

	void Update();

	void Send(const EOS_AntiCheatCommon_OnClientActionRequiredCallbackInfo* Message);
	void Send(const EOS_AntiCheatCommon_OnMessageToClientCallbackInfo* Message);

	void CloseClientConnection(void* ClientHandle);

private:
	FAntiCheatNetworkTransport();

	void Receive(void* From, char* Buffer, size_t Length);
	size_t ProcessMessage(void* From, char* Buffer, size_t StartPosition);

	template<typename T, typename = std::enable_if_t<!std::is_pointer<T>::value>>
	void Write(T ObjectToWrite, char* Buffer, size_t& Position)
	{
		memcpy(&Buffer[Position], &ObjectToWrite, sizeof(ObjectToWrite));
		Position += sizeof(ObjectToWrite);
	}

	template<typename T, typename = std::enable_if_t<std::is_pointer<T>::value>>
	void Write(T ObjectToWrite, size_t Size, char* Buffer, size_t& Position)
	{
		memcpy(&Buffer[Position], ObjectToWrite, Size);
		Position += Size;
	}

	template<typename T, typename = std::enable_if_t<!std::is_pointer<T>::value>>
	T Read(char* Buffer, size_t& StartingPosition)
	{
		T ObjectToRead;
		memcpy(&ObjectToRead, &Buffer[StartingPosition], sizeof(ObjectToRead));
		StartingPosition += sizeof(ObjectToRead);
		return ObjectToRead;
	}

	template<typename T, typename = std::enable_if_t<std::is_pointer<T>::value>>
	T Read(char* Buffer, size_t BytesToRead, size_t& StartingPosition)
	{
		T Result = &Buffer[StartingPosition];
		StartingPosition += BytesToRead;
		return Result;
	}

private:
	enum class FMessageType : char
	{
		Opaque = 1,
		RegistrationInfo = 2,
		ClientActionRequired = 3
	};

	FTCPClient TCPClient;

	FOnNewMessageCallback OnNewMessageCallback;
	FOnNewClientCallback OnNewClientCallback;
	FOnClientDisconnectedCallback OnClientDisconnectedCallback;
};

