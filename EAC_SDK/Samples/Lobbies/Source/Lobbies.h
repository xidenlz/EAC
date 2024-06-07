// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include <eos_sdk.h>
#include <eos_lobby.h>
#include <eos_lobby_types.h>
#include <eos_rtc_types.h>
#include <eos_rtc_audio_types.h>
#include <eos_rtc_data_types.h>

/**
 * Simple Attribute struct to contain lobby attribute information. It can have a value from one of the available types.
 */
struct FLobbyAttribute
{
	enum Type
	{
		String,
		Int64,
		Double,
		Bool
	};
	Type ValueType = Type::String;

	//Only one of the following properties will have valid data (depending on 'ValueType')
	int64_t AsInt64 = 0;
	double AsDouble = 0.0;
	bool AsBool = false;
	std::string AsString;

	//Attribute key
	std::string Key;

	//Is this attribute public or private to the lobby and its members
	EOS_ELobbyAttributeVisibility Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC;

	bool operator==(const FLobbyAttribute& Other) const
	{
		return ValueType == Other.ValueType &&
			AsInt64 == Other.AsInt64 &&
			AsDouble == Other.AsDouble &&
			AsBool == Other.AsBool &&
			Key == Other.Key &&
			Visibility == Other.Visibility &&
			AsString == Other.AsString;
	}

	bool operator !=(const FLobbyAttribute& Other) const
	{
		return !(operator==(Other));
	}

	void InitFromAttribute(EOS_Lobby_Attribute* Attr);
	EOS_Lobby_AttributeData ToAttributeData() const;
};

struct FLobbyMember
{
	enum class Skin : int32_t
	{
		Peasant = 0,
		Knight,
		Princess,
		Joker,
		Mage,
		Count
	};

	enum class SkinColor : uint8_t
	{
		White = 0,
		Yellow,
		LightGreen,
		Pink,
		Aqua,
		Count
	};

	std::wstring GetAttributesString() const;
	void ShuffleSkin()
	{
		CurrentSkin = static_cast<Skin>(static_cast<int32_t>(CurrentSkin) + 1);
		if (CurrentSkin >= Skin::Count)
		{
			CurrentSkin = Skin::Peasant;
		}
	}
	void ShuffleColor()
	{
		CurrentColor = static_cast<SkinColor>(static_cast<int32_t>(CurrentColor) + 1);
		if (CurrentColor >= SkinColor::Count)
		{
			CurrentColor = SkinColor::White;
		}

	}
	static std::string GetSkinString(Skin InSkin)
	{
		switch (InSkin)
		{
		case Skin::Peasant:
			return "Peasant";
		case Skin::Knight:
			return "Knight";
		case Skin::Princess:
			return "Princess";
		case Skin::Joker:
			return "Joker";
		case Skin::Mage:
			return "Mage";
		default:
			return "INVALID";
		}

		return "INVALID";
	}

	static FColor GetSkinColor(SkinColor InColor)
	{
		switch (InColor)
		{
		case SkinColor::White:
			return Color::White;
		case SkinColor::Yellow:
			return Color::Yellow;
		case SkinColor::LightGreen:
			return Color::LightGreen;
		case SkinColor::Pink:
			return Color::Pink;
		case SkinColor::Aqua:
			return Color::Aqua;
		default:
			return Color::White;
		}
	}

	void InitSkinFromString(const std::string& SkinString)
	{
		for (int32_t SkinInt = 0; SkinInt < static_cast<int32_t>(Skin::Count); ++SkinInt)
		{
			Skin NextSkin = static_cast<Skin>(SkinInt);
			if (SkinString == GetSkinString(NextSkin))
			{
				CurrentSkin = NextSkin;
				return;
			}
		}

		//fallback
		CurrentSkin = Skin::Peasant;
	}

	FEpicAccountId AccountId;
	FProductUserId ProductId;
	std::wstring DisplayName;
	Skin CurrentSkin = Skin::Peasant;
	SkinColor CurrentColor = SkinColor::White;
	std::vector<FLobbyAttribute> MemberAttributes;

