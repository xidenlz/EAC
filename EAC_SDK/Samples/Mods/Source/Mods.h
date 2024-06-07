// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include <eos_sdk.h>
#include <eos_mods.h>
#include <unordered_set>

using EOS_Mods_HModsInfo = EOS_Mods_ModInfo*;

/**
* Mod Identifier class.
*/
struct FModId
{
	/** Product namespace id in which this mod item exists */
	std::string NamespaceId;
	/* Item id of the Mod */
	std::string ItemId;
	/* Artifact id of the Mod */
	std::string ArtifactId;

	size_t Hash() const
	{
		size_t Seed = 0;
		Seed = std::hash<std::string>{}(NamespaceId);
		Seed ^= std::hash<std::string>{}(ArtifactId);
		return Seed ^ std::hash<std::string>{}(ItemId);
	}

	friend bool operator<(const FModId& Lhs, const FModId& Rhs)
	{
		return Lhs.Hash() < Rhs.Hash();
	}
};

/**
 * States in which the Mod can be in.
*/
enum class EModState
{
	Unknown = 0,
	Installed = 1,
	Available = 2,
};

/**
* Action Types that can be applied to Mods.
*/
enum class ERequestType
{
	Install = 0,
	Uninstall = 1,
	Update = 2,
};

/**
* Query Types that can be applied to Mods.
*/
enum class EModsQueryType
{
	Installed = 0,
	All = 1,
};

/**
* Contains basic details of the Mod.
*/
struct FModDetail
{
	/** Unique Identifier of Mod*/
	FModId Id;
	/** Represents Mod item title. */
	std::string Title;
	/** Represents Mod item version. */
	std::string Version;
	/** Shows Mod is installed or available*/
	EModState State = EModState::Unknown;

	FModDetail() = default;

	FModDetail(FModId Id);

	FModDetail(EOS_Mods_ModInfo* ModsInfo, int Index);
	
	/** Extracts Mod enumeration type from SDK ModsInfo object */
	EModState GetState(EOS_Mods_ModInfo* ModsInfo);
};

/**
* Manages Mods
*/
class FMods
{
public:
	/**
	* Constructor
	*/
	FMods() noexcept(false);

	/**
	* No copying or copy assignment allowed for this class.
	*/
	FMods(FMods const&) = delete;
	FMods& operator=(FMods const&) = delete;

	/**
	* Destructor
	*/
	virtual ~FMods();

	/**
	* Initialization
	*/
	void Init();

	/**
	* Update
	*/
	void Update();

	/**
	* Called when User is Loged out. 
	*/
	void OnLoggedOut(FEpicAccountId UserId);

	/**
	* Receives game event
	*
	* @param Event - Game event to act on
	*/
	void OnGameEvent(const FGameEvent& Event);
	
	/**
	* Sets RemoveAfterExit flag (When this flag is true then Mods will get uninstalled after the game exits).
	*/
	void SetRemoveAfterExit(bool DefaultRemoveAfterExit);
	
	/**
	* Installs mod.
	*/
	void InstallMod(FModId ModId);
	void InstallMod(FModId ModId, bool bRemoveAfterExit);
	static void EOS_CALL OnInstallComplete(const EOS_Mods_InstallModCallbackInfo* Data);
	
	/**
	* Uninstalls Mod.
	*/
	void UnInstallMod(FModId ModId);
	static void EOS_CALL OnUninstallComplete(const EOS_Mods_UninstallModCallbackInfo* Data);
	
	/**
	* Updates Mod.
	*/
	void UpdateMod(FModId ModId);
	static void EOS_CALL OnUpdateComplete(const EOS_Mods_UpdateModCallbackInfo* Data);

	/**
	* Enumerates Mods.
	* @param Type Shows to enumerate all available or only installed mods.
	*/
	void EnumerateMods(EModsQueryType Type);
	static void EOS_CALL OnEnumerateModsComplete(const EOS_Mods_EnumerateModsCallbackInfo* Data);

	/**
	* Shows whether Mods cached data are stale or not.
	*/
	bool IsDirty() const
	{
		return bIsModDetailsChanged;
	}

	/**
	* Sets whether Mods cached data is dirty or not.
	*/
	void SetIsDirty(bool IsDirty)
	{
		bIsModDetailsChanged = IsDirty;
	}

	/**
	* Returns Mods cached data.
	*/
	const std::map<FModId, FModDetail>& GetModDetails() const
	{
		return ModDetailsCache;
	}

	/** Adds data for testing dialog */
	void AddTestData();

private:
	
	/**
	* Completes current operation.
	*/
	void Complete();

	/**
	* Validates preconditions before each operation.
	*/
	bool ValidatePreconditions();

	/**
	* Extracts data from returned ModsInfo and Updates Mods cache data.
	*/
	void UpdateDetailsFromModsInfo(EOS_Mods_HModsInfo ModsInfo, EOS_EModEnumerationType Type, EOS_EpicAccountId UserId);

	/**
	* Queries Mods info from SDK.
	*/
	void FetchMods(EOS_EModEnumerationType Type);

	/**
	* Updates internal state after successful request.
	*/
	void UpdateStateAfterRequestCompletion();

	/**
	* Contains Request information.
	*/
	struct FRequest
	{
		/** Identity of the Mod. */
		FModId ModId;

		/** Request Type. [ install | uninstall | update ] */
		ERequestType Type;
		
		/** Passed to SDK API as parameter. */
		EOS_Mod_Identifier Identifier;

		const EOS_Mod_Identifier& ModIdentifier()
		{
			Identifier.ApiVersion = EOS_MOD_IDENTIFIER_API_LATEST;
			Identifier.NamespaceId = ModId.NamespaceId.c_str();
			Identifier.ArtifactId = ModId.ArtifactId.c_str();
			Identifier.ItemId = ModId.ItemId.c_str();
			Identifier.Title = nullptr;
			Identifier.Version = nullptr;
			return Identifier;
		}
	};
	
private:
	
	/** Handle to EOS SDK Mods system */
	EOS_HMods ModsHandle;
	
	/** Command to install, uninstall or update Mod. */
	std::unique_ptr<FRequest> Command;
	
	/** Shows whether to eumerate only installed or all Mods. */
	EModsQueryType QueryType;
	
	/** Showes whether the last Query is in progress or not. */
	bool bIsQueryInProgress = false;
	
	/** Stores cached result of last enumeration. */
	std::map<FModId, FModDetail> ModDetailsCache;

	/** True whenever ModDetails cache changes. */
	bool bIsModDetailsChanged = false;

	/** When true Mods will be removed after game is closed. */
	bool bDefaultRemoveAfterExit = false;
};
