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
class FStoreDialog;
class FExitDialog;
class FNotificationDialog;
class FAuthDialogs;
class FSpriteWidget;
class FTextLabelWidget;
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
	 * Creates the store dialog
	 */
	void CreateStoreDialog();

	/**
	 * Creates the auth dialogs
	 */
	virtual void CreateAuthDialogs() override;

	/**
	 * Creates the notification dialog
	 */
	void CreateNotificationDialog();

	/**
	* Updates store info
	*/
	void UpdateStore();

	/** Store Dialog */
	std::shared_ptr<FStoreDialog> StoreDialog;

	/** Notification Dialog */
	std::shared_ptr<FNotificationDialog> NotificationDialog;
};
