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
#include "Leaderboard.h"
#include "TextEditor.h"
#include "LeaderboardDialog.h"
#include "TableView.h"

const Vector2 StatsPosition = Vector2(10.0f, 120.0f);
const float ButtonsHeight = 30.0f;
const float HeaderLabelHeight = 20.0f;
const wchar_t* DefaultStatsLabelValue = L"NO LEADERBOARD SELECTED";

const bool bIsTestDataEnabled = false;
const int TestNumFriends = 100;
const int TestNumUsers = 1000;

static void RebuildTableEntriesWithRecordData(std::vector<FTableViewWidget::TableRowDataType>& TableData, FTableViewWidget::TableRowDataType& Labels)
{
	TableData.clear();
	Labels.Values.clear();

	Labels.Values.push_back(L"Rank");
	Labels.Values.push_back(L"Name");
	Labels.Values.push_back(L"Score");

	std::vector<std::shared_ptr<FLeaderboardsRecordData>>& Records = FGame::Get().GetLeaderboard()->GetCachedRecords();
	TableData.reserve(Records.size());

	for (const std::shared_ptr<FLeaderboardsRecordData>& NextRecord : Records)
	{
		if (NextRecord)
		{
			FTableViewWidget::TableRowDataType Row;
			wchar_t Buffer[64];
			wsprintf(Buffer, L"%d", NextRecord->Rank);
			Row.Values.emplace_back(Buffer);
			Row.Values.emplace_back(NextRecord->DisplayName);
			wsprintf(Buffer, L"%d", NextRecord->Score);
			Row.Values.emplace_back(Buffer);

			TableData.push_back(Row);
		}
	}
}

static void RebuildTableEntriesWithFriendsData(std::wstring LeaderboardId, std::vector<FTableViewWidget::TableRowDataType>& TableData, FTableViewWidget::TableRowDataType& Labels)
{
	TableData.clear();
	Labels.Values.clear();

	Labels.Values.push_back(L"Rank");
	Labels.Values.push_back(L"Name");
	Labels.Values.push_back(L"Score");

	const std::vector<std::shared_ptr<FLeaderboardsUserScoreData>>* FriendsScores;
	if (FGame::Get().GetLeaderboard()->GetCachedUserScoresFromLeaderboardId(LeaderboardId, &FriendsScores))
	{
		TableData.reserve(FriendsScores->size());

		size_t Index = 1;
		for (const std::shared_ptr<FLeaderboardsUserScoreData>& NextRecord : *FriendsScores)
		{
			if (NextRecord)
			{
				FTableViewWidget::TableRowDataType Row;

				wchar_t Buffer[64];
				wsprintf(Buffer, L"%d", Index);
				Row.Values.emplace_back(Buffer);
				Row.Values.emplace_back(FGame::Get().GetFriends()->GetFriendName(NextRecord->UserId));

				wsprintf(Buffer, L"%d", NextRecord->Score);
				Row.Values.emplace_back(Buffer);

				TableData.push_back(Row);

				Index++;
			}
		}
	}
}

