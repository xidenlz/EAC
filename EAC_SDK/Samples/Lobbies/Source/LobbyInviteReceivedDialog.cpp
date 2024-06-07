// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "Game.h"
#include "Menu.h"
#include "Sprite.h"
#include "TextLabel.h"
#include "Checkbox.h"
#include "Button.h"
#include "ProgressBar.h"
#include "LobbyInviteReceivedDialog.h"
#include "StringUtils.h"

FLobbyInviteReceivedDialog::FLobbyInviteReceivedDialog(Vector2 InPos,
						 Vector2 InSize,
						 UILayer InLayer,
						 FontPtr InNormalFont,
						 FontPtr InSmallFont) :
	FDialog(InPos, InSize, InLayer)
{
	Background = std::make_shared<FSpriteWidget>(
		Position,
		InSize,
		InLayer,
		L"Assets/textfield.dds",
		Color::Black);
	AddWidget(Background);

	Label = std::make_shared<FTextLabelWidget>(
		Vector2(Position.x + 30.0f, Position.y + 25.0f),
		Vector2(150.f, 30.f),
		InLayer - 1,
		L"Lobby invite received from: ",
		L"");
	Label->SetFont(InNormalFont);
	AddWidget(Label);

	LevelLabel = std::make_shared<FTextLabelWidget>(
		Vector2(Position.x + 30.0f, Position.y + 45.0f),
		Vector2(150.f, 30.f),
		InLayer - 1,
		L"Level: ",
		L"");
	LevelLabel->SetFont(InNormalFont);
	AddWidget(LevelLabel);

	const float CenterX = Position.x + InSize.x / 2.f;
	PresenceCheckbox = std::make_shared<FCheckboxWidget>(
		Vector2(CenterX - 75.f, LevelLabel->GetPosition().y + LevelLabel->GetSize().y + 5.f),
		Vector2(150.0f, 30.0f),
		InLayer - 1,
		L"Presence",
		L"",
		InNormalFont
		);
	AddWidget(PresenceCheckbox);

	AcceptInviteButton = std::make_shared<FButtonWidget>(
		Vector2(CenterX - 105.0f, PresenceCheckbox->GetPosition().y + PresenceCheckbox->GetSize().y + 10.f),
		Vector2(100.f, 30.f),
		InLayer - 1,
		L"Accept",
		assets::DefaultButtonAssets,
		InSmallFont,
		Color::UIButtonBlue);
	AcceptInviteButton->SetOnPressedCallback([this]()
	{
		Hide();
		const FLobby& Lobby = GetLobby();
		if (Lobby.IsValid())
		{
			FGame::Get().GetLobbies()->JoinLobby(Lobby.Id.c_str(), LobbyDetails, PresenceCheckbox->IsTicked());
		}
	});
	AcceptInviteButton->SetBackgroundColors(assets::DefaultButtonColors);
	AddWidget(AcceptInviteButton);

	DeclineInviteButton = std::make_shared<FButtonWidget>(
		AcceptInviteButton->GetPosition() + Vector2(AcceptInviteButton->GetSize().x + 10.f, 0.f),
		Vector2(100.f, 30.f),
		InLayer - 1,
		L"Decline",
		assets::DefaultButtonAssets,
		InSmallFont,
		Color::UIButtonBlue);
	DeclineInviteButton->SetOnPressedCallback([this]()
	{
		RejectInvite();
		Hide();
	});
	DeclineInviteButton->SetBackgroundColors(assets::DefaultButtonColors);
	AddWidget(DeclineInviteButton);
}

void FLobbyInviteReceivedDialog::SetPosition(Vector2 Pos)
{
	IWidget::SetPosition(Pos);

	Background->SetPosition(Position);
	Label->SetPosition(Vector2(Position.x + 30.0f, Position.y + 25.0f));
	LevelLabel->SetPosition(Vector2(Position.x + 30.0f, Position.y + 45.0f));
	PresenceCheckbox->SetPosition(Label->GetPosition() + Vector2(Label->GetSize().x / 2.0f + 5.0f, LevelLabel->GetSize().y + 10.0f));

	const float CenterX = Position.x + GetSize().x / 2.f;
	PresenceCheckbox->SetPosition(Vector2(CenterX - PresenceCheckbox->GetSize().x / 2.f, LevelLabel->GetPosition().y + LevelLabel->GetSize().y + 5.0f));
	const float AcceptSizeX = AcceptInviteButton->GetSize().x;
	AcceptInviteButton->SetPosition(Vector2(CenterX - AcceptSizeX - 5.0f, PresenceCheckbox->GetPosition().y + PresenceCheckbox->GetSize().y + 10.f));
	DeclineInviteButton->SetPosition(AcceptInviteButton->GetPosition() + Vector2(AcceptSizeX + 10.f, 0.f));
}

void FLobbyInviteReceivedDialog::Update()
{
	if (FriendName.empty())
	{
		FriendName = FGame::Get().GetLobbies()->GetInviteSenderName();

		if (!FriendName.empty())
		{
			if (Label)
			{
				Label->ClearText();
				Label->SetText(std::wstring(L"Lobby invite received from: ") + ((FriendName.empty()) ? L"?" : FriendName));
			}
		}
	}

	FDialog::Update();
}

void FLobbyInviteReceivedDialog::SetLobbyInvite(const FLobbyInvite* InCurrentInvite)
{
	FriendName = InCurrentInvite->FriendDisplayName;
	Lobby = InCurrentInvite->Lobby;
	LobbyDetails = InCurrentInvite->LobbyInfo;
	LobbyInviteId = InCurrentInvite->InviteId;

	std::wstring LevelName = L"?";
	for (FLobbyAttribute& Attr : Lobby.Attributes)
	{
		if (Attr.Key == "LEVEL")
		{
			LevelName = FStringUtils::Widen(Lobby.Attributes[0].AsString);
		}
	}

	if (Label)
	{
		Label->ClearText();
		Label->SetText(std::wstring(L"Lobby invite received from: ") + FriendName);
	}

	if (LevelLabel)
	{
		LevelLabel->ClearText();
		LevelLabel->SetText(std::wstring(L"Level: ") + LevelName);
	}
}

void FLobbyInviteReceivedDialog::RejectInvite()
{
	const FLobby& Lobby = GetLobby();
	if (Lobby.IsValid())
	{
		FGame::Get().GetLobbies()->RejectLobbyInvite(GetLobbyInviteId());
	}
}

void FLobbyInviteReceivedDialog::OnEscapePressed()
{
	RejectInvite();
	Hide();
}

