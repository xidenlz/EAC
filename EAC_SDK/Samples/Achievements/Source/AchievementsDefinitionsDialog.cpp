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
#include "AchievementsDefinitionsDialog.h"
#include "Achievements.h"
#include "Utils/Utils.h"

template<>
std::shared_ptr<FAchievementsDefinitionName> CreateListEntry(Vector2 Pos, Vector2 Size, UILayer Layer, const std::wstring& Data)
{
	return std::make_shared<FAchievementsDefinitionName>(Pos, Size, Layer, Data, L"Assets/black_grey_button.dds");
}

void FAchievementsDefinitionName::SetFocused(bool bValue)
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

void FAchievementsDefinitionName::SetData(const std::wstring& Data)
{
	if (Font)
	{
		const Vector2 TextSize = Font->MeasureString(Data.c_str());
		const float TextWidth = TextSize.x;
		const float TextHeight = TextSize.y;
		const float TextFieldWidth = Size.x - HorizontalOffset;

		//We need to truncate
		if (TextFieldWidth < TextWidth && TextFieldWidth > 0.0f)
		{
			const size_t NumTotalChars = Data.size();
			const size_t NumCharsThatCanFitApproximately = static_cast<size_t>((TextFieldWidth / TextWidth) * NumTotalChars) - 1;

			//Don't truncate when we have small number of characters
			if (NumTotalChars > 10)
			{
				std::wstring TruncatedString;

				//Take first 30% of text
				const size_t FirstPartOfTextSize = static_cast<size_t>(NumCharsThatCanFitApproximately * 0.3);
				const size_t LastPartOfTextSize = NumCharsThatCanFitApproximately - FirstPartOfTextSize - 3; //3 is for ... chars

				TruncatedString = Data.substr(0, FirstPartOfTextSize) + L"..." + Data.substr(NumTotalChars - LastPartOfTextSize);

				SetText(TruncatedString);
				return;
			}
		}
	}

	SetText(Data);
}

template<>
std::shared_ptr<FAchievementsDefinitionInfo> CreateListEntry(Vector2 Pos, Vector2 Size, UILayer Layer, const std::wstring& Data)
{
	return std::make_shared<FAchievementsDefinitionInfo>(Pos, Size, Layer, Data, L"Assets/black_grey_button.dds");
}

void FAchievementsDefinitionInfo::SetFocused(bool bValue)
{
	
}

const Vector2 DefinitionsPosition = Vector2(10.0f, 270.0f);

