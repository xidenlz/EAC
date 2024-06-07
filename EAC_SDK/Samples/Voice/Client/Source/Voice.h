// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Player.h"

#include <eos_sdk.h>
#include <eos_rtc_admin.h>
#include <eos_rtc_audio.h>

/**
 * Attributes for each member of a room
 */
struct FVoiceRoomMember
{
	FVoiceRoomMember() = default;

	PlayerPtr Player = nullptr;
	float Volume = 50.0f;
	bool bIsOwner = false;
	bool bIsLocal = false;
	bool bIsSpeaking = false;
	bool bIsMuted = false;
	bool bIsRemoteMuted = false;
};

/**
 * Structure to store cached data for joining rooms via a room token
 */
struct FVoiceUserRoomTokenData
{
	FVoiceUserRoomTokenData() = default;

	EOS_ProductUserId ProductUserId;
	std::string Token;
};

/**
* Manages voice.
*/
class FVoice
{
public:
	/**
	* Constructor
	*/
	FVoice() noexcept(false);

	/**
	* No copying or copy assignment allowed for this class.
	*/
	FVoice(FVoice const&) = delete;
	FVoice& operator=(FVoice const&) = delete;

	/**
	* Destructor
	*/
	virtual ~FVoice();

	void Init();

	void OnShutdown();

	void Update();

	/**
	* Receives game event
	*
	* @param Event - Game event to act on
	*/
	void OnGameEvent(const FGameEvent& Event);

	/** Collection of members in a room */
	std::map<EOS_ProductUserId, FVoiceRoomMember> GetRoomMembers() { return RoomMembers; }

	/** True if user is a member of the room */
	bool IsMember(FProductUserId ProductUserId);

	/** Clears saves collection of room members */
	void ClearRoomMembers();

	/** Mutes / unmutes a member in room toggling previous state */
	void LocalToggleMuteMember(FProductUserId ProductUserId);

	/**
	 * Starts room joining process by querying for a room token
	 */
	void QueryJoinRoomToken(FProductUserId ProductUserId, std::wstring InRoomName);

	/**
	 * Mutes / unmutes a member remotely in room toggling previous state
	 */
	void RemoteToggleMuteMember(FProductUserId ProductUserId);

	/**
	 * Mutes / unmutes a member remotely in room
	 */
	void RemoteMuteMember(FProductUserId ProductUserId, bool bIsMuted);

	/**
	 * Kick a member from room
	 */
	void KickMember(FProductUserId ProductUserId);

	/**
	 * Heartbeats the active voice session
	 */
	void HeartbeatVoiceSession(FProductUserId ProductUserId, const std::string& OwnerLock);

	/** Sets speaking state for a member */
	void SetMemberSpeakingState(FProductUserId ProductUserId, bool bIsMuted);

	/** Sets mute state for a member */
	void SetMemberMuteState(FProductUserId ProductUserId, bool bIsMuted);

	/** Sets remote mute status for member */
	void SetMemberRemoteMuteState(FProductUserId ProductUserId, bool bIsMuted);

	/** Sets volume for member */
	void SetMemberVolume(FProductUserId ProductUserId, float Volume);

	/** Get collection of audio input devices */
	std::vector<std::wstring> GetAudioInputDeviceNames() { return AudioInputDeviceNames; }

	/** Get collection of audio output devices */
	std::vector<std::wstring> GetAudioOutputDeviceNames() { return AudioOutputDeviceNames; }

	/** Sets the active audio input device */
	void SetAudioInputDeviceFromName(std::wstring DeviceName);

	/** Sets the active audio output device */
	void SetAudioOutputDeviceFromName(std::wstring DeviceName);

	/** Sets owner lock string */
	void SetOwnerLock(std::string InOwnerLock) { OwnerLock = InOwnerLock; }

	/** Set the output room's volume */
	void UpdateReceivingVolume(const std::wstring& InVolume);
	
	/** Set participant output volume */
	void UpdateParticipantVolume(FProductUserId ProductUserId, const std::wstring& InVolume);

private:
	/**
	 * Called when a user has logged in
	 */
	void OnLoggedIn(FEpicAccountId UserId);

	/**
	 * Called when a user has logged out
	 */
	void OnLoggedOut(FEpicAccountId UserId);

	/**
	 * Called when a user connect has logged in
	 */
	void OnUserConnectLoggedIn(FProductUserId ProductUserId);

	/**
	 * Subscribe to room-based notifications
	 */
	void SubscribeToRoomNotifications(std::wstring InRoomName);

	/**
	 * Unsubscribe from room-based notifications
	 */
	void UnsubscribeFromRoomNotifications();

	/**
	 * Subscribe to room-independent notifications
	 */
	void SubscribeToNotifications();

