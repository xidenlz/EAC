// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Dialog.h"
#include "Font.h"

/**
 * Forward declarations
 */
class FTextLabelWidget;
class FButtonWidget;
class FDropDownList;
class FCheckboxWidget;
class FTextFieldWidget;

/**
 * Lobby creation dialog. User can pick parameters and create new lobby.
 * Can be used for Lobby modification via SetEditingMode(true).
 */
class FNewLobbyDialog : public FDialog
{
public:
	/**
	 * Constructor
	 */
	FNewLobbyDialog(Vector2 InPos,
		Vector2 InSize,
		UILayer InLayer,
		FontPtr InNormalFont,
		FontPtr InSmallFont);

	/**
	 * Destructor
	 */
	virtual ~FNewLobbyDialog();

	/** IWidget */
	virtual void SetPosition(Vector2 Pos) override;
	virtual void OnEscapePressed() override;

	virtual void Create() override;
	virtual void Update() override;

	void SetEditingMode(bool bValue);

protected:
	void OnCreateLobbyPressed();
	void OnModifyLobbyPressed();
	void FinishLobbyModification();

	std::vector<std::wstring> GetUsersOptionsList(int MinUsers, int MaxUsers);

private:
	/** Background */
	WidgetPtr Background;

	/** Label */
	std::shared_ptr<FTextLabelWidget> HeaderLabel;

	/** Lobby's bucket id */
	std::shared_ptr<FTextFieldWidget> BucketIdField;

	/** Lobby level */
	std::shared_ptr<FDropDownList> LobbyLevelDropDown;

	/** Lobby max players */
	std::shared_ptr<FDropDownList> MaxPlayersDropDown;

	/** Is Lobby public? */
	std::shared_ptr<FCheckboxWidget> PublicCheckbox;

	/** Is Lobby invite only? */
	std::shared_ptr<FCheckboxWidget> InvitesAllowedCheckbox;

	/** Is Lobby for presence? */
	std::shared_ptr<FCheckboxWidget> PresenceCheckbox;

	/** Is Lobby RTC Room Enabled? */
	std::shared_ptr<FCheckboxWidget> RTCRoomCheckbox;

	/** Is this dialog in editing mode modifying existing lobby (instead of creating a new lobby) */
	bool bEditingLobby = false;

	/** Create Lobby button */
	std::shared_ptr<FButtonWidget> CreateButton;

	/** Close dialog button */
	std::shared_ptr<FButtonWidget> CloseButton;
};
