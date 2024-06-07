// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"

#include "AntiCheatNetworkTransport.h"

FAntiCheatNetworkTransport::FAntiCheatNetworkTransport(): TCPClient(4096)
{
	TCPClient.SetOnBufferReceivedCallback([this](char* Buffer, size_t Length) { Receive(Buffer, Length); });
}

FAntiCheatNetworkTransport::~FAntiCheatNetworkTransport()
{

}

bool FAntiCheatNetworkTransport::Connect(const char* Host, uint16_t Port)
{
	return TCPClient.Connect(Host, Port);
}

void FAntiCheatNetworkTransport::Disconnect()
{
	TCPClient.Disconnect();
}

void FAntiCheatNetworkTransport::SetOnNewMessageCallback(FOnNewMessageCallback Callback)
{
	OnNewMessageCallback = std::move(Callback);
}

void FAntiCheatNetworkTransport::SetOnClientActionRequiredCallback(FOnClientActionRequiredCallback Callback)
{
	OnClientActionRequiredCallback = std::move(Callback);
}

void FAntiCheatNetworkTransport::Send(const EOS_AntiCheatClient_OnMessageToServerCallbackInfo* Message)
{
	char Buffer[4096] = {};
	size_t BufferPos = {};

	constexpr FMessageType MessageType = FMessageType::Opaque;
	const uint32_t MessageLength = Message->MessageDataSizeBytes;

	Write(MessageType, Buffer, BufferPos);
	Write(MessageLength, Buffer, BufferPos);
	Write(Message->MessageData, Message->MessageDataSizeBytes, Buffer, BufferPos);

	TCPClient.Send(Buffer, BufferPos);
}

void FAntiCheatNetworkTransport::Send(const FRegistrationInfoMessage& Message)
{
	char Buffer[4096] = {};
	size_t BufferPos = {};

	constexpr FMessageType MessageType = FMessageType::RegistrationInfo;
	const uint32_t MessageLength = 
		static_cast<uint32_t>(Message.ProductUserId.size() + 1) +
		sizeof(Message.ClientPlatform);

	Write(MessageType, Buffer, BufferPos);
	Write(MessageLength, Buffer, BufferPos);

	Write(Message.ProductUserId.c_str(), Message.ProductUserId.size() + 1, Buffer, BufferPos);
	Write(Message.EOSConnectIdTokenJWT.c_str(), Message.EOSConnectIdTokenJWT.size() + 1, Buffer, BufferPos);
	Write(Message.ClientPlatform, Buffer, BufferPos);

	TCPClient.Send(Buffer, BufferPos);
}

size_t FAntiCheatNetworkTransport::ProcessMessage(char* Buffer, size_t StartPosition)
{
	size_t Position = StartPosition;

	const FMessageType MessageType = Read<FMessageType>(Buffer, Position);
	const uint32_t MessageLength = Read<uint32_t>(Buffer, Position);

	if (MessageType == FMessageType::Opaque)
	{
		const char* Message = Read<char*>(Buffer, MessageLength, Position);
		OnNewMessageCallback(Message, MessageLength);
	}
	else if (MessageType == FMessageType::ClientActionRequired)
	{
		EOS_AntiCheatCommon_OnClientActionRequiredCallbackInfo Message = {};
		Message.ClientAction = Read<EOS_EAntiCheatCommonClientAction>(Buffer, Position);
		Message.ActionReasonCode = Read<EOS_EAntiCheatCommonClientActionReason>(Buffer, Position);
		Message.ActionReasonDetailsString = Read<char*>(Buffer, strlen(Buffer + Position) + 1, Position);
		OnClientActionRequiredCallback(Message);
	}

	return Position - StartPosition;
}

void FAntiCheatNetworkTransport::Receive(char* Buffer, size_t Length)
{
	size_t Position = 0;
	size_t BytesRemaining = Length;
	while (BytesRemaining > 0)
	{
		const size_t BytesRead = ProcessMessage(Buffer, Position);
		Position += BytesRead;
		BytesRemaining -= BytesRead;
	}
}

void FAntiCheatNetworkTransport::Update()
{
	TCPClient.Update();
}