	/**
	 * Unsubscribe from room-based notifications
	 */
	void UnsubscribeFromNotifications();

	/**
	 * Get available audio input devices
	 */
	void GetAudioInputDevices();

	/**
	 * Update list available audio input devices
	 */
	void UpdateAudioInputDevices();

	/**
	 * Get available audio output devices
	 */
	void GetAudioOutputDevices();

	/**
	 * Update list available audio output devices
	 */
	void UpdateAudioOutputDevices();

	/**
	 * Requests joining a room via trusted server
	 */
	void RequestJoinRoom(FProductUserId ProductUserId, std::wstring InRoomName);

	/**
	 * Starts room joining process by querying to join a room
	 */
	void QueryJoinRoom(FProductUserId ProductUserId, std::wstring InRoomName, std::wstring InClientBaseUrl, std::string InToken);

	/**
	 * Starts room leaving process by querying to leave a room
	 */
	void QueryLeaveRoom(FProductUserId ProductUserId, std::wstring InRoomName);

	/**
	 * Starts joining a given room by name via trusted server
	 */
	void StartJoinRoom(FProductUserId ProductUserId, std::wstring InRoomName, std::wstring InClientBaseUrl, std::string InToken);

	/**
	 * Joins a given room by name
	 */
	void JoinRoom(FProductUserId ProductUserId, std::wstring InRoomName);

	/** Finalizes room joining process where player is ready to be added to room members */
	void FinalizeJoinRoom(PlayerPtr Player, FProductUserId ProductUserId, std::wstring InRoomName);

	/** Queries for a new player's information before they can be added to room members */
	void QueryPlayerInfo(FProductUserId LocalUserId, FProductUserId OtherUserId);

	/** EpicAccountId data has been retrieved for a new player */
	void OnEpicAccountsMappingRetrieved();

	/** Display name has been retrieved for a new player */
	void OnEpicAccountDisplayNameRetrieved(FEpicAccountId EpicUserId, std::wstring DisplayName);

	/**
	 * Add user room token data to locally cached data
	 */
	void AddLocalUserRoomTokenData(std::shared_ptr<FVoiceUserRoomTokenData> UserRoomToken);

	/**
	 * Join each participant in cached data to room
	 */
	void JoinFromCachedRoomTokenData(std::wstring InRoomName, std::wstring ClientBaseUrl);

	/**
	 * Leaves current room
	 */
	void LeaveRoom(FProductUserId ProductUserId, std::wstring InRoomName);

	/** Starts the join room process based on the room the friend with given account IDs is in using their presence join info */
	void JoinRoomWithFriend(FEpicAccountId EpicUserId, FProductUserId ProductUserId, std::wstring DisplayName);

	/** Clear saved owner lock */
	void ClearOwnerLock() { OwnerLock = std::string(); }

	/** Sets the presence join info to include the room id */
	void SetJoinInfo(const std::string& InRoomName);

	/** Sets audio input device based on select audio input device */
	void SetAudioInputDevice(const std::wstring& DeviceId);

	/** Sets audio output device based on select audio output device */
	void SetAudioOutputDevice(const std::wstring& DeviceId);

	/** Called when local user is disconnected */
	void OnDisconnected(FProductUserId LocalProductUserId, std::wstring InRoomName);

	/** Called when a user joins a room */
	void OnParticipantJoined(FProductUserId ProductUserId, std::wstring InRoomName);

	/** Called when a user leaves a room */
	void OnParticipantLeft(FProductUserId ProductUserId, std::wstring InRoomName);

	/** Called when a user in a room has their audio status updated */
	void OnParticipantAudioUpdated(FProductUserId ProductUserId, std::wstring InRoomName, EOS_ERTCAudioStatus InAudioStatus, bool bIsSpeaking);

	/** Called when audio devices have been updated */
	void OnAudioDevicesChanged();

