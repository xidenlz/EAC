// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BaseMenu.h"

/**
* Forward declarations
*/
class FUIEvent;
class FConsole;
class FGameEvent;
class FFont;
class FConsoleDialog;
class FFriendsDialog;
class FExitDialog;
class FAuthDialogs;
class FSpriteWidget;
class FTextLabelWidget;
class FModsDialog;
class FModsTableDialog;
class FPopupDialog;

/**
* In-Game Menu
*/
class FMenu : public FBaseMenu
{
public:
	/**
	* Constructor
	*/
	FMenu(std::weak_ptr<FConsole> console) noexcept(false);

	/**
	* No copying or copy assignment allowed for this class.
	*/
	FMenu(FMenu const&) = delete;
	FMenu& operator=(FMenu const&) = delete;

	/**
	* Destructor
	*/
	virtual ~FMenu() {};

	/**
	* IGfxComponent Overrides
	*/
	virtual void Create() override;
	virtual void Release() override;

	/**
	* Updates layout of elements
	*/
	void UpdateLayout(int Width, int Height) override;

	/**
	* Receives game event
	*
	* @param Event - Game event to act on
	*/
	void OnGameEvent(const FGameEvent& Event) override;

protected:
	/** 
	 * Create auth dialogs
	 */
	virtual void CreateAuthDialogs() override;

	/**
	 * Creates the Mods table dialog
	 */
	void CreateModsDialog();

	/**
	 * Creates the Mods dialog
	 */
	void CreateModsTableDialog();

	/**
	* Updates Mods
	*/
	void UpdateMods();

	/** Mods Dialog */
	std::shared_ptr<FModsDialog> ModsDialog;

	/** ModsTable Dialog */
	std::shared_ptr<FModsTableDialog> ModsTableDialog;
};
