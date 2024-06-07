// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "Console.h"
#include "GameEvent.h"
#include "Authentication.h"
#include "ConsoleDialog.h"
#include "FriendsDialog.h"
#include "ExitDialog.h"
#include "AuthDialogs.h"
#include "SampleConstants.h"
#include "Menu.h"
#include "P2PNATDialog.h"
#include "Utils/Utils.h"
#include "GUI/Button.h"
#include "PopupDialog.h"

const char* NATDocsURL = "";

const Vector2 ConsoleDialogSizeProportion = Vector2(0.68f, 0.35f);
const float P2PDialogPositionY = 140.0f;

FMenu::FMenu(std::weak_ptr<FConsole> InConsole) noexcept(false):
	FBaseMenu(InConsole)
{

}

void FMenu::Create()
{
	CreateFriendsDialog();
	CreateP2PNATDialog();
	CreateNATDocsButton();

	FBaseMenu::Create();

	AuthDialogs->SetSingleUserOnly(true);
}

void FMenu::Release()
{
	NATDocsButton->Release();

	FBaseMenu::Release();
}

void FMenu::Update()
{
	NATDocsButton->Update();

	FBaseMenu::Update();
}

void FMenu::Render(FSpriteBatchPtr& Batch)
{
	if (NATDocsButton)
	{
		NATDocsButton->Render(Batch);
	}

	FBaseMenu::Render(Batch);
}

#ifdef _DEBUG
void FMenu::DebugRender()
{
	if (NATDocsButton)
	{
		NATDocsButton->DebugRender();
	}

	FBaseMenu::DebugRender();
}
#endif

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

		if (P2PNATDialog)
		{
			Vector2 P2PDialogSize = Vector2(ConsoleWidgetSize.x, WindowSize.y - ConsoleDialog->GetSize().y - P2PDialogPositionY - 10.0f);
			P2PNATDialog->SetSize(P2PDialogSize);

			Vector2 P2PDialogPos = Vector2(ConsoleWidgetPos.x,
				ConsoleWidgetPos.y - P2PDialogSize.y - 10.f);
			P2PNATDialog->SetPosition(P2PDialogPos);
		}

		if (FriendsDialog)
		{
			Vector2 FriendsDialogSize = Vector2(WindowSize.x - ConsoleDialog->GetSize().x - 30.f,
				P2PNATDialog->GetSize().y + ConsoleDialog->GetSize().y + 10.f);
			FriendsDialog->SetSize(FriendsDialogSize);

			Vector2 FriendDialogPos = Vector2(ConsoleDialog->GetPosition().x + ConsoleDialog->GetSize().x + 10.f,
				P2PDialogPositionY - 10.0f);
			FriendsDialog->SetPosition(FriendDialogPos);
		}
	}

	if (NATDocsButton)
	{
		NATDocsButton->SetSize(Vector2(150.0f, 35.0f));

		if (FriendsDialog)
		{
			NATDocsButton->SetPosition(FriendsDialog->GetPosition() + Vector2(FriendsDialog->GetSize().x - NATDocsButton->GetSize().x, -70.f));
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

void FMenu::CreateP2PNATDialog()
{
	const float FX = 100.0f;
	const float FY = P2PDialogPositionY;
	const float Width = 300.0f;
	const float Height = 300.0f;

	P2PNATDialog = std::make_shared<FP2PNATDialog>(
		Vector2(FX, FY),
		Vector2(Width, Height),
		DefaultLayer - 2,
		NormalFont->GetFont(),
		BoldSmallFont->GetFont(),
		TinyFont->GetFont(),
		LargeFont->GetFont());

	P2PNATDialog->SetBorderColor(Color::UIBorderGrey);

	P2PNATDialog->Create();

	AddDialog(P2PNATDialog);
}

void FMenu::CreateNATDocsButton()
{
	Vector2 NATDocsButtonSize = Vector2(150.0f, 35.0f);

	NATDocsButton = std::make_shared<FButtonWidget>(
		Vector2(800.0f, 100.0f),
		NATDocsButtonSize,
		DefaultLayer - 1,
		L"DOCUMENTATION",
		assets::LargeButtonAssets,
		NormalFont->GetFont());
	NATDocsButton->SetOnPressedCallback([]()
	{
		FUtils::OpenURL(NATDocsURL);
	});
	//NATDocsButton->SetBackgroundColors(assets::DefaultButtonColors);
	NATDocsButton->SetBorderColor(Color::UIBorderGrey);
	NATDocsButton->Create();
	NATDocsButton->Hide();
}

void FMenu::OnUIEvent(const FUIEvent& Event)
{
	if (Event.GetType() == EUIEventType::MousePressed || Event.GetType() == EUIEventType::MouseReleased)
	{
		if (NATDocsButton->CheckCollision(Event.GetVector()))
		{
			NATDocsButton->OnUIEvent(Event);
		}
	}

	FBaseMenu::OnUIEvent(Event);
}

void FMenu::OnGameEvent(const FGameEvent& Event)
{
	FBaseMenu::OnGameEvent(Event);
	if (P2PNATDialog) P2PNATDialog->OnGameEvent(Event);
}