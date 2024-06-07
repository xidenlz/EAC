// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "Console.h"
#include "GameEvent.h"
#include "Authentication.h"
#include "ConsoleDialog.h"
#include "ExitDialog.h"
#include "AuthDialogs.h"
#include "FriendsDialog.h"
#include "LobbiesDialog.h"
#include "NewLobbyDialog.h"
#include "LobbyInviteReceivedDialog.h"
#include "SampleConstants.h"
#include "Menu.h"
#include "PopupDialog.h"

const float UpperDialogsVerticalOffset = 130.0f;

FMenu::FMenu(std::weak_ptr<FConsole> InConsole) noexcept(false):
	FBaseMenu(InConsole)
{

}

void FMenu::Create()
{
	CreateFriendsDialog();

	FBaseMenu::Create();

	CreateLobbiesDialog();
	CreateNewLobbyDialog();
	CreateLobbyInviteReceivedDialog();
}

void FMenu::Release()
{
	if (LobbiesDialog)
	{
		LobbiesDialog->Release();
		LobbiesDialog.reset();
	}

	FBaseMenu::Release();
}

void FMenu::UpdateLayout(int Width, int Height)
{
	Vector2 WindowSize = Vector2((float)Width, (float)Height);

	BackgroundImage->SetPosition(Vector2(0.f, 0.f));
	BackgroundImage->SetSize(Vector2((float)Width, ((float)Height) / 2.f));

	if (ConsoleDialog)
	{
		Vector2 ConsoleWidgetSize = Vector2(WindowSize.x * 0.68f, WindowSize.y * 0.35f);
		ConsoleDialog->SetSize(ConsoleWidgetSize);

		Vector2 ConsoleWidgetPos = Vector2(10.f, WindowSize.y - ConsoleWidgetSize.y - 10.f);
		ConsoleDialog->SetPosition(ConsoleWidgetPos);

		if (LobbiesDialog)
		{
			LobbiesDialog->SetSize(Vector2(ConsoleWidgetSize.x, ConsoleWidgetPos.y - UpperDialogsVerticalOffset - 10.0f));
			LobbiesDialog->SetPosition(Vector2(ConsoleWidgetPos.x, UpperDialogsVerticalOffset));
		}

		UpdateFriendsDialogTransform(WindowSize);
	}

	if (PopupDialog)
	{
		PopupDialog->SetPosition(Vector2((WindowSize.x / 2.f) - PopupDialog->GetSize().x / 2.0f, (WindowSize.y / 2.f) - PopupDialog->GetSize().y));
	}

	if (ExitDialog)
	{
		ExitDialog->SetPosition(Vector2((WindowSize.x / 2.f) - ExitDialog->GetSize().x / 2.0f, (WindowSize.y / 2.f) - ExitDialog->GetSize().y - 200.0f));
	}

	if (NewLobbyDialog)
	{
		NewLobbyDialog->SetPosition(Vector2((WindowSize.x / 2.f) - NewLobbyDialog->GetSize().x / 2.0f, (WindowSize.y / 2.f) - NewLobbyDialog->GetSize().y + 90.0f));
	}

	if (LobbyInviteReceivedDialog)
	{
		LobbyInviteReceivedDialog->SetPosition(Vector2((WindowSize.x / 2.f) - LobbyInviteReceivedDialog->GetSize().x / 2.0f, (WindowSize.y / 2.f) - LobbyInviteReceivedDialog->GetSize().y - 2.0f));
	}

	if (AuthDialogs) AuthDialogs->UpdateLayout();
}

void FMenu::CreateLobbiesDialog()
{
	const float FX = ConsoleDialog->GetPosition().x;
	const float FY = UpperDialogsVerticalOffset;
	const float Width = ConsoleDialog->GetSize().x;
	const float Height = ConsoleDialog->GetSize().y;

	LobbiesDialog = std::make_shared<FLobbiesDialog>(
		Vector2(FX, FY),
		Vector2(Width, Height),
		DefaultLayer - 2,
		NormalFont->GetFont(),
		BoldSmallFont->GetFont(),
		TinyFont->GetFont());

	LobbiesDialog->SetBorderColor(Color::UIBorderGrey);
	
	LobbiesDialog->Create();

	AddDialog(LobbiesDialog);
}

