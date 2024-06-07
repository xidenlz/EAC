// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "Console.h"
#include "GameEvent.h"
#include "Authentication.h"
#include "ConsoleDialog.h"
#include "ExitDialog.h"
#include "AuthDialogs.h"
#include "LeaderboardDialog.h"
#include "SampleConstants.h"
#include "Menu.h"
#include "PopupDialog.h"

const Vector2 ConsoleDialogSizeProportion = Vector2(0.68f, 0.35f);

FMenu::FMenu(std::weak_ptr<FConsole> InConsole) noexcept(false) :
	FBaseMenu(InConsole)
{

}

void FMenu::Create()
{
	CreateLeaderboardDialog();

	FBaseMenu::Create();
}

void FMenu::Release()
{
	FBaseMenu::Release();

	if (LeaderboardDialog)
	{
		LeaderboardDialog->Release();
		LeaderboardDialog.reset();
	}
}

void FMenu::UpdateLayout(int Width, int Height)
{
	Vector2 WindowSize = Vector2((float)Width, (float)Height);

	BackgroundImage->SetPosition(Vector2(0.f, 0.f));
	BackgroundImage->SetSize(Vector2((float)Width, ((float)Height) / 2.f));

	if (ConsoleDialog)
	{
		Vector2 ConsoleWidgetSize = Vector2(WindowSize.x * ConsoleDialogSizeProportion.x, WindowSize.y * ConsoleDialogSizeProportion.y);
		ConsoleDialog->SetSize(ConsoleWidgetSize);

		Vector2 ConsoleWidgetPos = Vector2(10.f, WindowSize.y - ConsoleWidgetSize.y - 10.f);
		ConsoleDialog->SetPosition(ConsoleWidgetPos);

		if (LeaderboardDialog)
		{
			LeaderboardDialog->SetWindowSize(WindowSize);

			Vector2 LeaderboardDialogSize = Vector2(WindowSize.x - ConsoleDialog->GetSize().x - 30.f, WindowSize.y - 130.0f);
			LeaderboardDialog->SetSize(LeaderboardDialogSize);

			Vector2 LeaderboardDialogPos = Vector2(ConsoleDialog->GetPosition().x + ConsoleDialog->GetSize().x + 10.f,
				WindowSize.y - LeaderboardDialogSize.y - 10.f);
			LeaderboardDialog->SetPosition(LeaderboardDialogPos);
		}
	}

	if (PopupDialog)
	{
		PopupDialog->SetPosition(Vector2((WindowSize.x / 2.f) - PopupDialog->GetSize().x / 2.0f, (WindowSize.y / 2.f) - PopupDialog->GetSize().y));
	}

	if (ExitDialog)
	{
		ExitDialog->SetPosition(Vector2((WindowSize.x / 2.f) - ExitDialog->GetSize().x / 2.0f, (WindowSize.y / 2.f) - ExitDialog->GetSize().y));
	}

	if (AuthDialogs) AuthDialogs->UpdateLayout();
}

void FMenu::CreateAuthDialogs()
{
	AuthDialogs = std::make_shared<FAuthDialogs>(
		LeaderboardDialog,
		L"Leaderboard",
		BoldSmallFont->GetFont(),
		SmallFont->GetFont(),
		TinyFont->GetFont());

	AuthDialogs->SetUserLabelOffset(Vector2(-30.0f, -10.0f));
	AuthDialogs->Create();
	AuthDialogs->SetSingleUserOnly(true);
}

void FMenu::CreateLeaderboardDialog()
{
	const float FX = 100.0f;
	const float FY = 100.0f;
	const float Width = 300.0f;
	const float Height = 300.0f;

	LeaderboardDialog = std::make_shared<FLeaderboardDialog>(
		Vector2(FX, FY),
		Vector2(Width, Height),
		DefaultLayer - 2,
		NormalFont->GetFont(),
		BoldSmallFont->GetFont(),
		TinyFont->GetFont());

	LeaderboardDialog->SetWindowProportion(ConsoleDialogSizeProportion);
	LeaderboardDialog->SetBorderColor(Color::UIBorderGrey);
	
	LeaderboardDialog->Create();

	AddDialog(LeaderboardDialog);
}

void FMenu::OnGameEvent(const FGameEvent& Event)
{
	FBaseMenu::OnGameEvent(Event);

	if (Event.GetType() == EGameEventType::ShowPrevUser)
	{
		UpdateLeaderboardDialog();
	}
	else if (Event.GetType() == EGameEventType::ShowNextUser)
	{
		UpdateLeaderboardDialog();
	}
	else if (Event.GetType() == EGameEventType::CancelLogin)
	{
		UpdateLeaderboardDialog();
	}

	if (LeaderboardDialog) LeaderboardDialog->OnGameEvent(Event);
}

void FMenu::UpdateLeaderboardDialog()
{
	if (LeaderboardDialog)
	{
		LeaderboardDialog->SetPosition(Vector2(ConsoleDialog->GetPosition().x + ConsoleDialog->GetSize().x + 10.f,
			LeaderboardDialog->GetPosition().y));
	}
}
