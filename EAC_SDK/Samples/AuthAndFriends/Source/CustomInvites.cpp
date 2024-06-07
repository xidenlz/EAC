// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "DebugLog.h"
#include "Game.h"
#include "GameEvent.h"
#include "Player.h"
#include "Platform.h"
#include "eos_friends.h"
#include "eos_custominvites.h"
#include "eos_presence.h"
#include <string>
#include "Users.h"
#include "eos_custominvites.h"
#include "eos_custominvites_types.h"


FCustomInvites::FCustomInvites() noexcept(false)
{
	CurrentUserId = FEpicAccountId();
}


FCustomInvites::~FCustomInvites()
{

}

void FCustomInvites::OnLoggedIn(FEpicAccountId UserId)
{
	if (!CurrentUserId.IsValid())
	{
		SetCurrentUser(UserId);

		EOS_HCustomInvites CustomInvitesHandle = EOS_Platform_GetCustomInvitesInterface(FPlatform::GetPlatformHandle());

		// subscribe to notifications for when we receive a custom invite
		EOS_CustomInvites_AddNotifyCustomInviteReceivedOptions AddNotifyInviteReceivedOptions = {};
		AddNotifyInviteReceivedOptions.ApiVersion = EOS_CUSTOMINVITES_ADDNOTIFYCUSTOMINVITERECEIVED_API_LATEST;
		EOS_CustomInvites_AddNotifyCustomInviteReceived(CustomInvitesHandle, &AddNotifyInviteReceivedOptions, nullptr, StaticOnNotifyCustomInviteReceived);
		
		// subscribe to notifications for when we accept a custom invite externally (e.g. via the Overlay)
		EOS_CustomInvites_AddNotifyCustomInviteAcceptedOptions AddNotifyInviteAcceptedOptions = {};
		AddNotifyInviteAcceptedOptions.ApiVersion = EOS_CUSTOMINVITES_ADDNOTIFYCUSTOMINVITEACCEPTED_API_LATEST;
		EOS_CustomInvites_AddNotifyCustomInviteAccepted(CustomInvitesHandle, &AddNotifyInviteAcceptedOptions, nullptr, StaticOnNotifyCustomInviteAccepted);

		// subscribe to notifications for when we reject a custom invite externally (e.g. via the Overlay)
		EOS_CustomInvites_AddNotifyCustomInviteRejectedOptions AddNotifyInviteRejectedOptions = {};
		AddNotifyInviteRejectedOptions.ApiVersion = EOS_CUSTOMINVITES_ADDNOTIFYCUSTOMINVITEREJECTED_API_LATEST;
		EOS_CustomInvites_AddNotifyCustomInviteRejected(CustomInvitesHandle, &AddNotifyInviteRejectedOptions, nullptr, StaticOnNotifyCustomInviteRejected);
	}
}

void FCustomInvites::OnConnectLoggedIn(FProductUserId ProductUserId)
{
	if (!CurrentProductUserId.IsValid())
	{
		CurrentProductUserId = ProductUserId;
	}
}

void FCustomInvites::OnLoggedOut(FEpicAccountId UserId)
{
	if (FPlayerManager::Get().GetNumPlayers() > 0)
	{
		if (GetCurrentUser() == UserId)
		{
			FGameEvent Event(EGameEventType::ShowNextUser);
			OnGameEvent(Event);
		}
	}
	else
	{
		SetCurrentUser(FEpicAccountId());
		CurrentProductUserId = FProductUserId();
	}
}

void FCustomInvites::OnGameEvent(const FGameEvent& Event)
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
		OnConnectLoggedIn(Event.GetProductUserId());
	}
	else if (Event.GetType() == EGameEventType::ShowPrevUser)
	{
		if (FPlayerManager::Get().GetNumPlayers() > 0)
		{
			FEpicAccountId PrevPlayerId = FPlayerManager::Get().GetPrevPlayerId(GetCurrentUser());
			if (PrevPlayerId.IsValid())
			{
				SetCurrentUser(PrevPlayerId);
			}
			else
			{
				SetCurrentUser(FEpicAccountId());
			}
		}
	}
	else if (Event.GetType() == EGameEventType::ShowNextUser)
	{
		if (FPlayerManager::Get().GetNumPlayers() > 0)
		{
			FEpicAccountId NextPlayerId = FPlayerManager::Get().GetNextPlayerId(GetCurrentUser());
			if (NextPlayerId.IsValid())
			{
				SetCurrentUser(NextPlayerId);
			}
			else
			{
				SetCurrentUser(FEpicAccountId());
			}
		}
	}
	else if (Event.GetType() == EGameEventType::CancelLogin)
	{
	}
}

