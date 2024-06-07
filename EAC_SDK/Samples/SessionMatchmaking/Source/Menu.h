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
class FSessionMatchmakingDialog;
class FSessionInviteReceivedDialog;
class FNewSessionDialog;
class FPopupDialog;
class FRequestToJoinSessionReceivedDialog;

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
	 * Creates the session matchmaking dialog
	 */
	void CreateSessionMatchmakingDialog();

	/**
	 * Creates the auth dialogs
	 */
	virtual void CreateAuthDialogs() override;

	/** 
	* Creates and hides the popup dialog with session invite info.
	*/
	void CreateSessionInviteDialog();

	/**
	* Creates and hides the popup dialog for session creation.
	*/
	void CreateNewSessionDialog();

	/**
	* Creates and hides the popup dialog for request to join session
	*/
	void CreateRequestToJoinSessionDialog();

	/**
	 * Updates friends dialog to match session matchmaking dialog. 
	 */
	void UpdateFriendsDialogTransform(const Vector2 WindowSize);

	/** Session Matchmaking Dialog */
	std::shared_ptr<FSessionMatchmakingDialog> SessionMatchmakingDialog;

	/** Session invite received dialog */
	std::shared_ptr<FSessionInviteReceivedDialog> SessionInviteDialog;

	/** Dialog with session settings */
	std::shared_ptr<FNewSessionDialog> NewSessionDialog;

	/** Request to Join Session received dialog */
	std::shared_ptr<FRequestToJoinSessionReceivedDialog> RequestToJoinSessionDialog;
};
