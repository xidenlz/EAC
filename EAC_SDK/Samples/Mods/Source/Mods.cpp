// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "DebugLog.h"
#include "StringUtils.h"
#include "Utils.h"
#include "AccountHelpers.h"
#include "Game.h"
#include "GameEvent.h"
#include "Main.h"
#include "Platform.h"
#include "Users.h"
#include "Player.h"
#include "Mods.h"
#include <eos_mods.h>


FModDetail::FModDetail(FModId Id) : Id(Id)
{
}

FModDetail::FModDetail(EOS_Mods_ModInfo* ModsInfo, int Index)
{
	auto& Mod = ModsInfo->Mods[Index];
	Id.ArtifactId = Mod.ArtifactId;
	Id.ItemId = Mod.ItemId;
	Id.NamespaceId = Mod.NamespaceId;
	Title = Mod.Title;
	Version = Mod.Version;
	State = GetState(ModsInfo);
}

EModState FModDetail::GetState(EOS_Mods_ModInfo* ModsInfo)
{
	switch (ModsInfo->Type)
	{
		case EOS_EModEnumerationType::EOS_MET_INSTALLED:
			return EModState::Installed;
		case EOS_EModEnumerationType::EOS_MET_ALL_AVAILABLE:
			return EModState::Available;
	}
	FDebugLog::LogError(L"Mods: Unknown State.");
	return EModState::Unknown;
}

FMods::FMods()
{
}

FMods::~FMods()
{

}

void FMods::Init()
{
	if (FPlatform::GetPlatformHandle() != nullptr)
	{
		ModsHandle = EOS_Platform_GetModsInterface(FPlatform::GetPlatformHandle());
	}
}

void FMods::Update()
{

}

void FMods::UpdateStateAfterRequestCompletion()
{
	ModDetailsCache.clear();
	FetchMods(EOS_EModEnumerationType::EOS_MET_INSTALLED);
}

void FMods::SetRemoveAfterExit(bool DefaultRemoveAfterExit)
{
	bDefaultRemoveAfterExit = DefaultRemoveAfterExit;
}

void FMods::InstallMod(FModId ModId)
{
	InstallMod(ModId, bDefaultRemoveAfterExit);
}

void FMods::InstallMod(FModId ModId, bool bRemoveAfterExit)
{
	if (ValidatePreconditions() == false)
	{
		return;
	}

	Command = std::make_unique<FRequest>(FRequest{ std::move(ModId), ERequestType::Install });

	EOS_Mods_InstallModOptions Opt = {};
	Opt.ApiVersion = EOS_MODS_INSTALLMOD_API_LATEST;
	Opt.LocalUserId = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser())->GetUserID();
	Opt.Mod = &Command->ModIdentifier();
	Opt.bRemoveAfterExit = bRemoveAfterExit ? EOS_TRUE : EOS_FALSE;

	EOS_Mods_InstallMod(ModsHandle, &Opt, this, OnInstallComplete);
}

void FMods::OnInstallComplete(const EOS_Mods_InstallModCallbackInfo* Data)
{
	if (!Data)
	{
		FDebugLog::LogError(L"Mods: Mod Installation failed.");
		return;
	}

	if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
	{
		// Operation is retrying so it is not complete yet
		return;
	}

	FMods& Self = *static_cast<FMods*>(Data->ClientData);
	
	if (Data->ResultCode != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogWarning(L"Mods: Install operation error: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		Self.Complete();
		return;
	}

	FDebugLog::Log(L"Mods: Install operation completed successfully.");

	Self.UpdateStateAfterRequestCompletion();
}

void FMods::UnInstallMod(FModId ModId)
{
	if (ValidatePreconditions() == false)
	{
		return;
	}

	Command = std::make_unique<FRequest>(FRequest{ std::move(ModId), ERequestType::Uninstall });

	EOS_Mods_UninstallModOptions Opt = {};
	Opt.ApiVersion = EOS_MODS_UNINSTALLMOD_API_LATEST;
	Opt.LocalUserId = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser())->GetUserID();
	Opt.Mod = &Command->ModIdentifier();

	EOS_Mods_UninstallMod(ModsHandle, &Opt, this, OnUninstallComplete);
}

