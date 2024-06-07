// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "DebugLog.h"
#include "StringUtils.h"
#include "AccountHelpers.h"
#include "Game.h"
#include "GameEvent.h"
#include "Main.h"
#include "Platform.h"
#include "Users.h"
#include "Player.h"
#include "SessionMatchmaking.h"
#include <eos_sdk.h>
#include <eos_sessions.h>
#include <eos_presence.h>
#include <eos_ui.h>
#include <eos_custominvites.h>

#include <regex>

//////////////////////////////////////////////////////////////////////////
// When set to 0 then the overlay will use the EOS Sessions Matchmaking service
// provided by the SDK for managing the game associated with presence and the overlay.
//
// Set to 1 to make the sample use the JoinInfo string with the overlay.
// This will demonstrate how to use a custom matchmaking service.  
// In this case the "custom matchmaking service" would be EOS Sessions Matchmaking.
//////////////////////////////////////////////////////////////////////////
#define USE_JOIN_INFO_WITH_OVERLAY 0

constexpr const char* SampleJoinInfoFormat = R"({"SessionId": "%s"})";
constexpr const char* SampleJoinInfoRegex = R"(\{\"SessionId\"\s*:\s*\"(.*)\"\})";
constexpr const char* JoinedSessionName = "Session#";
constexpr const size_t JoinedSessionNameRotationNum = 9;
constexpr const char* BucketId = "SessionSample:Region";

bool FSession::InitFromInfoOfSessionDetails(SessionDetailsKeeper SessionDetails)
{
	//retrieve info about session
	EOS_SessionDetails_Info* SessionInfo;

	EOS_SessionDetails_CopyInfoOptions CopyOptions = {};
	CopyOptions.ApiVersion = EOS_SESSIONDETAILS_COPYINFO_API_LATEST;
	EOS_EResult CopyResult = EOS_SessionDetails_CopyInfo(SessionDetails.get(), &CopyOptions, &SessionInfo);
	if (CopyResult != EOS_EResult::EOS_Success)
	{
		return false;
	}

	InitFromSessionInfo(SessionDetails.get(), SessionInfo);
	EOS_SessionDetails_Info_Release(SessionInfo);
	return true;
}

void FSession::InitFromSessionInfo(EOS_HSessionDetails SessionDetails, EOS_SessionDetails_Info* SessionInfo)
{
	if (SessionInfo && SessionInfo->Settings)
	{
		//copy session info
		bAllowJoinInProgress = SessionInfo->Settings->bAllowJoinInProgress;
		BucketId = SessionInfo->Settings->BucketId;
		PermissionLevel = SessionInfo->Settings->PermissionLevel;
		MaxPlayers = SessionInfo->Settings->NumPublicConnections;
		NumConnections = MaxPlayers - SessionInfo->NumOpenPublicConnections;
		Id = SessionInfo->SessionId;
		bPresenceSession = FGame::Get().GetSessions()->IsPresenceSession(Id);
	}

	//get attributes
	Attributes.clear();
	EOS_SessionDetails_GetSessionAttributeCountOptions CountOptions = {};
	CountOptions.ApiVersion = EOS_SESSIONDETAILS_GETSESSIONATTRIBUTECOUNT_API_LATEST;
	uint32_t AttrCount = EOS_SessionDetails_GetSessionAttributeCount(SessionDetails, &CountOptions);

	for (uint32_t AttrIndex = 0; AttrIndex < AttrCount; ++AttrIndex)
	{
		EOS_SessionDetails_CopySessionAttributeByIndexOptions AttrOptions = {};
		AttrOptions.ApiVersion = EOS_SESSIONDETAILS_COPYSESSIONATTRIBUTEBYINDEX_API_LATEST;
		AttrOptions.AttrIndex = AttrIndex;

		EOS_SessionDetails_Attribute* Attr = NULL;
		EOS_EResult AttrCopyResult = EOS_SessionDetails_CopySessionAttributeByIndex(SessionDetails, &AttrOptions, &Attr);
		if (AttrCopyResult == EOS_EResult::EOS_Success && Attr && Attr->Data)
		{
			FSession::Attribute NextAttribute;
			NextAttribute.Advertisement = Attr->AdvertisementType;
			NextAttribute.Key = Attr->Data->Key;
			switch (Attr->Data->ValueType)
			{
			case EOS_ESessionAttributeType::EOS_SAT_Boolean:
				NextAttribute.ValueType = FSession::Attribute::Bool;
				NextAttribute.AsBool = Attr->Data->Value.AsBool;
				break;
			case EOS_ESessionAttributeType::EOS_SAT_Int64:
				NextAttribute.ValueType = FSession::Attribute::Int64;
				NextAttribute.AsInt64 = Attr->Data->Value.AsInt64;
				break;
			case EOS_ESessionAttributeType::EOS_SAT_Double:
				NextAttribute.ValueType = FSession::Attribute::Double;
				NextAttribute.AsDouble = Attr->Data->Value.AsDouble;
				break;
			case EOS_ESessionAttributeType::EOS_AT_STRING:
				NextAttribute.ValueType = FSession::Attribute::String;
				NextAttribute.AsString = Attr->Data->Value.AsUtf8;
				break;
			}

			Attributes.push_back(NextAttribute);
		}

		EOS_SessionDetails_Attribute_Release(Attr);
	}

	InitActiveSession();

	bUpdateInProgress = false;
}

void FSession::InitActiveSession()
{
	if (!ActiveSession)
	{
		if (!Name.empty())
		{
			EOS_HSessions SessionsHandle = EOS_Platform_GetSessionsInterface(FPlatform::GetPlatformHandle());
			EOS_Sessions_CopyActiveSessionHandleOptions CopyASOptions = {};
			CopyASOptions.ApiVersion = EOS_SESSIONS_COPYACTIVESESSIONHANDLE_API_LATEST;
			CopyASOptions.SessionName = Name.c_str();
			EOS_HActiveSession ActiveSessionHandle;
			if (EOS_Sessions_CopyActiveSessionHandle(SessionsHandle, &CopyASOptions, &ActiveSessionHandle) != EOS_EResult::EOS_Success)
			{
				FDebugLog::LogError(L"Session Matchmaking: could not get ActiveSession for name: %ls.", FStringUtils::Widen(Name).c_str());
			}
			else
			{
				ActiveSession = MakeActiveSessionKeeper(ActiveSessionHandle);
			}
		}
	}
}

FSession FSession::InvalidSession = FSession();

FSessionSearch::~FSessionSearch()
{
	Release();
}

void FSessionSearch::Release()
{
	SearchResults.clear();

	if (SearchHandle)
	{
		EOS_SessionSearch_Release(SearchHandle);
		SearchHandle = nullptr;
	}

	ResultHandles.clear();
}

void FSessionSearch::SetNewSearch(EOS_HSessionSearch Handle)
{
	Release();
	SearchHandle = Handle;
}

void FSessionSearch::OnSearchResultsReceived(std::vector<FSession>&& Results, std::vector<SessionDetailsKeeper>&& Handles)
{
	SearchResults.swap(Results);
	ResultHandles.swap(Handles);
}


FSessionMatchmaking::FSessionMatchmaking()
{
	
}

FSessionMatchmaking::~FSessionMatchmaking()
{

}

void FSessionMatchmaking::OnShutdown()
{
	DestroyAllSessions();
	UnsubscribeFromGameInvites();
	UnsubscribeFromLeaveSessionUI();
}

