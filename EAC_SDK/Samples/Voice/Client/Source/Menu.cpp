// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "Console.h"
#include "GameEvent.h"
#include "Authentication.h"
#include "ConsoleDialog.h"
#include "ExitDialog.h"
#include "AuthDialogs.h"
#include "FriendsDialog.h"
#include "VoiceDialog.h"
#include "VoiceSetupDialog.h"
#include "SampleConstants.h"
#include "Menu.h"
#include "PopupDialog.h"
#include "CommandLine.h"

const float UpperDialogsVerticalOffset = 130.0f;
const Vector2 ConsoleDialogSizeProportion = Vector2(0.68f, 0.35f);

FMenu::FMenu(std::weak_ptr<FConsole> InConsole) noexcept(false):
	FBaseMenu(InConsole)
{

}

void FMenu::Create()
{
	if (!FCommandLine::Get().HasFlagParam(CommandLineConstants::Server))
	{
		CreateFriendsDialog();
	}

	if (!FCommandLine::Get().HasFlagParam(CommandLineConstants::Server))
	{
		CreateVoiceDialog();
		CreateVoiceSetupDialog();
	}
	else
	{
		TitleLabel->SetText(L"EOS SDK " + FStringUtils::Widen(SampleConstants::GameName) + L" SERVER");
	}

	FBaseMenu::Create();

	if (AuthDialogs)
	{
		AuthDialogs->SetSingleUserOnly(true);
	}
}

void FMenu::Release()
{
	if (VoiceDialog)
	{
		VoiceDialog->Release();
		VoiceDialog.reset();
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
		if (!FCommandLine::Get().HasFlagParam(CommandLineConstants::Server))
		{
			Vector2 ConsoleWidgetSize = Vector2(WindowSize.x * ConsoleDialogSizeProportion.x, WindowSize.y * ConsoleDialogSizeProportion.y);
			ConsoleDialog->SetSize(ConsoleWidgetSize);

			Vector2 ConsoleWidgetPos = Vector2(10.f, WindowSize.y - ConsoleWidgetSize.y - 10.f);
			ConsoleDialog->SetPosition(ConsoleWidgetPos);

			if (VoiceDialog)
			{
				VoiceDialog->SetSize(Vector2(ConsoleWidgetSize.x, ConsoleWidgetPos.y - UpperDialogsVerticalOffset - 10.0f));
				VoiceDialog->SetPosition(Vector2(ConsoleWidgetPos.x, UpperDialogsVerticalOffset));
			}

			UpdateFriendsDialogTransform(WindowSize);
		}
		else
		{
			Vector2 ConsoleWidgetSize = Vector2(WindowSize.x * 0.95f, WindowSize.y * 0.8f);
			ConsoleDialog->SetSize(ConsoleWidgetSize);

			Vector2 ConsoleWidgetPos = Vector2(10.f, WindowSize.y - ConsoleWidgetSize.y - 10.f);
			ConsoleDialog->SetPosition(ConsoleWidgetPos);
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

	if (VoiceSetupDialog && FriendsDialog)
	{
		VoiceSetupDialog->SetSize(Vector2(FriendsDialog->GetSize().x, Height - (FriendsDialog->GetPosition().y + FriendsDialog->GetSize().y + 16.f)));
		VoiceSetupDialog->SetPosition(Vector2(FriendsDialog->GetPosition().x, FriendsDialog->GetPosition().y + FriendsDialog->GetSize().y + 6.0f));
	}

	if (AuthDialogs) AuthDialogs->UpdateLayout();
}

void FMenu::CreateVoiceDialog()
{
	const float FX = 100.0f;
	const float FY = 100.0f;
	const float Width = 300.0f;
	const float Height = 300.0f;

	VoiceDialog = std::make_shared<FVoiceDialog>(
		Vector2(FX, FY),
		Vector2(Width, Height),
		DefaultLayer - 2,
		NormalFont->GetFont(),
		BoldSmallFont->GetFont(),
		TinyFont->GetFont());

	VoiceDialog->SetBorderColor(Color::UIBorderGrey);

	VoiceDialog->Create();

	AddDialog(VoiceDialog);
}

void FMenu::CreateVoiceSetupDialog()
{
	VoiceSetupDialog = std::make_shared<FVoiceSetupDialog>(
		Vector2(200.f, 270.f),
		Vector2(370.0f, 220.0f),
		DefaultLayer - 2,
		SmallFont->GetFont(),
		SmallFont->GetFont());

	VoiceSetupDialog->Create();
	VoiceSetupDialog->SetBorderColor(Color::UIBorderGrey);

	AddDialog(VoiceSetupDialog);
}

void FMenu::OnGameEvent(const FGameEvent& Event)
{
	if (VoiceDialog)
	{
		VoiceDialog->OnGameEvent(Event);
	}

	if (VoiceSetupDialog)
	{
		VoiceSetupDialog->OnGameEvent(Event);
	}

	FBaseMenu::OnGameEvent(Event);
}

void FMenu::UpdateFriendsDialogTransform(const Vector2 WindowSize)
{
	if (FriendsDialog)
	{
		const float FX = VoiceDialog->GetPosition().x + VoiceDialog->GetSize().x + 5.0f;
		const float FY = UpperDialogsVerticalOffset;
		const float FriendsWidth = WindowSize.x - FX - 5.0f;
		const float FriendsHeight = WindowSize.y - FY - 200.0f;

		FriendsDialog->SetSize(Vector2(FriendsWidth, FriendsHeight));
		FriendsDialog->SetPosition(Vector2(FX, FY));
	}
}