	/** Container for all RTC-related state of this lobby member */
	struct FLobbyRTCState
	{
		/** Is this person currently connected to the RTC room? */
		bool bIsInRTCRoom = false;
		/** Is this person currently talking (audible sounds from their audio output) */
		bool bIsTalking = false;
		/** We have locally muted this person (others can still hear them) */
		bool bIsLocallyMuted = false;
		/** Is this person hard muted (muted for everyone in the lobby by lobby owner */
		bool bIsHardMuted = false;
		/** Has this person muted their own audio output (nobody can hear them)*/
		bool bIsAudioOutputDisabled = false;
		/** Are we currently muting this person? */
		bool bMuteActionInProgress = false;
		/** Are we currently hard muting this person? */
		bool bHardMuteActionInProgress = false;
	};
	FLobbyRTCState RTCState;
};

/** 
 * Game lobby class. Contains lobby information such as game settings, participants and their own attributes.
 */
struct FLobby
{
	bool IsValid() const { return !Id.empty(); }
	bool IsOwner(EOS_ProductUserId UserProductId) const { return FProductUserId(UserProductId) == LobbyOwner; }
	bool AreInvitesAllowed() const { return bAllowInvites; }
	bool IsPresenceEnabled() const { return bPresenceEnabled; }
	bool IsRTCRoomEnabled() const { return bRTCRoomEnabled; }

	void Clear() { Id = std::string(); }

	void InitFromLobbyHandle(EOS_LobbyId Id);
	void InitFromLobbyDetails(EOS_HLobbyDetails Id);

	bool operator==(const FLobby& Other) const
	{
		return Id == Other.Id;
	}

	bool operator !=(const FLobby& Other) const
	{
		return !operator==(Other);
	}

	const FLobbyAttribute* GetAttribute(std::string AttrKey) const
	{
		for (const FLobbyAttribute& NextAttr : Attributes)
		{
			if (NextAttr.Key == AttrKey)
			{
				return &NextAttr;
			}
		}

		return nullptr;
	}

	FLobbyMember* GetMemberByProductUserId(const FProductUserId& ProductId)
	{
		for (FLobbyMember& LobbyMember : Members)
		{
			if (LobbyMember.ProductId == ProductId)
			{
				return &LobbyMember;
			}
		}

		return nullptr;
	}

	EOS_ELobbyPermissionLevel Permission = EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED;

	std::vector<FLobbyAttribute> Attributes;
	std::vector<FLobbyMember> Members;

	std::string Id;
	FProductUserId LobbyOwner;
	FEpicAccountId LobbyOwnerAccountId;
	std::wstring LobbyOwnerDisplayName;
	std::string BucketId;
	uint32_t MaxNumLobbyMembers = 0;
	uint32_t AvailableSlots = 0;
	bool bAllowInvites = true;

	/** Cached copy of the RoomName of the RTC room that our lobby has, if any */
	std::string RTCRoomName;
	/** Are we currently connected to an RTC room? */
	bool bRTCRoomConnected = false;
	/** Notification for RTC connection status changes */
	EOS_NotificationId RTCRoomConnectionChanged = EOS_INVALID_NOTIFICATIONID;
	/** Notification for RTC room participant updates (new players or players leaving) */
	EOS_NotificationId RTCRoomParticipantUpdate = EOS_INVALID_NOTIFICATIONID;
	/** Notification for RTC audio updates (talking status or mute changes) */
	EOS_NotificationId RTCRoomParticipantAudioUpdate = EOS_INVALID_NOTIFICATIONID;
	/** Notification for RTC data receiving */
	EOS_NotificationId RTCRoomDataReceived = EOS_INVALID_NOTIFICATIONID;

	//Utility data
	bool bBeingCreated = false;
	bool bSearchResult = false;
	bool bPresenceEnabled = false;
	bool bRTCRoomEnabled = false;
};

//Simple RAII wrapper to make sure LobbyDetails handles are released correctly.
using LobbyDetailsKeeper = std::shared_ptr<struct EOS_LobbyDetailsHandle>;
inline LobbyDetailsKeeper MakeLobbyDetailsKeeper(EOS_HLobbyDetails LobbyDetails)
{
	return LobbyDetailsKeeper(LobbyDetails, EOS_LobbyDetails_Release);
}

using LobbyModificationKeeper = std::shared_ptr<struct EOS_LobbyModificationHandle>;
inline LobbyModificationKeeper MakeLobbyDetailsKeeper(EOS_HLobbyModification LobbyModification)
{
	return LobbyModificationKeeper(LobbyModification, EOS_LobbyModification_Release);
}

/** 
 * Class to perform lobby query and to contain search results.
 */
class FLobbySearch
{
public:
	FLobbySearch() {}
	~FLobbySearch();