void FSessionMatchmaking::Update()
{
	//Update active session from time to time
	for (std::pair<const std::string, FSession>& NextSession : CurrentSessions)
	{
		if (!NextSession.first.empty() &&
			NextSession.second.IsValid())
		{
			FSession& Session = NextSession.second;
			if (Session.bUpdateInProgress)
			{
				continue;
			}

			if (Session.ActiveSession != nullptr)
			{
				EOS_HSessions SessionsHandle = EOS_Platform_GetSessionsInterface(FPlatform::GetPlatformHandle());
				EOS_ActiveSession_CopyInfoOptions CopyInfoOptions = {};
				CopyInfoOptions.ApiVersion = EOS_ACTIVESESSION_COPYINFO_API_LATEST;
				EOS_ActiveSession_Info* ActiveSessionInfo = nullptr;
				EOS_EResult Result = EOS_ActiveSession_CopyInfo(Session.ActiveSession.get(), &CopyInfoOptions, &ActiveSessionInfo);
				if (Result == EOS_EResult::EOS_Success)
				{
					if (ActiveSessionInfo)
					{
						Session.SessionState = ActiveSessionInfo->State;

						EOS_ActiveSession_Info_Release(ActiveSessionInfo);
					}
				}
				else
				{
					FDebugLog::LogError(L"Session Matchmaking: EOS_ActiveSession_CopyInfo failed. Errors code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
				}
			}

		}
	}
}

void FSessionMatchmaking::OnLoggedIn(FEpicAccountId UserId)
{
	SetJoinInfo("");
}

void FSessionMatchmaking::OnLoggedOut(FEpicAccountId UserId)
{
	SetJoinInfo("");
	CurrentSearch.Release();
	CurrentSessions.clear();
	CurrentInviteSessionHandle.reset();
	JoiningSessionDetails.reset();
}

void FSessionMatchmaking::OnUserConnectLoggedIn(FProductUserId ProductUserId)
{

}

void FSessionMatchmaking::OnGameEvent(const FGameEvent& Event)
{
	if (Event.GetType() == EGameEventType::UserLoggedIn)
	{
		FEpicAccountId UserId = Event.GetUserId();
		OnLoggedIn(UserId);
	}
	else if (Event.GetType() == EGameEventType::UserLoggedOut)
	{
		FEpicAccountId UserId = Event.GetUserId();
		OnLoggedOut(UserId);
	}
	else if (Event.GetType() == EGameEventType::UserConnectLoggedIn)
	{
		FProductUserId ProductUserId = Event.GetProductUserId();
		OnUserConnectLoggedIn(ProductUserId);
	}
	else if (Event.GetType() == EGameEventType::UserLogOutTriggered)
	{
		DestroyAllSessions();
	}
}

bool FSessionMatchmaking::CreateSession(const FSession& Session)
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player == nullptr)
	{
		FDebugLog::LogError(L"Session Matchmaking - CreateSession: Current player is invalid!");
		return false;
	}

	EOS_HSessions SessionsHandle = EOS_Platform_GetSessionsInterface(FPlatform::GetPlatformHandle());
	if (!SessionsHandle)
	{
		FDebugLog::LogError(L"Session Matchmaking: can't get sessions interface.");
		return false;
	}

	EOS_Sessions_CreateSessionModificationOptions CreateOptions = {};
	CreateOptions.ApiVersion = EOS_SESSIONS_CREATESESSIONMODIFICATION_API_LATEST;

	// The top - level, game - specific filtering information for session searches. This criteria should be set with mostly static, coarse settings, often formatted like "GameMode:Region:MapName".
	CreateOptions.BucketId = BucketId; 

	CreateOptions.MaxPlayers = Session.MaxPlayers;
	CreateOptions.SessionName = Session.Name.c_str();
	CreateOptions.LocalUserId = Player->GetProductUserID();
	CreateOptions.bPresenceEnabled = Session.bPresenceSession;
	CreateOptions.bSanctionsEnabled = EOS_FALSE;
	if (Session.RestrictedPlatform != ERestrictedPlatformType::Unrestricted)
	{
		RestrictedPlatform = static_cast<uint32_t>(Session.RestrictedPlatform);
		CreateOptions.AllowedPlatformIds = &RestrictedPlatform;
		CreateOptions.AllowedPlatformIdsCount = 1;
	}

	EOS_HSessionModification ModificationHandle = NULL;
	EOS_EResult CreateResult = EOS_Sessions_CreateSessionModification(SessionsHandle, &CreateOptions, &ModificationHandle);
	if (CreateResult != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"Session Matchmaking: could not create session modification. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(CreateResult)).c_str());
		return false;
	}


	EOS_SessionModification_SetPermissionLevelOptions PermOptions = {};
	PermOptions.ApiVersion = EOS_SESSIONMODIFICATION_SETPERMISSIONLEVEL_API_LATEST;
	PermOptions.PermissionLevel = Session.PermissionLevel;
	EOS_EResult SetPermsResult = EOS_SessionModification_SetPermissionLevel(ModificationHandle, &PermOptions);
	if (SetPermsResult != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"Session Matchmaking: failed to set permissions. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(SetPermsResult)).c_str());
		EOS_SessionModification_Release(ModificationHandle);
		return false;
	}

	EOS_SessionModification_SetJoinInProgressAllowedOptions JIPOptions = {};
	JIPOptions.ApiVersion = EOS_SESSIONMODIFICATION_SETJOININPROGRESSALLOWED_API_LATEST;
	JIPOptions.bAllowJoinInProgress = (Session.bAllowJoinInProgress) ? EOS_TRUE : EOS_FALSE;
	EOS_EResult SetJIPResult = EOS_SessionModification_SetJoinInProgressAllowed(ModificationHandle, &JIPOptions);
	if (SetJIPResult != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"Session Matchmaking: failed to set 'join in progress allowed' flag. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(SetJIPResult)).c_str());
		EOS_SessionModification_Release(ModificationHandle);
		return false;
	}

	EOS_SessionModification_SetInvitesAllowedOptions IAOptions = {};
	IAOptions.ApiVersion = EOS_SESSIONMODIFICATION_SETINVITESALLOWED_API_LATEST;
	IAOptions.bInvitesAllowed = (Session.bInvitesAllowed) ? EOS_TRUE : EOS_FALSE;
	EOS_EResult SetIAResult = EOS_SessionModification_SetInvitesAllowed(ModificationHandle, &IAOptions);
	if (SetIAResult != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"Session Matchmaking: failed to set invites allowed. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(SetIAResult)).c_str());
		EOS_SessionModification_Release(ModificationHandle);
		return false;
	}


	EOS_Sessions_AttributeData AttrData;
	AttrData.ApiVersion = EOS_SESSIONS_SESSIONATTRIBUTEDATA_API_LATEST;

	EOS_SessionModification_AddAttributeOptions AttrOptions = {};
	AttrOptions.ApiVersion = EOS_SESSIONMODIFICATION_ADDATTRIBUTE_API_LATEST;
	AttrOptions.SessionAttribute = &AttrData;

	//Set bucket id
	AttrData.Key = EOS_SESSIONS_SEARCH_BUCKET_ID;
	AttrData.Value.AsUtf8 = BucketId;
	AttrData.ValueType = EOS_EAttributeType::EOS_AT_STRING;
	EOS_EResult SetAttrResult = EOS_SessionModification_AddAttribute(ModificationHandle, &AttrOptions);
	if (SetAttrResult != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"Session Matchmaking: failed to set a bucket id attribute. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(SetAttrResult)).c_str());
		EOS_SessionModification_Release(ModificationHandle);
		return false;
	}

	//Set other attributes
	for (const FSession::Attribute& NextAttribute : Session.Attributes)
	{
		AttrData.Key = NextAttribute.Key.c_str();

		switch (NextAttribute.ValueType)
		{
		case FSession::Attribute::Bool:
			AttrData.ValueType = EOS_ESessionAttributeType::EOS_SAT_Boolean;
			AttrData.Value.AsBool = NextAttribute.AsBool;
			break;
		case FSession::Attribute::Double:
			AttrData.ValueType = EOS_ESessionAttributeType::EOS_SAT_Double;
			AttrData.Value.AsDouble = NextAttribute.AsDouble;
			break;
		case FSession::Attribute::Int64:
			AttrData.ValueType = EOS_ESessionAttributeType::EOS_SAT_Int64;
			AttrData.Value.AsInt64 = NextAttribute.AsInt64;
			break;
		case FSession::Attribute::String:
			AttrData.ValueType = EOS_ESessionAttributeType::EOS_AT_STRING;
			AttrData.Value.AsUtf8 = NextAttribute.AsString.c_str();
			break;
		}

		AttrOptions.AdvertisementType = NextAttribute.Advertisement;
		EOS_EResult SetAttrResult = EOS_SessionModification_AddAttribute(ModificationHandle, &AttrOptions);
		if (SetAttrResult != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Session Matchmaking: failed to set an attribute: %ls. Error code: %ls", NextAttribute.Key.c_str(), FStringUtils::Widen(EOS_EResult_ToString(SetAttrResult)).c_str());
			EOS_SessionModification_Release(ModificationHandle);
			return false;
		}
	}


	EOS_Sessions_UpdateSessionOptions UpdateOptions = {};
	UpdateOptions.ApiVersion = EOS_SESSIONS_UPDATESESSION_API_LATEST;
	UpdateOptions.SessionModificationHandle = ModificationHandle;
	EOS_Sessions_UpdateSession(SessionsHandle, &UpdateOptions, nullptr, OnUpdateSessionCompleteCallback_ForCreate);

	EOS_SessionModification_Release(ModificationHandle);

	CurrentSessions[Session.Name] = Session;
	CurrentSessions[Session.Name].bUpdateInProgress = true;

	return true;
}

bool FSessionMatchmaking::DestroySession(const std::string& Name)
{
	EOS_HSessions SessionsHandle = EOS_Platform_GetSessionsInterface(FPlatform::GetPlatformHandle());

	EOS_Sessions_DestroySessionOptions DestroyOptions = {};
	DestroyOptions.ApiVersion = EOS_SESSIONS_DESTROYSESSION_API_LATEST;
	DestroyOptions.SessionName = Name.c_str();

	EOS_Sessions_DestroySession(SessionsHandle, &DestroyOptions, new std::string(Name), OnDestroySessionCompleteCallback);

	return true;
}

void FSessionMatchmaking::DestroyAllSessions()
{
	SetJoinInfo("");
	for (const std::pair<std::string, FSession>& NextSession : CurrentSessions)
	{
		//We can't remove joined session
		if (NextSession.first.find(JoinedSessionName) == std::string::npos)
		{
			DestroySession(NextSession.first);
		}
	}
}

bool FSessionMatchmaking::HasActiveLocalSessions() const
{
	for (const std::pair<std::string, FSession>& NextSession : CurrentSessions)
	{
		if (NextSession.first.find(JoinedSessionName) == std::string::npos)
		{
			return true;
		}
	}
	return false;
}

bool FSessionMatchmaking::HasPresenceSession() const
{
	if (KnownPresenceSessionId.length() > 0)
	{
		if (CurrentSessions.find(KnownPresenceSessionId) != CurrentSessions.end())
		{
			return true;
		}
		KnownPresenceSessionId = std::string();
	}

	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player != nullptr)
	{
		FProductUserId CurrentProductUserId = Player->GetProductUserID();
		if (!CurrentProductUserId.IsValid())
		{
			return false;
		}
	}

	EOS_HSessions SessionsHandle = EOS_Platform_GetSessionsInterface(FPlatform::GetPlatformHandle());

	EOS_HSessionDetails SessionDetails = nullptr;

	EOS_Sessions_CopySessionHandleForPresenceOptions CopyOptions = {};
	CopyOptions.ApiVersion = EOS_SESSIONS_COPYSESSIONHANDLEFORPRESENCE_API_LATEST;
	CopyOptions.LocalUserId = Player->GetProductUserID();

	EOS_EResult CopyResult = EOS_Sessions_CopySessionHandleForPresence(SessionsHandle, &CopyOptions, &SessionDetails);
	if (CopyResult != EOS_EResult::EOS_Success)
	{
		return false;
	}

	if (!SessionDetails)
	{
		return false;
	}
	SessionDetailsKeeper  LocalSessionKeeper = MakeSessionDetailsKeeper(SessionDetails);

	EOS_SessionDetails_Info* SessionInfo = nullptr;
	EOS_SessionDetails_CopyInfoOptions CopyInfoOptions = {};
	CopyInfoOptions.ApiVersion = EOS_SESSIONDETAILS_COPYINFO_API_LATEST;
	CopyResult = EOS_SessionDetails_CopyInfo(SessionDetails, &CopyInfoOptions, &SessionInfo);
	if (CopyResult != EOS_EResult::EOS_Success)
	{
		return false;
	}

	KnownPresenceSessionId = SessionInfo->SessionId;
	return true;
}

bool FSessionMatchmaking::IsPresenceSession(const std::string& Id) const
{
	return HasPresenceSession() && Id == KnownPresenceSessionId;
}

const FSession* FSessionMatchmaking::GetPresenceSession() const
{
	if (!HasPresenceSession())
	{
		return nullptr;
	}

	for (const std::pair<const std::string, FSession>& CurrentSessionEntry : CurrentSessions)
	{
		if (CurrentSessionEntry.second.Id == KnownPresenceSessionId)
		{
			return &CurrentSessionEntry.second;
		}
	}

	return nullptr;
}

