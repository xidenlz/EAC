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
class FTitleStorageDialog;
class FTransferProgressDialog;
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

private:

	/**
	 * Creates the player data storage dialog
	 */
	void CreateTitleStorageDialog();

	/**
	 * Creates the auth dialogs
	 */
	virtual void CreateAuthDialogs() override;

	/** 
	 * Creates the file transfer dialog
	 */
	void CreateFileTransferDialog();

	/**
	* Updates player data storage
	*/
	void UpdateTitleStorage();

	/** Player Data Storage Dialog */
	std::shared_ptr<FTitleStorageDialog> TitleStorageDialog;

	/** File transfer progress dialog */
	std::shared_ptr<FTransferProgressDialog> TransferDialog;
};
