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
 * Request To Join Session Received dialog (simple popup to accept or decline the request to join)
 */
class FRequestToJoinSessionReceivedDialog : public FDialog
{
public:
	/**
	 * Constructor
	 */
	FRequestToJoinSessionReceivedDialog(Vector2 InPos,
		Vector2 InSize,
		UILayer InLayer,
		FontPtr InNormalFont,
		FontPtr InSmallFont);

	/**
	 * Destructor
	 */
	virtual ~FRequestToJoinSessionReceivedDialog() {}

	/** IWidget */
	virtual void SetPosition(Vector2 Pos) override;
	void OnEscapePressed() override;

	void SetFriendInfo(const std::wstring& InFriendName, const FProductUserId& InFriendId);

private:
	/** The name of the friend who requested to join a session. */
	std::wstring FriendName;

	/** The ID of the friend who requested to join a session. */
	FProductUserId FriendId;

	/** Background */
	WidgetPtr Background;

	/** Message label */
	std::shared_ptr<FTextLabelWidget> MessageLabel;

	/** Friend name label */
	std::shared_ptr<FTextLabelWidget> FriendNameLabel;

	/** Accept Request To Join button */
	std::shared_ptr<FButtonWidget> AcceptRequestToJoinButton;

	/** Decline Request to Join button */
	std::shared_ptr<FButtonWidget> DeclineRequestToJoinButton;
};