FAchievementsDefinitionsDialog::FAchievementsDefinitionsDialog(
	Vector2 DialogPos,
	Vector2 DialogSize,
	UILayer DialogLayer,
	FontPtr DialogNormalFont,
	FontPtr DialogSmallFont,
	FontPtr DialogTinyFont) :
	FDialog(DialogPos, DialogSize, DialogLayer),
	CurrentSelectionIndex(-1),
	CurrentLanguageIndex(0),
	UpdateStatsIngestButtonCounter(0)
{
	HeaderLabel = std::make_shared<FTextLabelWidget>(
		DefinitionsPosition,
		Vector2(DialogSize.x, 25.0f),
		Layer - 1,
		std::wstring(L"Achievements"),
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
		L"Assets/console.dds",
		FColor(1.f, 1.f, 1.f, 1.f));

	AchievementsDefinitionNames = std::make_shared<FAchievementsDefinitionNamesList>(
		DialogPos,
		DialogSize,
		DialogLayer - 1,
		20.0f, // entry height
		20.0f, // label height
		15.0f, // scroller width
		L"", // background
		L"Definitions", // title text
		DialogNormalFont, // normal text
		DialogNormalFont, // title
		DialogSmallFont,
		DialogTinyFont);

	AchievementsDefinitionsList = std::make_shared<FAchievementsDefinitionsList>(
		DialogPos,
		DialogSize,
		DialogLayer - 1,
		20.0f, // entry height
		20.0f, // label height
		15.0f, // scroller width
		L"", // background
		L"Selected Definition Info", //title text
		DialogTinyFont, // normal text
		DialogNormalFont, // title
		DialogTinyFont,
		DialogTinyFont);

	LanguageLabel = std::make_shared<FTextLabelWidget>(
		DefinitionsPosition,
		Vector2(DialogSize.x, 25.0f),
		Layer - 1,
		std::wstring(L"Language: en"),
		L"",
		FColor(1.f, 1.f, 1.f, 1.f),
		FColor(1.f, 1.f, 1.f, 1.f),
		EAlignmentType::Left
		);
	LanguageLabel->SetFont(DialogSmallFont);
	LanguageLabel->Hide();

	UnlockAchievementButton = std::make_shared<FButtonWidget>(
		Vector2(100.0f, 0.0f),
		Vector2(70, 25.f),
		Layer - 1,
		L"Unlock",
		assets::DefaultButtonAssets,
		DialogSmallFont,
		FColor(0.5f, 0.5f, 0.5f, 1.f)
		);
	UnlockAchievementButton->SetBackgroundColors(assets::DefaultButtonColors);
	UnlockAchievementButton->SetOnPressedCallback([this]()
	{
		UnlockSelectedAchievement();
	});
	UnlockAchievementButton->Disable();

	UpdatePlayerAchievementsButton = std::make_shared<FButtonWidget>(
		Vector2(100.0f, 0.0f),
		Vector2(70, 25.f),
		Layer - 1,
		L"Update",
		assets::DefaultButtonAssets,
		DialogSmallFont,
		FColor(0.5f, 0.5f, 0.5f, 1.f)
		);
	UpdatePlayerAchievementsButton->SetBackgroundColors(assets::DefaultButtonColors);
	UpdatePlayerAchievementsButton->SetOnPressedCallback([this]()
	{
		FGame::Get().GetAchievements()->QueryPlayerAchievements(FProductUserId());
		FGame::Get().GetAchievements()->QueryStats();
	});
	UpdatePlayerAchievementsButton->Disable();

	StatsIngestLabel = std::make_shared<FTextLabelWidget>(
		Vector2(50.f, 50.f),
		Vector2(100.f, 30.f),
		Layer - 1,
		L"Stats:",
		L"");
	StatsIngestLabel->SetFont(DialogSmallFont);

	StatsIngestNameField = std::make_shared<FTextFieldWidget>(
		Vector2(50.f, 50.f),
		Vector2(150.f, 30.f),
		Layer - 1,
		L"Name",
		L"Assets/textfield.dds",
		DialogSmallFont,
		FTextFieldWidget::EInputType::Normal,
		EAlignmentType::Left);
	StatsIngestNameField->SetBorderColor(Color::UIBorderGrey);
	StatsIngestNameField->Disable();

	StatsIngestAmountField = std::make_shared<FTextFieldWidget>(
		Vector2(50.f, 50.f),
		Vector2(80.f, 30.f),
		Layer - 1,
		L"Amount",
		L"Assets/textfield.dds",
		DialogSmallFont,
		FTextFieldWidget::EInputType::Normal,
		EAlignmentType::Left);
	StatsIngestAmountField->SetBorderColor(Color::UIBorderGrey);
	StatsIngestAmountField->Disable();

	StatsIngestButton = std::make_shared<FButtonWidget>(
		Vector2(100.0f, 0.0f),
		Vector2(60, 25.f),
		Layer - 1,
		L"Ingest Stat",
		assets::DefaultButtonAssets,
		DialogSmallFont,
		FColor(0.5f, 0.5f, 0.5f, 1.f)
		);
	StatsIngestButton->SetBackgroundColors(assets::DefaultButtonColors);
	StatsIngestButton->SetOnPressedCallback([this]()
	{
		StatsIngest();
	});
	StatsIngestButton->Disable();

	LockedIconStateLabel = std::make_shared<FTextLabelWidget>(
		Vector2(50.f, 50.f),
		Vector2(50.0f, 50.f),
		Layer - 1,
		L"Locked icon: NA",
		L"");
	LockedIconStateLabel->SetFont(DialogSmallFont);

	UnlockedIconStateLabel = std::make_shared<FTextLabelWidget>(
		Vector2(50.f, 50.f),
		Vector2(50.0f, 50.f),
		Layer - 1,
		L"Unlocked icon: NA",
		L"");
	UnlockedIconStateLabel->SetFont(DialogSmallFont);

	CurrentDefinition = std::make_shared<FAchievementsDefinitionData>();
}

