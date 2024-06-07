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
#include "Lobbies.h"
#include <eos_sdk.h>
#include <eos_lobby.h>
#include <eos_rtc.h>
#include <eos_rtc_audio.h>
#include <eos_rtc_data.h>

void FLobby::InitFromLobbyHandle(EOS_LobbyId InId)
{
	if (InId == nullptr)
	{
		return;
	}

	Id = InId;

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(FPlatform::GetPlatformHandle());
	if (!LobbyHandle)
	{
		FDebugLog::LogError(L"Lobbies: can't get lobby interface.");
		return;
	}

	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player == nullptr)
	{
		FDebugLog::LogError(L"Lobbies - InitLobbyFromHandle: Current player is invalid!");
		return;
	}

	EOS_Lobby_CopyLobbyDetailsHandleOptions CopyHandleOptions = {};
	CopyHandleOptions.ApiVersion = EOS_LOBBY_COPYLOBBYDETAILSHANDLE_API_LATEST;
	CopyHandleOptions.LobbyId = Id.c_str();
	CopyHandleOptions.LocalUserId = Player->GetProductUserID();

	EOS_HLobbyDetails LobbyDetailsHandle = nullptr;
	EOS_EResult Result;
	Result = EOS_Lobby_CopyLobbyDetailsHandle(LobbyHandle, &CopyHandleOptions, &LobbyDetailsHandle);
	if (Result != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"Lobbies: can't get lobby info handle. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
		return;
	}

	LobbyDetailsKeeper DetailsKeeper = MakeLobbyDetailsKeeper(LobbyDetailsHandle);

	InitFromLobbyDetails(DetailsKeeper.get());
}

