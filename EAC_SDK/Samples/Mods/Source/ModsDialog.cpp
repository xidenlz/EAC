// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"

#include "ModsDialog.h"

#include "Game.h"
#include "TextLabel.h"
#include "Button.h"
#include "UIEvent.h"
#include "GameEvent.h"
#include "AccountHelpers.h"
#include "Player.h"
#include "Sprite.h"
#include "Mods.h"
#include "TextEditor.h"
#include "TableView.h"

const float HeaderLabelHeight = 20.0f;

FModsDialog::FModsDialog(
	Vector2 DialogPos,
	Vector2 DialogSize,
	UILayer DialogLayer,
	FontPtr DialogNormalFont,
	FontPtr DialogSmallFont,
	FontPtr DialogTinyFont) :
	FDialog(DialogPos, DialogSize, DialogLayer)
{
	HeaderLabel = std::make_shared<FTextLabelWidget>(
		DialogPos,
		Vector2(500.0f, HeaderLabelHeight),
		Layer - 1,
		std::wstring(L"Mods: "),
		L"Assets/wide_label.dds",
		FColor(1.f, 1.f, 1.f, 1.f),
		FColor(1.f, 1.f, 1.f, 1.f),
		EAlignmentType::Left
		);
	HeaderLabel->SetFont(DialogNormalFont);

	BackgroundImage = std::make_shared<FSpriteWidget>(
		DialogPos,
		DialogSize,
		DialogLayer,
		L"Assets/texteditor.dds");

	EnumerateAllModsBtn = std::make_shared<FButtonWidget>(
		Vector2(Position.x, Position.y),
		Vector2(200.f, 30.f),
		Layer - 1,
		L"Enumerate All Mods",
		assets::DefaultButtonAssets,
		DialogNormalFont,
		assets::DefaultButtonColors[static_cast<size_t>(FButtonWidget::EButtonVisualState::Idle)]);
	EnumerateAllModsBtn->SetBackgroundColors(assets::DefaultButtonColors);

	EnumerateInstalledModsBtn = std::make_shared<FButtonWidget>(
		Vector2(Position.x, Position.y),
		Vector2(200.f, 30.f),
		Layer - 1,
		L"Enumerate Installed Mods",
		assets::DefaultButtonAssets,
		DialogNormalFont,
		assets::DefaultButtonColors[static_cast<size_t>(FButtonWidget::EButtonVisualState::Idle)]);
	EnumerateInstalledModsBtn->SetBackgroundColors(assets::DefaultButtonColors);
}

void FModsDialog::Update()
{
	FDialog::Update();
}

void FModsDialog::Create()
{
	if (HeaderLabel) HeaderLabel->Create();
	if (BackgroundImage) BackgroundImage->Create();
	if (EnumerateAllModsBtn)
	{
		EnumerateAllModsBtn->Create();
		EnumerateAllModsBtn->SetOnPressedCallback([this]()
		{
			FGame::Get().GetMods()->EnumerateMods(EModsQueryType::All);
		});
		EnumerateAllModsBtn->Hide();
	}
	if (EnumerateInstalledModsBtn)
	{
		EnumerateInstalledModsBtn->Create();
		EnumerateInstalledModsBtn->SetOnPressedCallback([this]()
		{
			FGame::Get().GetMods()->EnumerateMods(EModsQueryType::Installed);
		});
		EnumerateInstalledModsBtn->Hide();
	}

	AddWidget(HeaderLabel);
	AddWidget(BackgroundImage);
	AddWidget(EnumerateAllModsBtn);
	AddWidget(EnumerateInstalledModsBtn);
}

void FModsDialog::SetPosition(Vector2 Pos)
{
	IWidget::SetPosition(Pos);

	if (HeaderLabel) HeaderLabel->SetPosition(Pos);
	if (BackgroundImage) BackgroundImage->SetPosition(Vector2(Position.x, Position.y + 20.f));
	if (EnumerateAllModsBtn) EnumerateAllModsBtn->SetPosition(Vector2(Pos.x + Size.x/2.f - 100.f, Pos.y + 50.f));
	if (EnumerateInstalledModsBtn) EnumerateInstalledModsBtn->SetPosition(Vector2(Pos.x + Size.x/2.f - 100.f, Pos.y + 120.f));
}

void FModsDialog::SetSize(Vector2 NewSize)
{
	IWidget::SetSize(NewSize);
}

void FModsDialog::SetWindowSize(Vector2 WindowSize)
{
	const Vector2 DefListSize = Vector2(WindowSize.x * (1.0f - ConsoleWindowProportion.x) - 40.0f, WindowSize.y - 200.0f);
	const Vector2 DialogSize = Vector2(WindowSize.x * (1.0f - ConsoleWindowProportion.x) - 30.0f, WindowSize.y - 90.0f);

	if (BackgroundImage) BackgroundImage->SetSize(DialogSize);
	if (HeaderLabel) HeaderLabel->SetSize(Vector2(DialogSize.x, HeaderLabelHeight));
}

void FModsDialog::OnGameEvent(const FGameEvent& Event)
{
	if (Event.GetType() == EGameEventType::UserLoggedIn)
	{
		OnLoggedIn(Event.GetUserId());
	}
	else if (Event.GetType() == EGameEventType::UserLoggedOut)
	{
		OnLoggedOut();
	}
	else if (Event.GetType() == EGameEventType::ShowPrevUser)
	{
		
	}
	else if (Event.GetType() == EGameEventType::ShowNextUser)
	{
		
	}
	else if (Event.GetType() == EGameEventType::NewUserLogin)
	{
		
	}
	else if (Event.GetType() == EGameEventType::CancelLogin)
	{
		
	}
}

void FModsDialog::OnLoggedIn(FEpicAccountId UserId)
{
	if (UserId.IsValid())
	{
		CurrentUserId = UserId;
		
		if (EnumerateAllModsBtn) EnumerateAllModsBtn->Show();
		if (EnumerateInstalledModsBtn) EnumerateInstalledModsBtn->Show();
	}
}

void FModsDialog::OnLoggedOut()
{
	if (EnumerateAllModsBtn) EnumerateAllModsBtn->Hide();
	if (EnumerateInstalledModsBtn) EnumerateInstalledModsBtn->Hide();
}