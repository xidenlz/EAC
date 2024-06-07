// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "Console.h"
#include "GameEvent.h"
#include "Authentication.h"
#include "ConsoleDialog.h"
#include "ExitDialog.h"
#include "AuthDialogs.h"
#include "SessionMatchmakingDialog.h"
#include "FriendsDialog.h"
#include "SessionInviteReceivedDialog.h"
#include "RequestToJoinSessionReceivedDialog.h"
#include "NewSessionDialog.h"
#include "SampleConstants.h"
#include "Menu.h"
#include "PopupDialog.h"

FMenu::FMenu(std::weak_ptr<FConsole> InConsole) noexcept(false):
	FBaseMenu(InConsole)
{

}

void FMenu::Create()
{
	CreateSessionMatchmakingDialog();
	CreateFriendsDialog();
	CreateSessionInviteDialog();
	CreateNewSessionDialog();
	CreateRequestToJoinSessionDialog();

	FBaseMenu::Create();
}

void FMenu::Release()
{
	if (SessionMatchmakingDialog)
	{
		SessionMatchmakingDialog->Release();
		SessionMatchmakingDialog.reset();
	}

	if (RequestToJoinSessionDialog)
	{
		RequestToJoinSessionDialog->Release();
		RequestToJoinSessionDialog.reset();
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

		if (SessionMatchmakingDialog)
		{
			SessionMatchmakingDialog->SetSize(Vector2(ConsoleWidgetSize.x, ConsoleWidgetPos.y - 100.0f - 10.0f));
			SessionMatchmakingDialog->SetPosition(Vector2(ConsoleWidgetPos.x, 100.0f));
		}

		UpdateFriendsDialogTransform(WindowSize);
	}

	if (PopupDialog)
	{
		PopupDialog->SetPosition(Vector2((WindowSize.x / 2.f) - PopupDialog->GetSize().x / 2.0f, (WindowSize.y / 2.f) - PopupDialog->GetSize().y));
	}

	if (ExitDialog)
	{
		ExitDialog->SetPosition(Vector2((WindowSize.x / 2.f) - ExitDialog->GetSize().x / 2.0f, (WindowSize.y / 2.f) - ExitDialog->GetSize().y));
	}

	if (SessionInviteDialog)
	{
		SessionInviteDialog->SetPosition(Vector2((WindowSize.x / 2.f) - SessionInviteDialog->GetSize().x / 2.0f, (WindowSize.y / 2.f) - SessionInviteDialog->GetSize().y - 2.0f));
	}

	if (NewSessionDialog)
	{
		NewSessionDialog->SetPosition(Vector2((WindowSize.x / 2.f) - NewSessionDialog->GetSize().x / 2.0f, (WindowSize.y / 2.f) - NewSessionDialog->GetSize().y + 20.0f));
	}

	if (AuthDialogs) AuthDialogs->UpdateLayout();

	if (RequestToJoinSessionDialog)
	{
		RequestToJoinSessionDialog->SetPosition(Vector2((WindowSize.x / 2.f) - SessionInviteDialog->GetSize().x / 2.0f, (WindowSize.y / 2.f) - SessionInviteDialog->GetSize().y - 2.0f));
	}
}

void FMenu::CreateSessionMatchmakingDialog()
{
	const float FX = 100.0f;
	const float FY = 100.0f;
	const float Width = 300.0f;
	const float Height = 300.0f;

	SessionMatchmakingDialog = std::make_shared<FSessionMatchmakingDialog>(
		Vector2(FX, FY),
		Vector2(Width, Height),
		DefaultLayer - 2,
		NormalFont->GetFont(),
		BoldSmallFont->GetFont(),
		TinyFont->GetFont());

	SessionMatchmakingDialog->SetBorderColor(Color::UIBorderGrey);
	
	SessionMatchmakingDialog->Create();

	AddDialog(SessionMatchmakingDialog);
}

void FMenu::CreateAuthDialogs()
{
	AuthDialogs = std::make_shared<FAuthDialogs>(
		FriendsDialog,
		L"Session Matchmaking",
		BoldSmallFont->GetFont(),
		SmallFont->GetFont(),
		TinyFont->GetFont());
	
	AuthDialogs->Create();
	AuthDialogs->SetSingleUserOnly(true);
}