void FLobby::InitFromLobbyDetails(EOS_HLobbyDetails LobbyDetailsId)
{
	//get owner
	EOS_LobbyDetails_GetLobbyOwnerOptions GetOwnerOptions = {};
	GetOwnerOptions.ApiVersion = EOS_LOBBYDETAILS_GETLOBBYOWNER_API_LATEST;
	FProductUserId NewLobbyOwner = EOS_LobbyDetails_GetLobbyOwner(LobbyDetailsId, &GetOwnerOptions);
	if (NewLobbyOwner != LobbyOwner)
	{
		LobbyOwner = NewLobbyOwner;
		LobbyOwnerAccountId = FEpicAccountId();
		LobbyOwnerDisplayName.clear();
	}

	//copy lobby info
	EOS_LobbyDetails_CopyInfoOptions CopyInfoDetails = {};
	CopyInfoDetails.ApiVersion = EOS_LOBBYDETAILS_COPYINFO_API_LATEST;
	EOS_LobbyDetails_Info* LobbyInfo = nullptr;
	EOS_EResult Result = EOS_LobbyDetails_CopyInfo(LobbyDetailsId, &CopyInfoDetails, &LobbyInfo);
	if (Result != EOS_EResult::EOS_Success || !LobbyInfo)
	{
		FDebugLog::LogError(L"Lobbies: can't copy lobby info. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
		return;
	}

	Id = LobbyInfo->LobbyId;
	MaxNumLobbyMembers = LobbyInfo->MaxMembers;
	Permission = LobbyInfo->PermissionLevel;
	bAllowInvites = LobbyInfo->bAllowInvites;
	AvailableSlots = LobbyInfo->AvailableSlots;
	BucketId.assign(LobbyInfo->BucketId);
	bRTCRoomEnabled = LobbyInfo->bRTCRoomEnabled != EOS_FALSE;
	bPresenceEnabled = LobbyInfo->bPresenceEnabled != EOS_FALSE;

	EOS_LobbyDetails_Info_Release(LobbyInfo);


	//get attributes
	Attributes.clear();
	EOS_LobbyDetails_GetAttributeCountOptions CountOptions = {};
	CountOptions.ApiVersion = EOS_LOBBYDETAILS_GETATTRIBUTECOUNT_API_LATEST;
	const uint32_t AttrCount = EOS_LobbyDetails_GetAttributeCount(LobbyDetailsId, &CountOptions);
	for (uint32_t AttrIndex = 0; AttrIndex < AttrCount; ++AttrIndex)
	{
		EOS_LobbyDetails_CopyAttributeByIndexOptions AttrOptions = {};
		AttrOptions.ApiVersion = EOS_LOBBYDETAILS_COPYATTRIBUTEBYINDEX_API_LATEST;
		AttrOptions.AttrIndex = AttrIndex;
		
		EOS_Lobby_Attribute* Attr = NULL;
		EOS_EResult AttrCopyResult = EOS_LobbyDetails_CopyAttributeByIndex(LobbyDetailsId, &AttrOptions, &Attr);
		if (AttrCopyResult == EOS_EResult::EOS_Success && Attr->Data)
		{
			FLobbyAttribute NextAttribute;
			NextAttribute.InitFromAttribute(Attr);
			Attributes.push_back(NextAttribute);
		}

		//Release attribute
		EOS_Lobby_Attribute_Release(Attr);
	}

	//get members
	std::vector<FLobbyMember> OldMembers(std::move(Members));
	Members.clear();

	EOS_LobbyDetails_GetMemberCountOptions MemberCountOptions = {};
	MemberCountOptions.ApiVersion = EOS_LOBBYDETAILS_GETMEMBERCOUNT_API_LATEST;
	const uint32_t MemberCount = EOS_LobbyDetails_GetMemberCount(LobbyDetailsId, &MemberCountOptions);
	Members.resize(MemberCount);
	for (uint32_t MemberIndex = 0; MemberIndex < MemberCount; ++MemberIndex)
	{
		EOS_LobbyDetails_GetMemberByIndexOptions MemberOptions = {};
		MemberOptions.ApiVersion = EOS_LOBBYDETAILS_GETMEMBERBYINDEX_API_LATEST;
		MemberOptions.MemberIndex = MemberIndex;
		EOS_ProductUserId MemberId = EOS_LobbyDetails_GetMemberByIndex(LobbyDetailsId, &MemberOptions);
		Members[MemberIndex].ProductId = MemberId;

		//member attributes
		EOS_LobbyDetails_GetMemberAttributeCountOptions MemberAttrCountOptions = {};
		MemberAttrCountOptions.ApiVersion = EOS_LOBBYDETAILS_GETMEMBERATTRIBUTECOUNT_API_LATEST;
		MemberAttrCountOptions.TargetUserId = MemberId;
		const uint32_t MemberAttrCount = EOS_LobbyDetails_GetMemberAttributeCount(LobbyDetailsId, &MemberAttrCountOptions);
		Members[MemberIndex].MemberAttributes.resize(MemberAttrCount);
		for (uint32_t AttributeIndex = 0; AttributeIndex < MemberAttrCount; ++AttributeIndex)
		{
			EOS_LobbyDetails_CopyMemberAttributeByIndexOptions MemberAttrCopyOptions = {};
			MemberAttrCopyOptions.ApiVersion = EOS_LOBBYDETAILS_COPYMEMBERATTRIBUTEBYINDEX_API_LATEST;
			MemberAttrCopyOptions.AttrIndex = AttributeIndex;
			MemberAttrCopyOptions.TargetUserId = MemberId;
			EOS_Lobby_Attribute* MemberAttribute = nullptr;
			Result = EOS_LobbyDetails_CopyMemberAttributeByIndex(LobbyDetailsId, &MemberAttrCopyOptions, &MemberAttribute);
			if (Result != EOS_EResult::EOS_Success)
			{
				FDebugLog::LogError(L"Lobbies: can't copy member attribute. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
				continue;
			}

			FLobbyAttribute NewAttribute;
			NewAttribute.InitFromAttribute(MemberAttribute);
			Members[MemberIndex].MemberAttributes[AttributeIndex] = NewAttribute;
			if (NewAttribute.Key == "SKIN")
			{
				Members[MemberIndex].InitSkinFromString(NewAttribute.AsString);
			}

			EOS_Lobby_Attribute_Release(MemberAttribute);
		}

		// Copy RTC Status from OldMembers
		for (const FLobbyMember& OldMember : OldMembers)
		{
			FLobbyMember& NewMember = Members[MemberIndex];
			if (OldMember.ProductId != Members[MemberIndex].ProductId)
			{
				continue;
			}

			// Copy RTC status to new object
			NewMember.RTCState = OldMember.RTCState;
			NewMember.CurrentColor = OldMember.CurrentColor;
			break;
		}
	}
}

FLobbySearch::~FLobbySearch()
{
	Release();
}

void FLobbySearch::Release()
{
	SearchResults.clear();

	if (SearchHandle)
	{
		EOS_LobbySearch_Release(SearchHandle);
		SearchHandle = nullptr;
	}

	ResultHandles.clear();
}

void FLobbySearch::SetNewSearch(EOS_HLobbySearch Handle)
{
	Release();
	SearchHandle = Handle;
}

void FLobbySearch::OnSearchResultsReceived(std::vector<FLobby>&& Results, std::vector<LobbyDetailsKeeper>&& Handles)
{
	SearchResults.swap(Results);
	ResultHandles.swap(Handles);
}


FLobbies::FLobbies()
{
	
}

FLobbies::~FLobbies()
{

}

void FLobbies::OnShutdown()
{
	LeaveLobby();
	UnsubscribeFromLobbyInvites();
	UnsubscribeFromLobbyUpdates();
	UnsubscribeFromLeaveLobbyUI();
}

void FLobbies::Update()
{
	if (!CurrentUserProductId.IsValid())
	{
		return;
	}

	if (bDirty)
	{
		//Check if we need to get account mappings and/or display names for members
		std::vector<FEpicAccountId> DisplayNameQuery;
		std::vector<FProductUserId> AccountMappingQuery;

		if (CurrentLobby.IsValid())
		{
			//members
			for (FLobbyMember& NextMember : CurrentLobby.Members)
			{
				if (!NextMember.AccountId.IsValid())
				{
					NextMember.AccountId = FGame::Get().GetUsers()->GetAccountMapping(NextMember.ProductId);

					//Still invalid: need to query
					if (!NextMember.AccountId.IsValid())
					{
						AccountMappingQuery.push_back(NextMember.ProductId);
					}
				}
				if(NextMember.AccountId.IsValid())
				{
					if (NextMember.DisplayName.empty())
					{
						NextMember.DisplayName = FGame::Get().GetUsers()->GetDisplayName(NextMember.AccountId);
						if (NextMember.DisplayName.empty())
						{
							DisplayNameQuery.push_back(NextMember.AccountId);
						}
					}
				}
			}

			//lobby owner
			if (!CurrentLobby.LobbyOwnerAccountId.IsValid())
			{
				CurrentLobby.LobbyOwnerAccountId = FGame::Get().GetUsers()->GetAccountMapping(CurrentLobby.LobbyOwner);

				if (!CurrentLobby.LobbyOwnerAccountId.IsValid())
				{
					AccountMappingQuery.push_back(CurrentLobby.LobbyOwner);
				}
			}
			if(CurrentLobby.LobbyOwnerAccountId.IsValid())
			{
				if (CurrentLobby.LobbyOwnerDisplayName.empty())
				{
					CurrentLobby.LobbyOwnerDisplayName = FGame::Get().GetUsers()->GetDisplayName(CurrentLobby.LobbyOwnerAccountId);
					if (CurrentLobby.LobbyOwnerDisplayName.empty())
					{
						DisplayNameQuery.push_back(CurrentLobby.LobbyOwnerAccountId);
					}
				}
			}
		}

		//Search results
		if (CurrentSearch.IsValid())
		{
			for (FLobby& NextSearchResult : CurrentSearch.GetResults())
			{
				if (NextSearchResult.IsValid())
				{
					if (!NextSearchResult.LobbyOwnerAccountId.IsValid())
					{
						NextSearchResult.LobbyOwnerAccountId = FGame::Get().GetUsers()->GetAccountMapping(NextSearchResult.LobbyOwner);
						if (!NextSearchResult.LobbyOwnerAccountId.IsValid())
						{
							AccountMappingQuery.push_back(NextSearchResult.LobbyOwner);
						}
					}
					if(NextSearchResult.LobbyOwnerAccountId.IsValid())
					{
						if (NextSearchResult.LobbyOwnerDisplayName.empty())
						{
							NextSearchResult.LobbyOwnerDisplayName = FGame::Get().GetUsers()->GetDisplayName(NextSearchResult.LobbyOwnerAccountId);
							if (NextSearchResult.LobbyOwnerDisplayName.empty())
							{
								DisplayNameQuery.push_back(NextSearchResult.LobbyOwnerAccountId);
							}
						}
					}
				}
			}
		}

		//Invites (query account mappings and/or user display names for users that send us invite)
		for (std::pair<const FProductUserId, FLobbyInvite>& NextInvitePair : Invites)
		{
			FLobbyInvite& NextInvite = NextInvitePair.second;
			if (NextInvite.FriendId.IsValid())
			{
				if (!NextInvite.FriendEpicId.IsValid())
				{
					NextInvite.FriendEpicId = FGame::Get().GetUsers()->GetAccountMapping(NextInvite.FriendId);
					if (!NextInvite.FriendEpicId)
					{
						AccountMappingQuery.push_back(NextInvite.FriendId);
					}
				}
				if (NextInvite.FriendEpicId.IsValid())
				{
					if (NextInvite.FriendDisplayName.empty())
					{
						NextInvite.FriendDisplayName = FGame::Get().GetUsers()->GetDisplayName(NextInvite.FriendEpicId);
						if (NextInvite.FriendDisplayName.empty())
						{
							DisplayNameQuery.push_back(NextInvite.FriendEpicId);
						}
					}
				}
			}
		}
		
		if (!CurrentUserProductId.IsValid())
		{
			FDebugLog::LogError(L"Lobbies - Update: Current player is invalid!");
			return;
		}

		//Query display names and account mappings
		if (!AccountMappingQuery.empty())
		{
			FGame::Get().GetUsers()->QueryAccountMappings(CurrentUserProductId, AccountMappingQuery);
		}

		for (FEpicAccountId NextId : DisplayNameQuery)
		{
			FGame::Get().GetUsers()->QueryDisplayName(NextId);
		}

		bDirty = false;
	}
}

void FLobbies::OnLoggedIn(FEpicAccountId UserId)
{
	bDirty = true;
	CurrentInvite = nullptr;
}

void FLobbies::OnLoggedOut(FEpicAccountId UserId)
{
	if (CurrentLobby.IsValid())
	{
		LeaveLobby();
	}

	CurrentSearch.Release();
	CurrentLobby.Clear();

	CurrentInvite = nullptr;
	Invites.clear();
}

void FLobbies::OnUserConnectLoggedIn(FProductUserId ProductUserId)
{
	CurrentUserProductId = ProductUserId;
	CurrentInvite = nullptr;
	bDirty = true;
}

void FLobbies::OnGameEvent(const FGameEvent& Event)
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
	else if (Event.GetType() == EGameEventType::EpicAccountsMappingRetrieved ||
		Event.GetType() == EGameEventType::ExternalAccountsMappingRetrieved ||
		Event.GetType() == EGameEventType::UserInfoRetrieved)
	{
		bDirty = true;
	}
}

void FLobbies::PopLobbyInvite()
{
	//Clear current invite (if any)
	if (CurrentInvite)
	{
		auto CurrentInviteIter = Invites.find(CurrentInvite->FriendId);
		if (CurrentInviteIter != Invites.end())
		{
			Invites.erase(CurrentInviteIter);
		}

		CurrentInvite = nullptr;
	}

	//Enable next invite
	if (!Invites.empty())
	{
		auto NextInviteIter = Invites.begin();
		CurrentInvite = &NextInviteIter->second;

		FGameEvent GameEvent(EGameEventType::LobbyInviteReceived);
		FGame::Get().OnGameEvent(GameEvent);
	}
}

bool FLobbies::CanInviteToCurrentLobby(FProductUserId ProductUserId) const
{
	if (!HasActiveLobby())
	{
		return false;
	}

	if (!ProductUserId.IsValid())
	{
		return false;
	}

	if (!CurrentLobby.bAllowInvites ||
		CurrentLobby.AvailableSlots == 0 ||
		CurrentLobby.Members.size() >= CurrentLobby.MaxNumLobbyMembers)
	{
		return false;
	}

	//Check if already in the lobby
	for (size_t MemberIndex = 0; MemberIndex < CurrentLobby.Members.size(); ++MemberIndex)
	{
		if (CurrentLobby.Members[MemberIndex].ProductId == ProductUserId)
		{
			return false;
		}
	}

	return true;
}

bool FLobbies::CreateLobby(const FLobby& Lobby)
{
	if (!CurrentUserProductId.IsValid())
	{
		FDebugLog::LogError(L"Lobbies - CreateLobby: Current player is invalid!");
		return false;
	}

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(FPlatform::GetPlatformHandle());
	if (!LobbyHandle)
	{
		FDebugLog::LogError(L"Lobbies: can't get lobby interface.");
		return false;
	}

	//Check if there is current session. Leave it.
	if (CurrentLobby.IsValid())
	{
		LeaveLobby();
	}

	EOS_Lobby_CreateLobbyOptions CreateOptions = {};
	CreateOptions.ApiVersion = EOS_LOBBY_CREATELOBBY_API_LATEST;
	CreateOptions.LocalUserId = CurrentUserProductId;
	CreateOptions.MaxLobbyMembers = Lobby.MaxNumLobbyMembers;
	CreateOptions.PermissionLevel = Lobby.Permission;
	CreateOptions.bPresenceEnabled = Lobby.IsPresenceEnabled();
	CreateOptions.bAllowInvites = Lobby.AreInvitesAllowed();
	CreateOptions.BucketId = Lobby.BucketId.c_str();
	CreateOptions.bDisableHostMigration = EOS_FALSE;
	CreateOptions.LobbyId = nullptr;

	EOS_Lobby_LocalRTCOptions LocalRTCOptions = {};
	if (Lobby.IsRTCRoomEnabled())
	{
		CreateOptions.bEnableRTCRoom = EOS_TRUE;

		LocalRTCOptions.ApiVersion = EOS_LOBBY_LOCALRTCOPTIONS_API_LATEST;
		LocalRTCOptions.Flags = EOS_RTC_JOINROOMFLAGS_ENABLE_DATACHANNEL; // Enable data channel so it can be used to notify about skin color changes
		LocalRTCOptions.bUseManualAudioInput = EOS_FALSE;
		LocalRTCOptions.bUseManualAudioOutput = EOS_FALSE;
		LocalRTCOptions.bLocalAudioDeviceInputStartsMuted = EOS_FALSE;

		CreateOptions.LocalRTCOptions = &LocalRTCOptions;
	}
	else
	{
		CreateOptions.bEnableRTCRoom = EOS_FALSE;
		CreateOptions.LocalRTCOptions = nullptr;
	}

	EOS_Lobby_CreateLobby(LobbyHandle, &CreateOptions, nullptr, OnCreateLobbyFinished);

	//Save lobby data for modification
	CurrentLobby = Lobby;
	CurrentLobby.bBeingCreated = true;
	CurrentLobby.LobbyOwner = CurrentUserProductId;

	return true;
}

bool FLobbies::DestroyCurrentLobby()
{
	if (!CurrentLobby.IsValid())
	{
		return false;
	}

	UnsubscribeFromRTCEvents();

	if (!CurrentUserProductId.IsValid())
	{
		FDebugLog::LogError(L"Lobbies - DestroyCurrentLobby: Current player is invalid!");
		return false;
	}

	if (!CurrentLobby.IsOwner(CurrentUserProductId))
	{
		FDebugLog::LogError(L"Lobbies - DestroyCurrentLobby: Current player is now lobby owner!");
		return false;
	}

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(FPlatform::GetPlatformHandle());

	EOS_Lobby_DestroyLobbyOptions DestroyOptions = {};
	DestroyOptions.ApiVersion = EOS_LOBBY_DESTROYLOBBY_API_LATEST;
	DestroyOptions.LocalUserId = CurrentUserProductId;
	DestroyOptions.LobbyId = CurrentLobby.Id.c_str();

	EOS_Lobby_DestroyLobby(LobbyHandle, &DestroyOptions, nullptr, OnDestroyLobbyFinished);

	//No matter the result we clear current lobby.
	CurrentLobby = FLobby();

	return true;
}


bool FLobbies::IsReadyToShutdown() const
{
	if (CurrentLobby.IsValid())
	{
		return false;
	}

	return !bLobbyLeaveInProgress;
}

bool FLobbies::ModifyLobby(const FLobby& LobbyChanges)
{
	if (!CurrentLobby.IsValid())
	{
		FDebugLog::LogError(L"Lobbies - ModifyLobby: Current lobby is invalid.");
		return false;
	}

	if (!CurrentUserProductId.IsValid())
	{
		FDebugLog::LogError(L"Lobbies - ModifyLobby: Current player is invalid!");
		return false;
	}

	if (!CurrentLobby.IsOwner(CurrentUserProductId))
	{
		FDebugLog::LogError(L"Lobbies - ModifyLobby: Only owner can modify lobby.");
		return false;
	}

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(FPlatform::GetPlatformHandle());

	EOS_Lobby_UpdateLobbyModificationOptions ModifyOptions = {};
	ModifyOptions.ApiVersion = EOS_LOBBY_UPDATELOBBYMODIFICATION_API_LATEST;
	ModifyOptions.LobbyId = CurrentLobby.Id.c_str();
	ModifyOptions.LocalUserId = CurrentUserProductId;

	EOS_HLobbyModification LobbyModification = nullptr;
	EOS_EResult Result = EOS_Lobby_UpdateLobbyModification(LobbyHandle, &ModifyOptions, &LobbyModification);

	if (Result != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"Lobbies - ModifyLobby: Could not create lobby modification. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
		return false;
	}

	LobbyModificationKeeper ModKeeper = MakeLobbyDetailsKeeper(LobbyModification);

	//TODO: check if modification is needed by comparing current lobby against LobbyChanges

	// Lobby type
	EOS_LobbyModification_SetPermissionLevelOptions SetPermissionOptions = {};
	SetPermissionOptions.ApiVersion = EOS_LOBBYMODIFICATION_SETPERMISSIONLEVEL_API_LATEST;
	SetPermissionOptions.PermissionLevel = LobbyChanges.Permission;
	Result = EOS_LobbyModification_SetPermissionLevel(LobbyModification, &SetPermissionOptions);
	if (Result != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"Lobbies - ModifyLobby: Could not set permission level. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
		return false;
	}

	// BucketId
	if (!LobbyChanges.BucketId.empty())
	{
		EOS_LobbyModification_SetBucketIdOptions SetBucketIdOptions = {};
		SetBucketIdOptions.ApiVersion = EOS_LOBBYMODIFICATION_SETBUCKETID_API_LATEST;
		SetBucketIdOptions.BucketId = LobbyChanges.BucketId.c_str();
		Result = EOS_LobbyModification_SetBucketId(LobbyModification, &SetBucketIdOptions);
		if (Result != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Lobbies - ModifyLobby: Could not set the bucket id. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
			return false;
		}
	}

	// Invites allowed
	EOS_LobbyModification_SetInvitesAllowedOptions SetInvitesAllowedOptions = {};
	SetInvitesAllowedOptions.ApiVersion = EOS_LOBBYMODIFICATION_SETINVITESALLOWED_API_LATEST;
	SetInvitesAllowedOptions.bInvitesAllowed = LobbyChanges.AreInvitesAllowed() ? EOS_TRUE : EOS_FALSE;
	Result = EOS_LobbyModification_SetInvitesAllowed(LobbyModification, &SetInvitesAllowedOptions);
	if (Result != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"Lobbies - ModifyLobby: Could not set invites allowed. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
		return false;
	}

	// Max Players
	if (LobbyChanges.MaxNumLobbyMembers > 0)
	{
		EOS_LobbyModification_SetMaxMembersOptions SetMaxMembersOptions = {};
		SetMaxMembersOptions.ApiVersion = EOS_LOBBYMODIFICATION_SETMAXMEMBERS_API_LATEST;
		SetMaxMembersOptions.MaxMembers = LobbyChanges.MaxNumLobbyMembers;
		Result = EOS_LobbyModification_SetMaxMembers(LobbyModification, &SetMaxMembersOptions);
		if (Result != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Lobbies - ModifyLobby: Could not set max players. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
			return false;
		}
	}

	// Add Attributes (todo : check diff)
	for (size_t AttributeIndex = 0; AttributeIndex < LobbyChanges.Attributes.size(); ++AttributeIndex)
	{
		EOS_LobbyModification_AddAttributeOptions AddAttributeModOptions = {};
		AddAttributeModOptions.ApiVersion = EOS_LOBBYMODIFICATION_ADDATTRIBUTE_API_LATEST;
		EOS_Lobby_AttributeData AttributeData = LobbyChanges.Attributes[AttributeIndex].ToAttributeData();
		AddAttributeModOptions.Attribute = &AttributeData;
		AddAttributeModOptions.Visibility = LobbyChanges.Attributes[AttributeIndex].Visibility;

		Result = EOS_LobbyModification_AddAttribute(LobbyModification, &AddAttributeModOptions);
		if (Result != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Lobbies - ModifyLobby: Could not add attribute. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
			return false;
		}
	}

	//Trigger lobby update
	EOS_Lobby_UpdateLobbyOptions UpdateOptions = {};
	UpdateOptions.ApiVersion = EOS_LOBBY_UPDATELOBBY_API_LATEST;
	UpdateOptions.LobbyModificationHandle = ModKeeper.get();
	EOS_Lobby_UpdateLobby(LobbyHandle, &UpdateOptions, nullptr, OnLobbyUpdateFinished);

	return true;
}

void FLobbies::UpdateLobby()
{
	if (CurrentLobby.IsValid())
	{
		CurrentLobby.InitFromLobbyHandle(CurrentLobby.Id.c_str());
		bDirty = true;
	}
}

void FLobbies::KickMember(EOS_ProductUserId MemberId)
{
	if (!MemberId)
	{
		return;
	}

	
	if (!CurrentUserProductId.IsValid())
	{
		FDebugLog::LogError(L"Lobbies - KickMember: Current player is invalid!");
		return;
	}

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(FPlatform::GetPlatformHandle());

	EOS_Lobby_KickMemberOptions KickMemberOptions = {};
	KickMemberOptions.ApiVersion = EOS_LOBBY_KICKMEMBER_API_LATEST;
	KickMemberOptions.TargetUserId = MemberId;
	KickMemberOptions.LobbyId = CurrentLobby.Id.c_str();
	KickMemberOptions.LocalUserId = CurrentUserProductId;

	EOS_Lobby_KickMember(LobbyHandle, &KickMemberOptions, nullptr, OnKickMemberFinished);
}

void FLobbies::PromoteMember(EOS_ProductUserId MemberId)
{
	if (!MemberId)
	{
		return;
	}

	if (!CurrentUserProductId.IsValid())
	{
		FDebugLog::LogError(L"Lobbies - PromoteMember: Current player is invalid!");
		return;
	}

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(FPlatform::GetPlatformHandle());

	EOS_Lobby_PromoteMemberOptions PromoteMemberOptions = {};
	PromoteMemberOptions.ApiVersion = EOS_LOBBY_PROMOTEMEMBER_API_LATEST;
	PromoteMemberOptions.TargetUserId = MemberId;
	PromoteMemberOptions.LocalUserId = CurrentUserProductId;
	PromoteMemberOptions.LobbyId = CurrentLobby.Id.c_str();

	EOS_Lobby_PromoteMember(LobbyHandle, &PromoteMemberOptions, nullptr, OnPromoteMemberFinished);
}

void FLobbies::JoinLobby(EOS_LobbyId Id, LobbyDetailsKeeper LobbyInfo, bool bPresenceEnabled)
{
	if (!Id)
	{
		return;
	}

	if (CurrentLobby.IsValid() && CurrentLobby.GetMemberByProductUserId(CurrentUserProductId) != nullptr)
	{
		if (CurrentLobby.Id == Id)
		{
			FDebugLog::LogError(L"Lobbies - JoinLobby: Already in the lobby!");
			return;
		}

		ActiveJoin = FLobbyJoinRequest{ Id, LobbyInfo, bPresenceEnabled };
		LeaveLobby();
	}
	else
	{
		if (!CurrentUserProductId.IsValid())
		{
			FDebugLog::LogError(L"Lobbies - JoinLobby: Current player is invalid!");
			return;
		}

		EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(FPlatform::GetPlatformHandle());

		EOS_Lobby_JoinLobbyOptions JoinOptions = {};
		JoinOptions.ApiVersion = EOS_LOBBY_JOINLOBBY_API_LATEST;
		JoinOptions.LobbyDetailsHandle = LobbyInfo.get();
		JoinOptions.LocalUserId = CurrentUserProductId;
		JoinOptions.bPresenceEnabled = bPresenceEnabled;

		EOS_Lobby_LocalRTCOptions LocalRTCOptions = {};
		LocalRTCOptions.ApiVersion = EOS_LOBBY_LOCALRTCOPTIONS_API_LATEST;
		LocalRTCOptions.Flags = EOS_RTC_JOINROOMFLAGS_ENABLE_DATACHANNEL; // Enable data channel so it can be used to notify about skin color changes
		LocalRTCOptions.bUseManualAudioInput = EOS_FALSE;
		LocalRTCOptions.bUseManualAudioOutput = EOS_FALSE;
		LocalRTCOptions.bLocalAudioDeviceInputStartsMuted = EOS_FALSE;

		JoinOptions.LocalRTCOptions = &LocalRTCOptions;

		EOS_Lobby_JoinLobby(LobbyHandle, &JoinOptions, nullptr, OnJoinLobbyFinished);
	}
}

void FLobbies::JoinSearchResult(size_t Index)
{
	if (CurrentSearch.GetResults().size() > Index && CurrentSearch.GetDetailsHandles().size() > Index)
	{
		// Currently, joining from the sample UI will not allow joining a lobby in presence enabled mode.
		// It is required that all Lobbies initiated by the overlay UI be marked as presence enabled,
		// but the state of that flag can be defined for any lobbies initialed entirely by the game.
		const bool bPresenceEnabled = false;
		JoinLobby(CurrentSearch.GetResults()[Index].Id.c_str(), CurrentSearch.GetDetailsHandles()[Index], bPresenceEnabled);
	}
}

void FLobbies::RejectLobbyInvite(const std::string& InviteId)
{
	if (CurrentInvite->IsValid() && CurrentInvite->InviteId == InviteId)
	{
		if (!CurrentUserProductId.IsValid())
		{
			FDebugLog::LogError(L"Lobbies - RejectLobbyInvite: Current player is invalid!");
			return;
		}

		EOS_Lobby_RejectInviteOptions Options = {};
		Options.ApiVersion = EOS_LOBBY_REJECTINVITE_API_LATEST;
		Options.InviteId = InviteId.c_str();
		Options.LocalUserId = CurrentUserProductId;

		EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(FPlatform::GetPlatformHandle());

		EOS_Lobby_RejectInvite(LobbyHandle, &Options, nullptr, OnRejectInviteFinished);

		PopLobbyInvite();
	}

	//In case lobby id does not match current invite the reject can be ignored
}

void FLobbies::LeaveLobby()
{
	if (!CurrentLobby.IsValid())
	{
		return;
	}

	UnsubscribeFromRTCEvents();

	if (!CurrentUserProductId.IsValid())
	{
		FDebugLog::LogError(L"Lobbies - LeaveLobby: Current player is invalid!");
		return;
	}
	
	bLobbyLeaveInProgress = true;

	// Set this to true to test flow where owner always destroys lobby
	const bool bOwnerAlwaysDestroysLobby = false;
	const bool bDestroyLobby = CurrentLobby.IsOwner(CurrentUserProductId) && (bOwnerAlwaysDestroysLobby || CurrentLobby.Members.size() <= 1);

	//Destroy empty lobby on leave
	if (bDestroyLobby)
	{
		DestroyCurrentLobby();
		return;
	}

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(FPlatform::GetPlatformHandle());

	EOS_Lobby_LeaveLobbyOptions LeaveOptions = {};
	LeaveOptions.ApiVersion = EOS_LOBBY_LEAVELOBBY_API_LATEST;
	LeaveOptions.LobbyId = CurrentLobby.Id.c_str();
	LeaveOptions.LocalUserId = CurrentUserProductId;
	EOS_Lobby_LeaveLobby(LobbyHandle, &LeaveOptions, nullptr, OnLeaveLobbyFinished);
	
	//No matter the result we clear current lobby.
	CurrentLobby = FLobby();
}

void FLobbies::SendInvite(FProductUserId TargetUserId)
{
	if (!CurrentLobby.IsValid() || !TargetUserId)
	{
		return;
	}

	if (!CurrentUserProductId.IsValid())
	{
		FDebugLog::LogError(L"Lobbies - SendInvite: Current player is invalid!");
		return;
	}

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(FPlatform::GetPlatformHandle());

	EOS_Lobby_SendInviteOptions SendInviteOptions = {};
	SendInviteOptions.ApiVersion = EOS_LOBBY_SENDINVITE_API_LATEST;
	SendInviteOptions.LobbyId = CurrentLobby.Id.c_str();
	SendInviteOptions.LocalUserId = CurrentUserProductId;
	SendInviteOptions.TargetUserId = TargetUserId;

	EOS_Lobby_SendInvite(LobbyHandle, &SendInviteOptions, nullptr, OnLobbyInviteSendFinished);
}

void FLobbies::ShuffleSkin()
{
	if (!CurrentLobby.IsValid())
	{
		return;
	}

	if (!CurrentUserProductId.IsValid())
	{
		FDebugLog::LogError(L"Lobbies - ShuffleSkin: Current player is invalid!");
		return;
	}

	if (FLobbyMember* LocalLobbyMember = CurrentLobby.GetMemberByProductUserId(CurrentUserProductId))
	{
		if (LocalLobbyMember->MemberAttributes.empty())
		{
			FLobbyAttribute SkinAttribute;
			SkinAttribute.Key = "SKIN";
			SkinAttribute.ValueType = FLobbyAttribute::String;
			SkinAttribute.AsString = FLobbyMember::GetSkinString(LocalLobbyMember->CurrentSkin);
			LocalLobbyMember->MemberAttributes.push_back(SkinAttribute);
		}
		else
		{
			LocalLobbyMember->ShuffleSkin();
			LocalLobbyMember->MemberAttributes[0].AsString = FLobbyMember::GetSkinString(LocalLobbyMember->CurrentSkin);
		}

		SetMemberAttribute(LocalLobbyMember->MemberAttributes[0]);
	}
}

void FLobbies::ShuffleColor()
{
	if (!CurrentLobby.IsValid())
	{
		return;
	}

	if (!CurrentUserProductId.IsValid())
	{
		FDebugLog::LogError(L"Lobbies - ShuffleColor: Current player is invalid!");
		return;
	}

	if (FLobbyMember* LocalLobbyMember = CurrentLobby.GetMemberByProductUserId(CurrentUserProductId))
	{
		LocalLobbyMember->ShuffleColor();			
		SendSkinColorUpdate(LocalLobbyMember->CurrentColor);
	}
}

void FLobbies::SendSkinColorUpdate(FLobbyMember::SkinColor InColor)
{
	const size_t CommandSize = sizeof(ELobbyRTCDataCommand) + sizeof(FLobbyMember::SkinColor);
	uint8_t Data[CommandSize];

	Data[0] = static_cast<uint8_t>(ELobbyRTCDataCommand::SkinColor);
	Data[1] = static_cast<uint8_t>(InColor);

	EOS_RTCData_SendDataOptions SendDataOptions{};
	SendDataOptions.ApiVersion = EOS_RTCDATA_SENDDATA_API_LATEST;
	SendDataOptions.LocalUserId = CurrentUserProductId;
	SendDataOptions.RoomName = CurrentLobby.RTCRoomName.c_str();
	SendDataOptions.Data = Data;
	SendDataOptions.DataLengthBytes = CommandSize;

	EOS_HRTC RTCHandle = EOS_Platform_GetRTCInterface(FPlatform::GetPlatformHandle());
	EOS_HRTCData RTCDataHandle = EOS_RTC_GetDataInterface(RTCHandle);

	EOS_EResult Result = EOS_RTCData_SendData(RTCDataHandle, &SendDataOptions);
	if (Result != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"Lobbies: can't send data through data channel. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
	}
}

void FLobbies::MuteAudio(FProductUserId TargetUserId)
{
	EOS_HRTC RTCHandle = EOS_Platform_GetRTCInterface(FPlatform::GetPlatformHandle());
	EOS_HRTCAudio AudioHandle = EOS_RTC_GetAudioInterface(RTCHandle);

	// Find the correct lobby member
	if (FLobbyMember* LobbyMember = CurrentLobby.GetMemberByProductUserId(TargetUserId))
	{
		// Do not allow multiple local mutes/unmutes at the same time
		if (LobbyMember->RTCState.bMuteActionInProgress)
		{
			return;
		}

		// Set their mute action as in progress so we don't try to toggle their mute status again until it completes
		LobbyMember->RTCState.bMuteActionInProgress = true;

		// Check if we're muting ourselves vs muting someone else
		if (CurrentUserProductId == TargetUserId)
		{
			// Toggle our Mute/Unmute status
			EOS_RTCAudio_UpdateSendingOptions UpdateSendingOptions = {};
			UpdateSendingOptions.ApiVersion = EOS_RTCAUDIO_UPDATESENDING_API_LATEST;
			UpdateSendingOptions.LocalUserId = CurrentUserProductId;
			UpdateSendingOptions.RoomName = CurrentLobby.RTCRoomName.c_str();
			UpdateSendingOptions.AudioStatus = LobbyMember->RTCState.bIsAudioOutputDisabled ? EOS_ERTCAudioStatus::EOS_RTCAS_Enabled : EOS_ERTCAudioStatus::EOS_RTCAS_Disabled;

			const wchar_t* AudioStatusText = UpdateSendingOptions.AudioStatus == EOS_ERTCAudioStatus::EOS_RTCAS_Enabled ? L"Unmuted" : L"Muted";
			FDebugLog::LogError(L"Lobbies - MuteAudio: Setting audio output status to %ls", AudioStatusText);

			EOS_RTCAudio_UpdateSending(AudioHandle, &UpdateSendingOptions, nullptr, OnRTCRoomUpdateSendingComplete);
		}
		else
		{
			// Mute/Unmute others audio (this is a local-only action and does not block the other user from receiving your audio stream)
			EOS_RTCAudio_UpdateReceivingOptions UpdateReceivingOptions = {};
			UpdateReceivingOptions.ApiVersion = EOS_RTCAUDIO_UPDATERECEIVING_API_LATEST;
			UpdateReceivingOptions.LocalUserId = CurrentUserProductId;
			UpdateReceivingOptions.RoomName = CurrentLobby.RTCRoomName.c_str();
			UpdateReceivingOptions.ParticipantId = TargetUserId;
			UpdateReceivingOptions.bAudioEnabled = LobbyMember->RTCState.bIsLocallyMuted ? EOS_TRUE : EOS_FALSE;

			const wchar_t* AudioStatusText = UpdateReceivingOptions.bAudioEnabled ? L"Unmuting" : L"Muting";
			FDebugLog::LogError(L"Lobbies - MuteAudio: %ls remote player. TargetParticipantId=[%ls]",
				AudioStatusText,
				FStringUtils::Widen(FAccountHelpers::ProductUserIDToString(TargetUserId)).c_str());

			EOS_RTCAudio_UpdateReceiving(AudioHandle, &UpdateReceivingOptions, nullptr, OnRTCRoomUpdateReceivingComplete);
		}
	}
}

void FLobbies::ToggleHardMuteMember(FProductUserId TargetUserId)
{
	if (!TargetUserId)
	{
		FDebugLog::LogError(L"Lobbies - ToggleHardMuteMember: Target player is invalid!");
		return;
	}

	if (!CurrentUserProductId.IsValid())
	{
		FDebugLog::LogError(L"Lobbies - ToggleHardMuteMember: Current player is invalid!");
		return;
	}

	// Find the correct lobby member
	if (FLobbyMember* LobbyMember = CurrentLobby.GetMemberByProductUserId(TargetUserId))
	{
		// Do not allow multiple local mutes/unmutes at the same time
		if (LobbyMember->RTCState.bHardMuteActionInProgress)
		{
			FDebugLog::LogWarning(L"Lobbies - ToggleHardMuteMember: Hard muting %ls remote player is already in progress!",
				FStringUtils::Widen(FAccountHelpers::ProductUserIDToString(TargetUserId)).c_str());
			return;
		}

		// Set their hard mute action as in progress so we don't try to toggle their hard mute status again until it completes
		LobbyMember->RTCState.bHardMuteActionInProgress = true;

		HardMuteMember(TargetUserId, !LobbyMember->RTCState.bIsHardMuted); // toggle hard mute state
	}
}

void FLobbies::HardMuteMember(FProductUserId InTargetUserId, bool bInHardMute)
{
	EOS_Lobby_HardMuteMemberOptions HardMuteMemberOptions = {};
	HardMuteMemberOptions.ApiVersion = EOS_LOBBY_HARDMUTEMEMBER_API_LATEST;
	HardMuteMemberOptions.LobbyId = CurrentLobby.Id.c_str();
	HardMuteMemberOptions.LocalUserId = CurrentUserProductId;
	HardMuteMemberOptions.TargetUserId = InTargetUserId;
	HardMuteMemberOptions.bHardMute = bInHardMute ? EOS_TRUE : EOS_FALSE;

	const wchar_t* HardMuteStatusText = (HardMuteMemberOptions.bHardMute == EOS_TRUE) ? L"muting" : L"unmuting";
	FDebugLog::Log(L"Lobbies - HardMuteMember: %ls remote player. TargetParticipantId=[%ls]",
		HardMuteStatusText,
		FStringUtils::Widen(FAccountHelpers::ProductUserIDToString(InTargetUserId)).c_str());

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(FPlatform::GetPlatformHandle());
	EOS_Lobby_HardMuteMember(LobbyHandle, &HardMuteMemberOptions, nullptr, OnHardMuteMemberFinished);
}

void FLobbies::SetMemberAttribute(const FLobbyAttribute& MemberAttribute)
{
	if (!CurrentLobby.IsValid())
	{
		return;
	}

	if (!CurrentUserProductId.IsValid())
	{
		FDebugLog::LogError(L"Lobbies - SetMemberAttribute: Current player is invalid!");
		return;
	}

	//Modify lobby

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(FPlatform::GetPlatformHandle());

	EOS_Lobby_UpdateLobbyModificationOptions ModifyOptions = {};
	ModifyOptions.ApiVersion = EOS_LOBBY_UPDATELOBBYMODIFICATION_API_LATEST;
	ModifyOptions.LobbyId = CurrentLobby.Id.c_str();
	ModifyOptions.LocalUserId = CurrentUserProductId;

	EOS_HLobbyModification LobbyModification = nullptr;
	EOS_EResult Result = EOS_Lobby_UpdateLobbyModification(LobbyHandle, &ModifyOptions, &LobbyModification);

	if (Result != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"Lobbies - SetMemberAttribute: Could not create lobby modification. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
		return;
	}

	LobbyModificationKeeper ModKeeper = MakeLobbyDetailsKeeper(LobbyModification);

	//Update member attribute
	EOS_Lobby_AttributeData AttributeData = MemberAttribute.ToAttributeData();

	EOS_LobbyModification_AddMemberAttributeOptions AddAttrOptions = {};
	AddAttrOptions.ApiVersion = EOS_LOBBYMODIFICATION_ADDMEMBERATTRIBUTE_API_LATEST;
	AddAttrOptions.Attribute = &AttributeData;
	AddAttrOptions.Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC;

	Result = EOS_LobbyModification_AddMemberAttribute(LobbyModification, &AddAttrOptions);

	if (Result != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"Lobbies - SetMemberAttribute: Could not add member attribute. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
		return;
	}

	//Trigger lobby update
	EOS_Lobby_UpdateLobbyOptions UpdateOptions = {};
	UpdateOptions.ApiVersion = EOS_LOBBY_UPDATELOBBY_API_LATEST;
	UpdateOptions.LobbyModificationHandle = ModKeeper.get();
	EOS_Lobby_UpdateLobby(LobbyHandle, &UpdateOptions, nullptr, OnLobbyUpdateFinished);
}

void FLobbies::SetInitialMemberAttribute()
{
	if (!CurrentLobby.IsValid())
	{
		return;
	}

	
	if (!CurrentUserProductId.IsValid())
	{
		FDebugLog::LogError(L"Lobbies - SetInitialMemberAttribute: Current player is invalid!");
		return;
	}

	if (FLobbyMember* LocalLobbyMember = CurrentLobby.GetMemberByProductUserId(CurrentUserProductId))
	{
		//Check if skin is already set
		if (LocalLobbyMember->MemberAttributes.empty())
		{
			FLobbyAttribute SkinAttribute;
			SkinAttribute.Key = "SKIN";
			SkinAttribute.ValueType = FLobbyAttribute::String;
			SkinAttribute.AsString = FLobbyMember::GetSkinString(LocalLobbyMember->CurrentSkin);
			SetMemberAttribute(SkinAttribute);
		}
	}
}

void FLobbies::Search(const std::vector<FLobbyAttribute>& SearchAttributes, uint32_t MaxNumResults)
{
	//Clear previous search
	CurrentSearch.Release();

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(FPlatform::GetPlatformHandle());

	
	if (!CurrentUserProductId.IsValid())
	{
		FDebugLog::LogError(L"Lobbies - Search: Current player is invalid!");
		return;
	}

	EOS_Lobby_CreateLobbySearchOptions CreateSearchOptions = {};
	CreateSearchOptions.ApiVersion = EOS_LOBBY_CREATELOBBYSEARCH_API_LATEST;
	CreateSearchOptions.MaxResults = MaxNumResults;

	EOS_HLobbySearch LobbySearch = nullptr;
	EOS_EResult Result = EOS_Lobby_CreateLobbySearch(LobbyHandle, &CreateSearchOptions, &LobbySearch);
	if (Result != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"Lobbies: could not create search. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
		return;
	}

	CurrentSearch.SetNewSearch(LobbySearch);

	EOS_LobbySearch_SetParameterOptions ParamOptions = {};
	ParamOptions.ApiVersion = EOS_LOBBYSEARCH_SETPARAMETER_API_LATEST;
	ParamOptions.ComparisonOp = EOS_EComparisonOp::EOS_CO_EQUAL;

	EOS_Lobby_AttributeData AttrData;
	AttrData.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
	ParamOptions.Parameter = &AttrData;

	for (const FLobbyAttribute& NextAttr : SearchAttributes)
	{
		AttrData.Key = NextAttr.Key.c_str();

		switch (NextAttr.ValueType)
		{
		case FLobbyAttribute::Bool:
			AttrData.ValueType = EOS_ELobbyAttributeType::EOS_AT_BOOLEAN;
			AttrData.Value.AsBool = NextAttr.AsBool;
			break;
		case FLobbyAttribute::Int64:
			AttrData.ValueType = EOS_ELobbyAttributeType::EOS_AT_INT64;
			AttrData.Value.AsInt64 = NextAttr.AsInt64;
			break;
		case FLobbyAttribute::Double:
			AttrData.ValueType = EOS_ELobbyAttributeType::EOS_AT_DOUBLE;
			AttrData.Value.AsDouble = NextAttr.AsDouble;
			break;
		case FLobbyAttribute::String:
			AttrData.ValueType = EOS_ELobbyAttributeType::EOS_AT_STRING;
			//Do not use attributes with empty strings
			if (NextAttr.AsString.empty())
			{
				continue;
			}
			AttrData.Value.AsUtf8 = NextAttr.AsString.c_str();
			break;
		}

		Result = EOS_LobbySearch_SetParameter(LobbySearch, &ParamOptions);
		if (Result != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Lobbies: failed to update lobby search with parameter. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
			return;
		}
	}

	EOS_LobbySearch_FindOptions FindOptions = {};
	FindOptions.ApiVersion = EOS_LOBBYSEARCH_FIND_API_LATEST;
	FindOptions.LocalUserId = CurrentUserProductId;
	EOS_LobbySearch_Find(LobbySearch, &FindOptions, nullptr, OnLobbySearchFinished);
}

void FLobbies::Search(const std::string& LobbyId, uint32_t MaxNumResults)
{
	//Clear previous search
	CurrentSearch.Release();

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(FPlatform::GetPlatformHandle());
	
	if (!CurrentUserProductId.IsValid())
	{
		FDebugLog::LogError(L"Lobbies - Search: Current player is invalid!");
		return;
	}

	EOS_Lobby_CreateLobbySearchOptions CreateSearchOptions = {};
	CreateSearchOptions.ApiVersion = EOS_LOBBY_CREATELOBBYSEARCH_API_LATEST;
	CreateSearchOptions.MaxResults = MaxNumResults;

	EOS_HLobbySearch LobbySearch = nullptr;
	EOS_EResult Result = EOS_Lobby_CreateLobbySearch(LobbyHandle, &CreateSearchOptions, &LobbySearch);
	if (Result != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"Lobbies: could not create search. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
		return;
	}

	CurrentSearch.SetNewSearch(LobbySearch);

	EOS_LobbySearch_SetLobbyIdOptions SetLobbyOptions = {};
	SetLobbyOptions.ApiVersion = EOS_LOBBYSEARCH_SETLOBBYID_API_LATEST;
	SetLobbyOptions.LobbyId = LobbyId.c_str();

	Result = EOS_LobbySearch_SetLobbyId(LobbySearch, &SetLobbyOptions);
	if (Result != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"Lobbies: could not set lobby id for search. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
		return;
	}

	EOS_LobbySearch_FindOptions FindOptions = {};
	FindOptions.ApiVersion = EOS_LOBBYSEARCH_FIND_API_LATEST;
	FindOptions.LocalUserId = CurrentUserProductId;
	EOS_LobbySearch_Find(LobbySearch, &FindOptions, nullptr, OnLobbySearchFinished);
}

void FLobbies::SearchLobbyByLevel(const std::string& LevelName)
{
	std::vector<FLobbyAttribute> Attributes;

	FLobbyAttribute LevelAttribute;
	LevelAttribute.Key = "LEVEL";
	LevelAttribute.ValueType = FLobbyAttribute::String;
	LevelAttribute.AsString = LevelName;
	LevelAttribute.Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC;

	Attributes.push_back(LevelAttribute);

	Search(Attributes);
}

void FLobbies::SearchLobbyByBucketId(const std::string& BucketId)
{
	std::vector<FLobbyAttribute> Attributes;

	FLobbyAttribute BucketIdAttribute;
	BucketIdAttribute.Key = EOS_LOBBY_SEARCH_BUCKET_ID;
	BucketIdAttribute.ValueType = FLobbyAttribute::String;
	BucketIdAttribute.AsString = BucketId;
	BucketIdAttribute.Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC;

	Attributes.push_back(BucketIdAttribute);

	Search(Attributes, 1);
}

void FLobbies::ClearSearch()
{
	CurrentSearch.Release();
}

void FLobbies::SubscribeToLobbyUpdates()
{
	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(FPlatform::GetPlatformHandle());

	EOS_Lobby_AddNotifyLobbyUpdateReceivedOptions UpdateNotifyOptions = {};
	UpdateNotifyOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYLOBBYUPDATERECEIVED_API_LATEST;

	LobbyUpdateNotification = EOS_Lobby_AddNotifyLobbyUpdateReceived(LobbyHandle, &UpdateNotifyOptions, nullptr, OnLobbyUpdateReceived);

	EOS_Lobby_AddNotifyLobbyMemberUpdateReceivedOptions MemberUpdateOptions = {};
	MemberUpdateOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYLOBBYMEMBERUPDATERECEIVED_API_LATEST;
	LobbyMemberUpdateNotification = EOS_Lobby_AddNotifyLobbyMemberUpdateReceived(LobbyHandle, &MemberUpdateOptions, nullptr, OnMemberUpdateReceived);

	EOS_Lobby_AddNotifyLobbyMemberStatusReceivedOptions MemberStatusOptions = {};
	MemberStatusOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYLOBBYMEMBERSTATUSRECEIVED_API_LATEST;
	LobbyMemberStatusNotification = EOS_Lobby_AddNotifyLobbyMemberStatusReceived(LobbyHandle, &MemberStatusOptions, nullptr, OnMemberStatusReceived);
}

void FLobbies::UnsubscribeFromLobbyUpdates()
{
	if (LobbyUpdateNotification != EOS_INVALID_NOTIFICATIONID)
	{
		EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(FPlatform::GetPlatformHandle());

		EOS_Lobby_RemoveNotifyLobbyUpdateReceived(LobbyHandle, LobbyUpdateNotification);
		LobbyUpdateNotification = EOS_INVALID_NOTIFICATIONID;
	}

	if (LobbyMemberUpdateNotification != EOS_INVALID_NOTIFICATIONID)
	{
		EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(FPlatform::GetPlatformHandle());

		EOS_Lobby_RemoveNotifyLobbyMemberUpdateReceived(LobbyHandle, LobbyMemberUpdateNotification);
		LobbyMemberUpdateNotification = EOS_INVALID_NOTIFICATIONID;
	}

	if (LobbyMemberStatusNotification != EOS_INVALID_NOTIFICATIONID)
	{
		EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(FPlatform::GetPlatformHandle());

		EOS_Lobby_RemoveNotifyLobbyMemberStatusReceived(LobbyHandle, LobbyMemberStatusNotification);
		LobbyMemberStatusNotification = EOS_INVALID_NOTIFICATIONID;
	}
}

void FLobbies::SubscribeToLobbyInvites()
{
	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(FPlatform::GetPlatformHandle());

	EOS_Lobby_AddNotifyLobbyInviteReceivedOptions InviteOptions = {};
	InviteOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYLOBBYINVITERECEIVED_API_LATEST;
	LobbyInviteNotification = EOS_Lobby_AddNotifyLobbyInviteReceived(LobbyHandle, &InviteOptions, nullptr, OnLobbyInviteReceived);

	EOS_Lobby_AddNotifyLobbyInviteAcceptedOptions AcceptedOptions = {};
	AcceptedOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYLOBBYINVITEACCEPTED_API_LATEST;
	LobbyInviteAcceptedNotification = EOS_Lobby_AddNotifyLobbyInviteAccepted(LobbyHandle, &AcceptedOptions, nullptr, OnLobbyInviteAccepted);

	EOS_Lobby_AddNotifyJoinLobbyAcceptedOptions JoinGameOptions = {};
	JoinGameOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYJOINLOBBYACCEPTED_API_LATEST;
	JoinLobbyAcceptedNotification = EOS_Lobby_AddNotifyJoinLobbyAccepted(LobbyHandle, &JoinGameOptions, nullptr, OnJoinLobbyAccepted);
}

void FLobbies::UnsubscribeFromLobbyInvites()
{
	if (LobbyInviteNotification != EOS_INVALID_NOTIFICATIONID)
	{
		EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(FPlatform::GetPlatformHandle());

		EOS_Lobby_RemoveNotifyLobbyInviteReceived(LobbyHandle, LobbyInviteNotification);
		LobbyInviteNotification = EOS_INVALID_NOTIFICATIONID;

		EOS_Lobby_RemoveNotifyLobbyInviteAccepted(LobbyHandle, LobbyInviteAcceptedNotification);
		LobbyInviteAcceptedNotification = EOS_INVALID_NOTIFICATIONID;

		EOS_Lobby_RemoveNotifyJoinLobbyAccepted(LobbyHandle, JoinLobbyAcceptedNotification);
		JoinLobbyAcceptedNotification = EOS_INVALID_NOTIFICATIONID;
	}
}

void FLobbies::SubscribeToLeaveLobbyUI()
{
	EOS_HLobby LobbiesHandle = EOS_Platform_GetLobbyInterface(FPlatform::GetPlatformHandle());

	EOS_Lobby_AddNotifyLeaveLobbyRequestedOptions LeaveLobbyRequestedOptions = { };
	LeaveLobbyRequestedOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYLEAVELOBBYREQUESTED_API_LATEST;
	LeaveLobbyRequestedNotification = EOS_Lobby_AddNotifyLeaveLobbyRequested(LobbiesHandle, &LeaveLobbyRequestedOptions, nullptr, OnLeaveLobbyRequested);
}

void FLobbies::UnsubscribeFromLeaveLobbyUI()
{
	if (LeaveLobbyRequestedNotification != EOS_INVALID_NOTIFICATIONID)
	{
		EOS_Lobby_RemoveNotifyLeaveLobbyRequested(EOS_Platform_GetLobbyInterface(FPlatform::GetPlatformHandle()), LeaveLobbyRequestedNotification);
		LeaveLobbyRequestedNotification = EOS_INVALID_NOTIFICATIONID;
	}
}

std::string FLobbies::GetRTCRoomName()
{
	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(FPlatform::GetPlatformHandle());

	EOS_Lobby_GetRTCRoomNameOptions GetRTCRoomNameOptions = {0};
	GetRTCRoomNameOptions.ApiVersion = EOS_LOBBY_GETRTCROOMNAME_API_LATEST;
	GetRTCRoomNameOptions.LobbyId = CurrentLobby.Id.c_str();
	GetRTCRoomNameOptions.LocalUserId = CurrentUserProductId;

	char RoomNameBuffer[256];
	uint32_t InBufferSizeOutBytesWritten = sizeof(RoomNameBuffer) / sizeof(char);

	EOS_EResult GetRTCRoomNameResult = EOS_Lobby_GetRTCRoomName(LobbyHandle, &GetRTCRoomNameOptions, &RoomNameBuffer[0], &InBufferSizeOutBytesWritten);
	if (GetRTCRoomNameResult != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"Lobbies: could not get RTC room name. Error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(GetRTCRoomNameResult)).c_str());
		return std::string();
	}

	// Copy from our buffer, less the null-terminator
	std::string RoomName(&RoomNameBuffer[0], InBufferSizeOutBytesWritten - 1);

	FDebugLog::Log(L"Lobbies: Found RTC Room name for lobby. RoomName: %ls", FStringUtils::Widen(RoomName).c_str());

	return RoomName;
}

void FLobbies::SubscribeToRTCEvents()
{
	if (!CurrentLobby.IsRTCRoomEnabled())
	{
		FDebugLog::Log(L"Lobbies: RTC Room is disabled.");
		return;
	}

	CurrentLobby.RTCRoomName = GetRTCRoomName();

	// We don't have a RTC Room
	if (CurrentLobby.RTCRoomName.empty())
	{
		FDebugLog::LogError(L"Lobbies: Unable to bind to RTC Room Name, failing to bind delegates.");
		return;
	}

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(FPlatform::GetPlatformHandle());

	// Register for connection status changes
	EOS_Lobby_AddNotifyRTCRoomConnectionChangedOptions AddNotifyRTCRoomConnectionChangedOptions = {};
	AddNotifyRTCRoomConnectionChangedOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYRTCROOMCONNECTIONCHANGED_API_LATEST;
	CurrentLobby.RTCRoomConnectionChanged = EOS_Lobby_AddNotifyRTCRoomConnectionChanged(LobbyHandle, &AddNotifyRTCRoomConnectionChangedOptions, nullptr, OnRTCRoomConnectionChangeReceived);
	if (CurrentLobby.RTCRoomConnectionChanged == EOS_INVALID_NOTIFICATIONID)
	{
		FDebugLog::LogError(L"Lobbies: Failed to bind to Lobby NotifyRTCRoomConnectionChanged notification.");
	}

	// Get the current room connection status now that we're listening for changes
	EOS_Lobby_IsRTCRoomConnectedOptions IsRTCRoomConnectedOptions = {};
	IsRTCRoomConnectedOptions.ApiVersion = EOS_LOBBY_ISRTCROOMCONNECTED_API_LATEST;
	IsRTCRoomConnectedOptions.LobbyId = CurrentLobby.Id.c_str();
	IsRTCRoomConnectedOptions.LocalUserId = CurrentUserProductId;
	EOS_Bool bIsConnected = EOS_FALSE;
	EOS_EResult IsRoomConnectedResult = EOS_Lobby_IsRTCRoomConnected(LobbyHandle, &IsRTCRoomConnectedOptions, &bIsConnected);
	if (IsRoomConnectedResult != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"Lobbies: Failed to get RTC Room connection status: error code: %ls.", FStringUtils::Widen(EOS_EResult_ToString(IsRoomConnectedResult)).c_str());
	}
	CurrentLobby.bRTCRoomConnected = bIsConnected != EOS_FALSE;

	EOS_HRTC RTCHandle = EOS_Platform_GetRTCInterface(FPlatform::GetPlatformHandle());
	EOS_HRTCAudio RTCAudioHandle = EOS_RTC_GetAudioInterface(RTCHandle);
	EOS_HRTCData RTCDataHandle = EOS_RTC_GetDataInterface(RTCHandle);

	// Register for RTC Room participant changes
	EOS_RTC_AddNotifyParticipantStatusChangedOptions AddNotifyParticipantStatusChangedOptions = {};
	AddNotifyParticipantStatusChangedOptions.ApiVersion = EOS_RTC_ADDNOTIFYPARTICIPANTSTATUSCHANGED_API_LATEST;
	AddNotifyParticipantStatusChangedOptions.LocalUserId = CurrentUserProductId;
	AddNotifyParticipantStatusChangedOptions.RoomName = CurrentLobby.RTCRoomName.c_str();
	CurrentLobby.RTCRoomParticipantUpdate = EOS_RTC_AddNotifyParticipantStatusChanged(RTCHandle, &AddNotifyParticipantStatusChangedOptions, nullptr, OnRTCRoomParticipantStatusChanged);
	if (CurrentLobby.RTCRoomParticipantUpdate == EOS_INVALID_NOTIFICATIONID)
	{
		FDebugLog::LogError(L"Lobbies: Failed to bind to RTC AddNotifyParticipantStatusChanged notification.");
	}

	// Register for talking changes
	EOS_RTCAudio_AddNotifyParticipantUpdatedOptions AddNotifyParticipantUpdatedOptions = {};
	AddNotifyParticipantUpdatedOptions.ApiVersion = EOS_RTCAUDIO_ADDNOTIFYPARTICIPANTUPDATED_API_LATEST;
	AddNotifyParticipantUpdatedOptions.LocalUserId = CurrentUserProductId;
	AddNotifyParticipantUpdatedOptions.RoomName = CurrentLobby.RTCRoomName.c_str();
	CurrentLobby.RTCRoomParticipantAudioUpdate = EOS_RTCAudio_AddNotifyParticipantUpdated(RTCAudioHandle, &AddNotifyParticipantUpdatedOptions, nullptr, OnRTCRoomParticipantAudioUpdateRecieved);
	if (CurrentLobby.RTCRoomParticipantAudioUpdate == EOS_INVALID_NOTIFICATIONID)
	{
		FDebugLog::LogError(L"Lobbies: Failed to bind to RTCAudio AddNotifyParticipantUpdated notification.");
	}

	EOS_RTCData_AddNotifyDataReceivedOptions AddNotifyDataReceivedOptions = {};
	AddNotifyDataReceivedOptions.ApiVersion = EOS_RTCDATA_ADDNOTIFYDATARECEIVED_API_LATEST;
	AddNotifyDataReceivedOptions.LocalUserId = CurrentUserProductId;
	AddNotifyDataReceivedOptions.RoomName = CurrentLobby.RTCRoomName.c_str();
	CurrentLobby.RTCRoomDataReceived = EOS_RTCData_AddNotifyDataReceived(RTCDataHandle, &AddNotifyDataReceivedOptions, nullptr, OnRTCRoomDataReceived);
}

