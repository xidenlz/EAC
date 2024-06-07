// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Dialog.h"
#include "Font.h"
#include "ListView.h"
#include "TableView.h"
#include "Voice.h"

/**
 * Forward declarations
 */
class FGameEvent;
class FSpriteWidget;

/**
 * Voice dialog
 */
class FVoiceDialog : public FDialog
{
public:
	/**
	 * Constructor
	 */
	FVoiceDialog(Vector2 DialogPos,
		Vector2 DialogSize,
		UILayer DialogLayer,
		FontPtr DialogNormalFont,
		FontPtr DialogSmallFont,
		FontPtr DialogTinyFont);

	/**
	 * Destructor
	 */
	virtual ~FVoiceDialog() {};

	/**
	* IGfxComponent Overrides
	*/
	virtual void Update() override;
	virtual void Create() override;

	/** IWidget */
	virtual void SetPosition(Vector2 Pos) override;
	virtual void SetSize(Vector2 NewSize) override;
	
	virtual void Clear();

	/**
	* Receives game event
	*
	* @param Event - Game event to act on
	*/
	void OnGameEvent(const FGameEvent& Event);

private:
	/** Updates table with list of members in the room */
	void UpdateMemberListTable();

	/** Background Image */
	std::shared_ptr<FSpriteWidget> BackgroundImage;

	/** Title label */
	std::shared_ptr<FTextLabelWidget> TitleLabel;

	/** Room name text */
	std::shared_ptr<FTextViewWidget> RoomNameText;

	/** Join room button */
	std::shared_ptr<FButtonWidget> JoinRoomButton;

	/** Leave room button */
	std::shared_ptr<FButtonWidget> LeaveRoomButton;

	/** Room output volume label */
	std::shared_ptr<FTextLabelWidget> OutputVolumeLabel;

	/** Room output volume*/
	std::shared_ptr<FTextFieldWidget> OutputVolumeText;

	/** Room participant's output volume label */
	std::shared_ptr<FTextLabelWidget> OutputParticipantVolumeLabel;

	/** Room participant's output volume*/
	std::shared_ptr<FTextFieldWidget> OutputParticipantVolumeText;

	/** List of members */
	using FVoiceRoomMembersListWidget = FVoiceRoomMemberTableView;
	std::shared_ptr<FVoiceRoomMembersListWidget> VoiceRoomMembersList;
};
