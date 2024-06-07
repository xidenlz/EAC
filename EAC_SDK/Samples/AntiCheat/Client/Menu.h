// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BaseMenu.h"

/**
* Forward declarations
*/
class FAntiCheatDialog;

/**
* In-Game Menu
*/
class FMenu : public FBaseMenu
{
public:
	/**
	* Constructor
	*/
	FMenu(std::weak_ptr<FConsole> InConsole) noexcept(false);

	/**
	* No copying or copy assignment allowed for this class.
	*/
	FMenu(FMenu const&) = delete;
	FMenu& operator=(FMenu const&) = delete;

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

	/**
    * Creates the Anti-Cheat dialog
    */
	void CreateAntiCheatDialog();

	/**
	* Destructor
	*/
	virtual ~FMenu() = default;

	/**
	* IGfxComponent Overrides
	*/
	virtual void Create() override;
	virtual void Release() override;

protected:
	/**
	 * Create auth dialogs
	 */
	virtual void CreateAuthDialogs() override;

private:
	/** Anti-Cheat Dialog */
	std::shared_ptr<FAntiCheatDialog> AntiCheatDialog;
};
