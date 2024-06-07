// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BaseMenu.h"

/**
* Forward declarations
*/
class FLobbiesDialog;
class FLobbyInviteReceivedDialog;
class FNewLobbyDialog;

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
	* Event Callback
	*
	* @param Event - UI event
	*/
	virtual void OnUIEvent(const FUIEvent& Event) override;

	/**
	* Receives game event
	*
	* @param Event - Game event to act on
	*/
	virtual void OnGameEvent(const FGameEvent& Event) override;

private:
	/**
	 * Creates the lobbies dialog
	 */
	void CreateLobbiesDialog();

	/**
	* Creates and hides the popup dialog for lobby creation.
	*/
	void CreateNewLobbyDialog();

	/**
	* Creates and hides the popup dialog for lobby invite.
	*/
	void CreateLobbyInviteReceivedDialog();

	/**
	 * Updates friends dialog to match lobbies dialog. 
	 */
	void UpdateFriendsDialogTransform(const Vector2 WindowSize);

	/** Lobbies Dialog */
	std::shared_ptr<FLobbiesDialog> LobbiesDialog;

	/** Dialog with lobby settings */
	std::shared_ptr<FNewLobbyDialog> NewLobbyDialog;

	/** Popup dialog when user is being invited to lobby */
	std::shared_ptr<FLobbyInviteReceivedDialog> LobbyInviteReceivedDialog;
};
