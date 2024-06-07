// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Dialog.h"
#include "Font.h"
#include "Lobbies.h"

/**
 * Forward declarations
 */
class FTextLabelWidget;
class FCheckboxWidget;
class FButtonWidget;

/**
 * Lobby Invite Received dialog (simple popup to accept or decline the invitation)
 */
class FLobbyInviteReceivedDialog : public FDialog
{
public:
	/**
	 * Constructor
	 */
	FLobbyInviteReceivedDialog(Vector2 InPos,
		Vector2 InSize,
		UILayer InLayer,
		FontPtr InNormalFont,
		FontPtr InSmallFont);

	/**
	 * Destructor
	 */
	virtual ~FLobbyInviteReceivedDialog() {}

	/** IWidget */
	virtual void SetPosition(Vector2 Pos) override;
	virtual void Update() override;
	virtual void OnEscapePressed() override;

	const FLobby& GetLobby() const { return Lobby; }
	LobbyDetailsKeeper GetLobbyInfo() const { return LobbyDetails; }
	const std::string& GetLobbyInviteId() const { return LobbyInviteId; }

	void SetLobbyInvite(const FLobbyInvite* InCurrentInvite);

private:
	void RejectInvite();

	/** The name of friend who invited to lobby. */
	std::wstring FriendName;

	/** InviteId for the lobby invite to accept/reject */
	std::string LobbyInviteId;

	/** Lobby to join. Or not. */
	FLobby Lobby;

	/** Lobby details */
	LobbyDetailsKeeper LobbyDetails;

	/** Background */
	WidgetPtr Background;

	/** Main Label */
	std::shared_ptr<FTextLabelWidget> Label;

	/** Level Label */
	std::shared_ptr<FTextLabelWidget> LevelLabel;

	/** Is lobby for presence? */
	std::shared_ptr<FCheckboxWidget> PresenceCheckbox;

	/** Accept invite button */
	std::shared_ptr<FButtonWidget> AcceptInviteButton;

	/** Decline invite button */
	std::shared_ptr<FButtonWidget> DeclineInviteButton;
};