void FLobbies::UnsubscribeFromRTCEvents()
{
	if (!CurrentLobby.IsValid())
	{
		return;
	}

	EOS_HRTC RTCHandle = EOS_Platform_GetRTCInterface(FPlatform::GetPlatformHandle());
	EOS_HRTCAudio RTCAudioHandle = EOS_RTC_GetAudioInterface(RTCHandle);
	if (CurrentLobby.RTCRoomParticipantAudioUpdate != EOS_INVALID_NOTIFICATIONID)
	{
		EOS_RTCAudio_RemoveNotifyParticipantUpdated(RTCAudioHandle, CurrentLobby.RTCRoomParticipantAudioUpdate);
		CurrentLobby.RTCRoomParticipantAudioUpdate = EOS_INVALID_NOTIFICATIONID;
	}

	if (CurrentLobby.RTCRoomParticipantUpdate != EOS_INVALID_NOTIFICATIONID)
	{
		EOS_RTC_RemoveNotifyParticipantStatusChanged(RTCHandle, CurrentLobby.RTCRoomParticipantUpdate);
		CurrentLobby.RTCRoomParticipantUpdate = EOS_INVALID_NOTIFICATIONID;
	}

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(FPlatform::GetPlatformHandle());
	if (CurrentLobby.RTCRoomConnectionChanged != EOS_INVALID_NOTIFICATIONID)
	{
		EOS_Lobby_RemoveNotifyRTCRoomConnectionChanged(LobbyHandle, CurrentLobby.RTCRoomConnectionChanged);
		CurrentLobby.RTCRoomConnectionChanged = EOS_INVALID_NOTIFICATIONID;
	}

	if (CurrentLobby.RTCRoomDataReceived != EOS_INVALID_NOTIFICATIONID)
	{
		EOS_HRTCData RTCDataHandle = EOS_RTC_GetDataInterface(RTCHandle);

		EOS_RTCData_RemoveNotifyDataReceived(RTCDataHandle, CurrentLobby.RTCRoomDataReceived);
		CurrentLobby.RTCRoomDataReceived = EOS_INVALID_NOTIFICATIONID;
	}

	CurrentLobby.RTCRoomName.clear();
}