void FAchievementsDefinitionsDialog::Update()
{
	FDialog::Update();

	if (AchievementsDefinitionNames)
	{
		AchievementsDefinitionNames->Update();
	}

	if (AchievementsDefinitionsList)
	{
		AchievementsDefinitionsList->Update();
	}

	UpdateStatsIngestButtonState();

	//Update Icon UI
	UpdateIcons();
}

void FAchievementsDefinitionsDialog::Create()
{
	if (HeaderLabel) HeaderLabel->Create();
	if (BackgroundImage) BackgroundImage->Create();
	if (LanguageLabel) LanguageLabel->Create();

	if (AchievementsDefinitionNames)
	{
		AchievementsDefinitionNames->Create();
		AchievementsDefinitionNames->SetOnEntrySelectedCallback([this](size_t Index) { this->OnDefinitionNamesEntrySelected(Index); });
	}

	if (AchievementsDefinitionsList)
	{
		AchievementsDefinitionsList->Create();
	}

	if (UnlockAchievementButton) UnlockAchievementButton->Create();
	if (UpdatePlayerAchievementsButton) UpdatePlayerAchievementsButton->Create();

	if (StatsIngestLabel) StatsIngestLabel->Create();
	if (StatsIngestNameField) StatsIngestNameField->Create();
	if (StatsIngestAmountField) StatsIngestAmountField->Create();
	if (StatsIngestButton) StatsIngestButton->Create();

	if (LockedIconStateLabel) LockedIconStateLabel->Create();
	if (UnlockedIconStateLabel) UnlockedIconStateLabel->Create();

	AddWidget(HeaderLabel);
	AddWidget(BackgroundImage);
	AddWidget(AchievementsDefinitionNames);
	AddWidget(AchievementsDefinitionsList);
	AddWidget(LanguageLabel);
	AddWidget(UnlockAchievementButton);
	AddWidget(UpdatePlayerAchievementsButton);
	AddWidget(StatsIngestLabel);
	AddWidget(StatsIngestNameField);
	AddWidget(StatsIngestAmountField);
	AddWidget(StatsIngestButton);
	AddWidget(LockedIconStateLabel);
	AddWidget(UnlockedIconStateLabel);
}

void FAchievementsDefinitionsDialog::SetPosition(Vector2 Pos)
{
	IWidget::SetPosition(Pos);

	Vector2 NATDocsButtonSize = Vector2(25.0f, 25.0f);

	if (HeaderLabel) HeaderLabel->SetPosition(Pos);
	if (BackgroundImage) BackgroundImage->SetPosition(Vector2(Position.x, Position.y + 20.f));

	if (AchievementsDefinitionNames)
	{
		AchievementsDefinitionNames->SetPosition(Position + Vector2(10.0f, HeaderLabel->GetSize().y + 5.0f));

		if (AchievementsDefinitionsList)
		{
			AchievementsDefinitionsList->SetPosition(Vector2(AchievementsDefinitionNames->GetPosition().x + AchievementsDefinitionNames->GetSize().x + 7.f,
															 AchievementsDefinitionNames->GetPosition().y));

			if (LanguageLabel) LanguageLabel->SetPosition(AchievementsDefinitionsList->GetPosition() + Vector2(180.f, 0.f));

			if (UnlockAchievementButton) UnlockAchievementButton->SetPosition(AchievementsDefinitionsList->GetPosition() + Vector2(260.f, -30.f));
			if (UpdatePlayerAchievementsButton) UpdatePlayerAchievementsButton->SetPosition(AchievementsDefinitionsList->GetPosition() + Vector2(350.f, -30.f));
		}
	}

	if (StatsIngestLabel) StatsIngestLabel->SetPosition(GetPosition() + GetSize() - Vector2(435.f, 35.f));
	if (StatsIngestNameField) StatsIngestNameField->SetPosition(GetPosition() + GetSize() - Vector2(355.f, 35.f));
	if (StatsIngestAmountField) StatsIngestAmountField->SetPosition(GetPosition() + GetSize() - Vector2(200.f, 35.f));
	if (StatsIngestButton) StatsIngestButton->SetPosition(GetPosition() + GetSize() - Vector2(110.f, 35.f));
	if (LockedIconStateLabel) LockedIconStateLabel->SetPosition(GetPosition() + GetSize() - Vector2(450.f, 90.f));
	if (UnlockedIconStateLabel) UnlockedIconStateLabel->SetPosition(GetPosition() + GetSize() - Vector2(250.0f, 90.f));
	if (LockedIcon) LockedIcon->SetPosition(LockedIconStateLabel->GetPosition() + Vector2(130.0f, 0.0f));
	if (UnlockedIcon) UnlockedIcon->SetPosition(UnlockedIconStateLabel->GetPosition() + Vector2(130.0f, 0.0f));
}

