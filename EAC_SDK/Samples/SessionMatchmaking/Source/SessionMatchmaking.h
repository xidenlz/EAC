// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include <eos_sdk.h>
#include <eos_sessions.h>

constexpr char* SESSION_KEY_LEVEL = "LEVEL";

/**
 * These values are defined within the SDK in platform specific headers. In this example,
 * Sessions are created from PC so the values are all redefined here. This is indicative of what
 * game developers would need to employ for Windows or Linux based dedicated servers that
 * employ platform restrictions for console clients. There is an NDA concern regarding platform names
 * but the assumption is that a game developer creating a dedicated server to play with a console 
 * platform has an NDA in place for that platform.
 */
enum class ERestrictedPlatformType
{
	Unrestricted = 0,	// Unrestricted - all platforms allowed 
	PSN = 1000,			// Any Sony platform (PS4 or PS5)
	Switch = 2000,		// Nintendo (Switch only)
	XboxLive = 3000		// XSX and XboxOneGDK 
};

//Simple RAII wrapper to make sure SessionDetails handles are released correctly.
using SessionDetailsKeeper = std::shared_ptr<struct EOS_SessionDetailsHandle>;
inline SessionDetailsKeeper MakeSessionDetailsKeeper(EOS_HSessionDetails SessionDetails)
{
	return SessionDetailsKeeper(SessionDetails, EOS_SessionDetails_Release);
}

//Simple RAII wrapper to make sure ActiveSession handles are released correctly.
using ActiveSessionKeeper = std::shared_ptr<struct EOS_ActiveSessionHandle>;
inline ActiveSessionKeeper MakeActiveSessionKeeper(EOS_HActiveSession ActiveSession)
{
	return ActiveSessionKeeper(ActiveSession, EOS_ActiveSession_Release);
}

/** 
 * Game session class. Contains all the session information. Some properties can be unavailable (and left empty) at times.
 */
struct FSession
{
	/**
	 * Simple Attribute struct to contain session's attribute information. It can have a value from one of the available types.
	 */
	struct Attribute
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

		//Publicity of the attribute
		EOS_ESessionAttributeAdvertisementType Advertisement = EOS_ESessionAttributeAdvertisementType::EOS_SAAT_DontAdvertise;

		bool operator==(const Attribute& Other) const
		{
			return ValueType == Other.ValueType &&
				AsInt64 == Other.AsInt64 &&
				AsDouble == Other.AsDouble &&
				AsBool == Other.AsBool &&
				Key == Other.Key &&
				Advertisement == Other.Advertisement &&
				AsString == Other.AsString;
		}

		bool operator !=(const Attribute& Other) const
		{
			return !(operator==(Other));
		}
	};

	//Session is considered valid when either name or session id is valid.
	bool IsValid() const { return !Name.empty() || !Id.empty(); }

	bool operator==(const FSession& Other) const
	{
		return Name == Other.Name &&
			Id == Other.Id &&
			BucketId == Other.BucketId &&
			MaxPlayers == Other.MaxPlayers &&
			NumConnections == Other.NumConnections &&
			bAllowJoinInProgress == Other.bAllowJoinInProgress &&
			bPresenceSession == Other.bPresenceSession &&
			bInvitesAllowed == Other.bInvitesAllowed &&
			PermissionLevel == Other.PermissionLevel &&
			Attributes == Other.Attributes;
	}

	bool operator !=(const FSession& Other) const
	{
		return !(operator==(Other));
	}

	const Attribute* GetAttribute(std::string AttrKey) const
	{
		for (const auto& NextAttr : Attributes)
		{
			if (NextAttr.Key == AttrKey)
			{
				return &NextAttr;
			}
		}

		return nullptr;
	}

	//Initialize our structure based on SessionInfo of a SessionDetails from EOS_SDK
	bool InitFromInfoOfSessionDetails(SessionDetailsKeeper SessionDetails);
	//Initialize our structure based on SessionInfo from EOS_SDK
	void InitFromSessionInfo(EOS_HSessionDetails SessionHandle, EOS_SessionDetails_Info* SessionInfo);
	void InitActiveSession();

	std::string Name;
	std::string Id;
	std::string BucketId; // The top - level, game - specific filtering information for session searches. This criteria should be set with mostly static, coarse settings, often formatted like "GameMode:Region:MapName".
	uint32_t MaxPlayers;
	uint32_t NumConnections = 1; //optional ('1' because we will register the owner with the session right after create)
	bool bAllowJoinInProgress;
	bool bPresenceSession = false;
	bool bInvitesAllowed = true;
	ERestrictedPlatformType RestrictedPlatform = ERestrictedPlatformType::Unrestricted;
	EOS_EOnlineSessionPermissionLevel PermissionLevel;
	ActiveSessionKeeper ActiveSession;

	std::vector<Attribute> Attributes;

	//UI-related. Is this session coming from search query?
	bool bSearchResult = false;

	//Are we updating session at the moment?
	bool bUpdateInProgress = true;

	//What's current state of the session?
	EOS_EOnlineSessionState SessionState = EOS_EOnlineSessionState::EOS_OSS_NoSession;

	//Special value for an invalid session. Used as a sign of error.
	static FSession InvalidSession;
};

