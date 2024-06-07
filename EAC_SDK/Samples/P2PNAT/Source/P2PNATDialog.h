// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Dialog.h"
#include "Font.h"
#include "ListView.h"

/**
 * Forward declarations
 */
class FGameEvent;
class FSpriteWidget;

/**
 * Structure to store all data related to chat with a friend.
 */
struct FChatWithFriendData
{
	//bool defines who's line it was: yours (when true) or someone else's
	std::vector<std::pair<bool, std::wstring> > ChatLines;

	//Who we are chatting with.
	FProductUserId FriendId;

	FChatWithFriendData() = default;
	FChatWithFriendData(FProductUserId InFriendId) : FriendId(InFriendId) {}
};

/**
 * P2P NAT dialog
 */
class FP2PNATDialog : public FDialog
{
public:
	/**
	 * Constructor
	 */
	FP2PNATDialog(Vector2 DialogPos,
		Vector2 DialogSize,
		UILayer DialogLayer,
		FontPtr DialogNormalFont,
		FontPtr DialogSmallFont,
		FontPtr DialogTinyFont,
		FontPtr DialogTitleFont);

	/**
	 * Destructor
	 */
	virtual ~FP2PNATDialog() {};

	/**
	 * IGfxComponent Overrides
	 */
	virtual void Update() override;
	virtual void Create() override;

	/** IWidget */
	virtual void SetPosition(Vector2 Pos) override;
	virtual void SetSize(Vector2 NewSize) override;
	virtual void OnUIEvent(const FUIEvent& Event) override;

	void UpdateNATStatus();

	/**
	 * Receives game event
	 *
	 * @param Event - Game event to act on
	 */
	void OnGameEvent(const FGameEvent& Event);

	/** Find chat data related to particular friend. Creates and returns empty chat when no chat data found. */
	FChatWithFriendData& GetChat(FProductUserId FriendId);

	/** Who we are chatting with at the moment (returns invalid account id when no chat is active) */
	FProductUserId GetCurrentChat() const { return CurrentChat; }

	/** Close current chat */
	void CloseCurrentChat();

	/** Called when user wants to send text */
	void OnSendMessage(const std::wstring& Value);

	/** Called when a message is received from another user */
	void OnMessageReceived(const std::wstring& Message, FProductUserId FriendId);

private:
	/** Splits a multi-line message (with newline characters) into multiple lines of text */
	void SplitMessageIntoLines(const std::wstring& Message, std::vector<std::wstring>& OutLines);

	/** Header label */
	std::shared_ptr<FTextLabelWidget> HeaderLabel;

	/** Background Image */
	std::shared_ptr<FSpriteWidget> BackgroundImage;

	/** Chat text view */
	std::shared_ptr<FTextViewWidget> ChatTextView;

	/** Chat central info label */
	std::shared_ptr<FTextLabelWidget> ChatInfoLabel;

	/** Chat input field */
	std::shared_ptr<FTextFieldWidget> ChatInputField;

	/** NAT Status label */
	std::shared_ptr<FTextLabelWidget> NATStatusLabel;

	/** Close current chat button */
	std::shared_ptr<FButtonWidget> CloseCurrentChatButton;

	/** All active chats */
	std::vector<FChatWithFriendData> ChatData;

	/** Who we are chatting at the moment */
	FProductUserId CurrentChat;

	/** Do we need to refresh text in chat view? */
	bool bChatViewDirty = true;
};
