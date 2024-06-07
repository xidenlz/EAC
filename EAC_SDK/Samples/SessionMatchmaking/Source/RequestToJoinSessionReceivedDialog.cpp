// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "Game.h"
#include "Menu.h"
#include "Sprite.h"
#include "TextLabel.h"
#include "Button.h"
#include "Checkbox.h"
#include "ProgressBar.h"
#include "RequestToJoinSessionReceivedDialog.h"
#include "StringUtils.h"

FRequestToJoinSessionReceivedDialog::FRequestToJoinSessionReceivedDialog(Vector2 InPos,
						 Vector2 InSize,
						 UILayer InLayer,
						 FontPtr InNormalFont,
						 FontPtr InSmallFont) :
	FDialog(InPos, InSize, InLayer)
{
	Background = std::make_shared<FSpriteWidget>(
		Position,
		InSize,
		InLayer - 1,
		L"Assets/textfield.dds",
		Color::Black);
	AddWidget(Background);

	MessageLabel = std::make_shared<FTextLabelWidget>(
		Vector2(Position.x + 30.0f, Position.y + 25.0f),
		Vector2(150.f, 30.f),
		InLayer - 1,
		L"",
		L"");
	MessageLabel->SetFont(InNormalFont);
	AddWidget(MessageLabel);

	FriendNameLabel = std::make_shared<FTextLabelWidget>(
		Vector2(Position.x + 30.0f, Position.y + 25.0f),
		Vector2(150.f, 30.f),
		InLayer - 1,
		L"",
		L"",
		FColor(1.f, 1.f, 1.f, 1.f),
		FColor(1.f, 1.f, 1.f, 1.f),
		EAlignmentType::Left);
	FriendNameLabel->SetFont(InNormalFont);
	AddWidget(FriendNameLabel);

	FColor AcceptButtonCol = FColor(0.f, 0.47f, 0.95f, 1.f);
	AcceptRequestToJoinButton = std::make_shared<FButtonWidget>(
		Position + Vector2(InSize.x / 2.0f - 105.0f, InSize.y - 45.f),
		Vector2(100.f, 30.f),
		InLayer - 1,
		L"Accept",
		assets::DefaultButtonAssets,
		InSmallFont,
		AcceptButtonCol);
	AcceptRequestToJoinButton->SetOnPressedCallback([this]()
	{
		FGame::Get().GetSessions()->AcceptRequestToJoin(FriendId);
		Hide();
	});
	AcceptRequestToJoinButton->SetBackgroundColors(assets::DefaultButtonColors);
	AddWidget(AcceptRequestToJoinButton);

	FColor DeclineButtonCol = FColor(0.f, 0.47f, 0.95f, 1.f);
	DeclineRequestToJoinButton = std::make_shared<FButtonWidget>(
		Vector2(Position.x + InSize.x / 2.0f + 5.0f, AcceptRequestToJoinButton->GetPosition().y),
		Vector2(100.f, 30.f),
		InLayer - 1,
		L"Decline",
		assets::DefaultButtonAssets,
		InSmallFont,
		DeclineButtonCol);
	DeclineRequestToJoinButton->SetOnPressedCallback([this]()
	{
		FGame::Get().GetSessions()->RejectRequestToJoin(FriendId);
		Hide();
	});
	DeclineRequestToJoinButton->SetBackgroundColors(assets::DefaultButtonColors);
	AddWidget(DeclineRequestToJoinButton);
}

void FRequestToJoinSessionReceivedDialog::SetPosition(Vector2 Pos)
{
	IWidget::SetPosition(Pos);

	Background->SetPosition(Position);
	MessageLabel->SetPosition(Position + Vector2(30.0f, 25.0f));
	FriendNameLabel->SetPosition(MessageLabel->GetPosition() + Vector2(0.0f, MessageLabel->GetSize().y + 5.0f));
	AcceptRequestToJoinButton->SetPosition(Position + Vector2(GetSize().x / 2.0f - AcceptRequestToJoinButton->GetSize().x - 5.0f, GetSize().y - 45.0f));
	DeclineRequestToJoinButton->SetPosition(Position + Vector2(GetSize().x / 2.0f + 5.0f, GetSize().y - 45.0f));
}

void FRequestToJoinSessionReceivedDialog::SetFriendInfo(const std::wstring& InFriendName, const FProductUserId& InFriendId)
{
	FriendName = InFriendName;
	FriendId = InFriendId;

	if (MessageLabel)
	{
		MessageLabel->ClearText();
		MessageLabel->SetText(std::wstring(L"Request to Join Session Received"));
	}

	if (FriendNameLabel)
	{
		FriendNameLabel->ClearText();
		if (!InFriendName.empty())
		{
			FriendNameLabel->SetText(std::wstring(L"Friend: ") + InFriendName);
		}
	}
}

void FRequestToJoinSessionReceivedDialog::OnEscapePressed()
{
	Hide();
}