void FLobbies::OnLobbyCreated(EOS_LobbyId Id)
{
	if (Id && CurrentLobby.bBeingCreated)
	{
		CurrentLobby.Id = Id;
		ModifyLobby(CurrentLobby);
		if (CurrentLobby.IsRTCRoomEnabled())
		{
			SubscribeToRTCEvents();
		}
		bDirty = true;
	}
}

void FLobbies::OnLobbyUpdated(EOS_LobbyId Id)
{
	if (Id && CurrentLobby.Id == Id)
	{
		CurrentLobby.InitFromLobbyHandle(Id);
		SetInitialMemberAttribute();
		bDirty = true;
	}
}

void FLobbies::OnLobbyUpdate(EOS_LobbyId Id)
{
	if (Id && CurrentLobby.Id == Id)
	{
		CurrentLobby.InitFromLobbyHandle(Id);
		SetInitialMemberAttribute();
		bDirty = true;
	}
}

void FLobbies::OnLobbyJoined(EOS_LobbyId Id)
{
	if (CurrentLobby.IsValid() && CurrentLobby.Id != Id)
	{
		LeaveLobby();
	}

	CurrentLobby.InitFromLobbyHandle(Id);

	SetInitialMemberAttribute();

	if (CurrentLobby.IsRTCRoomEnabled())
	{
		SubscribeToRTCEvents();
	}

	bDirty = true;

	//Clear search
	if (CurrentSearch.IsValid())
	{
		CurrentSearch.Release();
	}

	PopLobbyInvite();

	FGameEvent JoinEvent(EGameEventType::LobbyJoined);
	FGame::Get().OnGameEvent(JoinEvent);
}