void FMenu::CreateNewLobbyDialog()
{
	NewLobbyDialog = std::make_shared<FNewLobbyDialog>(
		Vector2(200.f, 270.f),
		Vector2(370.0f, 280.0f),
		5,
		NormalFont->GetFont(),
		SmallFont->GetFont()
		);

	NewLobbyDialog->Create();
	NewLobbyDialog->SetBorderColor(Color::UIBorderGrey);

	AddDialog(NewLobbyDialog);
	HideDialog(NewLobbyDialog);
}

void FMenu::CreateLobbyInviteReceivedDialog()
{
	LobbyInviteReceivedDialog = std::make_shared<FLobbyInviteReceivedDialog>(
		Vector2(200.f, 270.f),
		Vector2(370.0f, 200.0f),
		5,
		NormalFont->GetFont(),
		SmallFont->GetFont()
		);

	LobbyInviteReceivedDialog->Create();
	LobbyInviteReceivedDialog->SetBorderColor(Color::UIBorderGrey);

	AddDialog(LobbyInviteReceivedDialog);
	HideDialog(LobbyInviteReceivedDialog);
}

void FMenu::OnUIEvent(const FUIEvent& Event)
{
	if ((!NewLobbyDialog || !NewLobbyDialog->IsShown()) &&
		(!LobbyInviteReceivedDialog || !LobbyInviteReceivedDialog->IsShown()))
	{
		FBaseMenu::OnUIEvent(Event);
	}
	else
	{
		if (NewLobbyDialog && NewLobbyDialog->IsShown())
		{
			NewLobbyDialog->OnUIEvent(Event);
		}
		if (LobbyInviteReceivedDialog && LobbyInviteReceivedDialog->IsShown())
		{
			LobbyInviteReceivedDialog->OnUIEvent(Event);
		}
	}
}

void FMenu::OnGameEvent(const FGameEvent& Event)
{
	if (Event.GetType() == EGameEventType::LobbyInviteReceived)
	{
		if (LobbyInviteReceivedDialog)
		{
			LobbyInviteReceivedDialog->SetLobbyInvite(FGame::Get().GetLobbies()->GetCurrentInvite());
			ShowDialog(LobbyInviteReceivedDialog);
		}
	}
	else if (Event.GetType() == EGameEventType::OverlayInviteToLobbyAccepted)
	{
		if (LobbyInviteReceivedDialog)
		{
			HideDialog(LobbyInviteReceivedDialog);
		}
	}
	else if (Event.GetType() == EGameEventType::NewLobby)
	{
		if (NewLobbyDialog)
		{
			LobbiesDialog->Disable();
			NewLobbyDialog->SetEditingMode(false);
			ShowDialog(NewLobbyDialog);
		}
	}
	else if (Event.GetType() == EGameEventType::LobbyCreationFinished)
	{
		if (NewLobbyDialog)
		{
			HideDialog(NewLobbyDialog);
			LobbiesDialog->Enable();
		}
	}
	else if (Event.GetType() == EGameEventType::ModifyLobby)
	{
		if (NewLobbyDialog)
		{
			LobbiesDialog->Disable();
			NewLobbyDialog->SetEditingMode(true);
			ShowDialog(NewLobbyDialog);
		}
	}
	else if (Event.GetType() == EGameEventType::LobbyModificationFinished)
	{
		if (NewLobbyDialog)
		{
			HideDialog(NewLobbyDialog);
			LobbiesDialog->Enable();
		}
	}

	if (LobbiesDialog)
	{
		LobbiesDialog->OnGameEvent(Event);
	}

	FBaseMenu::OnGameEvent(Event);
}

void FMenu::UpdateFriendsDialogTransform(const Vector2 WindowSize)
{
	if (FriendsDialog)
	{
		const float FX = LobbiesDialog->GetPosition().x + LobbiesDialog->GetSize().x + 5.0f;
		const float FY = UpperDialogsVerticalOffset;
		const float FriendsWidth = WindowSize.x - FX - 5.0f;
		const float FriendsHeight = WindowSize.y - FY - 10.0f;

		FriendsDialog->SetSize(Vector2(FriendsWidth, FriendsHeight));
		FriendsDialog->SetPosition(Vector2(FX, FY));
	}
}
