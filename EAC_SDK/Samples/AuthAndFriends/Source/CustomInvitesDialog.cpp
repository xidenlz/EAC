// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "Game.h"
#include "CustomInvitesDialog.h"
#include "TextLabel.h"
#include "Button.h"
#include "UIEvent.h"
#include "GameEvent.h"
#include "Sprite.h"
#include "Player.h"
#include "DebugLog.h"
#include "Checkbox.h"
#include "Users.h"
#include "DropDownList.h"
#include "TextEditor.h"
#include <functional>
#include "CustomInviteSendDialog.h"
#include "CustomInviteReceivedDialog.h"
#include "Menu.h"

#define CUSTOMINVITE_DIALOG_SENDINVITEBUTTON_WIDTH		120.f
#define CUSTOMINVITE_DIALOG_SENDINVITEBUTTON_HEIGHT		40.f

#define CUSTOMINVITE_DIALOG_SENDINVITEDIALOG_WIDTH		800.f
#define CUSTOMINVITE_DIALOG_SENDINVITEDIALOG_HEIGHT		200.f


FCustomInvitesDialog::FCustomInvitesDialog(
	Vector2 DialogPos,
	Vector2 DialogSize,
	UILayer DialogLayer,
	FontPtr DialogNormalFont,
	FontPtr DialogSmallFont,
	FontPtr DialogTinyFont) :
	FDialog(DialogPos, DialogSize, DialogLayer)
{
	Vector2 DummyVector{};  // the position / size values are irrelevant in constructor

	BackgroundImage = std::make_shared<FSpriteWidget>(
		DummyVector,
		DummyVector,
		DialogLayer,
		L"Assets/friends.dds");
	AddWidget(BackgroundImage);

	TitleLabel = std::make_shared<FTextLabelWidget>(
		DummyVector,
		DummyVector,
		DialogLayer,
		L"CUSTOM INVITES",
		L"Assets/dialog_title.dds",
		FColor(1.f, 1.f, 1.f, 1.f),
		FColor(1.f, 1.f, 1.f, 1.f),
		EAlignmentType::Left);
	TitleLabel->SetBorderColor(Color::UIBorderGrey);
	TitleLabel->SetFont(DialogNormalFont);
	AddWidget(TitleLabel);

	// "Send Custom Invite"
	OpenSendInviteDialogButton = std::make_shared<FButtonWidget>(
		DummyVector,
		DummyVector,
		DialogLayer,
		L"Send Custom Invite",
		assets::DefaultButtonAssets,
		DialogSmallFont,
		Color::UIButtonBlue,
		FColor(1.f, 1.f, 1.f, 1.f));
	OpenSendInviteDialogButton->SetOnPressedCallback([this]()
	{
		OpenSendInviteDialogButton->Hide();
		if (std::shared_ptr<FMenu> StrongOuterMenu = WeakOuterMenu.lock())
		{
			StrongOuterMenu->ShowDialog(CustomInviteSendDialog);
		}
	});
	AddWidget(OpenSendInviteDialogButton);
	OpenSendInviteDialogButton->Disable();

	CustomInviteSendDialog = std::make_shared<FCustomInviteSendDialog>(
		DummyVector,
		DummyVector,
		Layer - 2,
		DialogNormalFont,
		DialogSmallFont);
	CustomInviteSendDialog->Create();

	CustomInviteReceivedDialog = std::make_shared<FCustomInviteReceivedDialog>(
		DummyVector,
		DummyVector,
		Layer - 2,
		DialogNormalFont,
		DialogSmallFont);
	CustomInviteReceivedDialog->Create();
}

void FCustomInvitesDialog::SetPosition(Vector2 Pos)
{
	IWidget::SetPosition(Pos);

	if (BackgroundImage)
	{
		BackgroundImage->SetPosition(Pos);
	}

	if (TitleLabel)
	{
		TitleLabel->SetPosition(Pos);
	}

	if (OpenSendInviteDialogButton)
	{
		OpenSendInviteDialogButton->SetPosition(Pos + Vector2((GetSize().x - OpenSendInviteDialogButton->GetSize().x) / 2.f, (GetSize().y - OpenSendInviteDialogButton->GetSize().y) / 2.f));
	}

	if (!WeakOuterMenu.expired())
	{
		std::shared_ptr<FMenu> StrongOuterMenu = WeakOuterMenu.lock();
		const Vector2& WindowSize = StrongOuterMenu->WindowSize;

		if (CustomInviteSendDialog)
		{
			CustomInviteSendDialog->SetPosition(Vector2(Pos.x, Pos.y + TitleLabel->GetSize().y + 5.f));
		}

		if (CustomInviteReceivedDialog)
		{
			CustomInviteReceivedDialog->SetPosition(CustomInviteSendDialog->GetPosition());
		}
	}
}

