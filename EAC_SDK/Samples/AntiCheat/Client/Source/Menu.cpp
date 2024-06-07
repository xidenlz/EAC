// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "Menu.h"
#include "AuthDialogs.h"
#include "AntiCheatDialog.h"
#include "ConsoleDialog.h"
#include "ExitDialog.h"
#include "PopupDialog.h"
#include "Sprite.h"

const Vector2 ConsoleDialogSizeProportion = Vector2(0.68f, 0.35f);

FMenu::FMenu(std::weak_ptr<FConsole> InConsole) noexcept(false) :
	FBaseMenu(InConsole)
{

}

void FMenu::Create()
{
	CreateAntiCheatDialog();

	FBaseMenu::Create();
}

void FMenu::Release()
{
	FBaseMenu::Release();

	if (AntiCheatDialog)
	{
		AntiCheatDialog->Release();
		AntiCheatDialog.reset();
	}
}

void FMenu::CreateAntiCheatDialog()
{
	const float FX = 100.0f;
	const float FY = 100.0f;
	const float Width = 300.0f;
	const float Height = 300.0f;

	AntiCheatDialog = std::make_shared<FAntiCheatDialog>(
		Vector2(FX, FY),
		Vector2(Width, Height),
		DefaultLayer - 2,
		NormalFont->GetFont(),
		BoldSmallFont->GetFont());

	AntiCheatDialog->SetWindowProportion(ConsoleDialogSizeProportion);
	AntiCheatDialog->SetBorderColor(Color::UIBorderGrey);

	AntiCheatDialog->Create();

	AddDialog(AntiCheatDialog);
}

void FMenu::CreateAuthDialogs()
{
	AuthDialogs = std::make_shared<FAuthDialogs>(
		AntiCheatDialog,
		L"AntiCheat",
		BoldSmallFont->GetFont(),
		SmallFont->GetFont(),
		TinyFont->GetFont());

	AuthDialogs->SetUserLabelOffset(Vector2(0.0f, -5.0f));
	AuthDialogs->Create();

	AuthDialogs->SetSingleUserOnly(true);
}

void FMenu::UpdateLayout(int Width, int Height)
{
	Vector2 WindowSize = Vector2((float)Width, (float)Height);

	BackgroundImage->SetPosition(Vector2(0.f, 0.f));
	BackgroundImage->SetSize(Vector2((float)Width, ((float)Height) / 2.f));

	if (ConsoleDialog)
	{
		Vector2 ConsoleWidgetSize = Vector2(WindowSize.x * 0.7f, WindowSize.y * 0.75f);
		ConsoleDialog->SetSize(ConsoleWidgetSize);

		Vector2 ConsoleWidgetPos = Vector2(10.f, WindowSize.y - ConsoleWidgetSize.y - 10.f);
		ConsoleDialog->SetPosition(ConsoleWidgetPos);

		if (AntiCheatDialog)
		{
			Vector2 FriendsDialogSize = Vector2(WindowSize.x - ConsoleDialog->GetSize().x - 30.f,
				ConsoleDialog->GetSize().y);
			AntiCheatDialog->SetSize(FriendsDialogSize);

			Vector2 FriendDialogPos = Vector2(ConsoleDialog->GetPosition().x + ConsoleDialog->GetSize().x + 10.f,
				ConsoleDialog->GetPosition().y);
			AntiCheatDialog->SetPosition(FriendDialogPos);
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

void FMenu::OnGameEvent(const FGameEvent& Event)
{
	FBaseMenu::OnGameEvent(Event);

	if (AntiCheatDialog) AntiCheatDialog->OnGameEvent(Event);
}