// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "Game.h"
#include "Menu.h"
#include "Sprite.h"
#include "TextLabel.h"
#include "Checkbox.h"
#include "Button.h"
#include "CustomInviteReceivedDialog.h"
#include "StringUtils.h"
#include "TextEditor.h"
#include "GameEvent.h"

FCustomInviteReceivedDialog::FCustomInviteReceivedDialog(Vector2 InPos,
						 Vector2 InSize,
						 UILayer InLayer,
						 FontPtr InNormalFont,
						 FontPtr InSmallFont) :
	FDialog(InPos, InSize, InLayer)
{
	Vector2 DummyVector{};
	Background = std::make_shared<FSpriteWidget>(
		DummyVector,
		DummyVector,
		InLayer,
		L"",
		Color::Black);
	AddWidget(Background);

	FromNameLabel = std::make_shared<FTextLabelWidget>(
		DummyVector,
		DummyVector,
		InLayer - 1,
		L"Custom Invite received from: ",
		L"");
	FromNameLabel->SetFont(InNormalFont);
	AddWidget(FromNameLabel);

	FromNameValue = std::make_shared<FTextLabelWidget>(
		DummyVector,
		DummyVector,
		InLayer - 1,
		L"xxxxxxxx",
		L"");
	FromNameValue->SetFont(InNormalFont);
	AddWidget(FromNameValue);

	PayloadLabel = std::make_shared<FTextLabelWidget>(
		DummyVector,
		DummyVector,
		InLayer - 1,
		L"Payload Contents:",
		L"");
	PayloadLabel->SetFont(InNormalFont);
	AddWidget(PayloadLabel);

	PayloadValue = std::make_shared<FTextEditorWidget>(
		DummyVector,
		DummyVector,
		InLayer - 1,
		L"yyyyyyy",
		L"Assets/textfield.dds",
		InNormalFont,
		FColor(0.5f, 0.5f, 0.5f, 1.f),
		FColor(1.f, 1.f, 1.f, 1.f)
		);
	PayloadValue->SetEditingEnabled(false);
	AddWidget(PayloadValue);

	AcceptInviteButton = std::make_shared<FButtonWidget>(
		DummyVector,
		DummyVector,
		InLayer - 1,
		L"Accept",
		assets::DefaultButtonAssets,
		InSmallFont,
		Color::UIButtonBlue);
	AcceptInviteButton->SetOnPressedCallback([this]()
	{
		Hide();
		FGame::Get().GetCustomInvites()->AcceptLatestInvite();
	});
	AcceptInviteButton->SetBackgroundColors(assets::DefaultButtonColors);
	AddWidget(AcceptInviteButton);

	DeclineInviteButton = std::make_shared<FButtonWidget>(
		DummyVector,
		DummyVector,
		InLayer - 1,
		L"Decline",
		assets::DefaultButtonAssets,
		InSmallFont,
		Color::UIButtonBlue);
	DeclineInviteButton->SetOnPressedCallback([this]()
	{
		Hide();
		FGame::Get().GetCustomInvites()->RejectLatestInvite();
	});
	DeclineInviteButton->SetBackgroundColors(assets::DefaultButtonColors);
	AddWidget(DeclineInviteButton);
}

void FCustomInviteReceivedDialog::SetPosition(Vector2 NewPosition)
{
 	IWidget::SetPosition(NewPosition);

	Background->SetPosition(NewPosition);
	FromNameLabel->SetPosition(Vector2(NewPosition.x, NewPosition.y + 25.0f));
	FromNameValue->SetPosition(Vector2(FromNameLabel->GetPosition().x + 250.f, FromNameLabel->GetPosition().y));
	PayloadLabel->SetPosition(Vector2(FromNameLabel->GetPosition().x, FromNameLabel->GetPosition().y + FromNameLabel->GetSize().y));
	PayloadValue->SetPosition(Vector2(FromNameLabel->GetPosition().x + 250.f, PayloadLabel->GetPosition().y));
	AcceptInviteButton->SetPosition(Vector2(FromNameLabel->GetPosition().x + 5.f, PayloadValue->GetPosition().y + PayloadValue->GetSize().y - 5.f));
	DeclineInviteButton->SetPosition(Vector2(AcceptInviteButton->GetPosition().x + AcceptInviteButton->GetSize().x + 10.f, AcceptInviteButton->GetPosition().y));
}

void FCustomInviteReceivedDialog::SetSize(Vector2 NewSize)
{
	IWidget::SetSize(NewSize);

	Background->SetSize(NewSize);
	FromNameLabel->SetSize(Vector2(150.f, 25.f));
	FromNameValue->SetSize(Vector2(150.f, 25.f));
	PayloadLabel->SetSize(Vector2(150.f, 25.f));
	PayloadValue->SetSize(Vector2(300.f, 80.f));
	AcceptInviteButton->SetSize(Vector2(100.f, 30.f));
	DeclineInviteButton->SetSize(Vector2(100.f, 30.f));
}

void FCustomInviteReceivedDialog::OnEscapePressed()
{
	FGame::Get().GetCustomInvites()->RejectLatestInvite();
	Hide();
}

void FCustomInviteReceivedDialog::OnGameEvent(const FGameEvent& Event)
{
	if (Event.GetType() == EGameEventType::CustomInviteReceived)
	{
		FromNameValue->SetText(FGame::Get().GetCustomInvites()->LatestReceivedFromId.ToString());
		PayloadValue->SetText(FGame::Get().GetCustomInvites()->LatestReceivedPayload);
	}
	else if (Event.GetType() == EGameEventType::CustomInviteDeclined)
	{
		FromNameValue->ClearText();
		PayloadValue->Clear();
	}
}