	FLobbySearch(const FLobbySearch&) = delete;
	FLobbySearch& operator=(const FLobbySearch&) = delete;

	bool IsValid() const { return SearchHandle; }

	//Release a clear current search
	void Release();

	//Clear previous and prepare for new search results.
	void SetNewSearch(EOS_HLobbySearch);

	//Called when new search data arrives.
	void OnSearchResultsReceived(std::vector<FLobby>&&, std::vector<LobbyDetailsKeeper>&&);

	//Getters to query current search results
	std::vector<FLobby>& GetResults() { return SearchResults; }
	const std::vector<FLobby>& GetResults() const { return SearchResults; }
	const std::vector<LobbyDetailsKeeper> GetDetailsHandles() const { return ResultHandles; }
	EOS_HLobbySearch GetSearchHandle() const { return SearchHandle; }
	LobbyDetailsKeeper GetLobbyDetailsHandleById(EOS_LobbyId LobbyId) const
	{
		for (size_t i = 0; i < SearchResults.size(); ++i)
		{
			if (SearchResults[i].Id == LobbyId)
			{
				if (ResultHandles.size() > i)
				{
					return ResultHandles[i];
				}
				break;
			}
		}
		return nullptr;
	}

private:
	EOS_HLobbySearch SearchHandle = nullptr;
	std::vector<FLobby> SearchResults;
	std::vector<LobbyDetailsKeeper> ResultHandles;
};

struct FLobbyInvite
{
	FLobby Lobby;
	LobbyDetailsKeeper LobbyInfo;
	FProductUserId FriendId;
	FEpicAccountId FriendEpicId;
	std::wstring FriendDisplayName;
	std::string InviteId;

	bool IsValid() const { return Lobby.IsValid(); }
	void Clear()
	{
		Lobby.Clear();
		LobbyInfo.reset();
		FriendId = FProductUserId();
		FriendEpicId = FEpicAccountId();
		FriendDisplayName.clear();
	}
};

struct FLobbyJoinRequest
{
	EOS_LobbyId Id;
	LobbyDetailsKeeper LobbyInfo;
	bool bPresenceEnabled = false;

	bool IsValid() const { return (!!Id) && LobbyInfo; }
	void Clear()
	{
		Id = nullptr;
		LobbyInfo.reset();
		bPresenceEnabled = false;
	}
};

/* Command type using with RTC Data channel*/
enum class ELobbyRTCDataCommand : uint8_t
{
	SkinColor = 0
};

/**
* Manages game lobbies.
*/
class FLobbies
{
public:
	/**
	* Constructor
	*/
	FLobbies() noexcept(false);

	/**
	* No copying or copy assignment allowed for this class.
	*/
	FLobbies(FLobbies const&) = delete;
	FLobbies& operator=(FLobbies const&) = delete;

	/**
	* Destructor
	*/
	virtual ~FLobbies();

	void OnShutdown();

	void Update();

	/**
	* Receives game event
	*
	* @param Event - Game event to act on
	*/
	void OnGameEvent(const FGameEvent& Event);

	/**
	 * Get current local user
	 */
	FProductUserId GetLocalProductUserId() const { return CurrentUserProductId; }

	/**
	 *  Read-only access to current lobby. Sample only allows one lobby at a time.
	 */
	const FLobby& GetCurrentLobby() const { return CurrentLobby; }

	/**
	 *  Read-only access to current lobby we were invited to.
	 */
	const FLobbyInvite* GetCurrentInvite() const
	{
		return CurrentInvite;
	}

	/** 
	 * Get the name of a player who invited us to lobby
	 */
	const std::wstring& GetInviteSenderName() const
	{
		if (CurrentInvite)
		{
			return CurrentInvite->FriendDisplayName;
		}
		else
		{
			static std::wstring EmptyString;
			return EmptyString;
		}
	}

	/** 
	 * Drop current invite and pop the next pending invite
	 */
	void PopLobbyInvite();

	/**
	 * Accessor to search results.
	 */
	const FLobbySearch& GetCurrentSearch() const { return CurrentSearch; }

	bool HasActiveLobby() const { return CurrentLobby.IsValid(); }
	bool CanInviteToCurrentLobby(FProductUserId ProductUserId) const;

	//Lobby management (owner)

	/**
	* Leave current lobby (of any) then
	* create and automatically join lobby. 
	*/
	bool CreateLobby(const FLobby& Lobby);
	bool DestroyCurrentLobby();

	bool IsReadyToShutdown() const;