void FAchievementsDefinitionsDialog::SetSize(Vector2 NewSize)
{
	IWidget::SetSize(NewSize);

	if (HeaderLabel) HeaderLabel->SetSize(Vector2(NewSize.x, 25.0f));
	if (BackgroundImage) BackgroundImage->SetSize(NewSize);

	if (StatsIngestLabel) StatsIngestLabel->SetSize(Vector2(100.f, 30.f));
	if (StatsIngestNameField) StatsIngestNameField->SetSize(Vector2(150.f, 30.f));
	if (StatsIngestAmountField) StatsIngestAmountField->SetSize(Vector2(80.f, 30.f));
	if (StatsIngestButton) StatsIngestButton->SetSize(Vector2(100.f, 30.f));
}

void FAchievementsDefinitionsDialog::SetWindowSize(Vector2 WindowSize)
{
	const Vector2 DialogSize = Vector2(WindowSize.x * (1.0f - ConsoleWindowProportion.x) - 30.0f, WindowSize.y - 90.0f);
	if (BackgroundImage) BackgroundImage->SetSize(DialogSize);
	if (HeaderLabel) HeaderLabel->SetSize(Vector2(DialogSize.x, 20.0f));

	const Vector2 NamesListSize = Vector2(WindowSize.x * (1.0f - ConsoleWindowProportion.x) - 100.0f, WindowSize.y - 200.0f - DefinitionsPosition.y);
	if (AchievementsDefinitionNames) AchievementsDefinitionNames->SetSize(NamesListSize);

	const Vector2 DefinitionsListSize = Vector2(WindowSize.x * (1.0f - ConsoleWindowProportion.x) + 130.0f, WindowSize.y - 280.0f - DefinitionsPosition.y);
	if (AchievementsDefinitionsList) AchievementsDefinitionsList->SetSize(DefinitionsListSize);
}