const FSession& FSessionMatchmaking::GetSession(const std::string& Name)
{
	auto iter = CurrentSessions.find(Name);
	if (iter != CurrentSessions.end())
	{
		return iter->second;
	}

	return FSession::InvalidSession;
}

void FSessionMatchmaking::StartSession(const std::string& Name)
{
	EOS_HSessions SessionsHandle = EOS_Platform_GetSessionsInterface(FPlatform::GetPlatformHandle());

	//search for current session by name
	auto iter = CurrentSessions.find(Name);
	if (iter == CurrentSessions.end())
	{
		FDebugLog::LogError(L"Session Matchmaking: can't start session: no active session with specified name.");
		return;
	}

	EOS_Sessions_StartSessionOptions StartSessionOptions = {};
	StartSessionOptions.ApiVersion = EOS_SESSIONS_STARTSESSION_API_LATEST;
	StartSessionOptions.SessionName = Name.c_str();

	std::string* SessionNamePtr = new std::string(Name);
	EOS_Sessions_StartSession(SessionsHandle, &StartSessionOptions, SessionNamePtr, OnStartSessionCompleteCallback);
}

void FSessionMatchmaking::EndSession(const std::string& Name)
{
	EOS_HSessions SessionsHandle = EOS_Platform_GetSessionsInterface(FPlatform::GetPlatformHandle());

	//search for current session by name
	auto iter = CurrentSessions.find(Name);
	if (iter == CurrentSessions.end())
	{
		FDebugLog::LogError(L"Session Matchmaking: can't end session: no active session with specified name.");
		return;
	}

	EOS_Sessions_EndSessionOptions EndSessionOptions = {};
	EndSessionOptions.ApiVersion = EOS_SESSIONS_ENDSESSION_API_LATEST;
	EndSessionOptions.SessionName = Name.c_str();

	std::string* SessionNamePtr = new std::string(Name);
	EOS_Sessions_EndSession(SessionsHandle, &EndSessionOptions, SessionNamePtr, OnEndSessionCompleteCallback);
}

void FSessionMatchmaking::Register(const std::string& SessionName, EOS_ProductUserId FriendId)
{
	EOS_HSessions SessionsHandle = EOS_Platform_GetSessionsInterface(FPlatform::GetPlatformHandle());

	EOS_Sessions_RegisterPlayersOptions RegisterPlayersOptions = {};
	RegisterPlayersOptions.ApiVersion = EOS_SESSIONS_REGISTERPLAYERS_API_LATEST;
	RegisterPlayersOptions.SessionName = SessionName.c_str();
	RegisterPlayersOptions.PlayersToRegisterCount = 1;
	RegisterPlayersOptions.PlayersToRegister = &FriendId;

	EOS_Sessions_RegisterPlayers(SessionsHandle, &RegisterPlayersOptions, nullptr, OnRegisterCompleteCallback);
}

void FSessionMatchmaking::Unregister(const std::string& SessionName, EOS_ProductUserId FriendId)
{
	EOS_HSessions SessionsHandle = EOS_Platform_GetSessionsInterface(FPlatform::GetPlatformHandle());

	EOS_Sessions_UnregisterPlayersOptions UnregisterPlayersOptions = {};
	UnregisterPlayersOptions.ApiVersion = EOS_SESSIONS_UNREGISTERPLAYERS_API_LATEST;
	UnregisterPlayersOptions.SessionName = SessionName.c_str();
	UnregisterPlayersOptions.PlayersToUnregisterCount = 1;
	UnregisterPlayersOptions.PlayersToUnregister = &FriendId;

	EOS_Sessions_UnregisterPlayers(SessionsHandle, &UnregisterPlayersOptions, nullptr, OnUnregisterCompleteCallback);
}

void FSessionMatchmaking::InviteToSession(const std::string& Name, FProductUserId FriendProductUserId)
{
	if (!FriendProductUserId.IsValid())
	{
		FDebugLog::LogError(L"Session Matchmaking - InviteToSession: friend's product user id is invalid!");
		return;
	}

	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player != nullptr)
	{
		FProductUserId CurrentProductUserId = Player->GetProductUserID();
		if (!CurrentProductUserId.IsValid())
		{
			FDebugLog::LogError(L"Session Matchmaking - InviteToSession: current user's product user id is invalid!");
			return;
		}

		EOS_HSessions SessionsHandle = EOS_Platform_GetSessionsInterface(FPlatform::GetPlatformHandle());

		EOS_Sessions_SendInviteOptions SendInviteOptions = {};
		SendInviteOptions.ApiVersion = EOS_SESSIONS_SENDINVITE_API_LATEST;
		SendInviteOptions.LocalUserId = Player->GetProductUserID();
		SendInviteOptions.TargetUserId = FriendProductUserId;
		SendInviteOptions.SessionName = Name.c_str();
		EOS_Sessions_SendInvite(SessionsHandle, &SendInviteOptions, nullptr, OnSendInviteCompleteCallback);
	}
	else
	{
		FDebugLog::LogError(L"Session Matchmaking - InviteToSession: Current player is invalid!");
	}
}

void FSessionMatchmaking::RequestToJoinSession(const FProductUserId& FriendProductUserId)
{
	if (!FriendProductUserId.IsValid())
	{
		FDebugLog::LogError(L"Session Matchmaking - RequestToJoinSession: friend's product user id is invalid!");
		return;
	}

	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player != nullptr)
	{
		FProductUserId CurrentProductUserId = Player->GetProductUserID();
		if (!CurrentProductUserId.IsValid())
		{
			FDebugLog::LogError(L"Session Matchmaking - RequestToJoinSession: current user's product user id is invalid!");
			return;
		}

		EOS_HCustomInvites CustomInvitesHandle = EOS_Platform_GetCustomInvitesInterface(FPlatform::GetPlatformHandle());
		EOS_CustomInvites_SendRequestToJoinOptions Options = {};
		Options.ApiVersion = EOS_CUSTOMINVITES_SENDREQUESTTOJOIN_API_LATEST;
		Options.LocalUserId = Player->GetProductUserID();
		Options.TargetUserId = FriendProductUserId;

		EOS_CustomInvites_SendRequestToJoin(CustomInvitesHandle, &Options, nullptr, OnSendRequestToJoinCompleteCallback);
	}
	else
	{
		FDebugLog::LogError(L"Session Matchmaking - RequestToJoinSession: Current player is invalid!");
	}
}

void FSessionMatchmaking::AcceptRequestToJoin(const FProductUserId& FriendProductUserId)
{
	if (!FriendProductUserId.IsValid())
	{
		FDebugLog::LogError(L"Session Matchmaking - AcceptRequestToJoin: friend's product user id is invalid!");
		return;
	}

	const FSession* PresenceSession = GetPresenceSession();
	if (PresenceSession == nullptr)
	{
		FDebugLog::LogError(L"Session Matchmaking - AcceptRequestToJoin: attempted to accept a request to join but there is no presence session available from which to send an invite. This request to join will not be deleted.");
		return;
	}

	std::string PresenceSessionName = PresenceSession->Name;

	if (PresenceSessionName.empty())
	{
		FDebugLog::LogError(L"Session Matchmaking - AcceptRequestToJoin: attempted to accept a request to join but there is no presence session available from which to send an invite. This request to join will not be deleted.");
		return;
	}

	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player != nullptr)
	{
		FProductUserId CurrentProductUserId = Player->GetProductUserID();
		if (!CurrentProductUserId.IsValid())
		{
			FDebugLog::LogError(L"Session Matchmaking - AcceptRequestToJoin: current user's product user id is invalid!");
			return;
		}

		EOS_HCustomInvites CustomInvitesHandle = EOS_Platform_GetCustomInvitesInterface(FPlatform::GetPlatformHandle());
		EOS_CustomInvites_AcceptRequestToJoinOptions Options = {};
		Options.ApiVersion = EOS_CUSTOMINVITES_ACCEPTREQUESTTOJOIN_API_LATEST;
		Options.LocalUserId = Player->GetProductUserID();
		Options.TargetUserId = FriendProductUserId;

		EOS_CustomInvites_AcceptRequestToJoin(CustomInvitesHandle, &Options, nullptr, OnSendRequestToJoinAcceptedCallback);

		InviteToSession(PresenceSessionName, FriendProductUserId);
	}
	else
	{
		FDebugLog::LogError(L"Session Matchmaking - RequestToJoinSession: Current player is invalid!");
	}
}

void FSessionMatchmaking::RejectRequestToJoin(const FProductUserId& FriendProductUserId)
{
	if (!FriendProductUserId.IsValid())
	{
		FDebugLog::LogError(L"Session Matchmaking - RejectRequestToJoin: friend's product user id is invalid!");
		return;
	}

	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player != nullptr)
	{
		FProductUserId CurrentProductUserId = Player->GetProductUserID();
		if (!CurrentProductUserId.IsValid())
		{
			FDebugLog::LogError(L"Session Matchmaking - RejectRequestToJoin: current user's product user id is invalid!");
			return;
		}

		EOS_HCustomInvites CustomInvitesHandle = EOS_Platform_GetCustomInvitesInterface(FPlatform::GetPlatformHandle());
		EOS_CustomInvites_RejectRequestToJoinOptions Options = {};
		Options.ApiVersion = EOS_CUSTOMINVITES_REJECTREQUESTTOJOIN_API_LATEST;
		Options.LocalUserId = Player->GetProductUserID();
		Options.TargetUserId = FriendProductUserId;

		EOS_CustomInvites_RejectRequestToJoin(CustomInvitesHandle, &Options, nullptr, OnSendRequestToJoinRejectedCallback);
	}
	else
	{
		FDebugLog::LogError(L"Session Matchmaking - RejectRequestToJoin: Current player is invalid!");
	}
}

void FSessionMatchmaking::SetInviteSession(const FSession& Session, SessionDetailsKeeper SessionHandle)
{
	CurrentInviteSession = Session;
	CurrentInviteSessionHandle = SessionHandle;
}

