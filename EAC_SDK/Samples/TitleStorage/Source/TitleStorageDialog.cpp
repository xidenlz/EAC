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
#include "TitleStorageDialog.h"
#include "TextEditor.h"
#include "TextField.h"
#include "TitleStorage.h"

template<>
std::shared_ptr<FTitleStorageInfo> CreateListEntry(Vector2 Pos, Vector2 Size, UILayer Layer, const std::wstring& Data)
{
	return std::make_shared<FTitleStorageInfo>(Pos, Size, Layer, Data, L"Assets/black_grey_button.dds");
}

const Vector2 FileEditorPosition = Vector2(10.0f, 120.0f);

FTitleStorageDialog::FTitleStorageDialog(
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
		std::wstring(L"Title Storage: "),
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
	TextEditor->SetEditingEnabled(false);
	TextEditor->SetCanSelectText(true);

	Vector2 ButtonSize((DialogSize.x - 15.0f) / 4.0f, 35.0f);
	Vector2 ButtonVerticalOffset(0.0f, 37.0f);
	Vector2 ButtonHorizontalOffset(ButtonSize.x + 4.0f, 0.0f);

	FileNameTextField = std::make_shared<FTextFieldWidget>(
		DialogPos - ButtonVerticalOffset,
		Vector2(ButtonSize.x * 3.0f, ButtonSize.y),
		DialogLayer - 1,
		L"Enter file name...",
		L"Assets/textfield.dds",
		DialogSmallFont,
		FTextFieldWidget::EInputType::Normal,
		EAlignmentType::Left,
		FColor(0.5f, 0.5f, 0.5f, 1.f)
		);
	FileNameTextField->SetOnEnterPressedCallback([this](const std::wstring& SearchString)
	{
		if (!SearchString.empty())
		{
			CurrentSelection = SearchString;
			FGame::Get().GetTitleStorage()->StartFileDataDownload(SearchString);
		}
		FileNameTextField->Clear();
	});

	DownloadFileButton = std::make_shared<FButtonWidget>(
		DialogPos - ButtonVerticalOffset + ButtonHorizontalOffset * 3.0f,
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
		std::wstring CurrentSelectionFileName = GetCurrentSelection();

		//check if text field has a value
		if (FileNameTextField)
		{
			const std::wstring& TextFieldValue = FileNameTextField->GetText();
			if (!TextFieldValue.empty() && TextFieldValue != FileNameTextField->GetInitialText())
			{
				CurrentSelectionFileName = TextFieldValue;
				FileNameTextField->Clear();
			}
		}

		if (!CurrentSelectionFileName.empty())
		{
			//send download request
			FGame::Get().GetTitleStorage()->StartFileDataDownload(CurrentSelectionFileName);
		}
	});
	DownloadFileButton->SetBackgroundColors(assets::DefaultButtonColors);

	TitleDataList = std::make_shared<FTitleDataList>(
		DialogPos,
		DialogSize,
		DialogLayer - 1,
		20.0f, //entry height
		15.0f, //label height
		15.0f, //scroller width
		L"", //background
		L"Files:",
		DialogNormalFont,
		DialogNormalFont,
		DialogSmallFont,
		DialogTinyFont);

	TagNameTextField = std::make_shared<FTextFieldWidget>(
		FileNameTextField->GetPosition() + ButtonVerticalOffset,
		Vector2(ButtonSize.x * 3.0f, ButtonSize.y),
		DialogLayer - 1,
		L"Enter tag name...",
		L"Assets/textfield.dds",
		DialogSmallFont,
		FTextFieldWidget::EInputType::Normal,
		EAlignmentType::Left,
		FColor(0.5f, 0.5f, 0.5f, 1.f)
		);
	TagNameTextField->SetOnEnterPressedCallback([this](const std::wstring& TagString)
	{
		if (!TagString.empty())
		{
			FGame::Get().GetTitleStorage()->AddTag(FStringUtils::Narrow(TagString));
		}
		TagNameTextField->Clear();
	});

	AddTagButton = std::make_shared<FButtonWidget>(
		DownloadFileButton->GetPosition() + ButtonVerticalOffset + ButtonHorizontalOffset * 3.0f,
		ButtonSize,
		DialogLayer - 1,
		L"ADD TAG",
		assets::LargeButtonAssets,
		DialogTinyFont,
		Color::White,
		Color::White
		);
	AddTagButton->SetOnPressedCallback([this]()
	{
		//check if text field has a value
		if (TagNameTextField)
		{
			const std::wstring& TextFieldValue = TagNameTextField->GetText();
			if (!TextFieldValue.empty() && TextFieldValue != TagNameTextField->GetInitialText())
			{
				TagNameTextField->Clear();

				FGame::Get().GetTitleStorage()->AddTag(FStringUtils::Narrow(TextFieldValue));
			}
		}
	});
	AddTagButton->SetBackgroundColors(assets::DefaultButtonColors);

	TagList = std::make_shared<FTitleTagList>(
		TagNameTextField->GetPosition() + ButtonVerticalOffset,
		DialogSize,
		DialogLayer - 1,
		20.0f, //entry height
		15.0f, //label height
		15.0f, //scroller width
		L"", //background
		L"Tags:",
		DialogNormalFont,
		DialogNormalFont,
		DialogSmallFont,
		DialogTinyFont);

	QueryListButton = std::make_shared<FButtonWidget>(
		TagList->GetPosition() + Vector2(0.0f, TagList->GetSize().y),
		ButtonSize,
		DialogLayer - 1,
		L"QUERY LIST",
		assets::LargeButtonAssets,
		DialogTinyFont,
		Color::White,
		Color::White
		);
	QueryListButton->SetOnPressedCallback([]()
	{
		//send refresh request
		FGame::Get().GetTitleStorage()->QueryList();
	});
	QueryListButton->SetBackgroundColors(assets::DefaultButtonColors);

	ClearTagsButton = std::make_shared<FButtonWidget>(
		QueryListButton->GetPosition() + ButtonHorizontalOffset,
		ButtonSize,
		DialogLayer - 1,
		L"CLEAR TAGS",
		assets::LargeButtonAssets,
		DialogTinyFont,
		Color::White,
		Color::White
		);
	ClearTagsButton->SetOnPressedCallback([this]()
	{
		FGame::Get().GetTitleStorage()->ClearTags();
		TagList->Reset();
	});
	ClearTagsButton->SetBackgroundColors(assets::DefaultButtonColors);
}

