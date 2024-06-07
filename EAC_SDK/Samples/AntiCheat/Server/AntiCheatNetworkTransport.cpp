// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "AntiCheatNetworkTransport.h"

FAntiCheatNetworkTransport::FAntiCheatNetworkTransport()
	: TCPClient(10, 4096)
{
	TCPClient.SetOnBufferReceivedCallback([this](void* From, char* Buffer, size_t Length) { Receive(From, Buffer, Length); });
	TCPClient.SetOnClientDisconnectedCallback([this](void* Which) { OnClientDisconnectedCallback(Which); });
	TCPClient.Open(1234);
}

void FAntiCheatNetworkTransport::Send(const EOS_AntiCheatCommon_OnClientActionRequiredCallbackInfo* Message)
{
	char Buffer[4096] = {};
	size_t BufferPos = {};

	constexpr FMessageType MessageType = FMessageType::ClientActionRequired;
	const uint32_t MessageLength = 
		sizeof(Message->ActionReasonCode) +
		sizeof(Message->ClientAction) +
		static_cast<uint32_t>(strlen(Message->ActionReasonDetailsString)) + 1;

	Write(MessageType, Buffer, BufferPos);
	Write(MessageLength, Buffer, BufferPos);
	Write(Message->ClientAction, Buffer, BufferPos);
	Write(Message->ActionReasonCode, Buffer, BufferPos);
	Write(Message->ActionReasonDetailsString, strlen(Message->ActionReasonDetailsString) + 1, Buffer, BufferPos);

	TCPClient.Send(Message->ClientHandle, &Buffer, BufferPos);
}

void FAntiCheatNetworkTransport::Send(const EOS_AntiCheatCommon_OnMessageToClientCallbackInfo* Message)
{
	char Buffer[4096] = {};
	size_t BufferPos = {};

	constexpr FMessageType MessageType = FMessageType::Opaque;
	const uint32_t MessageLength = Message->MessageDataSizeBytes;

	Write(MessageType, Buffer, BufferPos);
	Write(MessageLength, Buffer, BufferPos);
	Write(Message->MessageData, Message->MessageDataSizeBytes, Buffer, BufferPos);

	TCPClient.Send(Message->ClientHandle, Buffer, BufferPos);
}

void FAntiCheatNetworkTransport::CloseClientConnection(void* ClientHandle)
{
	TCPClient.CloseClientConnection(reinterpret_cast<TCPsocket>(ClientHandle));
}

size_t FAntiCheatNetworkTransport::ProcessMessage(void* From, char* Buffer, size_t StartPosition)
{
	size_t Position = StartPosition;

	const FMessageType MessageType = Read<FMessageType>(Buffer, Position);
	const uint32_t MessageLength = Read<uint32_t>(Buffer, Position);

	if (MessageType == FMessageType::Opaque)
	{
		const char* Message = Read<char*>(Buffer, MessageLength, Position);
		OnNewMessageCallback(From, Message, MessageLength);
	}
	else if (MessageType == FMessageType::RegistrationInfo)
	{
		FRegistrationInfoMessage Message = {};
		Message.ProductUserId = Read<char*>(Buffer, strlen(Buffer + Position) + 1, Position);
		Message.EOSConnectIdTokenJWT = Read<char*>(Buffer, strlen(Buffer + Position) + 1, Position);
		Message.ClientPlatform = Read<EOS_EAntiCheatCommonClientPlatform>(Buffer, Position);
		OnNewClientCallback(From, Message);
	}

	return Position - StartPosition;
}

void FAntiCheatNetworkTransport::Receive(void* From, char* Buffer, size_t Length)
{
	size_t Position = 0;
	size_t BytesRemaining = Length;
	while (BytesRemaining > 0)
	{
		const size_t BytesRead = ProcessMessage(From, Buffer, Position);
		Position += BytesRead;
		BytesRemaining -= BytesRead;
	}
}

FAntiCheatNetworkTransport& FAntiCheatNetworkTransport::GetInstance()
{
	static FAntiCheatNetworkTransport Instance;
	return Instance;
}

void FAntiCheatNetworkTransport::Update()
{
	TCPClient.Update();
}

void FAntiCheatNetworkTransport::SetOnNewMessageCallback(FOnNewMessageCallback Callback)
{
	OnNewMessageCallback = std::move(Callback);
}

void FAntiCheatNetworkTransport::SetOnNewClientCallback(FOnNewClientCallback Callback)
{
	OnNewClientCallback = std::move(Callback);
}

void FAntiCheatNetworkTransport::SetOnClientDisconnectedCallback(FOnClientDisconnectedCallback Callback)
{
	OnClientDisconnectedCallback = std::move(Callback);
}