FLeaderboardDialog::FLeaderboardDialog(
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
		std::wstring(L"Leaderboard Data: "),
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

	DefinitionsList = std::make_shared<FDefinitionsList>(
		DialogPos,
		DialogSize - Vector2(0.0f, ButtonsHeight),
		DialogLayer - 1,
		30.0f, //entry height
		20.0f, //label height
		10.0f, //scroller width
		L"", //background
		L"",
		DialogNormalFont,
		DialogNormalFont,
		DialogSmallFont,
		DialogTinyFont);

	StatsTableLabel = std::make_shared<FTextLabelWidget>(
		StatsPosition,
		Vector2(500.0f, 20.0f),
		Layer - 1,
		std::wstring(DefaultStatsLabelValue),
		L"Assets/wide_label.dds",
		FColor(1.f, 1.f, 1.f, 1.f),
		FColor(1.f, 1.f, 1.f, 1.f),
		EAlignmentType::Left
		);
	StatsTableLabel->SetFont(DialogNormalFont);
	StatsTableLabel->SetBorderColor(Color::UIBorderGrey);

	const Vector2 FirstButtonPos = StatsPosition + Vector2(5.0f, HeaderLabelHeight);
	UpdateSelectedButton = std::make_shared<FButtonWidget>(
		FirstButtonPos,
		Vector2(DialogSize.x, 30.0f),
		DialogLayer - 1,
		L"SHOW GLOBAL",
		assets::LargeButtonAssets,
		DialogSmallFont,
		Color::White,
		Color::White
		);
	UpdateSelectedButton->SetOnPressedCallback([this]()
	{
		this->QueryGlobalRankings();
	});
	UpdateSelectedButton->SetBackgroundColors(assets::DefaultButtonColors);

	UpdateFriendRanksButton = std::make_shared<FButtonWidget>(
		FirstButtonPos + Vector2(UpdateSelectedButton->GetSize().x + 10.0f, 0.0f),
		Vector2(DialogSize.x, 30.0f),
		DialogLayer - 1,
		L"SHOW FRIENDS",
		assets::LargeButtonAssets,
		DialogSmallFont,
		Color::White,
		Color::White
		);
	UpdateFriendRanksButton->SetOnPressedCallback([this]()
	{
		this->QueryFriends();
	});
	UpdateFriendRanksButton->SetBackgroundColors(assets::DefaultButtonColors);

	RefreshButton = std::make_shared<FButtonWidget>(
		UpdateFriendRanksButton->GetPosition() + Vector2(UpdateFriendRanksButton->GetSize().x + 10.0f, 0.0f),
		Vector2(ButtonsHeight, ButtonsHeight),
		DialogLayer - 1,
		L"",
		std::vector<std::wstring>({ L"Assets/refresh.dds" }),
		DialogSmallFont,
		Color::White,
		Color::White
		);
	RefreshButton->SetOnPressedCallback([this]()
	{
		this->RepeatLastQuery();
	});
	RefreshButton->SetBackgroundColors(assets::DefaultButtonColors);

	StatsTable = std::make_shared<FTableViewWidget>(
		StatsPosition + Vector2(0.0f, HeaderLabelHeight + ButtonsHeight),
		Vector2(500.0f, 500.0f),
		DialogLayer - 1,
		L"",
		10.0f, //scroller width
		std::vector<FTableRowData>(),
		FTableRowData());
	StatsTable->SetFonts(DialogNormalFont, DialogSmallFont);

	StatsBackground = std::make_shared<FSpriteWidget>(
		StatsPosition + Vector2(0.0f, HeaderLabelHeight),
		Vector2(500.0f, 500.0f - HeaderLabelHeight),
		DialogLayer,
		L"Assets/texteditor.dds");
	StatsBackground->SetBorderColor(Color::UIBorderGrey);
}

void FLeaderboardDialog::Update()
{
	Definitions = FGame::Get().GetLeaderboard()->GetDefinitionIds();

	if (DefinitionsList)
	{
		std::vector<std::wstring> DefinitionIds = FGame::Get().GetLeaderboard()->GetDefinitionIds();
		DefinitionsList->RefreshData(DefinitionIds);
	}

	FDialog::Update();

	if (DefinitionsList)
	{
		DefinitionsList->Update();
	}

	FDialog::Update();
}

void FLeaderboardDialog::Create()
{
	if (HeaderLabel) HeaderLabel->Create();
	if (BackgroundImage) BackgroundImage->Create();
	if (DefinitionsList) DefinitionsList->Create();
	if (StatsTableLabel) StatsTableLabel->Create();
	if (StatsTable) StatsTable->Create();
	if (StatsBackground) StatsBackground->Create();
	if (UpdateSelectedButton) UpdateSelectedButton->Create();
	if (UpdateFriendRanksButton) UpdateFriendRanksButton->Create();
	if (RefreshButton) RefreshButton->Create();

	//make entries invisible until player logs in
	if (DefinitionsList)
	{
		DefinitionsList->SetEntriesVisible(false);
		DefinitionsList->SetOnEntrySelectedCallback([this](size_t Index) { this->OnDefinitionEntrySelected(Index); });
	}

	AddWidget(HeaderLabel);
	AddWidget(BackgroundImage);
	AddWidget(DefinitionsList);
	AddWidget(StatsBackground);
	AddWidget(StatsTableLabel);
	AddWidget(StatsTable);
	AddWidget(UpdateSelectedButton);
	AddWidget(UpdateFriendRanksButton);
	AddWidget(RefreshButton);

	if (UpdateSelectedButton) UpdateSelectedButton->Disable();
	if (UpdateFriendRanksButton) UpdateFriendRanksButton->Disable();
	if (RefreshButton) RefreshButton->Disable();
}