	//Callbacks
	/**
	 * Callback that is fired on join room token query
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnQueryJoinRoomTokenCb(const EOS_RTCAdmin_QueryJoinRoomTokenCompleteCallbackInfo* Data);

	/**
	 * Callback that is fired on join room query
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnJoinRoomCb(const EOS_RTC_JoinRoomCallbackInfo* Data);

	/**
	 * Callback that is fired on leave room query
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnLeaveRoomCb(const EOS_RTC_LeaveRoomCallbackInfo* Data);

	/**
	 * Callback that is fired on setting presence with join info
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnSetPresenceCb(const EOS_Presence_SetPresenceCallbackInfo* Data);

	/**
	 * Callback that is fired when local user is disconnected from room
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnDisconnectedCb(const EOS_RTC_DisconnectedCallbackInfo* Data);

	/**
	 * Callback that is fired when a user's status changes (user joins or leaves a room)
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnParticipantStatusChangedCb(const EOS_RTC_ParticipantStatusChangedCallbackInfo* Data);

	/**
	 * Callback that is fired when a user's audio state is updated
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnParticipantAudioUpdatedCb(const EOS_RTCAudio_ParticipantUpdatedCallbackInfo* Data);

	/**
	 * Callback that is fired when audio devices have been changed
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnAudioDevicesChangedCb(const EOS_RTCAudio_AudioDevicesChangedCallbackInfo* Data);

	/**
	 * Callback that is fired when sending audio state has been updated
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnAudioUpdateSendingCb(const EOS_RTCAudio_UpdateSendingCallbackInfo* Data);

	/**
	 * Callback that is fired when receiving audio state has been updated
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnAudioUpdateReceivingCb(const EOS_RTCAudio_UpdateReceivingCallbackInfo* Data);

	/**
	 * Callback that is fired on update receiving room volume
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnUpdateReceivingVolumeCb(const EOS_RTCAudio_UpdateReceivingVolumeCallbackInfo* Data);

	/**
	 * Callback that is fired on update participant's room volume
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnUpdateParticipantVolumeCb(const EOS_RTCAudio_UpdateParticipantVolumeCallbackInfo* Data);

	/**
	 * Callback that is fired on update list of input audio devices
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnInputDevicesInformationCb(const EOS_RTCAudio_OnQueryInputDevicesInformationCallbackInfo* Data);

	/**
	 * Callback that is fired on update list of output audio devices
	*
	* @param Data - out parameter
	*/
	static void EOS_CALL OnOutputDevicesInformationCb(const EOS_RTCAudio_OnQueryOutputDevicesInformationCallbackInfo* Data);

	/**
	 * Callback that is fired on finish setting audio input device
	*
	* @param Data - out parameter
	*/
	static void EOS_CALL OnSetAudioInputDeviceCb(const EOS_RTCAudio_OnSetInputDeviceSettingsCallbackInfo* Data);

	/**
	 * Callback that is fired on finish setting audio output device
	*
	* @param Data - out parameter
	*/
	static void EOS_CALL OnSetAudioOutputDeviceCb(const EOS_RTCAudio_OnSetOutputDeviceSettingsCallbackInfo* Data);

	/** Handle to EOS SDK RTC system */
	EOS_HRTC RTCHandle;

	/** Handle to EOS SDK RTC Admin system */
	EOS_HRTCAdmin RTCAdminHandle;

	/** Handle to EOS SDK RTC Audio system */
	EOS_HRTCAudio RTCAudioHandle;

	/** URL for trusted server */
	std::wstring TrustedServerURL;

	/** Port for trusted server */
	std::wstring TrustedServerPort;

	/** Full URL for trusted server */
	std::wstring FullTrustedServerURL;

	/** Current room name local player has joined */
	std::wstring CurrentRoomName;

	/** Members of the current room */
	std::map<EOS_ProductUserId, FVoiceRoomMember> RoomMembers;

	/** Cached room token info */
	std::vector<std::shared_ptr<FVoiceUserRoomTokenData>> CachedUserRoomTokenInfo;

	/** Epic User Id of local user */
	FEpicAccountId LocalEpicUserId;

	/** Product User Id of local user */
	FProductUserId LocalProductUserId;

	/** Product User Ids that are currently being queried to get EpicAccountId and Display Name */
	std::vector<FProductUserId> ProductUserIdsToQuery;

	/** Names of audio input devices available */
	std::vector<std::wstring> AudioInputDeviceNames;

	/** Names of audio input devices available */
	std::vector<std::wstring> AudioOutputDeviceNames;

	/** IDs of audio input devices available */
	std::vector<std::wstring> AudioInputDeviceIds;

	/** IDs of audio output devices available */
	std::vector<std::wstring> AudioOutputDeviceIds;

	/** Body param for QueryJoinRoomToken request */
	std::string QueryJoinRoomTokenRequestBody;

	/** Body param for Kick request */
	std::string KickRequestBody;

	/** Body param for Mute request */
	std::string MuteRequestBody;

	/** Body param for Mute request */
	std::string HeartbeatRequestBody;

	/** If non-empty the local client is the owner of the current room */
	std::string OwnerLock;

	// Notifications
	EOS_NotificationId DisconnectedNotification = EOS_INVALID_NOTIFICATIONID;
	EOS_NotificationId ParticipantStatusChangedNotification = EOS_INVALID_NOTIFICATIONID;
	EOS_NotificationId ParticipantAudioUpdatedNotification = EOS_INVALID_NOTIFICATIONID;
	EOS_NotificationId AudioDevicesChangedNotification = EOS_INVALID_NOTIFICATIONID;

	std::chrono::steady_clock::time_point NextHeartbeat;
};