	/*
	 * Modify existing lobby. Lobby object must have the same id. It should also contain all the properties that need to be updated.
	 * Empty properties are interpret as 'no change'.
	 */
	bool ModifyLobby(const FLobby& LobbyChanges);

	//Pull data about lobby
	void UpdateLobby();

	void KickMember(EOS_ProductUserId MemberId);
	void PromoteMember(EOS_ProductUserId MemberId);

	//Lobby management (member)
	void JoinLobby(EOS_LobbyId Id, LobbyDetailsKeeper LobbyInfo, bool bPresenceEnabled);
	void JoinSearchResult(size_t Index);
	void RejectLobbyInvite(const std::string& InviteId);
	void LeaveLobby();
	void SendInvite(FProductUserId TargetUserId);
	void ShuffleSkin(); //Toggles your own skin (member attribute) between possible options
	void ShuffleColor(); // Toggle your own color between possible options
	void SendSkinColorUpdate(FLobbyMember::SkinColor InColor);
	void MuteAudio(FProductUserId TargetUserId);
	void ToggleHardMuteMember(FProductUserId TargetUserId);
	void HardMuteMember(FProductUserId TargetUserId, bool bIsHardMuted); // mute player for everyone in the lobby
	void SetMemberAttribute(const FLobbyAttribute& MemberAttribute);
	void SetInitialMemberAttribute();

	//Search for lobby
	void Search(const std::vector<FLobbyAttribute>& SearchAttributes, uint32_t MaxNumResults = 10);
	void Search(const std::string& LobbyId, uint32_t MaxNumResults);
	void SearchLobbyByLevel(const std::string& LevelName);
	void SearchLobbyByBucketId(const std::string& BucketId);
	void ClearSearch();

	void SubscribeToLobbyUpdates();
	void UnsubscribeFromLobbyUpdates();

	void SubscribeToLobbyInvites();
	void UnsubscribeFromLobbyInvites();

	void SubscribeToLeaveLobbyUI();
	void UnsubscribeFromLeaveLobbyUI();

	std::string GetRTCRoomName();

	void SubscribeToRTCEvents();
	void UnsubscribeFromRTCEvents();

	void OnLobbyCreated(EOS_LobbyId Id);
	void OnLobbyUpdated(EOS_LobbyId Id);
	void OnLobbyUpdate(EOS_LobbyId Id);
	void OnLobbyJoined(EOS_LobbyId Id);
	void OnLobbyJoinFailed(EOS_LobbyId Id);
	void OnLobbyInvite(const char* InviteId, FProductUserId SenderId);
	void OnLobbyInviteAccepted(const char* InviteId, FProductUserId SenderId);
	void OnJoinLobbyAccepted(FProductUserId LocalUserId, EOS_UI_EventId UiEventId);
	void OnSearchResultsReceived();
	void OnKickedFromLobby(EOS_LobbyId Id);
	void OnLobbyLeftOrDestroyed(EOS_LobbyId Id);
	void OnHardMuteMemberFinished(EOS_LobbyId Id, FProductUserId ParticipantId, EOS_EResult Result);
	void OnSkinColorChanged(FProductUserId ParticipantId, const FLobbyMember::SkinColor InColor);

	void OnRTCRoomConnectionChanged(EOS_LobbyId Id, FProductUserId LocalUserId, bool bIsConnected);
	void OnRTCRoomParticipantJoined(const char* RoomName, FProductUserId ParticipantId);
	void OnRTCRoomParticipantLeft(const char* RoomName, FProductUserId ParticipantId);
	void OnRTCRoomParticipantAudioUpdated(const char* RoomName, FProductUserId ParticipantId, bool bIsTalking, bool bIsMuted, bool bIsHardMuted);
	void OnRTCRoomUpdateSendingComplete(const char* RoomName, FProductUserId ParticipantId, EOS_ERTCAudioStatus NewAudioStatus);
	void OnRTCRoomUpdateReceivingComplete(const char* RoomName, FProductUserId ParticipantId, bool bIsMuted);
	void OnRTCRoomDataReceived(const char* RoomName, FProductUserId ParticipantId, const void* Data, uint32_t DataLengthBytes);

