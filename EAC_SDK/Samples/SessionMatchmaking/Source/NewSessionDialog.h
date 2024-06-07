// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Dialog.h"
#include "Font.h"
#include "SessionMatchmaking.h"

/**
 * Forward declarations
 */
class FTextLabelWidget;
class FButtonWidget;
class FDropDownList;
class FCheckboxWidget;

/**
 * Session creation dialog. User can pick parameters and create new session.
 */
class FNewSessionDialog : public FDialog
{
public:
	/**
	 * Constructor
	 */
	FNewSessionDialog(Vector2 InPos,
		Vector2 InSize,
		UILayer InLayer,
		FontPtr InNormalFont,
		FontPtr InSmallFont);

	/**
	 * Destructor
	 */
	virtual ~FNewSessionDialog();

	/** IWidget */
	virtual void SetPosition(Vector2 Pos) override;

	virtual void Create() override;

	virtual void Show() override;

	virtual void Toggle() override;

	virtual void Update() override;

	virtual void OnEscapePressed() override;

protected:
	void CreateSessionPressed();
	void UpdateValuesOnShow();
	void FinishSessionModification();

	void OnSessionLevelSelected(const std::wstring& Selection)
	{
		bSessionLevelSelected = true;
	}

	void OnSessionMaxPlayersNumSelected(const std::wstring& Selection)
	{
		bSessionMaxPlayersSelected = true;
	}

	void OnSessionRestrictedPlatformSelected(const std::wstring& Selection)
	{
		bSessionRestrictedPlatformSelected = true;
	}

private:
	/** Background */
	WidgetPtr Background;

	/** Label */
	std::shared_ptr<FTextLabelWidget> HeaderLabel;

	/** Session name text input field */
	std::shared_ptr<FTextFieldWidget> SessionNameField;

	/** Session level */
	std::shared_ptr<FDropDownList> SessionLevelDropDown;

	/** Session max players */
	std::shared_ptr<FDropDownList> MaxPlayersDropDown;

	/** Restricted Platforms */
	std::shared_ptr<FDropDownList> RestrictedPlatformsDropDown;

	/** Session join in progress flag */
	std::shared_ptr<FCheckboxWidget> JoinInProgressCheckbox;

	/** Is session for presence? */
	std::shared_ptr<FCheckboxWidget> PresenceSessionCheckbox;

	/** Is session public? */
	std::shared_ptr<FCheckboxWidget> PublicCheckbox;

	/** Are invites allowed? */
	std::shared_ptr<FCheckboxWidget> InvitesAllowedCheckbox;

	/** Create session button */
	std::shared_ptr<FButtonWidget> CreateButton;

	/** Close dialog button */
	std::shared_ptr<FButtonWidget> CloseButton;
	
	/** Has user selected a value for session Level? */
	bool bSessionLevelSelected = false;

	/** Has user selected a value for max players num? */
	bool bSessionMaxPlayersSelected = false;

	/** Has user selected a value for restricted platform? */
	bool bSessionRestrictedPlatformSelected = false;
};