void FTitleStorageDialog::Update()
{
	NameList = FGame::Get().GetTitleStorage()->GetFileList();
	std::set<std::string> TagSet = FGame::Get().GetTitleStorage()->GetCurrentTags();
	Tags.clear();
	for (const std::string& Tag : TagSet)
	{
		Tags.push_back(FStringUtils::Widen(Tag));
	}

	if (TitleDataList)
	{
		TitleDataList->RefreshData(NameList);
	}

	if (TagList)
	{
		TagList->RefreshData(Tags);
	}

	FDialog::Update();

	if (TitleDataList)
	{
		TitleDataList->Update();
	}

	if (TagList)
	{
		TagList->Update();
	}
}

void FTitleStorageDialog::Create()
{
	if (HeaderLabel) HeaderLabel->Create();
	if (BackgroundImage) BackgroundImage->Create();
	if (TitleDataList) TitleDataList->Create();
	if (TextEditorLabel) TextEditorLabel->Create();
	if (TextEditor)	TextEditor->Create();

	//make entries invisible until player logs in
	if (TitleDataList)
	{
		TitleDataList->SetEntriesVisible(false);
		TitleDataList->SetOnEntrySelectedCallback([this](size_t Index) { this->OnDataStorageEntrySelected(Index); });
	}

	if (QueryListButton) QueryListButton->Create();
	if (DownloadFileButton) DownloadFileButton->Create();
	if (FileNameTextField) FileNameTextField->Create();

	if (AddTagButton) AddTagButton->Create();
	if (TagNameTextField) TagNameTextField->Create();
	if (ClearTagsButton) ClearTagsButton->Create();

	if (TagList) TagList->Create();

	if (TagList)
	{
		TagList->SetEntriesVisible(false);
	}

	AddWidget(HeaderLabel);
	AddWidget(BackgroundImage);
	AddWidget(TitleDataList);
	AddWidget(TextEditorLabel);
	AddWidget(TextEditor);

	AddWidget(QueryListButton);
	AddWidget(FileNameTextField);
	AddWidget(DownloadFileButton);

	AddWidget(AddTagButton);
	AddWidget(TagNameTextField);
	AddWidget(ClearTagsButton);
	AddWidget(TagList);

	HideWidgets();
}

