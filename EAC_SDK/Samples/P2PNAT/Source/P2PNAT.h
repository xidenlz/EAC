// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include <eos_sdk.h>
#include <eos_p2p.h>

/**
* Manages Peer 2 Peer connections with capabilities of NAT traversal.
*/
class FP2PNAT
{
public:
	/**
	* Constructor
	*/
	FP2PNAT() noexcept(false);

	/**
	* No copying or copy assignment allowed for this class.
	*/
	FP2PNAT(FP2PNAT const&) = delete;
	FP2PNAT& operator=(FP2PNAT const&) = delete;

	/**
	* Destructor
	*/
	virtual ~FP2PNAT();

	void Update();

	void RefreshNATType();
	EOS_ENATType GetNATType() const;
	
	/**
	* Receives game event
	*
	* @param Event - Game event to act on
	*/
	void OnGameEvent(const FGameEvent& Event);

	/** 
	 * Send chat message to your friend.
	 * @param FriendId - account Id of the receiver
	 * @param Message - chat text message to send
	 */
	void SendMessage(FProductUserId FriendId, const std::wstring& Message);
	void HandleReceivedMessages();

	void SubscribeToConnectionRequests();
	void UnsubscribeFromConnectionRequests();

	void SubscribeToConnectionEstablished();
	void UnsubscribeToConnectionEstablished();

	static void EOS_CALL OnIncomingConnectionRequest(const EOS_P2P_OnIncomingConnectionRequestInfo* Data);
	static void EOS_CALL OnIncomingConnectionEstablished(const EOS_P2P_OnPeerConnectionEstablishedInfo* Data);
	
private:

	/**
	* Called when a user has logged in
	*/
	void OnUserConnectLoggedIn(FProductUserId UserId);

	/**
	* Called when a user has logged out
	*/
	void OnLoggedOut(FEpicAccountId UserId);

	static void EOS_CALL OnRefreshNATTypeFinished(const EOS_P2P_OnQueryNATTypeCompleteInfo* Data);

	EOS_NotificationId ConnectionNotificationId = EOS_INVALID_NOTIFICATIONID;
	EOS_NotificationId ConnectionEstablishedNotificationId = EOS_INVALID_NOTIFICATIONID;
};