void FSessionMatchmaking::SubscribeToGameInvites()
{
	EOS_HSessions SessionsHandle = EOS_Platform_GetSessionsInterface(FPlatform::GetPlatformHandle());
	EOS_HPresence PresenceHandle = EOS_Platform_GetPresenceInterface(FPlatform::GetPlatformHandle());
	EOS_HCustomInvites CustomInvitesHandle = EOS_Platform_GetCustomInvitesInterface(FPlatform::GetPlatformHandle());

	EOS_Sessions_AddNotifySessionInviteReceivedOptions InviteNotifyOptions = {};
	InviteNotifyOptions.ApiVersion = EOS_SESSIONS_ADDNOTIFYSESSIONINVITERECEIVED_API_LATEST;
	SessionInviteNotificationHandle = EOS_Sessions_AddNotifySessionInviteReceived(SessionsHandle, &InviteNotifyOptions, nullptr, OnSessionInviteReceivedCallback);

	EOS_Sessions_AddNotifySessionInviteAcceptedOptions InviteAcceptOptions = {};
	InviteAcceptOptions.ApiVersion = EOS_SESSIONS_ADDNOTIFYSESSIONINVITEACCEPTED_API_LATEST;
	SessionInviteAcceptedNotificationHandle = EOS_Sessions_AddNotifySessionInviteAccepted(SessionsHandle, &InviteAcceptOptions, nullptr, OnSessionInviteAcceptedCallback);

	EOS_Sessions_AddNotifySessionInviteRejectedOptions InviteRejectOptions = {};
	InviteRejectOptions.ApiVersion = EOS_SESSIONS_ADDNOTIFYSESSIONINVITEREJECTED_API_LATEST;
	SessionInviteRejectedNotificationHandle = EOS_Sessions_AddNotifySessionInviteRejected(SessionsHandle, &InviteRejectOptions, nullptr, OnSessionInviteRejectedCallback);

	EOS_Presence_AddNotifyJoinGameAcceptedOptions JoinGameAcceptedOptions = {};
	JoinGameAcceptedOptions.ApiVersion = EOS_PRESENCE_ADDNOTIFYJOINGAMEACCEPTED_API_LATEST;
	JoinGameNotificationHandle = EOS_Presence_AddNotifyJoinGameAccepted(PresenceHandle, &JoinGameAcceptedOptions, nullptr, OnPresenceJoinGameAcceptedCallback);

	EOS_Sessions_AddNotifyJoinSessionAcceptedOptions SessionJoinSessionAcceptedOptions = {};
	SessionJoinSessionAcceptedOptions.ApiVersion = EOS_SESSIONS_ADDNOTIFYJOINSESSIONACCEPTED_API_LATEST;
	SessionJoinGameNotificationHandle = EOS_Sessions_AddNotifyJoinSessionAccepted(SessionsHandle, &SessionJoinSessionAcceptedOptions, nullptr, OnSessionsJoinSessionAcceptedCallback);

	EOS_CustomInvites_AddNotifyRequestToJoinReceivedOptions RequestToJoinSessionReceivedOptions = {};
	RequestToJoinSessionReceivedOptions.ApiVersion = EOS_CUSTOMINVITES_ADDNOTIFYREQUESTTOJOINRECEIVED_API_LATEST;
	RequestToJoinSessionReceivedNotificationHandle = EOS_CustomInvites_AddNotifyRequestToJoinReceived(CustomInvitesHandle, &RequestToJoinSessionReceivedOptions, nullptr, OnRequestToJoinSessionReceivedCallback);
}

void FSessionMatchmaking::UnsubscribeFromGameInvites()
{
	EOS_HSessions SessionsHandle = EOS_Platform_GetSessionsInterface(FPlatform::GetPlatformHandle());
	EOS_HPresence PresenceHandle = EOS_Platform_GetPresenceInterface(FPlatform::GetPlatformHandle());

	if (SessionInviteNotificationHandle != EOS_INVALID_NOTIFICATIONID)
	{
		EOS_Sessions_RemoveNotifySessionInviteReceived(SessionsHandle, SessionInviteNotificationHandle);
		SessionInviteNotificationHandle = EOS_INVALID_NOTIFICATIONID;
	}	
	if (SessionInviteAcceptedNotificationHandle != EOS_INVALID_NOTIFICATIONID)
	{
		EOS_Sessions_RemoveNotifySessionInviteAccepted(SessionsHandle, SessionInviteAcceptedNotificationHandle);
		SessionInviteAcceptedNotificationHandle = EOS_INVALID_NOTIFICATIONID;
	}
	if (SessionInviteRejectedNotificationHandle != EOS_INVALID_NOTIFICATIONID)
	{
		EOS_Sessions_RemoveNotifySessionInviteRejected(SessionsHandle, SessionInviteRejectedNotificationHandle);
		SessionInviteRejectedNotificationHandle = EOS_INVALID_NOTIFICATIONID;
	}
	if (JoinGameNotificationHandle != EOS_INVALID_NOTIFICATIONID)
	{
		EOS_Presence_RemoveNotifyJoinGameAccepted(PresenceHandle, JoinGameNotificationHandle);
		JoinGameNotificationHandle = EOS_INVALID_NOTIFICATIONID;
	}
	if (SessionJoinGameNotificationHandle != EOS_INVALID_NOTIFICATIONID)
	{
		EOS_Sessions_RemoveNotifyJoinSessionAccepted(SessionsHandle, SessionJoinGameNotificationHandle);
		SessionJoinGameNotificationHandle = EOS_INVALID_NOTIFICATIONID;
	}
}

void FSessionMatchmaking::SubscribeToLeaveSessionUI()
{
	EOS_HSessions SessionsHandle = EOS_Platform_GetSessionsInterface(FPlatform::GetPlatformHandle());

	EOS_Sessions_AddNotifyLeaveSessionRequestedOptions LeaveSessionRequestedOptions = { };
	LeaveSessionRequestedOptions.ApiVersion = EOS_SESSIONS_ADDNOTIFYLEAVESESSIONREQUESTED_API_LATEST;
	LeaveSessionRequestedNotificationHandle = EOS_Sessions_AddNotifyLeaveSessionRequested(SessionsHandle, &LeaveSessionRequestedOptions, nullptr, OnLeaveSessionRequestedCallback);
}

void FSessionMatchmaking::UnsubscribeFromLeaveSessionUI()
{
	if (LeaveSessionRequestedNotificationHandle != EOS_INVALID_NOTIFICATIONID)
	{
		EOS_Sessions_RemoveNotifyLeaveSessionRequested(EOS_Platform_GetSessionsInterface(FPlatform::GetPlatformHandle()), LeaveSessionRequestedNotificationHandle);
		LeaveSessionRequestedNotificationHandle = EOS_INVALID_NOTIFICATIONID;
	}
}

void FSessionMatchmaking::Search(const std::vector<FSession::Attribute>& Attributes)
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player != nullptr)
	{
		//Clear previous search
		CurrentSearch.Release();

		EOS_HSessions SessionsHandle = EOS_Platform_GetSessionsInterface(FPlatform::GetPlatformHandle());

		EOS_HSessionSearch SearchHandle;
		EOS_Sessions_CreateSessionSearchOptions SearchOptions = {};
		SearchOptions.ApiVersion = EOS_SESSIONS_CREATESESSIONSEARCH_API_LATEST;
		SearchOptions.MaxSearchResults = 10;

		EOS_EResult Result = EOS_Sessions_CreateSessionSearch(SessionsHandle, &SearchOptions, &SearchHandle);
		if (Result != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Session Matchmaking: failed to create session search. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
			return;
		}

		CurrentSearch.SetNewSearch(SearchHandle);

		EOS_SessionSearch_SetParameterOptions ParamOptions = {};
		ParamOptions.ApiVersion = EOS_SESSIONSEARCH_SETPARAMETER_API_LATEST;
		ParamOptions.ComparisonOp = EOS_EOnlineComparisonOp::EOS_CO_EQUAL;

		EOS_Sessions_AttributeData AttrData;
		AttrData.ApiVersion = EOS_SESSIONS_SESSIONATTRIBUTEDATA_API_LATEST;
		ParamOptions.Parameter = &AttrData;

		//Set bucket id first. Sample uses the same bucket ids for all its sessions.
		//This is why we are using the same bucket id to search for sessions. See documentation for more info about bucket id usage.
		AttrData.Key = EOS_SESSIONS_SEARCH_BUCKET_ID;
		AttrData.ValueType = EOS_EAttributeType::EOS_AT_STRING;
		AttrData.Value.AsUtf8 = BucketId;
		Result = EOS_SessionSearch_SetParameter(SearchHandle, &ParamOptions);
		if (Result != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Session Matchmaking: failed to update session search with bucket id parameter. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
			return;
		}

		//Set other attributes
		for (const FSession::Attribute& NextAttr : Attributes)
		{
			AttrData.Key = NextAttr.Key.c_str();

			switch (NextAttr.ValueType)
			{
			case FSession::Attribute::Bool:
				AttrData.ValueType = EOS_ESessionAttributeType::EOS_SAT_Boolean;
				AttrData.Value.AsBool = NextAttr.AsBool;
				break;
			case FSession::Attribute::Int64:
				AttrData.ValueType = EOS_ESessionAttributeType::EOS_SAT_Int64;
				AttrData.Value.AsInt64 = NextAttr.AsInt64;
				break;
			case FSession::Attribute::Double:
				AttrData.ValueType = EOS_ESessionAttributeType::EOS_SAT_Double;
				AttrData.Value.AsDouble = NextAttr.AsDouble;
				break;
			case FSession::Attribute::String:
				AttrData.ValueType = EOS_ESessionAttributeType::EOS_AT_STRING;
				AttrData.Value.AsUtf8 = NextAttr.AsString.c_str();
				break;
			}

			Result = EOS_SessionSearch_SetParameter(SearchHandle, &ParamOptions);
			if (Result != EOS_EResult::EOS_Success)
			{
				FDebugLog::LogError(L"Session Matchmaking: failed to update session search with parameter. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
				return;
			}
		}

		EOS_SessionSearch_FindOptions FindOptions = {};
		FindOptions.ApiVersion = EOS_SESSIONSEARCH_FIND_API_LATEST;
		FindOptions.LocalUserId = Player->GetProductUserID();
		EOS_SessionSearch_Find(SearchHandle, &FindOptions, nullptr, OnFindSessionsCompleteCallback);
	}
	else
	{
		FDebugLog::LogError(L"Session Matchmaking - Search: Current player is invalid!");
	}
}