void FCustomInvites::SetCurrentUser(FEpicAccountId UserId)
{
	CurrentUserId = UserId;

	if (CurrentUserId.IsValid())
	{
		PlayerPtr CurrentPlayer = FPlayerManager::Get().GetPlayer(CurrentUserId);
		if (CurrentPlayer)
		{
			CurrentProductUserId = CurrentPlayer->GetProductUserID();
		}
		else
		{
			CurrentProductUserId = FProductUserId();
		}
	}
	else
	{
		CurrentProductUserId = FProductUserId();
	}
}

void FCustomInvites::FinalizeInvite(EOS_EResult Result)
{
	EOS_HCustomInvites CustomInvitesHandle = EOS_Platform_GetCustomInvitesInterface(FPlatform::GetPlatformHandle());

	// Notify the SDK that this invite has been fully processed and can be removed from internal lists and the Overlay UI
	EOS_CustomInvites_FinalizeInviteOptions FinalizeOptions = {};
	FinalizeOptions.ApiVersion = EOS_CUSTOMINVITES_FINALIZEINVITE_API_LATEST;
	FinalizeOptions.LocalUserId = CurrentProductUserId;
	FinalizeOptions.TargetUserId = LatestReceivedFromId;
	std::string InviteId = FStringUtils::Narrow(LatestReceivedInviteId);
	FinalizeOptions.CustomInviteId = InviteId.c_str();
	FinalizeOptions.ProcessingResult = Result;
	EOS_CustomInvites_FinalizeInvite(CustomInvitesHandle, &FinalizeOptions);
}

void FCustomInvites::AcceptLatestInvite()
{
	FinalizeInvite(EOS_EResult::EOS_Success);

	LatestReceivedFromId = nullptr;
	LatestReceivedInviteId.clear();
	LatestReceivedPayload.clear();

	FGameEvent GameEvent(EGameEventType::CustomInviteAccepted);
	FGame::Get().OnGameEvent(GameEvent);
}

void FCustomInvites::RejectLatestInvite()
{
	FinalizeInvite(EOS_EResult::EOS_Success);

	LatestReceivedFromId = nullptr;
	LatestReceivedInviteId.clear();
	LatestReceivedPayload.clear();

	FGameEvent GameEvent(EGameEventType::CustomInviteDeclined);
	FGame::Get().OnGameEvent(GameEvent);
}

void EOS_CALL FCustomInvites::StaticOnNotifyCustomInviteReceived(const EOS_CustomInvites_OnCustomInviteReceivedCallbackInfo* Data)
{
	if (Data)
	{
		FDebugLog::Log(L"CustomInvites (OnNotifyCustomInviteReceived): invite received.");
		FGame::Get().GetCustomInvites()->HandleCustomInviteReceived(Data->Payload, Data->CustomInviteId, Data->TargetUserId);
	}
	else
	{
		FDebugLog::LogError(L"CustomInvites (OnNotifyCustomInviteReceived): EOS_CustomInvites_OnCustomInviteReceivedCallbackInfo is null");
	}
}

void EOS_CALL FCustomInvites::StaticOnNotifyCustomInviteAccepted(const EOS_CustomInvites_OnCustomInviteAcceptedCallbackInfo* Data)
{
	if (Data)
	{
		FDebugLog::Log(L"CustomInvites (OnNotifyCustomInviteAccepted): invite accepted from Overlay.");
		FGame::Get().GetCustomInvites()->HandleCustomInviteAccepted(Data->Payload, Data->CustomInviteId, Data->TargetUserId);
	}
	else
	{
		FDebugLog::LogError(L"CustomInvites (OnNotifyCustomInviteAccepted): EOS_CustomInvites_OnCustomInviteAcceptedCallbackInfo is null");
	}
}

