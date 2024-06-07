// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Dialog.h"
#include "Font.h"
#include "ListView.h"
#include "SessionMatchmaking.h"
#include "TableView.h"

/**
 * Forward declarations
 */
class FGameEvent;
class FSpriteWidget;
class FCheckboxWidget;
class FNewSessionDialog;

/**
 * Session Matchmaking dialog
 */
class FSessionMatchmakingDialog : public FDialog
{
public:
	/**
	 * Constructor
	 */
	FSessionMatchmakingDialog(Vector2 DialogPos,
		Vector2 DialogSize,
		UILayer DialogLayer,
		FontPtr DialogNormalFont,
		FontPtr DialogSmallFont,
		FontPtr DialogTinyFont);

	/**
	 * Destructor
	 */
	virtual ~FSessionMatchmakingDialog() {};

	/**
	* IGfxComponent Overrides
	*/
	virtual void Update() override;
	virtual void Create() override;

	/** IWidget */
	virtual void SetPosition(Vector2 Pos) override;
	virtual void SetSize(Vector2 NewSize) override;

	/**
	* Receives game event
	*
	* @param Event - Game event to act on
	*/
	void OnGameEvent(const FGameEvent& Event);

	/** 
	 * Called when session is selected.
	 */
	void SetSelectedSession(const std::wstring& SessionName, const std::string& SessionId, bool bPresenceSession);
	const std::wstring& GetSelectedSession() const { return SelectedSessionName; }

private:
	void SearchSession(const std::wstring& Pattern);
	void StopSearch();

	/** Background Image */
	std::shared_ptr<FSpriteWidget> BackgroundImage;

	/** Title label */
	std::shared_ptr<FTextLabelWidget> TitleLabel;

	/** Create session button */
	std::shared_ptr<FButtonWidget> CreateSessionButton;

	/** Search by level name input */
	std::shared_ptr<FTextFieldWidget> SearchByLevelNameField;

	/** Small search button within the search input field */
	std::shared_ptr<FButtonWidget> SearchButton;

	/** Small cancel search button within the search input field */
	std::shared_ptr<FButtonWidget> CancelSearchButton;

	/** Table view with the list of sessions. */
	std::shared_ptr<FSessionsTableView> SessionsTable;

	/** Name of session that is currently selected */
	std::wstring SelectedSessionName;

	bool bShowingSearchResults = false;
};