void FAchievementsDefinitionsDialog::OnGameEvent(const FGameEvent& Event)
{
	if (Event.GetType() == EGameEventType::UserLoggedIn)
	{
		if (UnlockAchievementButton) UnlockAchievementButton->Enable();
		if (UpdatePlayerAchievementsButton) UpdatePlayerAchievementsButton->Enable();

		if (AchievementsDefinitionNames) AchievementsDefinitionNames->SetEntriesVisible(true);
		if (AchievementsDefinitionsList) AchievementsDefinitionsList->SetEntriesVisible(true);

		if (StatsIngestNameField) StatsIngestNameField->Enable();
		if (StatsIngestAmountField) StatsIngestAmountField->Enable();

		UpdateDefaultLanugage();
	}
	else if (Event.GetType() == EGameEventType::UserLoginRequiresMFA)
	{
		SetFocused(false);
	}
	else if (Event.GetType() == EGameEventType::UserLoggedOut)
	{
		FGame::Get().GetAchievements()->QueryDefinitions();
	}
	else if (Event.GetType() == EGameEventType::ShowPrevUser)
	{
		UpdateInfoList();
	}
	else if (Event.GetType() == EGameEventType::ShowNextUser)
	{
		UpdateInfoList();
	}
	else if (Event.GetType() == EGameEventType::NoUserLoggedIn)
	{
		ClearCurrentSelection();

		UpdateInfoList();

		ClearCurrentDefinition();

		SetInfoList(CurrentDefinition);

		HideAchievementDefinitions();

		if (UnlockAchievementButton) UnlockAchievementButton->Disable();
		if (UpdatePlayerAchievementsButton) UpdatePlayerAchievementsButton->Disable();
		if (StatsIngestNameField) StatsIngestNameField->Disable();
		if (StatsIngestAmountField) StatsIngestAmountField->Disable();
	}
	else if (Event.GetType() == EGameEventType::DefinitionsReceived)
	{
		OnAchievementDefinitionsReceived();
	}
	else if (Event.GetType() == EGameEventType::PlayerAchievementsReceived)
	{
		FProductUserId ProductUserId = Event.GetProductUserId();
		OnPlayerAchievementsReceived(ProductUserId);
	}
	else if (Event.GetType() == EGameEventType::AchievementsUnlocked)
	{
		FProductUserId ProductUserId = Event.GetProductUserId();
		OnPlayerAchievementsUnlocked(ProductUserId);
	}
}

void FAchievementsDefinitionsDialog::HideAchievementDefinitions()
{
	if (AchievementsDefinitionNames)
	{
		AchievementsDefinitionNames->SetEntriesVisible(false);
	}

	if (AchievementsDefinitionsList)
	{
		AchievementsDefinitionsList->SetEntriesVisible(false);
	}

	if (LanguageLabel) LanguageLabel->Hide();

	if (UnlockAchievementButton) UnlockAchievementButton->Hide();
}

void FAchievementsDefinitionsDialog::OnAchievementDefinitionsReceived()
{
	if (AchievementsDefinitionNames)
	{
		DefinitionNamesList = FGame::Get().GetAchievements()->GetDefinitionIds();

		AchievementsDefinitionNames->RefreshData(DefinitionNamesList);

		if (AchievementsDefinitionNames)
		{
			AchievementsDefinitionNames->SetEntriesVisible(true);
		}

		if (CurrentSelectionIndex >= 0 && CurrentSelectionIndex < (int)DefinitionNamesList.size())
		{
			CurrentSelection = DefinitionNamesList[CurrentSelectionIndex];

			if (FGame::Get().GetAchievements()->GetDefinitionFromIndex(CurrentSelectionIndex, CurrentDefinition))
			{
				InvalidateDefinitionIcons();
				SetInfoList(CurrentDefinition);
			}
		}
	}

	UpdateInfoList();

	// Test
	//SetTestInfo();
}

void FAchievementsDefinitionsDialog::OnPlayerAchievementsReceived(FProductUserId UserId)
{
	UpdateInfoList();

	FGame::Get().GetAchievements()->QueryDefinitions();
}

void FAchievementsDefinitionsDialog::OnPlayerAchievementsUnlocked(FProductUserId UserId)
{
	UpdateInfoList();
}

void FAchievementsDefinitionsDialog::ClearDefinitionsList()
{
	AchievementsDefinitionsList->Reset();
}

void FAchievementsDefinitionsDialog::ShowDefinitionsList()
{
	AchievementsDefinitionsList->SetEntriesVisible(true);
}

void FAchievementsDefinitionsDialog::HideDefinitionsList()
{
	AchievementsDefinitionsList->SetEntriesVisible(false);
	SetFocused(false);
}

