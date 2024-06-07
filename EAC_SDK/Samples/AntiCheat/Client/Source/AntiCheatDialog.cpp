// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"

#include "Game.h"
#include "Authentication.h"
#include "AntiCheatDialog.h"
#include "AntiCheatClient.h"
#include "DebugLog.h"
#include "GameEvent.h"
#include "Player.h"
#include "Sprite.h"

constexpr float HeaderLabelHeight = 30.0f;

FAntiCheatDialog::FAntiCheatDialog(
	Vector2 InPosition,
	Vector2 InSize,
	UILayer InLayer,
	FontPtr DialogNormalFont,
	FontPtr DialogSmallFont)
	: FDialog(InPosition, InSize, InLayer)
{
	HeaderLabel = std::make_shared<FTextLabelWidget>(
		Position,
		Vector2(Size.x, HeaderLabelHeight),
		Layer - 1,
		L"ANTICHEAT",
		L"Assets/dialog_title.dds",
		FColor(1.f, 1.f, 1.f, 1.f),
		FColor(1.f, 1.f, 1.f, 1.f),
		EAlignmentType::Left
		);
	HeaderLabel->SetFont(DialogSmallFont);
	HeaderLabel->SetBorderColor(Color::UIBorderGrey);

	BackgroundImage = std::make_shared<FSpriteWidget>(
		Position,
		Size,
		Layer,
		L"Assets/texteditor.dds");

	IPLabel = std::make_shared<FTextLabelWidget>(
		Vector2(50.f, 50.f),
		Vector2(100.f, 30.f),
		Layer - 1,
		L"IP:",
		L"");
	IPLabel->SetFont(DialogNormalFont);

	PortLabel = std::make_shared<FTextLabelWidget>(
		Vector2(50.f, 50.f),
		Vector2(100.f, 30.f),
		Layer - 1,
		L"Port:",
		L"");
	PortLabel->SetFont(DialogNormalFont);

	JoinGameButton = std::make_shared<FButtonWidget>(
		Vector2(0.f, 0.f),
		Vector2(0, 0.f),
		Layer - 1,
		L"JOIN GAME",
		assets::DefaultButtonAssets,
		DialogNormalFont,
		Color::UIButtonBlue
		);
	JoinGameButton->SetBackgroundColors(assets::DefaultButtonColors);
	JoinGameButton->SetOnPressedCallback([this]()
	{
		OnJoinGameButtonPressed();
	});

	LeaveGameButton = std::make_shared<FButtonWidget>(
		Vector2(0.f, 0.f),
		Vector2(0, 0.f),
		Layer - 1,
		L"LEAVE GAME",
		assets::DefaultButtonAssets,
		DialogNormalFont,
		Color::UIButtonBlue
		);
	LeaveGameButton->SetBackgroundColors(assets::DefaultButtonColors);
	LeaveGameButton->SetOnPressedCallback([this]()
	{
		OnLeaveGameButtonPressed();
	});
	LeaveGameButton->Disable();

	PollStatusButton = std::make_shared<FButtonWidget>(
		Vector2(0.f, 0.f),
		Vector2(0, 0.f),
		Layer - 1,
		L"POLL STATUS",
		assets::DefaultButtonAssets,
		DialogNormalFont,
		Color::UIButtonBlue
		);
	PollStatusButton->SetBackgroundColors(assets::DefaultButtonColors);
	PollStatusButton->SetOnPressedCallback([this]()
	{
		OnPollStatusButtonPressed();
	});
	PollStatusButton->Disable();

	IPField = std::make_shared<FTextFieldWidget>(
		Vector2(50.f, 50.f),
		Vector2(150.f, 30.f),
		Layer - 1,
		L"127.0.0.1",
		L"Assets/textfield.dds",
		DialogNormalFont,
		FTextFieldWidget::EInputType::Normal,
		EAlignmentType::Left);
	IPField->SetBorderColor(Color::UIBorderGrey);

	PortField = std::make_shared<FTextFieldWidget>(
		Vector2(50.f, 50.f),
		Vector2(150.f, 30.f),
		Layer - 1,
		L"1234",
		L"Assets/textfield.dds",
		DialogNormalFont,
		FTextFieldWidget::EInputType::Normal,
		EAlignmentType::Left);
	PortField->SetBorderColor(Color::UIBorderGrey);

	HideUI();
}

