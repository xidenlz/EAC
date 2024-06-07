// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "GameEvent.h"
#include "Authentication.h"
#include "ConsoleDialog.h"
#include "FriendsDialog.h"
#include "ExitDialog.h"
#include "NotificationDialog.h"
#include "AuthDialogs.h"
#include "AchievementsDefinitionsDialog.h"
#include "SampleConstants.h"
#include "Menu.h"
#include "PopupDialog.h"

const Vector2 ConsoleDialogSizeProportion = Vector2(0.68f, 0.35f);
const float AchievementDefinitionsDialogPositionY = 140.0f;

FMenu::FMenu(std::weak_ptr<FConsole> InConsole) noexcept(false):
	FBaseMenu(InConsole)
{

}

void FMenu::Create()
{
	CreateFriendsDialog();

	FBaseMenu::Create();

	AuthDialogs->SetSingleUserOnly(true);

	CreateAchievementsDefinitionsDialog();
	CreateNotificationDialog();
}

void FMenu::Release()
{
	FBaseMenu::Release();

	if (AchievementsDefinitionsDialog)
	{
		AchievementsDefinitionsDialog->Release();
		AchievementsDefinitionsDialog.reset();
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

		if (AchievementsDefinitionsDialog)
		{
			AchievementsDefinitionsDialog->SetWindowSize(WindowSize);

			Vector2 AchDefDialogSize = Vector2(ConsoleWidgetSize.x, WindowSize.y - ConsoleDialog->GetSize().y - AchievementDefinitionsDialogPositionY - 10.0f);
			AchievementsDefinitionsDialog->SetSize(AchDefDialogSize);

			Vector2 AchDefDialogPos = Vector2(ConsoleWidgetPos.x, ConsoleWidgetPos.y - AchDefDialogSize.y - 10.f);
			AchievementsDefinitionsDialog->SetPosition(AchDefDialogPos);
		}

		if (FriendsDialog)
		{
			Vector2 FriendsDialogSize = Vector2(WindowSize.x - ConsoleDialog->GetSize().x - 30.f,
				AchievementsDefinitionsDialog->GetSize().y + ConsoleDialog->GetSize().y + 10.f);
			FriendsDialog->SetSize(FriendsDialogSize);

			Vector2 FriendDialogPos = Vector2(ConsoleDialog->GetPosition().x + ConsoleDialog->GetSize().x + 10.f,
				AchievementDefinitionsDialogPositionY - 10.0f);
			FriendsDialog->SetPosition(FriendDialogPos);
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

void FMenu::CreateAchievementsDefinitionsDialog()
{
	const float FX = ConsoleDialog->GetPosition().x;
	const float FY = AchievementDefinitionsDialogPositionY;
	const float Width = ConsoleDialog->GetSize().x;
	const float Height = (ConsoleDialog->GetSize().y / ConsoleDialogSizeProportion.y) * (1.0f - ConsoleDialogSizeProportion.y);

	AchievementsDefinitionsDialog = std::make_shared<FAchievementsDefinitionsDialog>(
		Vector2(FX, FY),
		Vector2(Width, Height),
		DefaultLayer - 2,
		NormalFont->GetFont(),
		BoldSmallFont->GetFont(),
		TinyFont->GetFont());

	AchievementsDefinitionsDialog->SetWindowProportion(ConsoleDialogSizeProportion);

	AchievementsDefinitionsDialog->SetBorderColor(Color::UIBorderGrey);

	AchievementsDefinitionsDialog->Create();

	AddDialog(AchievementsDefinitionsDialog);
}

void FMenu::OnGameEvent(const FGameEvent& Event)
{
	if (Event.GetType() == EGameEventType::ToggleNotification)
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

	if (AchievementsDefinitionsDialog) AchievementsDefinitionsDialog->OnGameEvent(Event);
	if (NotificationDialog) NotificationDialog->OnGameEvent(Event);

	FBaseMenu::OnGameEvent(Event);
}