void FLobbies::OnLobbyJoinFailed(EOS_LobbyId Id)
{
	bDirty = true;

	PopLobbyInvite();
}

void FLobbies::OnLobbyInvite(const char* InviteId, FProductUserId SenderId)
{
	FLobbyInvite NewLobbyInvite;

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(FPlatform::GetPlatformHandle());

	EOS_Lobby_CopyLobbyDetailsHandleByInviteIdOptions Options = {};
	Options.ApiVersion = EOS_LOBBY_COPYLOBBYDETAILSHANDLEBYINVITEID_API_LATEST;
	Options.InviteId = InviteId;
	EOS_HLobbyDetails DetailsHandle = nullptr;
	EOS_EResult Result = EOS_Lobby_CopyLobbyDetailsHandleByInviteId(LobbyHandle, &Options, &DetailsHandle);
	if (Result != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"Lobbies (OnLobbyInvite) could not get lobby details: error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
		return;
	}
	if (DetailsHandle == nullptr)
	{
		FDebugLog::LogError(L"Lobbies (OnLobbyInvite) could not get lobby details: null details handle.");
		return;
	}

	NewLobbyInvite.LobbyInfo = MakeLobbyDetailsKeeper(DetailsHandle);
	NewLobbyInvite.Lobby.InitFromLobbyDetails(DetailsHandle);
	NewLobbyInvite.InviteId.assign(InviteId);

	NewLobbyInvite.FriendId = SenderId;
	NewLobbyInvite.FriendEpicId = FEpicAccountId();
	NewLobbyInvite.FriendDisplayName.clear();

	auto NewLobbyInviteIter = Invites.find(SenderId);
	if (NewLobbyInviteIter != Invites.end())
	{
		//Replace existing invite
		NewLobbyInviteIter->second.Lobby = NewLobbyInvite.Lobby;
		NewLobbyInviteIter->second.LobbyInfo = NewLobbyInvite.LobbyInfo;

		//Update dialog data in case this invite is being shown
		if (CurrentInvite == &NewLobbyInviteIter->second)
		{
			FGameEvent GameEvent(EGameEventType::LobbyInviteReceived);
			FGame::Get().OnGameEvent(GameEvent);
		}
	}
	else
	{
		//Add new invite
		Invites[SenderId] = NewLobbyInvite;
	}

	if (!CurrentInvite)
	{
		PopLobbyInvite();
	}

	bDirty = true;
}

