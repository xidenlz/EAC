// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "Menu.h"
#include "CustomInvitesDialog.h"
#include "ConsoleDialog.h"
#include "Font.h"
#include "Sprite.h"
#include "FriendsDialog.h"
#include "ExitDialog.h"
#include "AuthDialogs.h"
#include "PopupDialog.h"
#include "GameEvent.h"

FMenu::FMenu(std::weak_ptr<FConsole> InConsole) noexcept(false) :
	FBaseMenu(InConsole)
{

}

FMenu::~FMenu()
{

}

void FMenu::Create()
{
	CreateFriendsDialog();

	// creates console dialog, which is needed for creating custom invites dialog
	FBaseMenu::Create();

	CreateCustomInvitesDialog();
}

void FMenu::Release()
{
	if (CustomInvitesDialog)
	{
		CustomInvitesDialog->Release();
		CustomInvitesDialog.reset();
	}

	FBaseMenu::Release();
}

void FMenu::OnGameEvent(const FGameEvent& Event)
{
	if (CustomInvitesDialog)
	{
		CustomInvitesDialog->OnGameEvent(Event);
	}

	FBaseMenu::OnGameEvent(Event);
}

void FMenu::CreateCustomInvitesDialog()
{
	Vector2 DummyVector{};

	CustomInvitesDialog = std::make_shared<FCustomInvitesDialog>(
		DummyVector,
		DummyVector,
		DefaultLayer - 2,
		NormalFont->GetFont(),
		BoldSmallFont->GetFont(),
		TinyFont->GetFont());

	AddDialog(CustomInvitesDialog);
	std::shared_ptr<FMenu> MenuPtr = std::static_pointer_cast<FMenu>(shared_from_this());
	CustomInvitesDialog->SetOuterMenu(std::weak_ptr<FMenu>(MenuPtr));
}

// custom invite / console frame
#define CUSTOMINVITE_CONSOLE_FRAME_WIDTH	.7f
#define CUSTOMINVITE_CONSOLE_FRAME_HEIGHT	0.99f
#define CUSTOMINVITE_CONSOLE_FRAME_SCALE	Vector2(CUSTOMINVITE_CONSOLE_FRAME_WIDTH, CUSTOMINVITE_CONSOLE_FRAME_HEIGHT)

// friends frame
#define FRIENDS_FRAME_WIDTH					1.f - CUSTOMINVITE_CONSOLE_FRAME_WIDTH
#define FRIENDS_FRAME_HEIGHT				0.99f
#define FRIENDS_FRAME_SCALE					Vector2(FRIENDS_FRAME_WIDTH, FRIENDS_FRAME_HEIGHT)

// custom invite dialog (inside custom invite / console frame)
#define CUSTOMINVITES_DIALOG_PERCENT_X		1.f
#define CUSTOMINVITES_DIALOG_PERCENT_Y		.29f
#define CUSTOMINVITE_DIALOG_SCALE			Vector2(CUSTOMINVITES_DIALOG_PERCENT_X, CUSTOMINVITES_DIALOG_PERCENT_Y)

// console dialog (inside custom invite / console frame)
#define CONSOLE_DIALOG_PERCENT_X			1.f
#define CONSOLE_DIALOG_PERCENT_Y			1.f - CUSTOMINVITES_DIALOG_PERCENT_Y
#define CONSOLE_DIALOG_SCALE				Vector2(CONSOLE_DIALOG_PERCENT_X, CONSOLE_DIALOG_PERCENT_Y)

// friends dialog (inside friends frame)
#define FRIENDS_USAGE_PERCENT_X				1.f
#define FRIENDS_USAGE_PERCENT_Y				1.f
#define FRIENDS_DIALOG_SCALE				Vector2(FRIENDS_USAGE_PERCENT_X, FRIENDS_USAGE_PERCENT_Y)

void FMenu::UpdateLayout(int Width, int Height)
{
	WindowSize = Vector2((float)Width, (float)Height);
	Vector2 LayoutPositionMarker{ };

	BackgroundImage->SetPosition(LayoutPositionMarker);
	BackgroundImage->SetSize(Vector2((float)Width, ((float)Height) / 2.f));

	// move the layout cursor down 10px, recalculate available percentage
	LayoutPositionMarker += Vector2(10.f, 120.f);
	const Vector2 MainFrameOrigin{ LayoutPositionMarker };
	const Vector2 MainFrameSize{ WindowSize - MainFrameOrigin };

	const Vector2 CustomInviteConsoleFrameOrigin{ MainFrameOrigin };
	const Vector2 CustomInviteConsoleFrameSize{ Vector2::CoeffProduct(CUSTOMINVITE_CONSOLE_FRAME_SCALE, MainFrameSize) };

	const Vector2 CustomInviteDialogSize = Vector2::CoeffProduct(CUSTOMINVITE_DIALOG_SCALE, CustomInviteConsoleFrameSize);
	if (CustomInvitesDialog)
	{
		CustomInvitesDialog->SetSize(CustomInviteDialogSize);
		CustomInvitesDialog->SetPosition(CustomInviteConsoleFrameOrigin);
	}
	LayoutPositionMarker.y += CustomInviteDialogSize.y;

	const Vector2 ConsoleWidgetSize = Vector2::CoeffProduct(CONSOLE_DIALOG_SCALE, CustomInviteConsoleFrameSize);
	if (ConsoleDialog)
	{
		ConsoleDialog->SetSize(ConsoleWidgetSize);
		ConsoleDialog->SetPosition(LayoutPositionMarker);
	}

	LayoutPositionMarker = MainFrameOrigin;
	LayoutPositionMarker.x += CustomInviteConsoleFrameSize.x;

	const Vector2 FriendsFrameOrigin{ LayoutPositionMarker };
	const Vector2 FriendsFrameSize = Vector2::CoeffProduct(FRIENDS_FRAME_SCALE, MainFrameSize);

	const Vector2 FriendsWidgetSize = Vector2::CoeffProduct(FRIENDS_DIALOG_SCALE, FriendsFrameSize);
	if (FriendsDialog)
	{
		FriendsDialog->SetPosition(LayoutPositionMarker);
		FriendsDialog->SetSize(FriendsWidgetSize);
	}

	if (PopupDialog)
	{
		PopupDialog->SetPosition(Vector2((WindowSize.x / 2.f) - PopupDialog->GetSize().x / 2.0f, (WindowSize.y / 2.f) - PopupDialog->GetSize().y + 130.0f));
	}

	if (ExitDialog)
	{
		ExitDialog->SetPosition(Vector2((WindowSize.x / 2.f) - ExitDialog->GetSize().x / 2.0f, (WindowSize.y / 2.f) - ExitDialog->GetSize().y + 130.0f));
	}

	if (AuthDialogs)
	{
		AuthDialogs->UpdateLayout();
	}
}