void FMods::OnUninstallComplete(const EOS_Mods_UninstallModCallbackInfo* Data)
{
	if (!Data)
	{
		FDebugLog::LogError(L"Mods: Mod Uninstall failed.");
		return;
	}

	if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
	{
		// Operation is retrying so it is not complete yet
		return;
	}

	FMods& Self = *static_cast<FMods*>(Data->ClientData);

	if (Data->ResultCode != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogWarning(L"Mods: Uninstall operation error: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		Self.Complete();
		return;
	}

	FDebugLog::Log(L"Mods: Uninstall operation completed successfully.");

	Self.UpdateStateAfterRequestCompletion();
}

bool FMods::ValidatePreconditions()
{
	if (FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser()) == nullptr)
	{
		FDebugLog::LogError(L"Mods: Validation failed. Current player is invalid, no user logged in.");
		return false;
	}
	if (Command)
	{
		FDebugLog::LogWarning(L"Mods: Validation failed. Previous request is in progress.");
		return false;
	}
	if (bIsQueryInProgress)
	{
		FDebugLog::LogWarning(L"Mods: Validation failed. Mods query is in progress.");
		return false;
	}
	return true;
}

void FMods::UpdateMod(FModId ModId)
{
	if (ValidatePreconditions() == false)
	{
		return;
	}

	Command = std::make_unique<FRequest>(FRequest{ std::move(ModId), ERequestType::Update });

	EOS_Mods_UpdateModOptions Opt = {};
	Opt.ApiVersion = EOS_MODS_UPDATEMOD_API_LATEST;
	Opt.LocalUserId = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser())->GetUserID();
	Opt.Mod = &Command->ModIdentifier();

	EOS_Mods_UpdateMod(ModsHandle, &Opt, this, OnUpdateComplete);
}

void FMods::OnUpdateComplete(const EOS_Mods_UpdateModCallbackInfo* Data)
{
	if (!Data)
	{
		FDebugLog::LogError(L"Mods: Mod Update failed.");
		return;
	}

	if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
	{
		// Operation is retrying so it is not complete yet
		return;
	}

	FMods& Self = *static_cast<FMods*>(Data->ClientData);

	if (Data->ResultCode != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogWarning(L"Mods: Update operation error: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		Self.Complete();
		return;
	}

	FDebugLog::Log(L"Mods: Update operation completed successfully.");

	Self.UpdateStateAfterRequestCompletion();
}

void FMods::EnumerateMods(EModsQueryType Type)
{
	if (ValidatePreconditions() == false)
	{
		return;
	}
	ModDetailsCache.clear();
	bIsQueryInProgress = true;
	QueryType = Type;
	FetchMods(EOS_EModEnumerationType::EOS_MET_INSTALLED);
}

void FMods::FetchMods(EOS_EModEnumerationType Type)
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player == nullptr)
	{
		FDebugLog::LogError(L"Mods - FetchMods: Current player is invalid!");
		return;
	}

	EOS_Mods_EnumerateModsOptions Options = {};
	Options.ApiVersion = EOS_MODS_ENUMERATEMODS_API_LATEST;
	Options.LocalUserId = Player->GetUserID();
	Options.Type = Type;

	EOS_Mods_EnumerateMods(ModsHandle, &Options, this, OnEnumerateModsComplete);
}