void FAntiCheatDialog::SetWindowProportion(Vector2 InWindowProportion)
{
	ConsoleWindowProportion = InWindowProportion;
}

void FAntiCheatDialog::SetWindowSize(Vector2 WindowSize)
{
	const Vector2 DefListSize = Vector2(WindowSize.x * (1.0f - ConsoleWindowProportion.x) - 40.0f, WindowSize.y - 200.0f);
	const Vector2 DialogSize = Vector2(WindowSize.x * (1.0f - ConsoleWindowProportion.x) - 30.0f, WindowSize.y - 90.0f);

	if (HeaderLabel) HeaderLabel->SetSize(Vector2(DialogSize.x, HeaderLabelHeight));
	if (BackgroundImage) BackgroundImage->SetSize(DialogSize);
}

void FAntiCheatDialog::Create()
{
	if (HeaderLabel) HeaderLabel->Create();
	if (BackgroundImage) BackgroundImage->Create();
	if (JoinGameButton) JoinGameButton->Create();
	if (LeaveGameButton) JoinGameButton->Create();
	if (PollStatusButton) PollStatusButton->Create();
	if (IPLabel) IPLabel->Create();
	if (IPField) IPField->Create();
	if (PortLabel) PortLabel->Create();
	if (PortField) PortField->Create();

	AddWidget(HeaderLabel);
	AddWidget(BackgroundImage);
	AddWidget(JoinGameButton);
	AddWidget(LeaveGameButton);
	AddWidget(PollStatusButton);
	AddWidget(IPLabel);
	AddWidget(IPField);
	AddWidget(PortLabel);
	AddWidget(PortField);
}

void FAntiCheatDialog::SetPosition(Vector2 Pos)
{
	IWidget::SetPosition(Pos);

	if (HeaderLabel) HeaderLabel->SetPosition(Pos);
	if (BackgroundImage) BackgroundImage->SetPosition(Vector2(Position.x, Position.y));
	if (IPLabel) IPLabel->SetPosition(Pos + Vector2(0.f, 80.f));
	if (IPField) IPField->SetPosition(Pos + Vector2(90.f, 80.f));
	if (PortLabel) PortLabel->SetPosition(Pos + Vector2(0.f, 120.f));
	if (PortField) PortField->SetPosition(Pos + Vector2(90.f, 120.f));
	if (JoinGameButton) JoinGameButton->SetPosition(PortLabel->GetPosition() + Vector2(35.f, 60.f));
	if (LeaveGameButton) LeaveGameButton->SetPosition(JoinGameButton->GetPosition() + Vector2(0.f, 40.f));
	if (PollStatusButton) PollStatusButton->SetPosition(LeaveGameButton->GetPosition() + Vector2(0.f, 80.f));
}

void FAntiCheatDialog::SetSize(Vector2 NewSize)
{
	IWidget::SetSize(NewSize);

	if (HeaderLabel) HeaderLabel->SetSize(Vector2(NewSize.x, HeaderLabelHeight));
	if (BackgroundImage) BackgroundImage->SetSize(Vector2(NewSize.x, NewSize.y));
	if (JoinGameButton) JoinGameButton->SetSize(Vector2(220.f, 30.f));
	if (LeaveGameButton) LeaveGameButton->SetSize(Vector2(220.f, 30.f));
	if (PollStatusButton) PollStatusButton->SetSize(Vector2(220.f, 30.f));
	if (IPLabel) IPLabel->SetSize(Vector2(100.f, 30.f));
	if (IPField) IPField->SetSize(Vector2(160.f, 30.f));
	if (PortLabel) PortLabel->SetSize(Vector2(100.f, 30.f));
	if (PortField) PortField->SetSize(Vector2(160.f, 30.f));
}

