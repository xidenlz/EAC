// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Dialog.h"
#include "Font.h"
#include "ListView.h"
#include "TableView.h"
#include "Lobbies.h"

/**
 * Forward declarations
 */
class FGameEvent;
class FSpriteWidget;
class FDropDownList;

/**
 * Lobbies dialog
 */
class FLobbiesDialog : public FDialog
{
public:
	/**
	 * Constructor
	 */
	FLobbiesDialog(Vector2 DialogPos,
		Vector2 DialogSize,
		UILayer DialogLayer,
		FontPtr DialogNormalFont,
		FontPtr DialogSmallFont,
		FontPtr DialogTinyFont);

	/**
	 * Destructor
	 */
	virtual ~FLobbiesDialog() {};

	/**
	* IGfxComponent Overrides
	*/
	virtual void Update() override;
	virtual void Create() override;

	/** IWidget */
	virtual void SetPosition(Vector2 Pos) override;
	virtual void SetSize(Vector2 NewSize) override;
	
	virtual void Clear();

	/**
	* Receives game event
	*
	* @param Event - Game event to act on
	*/
	void OnGameEvent(const FGameEvent& Event);

	/* Search for lobbies by level name */
	void SearchLobbyByLevel(const std::wstring& LevelName);

	/* Search for lobbies by bucket id */
	void SearchLobbyByBucketId(const std::wstring& BucketId);

	/* Search for lobby depending on UI widget state. */
	void SearchLobby(const std::wstring& SearchString);

	/* Cancel search */
	void StopSearch();

private:
	/** Background Image */
	std::shared_ptr<FSpriteWidget> BackgroundImage;

	/** Title label */
	std::shared_ptr<FTextLabelWidget> TitleLabel;

	/** Create lobby button */
	std::shared_ptr<FButtonWidget> CreateLobbyButton;

	/** Leave lobby button */
	std::shared_ptr<FButtonWidget> LeaveLobbyButton;

	/** Modify lobby button */
	std::shared_ptr<FButtonWidget> ModifyLobbyButton;

	/** Search type drop-down list */
	std::shared_ptr<FDropDownList> SearchTypeDropDown;

	/** Search by level name input */
	std::shared_ptr<FTextFieldWidget> SearchField;

	/** Small search button within the search input field */
	std::shared_ptr<FButtonWidget> SearchButton;

	/** Small cancel search button within the search input field */
	std::shared_ptr<FButtonWidget> CancelSearchButton;

	//Current lobby info
	/** Name of lobby's owner  */
	std::shared_ptr<FTextLabelWidget> OwnerLabel;
	/** Is public label */
	std::shared_ptr<FTextLabelWidget> IsPublicLabel;
	/** Current level label */
	std::shared_ptr<FTextLabelWidget> LevelNameLabel;
	/** List of members */
	using FLobbyMembersListWidget = FLobbyMemberTableView;
	std::shared_ptr<FLobbyMembersListWidget> LobbyMembersList;

	//Search results
	using FSearchResultsLobbyTableWidget = FLobbySearchResultTableView;
	std::shared_ptr<FSearchResultsLobbyTableWidget> ResultLobbiesTable;
};