void FTitleStorageDialog::SetPosition(Vector2 Pos)
{
	IWidget::SetPosition(Pos);

	if (HeaderLabel) HeaderLabel->SetPosition(Pos);
	if (BackgroundImage) BackgroundImage->SetPosition(Vector2(Position.x, Position.y + 20.f));
	if (TextEditorLabel) TextEditorLabel->SetPosition(Vector2(FileEditorPosition.x, FileEditorPosition.y));
	if (TextEditor) TextEditor->SetPosition(Vector2(FileEditorPosition.x, FileEditorPosition.y + TextEditorLabel->GetSize().y));

	Vector2 ButtonSize((GetSize().x - 12) / 4.0f, 35.0f);
	Vector2 ButtonVerticalOffset(0.0f, 37.0f);
	Vector2 InitialButtonHorizOffset(2.0f, 0.0f);
	Vector2 ButtonHorizontalOffset(ButtonSize.x + 1.0f, 0.0f);

	Vector2 CurrentButtonPos = Pos + ButtonVerticalOffset + InitialButtonHorizOffset;

	if (FileNameTextField) FileNameTextField->SetPosition(CurrentButtonPos);
	CurrentButtonPos += (ButtonHorizontalOffset * 3.0f);
	if (DownloadFileButton) DownloadFileButton->SetPosition(CurrentButtonPos);
	CurrentButtonPos -= (ButtonHorizontalOffset * 3.0f);
	CurrentButtonPos += ButtonVerticalOffset;
	if (TagNameTextField) TagNameTextField->SetPosition(CurrentButtonPos);
	
	CurrentButtonPos += (ButtonHorizontalOffset * 3.0f);
	if (AddTagButton) AddTagButton->SetPosition(CurrentButtonPos);

	CurrentButtonPos -= (ButtonHorizontalOffset * 3.0f);
	CurrentButtonPos += ButtonVerticalOffset;
	if (TagList) TagList->SetPosition(CurrentButtonPos);
	CurrentButtonPos += Vector2(0.0f, TagList->GetSize().y + 30.0f);

	if (QueryListButton) QueryListButton->SetPosition(CurrentButtonPos);

	CurrentButtonPos += ButtonHorizontalOffset;
	if (ClearTagsButton) ClearTagsButton->SetPosition(CurrentButtonPos);

	CurrentButtonPos -= ButtonHorizontalOffset;
	CurrentButtonPos += ButtonVerticalOffset;

	if (TitleDataList) TitleDataList->SetPosition(CurrentButtonPos);
}

void FTitleStorageDialog::SetSize(Vector2 NewSize)
{
	IWidget::SetSize(NewSize);

	Vector2 ButtonSize((GetSize().x - 12) / 4.0f, 35.0f);
	if (DownloadFileButton) DownloadFileButton->SetSize(ButtonSize);
	if (FileNameTextField) FileNameTextField->SetSize(Vector2(ButtonSize.x * 3.0f, ButtonSize.y));
	if (TagNameTextField) TagNameTextField->SetSize(Vector2(ButtonSize.x * 3.0f, ButtonSize.y));
	if (AddTagButton) AddTagButton->SetSize(ButtonSize);
	if (TagList) TagList->SetSize(Vector2(GetSize().x, 100.f));
	if (QueryListButton) QueryListButton->SetSize(ButtonSize);
	if (ClearTagsButton) ClearTagsButton->SetSize(ButtonSize);
	if (TitleDataList)TitleDataList->SetSize(Vector2(NewSize.x, NewSize.y - (TitleDataList->GetPosition().y - Position.y) - 100.0f));
}

void FTitleStorageDialog::SetWindowSize(Vector2 WindowSize)
{
	const Vector2 DialogSize = Vector2(WindowSize.x * (1.0f - ConsoleWindowProportion.x) - 30.0f, WindowSize.y - 90.0f);

	if (TextEditorLabel) TextEditorLabel->SetSize(Vector2(ConsoleWindowProportion.x * WindowSize.x, 20.0f));
	if (TextEditor) TextEditor->SetSize(Vector2(ConsoleWindowProportion.x * WindowSize.x, (1.0f - ConsoleWindowProportion.y) * WindowSize.y - FileEditorPosition.y - 50.0f));
	if (BackgroundImage) BackgroundImage->SetSize(DialogSize);
	if (HeaderLabel) HeaderLabel->SetSize(Vector2(DialogSize.x, 20.0f));
}