	//Callbacks
	static void EOS_CALL OnCreateLobbyFinished(const EOS_Lobby_CreateLobbyCallbackInfo* Data);
	static void EOS_CALL OnDestroyLobbyFinished(const EOS_Lobby_DestroyLobbyCallbackInfo* Data);
	static void EOS_CALL OnLobbyUpdateFinished(const EOS_Lobby_UpdateLobbyCallbackInfo* Data);
	static void EOS_CALL OnLobbyUpdateReceived(const EOS_Lobby_LobbyUpdateReceivedCallbackInfo* Data);
	static void EOS_CALL OnMemberUpdateReceived(const EOS_Lobby_LobbyMemberUpdateReceivedCallbackInfo* Data);
	static void EOS_CALL OnMemberStatusReceived(const EOS_Lobby_LobbyMemberStatusReceivedCallbackInfo* Data);

	static void EOS_CALL OnJoinLobbyFinished(const EOS_Lobby_JoinLobbyCallbackInfo* Data);
	static void EOS_CALL OnLeaveLobbyFinished(const EOS_Lobby_LeaveLobbyCallbackInfo* Data);
	static void EOS_CALL OnLobbyInviteSendFinished(const EOS_Lobby_SendInviteCallbackInfo* Data);
	static void EOS_CALL OnLobbyInviteReceived(const EOS_Lobby_LobbyInviteReceivedCallbackInfo* Data);
	static void EOS_CALL OnLobbyInviteAccepted(const EOS_Lobby_LobbyInviteAcceptedCallbackInfo* Data);
	static void EOS_CALL OnJoinLobbyAccepted(const EOS_Lobby_JoinLobbyAcceptedCallbackInfo* Data);
	static void EOS_CALL OnRejectInviteFinished(const EOS_Lobby_RejectInviteCallbackInfo* Data);

	static void EOS_CALL OnLobbySearchFinished(const EOS_LobbySearch_FindCallbackInfo* Data);
	static void EOS_CALL OnKickMemberFinished(const EOS_Lobby_KickMemberCallbackInfo* Data);
	static void EOS_CALL OnHardMuteMemberFinished(const EOS_Lobby_HardMuteMemberCallbackInfo* Data);
	static void EOS_CALL OnPromoteMemberFinished(const EOS_Lobby_PromoteMemberCallbackInfo* Data);

	static void EOS_CALL OnRTCRoomConnectionChangeReceived(const EOS_Lobby_RTCRoomConnectionChangedCallbackInfo* Data);
	static void EOS_CALL OnRTCRoomParticipantStatusChanged(const EOS_RTC_ParticipantStatusChangedCallbackInfo* Data);
	static void EOS_CALL OnRTCRoomParticipantAudioUpdateRecieved(const EOS_RTCAudio_ParticipantUpdatedCallbackInfo* Data);
	static void EOS_CALL OnRTCRoomUpdateSendingComplete(const EOS_RTCAudio_UpdateSendingCallbackInfo* Data);
	static void EOS_CALL OnRTCRoomUpdateReceivingComplete(const EOS_RTCAudio_UpdateReceivingCallbackInfo* Data);
	static void EOS_CALL OnRTCRoomDataReceived(const EOS_RTCData_DataReceivedCallbackInfo* Data);

	static void EOS_CALL OnLeaveLobbyRequested(const EOS_Lobby_LeaveLobbyRequestedCallbackInfo* Data);

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

	FProductUserId CurrentUserProductId;

	FLobby CurrentLobby;

	FLobbyJoinRequest ActiveJoin;

	//Pending invites (up to one invite per friend)
	std::map<FProductUserId, FLobbyInvite> Invites;

	//Invite that is being shown to the user (or null when nothing is being shown)
	FLobbyInvite* CurrentInvite = nullptr;

	//Search
	FLobbySearch CurrentSearch;

	EOS_NotificationId LobbyUpdateNotification = EOS_INVALID_NOTIFICATIONID;
	EOS_NotificationId LobbyMemberUpdateNotification = EOS_INVALID_NOTIFICATIONID;
	EOS_NotificationId LobbyMemberStatusNotification = EOS_INVALID_NOTIFICATIONID;
	EOS_NotificationId LobbyInviteNotification = EOS_INVALID_NOTIFICATIONID;
	EOS_NotificationId LobbyInviteAcceptedNotification = EOS_INVALID_NOTIFICATIONID;
	EOS_NotificationId JoinLobbyAcceptedNotification = EOS_INVALID_NOTIFICATIONID;
	EOS_NotificationId LeaveLobbyRequestedNotification = EOS_INVALID_NOTIFICATIONID;

	bool bLobbyLeaveInProgress = false;
	bool bDirty = true;
};
