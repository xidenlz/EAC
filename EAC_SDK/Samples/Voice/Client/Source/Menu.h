// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BaseMenu.h"

/**
* Forward declarations
*/
class FVoiceDialog;
class FVoiceSetupDialog;

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
	 * Creates the voice dialog
	 */
	void CreateVoiceDialog();

	/**
	 * Creates the voice setup dialog
	 */
	void CreateVoiceSetupDialog();

	/**
	 * Updates friends dialog to match Voice dialog. 
	 */
	void UpdateFriendsDialogTransform(const Vector2 WindowSize);

	/** Voice Dialog */
	std::shared_ptr<FVoiceDialog> VoiceDialog;

	/** Voice Setup Dialog */
	std::shared_ptr<FVoiceSetupDialog> VoiceSetupDialog;
};
