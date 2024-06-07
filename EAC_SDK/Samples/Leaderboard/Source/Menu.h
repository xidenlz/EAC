// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BaseMenu.h"

/**
* Forward declarations
*/
class FLeaderboardDialog;

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

protected:
	/** 
	 * Create auth dialogs
	 */
	virtual void CreateAuthDialogs() override;

	/**
	 * Creates achievements definitions dialog
	 */
	void CreateLeaderboardDialog();

	/** 
	 * Updates leaderboard dialog
	 */
	void UpdateLeaderboardDialog();

	/** Leaderboard Dialog */
	std::shared_ptr<FLeaderboardDialog> LeaderboardDialog;
};