void FTitleStorageDialog::UpdateUserInfo()
{
	if (HeaderLabel)
	{
		HeaderLabel->SetText(L"Title Storage: log in to proceed.");
		if (FPlayerManager::Get().GetCurrentUser().IsValid())
		{
			std::wstring DisplayName = FPlayerManager::Get().GetDisplayName(FPlayerManager::Get().GetCurrentUser());
			HeaderLabel->SetText(std::wstring(L"Title Storage: ") + DisplayName);
		}
	}
}

void FTitleStorageDialog::ClearDataLists()
{
	TitleDataList->Reset();
	TextEditor->Clear();

	TagList->Reset();
}

void FTitleStorageDialog::ShowDataLists()
{
	TitleDataList->SetEntriesVisible(true);
	TagList->SetEntriesVisible(true);
}

void FTitleStorageDialog::HideDataLists()
{
	TitleDataList->SetEntriesVisible(false);
	TagList->SetEntriesVisible(false);
	SetFocused(false);
}

void FTitleStorageDialog::ShowWidgets()
{
	ShowDataLists();

	TitleDataList->Show();
	TagList->Show();

	FileNameTextField->Show();
	QueryListButton->Show();
	DownloadFileButton->Show();
	TagNameTextField->Show();
	AddTagButton->Show();
	ClearTagsButton->Show();
}

void FTitleStorageDialog::HideWidgets()
{
	HideDataLists();

	TitleDataList->Hide();
	TagList->Hide();

	FileNameTextField->Hide();
	QueryListButton->Hide();
	DownloadFileButton->Hide();
	TagNameTextField->Hide();
	AddTagButton->Hide();
	ClearTagsButton->Hide();
}

void FTitleStorageDialog::OnDataStorageEntrySelected(size_t Index)
{
	if (Index >= NameList.size())
		return;

	std::wstring Name = NameList[Index];

	TextEditor->Clear();

	CurrentSelection = Name;

	bool NoData = false;
	std::wstring Data = FGame::Get().GetTitleStorage()->GetLocalData(Name, NoData);

	if (!NoData)
	{
		TextEditor->SetText(Data);
	}
	else
	{
		TextEditor->SetText(L"* No local data available for the file selected.\n   Please press 'DOWNLOAD' to get data from the cloud. *");
	}
	TextEditor->SetEditingEnabled(false);

	FileNameTextField->SetText(CurrentSelection);
}

std::wstring FTitleStorageDialog::GetEditorContents() const
{
	if (TextEditor)
	{
		return TextEditor->GetText();
	}

	return std::wstring();
}

void FTitleStorageDialog::ClearCurrentSelection()
{
	CurrentSelection.clear();
	if (TitleDataList)
	{
		TitleDataList->SetFocused(false);
	}
	if (TextEditor)
	{
		TextEditor->Clear();
	}
}

void FTitleStorageDialog::OnGameEvent(const FGameEvent& Event)
{
	if (Event.GetType() == EGameEventType::UserLoggedIn)
	{
		UpdateUserInfo();
		ShowWidgets();
	}
	else if (Event.GetType() == EGameEventType::UserLoginRequiresMFA)
	{
		UpdateUserInfo();
		HideWidgets();
	}
	else if (Event.GetType() == EGameEventType::UserLoginEnteredMFA)
	{
		UpdateUserInfo();
		HideWidgets();
	}
	else if (Event.GetType() == EGameEventType::UserLoggedOut)
	{
		UpdateUserInfo();
		if (FPlayerManager::Get().GetNumPlayers() == 0)
		{
			ClearDataLists();
			HideWidgets();
		}
	}
	else if (Event.GetType() == EGameEventType::ShowPrevUser)
	{
		ClearDataLists();
		UpdateUserInfo();
	}
	else if (Event.GetType() == EGameEventType::ShowNextUser)
	{
		ClearDataLists();
		UpdateUserInfo();
	}
	else if (Event.GetType() == EGameEventType::NewUserLogin)
	{
		UpdateUserInfo();
		ClearDataLists();
		HideWidgets();
	}
	else if (Event.GetType() == EGameEventType::CancelLogin)
	{
		ShowDataLists();
		UpdateUserInfo();
	}
	else if (Event.GetType() == EGameEventType::FileTransferFinished)
	{
		const std::wstring& FileName = Event.GetFirstStr();
		if (FileName == CurrentSelection && TextEditor)
		{
			bool NoData = false;
			std::wstring Data = FGame::Get().GetTitleStorage()->GetLocalData(FileName, NoData);
			TextEditor->Clear();
			TextEditor->SetText(Data);
		}
	}
}

void FTitleStorageInfo::SetFocused(bool bValue)
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