void FCustomInvitesDialog::SetSize(Vector2 NewSize)
{
	IWidget::SetSize(NewSize);

	if (BackgroundImage)
	{
		BackgroundImage->SetSize(NewSize);
	}

	if (TitleLabel)
	{
		TitleLabel->SetSize(Vector2(NewSize.x, 30.0f));
	}

	if (OpenSendInviteDialogButton)
	{
		OpenSendInviteDialogButton->SetSize(Vector2(CUSTOMINVITE_DIALOG_SENDINVITEBUTTON_WIDTH, CUSTOMINVITE_DIALOG_SENDINVITEBUTTON_HEIGHT));
	}

	if (CustomInviteSendDialog)
	{
		CustomInviteSendDialog->SetSize(Vector2(CUSTOMINVITE_DIALOG_SENDINVITEDIALOG_WIDTH, CUSTOMINVITE_DIALOG_SENDINVITEDIALOG_HEIGHT));
	}

	if (CustomInviteReceivedDialog)
	{
		CustomInviteReceivedDialog->SetSize(Vector2(CUSTOMINVITE_DIALOG_SENDINVITEDIALOG_WIDTH, CUSTOMINVITE_DIALOG_SENDINVITEDIALOG_HEIGHT));
	}
}

void FCustomInvitesDialog::OnEscapePressed()
{
	if (OpenSendInviteDialogButton)
	{
		OpenSendInviteDialogButton->Show();
	}
}

void FCustomInvitesDialog::OnGameEvent(const FGameEvent& Event)
{
	if (Event.GetType() == EGameEventType::UserLoggedIn)
	{
		Enable();
	}
	else if (Event.GetType() == EGameEventType::UserLoginRequiresMFA)
	{
		Disable();
		SetFocused(false);
	}
	else if (Event.GetType() == EGameEventType::UserLoginEnteredMFA)
	{
		Disable();
	}
	else if (Event.GetType() == EGameEventType::UserLoggedOut)
	{
		if (FPlayerManager::Get().GetNumPlayers() == 0)
		{
			Disable();
		}
	}
	else if (Event.GetType() == EGameEventType::NewUserLogin)
	{
		Disable();
	}
	else if (Event.GetType() == EGameEventType::CancelLogin)
	{
		Disable();
	}
	else if (Event.GetType() == EGameEventType::UserConnectLoggedIn)
	{
		Enable();
	}
	else if (Event.GetType() == EGameEventType::CustomInviteReceived)
	{
		if (std::shared_ptr<FMenu> StrongOuterMenu = WeakOuterMenu.lock())
		{
			if (CustomInviteSendDialog)
			{
				StrongOuterMenu->HideDialog(CustomInviteSendDialog);
			}
			if (CustomInviteReceivedDialog)
			{
				StrongOuterMenu->ShowDialog(CustomInviteReceivedDialog);
			}
		}
		if (OpenSendInviteDialogButton)
		{
			OpenSendInviteDialogButton->Hide();
		}
	}
	else if (Event.GetType() == EGameEventType::CustomInviteAccepted ||
			Event.GetType() == EGameEventType::CustomInviteDeclined)
	{
		if (std::shared_ptr<FMenu> StrongOuterMenu = WeakOuterMenu.lock())
		{
			if (CustomInviteReceivedDialog)
			{
				StrongOuterMenu->HideDialog(CustomInviteReceivedDialog);
			}
		}
		if (OpenSendInviteDialogButton)
		{
			OpenSendInviteDialogButton->Show();
		}
	}

	if (CustomInviteReceivedDialog)
	{
		CustomInviteReceivedDialog->OnGameEvent(Event);
	}
}

void FCustomInvitesDialog::SetOuterMenu(std::weak_ptr<FMenu> InOuterMenu)
{
	WeakOuterMenu = InOuterMenu;
	if (std::shared_ptr<FMenu> StrongOuterMenu = WeakOuterMenu.lock())
	{
		StrongOuterMenu->AddDialog(CustomInviteSendDialog);
		StrongOuterMenu->HideDialog(CustomInviteSendDialog);

		StrongOuterMenu->AddDialog(CustomInviteReceivedDialog);
		StrongOuterMenu->HideDialog(CustomInviteReceivedDialog);
	}
}
