// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "Game.h"
#include "Menu.h"
#include "Sprite.h"
#include "TextLabel.h"
#include "Button.h"
#include "StringUtils.h"
#include "CustomInviteSendDialog.h"
#include "TextEditor.h"
#include "DropDownList.h"

#define CUSTOMINVITE_DIALOG_PAYLOAD_LABEL_WIDTH		250.f
#define CUSTOMINVITE_DIALOG_PAYLOAD_TEXT_WIDTH		150.f
#define CUSTOMINVITE_DIALOG_PAYLOAD_TEXT_HEIGHT		75.f
#define CUSTOMINVITE_DIALOG_PAYLOAD_TEXTENTRY_WIDTH 150.f
#define CUSTOMINVITE_DIALOG_PAYLOAD_TEXTENTRY_HEIGHT 75.f
#define CUSTOMINVITE_DIALOG_SETPAYLOAD_BUTTON_WIDTH  100.f
#define CUSTOMINVITE_DIALOG_SETPAYLOAD_BUTTON_HEIGHT 30.f
#define CUSTOMINVITE_DIALOG_FRIENDLIST_LIST_WIDTH    300.f
#define CUSTOMINVITE_DIALOG_FRIENDLIST_LIST_HEIGHT   40.f
#define CUSTOMINVITE_DIALOG_FRIENDLIST_LIST_MAXHEIGHT  200.f

FCustomInviteSendDialog::FCustomInviteSendDialog(Vector2 InPos,
						 Vector2 InSize,
						 UILayer InLayer,
						 FontPtr InNormalFont,
						 FontPtr InSmallFont) :
	FDialog(InPos, InSize, InLayer)
{
	Vector2 DummyVector{};  // the position / size values are irrelevant in constructor

	BackgroundImage = std::make_shared<FSpriteWidget>(
		DummyVector,
		DummyVector,
		InLayer,
		L"");
	AddWidget(BackgroundImage);

	// "Current Payload:"
	CurrentPayloadLabel = std::make_shared<FTextLabelWidget>(
		DummyVector,
		DummyVector,
		InLayer,
		L"Current Custom Invite Payload",
		L"",
		FColor(1.f, 1.f, 1.f, 1.f),
		FColor(1.f, 1.f, 1.f, 1.f),
		EAlignmentType::Left);
	CurrentPayloadLabel->SetFont(InNormalFont);
	AddWidget(CurrentPayloadLabel);

	PayloadTextEditor = std::make_shared<FTextEditorWidget>(
		DummyVector,
		DummyVector,
		InLayer,
		L"Enter custom invite payload...",
		L"Assets/textfield.dds",
		InSmallFont,
		FColor(0.5f, 0.5f, 0.5f, 1.f),
		FColor(1.f, 1.f, 1.f, 1.f)
		);
	PayloadTextEditor->SetBorderColor(Color::UIBorderGrey);
	PayloadTextEditor->SetBorderOffsets(Vector2(4.f, 4.f));
	AddWidget(PayloadTextEditor);

	SetPayloadButton = std::make_shared<FButtonWidget>(
		DummyVector,
		DummyVector,
		InLayer,
		L"Set Payload >>",
		assets::DefaultButtonAssets,
		InSmallFont,
		Color::UIButtonBlue,
		FColor(1.f, 1.f, 1.f, 1.f));
	SetPayloadButton->SetOnPressedCallback([this]()
	{
		SetOutgoingCustomInvitePayload(PayloadTextEditor->GetText());
	});
	AddWidget(SetPayloadButton);
	SetPayloadButton->Disable();

	// <current payload value>
	CurrentPayloadText = std::make_shared<FTextEditorWidget>(
		DummyVector,
		DummyVector,
		InLayer,
		L"",
		L"",
		InSmallFont,
		FColor(1.f, 1.f, 1.f, 1.f),
		Color::LawnGreen);
	CurrentPayloadText->SetBorderColor(Color::UIBorderGrey);
	CurrentPayloadText->SetEditingEnabled(false);
	CurrentPayloadText->SetBorderOffsets(Vector2(4.f, 4.f));
	AddWidget(CurrentPayloadText);

	InviteTargetDropdown = std::make_shared<FDropDownList>(
		DummyVector,
		DummyVector,
		Vector2(CUSTOMINVITE_DIALOG_FRIENDLIST_LIST_WIDTH, CUSTOMINVITE_DIALOG_FRIENDLIST_LIST_MAXHEIGHT),  // can't be dummy, currently there is no mutator
		InLayer,
		L"Target Friend: ",
		std::vector<std::wstring>({ L"No Friends", }),
		InNormalFont,
		EAlignmentType::Left,
		Color::UIBackgroundGrey
		);
	AddWidget(InviteTargetDropdown);

	SendInviteButton = std::make_shared<FButtonWidget>(
		DummyVector,
		DummyVector,
		InLayer,
		L"Send Invite",
		assets::DefaultButtonAssets,
		InSmallFont,
		Color::UIButtonBlue,
		FColor(1.f, 1.f, 1.f, 1.f));
	SendInviteButton->SetOnPressedCallback([this]()
	{
		SendPayloadToFriend(InviteTargetDropdown->GetCurrentSelection());
	});
	AddWidget(SendInviteButton);
	SendInviteButton->Disable();
}