void FMenu::CreateSessionInviteDialog()
{
	SessionInviteDialog = std::make_shared<FSessionInviteReceivedDialog>(
		Vector2(200.f, 200.f),
		Vector2(370.0f, 210.f),
		5,
		NormalFont->GetFont(),
		SmallFont->GetFont());

	SessionInviteDialog->Create();
	SessionInviteDialog->SetBorderColor(Color::UIBorderGrey);

	AddDialog(SessionInviteDialog);
	HideDialog(SessionInviteDialog);
}

void FMenu::CreateNewSessionDialog()
{
	NewSessionDialog = std::make_shared<FNewSessionDialog>(
		Vector2(200.f, 410.f),
		Vector2(370.0f, 300.0f),
		5,
		NormalFont->GetFont(),
		SmallFont->GetFont()
		);

	NewSessionDialog->Create();
	NewSessionDialog->SetBorderColor(Color::UIBorderGrey);

	AddDialog(NewSessionDialog);
	HideDialog(NewSessionDialog);
}

void FMenu::CreateRequestToJoinSessionDialog()
{
	RequestToJoinSessionDialog = std::make_shared<FRequestToJoinSessionReceivedDialog>(
		Vector2(200.f, 200.f),
		Vector2(370.0f, 210.f),
		5,
		NormalFont->GetFont(),
		SmallFont->GetFont()
		);

	RequestToJoinSessionDialog->Create();
	RequestToJoinSessionDialog->SetBorderColor(Color::UIBorderGrey);

	AddDialog(RequestToJoinSessionDialog);
	HideDialog(RequestToJoinSessionDialog);
}

void FMenu::OnGameEvent(const FGameEvent& Event)
{
	if (Event.GetType() == EGameEventType::InviteToSessionReceived)
	{
		if (SessionInviteDialog)
		{
			SessionInviteDialog->SetSessionInfo(FGame::Get().GetFriends()->GetFriendName(Event.GetProductUserId()), FGame::Get().GetSessions()->GetInviteSession());
			ShowDialog(SessionInviteDialog);
		}
	}
	else if (Event.GetType() == EGameEventType::OverlayInviteToSessionAccepted
			|| Event.GetType() == EGameEventType::OverlayInviteToSessionRejected)
	{
		if (SessionInviteDialog)
		{
			HideDialog(SessionInviteDialog);
		}
	}
	else if (Event.GetType() == EGameEventType::NewSession)
	{
		if (NewSessionDialog)
		{
			SessionMatchmakingDialog->Disable();
			ShowDialog(NewSessionDialog);
		}
	}
	else if (Event.GetType() == EGameEventType::SessionCreationFinished)
	{
		if (NewSessionDialog)
		{
			HideDialog(NewSessionDialog);
			SessionMatchmakingDialog->Enable();
		}
	}
	else if (Event.GetType() == EGameEventType::RequestToJoinSessionReceived)
	{
		// Note that there is only one Request to Join session dialog available so if a new Request to Join
		// arrives before an existing Request To Join has been dispositioned, the incoming Request To Join
		// will stomp the existing.
		if (RequestToJoinSessionDialog)
		{
			RequestToJoinSessionDialog->SetFriendInfo(FGame::Get().GetFriends()->GetFriendName(Event.GetProductUserId()), Event.GetProductUserId());
			ShowDialog(RequestToJoinSessionDialog);
		}
	}

	if (SessionMatchmakingDialog) SessionMatchmakingDialog->OnGameEvent(Event);

	FBaseMenu::OnGameEvent(Event);
}

void FMenu::UpdateFriendsDialogTransform(const Vector2 WindowSize)
{
	if (FriendsDialog)
	{
		const float FX = SessionMatchmakingDialog->GetPosition().x + SessionMatchmakingDialog->GetSize().x + 5.0f;
		const float FY = 100.0f;
		const float FriendsWidth = WindowSize.x - FX - 5.0f;
		const float FriendsHeight = WindowSize.y - FY - 10.0f;

		FriendsDialog->SetSize(Vector2(FriendsWidth, FriendsHeight));
		FriendsDialog->SetPosition(Vector2(FX, FY));
	}
}