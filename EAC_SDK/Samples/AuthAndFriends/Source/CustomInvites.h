// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "eos_sdk.h"
#include "AccountHelpers.h"

/**
* Forward declarations
*/
class FGameEvent;

/**
* Manages custom invite process for local user
*/
class FCustomInvites
{
public:
	/**
	* Constructor
	*/
	FCustomInvites() noexcept(false);

	/**
	* No copying or copy assignment allowed for this class.
	*/
	FCustomInvites(FCustomInvites const&) = delete;
	FCustomInvites& operator=(FCustomInvites const&) = delete;

	/**
	* Destructor
	*/
	virtual ~FCustomInvites();

	void OnLoggedIn(FEpicAccountId UserId);
	void OnConnectLoggedIn(FProductUserId ProductUserId);
	void OnLoggedOut(FEpicAccountId UserId);
	void FriendStatusChanged(FEpicAccountId LocalUserId, FEpicAccountId TargetUserId, EOS_EFriendsStatus NewStatus);
	EOS_EResult SetCurrentPayload(const std::wstring& Payload);

	void SendInviteToFriend(const std::wstring& FriendName);
	void SendInviteToFriend(FProductUserId FriendId);
	
	void OnGameEvent(const FGameEvent& Event);

	void SetCurrentUser(FEpicAccountId UserId);

	void FinalizeInvite(EOS_EResult Result);

	void AcceptLatestInvite();
	void RejectLatestInvite();

	static void EOS_CALL StaticOnNotifyCustomInviteReceived(const EOS_CustomInvites_OnCustomInviteReceivedCallbackInfo* Data);
	static void EOS_CALL StaticOnNotifyCustomInviteAccepted(const EOS_CustomInvites_OnCustomInviteAcceptedCallbackInfo* Data);
	static void EOS_CALL StaticOnNotifyCustomInviteRejected(const EOS_CustomInvites_CustomInviteRejectedCallbackInfo* Data);

	void HandleCustomInviteReceived(const char* Payload, const char* InviteId, FProductUserId SenderId);
	void HandleCustomInviteAccepted(const char* Payload, const char* InviteId, FProductUserId SenderId);
	void HandleCustomInviteRejected(const char* Payload, const char* InviteId, FProductUserId SenderId);

	/**
	 * Accessor for current user id
	 *
	 * @return User id for current user
	 */
	FEpicAccountId GetCurrentUser() { return CurrentUserId; };

	FProductUserId LatestReceivedFromId;
	std::wstring LatestReceivedInviteId;
	std::wstring LatestReceivedPayload;

private:
	/** Map that contains friend notification ids for all local users. */
	EOS_NotificationId FriendNotificationId;

	/** Id for current user. */
	FEpicAccountId CurrentUserId;

	/** Current product user id. */
	FProductUserId CurrentProductUserId;

};