void FCustomInviteSendDialog::Create()
{
	FDialog::Create();

	// set up an initial payload
	const std::string InitialPayload = "A1B2C3D4E5";
	const std::wstring WideInitialPayload = FStringUtils::Widen(std::move(InitialPayload));
	SetOutgoingCustomInvitePayload(WideInitialPayload);

	RebuildFriendsList(FGame::Get().GetFriends()->GetFriends());
	FGame::Get().GetFriends()->SubscribeToFriendStatusUpdates(this, [this](const std::vector<FFriendData>& Friends)
	{
		RebuildFriendsList(Friends);
	});
}

void FCustomInviteSendDialog::Release()
{
	FDialog::Release();

	FGame::Get().GetFriends()->UnsubscribeToFriendStatusUpdates(this);
}

void FCustomInviteSendDialog::OnUIEvent(const FUIEvent& Event)
{
	FDialog::OnUIEvent(Event);
}

void FCustomInviteSendDialog::Update()
{
	FDialog::Update();

	// Do we need to refresh data?
	uint64_t NewFriendsDirtyCounter = FGame::Get().GetFriends()->GetDirtyCounter();
	if (NewFriendsDirtyCounter != FriendsDirtyCounter)
	{
		const std::vector<FFriendData>& Friends = FGame::Get().GetFriends()->GetFriends();
		RebuildFriendsList(Friends);
	}

	// Enable / disable set payload button based on current payload text
	if (PayloadTextEditor)
	{
		std::wstring TextValue = PayloadTextEditor->GetText();
		if (TextValue.empty() || TextValue == PayloadTextEditor->GetInitialText())
		{
			SetPayloadButton->Disable();
		}
		else
		{
			SetPayloadButton->Enable();
		}
	}
}


void FCustomInviteSendDialog::SetPosition(Vector2 Pos)
{
	IWidget::SetPosition(Pos);

	if (BackgroundImage && CurrentPayloadLabel && CurrentPayloadText && PayloadTextEditor && SetPayloadButton)
	{
		BackgroundImage->SetPosition(Vector2(Pos.x - 20.f, Pos.y));

		CurrentPayloadLabel->SetPosition(Vector2(Pos.x + 5.f, Pos.y + 5.f));

		PayloadTextEditor->SetPosition(Vector2(CurrentPayloadLabel->GetPosition().x,
										CurrentPayloadLabel->GetPosition().y + CurrentPayloadLabel->GetSize().y));

		SetPayloadButton->SetPosition(Vector2(PayloadTextEditor->GetPosition().x + PayloadTextEditor->GetSize().x,
												PayloadTextEditor->GetPosition().y + (PayloadTextEditor->GetSize().y - SetPayloadButton->GetSize().y) / 2.f));

		CurrentPayloadText->SetPosition(Vector2(SetPayloadButton->GetPosition().x + SetPayloadButton->GetSize().x + 5.f,
												PayloadTextEditor->GetPosition().y));

		InviteTargetDropdown->SetPosition(Vector2(CurrentPayloadText->GetPosition().x + CurrentPayloadText->GetSize().x + 5.f,
													CurrentPayloadText->GetPosition().y + (CurrentPayloadText->GetSize().y - InviteTargetDropdown->GetSize().y) / 2.f));

		SendInviteButton->SetPosition(Vector2(Pos.x + 5.f, CurrentPayloadText->GetPosition().y + CurrentPayloadText->GetSize().y + 5.f));
	}
}

