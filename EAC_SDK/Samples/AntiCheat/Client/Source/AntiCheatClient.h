// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include <eos_anticheatclient_types.h>

class FGameEvent;

template<typename> struct TEpicAccountId;
using FProductUserId = TEpicAccountId<EOS_ProductUserId>;

class FAntiCheatClient
{
public:
	/**
	* Constructor
	*/
	FAntiCheatClient() noexcept(false);

	/**
	* No copying or copy assignment allowed for this class.
	*/
	FAntiCheatClient(FAntiCheatClient const&) = delete;
	FAntiCheatClient& operator=(FAntiCheatClient const&) = delete;

	/**
	 * Destructor
	 */
	virtual ~FAntiCheatClient();

	void Init();
	void OnShutdown();

	bool Start(const std::string& Host, int Port, const FProductUserId& LocalUserId, const std::string& EOSConnectIdTokenJWT);
	void Stop();

	void PollStatus();

private:
	bool AddNotifyMessageToServerCallback();
	void RemoveNotifyMessageToServerCallback();
	bool BeginSession(const FProductUserId& LocalUserId);
	void EndSession();

	bool ConnectToAntiCheatServer(const std::string& Host, int Port);
	void SendRegistrationInfoToAntiCheatServer(const FProductUserId& LocalUserId, const std::string& EOSConnectIdTokenJWT);
	void DisconnectFromAntiCheatServer();

	void OnMessageFromServerReceived(const void* Data, uint32_t DataLengthBytes) const;
	static void EOS_CALL OnMessageToServerCallback(const EOS_AntiCheatClient_OnMessageToServerCallbackInfo* Message);

private:
	EOS_HAntiCheatClient AntiCheatClientHandle = nullptr;
	EOS_NotificationId NotificationId = EOS_INVALID_NOTIFICATIONID;
	bool bConnected = false;
};