void FLeaderboardDialog::SetPosition(Vector2 Pos)
{
	IWidget::SetPosition(Pos);

	if (HeaderLabel) HeaderLabel->SetPosition(Pos);
	if (BackgroundImage) BackgroundImage->SetPosition(Vector2(Position.x, Position.y + 20.f));
	if (DefinitionsList) DefinitionsList->SetPosition(Position);
	if (StatsTableLabel) StatsTableLabel->SetPosition(StatsPosition);
	if (StatsBackground) StatsBackground->SetPosition(StatsPosition + Vector2(0.0f, HeaderLabelHeight));

	Vector2 FirstButtonPos = StatsPosition + Vector2(5.0f, HeaderLabelHeight);
	if (UpdateSelectedButton) UpdateSelectedButton->SetPosition(FirstButtonPos);
	if (UpdateFriendRanksButton) UpdateFriendRanksButton->SetPosition(FirstButtonPos + Vector2(UpdateSelectedButton->GetSize().x + 10.0f, 0.0f));
	if (RefreshButton) RefreshButton->SetPosition(UpdateFriendRanksButton->GetPosition() + Vector2(UpdateFriendRanksButton->GetSize().x + 10.0f, 0.0f));

	if (StatsTable) StatsTable->SetPosition(StatsPosition + Vector2(0.0f, ButtonsHeight));
}

void FLeaderboardDialog::SetSize(Vector2 NewSize)
{
	IWidget::SetSize(NewSize);
}

void FLeaderboardDialog::SetWindowSize(Vector2 WindowSize)
{
	const Vector2 DefListSize = Vector2(WindowSize.x * (1.0f - ConsoleWindowProportion.x) - 40.0f, WindowSize.y - 200.0f - StatsPosition.y);
	const Vector2 DialogSize = Vector2(WindowSize.x * (1.0f - ConsoleWindowProportion.x) - 30.0f, WindowSize.y - 90.0f);

	if (BackgroundImage) BackgroundImage->SetSize(DialogSize);
	if (HeaderLabel) HeaderLabel->SetSize(Vector2(DialogSize.x, HeaderLabelHeight));
	if (DefinitionsList) DefinitionsList->SetSize(DefListSize);
	if (StatsTable) StatsTable->SetSize(Vector2(ConsoleWindowProportion.x * WindowSize.x, (1.0f - ConsoleWindowProportion.y) * WindowSize.y - StatsPosition.y - 80.0f));
	if (StatsTableLabel) StatsTableLabel->SetSize(Vector2(StatsTable->GetSize().x, HeaderLabelHeight));
	if (StatsBackground) StatsBackground->SetSize(StatsTable->GetSize() + Vector2(0.0f, ButtonsHeight));
	if (UpdateSelectedButton) UpdateSelectedButton->SetSize(Vector2(120.0f, ButtonsHeight));
	if (UpdateFriendRanksButton) UpdateFriendRanksButton->SetSize(Vector2(120.0f, ButtonsHeight));
	if (RefreshButton) RefreshButton->SetSize(Vector2(ButtonsHeight, ButtonsHeight));
}


void FLeaderboardDialog::ClearDataList()
{
	if (DefinitionsList)
	{
		DefinitionsList->Reset();
	}
	
	if (StatsTable)
	{
		StatsTable->Clear();
	}

	if (StatsTableLabel)
	{
		StatsTableLabel->SetText(DefaultStatsLabelValue);
	}
}

