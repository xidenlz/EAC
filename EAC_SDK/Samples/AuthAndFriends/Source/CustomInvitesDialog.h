// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Dialog.h"
#include "Font.h"
/**
 * Forward declarations
 */
class FTextEditorWidget;
class FSpriteWidget;
class FTextLabelWidget;
class FDropDownList;
class FButtonWidget;
class FCustomInviteSendDialog;
class FCustomInviteReceivedDialog;
class FBaseMenu;
class FMenu;
struct FFriendData;

/**
 * Custom Invites dialog
 */
class FCustomInvitesDialog : public FDialog
{
public:
	/**
	 * Constructor
	 */
	FCustomInvitesDialog(Vector2 DialogPos,
		Vector2 DialogSize,
		UILayer DialogLayer,
		FontPtr DialogNormalFont,
		FontPtr DialogSmallFont,
		FontPtr DialogTinyFont);

	/**
	 * Destructor
	 */
	virtual ~FCustomInvitesDialog() {};

	/** IWidget */
	virtual void SetPosition(Vector2 Pos) override;
	virtual void SetSize(Vector2 NewSize) override;
	virtual void OnEscapePressed() override;

	/**
	* Receives game event
	*
	* @param Event - Game event to act on
	*/
	void OnGameEvent(const FGameEvent& Event);

	/**
	 * Assigns the Outer Menu (used to show/hide dialogs)
	 */
	void SetOuterMenu(std::weak_ptr<FMenu> InOuterMenu);

private:

	/** Outer menu (used to show/hide dialogs) */
	std::weak_ptr<FMenu> WeakOuterMenu;

	/** Background Image */
	std::shared_ptr<FSpriteWidget> BackgroundImage;

	/** Title label */
	std::shared_ptr<FTextLabelWidget> TitleLabel;

	/** Send Invite button (opens Send Invite Dialog) */
	std::shared_ptr<FButtonWidget> OpenSendInviteDialogButton;

	/** Send Custom Invite Dialog */
	std::shared_ptr<FCustomInviteSendDialog> CustomInviteSendDialog;

	/** Custom Invite Received Dialog */
	std::shared_ptr<FCustomInviteReceivedDialog> CustomInviteReceivedDialog;
};