void FAntiCheatDialog::ShowUI()
{
	if (IPLabel)
	{
		IPLabel->Enable();
		IPLabel->Show();
	}

	if (IPField)
	{
		IPField->Enable();
		IPField->Show();
	}

	if (PortLabel)
	{
		PortLabel->Enable();
		PortLabel->Show();
	}

	if (PortField)
	{
		PortField->Enable();
		PortField->Show();
	}

	if (JoinGameButton)
	{
		JoinGameButton->Show();
	}

	if (LeaveGameButton)
	{
		LeaveGameButton->Show();
	}

	if (PollStatusButton)
	{
		PollStatusButton->Show();
	}
}

void FAntiCheatDialog::HideUI()
{
	if (IPLabel)
	{
		IPLabel->Hide();
	}

	if (IPField)
	{
		IPField->Disable();
		IPField->Hide();
	}

	if (PortLabel)
	{
		PortLabel->Hide();
	}

	if (PortField)
	{
		PortField->Disable();
		PortField->Hide();
	}

	if (JoinGameButton)
	{
		JoinGameButton->Hide();
	}

	if (LeaveGameButton)
	{
		LeaveGameButton->Hide();
	}

	if (PollStatusButton)
	{
		PollStatusButton->Hide();
	}

	SetFocused(false);
}

void FAntiCheatDialog::OnGameEvent(const FGameEvent& Event)
{
	if (Event.GetType() == EGameEventType::UserLoggedIn)
	{
		ShowUI();
	}
	else if (Event.GetType() == EGameEventType::UserLoginRequiresMFA)
	{
		SetFocused(false);
	}
	else if (Event.GetType() == EGameEventType::UserLoginEnteredMFA)
	{
	}
	else if (Event.GetType() == EGameEventType::UserLoggedOut)
	{
		LeaveGame();
		HideUI();
	}
	else if (Event.GetType() == EGameEventType::ShowPrevUser)
	{
	}
	else if (Event.GetType() == EGameEventType::ShowNextUser)
	{
	}
	else if (Event.GetType() == EGameEventType::NewUserLogin)
	{
		HideUI();
	}
	else if (Event.GetType() == EGameEventType::CancelLogin)
	{
		ShowUI();
	}
	else if (Event.GetType() == EGameEventType::AntiCheatKicked)
	{
		LeaveGame();
	}
}

void FAntiCheatDialog::OnJoinGameButtonPressed()
{
	const PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (!Player)
	{
		FDebugLog::LogError(L"AntiCheatDialog - OnJoinGameButtonPressed: Current player is invalid!");
		return;
	}

	const std::string IP = FStringUtils::Narrow(IPField->GetText());
	const int Port = std::stoi(PortField->GetText());

	// Get a Connect ID Token which will be sent to the server as part of the registration message.
	EOS_ProductUserId ProductUserId = Player->GetProductUserID();
	std::string ConnectIdToken = FGame::Get().GetAuthentication()->GetConnectIdToken(ProductUserId);
	if (ConnectIdToken.empty())
	{
		FDebugLog::LogError(L"AntiCheatDialog - OnJoinGameButtonPressed: Failed to get Connect ID Token!");
		return;
	}

	const bool bDidSessionBegin = FGame::Get().GetAntiCheatClient()->Start(IP, Port, Player->GetProductUserID(), ConnectIdToken.c_str());
	if (bDidSessionBegin)
	{
		JoinGameButton->Disable();
		JoinGameButton->SetFocused(false);

		LeaveGameButton->Enable();
		LeaveGameButton->SetFocused(true);

		PollStatusButton->Enable();
	}
}

void FAntiCheatDialog::OnLeaveGameButtonPressed()
{
	LeaveGame();	
}

void FAntiCheatDialog::OnPollStatusButtonPressed()
{
	FGame::Get().GetAntiCheatClient()->PollStatus();
}

void FAntiCheatDialog::LeaveGame()
{
	FGame::Get().GetAntiCheatClient()->Stop();

	LeaveGameButton->Disable();
	LeaveGameButton->SetFocused(false);

	PollStatusButton->Disable();

	JoinGameButton->Enable();
	JoinGameButton->SetFocused(true);
}