void FLobbies::OnLobbyInviteAccepted(const char* InviteId, FProductUserId SenderId)
{
	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(FPlatform::GetPlatformHandle());

	EOS_Lobby_CopyLobbyDetailsHandleByInviteIdOptions Options = {};
	Options.ApiVersion = EOS_LOBBY_COPYLOBBYDETAILSHANDLEBYINVITEID_API_LATEST;
	Options.InviteId = InviteId;
	EOS_HLobbyDetails DetailsHandle = nullptr;
	EOS_EResult Result = EOS_Lobby_CopyLobbyDetailsHandleByInviteId(LobbyHandle, &Options, &DetailsHandle);
	if (Result != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"Lobbies (OnLobbyInviteAccepted) could not get lobby details: error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
		return;
	}
	if (DetailsHandle == nullptr)
	{
		FDebugLog::LogError(L"Lobbies (OnLobbyInviteAccepted) could not get lobby details: null details handle.");
		return;
	}

	LobbyDetailsKeeper LobbyInfo = MakeLobbyDetailsKeeper(DetailsHandle);
	FLobby Lobby; 
	Lobby.InitFromLobbyDetails(DetailsHandle);

	const bool bPresenceEnabled = true;
	JoinLobby(Lobby.Id.c_str(), LobbyInfo, bPresenceEnabled);
}

void FLobbies::OnJoinLobbyAccepted(FProductUserId LocalUserId, EOS_UI_EventId UiEventId)
{
	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(FPlatform::GetPlatformHandle());

	EOS_Lobby_CopyLobbyDetailsHandleByUiEventIdOptions CopyOptions = {};
	CopyOptions.ApiVersion = EOS_LOBBY_COPYLOBBYDETAILSHANDLEBYUIEVENTID_API_LATEST;
	CopyOptions.UiEventId = UiEventId;

	EOS_HLobbyDetails DetailsHandle;
	EOS_EResult Result = EOS_Lobby_CopyLobbyDetailsHandleByUiEventId(LobbyHandle, &CopyOptions, &DetailsHandle);

	if (Result != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"Lobbies (OnJoinLobbyAccepted) could not get lobby details: error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
		return;
	}
	if (DetailsHandle == nullptr)
	{
		FDebugLog::LogError(L"Lobbies (OnJoinLobbyAccepted) could not get lobby details: null details handle.");
		return;
	}

	LobbyDetailsKeeper LobbyInfo = MakeLobbyDetailsKeeper(DetailsHandle);
	FLobby Lobby; 
	Lobby.InitFromLobbyDetails(DetailsHandle);

	const bool bPresenceEnabled = true;
	JoinLobby(Lobby.Id.c_str(), LobbyInfo, bPresenceEnabled);
}

void FLobbies::OnSearchResultsReceived()
{
	EOS_LobbySearch_GetSearchResultCountOptions SearchResultOptions = {};
	SearchResultOptions.ApiVersion = EOS_LOBBYSEARCH_GETSEARCHRESULTCOUNT_API_LATEST;
	uint32_t NumSearchResults = EOS_LobbySearch_GetSearchResultCount(CurrentSearch.GetSearchHandle(), &SearchResultOptions);

	std::vector<FLobby> SearchResults;
	std::vector<LobbyDetailsKeeper> ResultHandles;

	EOS_LobbySearch_CopySearchResultByIndexOptions IndexOptions = {};
	IndexOptions.ApiVersion = EOS_LOBBYSEARCH_COPYSEARCHRESULTBYINDEX_API_LATEST;
	for (uint32_t SearchIndex = 0; SearchIndex < NumSearchResults; ++SearchIndex)
	{
		FLobby NextLobby;

		EOS_HLobbyDetails NextLobbyDetails = nullptr;
		IndexOptions.LobbyIndex = SearchIndex;
		EOS_EResult Result = EOS_LobbySearch_CopySearchResultByIndex(CurrentSearch.GetSearchHandle(), &IndexOptions, &NextLobbyDetails);
		if (Result == EOS_EResult::EOS_Success && NextLobbyDetails)
		{
			NextLobby.InitFromLobbyDetails(NextLobbyDetails);

			NextLobby.bSearchResult = true;
			SearchResults.push_back(NextLobby);
			ResultHandles.push_back(MakeLobbyDetailsKeeper(NextLobbyDetails));
		}
	}

	CurrentSearch.OnSearchResultsReceived(std::move(SearchResults), std::move(ResultHandles));
	bDirty = true;
}

void FLobbies::OnKickedFromLobby(EOS_LobbyId Id)
{
	if (CurrentLobby.IsValid() && CurrentLobby.Id == Id)
	{
		CurrentLobby.Clear();
		bDirty = true;
	}
}

void FLobbies::OnLobbyLeftOrDestroyed(EOS_LobbyId Id)
{
	bLobbyLeaveInProgress = false;
	if (ActiveJoin.IsValid())
	{
		FLobbyJoinRequest CurrentJoin(std::move(ActiveJoin));
		ActiveJoin.Clear();
		JoinLobby(CurrentJoin.Id, CurrentJoin.LobbyInfo, CurrentJoin.bPresenceEnabled);
	}
}

