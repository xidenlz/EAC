// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Dialog.h"
#include "Font.h"
#include "ListView.h"
#include "SelectableStringViewListEntry.h"
#include "TableView.h"

/**
 * Forward declarations
 */
class FGameEvent;
class FSpriteWidget;
class FTextEditorWidget;

/**
 * Player Data Storage dialog
 */
class FLeaderboardDialog : public FDialog
{
public:
	/**
	 * Constructor
	 */
	FLeaderboardDialog(Vector2 DialogPos,
		Vector2 DialogSize,
		UILayer DialogLayer,
		FontPtr DialogNormalFont,
		FontPtr DialogSmallFont,
		FontPtr DialogTinyFont);

	/**
	 * Destructor
	 */
	virtual ~FLeaderboardDialog() {};

	/**
	* IGfxComponent Overrides
	*/
	virtual void Update() override;
	virtual void Create() override;

	/** IWidget */
	virtual void SetPosition(Vector2 Pos) override;
	virtual void SetSize(Vector2 NewSize) override;
	void SetWindowSize(Vector2 WindowSize);
	void SetWindowProportion(Vector2 InWindowProportion) { ConsoleWindowProportion = InWindowProportion; }


	/**
	* Receives game event
	*
	* @param Event - Game event to act on
	*/
	void OnGameEvent(const FGameEvent& Event);

	void OnDefinitionEntrySelected(size_t Index);
	void ClearDataList();
	void ShowUI();
	void HideUI();
	void ClearCurrentSelection();
	void UpdateUserInfo();
	void OnQueryInitiated();
	void QueryFriends();
	void QueryGlobalRankings();
	void RepeatLastQuery();

	// Test
	void ShowTestFriendsData();
	void ShowTestGlobalData();

private:
	/** Header label */
	std::shared_ptr<FTextLabelWidget> HeaderLabel;

	/** Background Image */
	std::shared_ptr<FSpriteWidget> BackgroundImage;

	/** List of leaderboard definitions to pick from */
	using FDefinitionsList = FListViewWidget<std::wstring, FSelectableStringViewListEntry>;
	std::shared_ptr<FDefinitionsList> DefinitionsList;

	/** Stats label */
	std::shared_ptr<FTextLabelWidget> StatsTableLabel;

	/** Table with stats */
	std::shared_ptr<FTableViewWidget> StatsTable;

	/** Background Image */
	std::shared_ptr<FSpriteWidget> StatsBackground;

	/** Button to update stats for selected definition */
	std::shared_ptr<FButtonWidget> UpdateSelectedButton;

	/** Button to update friends' ranks */
	std::shared_ptr<FButtonWidget> UpdateFriendRanksButton;

	/** Button to refresh current selection */
	std::shared_ptr<FButtonWidget> RefreshButton;

	/** Available definitions */
	std::vector<std::wstring> Definitions;

	/** Currently selected definition */
	std::wstring CurrentSelection;

	/** Part of window that console is taking */
	Vector2 ConsoleWindowProportion;

	/** Are we currently waiting for friends update */
	bool bWaitingForFriendsUpdate = false;

	/** What was the selection for last query */
	std::wstring LastSelection;

	/** Were we querying for friends last time */
	bool bLastQueryWasFriends = false;
};