void FAchievementsDefinitionsDialog::OnDefinitionNamesEntrySelected(size_t Index)
{
	if (Index >= DefinitionNamesList.size())
		return;

	CurrentSelection = DefinitionNamesList[Index];

	CurrentSelectionIndex = (int)Index;

	if (FGame::Get().GetAchievements()->GetDefinitionFromIndex(CurrentSelectionIndex, CurrentDefinition))
	{
		InvalidateDefinitionIcons();
		SetInfoList(CurrentDefinition);
	}
}

void FAchievementsDefinitionsDialog::ClearCurrentSelection()
{
	CurrentSelection.clear();
	CurrentSelectionIndex = -1;
}

void FAchievementsDefinitionsDialog::UpdateInfoList()
{
	if (CurrentSelectionIndex == -1)
	{
		DefinitionsList.clear();

		if (UnlockAchievementButton) UnlockAchievementButton->Disable();
	}
	else
	{
		SetInfoList(CurrentDefinition);
	}
}

void FAchievementsDefinitionsDialog::UpdateIcons()
{
	//Check if icon data became available
	if (CurrentDefinition && !CurrentDefinition->AchievementId.empty())
	{
		LockedIconStateLabel->Show();
		UnlockedIconStateLabel->Show();

		UpdateIcon(LockedIcon, true, LockedIconStateLabel);
		UpdateIcon(UnlockedIcon, false, UnlockedIconStateLabel);
	}
	else
	{
		LockedIconStateLabel->Hide();
		UnlockedIconStateLabel->Hide();
	}
}

void FAchievementsDefinitionsDialog::UpdateIcon(std::shared_ptr<FSpriteWidget>& IconWidget, const bool bLocked, std::shared_ptr<FTextLabelWidget> LabelWidget)
{
	bool bIconTextureBroken = IconWidget && !IconWidget->IsTextureValid();
	bool bIconDataLoaded = true;
	if (!IconWidget && !bIconTextureBroken)
	{
		std::vector<char> IconData;
		if (FGame::Get().GetAchievements()->GetIconData(CurrentDefinition->AchievementId, IconData, bLocked))
		{
			const std::wstring& IconURL = (bLocked) ? CurrentDefinition->LockedIconURL : CurrentDefinition->UnlockedIconURL;

			IconWidget = std::make_shared<FSpriteWidget>(
				LabelWidget->GetPosition() + Vector2(130.0f, 0.0f),
				LabelWidget->GetSize(),
				LabelWidget->GetLayer(),
				IconURL,
				IconData
				);
			IconWidget->Create();

			if (IconWidget->IsTextureValid())
			{
				AddWidget(IconWidget);
			}
			else
			{
				bIconTextureBroken = true;
				IconWidget->Hide();
			}
		}
		else
		{
			bIconDataLoaded = false;
		}
	}

	std::wstring IconString = (bLocked) ? std::wstring(L"Locked icon: ") : std::wstring(L"Unlocked icon: ");

	if (IconWidget && !bIconTextureBroken)
	{
		LabelWidget->SetText(IconString);
	}
	else
	{
		if (!bIconDataLoaded)
		{
			LabelWidget->SetText(IconString + L" Downloading");
		}
		else
		{
			LabelWidget->SetText(IconString + L" Invalid format");
		}
	}
}

void FAchievementsDefinitionsDialog::UnlockSelectedAchievement()
{
	if (!CurrentSelection.empty())
	{
		std::vector<std::wstring> AchievementIds;
		AchievementIds.emplace_back(CurrentSelection);

		FGame::Get().GetAchievements()->UnlockAchievements(AchievementIds);
	}
}

void FAchievementsDefinitionsDialog::StatsIngest()
{
	if (StatsIngestNameField && StatsIngestAmountField)
	{
		std::wstring StatsIngestName = StatsIngestNameField->GetText();
		if (!StatsIngestName.empty() && StatsIngestName != StatsIngestNameField->GetInitialText())
		{
			std::wstring StatsIngestAmountStr = StatsIngestAmountField->GetText();
			if (!StatsIngestAmountStr.empty() && StatsIngestAmountStr != StatsIngestAmountField->GetInitialText())
			{
				int StatsIngestAmount = std::stoi(StatsIngestAmountStr);

				FGame::Get().GetAchievements()->IngestStat(StatsIngestName, StatsIngestAmount);
			}
		}
	}
}

