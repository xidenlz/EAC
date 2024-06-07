// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Dialog.h"
#include "Font.h"
#include "ListView.h"

/**
 * Forward declarations
 */
class FGameEvent;
class FSpriteWidget;
struct FAchievementsDefinitionData;

/**
 * Achievements Definitions Name
 */
class FAchievementsDefinitionName : public FTextLabelWidget
{
public:
	FAchievementsDefinitionName(Vector2 LabelPos,
		Vector2 LabelSize,
		UILayer LabelLayer,
		const std::wstring& LabelText,
		const std::wstring& LabelAssetFile) : FTextLabelWidget(LabelPos, LabelSize, LabelLayer, LabelText, LabelAssetFile, Color::White, Color::White, EAlignmentType::Left)
	{}

	void SetFocused(bool bValue) override;

	/**
	* Implementation of 'SetData' for List view. Compresses the string (by truncating the middle part) to match the widget size before setting this text.
	*/
	void SetData(const std::wstring& Data);
};

template<>
std::shared_ptr<FAchievementsDefinitionName> CreateListEntry(Vector2 Pos, Vector2 Size, UILayer Layer, const std::wstring& Data);

using FAchievementsDefinitionNamesList = FListViewWidget<std::wstring, FAchievementsDefinitionName>;

/**
 * Achievements Definitions Info
 */
class FAchievementsDefinitionInfo : public FTextLabelWidget
{
public:
	FAchievementsDefinitionInfo(Vector2 LabelPos,
		Vector2 LabelSize,
		UILayer LabelLayer,
		const std::wstring& LabelText,
		const std::wstring& LabelAssetFile) : FTextLabelWidget(LabelPos, LabelSize, LabelLayer, LabelText, LabelAssetFile, Color::White, Color::White, EAlignmentType::Left)
	{}

	void SetFocused(bool bValue) override;
};

template<>
std::shared_ptr<FAchievementsDefinitionInfo> CreateListEntry(Vector2 Pos, Vector2 Size, UILayer Layer, const std::wstring& Data);

using FAchievementsDefinitionsList = FListViewWidget<std::wstring, FAchievementsDefinitionInfo>;

/**
 * Achievements Definitions dialog
 */
class FAchievementsDefinitionsDialog : public FDialog
{
public:
	/**
	 * Constructor
	 */
	FAchievementsDefinitionsDialog(Vector2 DialogPos,
		Vector2 DialogSize,
		UILayer DialogLayer,
		FontPtr DialogNormalFont,
		FontPtr DialogSmallFont,
		FontPtr DialogTinyFont);

	/**
	 * Destructor
	 */
	virtual ~FAchievementsDefinitionsDialog() {};

	/**
	* IGfxComponent Overrides
	*/
	virtual void Update() override;
	virtual void Create() override;

	/** IWidget */
	virtual void SetPosition(Vector2 Pos) override;
	virtual void SetSize(Vector2 NewSize) override;

	/** Set Window Size */
	void SetWindowSize(Vector2 WindowSize);

	/** Sets proportion console is using */
	void SetWindowProportion(Vector2 InWindowProportion) { ConsoleWindowProportion = InWindowProportion; }

	/**
	* Receives game event
	*
	* @param Event - Game event to act on
	*/
	void OnGameEvent(const FGameEvent& Event);

private:
	/** Hides achievement definitions */
	void HideAchievementDefinitions();

	/** Achievement definitions have been received */
	void OnAchievementDefinitionsReceived();

	/** Player achievements have been received */
	void OnPlayerAchievementsReceived(FProductUserId UserId);

	/** Player achievements have been unlocked */
	void OnPlayerAchievementsUnlocked(FProductUserId UserId);

	void ClearDefinitionsList();

	void ShowDefinitionsList();

	void HideDefinitionsList();

	void OnDefinitionNamesEntrySelected(size_t Index);

	void ClearCurrentSelection();

	void UpdateInfoList();

	void UpdateIcons();

	void UpdateIcon(std::shared_ptr<FSpriteWidget>& IconWidget, const bool bLocked, std::shared_ptr<FTextLabelWidget> LabelWidget);

	void UnlockSelectedAchievement();

	void StatsIngest();

	void UpdateStatsIngestButtonState();

	void SetTestInfo();

	void ClearCurrentDefinition();

	void InvalidateDefinitionIcons();

	void UpdateDefaultLanugage();

	void SetInfoList(std::shared_ptr<FAchievementsDefinitionData> Def);

	/** Header label */
	std::shared_ptr<FTextLabelWidget> HeaderLabel;

	/** Background Image */
	std::shared_ptr<FSpriteWidget> BackgroundImage;

	/** Language label */
	std::shared_ptr<FTextLabelWidget> LanguageLabel;

	/** Unlock Achievement button */
	std::shared_ptr<FButtonWidget> UnlockAchievementButton;

	/** Update Player Achievements button */
	std::shared_ptr<FButtonWidget> UpdatePlayerAchievementsButton;

	/** Stats ingest label */
	std::shared_ptr<FTextLabelWidget> StatsIngestLabel;

	/** Stats ingest name */
	std::shared_ptr<FTextFieldWidget> StatsIngestNameField;

	/** Stats ingest amount */
	std::shared_ptr<FTextFieldWidget> StatsIngestAmountField;

	/** Stats Ingest button */
	std::shared_ptr<FButtonWidget> StatsIngestButton;

	/** Achievement Icon (Locked) */
	std::shared_ptr<FSpriteWidget> LockedIcon;

	/** Achievement Icon (Unlocked) */
	std::shared_ptr<FSpriteWidget> UnlockedIcon;

	/** Label that output icon status when there is no icon to show (e.g. it's being loaded) */
	std::shared_ptr<FTextLabelWidget> LockedIconStateLabel;

	/** Label that output icon status when there is no icon to show (e.g. it's being loaded) */
	std::shared_ptr<FTextLabelWidget> UnlockedIconStateLabel;

	/** List of achievement definition name entries */
	std::shared_ptr<FAchievementsDefinitionNamesList> AchievementsDefinitionNames;

	/** List of achievement definitions entries */
	std::shared_ptr<FAchievementsDefinitionsList> AchievementsDefinitionsList;

	/** List of achievement definition names */
	std::vector<std::wstring> DefinitionNamesList;

	/** List of achievement definition info */
	std::vector<std::wstring> DefinitionsList;

	/** Currently selected entry */
	std::wstring CurrentSelection;

	/** Currently selected entry index */
	int CurrentSelectionIndex;

	/** Part of window that console is taking */
	Vector2 ConsoleWindowProportion;

	/** Currently selected definition */
	std::shared_ptr<FAchievementsDefinitionData> CurrentDefinition;

	/** Languages available to display info */
	std::vector<std::wstring> AvailableLanguages;

	/** Current Language Index */
	int CurrentLanguageIndex;

	/** Counter to update stats ingest button */
	int UpdateStatsIngestButtonCounter;
};
