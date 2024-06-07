// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TCPClient.h"

#include <eos_anticheatclient_types.h>

class FAntiCheatNetworkTransport
{
public:
	struct FRegistrationInfoMessage
	{
		std::string ProductUserId;
		std::string EOSConnectIdTokenJWT;
		EOS_EAntiCheatCommonClientPlatform ClientPlatform = EOS_EAntiCheatCommonClientPlatform::EOS_ACCCP_Unknown;
	};

	/**
	* Constructor
	*/
	FAntiCheatNetworkTransport() noexcept(false);

	/**
	* No copying or copy assignment allowed for this class.
	*/
	FAntiCheatNetworkTransport(FAntiCheatNetworkTransport const&) = delete;
	FAntiCheatNetworkTransport& operator=(FAntiCheatNetworkTransport const&) = delete;

	/**
	 * Destructor
	 */
	virtual ~FAntiCheatNetworkTransport();

	bool Connect(const char* Host, uint16_t Port);
	void Disconnect();

	using FOnNewMessageCallback = std::function<void(const void*, uint32_t)>;
	void SetOnNewMessageCallback(FOnNewMessageCallback Callback);

	using FOnClientActionRequiredCallback = std::function<void(EOS_AntiCheatCommon_OnClientActionRequiredCallbackInfo)>;
	void SetOnClientActionRequiredCallback(FOnClientActionRequiredCallback Callback);

	void Send(const EOS_AntiCheatClient_OnMessageToServerCallbackInfo* Message);
	void Send(const FRegistrationInfoMessage& Message);

	void Update();

private:
	size_t ProcessMessage(char* Buffer, size_t StartPosition);
	void Receive(char* Buffer, size_t Length);

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
	FOnClientActionRequiredCallback OnClientActionRequiredCallback;
};