void FSessionMatchmaking::SearchById(const std::string& SessionId)
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player != nullptr)
	{
		//Clear previous search
		CurrentSearch.Release();

		EOS_HSessions SessionsHandle = EOS_Platform_GetSessionsInterface(FPlatform::GetPlatformHandle());

		EOS_HSessionSearch SearchHandle;
		EOS_Sessions_CreateSessionSearchOptions SearchOptions = {};
		SearchOptions.ApiVersion = EOS_SESSIONS_CREATESESSIONSEARCH_API_LATEST;
		SearchOptions.MaxSearchResults = 10;

		EOS_EResult Result = EOS_Sessions_CreateSessionSearch(SessionsHandle, &SearchOptions, &SearchHandle);
		if (Result != EOS_EResult::EOS_Success)
		{
			// Sample currently doesn't retry after any errors; inform the UI that the Join Game attempt is complete. 
			AcknowledgeEventId(Result);
			FDebugLog::LogError(L"Session Matchmaking: failed create session search. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
			return;
		}

		CurrentSearch.SetNewSearch(SearchHandle);

		EOS_SessionSearch_SetSessionIdOptions SessionIdOptions = {};
		SessionIdOptions.ApiVersion = EOS_SESSIONSEARCH_SETSESSIONID_API_LATEST;
		SessionIdOptions.SessionId = SessionId.c_str();

		Result = EOS_SessionSearch_SetSessionId(SearchHandle, &SessionIdOptions);
		if (Result != EOS_EResult::EOS_Success)
		{
			// Sample currently doesn't retry after any errors; inform the UI that the Join Game attempt is complete. 
			AcknowledgeEventId(Result);
			FDebugLog::LogError(L"Session Matchmaking: failed to update session search with session ID. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
			return;
		}

		EOS_SessionSearch_FindOptions FindOptions = {};
		FindOptions.ApiVersion = EOS_SESSIONSEARCH_FIND_API_LATEST;
		FindOptions.LocalUserId = Player->GetProductUserID();
		EOS_SessionSearch_Find(SearchHandle, &FindOptions, nullptr, OnFindSessionsCompleteCallback);
	}
	else
	{
		FDebugLog::LogError(L"Session Matchmaking - SearchById: Current player is invalid!");
	}
}

SessionDetailsKeeper FSessionMatchmaking::MakeSessionHandleByInviteId(const std::string& InviteId)
{
	// retrieve handle to session details.
	EOS_HSessions SessionsHandle = EOS_Platform_GetSessionsInterface(FPlatform::GetPlatformHandle());
	EOS_Sessions_CopySessionHandleByInviteIdOptions Options = {};
	Options.ApiVersion = EOS_SESSIONS_COPYSESSIONHANDLEBYINVITEID_API_LATEST;
	Options.InviteId = InviteId.c_str();
	EOS_HSessionDetails SessionDetails = nullptr;
	if (EOS_Sessions_CopySessionHandleByInviteId(SessionsHandle, &Options, &SessionDetails) == EOS_EResult::EOS_Success)
	{
		return MakeSessionDetailsKeeper(SessionDetails);
	}

	return nullptr;
}

SessionDetailsKeeper FSessionMatchmaking::MakeSessionHandleByEventId(const EOS_UI_EventId UiEventId)
{
	// retrieve handle to session details.
	EOS_HSessions SessionsHandle = EOS_Platform_GetSessionsInterface(FPlatform::GetPlatformHandle());
	EOS_Sessions_CopySessionHandleByUiEventIdOptions Options = {};
	Options.ApiVersion = EOS_SESSIONS_COPYSESSIONHANDLEBYUIEVENTID_API_LATEST;
	Options.UiEventId = UiEventId;
	EOS_HSessionDetails SessionDetails = nullptr;
	if (EOS_Sessions_CopySessionHandleByUiEventId(SessionsHandle, &Options, &SessionDetails) == EOS_EResult::EOS_Success)
	{
		return MakeSessionDetailsKeeper(SessionDetails);
	}

	return nullptr;
}

SessionDetailsKeeper FSessionMatchmaking::GetSessionHandleFromSearch(const std::string& SessionId) const
{
	return CurrentSearch.GetSessionHandleById(SessionId);
}

void FSessionMatchmaking::JoinSession(SessionDetailsKeeper SessionHandle, bool bPresenceSession)
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player != nullptr)
	{
		EOS_HSessions SessionsHandle = EOS_Platform_GetSessionsInterface(FPlatform::GetPlatformHandle());

		std::string NewJoinedSessionName = GenerateJoinedSessionName();

		EOS_Sessions_JoinSessionOptions JoinOptions = {};
		JoinOptions.ApiVersion = EOS_SESSIONS_JOINSESSION_API_LATEST;
		JoinOptions.SessionHandle = SessionHandle.get();
		JoinOptions.SessionName = NewJoinedSessionName.c_str();
		JoinOptions.LocalUserId = Player->GetProductUserID();
		JoinOptions.bPresenceEnabled = bPresenceSession;
		
		EOS_Sessions_JoinSession(SessionsHandle, &JoinOptions, nullptr, OnJoinSessionCallback);

		SetJoiningSessionDetails(SessionHandle);
	}
	else
	{
		FDebugLog::LogError(L"Session Matchmaking - JoinSession: Current player is invalid!");
	}
}


bool FSessionMatchmaking::ModifySession(const FSession& Session)
{
	EOS_HSessions SessionsHandle = EOS_Platform_GetSessionsInterface(FPlatform::GetPlatformHandle());
	if (!SessionsHandle)
	{
		FDebugLog::LogError(L"Session Matchmaking: can't get sessions interface.");
		return false;
	}

	//search for current session by name
	auto iter = CurrentSessions.find(Session.Name);
	if (iter == CurrentSessions.end())
	{
		FDebugLog::LogError(L"Session Matchmaking: can't modify session: no active session with specified name.");
		return false;
	}

	FSession& CurrentSession = iter->second;

	EOS_Sessions_UpdateSessionModificationOptions UpdateModOptions = {};
	UpdateModOptions.ApiVersion = EOS_SESSIONS_UPDATESESSIONMODIFICATION_API_LATEST;
	UpdateModOptions.SessionName = Session.Name.c_str();

	EOS_HSessionModification ModificationHandle = NULL;
	EOS_EResult UpdateResult = EOS_Sessions_UpdateSessionModification(SessionsHandle, &UpdateModOptions, &ModificationHandle);
	if (UpdateResult != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"Session Matchmaking: failed create session modification. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(UpdateResult)).c_str());
		return false;
	}

	//bucket id
	if (Session.BucketId != CurrentSession.BucketId)
	{
		EOS_SessionModification_SetBucketIdOptions BucketOptions = {};
		BucketOptions.ApiVersion = EOS_SESSIONMODIFICATION_SETBUCKETID_API_LATEST;
		BucketOptions.BucketId = Session.BucketId.c_str();
		EOS_EResult SetBucketIdResult = EOS_SessionModification_SetBucketId(ModificationHandle, &BucketOptions);
		if (SetBucketIdResult != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Session Matchmaking: failed to set bucket id. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(SetBucketIdResult)).c_str());
			EOS_SessionModification_Release(ModificationHandle);
			return false;
		}
	}

	//max players
	if (Session.MaxPlayers != CurrentSession.MaxPlayers)
	{
		EOS_SessionModification_SetMaxPlayersOptions MaxPlayerOptions = {};
		MaxPlayerOptions.ApiVersion = EOS_SESSIONMODIFICATION_SETMAXPLAYERS_API_LATEST;
		MaxPlayerOptions.MaxPlayers = Session.MaxPlayers;
		EOS_EResult SetMaxPlayersResult = EOS_SessionModification_SetMaxPlayers(ModificationHandle, &MaxPlayerOptions);
		if (SetMaxPlayersResult != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Session Matchmaking: failed to set maxp layers. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(SetMaxPlayersResult)).c_str());
			EOS_SessionModification_Release(ModificationHandle);
			return false;
		}
	}

	// modify permissions
	if (Session.PermissionLevel != CurrentSession.PermissionLevel)
	{
		EOS_SessionModification_SetPermissionLevelOptions PermOptions = {};
		PermOptions.ApiVersion = EOS_SESSIONMODIFICATION_SETPERMISSIONLEVEL_API_LATEST;
		PermOptions.PermissionLevel = Session.PermissionLevel;
		EOS_EResult SetPermsResult = EOS_SessionModification_SetPermissionLevel(ModificationHandle, &PermOptions);
		if (SetPermsResult != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Session Matchmaking: failed to set permissions. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(SetPermsResult)).c_str());
			EOS_SessionModification_Release(ModificationHandle);
			return false;
		}
	}

	// join in progress
	if (Session.bAllowJoinInProgress != CurrentSession.bAllowJoinInProgress)
	{
		EOS_SessionModification_SetJoinInProgressAllowedOptions JIPOptions = {};
		JIPOptions.ApiVersion = EOS_SESSIONMODIFICATION_SETJOININPROGRESSALLOWED_API_LATEST;
		JIPOptions.bAllowJoinInProgress = (Session.bAllowJoinInProgress) ? EOS_TRUE : EOS_FALSE;
		EOS_EResult SetJIPResult = EOS_SessionModification_SetJoinInProgressAllowed(ModificationHandle, &JIPOptions);
		if (SetJIPResult != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Session Matchmaking: failed to set 'join in progress allowed' flag. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(SetJIPResult)).c_str());
			EOS_SessionModification_Release(ModificationHandle);
			return false;
		}
	}

	EOS_Sessions_AttributeData AttrData;
	AttrData.ApiVersion = EOS_SESSIONS_SESSIONATTRIBUTEDATA_API_LATEST;

	EOS_SessionModification_AddAttributeOptions AttrOptions = {};
	AttrOptions.ApiVersion = EOS_SESSIONMODIFICATION_ADDATTRIBUTE_API_LATEST;
	AttrOptions.SessionAttribute = &AttrData;

	for (const FSession::Attribute& NextAttribute : Session.Attributes)
	{
		//check if the attribute changed
		auto AttribFound = std::find_if(CurrentSession.Attributes.begin(), CurrentSession.Attributes.end(), [NextAttribute](const FSession::Attribute& Attrib) { return NextAttribute.Key == Attrib.Key; });
		if (AttribFound != CurrentSession.Attributes.end())
		{
			if (*AttribFound == NextAttribute)
			{
				//Attributes are equal, no need to change
				continue;
			}
		}

		AttrData.Key = NextAttribute.Key.c_str();

		switch (NextAttribute.ValueType)
		{
		case FSession::Attribute::Bool:
			AttrData.ValueType = EOS_ESessionAttributeType::EOS_SAT_Boolean;
			AttrData.Value.AsBool = NextAttribute.AsBool;
			break;
		case FSession::Attribute::Double:
			AttrData.ValueType = EOS_ESessionAttributeType::EOS_SAT_Double;
			AttrData.Value.AsDouble = NextAttribute.AsDouble;
			break;
		case FSession::Attribute::Int64:
			AttrData.ValueType = EOS_ESessionAttributeType::EOS_SAT_Int64;
			AttrData.Value.AsInt64 = NextAttribute.AsInt64;
			break;
		case FSession::Attribute::String:
			AttrData.ValueType = EOS_ESessionAttributeType::EOS_AT_STRING;
			AttrData.Value.AsUtf8 = NextAttribute.AsString.c_str();
			break;
		}

		AttrOptions.AdvertisementType = NextAttribute.Advertisement;
		EOS_EResult SetAttrResult = EOS_SessionModification_AddAttribute(ModificationHandle, &AttrOptions);
		if (SetAttrResult != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Session Matchmaking: failed to set an attribute: %ls. Error code: %ls", NextAttribute.Key.c_str(), FStringUtils::Widen(EOS_EResult_ToString(SetAttrResult)).c_str());
			EOS_SessionModification_Release(ModificationHandle);
			return false;
		}
	}

	EOS_Sessions_UpdateSessionOptions UpdateOptions = {};
	UpdateOptions.ApiVersion = EOS_SESSIONS_UPDATESESSION_API_LATEST;
	UpdateOptions.SessionModificationHandle = ModificationHandle;
	EOS_Sessions_UpdateSession(SessionsHandle, &UpdateOptions, nullptr, OnUpdateSessionCompleteCallback);

	CurrentSession = Session;
	CurrentSession.bUpdateInProgress = true;

	EOS_SessionModification_Release(ModificationHandle);

	return true;
}

