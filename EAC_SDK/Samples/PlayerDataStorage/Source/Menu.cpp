// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "Console.h"
#include "GameEvent.h"
#include "Authentication.h"
#include "ConsoleDialog.h"
#include "ExitDialog.h"
#include "AuthDialogs.h"
#include "PlayerDataStorageDialog.h"
#include "TransferProgressDialog.h"
#include "NewFileDialog.h"
#include "SampleConstants.h"
#include "Menu.h"
#include "PlayerDataStorage.h"
#include "PopupDialog.h"

const Vector2 ConsoleDialogSizeProportion = Vector2(0.68f, 0.35f);

FMenu::FMenu(std::weak_ptr<FConsole> InConsole) noexcept(false):
	FBaseMenu(InConsole)
{

}

void FMenu::Create()
{
	CreatePlayerDataStorageDialog();
	CreateFileTransferDialog();
	CreateNewFileDialog();

	FBaseMenu::Create();
}

void FMenu::Release()
{
	if (PlayerDataStorageDialog)
	{
		PlayerDataStorageDialog->Release();
		PlayerDataStorageDialog.reset();
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

		if (PlayerDataStorageDialog)
		{
			PlayerDataStorageDialog->SetWindowSize(WindowSize);

			Vector2 PlayerDataStorageDialogSize = Vector2(WindowSize.x - ConsoleDialog->GetSize().x - 30.f, WindowSize.y - 130.0f);
			PlayerDataStorageDialog->SetSize(PlayerDataStorageDialogSize);

			Vector2 PlayerDataStorageDialogPos = Vector2(ConsoleDialog->GetPosition().x + ConsoleDialog->GetSize().x + 10.f,
				WindowSize.y - PlayerDataStorageDialogSize.y - 10.f);
			PlayerDataStorageDialog->SetPosition(PlayerDataStorageDialogPos);
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

	if (TransferDialog)
	{
		TransferDialog->SetPosition(Vector2((WindowSize.x / 2.f) - TransferDialog->GetSize().x / 2.0f, (WindowSize.y / 2.f) - TransferDialog->GetSize().y));
	}

	if (NewFileDialog)
	{
		NewFileDialog->SetPosition(Vector2((WindowSize.x / 2.f) - NewFileDialog->GetSize().x / 2.0f, (WindowSize.y / 2.f) - NewFileDialog->GetSize().y));
	}

	if (AuthDialogs) AuthDialogs->UpdateLayout();
}

void FMenu::CreatePlayerDataStorageDialog()
{
	const float FX = 100.0f;
	const float FY = 100.0f;
	const float Width = 300.0f;
	const float Height = 300.0f;

	PlayerDataStorageDialog = std::make_shared<FPlayerDataStorageDialog>(
		Vector2(FX, FY),
		Vector2(Width, Height),
		DefaultLayer - 2,
		NormalFont->GetFont(),
		BoldSmallFont->GetFont(),
		TinyFont->GetFont());

	PlayerDataStorageDialog->SetWindowProportion(ConsoleDialogSizeProportion);
	PlayerDataStorageDialog->SetBorderColor(Color::UIBorderGrey);
	
	PlayerDataStorageDialog->Create();

	AddDialog(PlayerDataStorageDialog);
}

void FMenu::CreateFileTransferDialog()
{
	class FPlayerDataStorageProgressDelegate : public FTransferProgressDialog::FDelegate
	{
		virtual void CancelTransfer() override
		{
			FGame::Get().GetPlayerDataStorage()->CancelCurrentTransfer();
		}

		virtual const std::wstring& GetCurrentTransferName() override
		{
			return FGame::Get().GetPlayerDataStorage()->GetCurrentTransferName();
		}

		virtual float GetCurrentTransferProgress() override
		{
			return FGame::Get().GetPlayerDataStorage()->GetCurrentTransferProgress();
		}
	};

	std::shared_ptr<FTransferProgressDialog::FDelegate> TitleStorageProgressDelegate = std::make_shared<FPlayerDataStorageProgressDelegate>();

	TransferDialog = std::make_shared<FTransferProgressDialog>(
		Vector2(200.f, 200.f),
		Vector2(330.f, 160.f),
		5,
		L"",
		NormalFont->GetFont(),
		SmallFont->GetFont(),
		TitleStorageProgressDelegate);

	TransferDialog->SetBorderColor(Color::UIBorderGrey);
	TransferDialog->Create();

	AddDialog(TransferDialog);

	HideDialog(TransferDialog);
}


void FMenu::CreateNewFileDialog()
{
	NewFileDialog = std::make_shared<FNewFileDialog>(
		Vector2(200.f, 200.f),
		Vector2(330.f, 200.0f),
		5,
		NormalFont->GetFont(),
		SmallFont->GetFont());

	NewFileDialog->SetBorderColor(Color::UIBorderGrey);
	NewFileDialog->Create();

	AddDialog(NewFileDialog);

	HideDialog(NewFileDialog);
}

void FMenu::OnGameEvent(const FGameEvent& Event)
{
	FBaseMenu::OnGameEvent(Event);

	if (Event.GetType() == EGameEventType::ShowPrevUser)
	{
		UpdatePlayerDataStorage();
	}
	else if (Event.GetType() == EGameEventType::ShowNextUser)
	{
		UpdatePlayerDataStorage();
	}
	else if (Event.GetType() == EGameEventType::CancelLogin)
	{
		UpdatePlayerDataStorage();
	}
	else if (Event.GetType() == EGameEventType::FileTransferStarted)
	{
		const std::wstring& TransferName = Event.GetFirstStr();
		if (TransferDialog)
		{
			TransferDialog->SetTransferName(TransferName);
			ShowDialog(TransferDialog);
		}
	}
	else if (Event.GetType() == EGameEventType::FileTransferFinished)
	{
		const std::wstring& TransferName = Event.GetFirstStr();
		if (TransferDialog)
		{
			if (TransferDialog->GetTransferName() == TransferName)
			{
				HideDialog(TransferDialog);
			}
		}
	}
	else if (Event.GetType() == EGameEventType::NewFileCreationStarted)
	{
		if (NewFileDialog)
		{
			ShowDialog(NewFileDialog);
		}
	}

	if (PlayerDataStorageDialog) PlayerDataStorageDialog->OnGameEvent(Event);
}

void FMenu::CreateAuthDialogs()
{
	AuthDialogs = std::make_shared<FAuthDialogs>(
		PlayerDataStorageDialog,
		L"Player Data Storage",
		BoldSmallFont->GetFont(),
		SmallFont->GetFont(),
		TinyFont->GetFont());

	AuthDialogs->SetUserLabelOffset(Vector2(-30.0f, -10.0f));
	AuthDialogs->Create();
	AuthDialogs->SetSingleUserOnly(true);
}

void FMenu::UpdatePlayerDataStorage()
{
	if (PlayerDataStorageDialog)
	{
		PlayerDataStorageDialog->SetPosition(Vector2(ConsoleDialog->GetPosition().x + ConsoleDialog->GetSize().x + 10.f,
			PlayerDataStorageDialog->GetPosition().y));
	}
}