void FLeaderboardDialog::ShowUI()
{
	if (DefinitionsList)
	{
		DefinitionsList->SetEntriesVisible(true);
	}

	if (UpdateSelectedButton)
	{
		UpdateSelectedButton->Enable();
		UpdateSelectedButton->Show();
	}

	if (UpdateFriendRanksButton)
	{
		UpdateFriendRanksButton->Enable();
		UpdateFriendRanksButton->Show();
	}

	if (RefreshButton)
	{
		RefreshButton->Enable();
		RefreshButton->Show();
	}
}

void FLeaderboardDialog::HideUI()
{
	if (DefinitionsList)
	{
		DefinitionsList->SetEntriesVisible(false);
	}

	if (UpdateSelectedButton)
	{
		UpdateSelectedButton->Disable();
		UpdateSelectedButton->Hide();
	}

	if (UpdateFriendRanksButton)
	{
		UpdateFriendRanksButton->Disable();
		UpdateFriendRanksButton->Hide();
	}

	if (RefreshButton)
	{
		RefreshButton->Disable();
		RefreshButton->Hide();
	}

	SetFocused(false);
}

void FLeaderboardDialog::ClearCurrentSelection()
{
	CurrentSelection.clear();
	if (DefinitionsList)
	{
		DefinitionsList->SetFocused(false);
	}
	if (StatsTable)
	{
		StatsTable->Clear();
	}
}


void FLeaderboardDialog::UpdateUserInfo()
{
	if (HeaderLabel)
	{
		HeaderLabel->SetText(L"Leaderboard: log in to proceed.");
		if (FPlayerManager::Get().GetCurrentUser().IsValid())
		{
			std::wstring DisplayName = FPlayerManager::Get().GetDisplayName(FPlayerManager::Get().GetCurrentUser());
			HeaderLabel->SetText(std::wstring(L"Leaderboard: ") + DisplayName);
		}
	}
}

void FLeaderboardDialog::OnQueryInitiated()
{
	if (StatsTableLabel)
	{
		StatsTableLabel->SetText(FStringUtils::ToUpper(CurrentSelection));
	}

	if (StatsTable)
	{
		StatsTable->Clear();
	}

	LastSelection = CurrentSelection;
	bLastQueryWasFriends = bWaitingForFriendsUpdate;
}

void FLeaderboardDialog::QueryFriends()
{
	if (bIsTestDataEnabled)
	{
		ShowTestFriendsData();
		return;
	}

	if (!CurrentSelection.empty())
	{
		std::vector<EOS_ProductUserId> Friends;
		const std::vector<FFriendData>& FriendData = FGame::Get().GetFriends()->GetFriends();
		for (const FFriendData& NextFriend : FriendData)
		{
			Friends.push_back(NextFriend.UserProductUserId);
		}

		if (!Friends.empty())
		{
			bWaitingForFriendsUpdate = true;
			std::vector<std::wstring> LeaderboardIds;
			LeaderboardIds.emplace_back(CurrentSelection);
			FGame::Get().GetLeaderboard()->QueryUserScores(LeaderboardIds, Friends);
		}

		OnQueryInitiated();
	}
}

void FLeaderboardDialog::QueryGlobalRankings()
{
	if (bIsTestDataEnabled)
	{
		ShowTestGlobalData();
		return;
	}

	if (!CurrentSelection.empty())
	{
		bWaitingForFriendsUpdate = false;
		FGame::Get().GetLeaderboard()->QueryRanks(CurrentSelection);

		OnQueryInitiated();
	}
}

void FLeaderboardDialog::RepeatLastQuery()
{
	if (!LastSelection.empty())
	{
		//do the same query
		CurrentSelection = LastSelection;
		if (bLastQueryWasFriends)
		{
			QueryFriends();
		}
		else
		{
			QueryGlobalRankings();
		}
	}
}

