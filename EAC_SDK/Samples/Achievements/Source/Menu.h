// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BaseMenu.h"

/**
* Forward declarations
*/
class FNotificationDialog;
class FAchievementsDefinitionsDialog;

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
	virtual void UpdateLayout(int Width, int Height) override;

	/**
	* Receives game event
	*
	* @param Event - Game event to act on
	*/
	virtual void OnGameEvent(const FGameEvent& Event) override;

private:

	/**
	 * Creates the notification dialog
	 */
	void CreateNotificationDialog();

	/**
	 * Creates achievements definitions dialog
	 */
	void CreateAchievementsDefinitionsDialog();

	/** Notification Dialog */
	std::shared_ptr<FNotificationDialog> NotificationDialog;

	/** Achievements Definitions Dialog dialog */
	std::shared_ptr<FAchievementsDefinitionsDialog> AchievementsDefinitionsDialog;
};