/** 
 * Class to perform session queries and to contain search results.
 */
class FSessionSearch
{
public:
	FSessionSearch() {}
	~FSessionSearch();

	FSessionSearch(const FSessionSearch&) = delete;
	FSessionSearch& operator=(const FSessionSearch&) = delete;

	//Release a clear current search
	void Release();

	//Clear previous and prepare for new search results.
	void SetNewSearch(EOS_HSessionSearch);

	//Called when new search data arrives.
	void OnSearchResultsReceived(std::vector<FSession>&&, std::vector<SessionDetailsKeeper>&&);

	//Getters to query current search results
	const std::vector<FSession>& GetResults() const { return SearchResults; }
	const std::vector<SessionDetailsKeeper> GetHandles() const { return ResultHandles; }
	EOS_HSessionSearch GetSearchHandle() const { return SearchHandle; }
	SessionDetailsKeeper GetSessionHandleById(const std::string& SessionId) const
	{
		for (size_t i = 0; i < SearchResults.size(); ++i)
		{
			if (SearchResults[i].Id == SessionId)
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
	EOS_HSessionSearch SearchHandle = nullptr;
	std::vector<FSession> SearchResults;
	std::vector<SessionDetailsKeeper> ResultHandles;
};

/**
 * Manages game sessions and matchmaking.
 */
class FSessionMatchmaking
{
public:
	/**
	 * Constructor
	 */
	FSessionMatchmaking() noexcept(false);

	/**
	 * No copying or copy assignment allowed for this class.
	 */
	FSessionMatchmaking(FSessionMatchmaking const&) = delete;
	FSessionMatchmaking& operator=(FSessionMatchmaking const&) = delete;

	/**
	 * Destructor
	 */
	virtual ~FSessionMatchmaking();

	void OnShutdown();

	void Update();

	/**
	 * Receives game event
	 *
	 * @param Event - Game event to act on
	 */
	void OnGameEvent(const FGameEvent& Event);

	/**
	 *  Read-only access to current sessions.
	 */
	const std::unordered_map<std::string, FSession>& GetCurrentSessions() const { return CurrentSessions; }

	/**
	 * Accessor to search results.
	 */
	const FSessionSearch& GetCurrentSearch() const { return CurrentSearch; }

	//Session management
	bool CreateSession(const FSession& Session);
	bool DestroySession(const std::string& Name);
	void DestroyAllSessions();
	bool HasActiveLocalSessions() const;
	bool HasPresenceSession() const;
	bool IsPresenceSession(const std::string& Id) const;
	/**
	 * Returns the PresenceSession if available; otherwise returns nullptr
	 */
	const FSession* GetPresenceSession() const;

	//Get current local session by name (if exists). Returns invalid session on error/missing.
	const FSession& GetSession(const std::string& Name);
	void StartSession(const std::string& Name);
	void EndSession(const std::string& Name);
	void JoinSession(SessionDetailsKeeper SessionHandle, bool bPresenceSession);
	
	void Register(const std::string& SessionName, EOS_ProductUserId FriendId);
	void Unregister(const std::string& SessionName, EOS_ProductUserId FriendId);

	void InviteToSession(const std::string& Name, FProductUserId FriendProductUserId);
	void RequestToJoinSession(const FProductUserId& FriendProductUserId);
	void AcceptRequestToJoin(const FProductUserId& FriendProductUserId);
	void RejectRequestToJoin(const FProductUserId& FriendProductUserId);

	//Saves session as the current one player is invited to.
	void SetInviteSession(const FSession& Session, SessionDetailsKeeper SessionHandle);
	const FSession& GetInviteSession() const { return CurrentInviteSession; }
	SessionDetailsKeeper GetInviteSessionHandle() const { return CurrentInviteSessionHandle; }

	void SubscribeToGameInvites();
	void UnsubscribeFromGameInvites();

	void SubscribeToLeaveSessionUI();
	void UnsubscribeFromLeaveSessionUI();
		
	//Search by attributes
	void Search(const std::vector<FSession::Attribute>& Attributes);
	//Search by session ID
	void SearchById(const std::string& SessionId);

	SessionDetailsKeeper GetSessionHandleFromSearch(const std::string& SessionId) const;

	// Find the session associated with the invite and create a handle for it.
	static SessionDetailsKeeper MakeSessionHandleByInviteId(const std::string& InviteId);

	static SessionDetailsKeeper MakeSessionHandleByEventId(const EOS_UI_EventId UiEventId);

	/*
	 * Modify existing session by name. Session object must contain the name of the session. It should also contain all the properties that need to be updated.
	 * Empty properties are interpret as 'no change'.
	 */
	bool ModifySession(const FSession& Session);

	void OnSessionDestroyed(const std::string& SessionName);

	/**
	 * Callback that is fired when we get response regarding session update request.
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnUpdateSessionCompleteCallback(const EOS_Sessions_UpdateSessionCallbackInfo* Data);

	/**
	 * Callback that is fired when we get response regarding session create request.
	 * The signature is the same as that used when performing a session update but this
	 * version is used only for a session create.
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnUpdateSessionCompleteCallback_ForCreate(const EOS_Sessions_UpdateSessionCallbackInfo* Data);

	/**
	 * Callback that is fired when we get response regarding session start request.
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnStartSessionCompleteCallback(const EOS_Sessions_StartSessionCallbackInfo* Data);

	/**
	 * Callback that is fired when we get response regarding session end request.
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnEndSessionCompleteCallback(const EOS_Sessions_EndSessionCallbackInfo* Data);
	
	/**
	 * Callback that is fired when we get response regarding session destroy request.
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnDestroySessionCompleteCallback(const EOS_Sessions_DestroySessionCallbackInfo* Data);

	/**
	 * Callback that is fired when we get response regarding session register request.
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnRegisterCompleteCallback(const EOS_Sessions_RegisterPlayersCallbackInfo* Data);

	/**
	 * Callback that is fired when we get response regarding session unregister request.
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnUnregisterCompleteCallback(const EOS_Sessions_UnregisterPlayersCallbackInfo* Data);

	/**
	 * Callback that is fired on search finish
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnFindSessionsCompleteCallback(const EOS_SessionSearch_FindCallbackInfo* Data);

	/**
	 * Callback that is fired on invite to session sent
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnSendInviteCompleteCallback(const EOS_Sessions_SendInviteCallbackInfo* Data);

	/**
	 * Callback that is fired on invite to session received
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnSessionInviteReceivedCallback(const EOS_Sessions_SessionInviteReceivedCallbackInfo* Data);

	/**
	 * Callback that is fired on invite to session accepted 
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnSessionInviteAcceptedCallback(const EOS_Sessions_SessionInviteAcceptedCallbackInfo* Data);

	/**
	 * Callback that is fired on invite to session rejected
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnSessionInviteRejectedCallback(const EOS_Sessions_SessionInviteRejectedCallbackInfo* Data);

	/**
	 * Callback that is fired on request to join session sent
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnSendRequestToJoinCompleteCallback(const EOS_CustomInvites_SendRequestToJoinCallbackInfo* Data);

	/**
	 * Callback that is fired after a request to join session is accepted
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnSendRequestToJoinAcceptedCallback(const EOS_CustomInvites_AcceptRequestToJoinCallbackInfo* Data);

	/**
	 * Callback that is fired after a request to join session is rejected 
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnSendRequestToJoinRejectedCallback(const EOS_CustomInvites_RejectRequestToJoinCallbackInfo* Data);

	/**
	 * Callback that is fired when a request to join session is received
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnRequestToJoinSessionReceivedCallback(const EOS_CustomInvites_RequestToJoinReceivedCallbackInfo* Data);

	/**
	 * Callback that is fired on join session
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnJoinSessionCallback(const EOS_Sessions_JoinSessionCallbackInfo* Data);

	/**
	 * Callback that is fired on join game accepted
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnPresenceJoinGameAcceptedCallback(const EOS_Presence_JoinGameAcceptedCallbackInfo* Data);

	/**
	 * Callback that is fired on join game accepted
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnSessionsJoinSessionAcceptedCallback(const EOS_Sessions_JoinSessionAcceptedCallbackInfo* Data);

	/**
	 * Callback that is fired when set presence is completed
	 *
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnSetPresenceCallback(const EOS_Presence_SetPresenceCallbackInfo* Data);

	/**
	 * Callback that is fired when leave session is requested from UI.
	 * 
	 * @param Data - out parameter
	 */
	static void EOS_CALL OnLeaveSessionRequestedCallback(const EOS_Sessions_LeaveSessionRequestedCallbackInfo* Data);
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

	void OnSessionUpdateFinished(bool bSuccess, const std::string& Name, const std::string& SessionId, bool bRemoveSessionOnFailure = false);
	void OnSearchResultsReceived();
	void SetJoiningSessionDetails(SessionDetailsKeeper NewDetails);
	void OnJoinSessionFinished();
	void OnSessionStarted(const std::string& Name);
	void OnSessionEnded(const std::string& Name);
	void SetJoinInfo(const std::string& SessionId);
	void OnJoinGameAcceptedByJoinInfo(const std::string& LocationString, EOS_UI_EventId UiEventId);
	void OnJoinGameAcceptedByEventId(EOS_UI_EventId UiEventId);
	void JoinPresenceSessionById(const std::string& SessionId);
	void AcknowledgeEventId(EOS_EResult Result);

	std::string GenerateJoinedSessionName(bool bNoIncrement = false);

	std::unordered_map<std::string, FSession> CurrentSessions;

	FSessionSearch CurrentSearch;
	// The ID which is being joined as a presence session.  Set as a reaction to Social Overlay buttons.
	std::string JoinPresenceSessionId;
	// The Correlation ID which was provided by the UI.
	EOS_UI_EventId JoinUiEvent;
	// The known presence session ID found during a `HasPresenceSession` check.
	mutable std::string KnownPresenceSessionId;

	//The session current user is invited to. Only one invite session at a time is supported.
	FSession CurrentInviteSession = FSession::InvalidSession;
	SessionDetailsKeeper CurrentInviteSessionHandle = nullptr;

	EOS_NotificationId SessionInviteNotificationHandle = EOS_INVALID_NOTIFICATIONID;
	EOS_NotificationId SessionInviteAcceptedNotificationHandle = EOS_INVALID_NOTIFICATIONID;
	EOS_NotificationId SessionInviteRejectedNotificationHandle = EOS_INVALID_NOTIFICATIONID;
	EOS_NotificationId JoinGameNotificationHandle = EOS_INVALID_NOTIFICATIONID;
	EOS_NotificationId SessionJoinGameNotificationHandle = EOS_INVALID_NOTIFICATIONID;
	EOS_NotificationId RequestToJoinSessionReceivedNotificationHandle = EOS_INVALID_NOTIFICATIONID;
	EOS_NotificationId LeaveSessionRequestedNotificationHandle = EOS_INVALID_NOTIFICATIONID;

	SessionDetailsKeeper JoiningSessionDetails = nullptr;
	size_t JoinedSessionIndex = 0;

	uint32_t RestrictedPlatform;
};