void EOS_CALL FMods::OnEnumerateModsComplete(const EOS_Mods_EnumerateModsCallbackInfo* Data)
{
	if (Data)
	{
		if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
		{
			// Operation is retrying so it is not complete yet
			return;
		}

		FMods& Mods = *static_cast<FMods*>(Data->ClientData);
		
		if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			switch (Data->Type)
			{
				case EOS_EModEnumerationType::EOS_MET_INSTALLED:
					FDebugLog::LogError(L"Mods: Enumerate installed mods error: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
					break;
				case EOS_EModEnumerationType::EOS_MET_ALL_AVAILABLE:
					FDebugLog::LogError(L"Mods: Enumerate available mods error: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
					break;
				default:
					FDebugLog::LogError(L"Mods: Unknown Type {%d} was specified for mod enumeration.", static_cast<int>(Data->Type));
			}
			Mods.Complete();
			return;
		}

		EOS_Mods_CopyModInfoOptions CopyAsOptions = {};
		CopyAsOptions.ApiVersion = EOS_MODS_COPYMODINFO_API_LATEST;
		CopyAsOptions.LocalUserId = Data->LocalUserId;
		CopyAsOptions.Type = Data->Type;
		EOS_Mods_HModsInfo ModsInfoHandle;

		if (EOS_Mods_CopyModInfo(Mods.ModsHandle, &CopyAsOptions, &ModsInfoHandle) != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"Mods: Enumerate Mods failed when copying from ModsInfo with error: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
			Mods.Complete();
		}
		else
		{
			FDebugLog::Log(L"Mods: Mod Enumeration succeeded.");
			Mods.UpdateDetailsFromModsInfo(ModsInfoHandle, Data->Type, Data->LocalUserId);
		}
	}
	else
	{
		FDebugLog::LogError(L"Mods: Mod Enumeration failed.");
	}
}

void FMods::OnLoggedOut(FEpicAccountId UserId)
{
	ModDetailsCache.clear();
	SetIsDirty(true);
}

void FMods::OnGameEvent(const FGameEvent& Event)
{
	if (Event.GetType() == EGameEventType::UserLoggedOut)
	{
		OnLoggedOut(Event.GetUserId());
	}
}

void FMods::Complete()
{
	SetIsDirty(true);
	bIsQueryInProgress = false;
	Command.reset();
}

void FMods::UpdateDetailsFromModsInfo(EOS_Mods_HModsInfo ModsInfo, EOS_EModEnumerationType Type, EOS_EpicAccountId UserId)
{
	if (FPlayerManager::Get().GetPlayer(UserId) == nullptr)
	{
		// User is no longer logged in
		Complete();
		return;
	}

	if (ModsInfo && ModsInfo->Mods)
	{
		for (auto Index = 0; Index < ModsInfo->ModsCount; ++Index)
		{
			FModDetail ModDetail(ModsInfo, Index);

			switch (Type)
			{
				case EOS_EModEnumerationType::EOS_MET_INSTALLED:
				{
					auto Found = ModDetailsCache.find(ModDetail.Id);
					ModDetail.State = EModState::Installed;

					if (Found != ModDetailsCache.end())
					{
						Found->second = ModDetail;
					}
					else
					{
						ModDetailsCache.emplace(ModDetail.Id, ModDetail);
					}
				}
				break;
				case EOS_EModEnumerationType::EOS_MET_ALL_AVAILABLE:
				{
					auto Found = ModDetailsCache.find(ModDetail.Id);
					if (Found == ModDetailsCache.end())
					{
						ModDetail.State = EModState::Available;
						ModDetailsCache.emplace(ModDetail.Id, ModDetail);
					}
				}
				break;
			}
		}
		if (QueryType == EModsQueryType::All && Type == EOS_EModEnumerationType::EOS_MET_INSTALLED)
		{
			FetchMods(EOS_EModEnumerationType::EOS_MET_ALL_AVAILABLE);
		}
		else
		{
			Complete();
		}
	}
}

void FMods::AddTestData()
{
	const int ModsCount = 20;

	const char* NamespaceIds[ModsCount] =
	{
		"2216ecfe2aac4a7798b07eacea14ed01", "2216ecfe2aac4a7798b07eacea14ed02", "2216ecfe2aac4a7798b07eacea14ed03", "2216ecfe2aac4a7798b07eacea14ed04",
		"2216ecfe2aac4a7798b07eacea14ed05", "2216ecfe2aac4a7798b07eacea14ed06", "2216ecfe2aac4a7798b07eacea14ed07", "2216ecfe2aac4a7798b07eacea14ed08",
		"2216ecfe2aac4a7798b07eacea14ed09", "2216ecfe2aac4a7798b07eacea14ed10", "2216ecfe2aac4a7798b07eacea14ed11", "2216ecfe2aac4a7798b07eacea14ed12",
		"2216ecfe2aac4a7798b07eacea14ed13", "2216ecfe2aac4a7798b07eacea14ed14", "2216ecfe2aac4a7798b07eacea14ed15", "2216ecfe2aac4a7798b07eacea14ed16",
		"2216ecfe2aac4a7798b07eacea14ed17", "2216ecfe2aac4a7798b07eacea14ed18", "2216ecfe2aac4a7798b07eacea14ed19", "2216ecfe2aac4a7798b07eacea14ed20"
	};

	const char* ItemIds[ModsCount] =
	{
		"037d913458f04e2887f8d5a884d5f901", "037d913458f04e2887f8d5a884d5f902", "037d913458f04e2887f8d5a884d5f903", "037d913458f04e2887f8d5a884d5f904",
		"037d913458f04e2887f8d5a884d5f905", "037d913458f04e2887f8d5a884d5f906", "037d913458f04e2887f8d5a884d5f907", "037d913458f04e2887f8d5a884d5f908",
		"037d913458f04e2887f8d5a884d5f909", "037d913458f04e2887f8d5a884d5f910", "037d913458f04e2887f8d5a884d5f911", "037d913458f04e2887f8d5a884d5f912",
		"037d913458f04e2887f8d5a884d5f913", "037d913458f04e2887f8d5a884d5f914", "037d913458f04e2887f8d5a884d5f915", "037d913458f04e2887f8d5a884d5f916",
		"037d913458f04e2887f8d5a884d5f917", "037d913458f04e2887f8d5a884d5f918", "037d913458f04e2887f8d5a884d5f919", "037d913458f04e2887f8d5a884d5f920"
	};

	const char* ArtifactIds[ModsCount] =
	{
		"5bc0de68c52549649ac0a695e1dd6901", "5bc0de68c52549649ac0a695e1dd6902", "5bc0de68c52549649ac0a695e1dd6903", "5bc0de68c52549649ac0a695e1dd6904",
		"5bc0de68c52549649ac0a695e1dd6905", "5bc0de68c52549649ac0a695e1dd6906", "5bc0de68c52549649ac0a695e1dd6907", "5bc0de68c52549649ac0a695e1dd6908",
		"5bc0de68c52549649ac0a695e1dd6909", "5bc0de68c52549649ac0a695e1dd6910", "5bc0de68c52549649ac0a695e1dd6911", "5bc0de68c52549649ac0a695e1dd6912",
		"5bc0de68c52549649ac0a695e1dd6913", "5bc0de68c52549649ac0a695e1dd6914", "5bc0de68c52549649ac0a695e1dd6915", "5bc0de68c52549649ac0a695e1dd6916",
		"5bc0de68c52549649ac0a695e1dd6917", "5bc0de68c52549649ac0a695e1dd6918", "5bc0de68c52549649ac0a695e1dd6919", "5bc0de68c52549649ac0a695e1dd6920"
	};

	const char* Titles[ModsCount] =
	{
		"Mod 1",	"Mod 2",	"Mod 3",	"Mod 4",
		"Mod 5",	"Mod 6",	"Mod 7",	"Mod 8",
		"Mod 9",	"Mod 10",	"Mod 11",	"Mod 12",
		"Mod 13",	"Mod 14",	"Mod 15",	"Mod 16",
		"Mod 17",	"Mod 18",	"Mod 19",	"Mod 20"
	};

	const char* Versions[ModsCount] =
	{
		"1.0.0",	"1.0.0",	"1.0.0",	"1.0.0",
		"1.0.1",	"1.0.0",	"1.0.0",	"1.0.0",
		"1.0.0",	"1.2.0",	"3.1.0",	"1.0.0",
		"1.0.0",	"1.0.0",	"1.0.0",	"1.0.0",
		"1.0.0",	"2.0.0",	"1.0.0",	"1.0.0"
	};

	ModDetailsCache.clear();

	EOS_Mods_ModInfo* ModsInfo = new EOS_Mods_ModInfo{ 0 };
	ModsInfo->ApiVersion = EOS_MODS_MODINFO_API_LATEST;
	ModsInfo->ModsCount = ModsCount;
	EOS_Mod_Identifier* Mods = new EOS_Mod_Identifier[ModsInfo->ModsCount];
	for (int32_t Index = 0; Index < ModsInfo->ModsCount; ++Index)
	{
		EOS_Mod_Identifier& Mod = Mods[Index];
		Mod.ApiVersion = EOS_MOD_IDENTIFIER_API_LATEST;
		Mod.NamespaceId = NamespaceIds[Index];
		Mod.ItemId = ItemIds[Index];
		Mod.ArtifactId = ArtifactIds[Index];
		Mod.Title = Titles[Index];
		Mod.Version = Versions[Index];
	}
	ModsInfo->Mods = Mods;

	for (auto Index = 0; Index < ModsCount; ++Index)
	{
		FModDetail ModDetail(ModsInfo, Index);
		ModDetail.State = Index % 2 == 0 ? EModState::Installed : EModState::Available;
		ModDetailsCache.emplace(ModDetail.Id, ModDetail);
	}

	Complete();
}