void FAchievementsDefinitionsDialog::UpdateStatsIngestButtonState()
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player == nullptr)
	{
		if (StatsIngestButton)
		{
			StatsIngestButton->Disable();
		}
		return;
	}

	UpdateStatsIngestButtonCounter++;
	if (UpdateStatsIngestButtonCounter < 30)
	{
		return;
	}

	if (StatsIngestButton)
	{
		StatsIngestButton->Disable();
		if (StatsIngestNameField && StatsIngestAmountField)
		{
			std::wstring StatsIngestName = StatsIngestNameField->GetText();
			if (!StatsIngestName.empty() && StatsIngestName != StatsIngestNameField->GetInitialText())
			{
				std::wstring StatsIngestAmountStr = StatsIngestAmountField->GetText();
				if (!StatsIngestAmountStr.empty() && StatsIngestAmountStr != StatsIngestAmountField->GetInitialText())
				{
					StatsIngestButton->Enable();
				}
			}
		}
	}

	UpdateStatsIngestButtonCounter = 0;
}

void FAchievementsDefinitionsDialog::SetTestInfo()
{
	CurrentDefinition->AchievementId = L"Achievement 2";
	CurrentDefinition->bIsHidden = true;
	CurrentDefinition->UnlockedDisplayName = L"Test Achievement";
	CurrentDefinition->UnlockedDescription = L"Test Achievement";
	CurrentDefinition->LockedDisplayName = L"Locked";
	CurrentDefinition->LockedDescription = L"Locked";
	CurrentDefinition->LockedIconURL = L"LockedIconURL";
	CurrentDefinition->UnlockedIconURL = L"UnlockedURL";
	CurrentDefinition->FlavorText = L"Flavor description";
	FStatInfo StatInfo;
	StatInfo.Name = L"stat_1";
	StatInfo.ThresholdValue = 5;
	CurrentDefinition->StatInfo.clear();
	CurrentDefinition->StatInfo.emplace_back(StatInfo);

	DefinitionNamesList.clear();
	for (int i = 0; i < 10; ++i)
	{
		wchar_t Name[16] = {};
		wsprintf(Name, L"Achievement %d", i + 1);
		std::wstring NameStr = std::wstring(Name);

		DefinitionNamesList.emplace_back(NameStr);
	}

	if (AchievementsDefinitionNames)
	{
		AchievementsDefinitionNames->RefreshData(DefinitionNamesList);
	}

	SetInfoList(CurrentDefinition);

	if (UnlockAchievementButton) UnlockAchievementButton->Enable();
}

void FAchievementsDefinitionsDialog::ClearCurrentDefinition()
{
	CurrentDefinition->AchievementId = L"";
	CurrentDefinition->bIsHidden = false;
	CurrentDefinition->UnlockedDisplayName = L"";
	CurrentDefinition->UnlockedDescription = L"";
	CurrentDefinition->LockedDisplayName = L"";
	CurrentDefinition->LockedDescription = L"";
	CurrentDefinition->LockedIconURL = L"";
	CurrentDefinition->UnlockedIconURL = L"";
	CurrentDefinition->FlavorText = L"";
	CurrentDefinition->StatInfo.clear();
}

void FAchievementsDefinitionsDialog::InvalidateDefinitionIcons()
{
	if (LockedIcon)
	{
		RemoveWidgets<FSpriteWidget>({ LockedIcon });
		LockedIcon.reset();
	}

	if (UnlockedIcon)
	{
		RemoveWidgets<FSpriteWidget>({ UnlockedIcon });
		UnlockedIcon.reset();
	}
}

void FAchievementsDefinitionsDialog::UpdateDefaultLanugage()
{
	FGame::Get().GetAchievements()->SetDefaultLanguage();
	std::wstring DefaultLanguage = FGame::Get().GetAchievements()->GetDefaultLanguage();

	SetInfoList(CurrentDefinition);
}

