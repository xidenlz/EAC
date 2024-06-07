// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "Console.h"
#include "GameEvent.h"
#include "Authentication.h"
#include "ConsoleDialog.h"
#include "ExitDialog.h"
#include "AuthDialogs.h"
#include "TitleStorageDialog.h"
#include "TransferProgressDialog.h"
#include "SampleConstants.h"
#include "Menu.h"
#include "TitleStorage.h"
#include "PopupDialog.h"

const Vector2 ConsoleDialogSizeProportion = Vector2(0.68f, 0.35f);

FMenu::FMenu(std::weak_ptr<FConsole> InConsole) noexcept(false):
	FBaseMenu(InConsole)
{

}

void FMenu::Create()
{
	CreateTitleStorageDialog();
	CreateFileTransferDialog();
	
	FBaseMenu::Create();
}

void FMenu::Release()
{
	if (TitleStorageDialog)
	{
		TitleStorageDialog->Release();
		TitleStorageDialog.reset();
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

		if (TitleStorageDialog)
		{
			TitleStorageDialog->SetWindowSize(WindowSize);

			Vector2 TitleStorageDialogSize = Vector2(WindowSize.x - ConsoleDialog->GetSize().x - 30.f, WindowSize.y - 130.0f);
			TitleStorageDialog->SetSize(TitleStorageDialogSize);

			Vector2 TitleStorageDialogPos = Vector2(ConsoleDialog->GetPosition().x + ConsoleDialog->GetSize().x + 10.f,
				WindowSize.y - TitleStorageDialogSize.y - 10.f);
			TitleStorageDialog->SetPosition(TitleStorageDialogPos);
		}
	}

	if (TransferDialog)
	{
		TransferDialog->SetPosition(Vector2((WindowSize.x / 2.f) - TransferDialog->GetSize().x / 2.0f, (WindowSize.y / 2.f) - TransferDialog->GetSize().y));
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

void FMenu::CreateTitleStorageDialog()
{
	const float FX = 100.0f;
	const float FY = 100.0f;
	const float Width = 300.0f;
	const float Height = 300.0f;

	TitleStorageDialog = std::make_shared<FTitleStorageDialog>(
		Vector2(FX, FY),
		Vector2(Width, Height),
		DefaultLayer - 2,
		NormalFont->GetFont(),
		BoldSmallFont->GetFont(),
		TinyFont->GetFont());

	TitleStorageDialog->SetWindowProportion(ConsoleDialogSizeProportion);
	TitleStorageDialog->SetBorderColor(Color::UIBorderGrey);
	
	TitleStorageDialog->Create();

	AddDialog(TitleStorageDialog);
}

void FMenu::CreateAuthDialogs()
{
	AuthDialogs = std::make_shared<FAuthDialogs>(
		TitleStorageDialog,
		L"Title Storage",
		BoldSmallFont->GetFont(),
		SmallFont->GetFont(),
		TinyFont->GetFont());

	AuthDialogs->SetUserLabelOffset(Vector2(-30.0f, -10.0f));
	AuthDialogs->Create();
	AuthDialogs->SetSingleUserOnly(true);
}

void FMenu::CreateFileTransferDialog()
{
	class FTitleStorageProgressDelegate : public FTransferProgressDialog::FDelegate
	{
		virtual void CancelTransfer() override
		{
			FGame::Get().GetTitleStorage()->CancelCurrentTransfer();
		}

		virtual const std::wstring& GetCurrentTransferName() override
		{
			return FGame::Get().GetTitleStorage()->GetCurrentTransferName();
		}

		virtual float GetCurrentTransferProgress() override
		{
			return FGame::Get().GetTitleStorage()->GetCurrentTransferProgress();
		}
	};

	std::shared_ptr<FTransferProgressDialog::FDelegate> TitleStorageProgressDelegate = std::make_shared<FTitleStorageProgressDelegate>();

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

void FMenu::OnGameEvent(const FGameEvent& Event)
{
	if (Event.GetType() == EGameEventType::ShowPrevUser)
	{
		UpdateTitleStorage();
	}
	else if (Event.GetType() == EGameEventType::ShowNextUser)
	{
		UpdateTitleStorage();
	}
	else if (Event.GetType() == EGameEventType::CancelLogin)
	{
		UpdateTitleStorage();
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

	if (TitleStorageDialog) TitleStorageDialog->OnGameEvent(Event);

	FBaseMenu::OnGameEvent(Event);
}

void FMenu::UpdateTitleStorage()
{
	if (TitleStorageDialog)
	{
		TitleStorageDialog->SetPosition(Vector2(ConsoleDialog->GetPosition().x + ConsoleDialog->GetSize().x + 10.f,
			TitleStorageDialog->GetPosition().y));
	}
}