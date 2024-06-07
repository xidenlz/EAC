// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Dialog.h"
#include "Font.h"
#include "Friends.h"

/**
 * Forward declarations
 */
class FTextLabelWidget;
class FButtonWidget;
class FTextEditorWidget;
class FDropDownList;
class FSpriteWidget;

/**
 * Custom Invite Send dialog (simple popup to set payload or send invitation)
 */
class FCustomInviteSendDialog : public FDialog
{
public:
	/**
	 * Constructor
	 */
	FCustomInviteSendDialog(Vector2 InPos,
		Vector2 InSize,
		UILayer InLayer,
		FontPtr InNormalFont,
		FontPtr InSmallFont);

	/** FDialog */
	virtual void Create() override;
	virtual void Update() override;
	virtual void Release() override;
	virtual void OnUIEvent(const FUIEvent& Event) override;


	/**
	 * Destructor
	 */
	virtual ~FCustomInviteSendDialog() {}

	/** IWidget */
	virtual void SetSize(Vector2 NewSize) override;
	virtual void SetPosition(Vector2 Pos) override;
	virtual void OnEscapePressed() override;

	void SendInviteToFriend(const std::wstring& FriendName);
	void SendInviteToFriend(FProductUserId FriendId);

	void HandleFriendStatusUpdated(const std::vector<FFriendData>& Friends);

	void RebuildFriendsList(const std::vector<FFriendData>& Friends);

	/* Set the current payload for outgoing custom invites */
	EOS_EResult SetOutgoingCustomInvitePayload(const std::wstring& Payload);

	/* Send the current payload to a specific user (by name) */
	void SendPayloadToFriend(const std::wstring& FriendName);

private:
	/** Synchronized to the FFriends Dirty Counter, if numbers are different an update has occurred */
	uint64_t FriendsDirtyCounter = 0;

	/** Background Image */
	std::shared_ptr<FSpriteWidget> BackgroundImage;

	/** Current Payload label */
	std::shared_ptr<FTextLabelWidget> CurrentPayloadLabel;

	/** Current Payload text */
	std::shared_ptr<FTextEditorWidget> CurrentPayloadText;

	/** Current Payload display/edit field */
	std::shared_ptr<FTextEditorWidget> PayloadTextEditor;

	/** Set Payload button */
	std::shared_ptr<FButtonWidget> SetPayloadButton;

	/** Search by level name input */
	std::shared_ptr<FTextLabelWidget> SendInviteLabel;

	/** Drop-down list of invite targets */
	std::shared_ptr<FDropDownList> InviteTargetDropdown;

	/** Send Invite button */
	std::shared_ptr<FButtonWidget> SendInviteButton;
};
