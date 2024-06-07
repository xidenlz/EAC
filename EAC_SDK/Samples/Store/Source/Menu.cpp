// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "Console.h"
#include "GameEvent.h"
#include "Authentication.h"
#include "ConsoleDialog.h"
#include "StoreDialog.h"
#include "ExitDialog.h"
#include "NotificationDialog.h"
#include "AuthDialogs.h"
#include "SampleConstants.h"
#include "Menu.h"
#include "PopupDialog.h"
#include "GUI/Sprite.h"

namespace
{
	const Vector2 ConsoleDialogScale = Vector2(0.68f, 0.35f);
	constexpr float UpperDialogVerticalOffset = 100.f;
	constexpr float Padding = 10.f;
}

FMenu::FMenu(std::weak_ptr<FConsole> InConsole) noexcept(false):
	FBaseMenu(InConsole)
{
}

void FMenu::Create()
{
	CreateStoreDialog();
	CreateNotificationDialog();
	
	FBaseMenu::Create();
}

void FMenu::Release()
{
	if (StoreDialog)
	{
		StoreDialog->Release();
		StoreDialog.reset();
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
		Vector2 ConsoleDialogSize = Vector2(WindowSize.x * ConsoleDialogScale.x, WindowSize.y * ConsoleDialogScale.y);
		ConsoleDialog->SetSize(ConsoleDialogSize);

		Vector2 ConsoleDialogPos = Vector2(Padding, WindowSize.y - ConsoleDialogSize.y - Padding);
		ConsoleDialog->SetPosition(ConsoleDialogPos);

		if (StoreDialog)
		{
			float ConsoleDialogRight = ConsoleDialogPos.x + ConsoleDialogSize.x;
			float ConsoleDialogBottom = ConsoleDialogPos.y + ConsoleDialogSize.y;

			StoreDialog->SetConsoleDialogSizeAndPosition(ConsoleDialogSize, ConsoleDialogPos);

			Vector2 StoreDialogSize = Vector2(WindowSize.x - ConsoleDialog->GetSize().x - 3 * Padding,	ConsoleDialogBottom - UpperDialogVerticalOffset);
			StoreDialog->SetSize(StoreDialogSize);

			Vector2 StoreDialogPos = Vector2(ConsoleDialogRight + Padding, UpperDialogVerticalOffset);
			StoreDialog->SetPosition(StoreDialogPos);
		}
	}

	if (NotificationDialog)
	{
		NotificationDialog->SetPosition(Vector2(WindowSize.x - NotificationDialog->GetSize().x - 30.f, 30.f));
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

void FMenu::CreateStoreDialog()
{
	const float FX = 100.0f;
	const float FY = 100.0f;
	const float StoreWidth = 300.0f;
	const float StoreHeight = 300.0f;

	StoreDialog = std::make_shared<FStoreDialog>(
		Vector2(FX, FY),
		Vector2(StoreWidth, StoreHeight),
		DefaultLayer - 2,
		NormalFont->GetFont(),
		BoldSmallFont->GetFont(),
		TinyFont->GetFont());

	StoreDialog->SetBorderColor(Color::UIBorderGrey);
	StoreDialog->Create();
	
	AddDialog(StoreDialog);
}

void FMenu::CreateAuthDialogs()
{
	AuthDialogs = std::make_shared<FAuthDialogs>(
		StoreDialog,
		L"Catalog",
		BoldSmallFont->GetFont(),
		SmallFont->GetFont(),
		TinyFont->GetFont());
	
	AuthDialogs->Create();
	AuthDialogs->SetSingleUserOnly(true);
}

void FMenu::CreateNotificationDialog()
{
	NotificationDialog = std::make_shared<FNotificationDialog>(
		Vector2(200.f, 200.f),
		Vector2(330.f, 60.f),
		5,
		NormalFont->GetFont(),
		SmallFont->GetFont());

	NotificationDialog->SetBorderColor(Color::UIBorderGrey);
	NotificationDialog->Create();

	AddDialog(NotificationDialog);

	HideDialog(NotificationDialog);
}

void FMenu::OnGameEvent(const FGameEvent& Event)
{
	if (Event.GetType() == EGameEventType::ShowPrevUser)
	{
		UpdateStore();
	}
	else if (Event.GetType() == EGameEventType::ShowNextUser)
	{
		UpdateStore();
	}
	else if (Event.GetType() == EGameEventType::CancelLogin)
	{
		UpdateStore();
	}
	else if (Event.GetType() == EGameEventType::ToggleNotification)
	{
		if (NotificationDialog->IsShown())
		{
			HideDialog(NotificationDialog);
		}
		else
		{
			ShowDialog(NotificationDialog);
		}
	}

	if (StoreDialog) StoreDialog->OnGameEvent(Event);

	if (NotificationDialog) NotificationDialog->OnGameEvent(Event);

	FBaseMenu::OnGameEvent(Event);
}

void FMenu::UpdateStore()
{
	if (StoreDialog)
	{
		StoreDialog->SetPosition(Vector2(ConsoleDialog->GetPosition().x + ConsoleDialog->GetSize().x + 10.f,
										 StoreDialog->GetPosition().y));
	}
}