void FSessionMatchmaking::OnSessionDestroyed(const std::string& SessionName)
{
	if (!SessionName.empty())
	{
		auto iter = CurrentSessions.find(SessionName);
		if (iter != CurrentSessions.end())
		{
			if (iter->second.bPresenceSession)
			{
				SetJoinInfo("");
			}
			CurrentSessions.erase(iter);
		}
	}
}

void FSessionMatchmaking::OnSessionUpdateFinished(bool bSuccess, const std::string& Name, const std::string& SessionId, bool bRemoveSessionOnFailure /* = false */)
{
	auto Iter = CurrentSessions.find(Name);
	if (Iter != CurrentSessions.end())
	{
		FSession& Session = Iter->second;
		Session.Name = Name;
		Session.InitActiveSession();
		Session.bUpdateInProgress = false;

		if (bSuccess)
		{
			Session.Id = SessionId;
			if (Session.bPresenceSession)
			{
				SetJoinInfo(SessionId);
			}
		}
		else
		{
			if (bRemoveSessionOnFailure)
			{
				CurrentSessions.erase(Iter);
			}
		}
	}
}

void FSessionMatchmaking::OnSearchResultsReceived()
{
	EOS_SessionSearch_GetSearchResultCountOptions SearchResultOptions = {};
	SearchResultOptions.ApiVersion = EOS_SESSIONSEARCH_GETSEARCHRESULTCOUNT_API_LATEST;
	uint32_t NumSearchResults = EOS_SessionSearch_GetSearchResultCount(CurrentSearch.GetSearchHandle(), &SearchResultOptions);

	std::vector<FSession> SearchResults;
	std::vector<SessionDetailsKeeper> ResultHandles;

	EOS_SessionSearch_CopySearchResultByIndexOptions IndexOptions = {};
	IndexOptions.ApiVersion = EOS_SESSIONSEARCH_COPYSEARCHRESULTBYINDEX_API_LATEST;
	for (uint32_t i = 0; i < NumSearchResults; ++i)
	{
		FSession NextSession;

		EOS_HSessionDetails NextSessionHandle = nullptr;

		IndexOptions.SessionIndex = i;
		EOS_EResult Result = EOS_SessionSearch_CopySearchResultByIndex(CurrentSearch.GetSearchHandle(), &IndexOptions, &NextSessionHandle);
		if (Result == EOS_EResult::EOS_Success && NextSessionHandle)
		{
			EOS_SessionDetails_Info* SessionInfo = NULL;
			EOS_SessionDetails_CopyInfoOptions CopyOptions = {};
			CopyOptions.ApiVersion = EOS_SESSIONDETAILS_COPYINFO_API_LATEST;
			EOS_EResult CopyResult = EOS_SessionDetails_CopyInfo(NextSessionHandle, &CopyOptions, &SessionInfo);
			if (CopyResult == EOS_EResult::EOS_Success)
			{
				NextSession.InitFromSessionInfo(NextSessionHandle, SessionInfo);
				EOS_SessionDetails_Info_Release(SessionInfo);
			}
			NextSession.bSearchResult = true;
			SearchResults.push_back(NextSession);
			ResultHandles.push_back(MakeSessionDetailsKeeper(NextSessionHandle));

			//Check is we have a local session with same ID (we can retrieve the name this way).
			for (auto SessionPair : CurrentSessions)
			{
				if (SessionPair.second.Id == SearchResults.back().Id)
				{
					SearchResults.back().Name = SessionPair.first;
					break;
				}
			}
		}
	}

	CurrentSearch.OnSearchResultsReceived(std::move(SearchResults), std::move(ResultHandles));
	if (JoinPresenceSessionId.size() > 0)
	{
		if (SessionDetailsKeeper Handle = CurrentSearch.GetSessionHandleById(JoinPresenceSessionId))
		{
			// clear the session ID.
			JoinPresenceSessionId = std::string();
			const bool bPresenceEnabled = true;
			JoinSession(Handle, bPresenceEnabled);
		}
		else
		{
			// Sample currently doesn't retry after any errors; inform the UI that the Join Game attempt is complete. 
			AcknowledgeEventId(EOS_EResult::EOS_NotFound);
		}
	}
	else
	{
		// Sample currently doesn't retry after any errors; inform the UI that the Join Game attempt is complete. 
		AcknowledgeEventId(EOS_EResult::EOS_NotFound);
	}
}

void FSessionMatchmaking::SetJoiningSessionDetails(SessionDetailsKeeper NewDetails)
{
	JoiningSessionDetails = NewDetails;
}

void FSessionMatchmaking::OnJoinSessionFinished()
{
	if (JoiningSessionDetails)
	{
		EOS_SessionDetails_Info* SessionInfo = NULL;
		EOS_SessionDetails_CopyInfoOptions CopyOptions = {};
		CopyOptions.ApiVersion = EOS_SESSIONDETAILS_COPYINFO_API_LATEST;
		EOS_EResult CopyResult = EOS_SessionDetails_CopyInfo(JoiningSessionDetails.get(), &CopyOptions, &SessionInfo);
		if (CopyResult == EOS_EResult::EOS_Success)
		{
			FSession Session;
			Session.Name = GenerateJoinedSessionName(true);
			Session.InitFromSessionInfo(JoiningSessionDetails.get(), SessionInfo);
			// local user is joining this session
			// Without p2p, the sample is incapable of communicating to host to call EOS_Sessions_RegisterPlayer and update this globally
			Session.NumConnections += 1; 
			EOS_SessionDetails_Info_Release(SessionInfo);

			// Check if we have a local session with same ID (no need to add an extra one in this case).
			bool bLocalSessionFound = false;
			for (auto SessionPair : CurrentSessions)
			{
				if (SessionPair.second.Id == Session.Id)
				{
					bLocalSessionFound = true;
					if (Session.bPresenceSession)
					{
						SetJoinInfo(Session.Id);
					}
					break;
				}
			}

			if (!bLocalSessionFound)
			{
				CurrentSessions[Session.Name] = Session;
				if (Session.bPresenceSession)
				{
					SetJoinInfo(Session.Id);
				}
			}
		}
	}

	//Trigger UI event to clear search (if started)
	FGameEvent Event(EGameEventType::SessionJoined);
	FGame::Get().OnGameEvent(Event);
}

void FSessionMatchmaking::OnSessionStarted(const std::string& Name)
{
	auto iter = CurrentSessions.find(Name);
	if (iter != CurrentSessions.end())
	{
		iter->second.SessionState = EOS_EOnlineSessionState::EOS_OSS_InProgress;
	}
}

void FSessionMatchmaking::OnSessionEnded(const std::string& Name)
{
	auto iter = CurrentSessions.find(Name);
	if (iter != CurrentSessions.end())
	{
		iter->second.SessionState = EOS_EOnlineSessionState::EOS_OSS_Ended;
	}
}

void FSessionMatchmaking::SetJoinInfo(const std::string& SessionId)
{
#if USE_JOIN_INFO_WITH_OVERLAY
	// Local buffer to hold the `JoinInfo` string if its used.
	char JoinInfoBuffer[EOS_PRESENCEMODIFICATION_JOININFO_MAX_LENGTH + 1];

	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player == nullptr)
	{
		FDebugLog::LogError(L"Session Matchmaking - SetJoinInfo: Current player is invalid!");
		return;
	}

	EOS_HPresence PresenceHandle = EOS_Platform_GetPresenceInterface(FPlatform::GetPlatformHandle());

	EOS_Presence_CreatePresenceModificationOptions CreateModOpt = {};
	CreateModOpt.ApiVersion = EOS_PRESENCE_CREATEPRESENCEMODIFICATION_API_LATEST;
	CreateModOpt.LocalUserId = Player->GetUserID();

	EOS_HPresenceModification PresenceModification;
	EOS_EResult Result = EOS_Presence_CreatePresenceModification(PresenceHandle, &CreateModOpt, &PresenceModification);
	if (Result != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"[EOS SDK] Create presence modification failed: %ls.", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
		return;
	}

	EOS_PresenceModification_SetJoinInfoOptions JoinOptions = {};
	JoinOptions.ApiVersion = EOS_PRESENCEMODIFICATION_SETJOININFO_API_LATEST;
	if (SessionId.length() == 0)
	{
		// Clear the JoinInfo string if there is no local sessionId.
		JoinOptions.JoinInfo = nullptr;
	}
	else
	{
		// Use the local sessionId to build a JoinInfo string to share with friends.
		snprintf(JoinInfoBuffer, EOS_PRESENCEMODIFICATION_JOININFO_MAX_LENGTH, SampleJoinInfoFormat, SessionId.c_str());
		JoinOptions.JoinInfo = JoinInfoBuffer;
	}
	EOS_PresenceModification_SetJoinInfo(PresenceModification, &JoinOptions);

	EOS_Presence_SetPresenceOptions SetOpt = {};
	SetOpt.ApiVersion = EOS_PRESENCE_SETPRESENCE_API_LATEST;
	SetOpt.LocalUserId = Player->GetUserID();
	SetOpt.PresenceModificationHandle = PresenceModification;
	EOS_Presence_SetPresence(PresenceHandle, &SetOpt, nullptr, FSessionMatchmaking::OnSetPresenceCallback);

	EOS_PresenceModification_Release(PresenceModification);