void FAchievementsDefinitionsDialog::SetInfoList(std::shared_ptr<FAchievementsDefinitionData> Def)
{
	DefinitionsList.clear();

	if (!Def->AchievementId.empty())
	{
		std::wstring AchievementId = L"ID: " + Def->AchievementId;
		DefinitionsList.emplace_back(AchievementId);

		std::wstring DisplayName = L"Unlocked Display Name: " + Def->UnlockedDisplayName;
		DefinitionsList.emplace_back(DisplayName);

		std::wstring Description = L"Unlocked Description: " + Def->UnlockedDescription;
		DefinitionsList.emplace_back(Description);

		std::wstring LockedDisplayName = L"Locked Display Name: " + Def->LockedDisplayName;
		DefinitionsList.emplace_back(LockedDisplayName);

		std::wstring LockedDescription = L"Locked Description: " + Def->LockedDescription;
		DefinitionsList.emplace_back(LockedDescription);

		std::wstring FlavorText = L"Flavor Text: " + Def->FlavorText;
		DefinitionsList.emplace_back(FlavorText);

		std::wstring IsHidden;
		if (Def->bIsHidden == EOS_TRUE)
		{
			IsHidden = L"Hidden: true";
		}
		else
		{
			IsHidden = L"Hidden: false";
		}
		DefinitionsList.emplace_back(IsHidden);

		wchar_t StatInfoStr[256] = L"Stat Thresholds: ";
		for (const FStatInfo& StatInfo : Def->StatInfo)
		{
			swprintf(StatInfoStr, 256, L"%ls '%ls': %d", StatInfoStr, StatInfo.Name.c_str(), StatInfo.ThresholdValue);
		}
		DefinitionsList.emplace_back(StatInfoStr);

		std::shared_ptr<FPlayerAchievementData> PlayerAchievement;
		if (FGame::Get().GetAchievements()->GetPlayerAchievementFromId(Def->AchievementId, PlayerAchievement))
		{
			if (!PlayerAchievement->Stats.empty())
			{
				wchar_t StatProgressStr[256] = L"Stat Progress: ";
				for (auto Iter : PlayerAchievement->Stats)
				{
					swprintf(StatProgressStr, 256, L"%ls '%ls': %d/%d", StatProgressStr, Iter.first.c_str(), Iter.second.CurValue, Iter.second.ThresholdValue);
				}
				DefinitionsList.emplace_back(StatProgressStr);
			}

			const bool bUnlocked = PlayerAchievement->UnlockTime != EOS_ACHIEVEMENTS_ACHIEVEMENT_UNLOCKTIME_UNDEFINED;
			wchar_t ProgressStr[32] = {};
			swprintf(ProgressStr, 32, L"%f", (!bUnlocked) ? PlayerAchievement->Progress : 1.0f);
			std::wstring Progress = L"Total Progress: " + std::wstring(ProgressStr);
			DefinitionsList.emplace_back(Progress);
		}
	}
	else
	{
		if (!Def->LockedDisplayName.empty())
		{
			std::wstring LockedDisplayName = L"Locked Display Name: " + Def->LockedDisplayName;
			DefinitionsList.emplace_back(LockedDisplayName);
		}
	}

	if (AchievementsDefinitionsList)
	{
		AchievementsDefinitionsList->RefreshData(DefinitionsList);

		if (AchievementsDefinitionsList)
		{
			AchievementsDefinitionsList->SetEntriesVisible(true);
		}
	}

	if (LanguageLabel)
	{
		LanguageLabel->SetText(L"Language: " + FGame::Get().GetAchievements()->GetDefaultLanguage());
		LanguageLabel->Show();
	}

	if (FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser()) == nullptr)
	{
		if (UnlockAchievementButton) UnlockAchievementButton->Disable();
	}
	else
	{
		if (UnlockAchievementButton) UnlockAchievementButton->Enable();
	}
}