void FLobbies::OnHardMuteMemberFinished(EOS_LobbyId Id, FProductUserId ParticipantId, EOS_EResult Result)
{
	if (!CurrentLobby.IsValid() || CurrentLobby.Id != Id)
	{
		return;
	}

	if (FLobbyMember* LobbyMember = CurrentLobby.GetMemberByProductUserId(ParticipantId))
	{
		LobbyMember->RTCState.bHardMuteActionInProgress = false;
			
		if (Result == EOS_EResult::EOS_Success)
		{
			FDebugLog::Log(L"Lobbies (OnHardMuteMemberFinished): success.");
			LobbyMember->RTCState.bIsHardMuted = !LobbyMember->RTCState.bIsHardMuted; // we only support toggle functionality in this sample so we know the new state
		}
		else
		{
			FDebugLog::LogError(L"Lobbies (OnHardMuteMemberFinished): error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Result)).c_str());
		}
	}
}

void FLobbies::OnRTCRoomConnectionChanged(EOS_LobbyId Id, FProductUserId LocalUserId, bool bIsConnected)
{
	// Ensure this update is for our room
	if (!CurrentLobby.IsValid() || CurrentLobby.Id != Id)
	{
		return;
	}

	if (CurrentUserProductId != LocalUserId)
	{
		return;
	}

	CurrentLobby.bRTCRoomConnected = bIsConnected;

	if (FLobbyMember* LobbyMember = CurrentLobby.GetMemberByProductUserId(LocalUserId))
	{
		LobbyMember->RTCState.bIsInRTCRoom = bIsConnected;
		if (!bIsConnected)
		{
			LobbyMember->RTCState.bIsTalking = false;
		}
	}

	bDirty = true;
}

void FLobbies::OnRTCRoomParticipantJoined(const char* RoomName, FProductUserId ParticipantId)
{
	// Ensure this update is for our room
	if (CurrentLobby.RTCRoomName.empty() || CurrentLobby.RTCRoomName != RoomName)
	{
		return;
	}

	// Find this participant in our list
	if (FLobbyMember* LobbyMember = CurrentLobby.GetMemberByProductUserId(ParticipantId))
	{
		// Update in-room status
		LobbyMember->RTCState.bIsInRTCRoom = true;

		bDirty = true;
	}

	// Send update of skin color to new participant
	if (FLobbyMember* LocalLobbyMember = CurrentLobby.GetMemberByProductUserId(CurrentUserProductId))
	{
		SendSkinColorUpdate(LocalLobbyMember->CurrentColor);
	}
}

void FLobbies::OnRTCRoomParticipantLeft(const char* RoomName, FProductUserId ParticipantId)
{
	// Ensure this update is for our room
	if (CurrentLobby.RTCRoomName.empty() || CurrentLobby.RTCRoomName != RoomName)
	{
		return;
	}

	// Find this participant in our list
	if (FLobbyMember* LobbyMember = CurrentLobby.GetMemberByProductUserId(ParticipantId))
	{
		// Update in-room status
		LobbyMember->RTCState.bIsInRTCRoom = false;

		// Additionally clear their talking status when they leave, just to be safe
		LobbyMember->RTCState.bIsTalking = false;

		bDirty = true;
	}
}

void FLobbies::OnRTCRoomParticipantAudioUpdated(const char* RoomName, FProductUserId ParticipantId, bool bIsTalking, bool bIsAudioDisabled, bool bIsHardMuted)
{
	// Ensure this update is for our room
	if (CurrentLobby.RTCRoomName.empty() || CurrentLobby.RTCRoomName != RoomName)
	{
		return;
	}

	// Find this participant in our list
	if (FLobbyMember* LobbyMember = CurrentLobby.GetMemberByProductUserId(ParticipantId))
	{
		// Update talking status
		LobbyMember->RTCState.bIsTalking = bIsTalking;

		// Update the audio output status for other players
		if (LobbyMember->ProductId != CurrentUserProductId)
		{
			LobbyMember->RTCState.bIsAudioOutputDisabled = bIsAudioDisabled;
		}
		else // I could have been hard-muted by the lobby owner
		{
			LobbyMember->RTCState.bIsHardMuted = bIsHardMuted;
		}

		bDirty = true;
	}
}

void FLobbies::OnRTCRoomUpdateSendingComplete(const char* RoomName, FProductUserId ParticipantId, EOS_ERTCAudioStatus NewAudioStatus)
{
	// Ensure this update is for our room
	if (CurrentLobby.RTCRoomName.empty() || CurrentLobby.RTCRoomName != RoomName)
	{
		return;
	}

	// Ensure this update is for us
	if (CurrentUserProductId != ParticipantId)
	{
		return;
	}

	// Update our mute status
	if (FLobbyMember* LobbyMember = CurrentLobby.GetMemberByProductUserId(ParticipantId))
	{
		LobbyMember->RTCState.bIsAudioOutputDisabled = NewAudioStatus == EOS_ERTCAudioStatus::EOS_RTCAS_Disabled;
		LobbyMember->RTCState.bMuteActionInProgress = false;
		bDirty = true;
	}
}

void FLobbies::OnRTCRoomUpdateReceivingComplete(const char* RoomName, FProductUserId ParticipantId, bool bIsMuted)
{
	// Ensure this update is for our room
	if (CurrentLobby.RTCRoomName.empty() || CurrentLobby.RTCRoomName != RoomName)
	{
		return;
	}

	if (FLobbyMember* LobbyMember = CurrentLobby.GetMemberByProductUserId(ParticipantId))
	{
		LobbyMember->RTCState.bIsLocallyMuted = bIsMuted;
		LobbyMember->RTCState.bMuteActionInProgress = false;
		bDirty = true;
	}
}

void FLobbies::OnRTCRoomDataReceived(const char* RoomName, FProductUserId ParticipantId, const void* Data, uint32_t DataLengthBytes)
{
	// Ensure this update is for our room
	if (CurrentLobby.RTCRoomName.empty() || CurrentLobby.RTCRoomName != RoomName)
	{
		return;
	}

	if (DataLengthBytes < sizeof(ELobbyRTCDataCommand))
	{
		FDebugLog::LogError(L"Lobbies (OnRTCRoomDataReceived): wrong command format");
		return;
	}

	ELobbyRTCDataCommand Command = *(static_cast<const ELobbyRTCDataCommand*>(Data));
	const void* Payload = static_cast<const uint8_t*>(Data) + sizeof(ELobbyRTCDataCommand);

	switch (Command)
	{
	case ELobbyRTCDataCommand::SkinColor:
		{
			const size_t SkinColorCommandLength = sizeof(ELobbyRTCDataCommand) + sizeof(FLobbyMember::SkinColor);
			if (DataLengthBytes >= SkinColorCommandLength)
			{
				uint8_t SkinColorInt = *(static_cast<const uint8_t*>(Payload));
				if (static_cast<uint8_t>(FLobbyMember::SkinColor::White) <= SkinColorInt && SkinColorInt < static_cast<uint8_t>(FLobbyMember::SkinColor::Count))
				{
					OnSkinColorChanged(ParticipantId, *(static_cast<const FLobbyMember::SkinColor*>(Payload)));
				}
				else
				{
					FDebugLog::LogError(L"Lobbies (OnRTCRoomDataReceived): wrong skin color value");
				}
			}
			else
			{
				FDebugLog::LogError(L"Lobbies (OnRTCRoomDataReceived): wrong command format");
			}
		}
		break;
	default:
		FDebugLog::Log(L"Lobbies (OnRTCRoomDataReceived): unexpected command %d", Command);
	}
}

void FLobbies::OnSkinColorChanged(FProductUserId ParticipantId, const FLobbyMember::SkinColor InColor)
{
	// Find this participant in our list
	if (FLobbyMember* LobbyMember = CurrentLobby.GetMemberByProductUserId(ParticipantId))
	{
		// Update skin color of participant
		LobbyMember->CurrentColor = InColor;

		bDirty = true;
	}
}

void EOS_CALL FLobbies::OnCreateLobbyFinished(const EOS_Lobby_CreateLobbyCallbackInfo* Data)
{
	if (Data)
	{
		if (!EOS_EResult_IsOperationComplete(Data->ResultCode))
		{
			FDebugLog::Log(L"Lobbies (OnCreateLobbyFinished): operation not complete: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Lobbies (OnCreateLobbyFinished): error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else
		{
			FDebugLog::Log(L"Lobbies (OnCreateLobbyFinished): lobby created.");

			FGame::Get().GetLobbies()->OnLobbyCreated(Data->LobbyId);
		}
	}
	else
	{
		FDebugLog::LogError(L"Lobbies (OnCreateLobbyFinished): EOS_Lobby_CreateLobbyCallbackInfo is null");
	}
}

void EOS_CALL FLobbies::OnDestroyLobbyFinished(const EOS_Lobby_DestroyLobbyCallbackInfo* Data)
{
	if (Data)
	{
		if (!EOS_EResult_IsOperationComplete(Data->ResultCode))
		{
			FDebugLog::Log(L"Lobbies (OnDestroyLobbyFinished): operation not complete: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Lobbies (OnDestroyLobbyFinished): error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else
		{
			FDebugLog::Log(L"Lobbies (OnDestroyLobbyFinished): lobby destroyed.");
			FGame::Get().GetLobbies()->OnLobbyLeftOrDestroyed(Data->LobbyId);
		}
	}
	else
	{
		FDebugLog::LogError(L"Lobbies (OnDestroyLobbyFinished): EOS_Lobby_DestroyLobbyCallbackInfo is null");
	}
}

void EOS_CALL FLobbies::OnLobbyUpdateFinished(const EOS_Lobby_UpdateLobbyCallbackInfo* Data)
{
	if (Data)
	{
		if (!EOS_EResult_IsOperationComplete(Data->ResultCode))
		{
			FDebugLog::Log(L"Lobbies (OnLobbyUpdateFinished): operation not complete: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Lobbies (OnLobbyUpdateFinished): error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else
		{
			FDebugLog::Log(L"Lobbies (OnLobbyUpdateFinished): lobby updated.");
			FGame::Get().GetLobbies()->OnLobbyUpdated(Data->LobbyId);
		}
	}
	else
	{
		FDebugLog::LogError(L"Lobbies (OnLobbyUpdateFinished): EOS_Lobby_UpdateLobbyCallbackInfo is null");
	}
}

void EOS_CALL FLobbies::OnLobbyUpdateReceived(const EOS_Lobby_LobbyUpdateReceivedCallbackInfo* Data)
{
	if (Data)
	{
		FDebugLog::Log(L"Lobbies (OnLobbyUpdateReceived): lobby update received.");
		FGame::Get().GetLobbies()->OnLobbyUpdate(Data->LobbyId);
	}
	else
	{
		FDebugLog::LogError(L"Lobbies (OnLobbyUpdateReceived): EOS_Lobby_LobbyUpdateReceivedCallbackInfo is null");
	}
}

void EOS_CALL FLobbies::OnMemberUpdateReceived(const EOS_Lobby_LobbyMemberUpdateReceivedCallbackInfo* Data)
{
	if (Data)
	{
		FDebugLog::Log(L"Lobbies (OnMemberUpdateReceived): member update received.");
		//Simply update the whole lobby
		FGame::Get().GetLobbies()->OnLobbyUpdate(Data->LobbyId);
	}
	else
	{
		FDebugLog::LogError(L"Lobbies (OnMemberUpdateReceived): EOS_Lobby_LobbyMemberUpdateReceivedCallbackInfo is null");
	}
}

void EOS_CALL FLobbies::OnMemberStatusReceived(const EOS_Lobby_LobbyMemberStatusReceivedCallbackInfo* Data)
{
	if (Data)
	{
		FDebugLog::Log(L"Lobbies (OnMemberStatusReceived): member status update received.");


		PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
		if (Player == nullptr)
		{
			FDebugLog::LogError(L"Lobbies - OnMemberStatusReceived: Current player is invalid!");
			return;
		}
		
		if (!Player->GetProductUserID())
		{
			FDebugLog::LogError(L"Lobbies - OnMemberStatusReceived: Current player is invalid!");

			//Simply update the whole lobby
			FGame::Get().GetLobbies()->OnLobbyUpdate(Data->LobbyId);

			return;
		}

		bool bUpdateLobby = true;
		//Current player updates need special handling
		if (Data->TargetUserId == Player->GetProductUserID())
		{
			if (Data->CurrentStatus == EOS_ELobbyMemberStatus::EOS_LMS_CLOSED ||
				Data->CurrentStatus == EOS_ELobbyMemberStatus::EOS_LMS_KICKED ||
				Data->CurrentStatus == EOS_ELobbyMemberStatus::EOS_LMS_DISCONNECTED)
			{
				FGame::Get().GetLobbies()->OnKickedFromLobby(Data->LobbyId);
				bUpdateLobby = false;
			}
		}

		if (bUpdateLobby)
		{
			//Simply update the whole lobby
			FGame::Get().GetLobbies()->OnLobbyUpdate(Data->LobbyId);
		}
	}
	else
	{
		FDebugLog::LogError(L"Lobbies (OnMemberStatusReceived): EOS_Lobby_LobbyMemberStatusReceivedCallbackInfo is null");
	}
}

void EOS_CALL FLobbies::OnJoinLobbyFinished(const EOS_Lobby_JoinLobbyCallbackInfo* Data)
{
	if (Data)
	{
		if (!EOS_EResult_IsOperationComplete(Data->ResultCode))
		{
			FDebugLog::Log(L"Lobbies (OnJoinLobbyFinished): operation not complete: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Lobbies (OnJoinLobbyFinished): error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
			FGame::Get().GetLobbies()->OnLobbyJoinFailed(Data->LobbyId);
		}
		else
		{
			FDebugLog::Log(L"Lobbies (OnJoinLobbyFinished): lobby join finished.");
			FGame::Get().GetLobbies()->OnLobbyJoined(Data->LobbyId);
		}
	}
	else
	{
		FDebugLog::LogError(L"Lobbies (OnJoinLobbyFinished): EOS_Lobby_JoinLobbyCallbackInfo is null");
	}
}

void EOS_CALL FLobbies::OnLeaveLobbyFinished(const EOS_Lobby_LeaveLobbyCallbackInfo* Data)
{
	if (Data)
	{
		if (!EOS_EResult_IsOperationComplete(Data->ResultCode))
		{
			FDebugLog::Log(L"Lobbies (OnLeaveLobbyFinished): operation not complete: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Lobbies (OnLeaveLobbyFinished): error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else
		{
			FDebugLog::Log(L"Lobbies (OnLeaveLobbyFinished): lobby left.");
			FGame::Get().GetLobbies()->OnLobbyLeftOrDestroyed(Data->LobbyId);
		}
	}
	else
	{
		FDebugLog::LogError(L"Lobbies (OnLeaveLobbyFinished): EOS_Lobby_LobbyUpdateReceivedCallbackInfo is null");
	}
}

void EOS_CALL FLobbies::OnLobbyInviteSendFinished(const EOS_Lobby_SendInviteCallbackInfo* Data)
{
	if (Data)
	{
		if (!EOS_EResult_IsOperationComplete(Data->ResultCode))
		{
			FDebugLog::Log(L"Lobbies (OnLobbyInviteSendFinished): operation not complete: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Lobbies (OnLobbyInviteSendFinished): error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else
		{
			FDebugLog::Log(L"Lobbies (OnLobbyInviteSendFinished): invite sent.");
		}
	}
	else
	{
		FDebugLog::LogError(L"Lobbies (OnLobbyInviteSendFinished): EOS_Lobby_SendInviteCallbackInfo is null");
	}
}

void EOS_CALL FLobbies::OnLobbyInviteReceived(const EOS_Lobby_LobbyInviteReceivedCallbackInfo* Data)
{
	if (Data)
	{
		FDebugLog::Log(L"Lobbies (OnLobbyInviteReceived): invite received.");
		FGame::Get().GetLobbies()->OnLobbyInvite(Data->InviteId, Data->TargetUserId);
	}
	else
	{
		FDebugLog::LogError(L"Lobbies (OnLobbyInviteReceived): EOS_Lobby_LobbyInviteReceivedCallbackInfo is null");
	}
}

void EOS_CALL FLobbies::OnLobbyInviteAccepted(const EOS_Lobby_LobbyInviteAcceptedCallbackInfo* Data)
{
	if (Data)
	{
		FDebugLog::Log(L"Lobbies (OnLobbyInviteAccepted): invite received.");

		//Hide popup
		FGameEvent Event(EGameEventType::OverlayInviteToLobbyAccepted);
		FGame::Get().OnGameEvent(Event);

		FGame::Get().GetLobbies()->OnLobbyInviteAccepted(Data->InviteId, Data->TargetUserId);
	}
	else
	{
		FDebugLog::LogError(L"Lobbies (OnLobbyInviteAccepted): EOS_Lobby_LobbyInviteAcceptedCallbackInfo is null");
	}
}

void EOS_CALL FLobbies::OnJoinLobbyAccepted(const EOS_Lobby_JoinLobbyAcceptedCallbackInfo* Data)
{
	if (Data)
	{
		FDebugLog::Log(L"Lobbies (OnJoinLobbyAccepted): invite received.");

		FGame::Get().GetLobbies()->OnJoinLobbyAccepted(Data->LocalUserId, Data->UiEventId);
	}
	else
	{
		FDebugLog::LogError(L"Lobbies (OnJoinLobbyAccepted): EOS_Lobby_JoinLobbyAcceptedCallbackInfo is null");
	}
}

void EOS_CALL FLobbies::OnRejectInviteFinished(const EOS_Lobby_RejectInviteCallbackInfo* Data)
{
	if (Data)
	{
		if (!EOS_EResult_IsOperationComplete(Data->ResultCode))
		{
			FDebugLog::Log(L"Lobbies (OnRejectInviteFinished): operation not complete: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Lobbies (OnRejectInviteFinished): error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else
		{
			FDebugLog::Log(L"Lobbies (OnRejectInviteFinished): invite rejected.");
		}
	}
	else
	{
		FDebugLog::LogError(L"Lobbies (OnRejectInviteFinished): EOS_Lobby_RejectInviteCallbackInfo is null");
	}
}

void EOS_CALL FLobbies::OnLobbySearchFinished(const EOS_LobbySearch_FindCallbackInfo* Data)
{
	if (Data)
	{
		if (!EOS_EResult_IsOperationComplete(Data->ResultCode))
		{
			FDebugLog::Log(L"Lobbies (OnLobbySearchFinished): operation not complete: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Lobbies (OnLobbySearchFinished): error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else
		{
			FDebugLog::Log(L"Lobbies (OnLobbySearchFinished): search finished.");

			FGame::Get().GetLobbies()->OnSearchResultsReceived();
		}
	}
	else
	{
		FDebugLog::LogError(L"Lobbies (OnLobbySearchFinished): EOS_LobbySearch_FindCallbackInfo is null");
	}
}

void EOS_CALL FLobbies::OnKickMemberFinished(const EOS_Lobby_KickMemberCallbackInfo* Data)
{
	if (Data)
	{
		if (!EOS_EResult_IsOperationComplete(Data->ResultCode))
		{
			FDebugLog::Log(L"Lobbies (OnKickMemberFinished): operation not complete: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Lobbies (OnKickMemberFinished): error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else
		{
			FDebugLog::Log(L"Lobbies (OnKickMemberFinished): member kicked.");
		}
	}
	else
	{
		FDebugLog::LogError(L"Lobbies (OnKickMemberFinished): EOS_Lobby_KickMemberCallbackInfo is null");
	}
}

void EOS_CALL FLobbies::OnHardMuteMemberFinished(const EOS_Lobby_HardMuteMemberCallbackInfo* Data)
{
	if (Data)
	{
		if (!EOS_EResult_IsOperationComplete(Data->ResultCode))
		{
			FDebugLog::Log(L"Lobbies (OnHardMuteMemberFinished): operation not complete: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
			return;
		}

		FGame::Get().GetLobbies()->OnHardMuteMemberFinished(Data->LobbyId, Data->TargetUserId, Data->ResultCode);
	}
	else
	{
		FDebugLog::LogError(L"Lobbies (OnHardMuteMemberFinished): EOS_Lobby_HardMuteMemberCallbackInfo is null");
	}
}

void EOS_CALL FLobbies::OnPromoteMemberFinished(const EOS_Lobby_PromoteMemberCallbackInfo* Data)
{
	if (Data)
	{
		if (!EOS_EResult_IsOperationComplete(Data->ResultCode))
		{
			FDebugLog::Log(L"Lobbies (OnPromoteMemberFinished): operation not complete: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Lobbies (OnPromoteMemberFinished): error code: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else
		{
			FDebugLog::Log(L"Lobbies (OnPromoteMemberFinished): member promoted.");
		}
	}
	else
	{
		FDebugLog::LogError(L"Lobbies (OnPromoteMemberFinished): EOS_Lobby_PromoteMemberCallbackInfo is null");
	}
}

void EOS_CALL FLobbies::OnRTCRoomConnectionChangeReceived(const EOS_Lobby_RTCRoomConnectionChangedCallbackInfo* Data)
{
	if (Data)
	{
		const bool bIsConnected = Data->bIsConnected != EOS_FALSE;
		FDebugLog::Log(L"Lobbies (OnRTCRoomConnectionChangeReceived): connection status changed. LobbyId=[%ls] bIsConnected=[%d] DisconnectReason=[%ls]",
			FStringUtils::Widen(Data->LobbyId).c_str(),
			bIsConnected,
			FStringUtils::Widen(EOS_EResult_ToString(Data->DisconnectReason)).c_str());

		FGame::Get().GetLobbies()->OnRTCRoomConnectionChanged(Data->LobbyId, Data->LocalUserId, bIsConnected);
	}
	else
	{
		FDebugLog::LogError(L"Lobbies (OnRTCRoomConnectionChangeReceived): EOS_Lobby_RTCRoomConnectionChangedCallbackInfo is null");
	}
}

void EOS_CALL FLobbies::OnRTCRoomParticipantStatusChanged(const EOS_RTC_ParticipantStatusChangedCallbackInfo* Data)
{
	if (Data)
	{
		FDebugLog::Log(L"Lobbies (OnRTCRoomParticipantStatusChanged): participant updated.");

		const wchar_t* StatusText = Data->ParticipantStatus == EOS_ERTCParticipantStatus::EOS_RTCPS_Joined ? L"Joined" : L"Left";

		FDebugLog::Log(L"Lobbies (OnRTCRoomParticipantStatusChanged): participant updated. LocalUserId=[%ls] Room=[%ls] ParticipantUserId=[%ls] ParticipantStatus=[%ls] MetadataCount=[%u]",
			FStringUtils::Widen(FAccountHelpers::ProductUserIDToString(Data->LocalUserId)).c_str(),
			FStringUtils::Widen(Data->RoomName).c_str(),
			FStringUtils::Widen(FAccountHelpers::ProductUserIDToString(Data->ParticipantId)).c_str(),
			StatusText,
			Data->ParticipantMetadataCount);

		if (Data->ParticipantStatus == EOS_ERTCParticipantStatus::EOS_RTCPS_Joined)
		{
			FGame::Get().GetLobbies()->OnRTCRoomParticipantJoined(Data->RoomName, Data->ParticipantId);
		}
		else
		{
			FGame::Get().GetLobbies()->OnRTCRoomParticipantLeft(Data->RoomName, Data->ParticipantId);
		}
	}
	else
	{
		FDebugLog::LogError(L"Lobbies (OnRTCRoomParticipantStatusChanged): EOS_RTCAudio_ParticipantUpdatedCallbackInfo is null");
	}
}


void EOS_CALL FLobbies::OnRTCRoomParticipantAudioUpdateRecieved(const EOS_RTCAudio_ParticipantUpdatedCallbackInfo* Data)
{
	if (Data)
	{
		const bool bIsTalking = Data->bSpeaking != EOS_FALSE;
		const bool bIsAudioDisabled = Data->AudioStatus != EOS_ERTCAudioStatus::EOS_RTCAS_Enabled;
		const bool bIsHardMuted = Data->AudioStatus == EOS_ERTCAudioStatus::EOS_RTCAS_AdminDisabled;

		FDebugLog::Log(L"Lobbies (OnAudioParticipantUpdateRecieved): participant audio updated. LocalUserId=[%ls] Room=[%ls] ParticipantUserId=[%ls] bIsTalking=[%d] bIsAudioDisabled=[%d (%d)]",
			FStringUtils::Widen(FAccountHelpers::ProductUserIDToString(Data->LocalUserId)).c_str(),
			FStringUtils::Widen(Data->RoomName).c_str(),
			FStringUtils::Widen(FAccountHelpers::ProductUserIDToString(Data->ParticipantId)).c_str(),
			bIsTalking,
			bIsAudioDisabled,
			static_cast<int32_t>(Data->AudioStatus));

		FGame::Get().GetLobbies()->OnRTCRoomParticipantAudioUpdated(Data->RoomName, Data->ParticipantId, bIsTalking, bIsAudioDisabled, bIsHardMuted);
	}
	else
	{
		FDebugLog::LogError(L"Lobbies (OnAudioParticipantUpdateRecieved): EOS_RTCAudio_ParticipantUpdatedCallbackInfo is null");
	}
}

void EOS_CALL FLobbies::OnRTCRoomUpdateSendingComplete(const EOS_RTCAudio_UpdateSendingCallbackInfo* Data)
{
	if (Data)
	{
		if (!EOS_EResult_IsOperationComplete(Data->ResultCode))
		{
			FDebugLog::Log(L"Lobbies (OnRTCRoomUpdateSendingComplete): operation not complete: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else
		{
			if (Data->ResultCode != EOS_EResult::EOS_Success)
			{
				FDebugLog::LogError(L"Lobbies (OnRTCRoomUpdateSendingComplete): failed to update sending status");
			}
			else
			{
				const bool bIsAudioOutputDisabled = Data->AudioStatus != EOS_ERTCAudioStatus::EOS_RTCAS_Enabled;

				FDebugLog::Log(L"Lobbies (OnRTCRoomUpdateSendingComplete): Updated sending status successfully. LocalUserId=[%ls] Room=[%ls] bIsAudioOutputDisabled=[%d (%d)]",
					FStringUtils::Widen(FAccountHelpers::ProductUserIDToString(Data->LocalUserId)).c_str(),
					FStringUtils::Widen(Data->RoomName).c_str(),
					bIsAudioOutputDisabled,
					static_cast<int32_t>(Data->AudioStatus));
			}

			FGame::Get().GetLobbies()->OnRTCRoomUpdateSendingComplete(Data->RoomName, Data->LocalUserId, Data->AudioStatus);
		}
	}
	else
	{
		FDebugLog::LogError(L"Lobbies (OnRTCRoomUpdateSendingComplete): EOS_RTCAudio_UpdateSendingCallbackInfo is null");
	}
}

void EOS_CALL FLobbies::OnRTCRoomUpdateReceivingComplete(const EOS_RTCAudio_UpdateReceivingCallbackInfo* Data)
{
	if (Data)
	{
		if (!EOS_EResult_IsOperationComplete(Data->ResultCode))
		{
			FDebugLog::Log(L"Lobbies (OnRTCRoomUpdateReceivingComplete): operation not complete: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else
		{
			const bool bIsMuted = Data->bAudioEnabled == EOS_FALSE;
			if (Data->ResultCode != EOS_EResult::EOS_Success)
			{
				FDebugLog::LogError(L"Lobbies (OnRTCRoomUpdateReceivingComplete): failed to update receiving status");
			}
			else
			{
				FDebugLog::Log(L"Lobbies (OnRTCRoomUpdateReceivingComplete): Updated receiving status successfully. ParticipantId=[%ls] Room=[%ls] bIsMuted=[%d]",
					FStringUtils::Widen(FAccountHelpers::ProductUserIDToString(Data->ParticipantId)).c_str(),
					FStringUtils::Widen(Data->RoomName).c_str(),
					bIsMuted);
			}
			FGame::Get().GetLobbies()->OnRTCRoomUpdateReceivingComplete(Data->RoomName, Data->ParticipantId, bIsMuted);
		}
	}
	else
	{
		FDebugLog::LogError(L"Lobbies (OnRTCRoomUpdateReceivingComplete): EOS_RTCAudio_UpdateReceivingCallbackInfo is null");
	}
}

void EOS_CALL FLobbies::OnRTCRoomDataReceived(const EOS_RTCData_DataReceivedCallbackInfo* Data)
{
	if (Data)
	{
		FGame::Get().GetLobbies()->OnRTCRoomDataReceived(Data->RoomName, Data->ParticipantId, Data->Data, Data->DataLengthBytes);
	}
	else
	{
		FDebugLog::LogError(L"Lobbies (OnRTCRoomDataReceived): EOS_RTCData_DataReceivedCallbackInfo is null");
	}
}

void EOS_CALL FLobbies::OnLeaveLobbyRequested(const EOS_Lobby_LeaveLobbyRequestedCallbackInfo* Data)
{
	if (Data)
	{
		const std::string LeaveLobbyId = std::string(Data->LobbyId);
		const std::string CurrentLobbyId = FGame::Get().GetLobbies()->GetCurrentLobby().Id;
		if (LeaveLobbyId == CurrentLobbyId)
		{
			FDebugLog::Log(L"Lobbies: Leave lobby requested for LobbyId = %ls", FStringUtils::Widen(LeaveLobbyId).c_str());
			FGame::Get().GetLobbies()->LeaveLobby();
		}
		else
		{
			FDebugLog::LogError(L"Lobbies (OnLeaveLobbyRequested): Leave lobby requested on non active lobby. LeaveLobbyId = %ls, CurrentLobbyId = %ls", FStringUtils::Widen(LeaveLobbyId).c_str(), FStringUtils::Widen(CurrentLobbyId).c_str());
		}
	}
	else
	{
		FDebugLog::LogError(L"Lobbies (OnLeaveLobbyRequested): EOS_Lobby_LeaveLobbyRequestedCallbackInfo is null");
	}
}

void FLobbyAttribute::InitFromAttribute(EOS_Lobby_Attribute* Attr)
{
	if (!Attr)
	{
		return;
	}

	Visibility = Attr->Visibility;
	Key = Attr->Data->Key;
	switch (Attr->Data->ValueType)
	{
	case EOS_ELobbyAttributeType::EOS_AT_BOOLEAN:
		ValueType = FLobbyAttribute::Bool;
		AsBool = Attr->Data->Value.AsBool;
		break;
	case EOS_ELobbyAttributeType::EOS_AT_INT64:
		ValueType = FLobbyAttribute::Int64;
		AsInt64 = Attr->Data->Value.AsInt64;
		break;
	case EOS_ELobbyAttributeType::EOS_AT_DOUBLE:
		ValueType = FLobbyAttribute::Double;
		AsDouble = Attr->Data->Value.AsDouble;
		break;
	case EOS_ELobbyAttributeType::EOS_AT_STRING:
		ValueType = FLobbyAttribute::String;
		AsString = Attr->Data->Value.AsUtf8;
		break;
	}
}

EOS_Lobby_AttributeData FLobbyAttribute::ToAttributeData() const
{
	EOS_Lobby_AttributeData AttributeData;

	AttributeData.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
	AttributeData.Key = Key.c_str();

	switch (ValueType)
	{
	case FLobbyAttribute::Type::Bool:
		AttributeData.ValueType = EOS_ELobbyAttributeType::EOS_AT_BOOLEAN;
		AttributeData.Value.AsBool = AsBool;
		break;
	case FLobbyAttribute::Type::Double:
		AttributeData.ValueType = EOS_ELobbyAttributeType::EOS_AT_DOUBLE;
		AttributeData.Value.AsDouble = AsDouble;
		break;
	case FLobbyAttribute::Type::Int64:
		AttributeData.ValueType = EOS_ELobbyAttributeType::EOS_AT_INT64;
		AttributeData.Value.AsInt64 = AsInt64;
		break;
	case FLobbyAttribute::Type::String:
		AttributeData.ValueType = EOS_ELobbyAttributeType::EOS_AT_STRING;
		AttributeData.Value.AsUtf8 = AsString.c_str();
		break;
	}

	return AttributeData;
}

std::wstring FLobbyMember::GetAttributesString() const
{
	std::wstring Result;
	for (const FLobbyAttribute& Attribute : MemberAttributes)
	{
		std::wstring AttributeString = L"{ Key = " + FStringUtils::Widen(Attribute.Key);

		//We only support strings in the sample
		AttributeString += L" Value = " + FStringUtils::Widen(Attribute.AsString) + L" } ";

		Result += AttributeString;
	}

	return Result;
}
