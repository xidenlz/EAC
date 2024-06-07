// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "Game.h"
#include "TextLabel.h"
#include "Button.h"
#include "UIEvent.h"
#include "GameEvent.h"
#include "AccountHelpers.h"
#include "Player.h"
#include "Sprite.h"
#include "PlayerDataStorageDialog.h"
#include "TextEditor.h"
#include "PlayerDataStorage.h"

template<>
std::shared_ptr<FPlayerDataStorageInfo> CreateListEntry(Vector2 Pos, Vector2 Size, UILayer Layer, const std::wstring& Data)
{
	return std::make_shared<FPlayerDataStorageInfo>(Pos, Size, Layer, Data, L"Assets/black_grey_button.dds");
}

const Vector2 FileEditorPosition = Vector2(10.0f, 120.0f);

FPlayerDataStorageDialog::FPlayerDataStorageDialog(
	Vector2 DialogPos,
	Vector2 DialogSize,
	UILayer DialogLayer,
	FontPtr DialogNormalFont,
	FontPtr DialogSmallFont,
	FontPtr DialogTinyFont) :
	FDialog(DialogPos, DialogSize, DialogLayer)
{
	HeaderLabel = std::make_shared<FTextLabelWidget>(
		FileEditorPosition,
		Vector2(500.0f, 20.0f),
		Layer - 1,
		std::wstring(L"Player Data: "),
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

	PlayerDataList = std::make_shared<FPlayerDataList>(
		DialogPos,
		DialogSize,
		DialogLayer - 1,
		20.0f, //entry height
		20.0f, //label height
		15.0f, //scroller width
		L"", //background
		L"",
		DialogNormalFont,
		DialogNormalFont,
		DialogSmallFont,
		DialogTinyFont);

	TextEditorLabel = std::make_shared<FTextLabelWidget>(
		FileEditorPosition,
		Vector2(500.0f, 20.0f),
		Layer - 1,
		L"FILE",
		L"Assets/wide_label.dds",
		FColor(1.f, 1.f, 1.f, 1.f),
		FColor(1.f, 1.f, 1.f, 1.f),
		EAlignmentType::Left
		);
	TextEditorLabel->SetFont(DialogSmallFont);
	TextEditorLabel->SetBorderColor(Color::UIBorderGrey);

	TextEditor = std::make_shared<FTextEditorWidget>(
		FileEditorPosition + Vector2(0.0f, 20.0f),
		Vector2(500.0f, 500.0f),
		DialogLayer - 1,
		L"",
		L"Assets/texteditor.dds",
		DialogNormalFont
		);
	TextEditor->SetBorderOffsets(Vector2(10.0f, 10.0f));
	TextEditor->SetBorderColor(Color::UIBorderGrey);

	Vector2 RefreshButtonSize(20.0f, 20.0f);
	Vector2 ButtonSize((DialogSize.x - 15.0f) / 4.0f, 35.0f);
	Vector2 ButtonVerticalOffset(0.0f, 25.0f);
	Vector2 ButtonHorizontalOffset(ButtonSize.x + 4.0f, 0.0f);

	RefreshListButton = std::make_shared<FButtonWidget>(
		DialogPos + Vector2(HeaderLabel->GetSize().x - 20.0f, 0.0f),
		RefreshButtonSize,
		DialogLayer - 1,
		L"",
		std::vector<std::wstring>({ L"Assets/refresh.dds" }),
		DialogSmallFont,
		Color::White,
		Color::White
		);
	RefreshListButton->SetOnPressedCallback([]()
	{
		//send refresh request
		FGame::Get().GetPlayerDataStorage()->QueryList();
	});
	RefreshListButton->SetBackgroundColors(assets::DefaultButtonColors);

	DownloadFileButton = std::make_shared<FButtonWidget>(
		DialogPos - ButtonVerticalOffset + ButtonHorizontalOffset,
		ButtonSize,
		DialogLayer - 1,
		L"DOWNLOAD",
		assets::LargeButtonAssets,
		DialogTinyFont,
		Color::White,
		Color::White
		);
	DownloadFileButton->SetOnPressedCallback([this]()
	{
		//get selected file name
		const std::wstring& CurrentSelectionFileName = this->GetCurrentSelection();

		if (!CurrentSelectionFileName.empty())
		{
			//send download request
			FGame::Get().GetPlayerDataStorage()->StartFileDataDownload(CurrentSelectionFileName);
		}
	});
	DownloadFileButton->SetBackgroundColors(assets::DefaultButtonColors);

	SaveFileButton = std::make_shared<FButtonWidget>(
		DialogPos - ButtonVerticalOffset + ButtonHorizontalOffset * 2,
		ButtonSize,
		DialogLayer - 1,
		L"SAVE",
		assets::LargeButtonAssets,
		DialogTinyFont,
		Color::White,
		Color::White
		);
	SaveFileButton->SetOnPressedCallback([this]()
	{
		//get selected file name
		const std::wstring& CurrentSelectionFileName = this->GetCurrentSelection();

		if (!CurrentSelectionFileName.empty())
		{
			//set local data and upload it
			FGame::Get().GetPlayerDataStorage()->AddFile(CurrentSelectionFileName, this->GetEditorContents());
		}
	});
	SaveFileButton->SetBackgroundColors(assets::DefaultButtonColors);

	DuplicateFileButton = std::make_shared<FButtonWidget>(
		DialogPos - ButtonVerticalOffset + ButtonHorizontalOffset * 3,
		ButtonSize,
		DialogLayer - 1,
		L"DUPLICATE",
		assets::LargeButtonAssets,
		DialogTinyFont,
		Color::White,
		Color::White
		);
	DuplicateFileButton->SetOnPressedCallback([this]()
	{
		//get selected file name
		const std::wstring& CurrentSelectionFileName = this->GetCurrentSelection();

		if (!CurrentSelectionFileName.empty())
		{
			//send copy request
			//TODO: add text field to enter new name
			FGame::Get().GetPlayerDataStorage()->CopyFile(CurrentSelectionFileName, CurrentSelectionFileName + L"_Copy");
		}
	});
	DuplicateFileButton->SetBackgroundColors(assets::DefaultButtonColors);

	DeleteFileButton = std::make_shared<FButtonWidget>(
		DialogPos - ButtonVerticalOffset + ButtonHorizontalOffset * 4,
		ButtonSize,
		DialogLayer - 1,
		L"DELETE",
		assets::LargeButtonAssets,
		DialogTinyFont,
		Color::White,
		Color::White
		);
	DeleteFileButton->SetOnPressedCallback([this]()
	{
		//get selected file name
		const std::wstring& CurrentSelectionFileName = this->GetCurrentSelection();

		if (!CurrentSelectionFileName.empty())
		{
			//send removal request
			FGame::Get().GetPlayerDataStorage()->RemoveFile(CurrentSelectionFileName);
			this->ClearCurrentSelection();
		}
	});
	DeleteFileButton->SetBackgroundColors(assets::DefaultButtonColors);
}

void FPlayerDataStorageDialog::Update()
{
	NameList = FGame::Get().GetPlayerDataStorage()->GetFileList();
	NameList.push_back(L"New File");

	if (PlayerDataList)
	{
		PlayerDataList->RefreshData(NameList);
	}

	FDialog::Update();

	if (PlayerDataList)
	{
		PlayerDataList->Update();
	}
}

void FPlayerDataStorageDialog::Create()
{
	if (HeaderLabel) HeaderLabel->Create();
	if (BackgroundImage) BackgroundImage->Create();
	if (PlayerDataList) PlayerDataList->Create();
	if (TextEditorLabel) TextEditorLabel->Create();
	if (TextEditor)	TextEditor->Create();

	//make entries invisible until player logs in
	if (PlayerDataList)
	{
		PlayerDataList->SetEntriesVisible(false);
		PlayerDataList->SetOnEntrySelectedCallback([this](size_t Index) { this->OnDataStorageEntrySelected(Index); });
	}

	if (RefreshListButton) RefreshListButton->Create();
	if (DownloadFileButton) DownloadFileButton->Create();
	if (SaveFileButton) SaveFileButton->Create();
	if (DuplicateFileButton) DuplicateFileButton->Create();
	if (DeleteFileButton) DeleteFileButton->Create();

	AddWidget(HeaderLabel);
	AddWidget(BackgroundImage);
	AddWidget(PlayerDataList);
	AddWidget(TextEditorLabel);
	AddWidget(TextEditor);

	AddWidget(RefreshListButton);
	AddWidget(DownloadFileButton);
	AddWidget(SaveFileButton);
	AddWidget(DuplicateFileButton);
	AddWidget(DeleteFileButton);
}

void FPlayerDataStorageDialog::SetPosition(Vector2 Pos)
{
	IWidget::SetPosition(Pos);

	if (HeaderLabel) HeaderLabel->SetPosition(Pos);
	if (BackgroundImage) BackgroundImage->SetPosition(Vector2(Position.x, Position.y + 20.f));
	if (TextEditorLabel) TextEditorLabel->SetPosition(Vector2(FileEditorPosition.x, FileEditorPosition.y));
	if (TextEditor) TextEditor->SetPosition(Vector2(FileEditorPosition.x, FileEditorPosition.y + TextEditorLabel->GetSize().y));

	const Vector2 RefreshButtonSize(20.0f, 20.0f);

	if (RefreshListButton) RefreshListButton->SetPosition(Pos + Vector2(HeaderLabel->GetSize().x - RefreshButtonSize.x, 0.0f));

	Vector2 ButtonSize((GetSize().x - 12) / 4.0f, 35.0f);
	Vector2 ButtonVerticalOffset(0.0f, 25.0f);
	Vector2 InitialButtonHorizOffset(2.0f, 0.0f);
	Vector2 ButtonHorizontalOffset(ButtonSize.x + 3.0f, 0.0f);

	Vector2 CurrentButtonPos = Pos + ButtonVerticalOffset + InitialButtonHorizOffset;

	if (SaveFileButton) SaveFileButton->SetPosition(CurrentButtonPos);
	CurrentButtonPos = CurrentButtonPos + ButtonHorizontalOffset;
	if (DownloadFileButton) DownloadFileButton->SetPosition(CurrentButtonPos);
	CurrentButtonPos = CurrentButtonPos + ButtonHorizontalOffset;
	if (DuplicateFileButton) DuplicateFileButton->SetPosition(CurrentButtonPos);
	CurrentButtonPos = CurrentButtonPos + ButtonHorizontalOffset;
	if (DeleteFileButton) DeleteFileButton->SetPosition(CurrentButtonPos);

	if (PlayerDataList) PlayerDataList->SetPosition(Position + Vector2(3.0f, HeaderLabel->GetSize().y + 4.0f));
}

void FPlayerDataStorageDialog::SetSize(Vector2 NewSize)
{
	IWidget::SetSize(NewSize);

	Vector2 ButtonSize((GetSize().x - 12) / 4.0f, 35.0f);
	if (SaveFileButton) SaveFileButton->SetSize(ButtonSize);
	if (DownloadFileButton) DownloadFileButton->SetSize(ButtonSize);
	if (DuplicateFileButton) DuplicateFileButton->SetSize(ButtonSize);
	if (DeleteFileButton) DeleteFileButton->SetSize(ButtonSize);
}

void FPlayerDataStorageDialog::SetWindowSize(Vector2 WindowSize)
{
	const Vector2 PlayerListSize = Vector2(WindowSize.x * (1.0f - ConsoleWindowProportion.x) - 40.0f, WindowSize.y - 200.0f - FileEditorPosition.y);
	const Vector2 DialogSize = Vector2(WindowSize.x * (1.0f - ConsoleWindowProportion.x) - 30.0f, WindowSize.y - 90.0f);

	if (TextEditorLabel) TextEditorLabel->SetSize(Vector2(ConsoleWindowProportion.x * WindowSize.x, 20.0f));
	if (TextEditor) TextEditor->SetSize(Vector2(ConsoleWindowProportion.x * WindowSize.x, (1.0f - ConsoleWindowProportion.y) * WindowSize.y - FileEditorPosition.y - 50.0f));
	if (BackgroundImage) BackgroundImage->SetSize(DialogSize);
	if (PlayerDataList) PlayerDataList->SetSize(PlayerListSize);
	if (HeaderLabel) HeaderLabel->SetSize(Vector2(DialogSize.x, 20.0f));
}

void FPlayerDataStorageDialog::UpdateUserInfo()
{
	if (HeaderLabel)
	{
		HeaderLabel->SetText(L"Player Data: log in to proceed.");
		if (FPlayerManager::Get().GetCurrentUser().IsValid())
		{
			std::wstring DisplayName = FPlayerManager::Get().GetDisplayName(FPlayerManager::Get().GetCurrentUser());
			HeaderLabel->SetText(std::wstring(L"Player Data: ") + DisplayName);
		}
	}
}

void FPlayerDataStorageDialog::ClearDataList()
{
	PlayerDataList->Reset();
	TextEditor->Clear();
}

void FPlayerDataStorageDialog::ShowDataList()
{
	PlayerDataList->SetEntriesVisible(true);
}

void FPlayerDataStorageDialog::HideDataList()
{
	PlayerDataList->SetEntriesVisible(false);
	SetFocused(false);
}

void FPlayerDataStorageDialog::OnDataStorageEntrySelected(size_t Index)
{
	if (Index >= NameList.size())
		return;

	//New file selected?
	bool bNewFileSelected = (Index == NameList.size() - 1);

	std::wstring Name = NameList[Index];

	if (!CurrentSelection.empty())
	{
		FGame::Get().GetPlayerDataStorage()->SetLocalData(CurrentSelection, TextEditor->GetText());
		CurrentSelection.clear();
	}

	TextEditor->Clear();

	if (!bNewFileSelected)
	{
		CurrentSelection = Name;

		bool bNoData = false;
		std::wstring Data = FGame::Get().GetPlayerDataStorage()->GetLocalData(Name, bNoData);

		if (!bNoData)
		{
			TextEditor->SetText(Data);
			TextEditor->SetEditingEnabled(true);
		}
		else
		{
			TextEditor->SetText(L"* No local data available for the file selected.\n   Please press 'DOWNLOAD' to get data from the cloud. *");
			TextEditor->SetEditingEnabled(false);
		}
	}
	else
	{
		//New File
		FGameEvent NewFileEvent(EGameEventType::NewFileCreationStarted);
		FGame::Get().OnGameEvent(NewFileEvent);
	}
}

std::wstring FPlayerDataStorageDialog::GetEditorContents() const
{
	if (TextEditor)
	{
		return TextEditor->GetText();
	}

	return std::wstring();
}

void FPlayerDataStorageDialog::ClearCurrentSelection()
{
	CurrentSelection.clear();
	if (PlayerDataList)
	{
		PlayerDataList->SetFocused(false);
	}
	if (TextEditor)
	{
		TextEditor->Clear();
	}
}

void FPlayerDataStorageDialog::OnGameEvent(const FGameEvent& Event)
{
	if (Event.GetType() == EGameEventType::UserLoggedIn)
	{
		UpdateUserInfo();
		ShowDataList();
	}
	else if (Event.GetType() == EGameEventType::UserLoginRequiresMFA)
	{
		UpdateUserInfo();
		PlayerDataList->SetEntriesVisible(false);
		SetFocused(false);
	}
	else if (Event.GetType() == EGameEventType::UserLoginEnteredMFA)
	{
		UpdateUserInfo();
		PlayerDataList->SetEntriesVisible(false);
	}
	else if (Event.GetType() == EGameEventType::UserLoggedOut)
	{
		UpdateUserInfo();
		if (FPlayerManager::Get().GetNumPlayers() == 0)
		{
			ClearDataList();
			HideDataList();
		}
	}
	else if (Event.GetType() == EGameEventType::ShowPrevUser)
	{
		ClearDataList();
		UpdateUserInfo();
	}
	else if (Event.GetType() == EGameEventType::ShowNextUser)
	{
		ClearDataList();
		UpdateUserInfo();
	}
	else if (Event.GetType() == EGameEventType::NewUserLogin)
	{
		UpdateUserInfo();
		ClearDataList();
		HideDataList();
	}
	else if (Event.GetType() == EGameEventType::CancelLogin)
	{
		ShowDataList();
		UpdateUserInfo();
	}
	else if (Event.GetType() == EGameEventType::FileTransferFinished)
	{
		const std::wstring& FileName = Event.GetFirstStr();
		if (FileName == CurrentSelection && TextEditor)
		{
			bool NoData = false;
			std::wstring Data = FGame::Get().GetPlayerDataStorage()->GetLocalData(FileName, NoData);
			TextEditor->Clear();
			TextEditor->SetText(Data);
			TextEditor->SetEditingEnabled(true);
		}
	}
}

void FPlayerDataStorageInfo::SetFocused(bool bValue)
{
	if (BackgroundImage && bValue != IsFocused())
	{
		FColor CurrentColor = BackgroundImage->GetBackgroundColor();

		const float Diff = 0.3f;
		FColor Adjustment = (bValue) ? FColor(Diff, Diff, Diff, Diff) : FColor(-Diff, -Diff, -Diff, 0.0f);

		CurrentColor.A += Adjustment.A;
		CurrentColor.R += Adjustment.R;
		CurrentColor.G += Adjustment.G;
		CurrentColor.B += Adjustment.B;

		BackgroundImage->SetBackgroundColor(CurrentColor);
	}

	FTextLabelWidget::SetFocused(bValue);
}