void FLeaderboardDialog::OnGameEvent(const FGameEvent& Event)
{
	if (Event.GetType() == EGameEventType::UserLoggedIn)
	{
		LastSelection.clear();
		UpdateUserInfo();
		ShowUI();
	}
	else if (Event.GetType() == EGameEventType::UserLoginRequiresMFA)
	{
		UpdateUserInfo();
		DefinitionsList->SetEntriesVisible(false);
		SetFocused(false);
	}
	else if (Event.GetType() == EGameEventType::UserLoginEnteredMFA)
	{
		UpdateUserInfo();
		DefinitionsList->SetEntriesVisible(false);
	}
	else if (Event.GetType() == EGameEventType::UserLoggedOut)
	{
		UpdateUserInfo();
		if (FPlayerManager::Get().GetNumPlayers() == 0)
		{
			ClearDataList();
			HideUI();
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
		HideUI();
	}
	else if (Event.GetType() == EGameEventType::CancelLogin)
	{
		ShowUI();
		UpdateUserInfo();
	}
	else if (Event.GetType() == EGameEventType::LeaderboardRecordsReceived)
	{
		//Retrieve data and populate stats table
		if (bWaitingForFriendsUpdate)
		{
			//Friends
			std::vector<FTableViewWidget::TableRowDataType> TableRows;
			FTableViewWidget::TableRowDataType Labels;
			RebuildTableEntriesWithFriendsData(CurrentSelection, TableRows, Labels);
			StatsTable->RefreshData(std::move(TableRows));
			StatsTable->RefreshLabels(std::move(Labels));
		}
		else
		{
			//Global rankings
			std::vector<FTableViewWidget::TableRowDataType> TableRows;
			FTableViewWidget::TableRowDataType Labels;
			RebuildTableEntriesWithRecordData(TableRows, Labels);
			StatsTable->RefreshData(std::move(TableRows));
			StatsTable->RefreshLabels(std::move(Labels));
		}
	}
}

void FLeaderboardDialog::OnDefinitionEntrySelected(size_t Index)
{
	if (Index < Definitions.size())
	{
		std::wstring Name = Definitions[Index];

		CurrentSelection = Name;
	}
}

void FLeaderboardDialog::ShowTestFriendsData()
{
	std::vector<FTableViewWidget::TableRowDataType> TableRows;
	FTableViewWidget::TableRowDataType Labels;

	Labels.Values.push_back(L"Rank");
	Labels.Values.push_back(L"Name");
	Labels.Values.push_back(L"Score");

	TableRows.reserve(TestNumFriends);

	for (int UserIndex = 1; UserIndex <= TestNumFriends; ++UserIndex)
	{
		std::wstring UserIndexStr = std::to_wstring(UserIndex);

		FTableViewWidget::TableRowDataType Row;

		Row.Values.emplace_back(UserIndexStr);
		Row.Values.emplace_back(L"Friend " + UserIndexStr);
		Row.Values.emplace_back(L"" + std::to_wstring((TestNumFriends - UserIndex + 1)));
		
		TableRows.push_back(Row);
	}

	StatsTable->RefreshData(std::move(TableRows));
	StatsTable->RefreshLabels(std::move(Labels));

	StatsTableLabel->SetText(L"First");
}

void FLeaderboardDialog::ShowTestGlobalData()
{
	std::vector<FTableViewWidget::TableRowDataType> TableRows;
	FTableViewWidget::TableRowDataType Labels;

	Labels.Values.push_back(L"Rank");
	Labels.Values.push_back(L"Name");
	Labels.Values.push_back(L"Score");

	TableRows.reserve(TestNumUsers);

	for (int UserIndex = 1; UserIndex <= TestNumUsers; ++UserIndex)
	{
		std::wstring UserIndexStr = std::to_wstring(UserIndex);

		FTableViewWidget::TableRowDataType Row;

		Row.Values.emplace_back(UserIndexStr);
		Row.Values.emplace_back(L"User " + UserIndexStr);
		Row.Values.emplace_back(L"" + std::to_wstring((TestNumUsers - UserIndex + 1)));

		TableRows.push_back(Row);
	}

	StatsTable->RefreshData(std::move(TableRows));
	StatsTable->RefreshLabels(std::move(Labels));

	StatsTableLabel->SetText(L"Second");
}