#endif
}

void FSessionMatchmaking::OnJoinGameAcceptedByJoinInfo(const std::string& JoinInfo, EOS_UI_EventId UiEventId)
{
	JoinUiEvent = UiEventId;

	std::regex JoinInfoRegex(SampleJoinInfoRegex);
	std::smatch JoinInfoMatch;
	if (std::regex_match(JoinInfo, JoinInfoMatch, JoinInfoRegex))
	{
		if (JoinInfoMatch.size() == 2)
		{
			JoinPresenceSessionById(JoinInfoMatch[1].str());
			return;
		}
	}

	// This should be impossible since the JoinInfo string was generated by the sample; inform the UI of the completed join game attempt.
	AcknowledgeEventId(EOS_EResult::EOS_UnexpectedError);
	FDebugLog::LogError(L"Session Matchmaking - OnJoinGameAccepted: unable to parse location string: %ls", FStringUtils::Widen(JoinInfo).c_str());
}

void FSessionMatchmaking::OnJoinGameAcceptedByEventId(EOS_UI_EventId UiEventId)
{
	SessionDetailsKeeper EventSession = MakeSessionHandleByEventId(UiEventId);
	if (EventSession)
	{
		const bool bPresenceEnabled = true;
		JoinSession(EventSession, bPresenceEnabled);
	}
	else
	{
		// This should be impossible since the fact that the callback was received means that the session handle exists.
		JoinUiEvent = UiEventId;
		AcknowledgeEventId(EOS_EResult::EOS_UnexpectedError);
		FDebugLog::LogError(L"Session Matchmaking - OnJoinGameAccepted: unable to get details for event ID: %d", UiEventId);
	}
}

void FSessionMatchmaking::JoinPresenceSessionById(const std::string& SessionId)
{
	JoinPresenceSessionId = SessionId;
	FDebugLog::Log(L"Session Matchmaking - JoinPresenceSessionById: looking for session ID: %ls", FStringUtils::Widen(JoinPresenceSessionId).c_str());
	SearchById(JoinPresenceSessionId);
}

void FSessionMatchmaking::AcknowledgeEventId(EOS_EResult Result)
{
	if (JoinUiEvent != EOS_UI_EVENTID_INVALID)
	{
		EOS_HUI UIHandle = EOS_Platform_GetUIInterface(FPlatform::GetPlatformHandle());

		EOS_UI_AcknowledgeEventIdOptions Options = {};
		Options.ApiVersion = EOS_UI_ACKNOWLEDGECORRELATIONID_API_LATEST;
		Options.UiEventId = JoinUiEvent;
		Options.Result = Result;
		EOS_UI_AcknowledgeEventId(UIHandle, &Options);
		JoinUiEvent = EOS_UI_EVENTID_INVALID;
	}
}

std::string FSessionMatchmaking::GenerateJoinedSessionName(bool bNoIncrement /* = false */)
{
	if (!bNoIncrement)
	{
		JoinedSessionIndex = (JoinedSessionIndex + 1) % JoinedSessionNameRotationNum;
	}

	static char Buffer[32];
	sprintf_s(Buffer, sizeof(Buffer), "%s%d", JoinedSessionName, (int)JoinedSessionIndex);

	return std::string(Buffer);
}

