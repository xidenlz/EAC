// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Dialog.h"
#include "Font.h"
#include "SessionMatchmaking.h"

/**
 * Forward declarations
 */
class FTextLabelWidget;
class FButtonWidget;
class FCheckboxWidget;

/**
 * Session Invite Received dialog (simple popup to accept or decline the invitation)
 */
class FSessionInviteReceivedDialog : public FDialog
{
public:
	/**
	 * Constructor
	 */
	FSessionInviteReceivedDialog(Vector2 InPos,
		Vector2 InSize,
		UILayer InLayer,
		FontPtr InNormalFont,
		FontPtr InSmallFont);

	/**
	 * Destructor
	 */
	virtual ~FSessionInviteReceivedDialog() {}

	/** IWidget */
	virtual void SetPosition(Vector2 Pos) override;
	const FSession& GetSession() const { return Session; }
	void OnEscapePressed() override;

	void SetSessionInfo(const std::wstring& InFriendName, const FSession& InSession);

private:
	/** The name of friend who invited to session. */
	std::wstring FriendName;

	/** Session to join. Or not. */
	FSession Session;

	/** Background */
	WidgetPtr Background;

	/** Message label */
	std::shared_ptr<FTextLabelWidget> MessageLabel;

	/** Friend name label */
	std::shared_ptr<FTextLabelWidget> FriendNameLabel;

	/** Level name label */
	std::shared_ptr<FTextLabelWidget> LevelNameLabel;

	/** Accept invite button */
	std::shared_ptr<FButtonWidget> AcceptInviteButton;

	/** Decline invite button */
	std::shared_ptr<FButtonWidget> DeclineInviteButton;

	/** Is session for presence? */
	std::shared_ptr<FCheckboxWidget> PresenceSessionCheckbox;
};
