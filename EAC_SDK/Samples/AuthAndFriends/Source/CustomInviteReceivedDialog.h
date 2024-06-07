// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Dialog.h"
#include "Font.h"

/**
 * Forward declarations
 */
class FTextLabelWidget;
class FCheckboxWidget;
class FButtonWidget;
class FTextEditorWidget;

/**
 * Lobby Invite Received dialog (simple popup to accept or decline the invitation)
 */
class FCustomInviteReceivedDialog : public FDialog
{
public:
	/**
	 * Constructor
	 */
	FCustomInviteReceivedDialog(Vector2 InPos,
		Vector2 InSize,
		UILayer InLayer,
		FontPtr InNormalFont,
		FontPtr InSmallFont);

	/**
	 * Destructor
	 */
	virtual ~FCustomInviteReceivedDialog() {}

	/** IWidget */
	virtual void SetPosition(Vector2 Pos) override;
	virtual void SetSize(Vector2 Pos) override;
	virtual void OnEscapePressed() override;

	/**
	* Receives game event
	*
	* @param Event - Game event to act on
	*/
	void OnGameEvent(const FGameEvent& Event);

private:

	/** The name of friend who invited to lobby. */
	std::wstring FriendName;

	/** InviteId for the custom invite to accept/reject */
	std::string CustomInvite;

	/** Background */
	WidgetPtr Background;

	/** "From" */
	std::shared_ptr<FTextLabelWidget> FromNameLabel;

	/** From */
	std::shared_ptr<FTextLabelWidget> FromNameValue;

	/** "Payload" */
	std::shared_ptr<FTextLabelWidget> PayloadLabel;

	/** Actual payload */
	std::shared_ptr<FTextEditorWidget> PayloadValue;

	/** Accept invite button */
	std::shared_ptr<FButtonWidget> AcceptInviteButton;

	/** Decline invite button */
	std::shared_ptr<FButtonWidget> DeclineInviteButton;
};
