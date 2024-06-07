// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "Console.h"
#include "GameEvent.h"
#include "Authentication.h"
#include "ConsoleDialog.h"
#include "ExitDialog.h"
#include "AuthDialogs.h"
#include "ModsDialog.h"
#include "ModsTableDialog.h"
#include "SampleConstants.h"
#include "Menu.h"
#include "PopupDialog.h"

const Vector2 ConsoleDialogSizeProportion = Vector2(0.68f, 0.35f);

FMenu::FMenu(std::weak_ptr<FConsole> InConsole) noexcept(false):
	FBaseMenu(InConsole)
{

}

void FMenu::Create()
{
	CreateModsDialog();
	CreateModsTableDialog();

	FBaseMenu::Create();
}

void FMenu::Release()
{
	if (ModsDialog)
	{
		ModsDialog->Release();
		ModsDialog.reset();
	}

	if (ModsTableDialog)
	{
		ModsTableDialog->Release();
		ModsTableDialog.reset();
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
		Vector2 ConsoleWidgetSize = Vector2(WindowSize.x * ConsoleDialogSizeProportion.x, WindowSize.y * ConsoleDialogSizeProportion.y);
		ConsoleDialog->SetSize(ConsoleWidgetSize);

		Vector2 ConsoleWidgetPos = Vector2(10.f, WindowSize.y - ConsoleWidgetSize.y - 10.f);
		ConsoleDialog->SetPosition(ConsoleWidgetPos);

		if (ModsDialog)
		{
			ModsDialog->SetWindowSize(WindowSize);

			Vector2 ModsDialogSize = Vector2(WindowSize.x - ConsoleDialog->GetSize().x - 30.f, WindowSize.y - 130.0f);
			ModsDialog->SetSize(ModsDialogSize);

			Vector2 ModsDialogPos = Vector2(ConsoleDialog->GetPosition().x + ConsoleDialog->GetSize().x + 10.f,
				WindowSize.y - ModsDialogSize.y - 10.f);
			ModsDialog->SetPosition(ModsDialogPos);
		}

		if (ModsTableDialog)
		{
			ModsTableDialog->SetWindowSize(WindowSize);

			Vector2 ModsTableDialogSize = Vector2(ConsoleWidgetSize.x, WindowSize.y - ConsoleDialog->GetSize().y - 130.0f);
			ModsTableDialog->SetSize(ModsTableDialogSize);

			Vector2 ModsTableDialogPos = Vector2(ConsoleDialog->GetPosition().x, WindowSize.y - ConsoleWidgetSize.y - ModsTableDialogSize.y - 10.f);
			ModsTableDialog->SetPosition(ModsTableDialogPos);
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
		ModsDialog,
		L"Mods",
		BoldSmallFont->GetFont(),
		SmallFont->GetFont(),
		TinyFont->GetFont());

	AuthDialogs->SetUserLabelOffset(Vector2(-30.0f, -10.0f));
	AuthDialogs->Create();
	AuthDialogs->SetSingleUserOnly(true);
}

void FMenu::CreateModsDialog()
{
	const float FX = 100.0f;
	const float FY = 100.0f;
	const float Width = 300.0f;
	const float Height = 300.0f;

	ModsDialog = std::make_shared<FModsDialog>(
		Vector2(FX, FY),
		Vector2(Width, Height),
		DefaultLayer - 2,
		NormalFont->GetFont(),
		BoldSmallFont->GetFont(),
		TinyFont->GetFont());

	ModsDialog->SetWindowProportion(ConsoleDialogSizeProportion);
	ModsDialog->SetBorderColor(Color::UIBorderGrey);
	
	ModsDialog->Create();

	AddDialog(ModsDialog);
}

void FMenu::CreateModsTableDialog()
{
	const float FX = 20.f;
	const float FY = 170.f;
	const float Width = 300.0f;
	const float Height = 300.f;

	ModsTableDialog = std::make_shared<FModsTableDialog>(
		Vector2(FX, FY),
		Vector2(Width, Height),
		DefaultLayer - 2,
		NormalFont->GetFont(),
		BoldSmallFont->GetFont(),
		TinyFont->GetFont());

	ModsTableDialog->SetWindowProportion(ConsoleDialogSizeProportion);
	ModsTableDialog->SetBorderColor(Color::UIBorderGrey);

	ModsTableDialog->Create();

	AddDialog(ModsTableDialog);
}

void FMenu::OnGameEvent(const FGameEvent& Event)
{
	if (Event.GetType() == EGameEventType::ShowPrevUser)
	{
		UpdateMods();
	}
	else if (Event.GetType() == EGameEventType::ShowNextUser)
	{
		UpdateMods();
	}
	else if (Event.GetType() == EGameEventType::CancelLogin)
	{
		UpdateMods();
	}

	if (ModsDialog) ModsDialog->OnGameEvent(Event);
	if (ModsTableDialog) ModsTableDialog->OnGameEvent(Event);

	FBaseMenu::OnGameEvent(Event);
}

void FMenu::UpdateMods()
{
	if (ModsDialog)
	{
		ModsDialog->SetPosition(Vector2(ConsoleDialog->GetPosition().x + ConsoleDialog->GetSize().x + 10.f,
			ModsDialog->GetPosition().y));
	}

	if (ModsTableDialog)
	{
		ModsTableDialog->SetPosition(Vector2(ConsoleDialog->GetPosition().x + ConsoleDialog->GetSize().x + 10.f,
			ModsTableDialog->GetPosition().y));
	}
}