void FCustomInviteSendDialog::SetSize(Vector2 NewSize)
{
	IWidget::SetSize(NewSize);

	if (BackgroundImage && CurrentPayloadLabel && CurrentPayloadText && PayloadTextEditor && SetPayloadButton && InviteTargetDropdown)
	{
		BackgroundImage->SetSize(Vector2(NewSize.x + 35.f, NewSize.y));
		CurrentPayloadLabel->SetSize(Vector2(CUSTOMINVITE_DIALOG_PAYLOAD_LABEL_WIDTH, 30.f));
		CurrentPayloadText->SetSize(Vector2(CUSTOMINVITE_DIALOG_PAYLOAD_TEXT_WIDTH, CUSTOMINVITE_DIALOG_PAYLOAD_TEXT_HEIGHT));
		PayloadTextEditor->SetSize(Vector2(CUSTOMINVITE_DIALOG_PAYLOAD_TEXTENTRY_WIDTH, CUSTOMINVITE_DIALOG_PAYLOAD_TEXTENTRY_HEIGHT));
		SetPayloadButton->SetSize(Vector2(CUSTOMINVITE_DIALOG_SETPAYLOAD_BUTTON_WIDTH, CUSTOMINVITE_DIALOG_SETPAYLOAD_BUTTON_HEIGHT));
		SendInviteButton->SetSize(Vector2(CUSTOMINVITE_DIALOG_SETPAYLOAD_BUTTON_WIDTH, CUSTOMINVITE_DIALOG_SETPAYLOAD_BUTTON_HEIGHT));
		InviteTargetDropdown->SetSize(Vector2(CUSTOMINVITE_DIALOG_FRIENDLIST_LIST_WIDTH, CUSTOMINVITE_DIALOG_FRIENDLIST_LIST_HEIGHT));
	}
}

void FCustomInviteSendDialog::OnEscapePressed()
{
	Hide();
}

EOS_EResult FCustomInviteSendDialog::SetOutgoingCustomInvitePayload(const std::wstring& Payload)
{
	EOS_EResult Result = FGame::Get().GetCustomInvites()->SetCurrentPayload(Payload);
	if (Result == EOS_EResult::EOS_Success)
	{
		if (CurrentPayloadText)
		{
			CurrentPayloadText->Clear();
			CurrentPayloadText->AddLine(std::move(Payload), Color::LawnGreen);
		}

		if (SendInviteButton)
		{
			SendInviteButton->Enable();
		}
	}

	return Result;
}

void FCustomInviteSendDialog::SendPayloadToFriend(const std::wstring& FriendName)
{
	FGame::Get().GetCustomInvites()->SendInviteToFriend(FriendName);
}

void FCustomInviteSendDialog::RebuildFriendsList(const std::vector<FFriendData>& Friends)
{
	std::vector<std::wstring> FriendNames;
	std::vector<EOS_ProductUserId> Puids;
	std::transform(Friends.begin(), Friends.end(), std::back_inserter(FriendNames),
		[](const FFriendData& Friend) -> std::wstring { return Friend.Name; });

	InviteTargetDropdown->UpdateOptionsList(FriendNames);

	FriendsDirtyCounter = FGame::Get().GetFriends()->GetDirtyCounter();
}