void EOS_CALL FCustomInvites::StaticOnNotifyCustomInviteRejected(const EOS_CustomInvites_CustomInviteRejectedCallbackInfo* Data)
{
	if (Data)
	{
		FDebugLog::Log(L"CustomInvites (OnNotifyCustomInviteRejected): invite rejected from Overlay.");
		FGame::Get().GetCustomInvites()->HandleCustomInviteRejected(Data->Payload, Data->CustomInviteId, Data->TargetUserId);
	}
	else
	{
		FDebugLog::LogError(L"CustomInvites (OnNotifyCustomInviteRejected): EOS_CustomInvites_CustomInviteRejectedCallbackInfo is null");
	}
}

void FCustomInvites::HandleCustomInviteReceived(const char* Payload, const char* InviteId, FProductUserId SenderId)
{
	LatestReceivedFromId = SenderId;
	LatestReceivedInviteId = FStringUtils::Widen(InviteId);
	LatestReceivedPayload = FStringUtils::Widen(Payload);

	FGameEvent GameEvent(EGameEventType::CustomInviteReceived);
	FGame::Get().OnGameEvent(GameEvent);
}


void FCustomInvites::HandleCustomInviteAccepted(const char* Payload, const char* InviteId, FProductUserId SenderId)
{
	AcceptLatestInvite();
}

void FCustomInvites::HandleCustomInviteRejected(const char* Payload, const char* InviteId, FProductUserId SenderId)
{
	RejectLatestInvite();
}

EOS_EResult FCustomInvites::SetCurrentPayload(const std::wstring& Payload)
{
	FDebugLog::Log(L"EOS SDK SetCurrentPayload: %ls", Payload.c_str());

	EOS_HCustomInvites CustomInvitesHandle = EOS_Platform_GetCustomInvitesInterface(FPlatform::GetPlatformHandle());

	EOS_CustomInvites_SetCustomInviteOptions SetInviteOptions = {};
	SetInviteOptions.ApiVersion = EOS_CUSTOMINVITES_SETCUSTOMINVITE_API_LATEST;
	SetInviteOptions.LocalUserId = CurrentProductUserId;
	std::string NarrowPayload = FStringUtils::Narrow(Payload);
	SetInviteOptions.Payload = NarrowPayload.c_str();
	return EOS_CustomInvites_SetCustomInvite(CustomInvitesHandle, &SetInviteOptions);
}

void FCustomInvites::SendInviteToFriend(const std::wstring& FriendName)
{
	FGame::Get().GetUsers()->QueryUserInfo(CurrentUserId, FriendName, [this, FriendName](const FUserData& UserData)
	{
		FProductUserId UserId = FGame::Get().GetUsers()->GetExternalAccountMapping(UserData.UserId);
		if (UserId.IsValid())
		{
			SendInviteToFriend(UserId);
		}
		else
		{
			FDebugLog::Log(L"EOS SDK SendInviteToFriend: No External Account Mapping found for %ls", FriendName.c_str());
			// user does not have a PUID which means either:
				// it's very early on and the refresh / mapping process hasn't completed yet
				// user hasn't played this game before
		}
	});
}

void FCustomInvites::SendInviteToFriend(FProductUserId FriendId)
{
	FDebugLog::Log(L"EOS SDK SendInviteToFriend: %ls", FriendId.ToString().c_str());

	EOS_HCustomInvites CustomInvitesHandle = EOS_Platform_GetCustomInvitesInterface(FPlatform::GetPlatformHandle());

	EOS_CustomInvites_SendCustomInviteOptions SendInviteOptions = {};
	SendInviteOptions.ApiVersion = EOS_CUSTOMINVITES_SENDCUSTOMINVITE_API_LATEST;
	SendInviteOptions.LocalUserId = CurrentProductUserId;
	EOS_ProductUserId TargetUserId(FriendId.AccountId);
	SendInviteOptions.TargetUserIds = &TargetUserId;
	SendInviteOptions.TargetUserIdsCount = 1;
	EOS_CustomInvites_SendCustomInvite(CustomInvitesHandle, &SendInviteOptions, nullptr, [](const EOS_CustomInvites_SendCustomInviteCallbackInfo* Data) {});
}