void EOS_CALL FSessionMatchmaking::OnUpdateSessionCompleteCallback(const EOS_Sessions_UpdateSessionCallbackInfo* Data)
{
	if (Data)
	{
		if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
		{
			// Operation is retrying so it is not complete yet
			return;
		}

		if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			FGame::Get().GetSessions()->OnSessionUpdateFinished(false, Data->SessionName, Data->SessionId);
			FDebugLog::LogError(L"Session Matchmaking (OnUpdateSessionCompleteCallback): error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else
		{
			FGame::Get().GetSessions()->OnSessionUpdateFinished(true, Data->SessionName, Data->SessionId);
			FDebugLog::Log(L"Session Matchmaking: game session updated successfully.");
		}
	}
	else
	{
		FDebugLog::LogError(L"Session Matchmaking (OnUpdateSessionCompleteCallback): EOS_Sessions_UpdateSessionCallbackInfo is null");
	}
}

void EOS_CALL FSessionMatchmaking::OnUpdateSessionCompleteCallback_ForCreate(const EOS_Sessions_UpdateSessionCallbackInfo* Data)
{
	if (!Data)
	{
		FDebugLog::LogError(L"Session Matchmaking (OnUpdateSessionCompleteCallback): EOS_Sessions_UpdateSessionCallbackInfo is null");
		return;
	}

	bool bRemoveSession = true;
	const bool bSuccess = Data->ResultCode == EOS_EResult::EOS_Success;
	if (bSuccess)
	{
		PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
		if (Player != nullptr)
		{
			//Register session owner
			FGame::Get().GetSessions()->Register(Data->SessionName, Player->GetProductUserID());
			bRemoveSession = false;
		}
		else
		{
			//Failing to register is not considered a critical error, that's why we don't remove created session here
			FDebugLog::LogError(L"Session Matchmaking (OnUpdateSessionCompleteCallback_ForCreate): player is null, can't register yourself in created session.");
		}

		//We don't wait for register to finish
		FDebugLog::Log(L"Session Matchmaking: game session created successfully.");
	}
	else
	{
		FDebugLog::LogError(L"Session Matchmaking (OnUpdateSessionCompleteCallback): error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
	}

	FGame::Get().GetSessions()->OnSessionUpdateFinished(bSuccess, Data->SessionName, Data->SessionId, bRemoveSession);
}

void EOS_CALL FSessionMatchmaking::OnStartSessionCompleteCallback(const EOS_Sessions_StartSessionCallbackInfo* Data)
{
	if (Data)
	{
		if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
		{
			// Operation is retrying so it is not complete yet
			return;
		}

		std::string* SessionNamePtr = static_cast<std::string*>(Data->ClientData);

		if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Session Matchmaking (OnStartSessionCompleteCallback): session name: '%ls' error code: %ls", FStringUtils::Widen(*SessionNamePtr).c_str(), FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else
		{
			FGame::Get().GetSessions()->OnSessionStarted(*SessionNamePtr);
		}
		delete SessionNamePtr;
	}
	else
	{
		FDebugLog::LogError(L"Session Matchmaking (OnStartSessionCompleteCallback): EOS_Sessions_StartSessionCallbackInfo is null");
	}
}

void EOS_CALL FSessionMatchmaking::OnEndSessionCompleteCallback(const EOS_Sessions_EndSessionCallbackInfo* Data)
{
	if (Data)
	{
		if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
		{
			// Operation is retrying so it is not complete yet
			return;
		}

		std::string* SessionNamePtr = static_cast<std::string*>(Data->ClientData);

		if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Session Matchmaking (OnEndSessionCompleteCallback): session name: '%ls' error code: %ls", FStringUtils::Widen(*SessionNamePtr).c_str(), FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else
		{
			FGame::Get().GetSessions()->OnSessionEnded(*SessionNamePtr);
		}
		delete SessionNamePtr;
	}
	else
	{
		FDebugLog::LogError(L"Session Matchmaking (OnEndSessionCompleteCallback): EOS_Sessions_EndSessionCallbackInfo is null");
	}
}

void EOS_CALL FSessionMatchmaking::OnDestroySessionCompleteCallback(const EOS_Sessions_DestroySessionCallbackInfo* Data)
{
	if (Data)
	{
		if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
		{
			// Operation is retrying so it is not complete yet
			return;
		}

		std::string* SessionNameStringPtr = static_cast<std::string*>(Data->ClientData);
		if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Session Matchmaking (OnDestroySessionCompleteCallback): error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else
		{
			if (SessionNameStringPtr)
			{
				FGame::Get().GetSessions()->OnSessionDestroyed(*SessionNameStringPtr);
			}
		}
		delete SessionNameStringPtr;
	}
	else
	{
		FDebugLog::LogError(L"Session Matchmaking (OnDestroySessionCompleteCallback): OnDestroySessionCompleteCallback is null");
	}
}

void EOS_CALL FSessionMatchmaking::OnRegisterCompleteCallback(const EOS_Sessions_RegisterPlayersCallbackInfo* Data)
{
	if (Data)
	{
		if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
		{
			// Operation is retrying so it is not complete yet
			return;
		}

		if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Session Matchmaking (OnRegisterCompleteCallback): error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
	}
	else
	{
		FDebugLog::LogError(L"Session Matchmaking (OnRegisterCompleteCallback): OnRegisterCompleteCallback is null");
	}
}

void EOS_CALL FSessionMatchmaking::OnUnregisterCompleteCallback(const EOS_Sessions_UnregisterPlayersCallbackInfo* Data)
{
	if (Data)
	{
		if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
		{
			// Operation is retrying so it is not complete yet
			return;
		}

		if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Session Matchmaking (OnUnregisterCompleteCallback): error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
	}
	else
	{
		FDebugLog::LogError(L"Session Matchmaking (OnUnregisterCompleteCallback): OnUnregisterCompleteCallback is null");
	}
}

void EOS_CALL FSessionMatchmaking::OnFindSessionsCompleteCallback(const EOS_SessionSearch_FindCallbackInfo* Data)
{
	if (Data)
	{
		if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
		{
			// Operation is retrying so it is not complete yet
			return;
		}

		if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			// Session no longer exists to join; inform the UI of the completed join game attempt.
			FGame::Get().GetSessions()->AcknowledgeEventId(Data->ResultCode);
			FDebugLog::LogError(L"Session Matchmaking (OnFindSessionsCompleteCallback): error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else
		{
			FGame::Get().GetSessions()->OnSearchResultsReceived();
		}
	}
	else
	{
		// Should be impossible; inform the UI of the completed join game attempt.
		FGame::Get().GetSessions()->AcknowledgeEventId(EOS_EResult::EOS_UnexpectedError);
		FDebugLog::LogError(L"Session Matchmaking (OnUnregisterCompleteCallback): OnUnregisterCompleteCallback is null");
	}
}

void EOS_CALL FSessionMatchmaking::OnSendInviteCompleteCallback(const EOS_Sessions_SendInviteCallbackInfo* Data)
{
	if (Data)
	{
		if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
		{
			// Operation is retrying so it is not complete yet
			return;
		}

		if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Session Matchmaking (OnSendInviteCompleteCallback): error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else
		{
			FDebugLog::Log(L"Session Matchmaking: invite to session sent successfully.");
		}
	}
	else
	{
		FDebugLog::LogError(L"Session Matchmaking (OnSendInviteCompleteCallback): OnUnregisterCompleteCallback is null");
	}
}

void EOS_CALL FSessionMatchmaking::OnSessionInviteReceivedCallback(const EOS_Sessions_SessionInviteReceivedCallbackInfo* Data)
{
	if (Data)
	{
		FDebugLog::Log(L"Session Matchmaking: invite to session received. Invite id: %ls", FStringUtils::Widen(Data->InviteId).c_str());

		if (SessionDetailsKeeper SessionDetails = MakeSessionHandleByInviteId(Data->InviteId))
		{
			FSession InviteSession;
			if (InviteSession.InitFromInfoOfSessionDetails(SessionDetails))
			{

				FGame::Get().GetSessions()->SetInviteSession(InviteSession, SessionDetails);

				//Show popup
				FGameEvent Event(EGameEventType::InviteToSessionReceived, Data->TargetUserId, FStringUtils::Widen(Data->InviteId));
				FGame::Get().OnGameEvent(Event);
			}
			else
			{
				FDebugLog::LogError(L"Session Matchmaking (OnSessionInviteReceivedCallback): Could not copy session information for invite id %ls.", FStringUtils::Widen(Data->InviteId).c_str());
			}
		}
		else
		{
			FDebugLog::LogError(L"Session Matchmaking (OnSessionInviteReceivedCallback): Could not get session details for invite id %ls.", FStringUtils::Widen(Data->InviteId).c_str());
		}
	}
}

void EOS_CALL FSessionMatchmaking::OnSessionInviteAcceptedCallback(const EOS_Sessions_SessionInviteAcceptedCallbackInfo* Data)
{
	if (Data)
	{
		FDebugLog::Log(L"Session Matchmaking: invite to session accepted. Session id: %ls; Invite id: %ls", FStringUtils::Widen(Data->SessionId).c_str(), FStringUtils::Widen(Data->InviteId).c_str());

		//Hide popup
		FGameEvent Event(EGameEventType::OverlayInviteToSessionAccepted);
		FGame::Get().OnGameEvent(Event);

		if (SessionDetailsKeeper InviteSessionKeeper = MakeSessionHandleByInviteId(Data->InviteId))
		{
			const bool bPresenceEnabled = true;
			FGame::Get().GetSessions()->JoinSession(InviteSessionKeeper, bPresenceEnabled);
		}
		else
		{
			FDebugLog::LogError(L"Session Matchmaking (OnSessionInviteReceivedCallback): Could not get session details by invite id %ls.", FStringUtils::Widen(Data->InviteId).c_str());
		}
	}
}

void EOS_CALL FSessionMatchmaking::OnSessionInviteRejectedCallback(const EOS_Sessions_SessionInviteRejectedCallbackInfo* Data)
{
	if (Data)
	{
		FDebugLog::Log(L"Session Matchmaking: invite to session rejected. Session id: %ls; Invite id: %ls", FStringUtils::Widen(Data->SessionId).c_str(), FStringUtils::Widen(Data->InviteId).c_str());

		//Hide popup
		FGameEvent Event(EGameEventType::OverlayInviteToSessionAccepted);
		FGame::Get().OnGameEvent(Event);
	}
}

void EOS_CALL FSessionMatchmaking::OnSendRequestToJoinCompleteCallback(const EOS_CustomInvites_SendRequestToJoinCallbackInfo* Data)
{
	if (Data == nullptr)
	{
		FDebugLog::LogError(L"Session Matchmaking (OnSendRequestToJoinCompleteCallback): EOS_CustomInvites_SendRequestToJoinCallbackInfo is null");
	}

	if (!EOS_EResult_IsOperationComplete(Data->ResultCode))
	{
		FDebugLog::Log(L"Session Matchmaking (OnSendRequestToJoinCompleteCallback): operation not complete: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
	}
	else if (Data->ResultCode != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"Session Matchmaking (OnSendRequestToJoinCompleteCallback): error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
	}
	else
	{
		FDebugLog::Log(L"Session Matchmaking: request to join session sent successfully.");
	}
}

void EOS_CALL FSessionMatchmaking::OnSendRequestToJoinAcceptedCallback(const EOS_CustomInvites_AcceptRequestToJoinCallbackInfo* Data)
{
	if (Data == nullptr)
	{
		FDebugLog::LogError(L"Session Matchmaking (OnSendRequestToJoinAcceptedCallback): EOS_CustomInvites_SendRequestToJoinCallbackInfo is null");
		return;
	}

	if (!EOS_EResult_IsOperationComplete(Data->ResultCode))
	{
		FDebugLog::Log(L"Session Matchmaking (OnSendRequestToJoinAcceptedCallback): operation not complete: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
	}
	else if (Data->ResultCode != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"Session Matchmaking (OnSendRequestToJoinAcceptedCallback): error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
	}
	else
	{
		FDebugLog::Log(L"Session Matchmaking: request to join session accepted successfully.");
	}
}

void EOS_CALL FSessionMatchmaking::OnSendRequestToJoinRejectedCallback(const EOS_CustomInvites_RejectRequestToJoinCallbackInfo* Data)
{
	if (Data == nullptr)
	{
		FDebugLog::LogError(L"Session Matchmaking (OnSendRequestToJoinRejectedCallback): EOS_CustomInvites_RejectRequestToJoinCallbackInfo is null");
		return;
	}

	if (!EOS_EResult_IsOperationComplete(Data->ResultCode))
	{
		FDebugLog::Log(L"Session Matchmaking (OnSendRequestToJoinRejectedCallback): operation not complete: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
	}
	else if (Data->ResultCode != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"Session Matchmaking (OnSendRequestToJoinRejectedCallback): error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
	}
	else
	{
		FDebugLog::Log(L"Session Matchmaking: request to join session rejected successfully.");
	}
}

void EOS_CALL FSessionMatchmaking::OnRequestToJoinSessionReceivedCallback(const EOS_CustomInvites_RequestToJoinReceivedCallbackInfo* Data)
{
	if (Data == nullptr)
	{
		FDebugLog::Log(L"Session Matchmaking: request to join received callback invoked but the callback data was not set.");
		return;
	}

	FDebugLog::Log(L"Session Matchmaking: request to join received.");

	const FSession* PresenceSession = FGame::Get().GetSessions()->GetPresenceSession();

	if (PresenceSession == nullptr)
	{
		FDebugLog::LogError(L"Session Matchmaking: request to join received but there is no presence session.");
		FGame::Get().GetSessions()->RejectRequestToJoin(Data->FromUserId);
		return;
	}

	//Show popup
	FGameEvent Event(EGameEventType::RequestToJoinSessionReceived, Data->FromUserId);
	FGame::Get().OnGameEvent(Event);
}

void EOS_CALL FSessionMatchmaking::OnJoinSessionCallback(const EOS_Sessions_JoinSessionCallbackInfo* Data)
{
	if (Data)
	{
		if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
		{
			// Operation is retrying so it is not complete yet
			return;
		}

		if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Session Matchmaking (OnJoinSessionCallback): error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else
		{
			FDebugLog::Log(L"Session Matchmaking: joined session successfully.");

			//add joined session to the list of current sessions
			FGame::Get().GetSessions()->OnJoinSessionFinished();
		}

		// Always inform the UI of the completed join game attempt.
		FGame::Get().GetSessions()->AcknowledgeEventId(Data->ResultCode);
	}
	else
	{
		// Should be impossible; inform the UI of the completed join game attempt.
		FGame::Get().GetSessions()->AcknowledgeEventId(EOS_EResult::EOS_UnexpectedError);
		FDebugLog::LogError(L"Session Matchmaking (OnJoinSessionCallback): EOS_Sessions_JoinSessionCallbackInfo is null");
	}
}

void EOS_CALL FSessionMatchmaking::OnPresenceJoinGameAcceptedCallback(const EOS_Presence_JoinGameAcceptedCallbackInfo* Data)
{
	if (Data)
	{
		FDebugLog::Log(L"Session Matchmaking: join game accepted successfully.");

		//add joined session to the list of current sessions
		FGame::Get().GetSessions()->OnJoinGameAcceptedByJoinInfo(Data->JoinInfo, Data->UiEventId);
	}
	else
	{
		FDebugLog::LogError(L"Session Matchmaking (OnPresenceJoinGameAcceptedCallback): EOS_Presence_JoinGameAcceptedCallbackInfo is null");
	}
}

void EOS_CALL FSessionMatchmaking::OnSessionsJoinSessionAcceptedCallback(const EOS_Sessions_JoinSessionAcceptedCallbackInfo* Data)
{
	if (Data)
	{
		FDebugLog::Log(L"Session Matchmaking: join game accepted successfully.");

		//add joined session to the list of current sessions
		FGame::Get().GetSessions()->OnJoinGameAcceptedByEventId(Data->UiEventId);
	}
	else
	{
		FDebugLog::LogError(L"Session Matchmaking (OnSessionsJoinGameAcceptedCallback): EOS_Sessions_JoinGameAcceptedCallbackInfo is null");
	}
}

void EOS_CALL FSessionMatchmaking::OnSetPresenceCallback(const EOS_Presence_SetPresenceCallbackInfo* Data)
{
	if (!Data)
	{
		FDebugLog::LogError(L"Session Matchmaking (OnSetPresenceCallback): EOS_Presence_SetPresenceCallbackInfo is null");
	}
	else if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
	{
		// Operation is retrying so it is not complete yet
		return;
	}
	else if (Data->ResultCode != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"Session Matchmaking (OnSetPresenceCallback): error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
	}
	else
	{
		FDebugLog::Log(L"Session Matchmaking: set presence successfully.");
	}

}

void EOS_CALL FSessionMatchmaking::OnLeaveSessionRequestedCallback(const EOS_Sessions_LeaveSessionRequestedCallbackInfo* Data)
{
	if (Data)
	{
		FDebugLog::Log(L"Session Matchmaking: leave session requested for session = %ls", FStringUtils::Widen(Data->SessionName).c_str());
		FGame::Get().GetSessions()->DestroySession(Data->SessionName);
	}
	else
	{
		FDebugLog::LogError(L"Session Matchmaking (OnLeaveSessionRequestedCallback): EOS_Sessions_LeaveSessionRequestedCallbackInfo is null");
	}
}
