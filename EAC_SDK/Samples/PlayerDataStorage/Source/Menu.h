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
class FPlayerDataStorageDialog;
class FTransferProgressDialog;
class FNewFileDialog;
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
	 * Create Auth dialogs
	 */
	virtual void CreateAuthDialogs() override;

	/**
	 * Creates the player data storage dialog
	 */
	void CreatePlayerDataStorageDialog();

	/** 
	 * Creates the file transfer dialog
	 */
	void CreateFileTransferDialog();

	/** 
	 * Creates the create new file dialog
	 */
	void CreateNewFileDialog();

	/**
	* Updates player data storage
	*/
	void UpdatePlayerDataStorage();

	/** Player Data Storage Dialog */
	std::shared_ptr<FPlayerDataStorageDialog> PlayerDataStorageDialog;

	/** File transfer progress dialog */
	std::shared_ptr<FTransferProgressDialog> TransferDialog;

	/** New file dialog */
	std::shared_ptr<FNewFileDialog> NewFileDialog;
};
