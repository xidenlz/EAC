// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Dialog.h"
#include "Font.h"
#include "TableView.h"

/**
 * Forward declarations
 */
class FGameEvent;
class FSpriteWidget;

/**
 * Mods Dialog
 */
class FModsDialog : public FDialog
{
public:
	/**
	 * Constructor
	 */
	FModsDialog(Vector2 DialogPos,
		Vector2 DialogSize,
		UILayer DialogLayer,
		FontPtr DialogNormalFont,
		FontPtr DialogSmallFont,
		FontPtr DialogTinyFont);

	/**
	 * Destructor
	 */
	virtual ~FModsDialog() {};

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

	/**
	 * Handle user login
	 */
	void OnLoggedIn(FEpicAccountId UserId);

	/**
	 * Handle user logout
	 */
	void OnLoggedOut();

private:
	/** Header label */
	std::shared_ptr<FTextLabelWidget> HeaderLabel;

	/** Background Image */
	std::shared_ptr<FSpriteWidget> BackgroundImage;

	/** Part of window that console is taking */
	Vector2 ConsoleWindowProportion;

	/** Enumerate all mods */
	std::shared_ptr<FButtonWidget> EnumerateAllModsBtn;
	/** Enumerate all Installed mods */
	std::shared_ptr<FButtonWidget> EnumerateInstalledModsBtn;

	/** Id for current user. */
	FEpicAccountId CurrentUserId;
};
