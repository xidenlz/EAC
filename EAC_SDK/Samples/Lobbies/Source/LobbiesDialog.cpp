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
#include "LobbiesDialog.h"
#include "DebugLog.h"
#include "Checkbox.h"
#include "Users.h"
#include "LobbyMemberTableRowView.h"
#include "DropDownList.h"

#include <algorithm>

#define LOBBY_DIALOG_SEARCH_TYPE_LEVEL L"By Level"
#define LOBBY_DIALOG_SEARCH_TYPE_BUCKET L"By Bucket Id"
#define LOBBY_DIALOG_UPPER_BUTTON_WIDTH 150.0f

static FLobbySearchResultTableRowData BuildTableRowDataFromLobby(const FLobby& Lobby, uint32_t SearchIndex)
{
	FLobbySearchResultTableRowData Result;

	Result.SearchResultIndex = SearchIndex;

	std::wstring OwnerName = L"?";
	if (!Lobby.LobbyOwnerDisplayName.empty())
	{
		OwnerName = Lobby.LobbyOwnerDisplayName;
	}
	Result.Values[FLobbySearchResultTableRowData::EValue::OwnerDisplayName] = OwnerName;

	wchar_t Buffer[32];
	wsprintf(Buffer, L"%d/%d", Lobby.MaxNumLobbyMembers - Lobby.AvailableSlots, Lobby.MaxNumLobbyMembers);

	Result.Values[FLobbySearchResultTableRowData::EValue::NumMembers] = std::wstring(Buffer);

	std::wstring LevelName = L"?";
	if (const FLobbyAttribute* LevelAttr = Lobby.GetAttribute("LEVEL"))
	{
		LevelName = FStringUtils::Widen(LevelAttr->AsString);
	}
	Result.Values[FLobbySearchResultTableRowData::EValue::LevelName] = LevelName;

	Result.bActionsAvailable[FLobbySearchResultTableRowData::EAction::Join] = true;

	return Result;
}

static FLobbyMemberTableRowData BuildMemberRow(const FLobbyMember& Member, const FLobby& CurrentLobby, bool bOwnerMode, bool bIsSelf)
{
	FLobbyMemberTableRowData Result;
	Result.ValueColors.fill(Color::White);

	std::wstring MemberName = Member.DisplayName.empty() ? L"?" : Member.DisplayName;
	Result.DisplayName = MemberName;

	Result.UserId = Member.ProductId;
	Result.Values[FLobbyMemberTableRowData::EValue::DisplayName] = MemberName;
	Result.ValueColors[FLobbyMemberTableRowData::EValue::DisplayName] = bIsSelf ? Color::Cyan : Color::White;
	Result.Values[FLobbyMemberTableRowData::EValue::IsOwner] = CurrentLobby.IsOwner(Result.UserId) ? L"Owner" : L"Member";
	Result.Values[FLobbyMemberTableRowData::EValue::Skin] = FStringUtils::Widen(FLobbyMember::GetSkinString(Member.CurrentSkin));
	Result.ValueColors[FLobbyMemberTableRowData::EValue::Skin] = FLobbyMember::GetSkinColor(Member.CurrentColor);

	// Count how many members we have currently connected to the RTC room
	size_t ConnectedMembers = 0;
	std::for_each(CurrentLobby.Members.begin(), CurrentLobby.Members.end(),
		[&](const FLobbyMember& LobbyMember)
		{
			if (LobbyMember.RTCState.bIsInRTCRoom)
			{
				++ConnectedMembers;
			}
		}
	);

	std::wstring TalkingStatus;
	FColor TextColor;
	if (!CurrentLobby.bRTCRoomConnected)
	{
		TalkingStatus = L"Unknown";
		TextColor = Color::DarkGray;
	}
	else if (!Member.RTCState.bIsInRTCRoom)
	{
		TalkingStatus = L"Disconnected";
		TextColor = Color::DarkGray;
	}
	else if (Member.RTCState.bIsHardMuted)
	{
		TalkingStatus = L"Hard-Muted";
		TextColor = Color::Red;
	}
	else if (Member.RTCState.bIsLocallyMuted)
	{
		TalkingStatus = L"Muted";
		TextColor = Color::Red;
	}
	else if (Member.RTCState.bIsAudioOutputDisabled)
	{
		TalkingStatus = L"Self-Muted";
		TextColor = Color::DarkRed;
	}
	else if (Member.RTCState.bIsTalking)
	{
		TalkingStatus = L"Talking";
		TextColor = Color::Yellow;
	}
	// if we're the only one in the lobby, we don't currently get talking notifications, so we show "Connected" instead of "Not Talking"
	// as to not mislead people into thinking their microphones aren't working.
	else if (ConnectedMembers < 2)
	{
		TalkingStatus = L"Connected";
		TextColor = Color::White;
	}
	else
	{
		TalkingStatus = L"Not Talking";
		TextColor = Color::White;
	}

	Result.Values[FLobbyMemberTableRowData::EValue::TalkingStatus] = std::move(TalkingStatus);
	Result.ValueColors[FLobbyMemberTableRowData::EValue::TalkingStatus] = TextColor;

	Result.bActionsAvailable[FLobbyMemberTableRowData::EAction::Kick] = std::make_pair(bOwnerMode && !bIsSelf, Color::DarkRed);
	Result.bActionsAvailable[FLobbyMemberTableRowData::EAction::Promote] =  std::make_pair(bOwnerMode && !bIsSelf, Color::DarkGreen);
	Result.bActionsAvailable[FLobbyMemberTableRowData::EAction::ShuffleSkin] =  std::make_pair(bIsSelf, Color::DarkBlue);
	Result.bActionsAvailable[FLobbyMemberTableRowData::EAction::ChangeColor] = std::make_pair(bIsSelf && CurrentLobby.bRTCRoomConnected, Color::ForestGreen);

	FColor MuteButtonColor;
	if (bIsSelf)
	{
		MuteButtonColor = Member.RTCState.bIsAudioOutputDisabled ? Color::Red : Color::ForestGreen;
	}
	else
	{
		MuteButtonColor = Member.RTCState.bIsLocallyMuted ? Color::Red : Color::ForestGreen;
	}

	FColor HardMuteButtonColor;
	HardMuteButtonColor = Member.RTCState.bIsHardMuted ? Color::Red : Color::ForestGreen;

	Result.bActionsAvailable[FLobbyMemberTableRowData::EAction::MuteAudio] = std::make_pair(CurrentLobby.bRTCRoomConnected && Member.RTCState.bIsInRTCRoom && !Member.RTCState.bMuteActionInProgress, MuteButtonColor);
	Result.bActionsAvailable[FLobbyMemberTableRowData::EAction::HardMuteAudio] = std::make_pair(bOwnerMode && !bIsSelf && CurrentLobby.bRTCRoomConnected && Member.RTCState.bIsInRTCRoom, HardMuteButtonColor); // only lobby owner can hard-mute someone

	return Result;
}

FLobbiesDialog::FLobbiesDialog(
	Vector2 DialogPos,
	Vector2 DialogSize,
	UILayer DialogLayer,
	FontPtr DialogNormalFont,
	FontPtr DialogSmallFont,
	FontPtr DialogTinyFont) :
	FDialog(DialogPos, DialogSize, DialogLayer)
{
	BackgroundImage = std::make_shared<FSpriteWidget>(
		DialogPos,
		DialogSize,
		DialogLayer,
		L"Assets/friends.dds");

	TitleLabel = std::make_shared<FTextLabelWidget>(
		DialogPos,
		Vector2(DialogSize.x, 30.f),
		DialogLayer - 1,
		L"LOBBIES",
		L"Assets/dialog_title.dds",
		FColor(1.f, 1.f, 1.f, 1.f),
		FColor(1.f, 1.f, 1.f, 1.f),
		EAlignmentType::Left);
	TitleLabel->SetBorderColor(Color::UIBorderGrey);
	TitleLabel->SetFont(DialogNormalFont);

	CreateLobbyButton = std::make_shared<FButtonWidget>(
		Vector2(DialogPos.x + 5.0f, DialogPos.y + TitleLabel->GetSize().y + 5.0f),
		Vector2(LOBBY_DIALOG_UPPER_BUTTON_WIDTH, 25.f),
		DialogLayer - 1,
		L"CREATE NEW LOBBY",
		assets::DefaultButtonAssets,
		DialogNormalFont,
		Color::UIButtonBlue
	);
	//CreateLobbyButton->SetBorderColor(Color::UIBorderGrey);
	CreateLobbyButton->SetBackgroundColors(assets::DefaultButtonColors);
	CreateLobbyButton->SetOnPressedCallback([this]()
	{
		//Open New Lobby dialog
		FGameEvent Event(EGameEventType::NewLobby);
		FGame::Get().OnGameEvent(Event);
	}
	);

	LeaveLobbyButton = std::make_shared<FButtonWidget>(
		CreateLobbyButton->GetPosition(),
		CreateLobbyButton->GetSize(),
		DialogLayer - 1,
		L"LEAVE LOBBY",
		assets::DefaultButtonAssets,
		DialogNormalFont,
		Color::UIButtonBlue
		);
	//LeaveLobbyButton->SetBorderColor(Color::UIBorderGrey);
	LeaveLobbyButton->SetBackgroundColors(assets::DefaultButtonColors);
	LeaveLobbyButton->SetOnPressedCallback([this]()
	{
		FGame::Get().GetLobbies()->LeaveLobby();
	}
	);
	LeaveLobbyButton->Hide();

	ModifyLobbyButton = std::make_shared<FButtonWidget>(
		CreateLobbyButton->GetPosition() + Vector2(CreateLobbyButton->GetSize().x, 0.0f),
		CreateLobbyButton->GetSize(),
		DialogLayer - 1,
		L"MODIFY LOBBY",
		assets::DefaultButtonAssets,
		DialogNormalFont,
		Color::UIButtonBlue
		);
	//ModifyLobbyButton->SetBorderColor(Color::UIBorderGrey);
	ModifyLobbyButton->SetBackgroundColors(assets::DefaultButtonColors);
	ModifyLobbyButton->SetOnPressedCallback([this]()
	{
		//Open Modify Lobby dialog
		FGameEvent Event(EGameEventType::ModifyLobby);
		FGame::Get().OnGameEvent(Event);
	}
	);
	ModifyLobbyButton->Hide();

	SearchField = std::make_shared<FTextFieldWidget>(
		CreateLobbyButton->GetPosition() + Vector2(DialogSize.x - 200.0f, 0.0f),
		Vector2(175.0f, 25.0f),
		DialogLayer - 1,
		L"Search...",
		L"Assets/textfield.dds",
		DialogNormalFont,
		FTextFieldWidget::EInputType::Normal,
		EAlignmentType::Left,
		FColor(0.5f, 0.5f, 0.5f, 1.f)
	);
	SearchField->SetOnEnterPressedCallback([this](const std::wstring& SearchString)
	{
		SearchLobby(SearchString);
	});

	SearchTypeDropDown = std::make_shared<FDropDownList>(
		SearchField->GetPosition() - Vector2(175.0f, 0.0f),
		Vector2(175.0f, 25.0f),
		Vector2(175.0f, 25.0f) + Vector2(0.0f, 50.0f),
		DialogLayer - 2,
		L"Search: ",
		std::vector<std::wstring>({ LOBBY_DIALOG_SEARCH_TYPE_LEVEL, LOBBY_DIALOG_SEARCH_TYPE_BUCKET }),
		DialogNormalFont,
		EAlignmentType::Left,
		Color::UIBackgroundGrey
		);
	SearchTypeDropDown->SetBorderColor(Color::UIBorderGrey);

	SearchButton = std::make_shared<FButtonWidget>(
		SearchField->GetPosition() + Vector2(SearchField->GetSize().x, 0.0f),
		Vector2(20.0f, 20.f),
		DialogLayer - 1,
		L"",
		std::vector<std::wstring>({ L"Assets/search.dds" }),
		DialogSmallFont,
		FColor(0.5f, 0.5f, 0.5f, 1.f)
	);
	SearchButton->SetOnPressedCallback([this]()
	{
		SearchLobby(SearchField->GetText());
	}
	);

	CancelSearchButton = std::make_shared<FButtonWidget>(
		SearchButton->GetPosition(),
		SearchButton->GetSize(),
		DialogLayer - 1,
		L"",
		std::vector<std::wstring>({ L"Assets/nobutton.dds" }),
		DialogSmallFont,
		FColor(1.f, 1.f, 1.f, 1.f)
		);
	CancelSearchButton->SetOnPressedCallback([this]()
	{
		StopSearch();
	}
	);
	CancelSearchButton->Hide();

	Vector2 MainPartPosition = CreateLobbyButton->GetPosition() + Vector2(0.0f, CreateLobbyButton->GetSize().y + 5.0f);

	OwnerLabel = std::make_shared<FTextLabelWidget>(
		MainPartPosition,
		Vector2(200.0f, 30.f),
		DialogLayer - 1,
		L"Owner: ?",
		L"Assets/textfield.dds",
		FColor(1.f, 1.f, 1.f, 1.f),
		FColor(1.f, 1.f, 1.f, 1.f),
		EAlignmentType::Left);
	OwnerLabel->SetFont(DialogNormalFont);

	IsPublicLabel = std::make_shared<FTextLabelWidget>(
		MainPartPosition + Vector2(200.0f, 0.0f),
		Vector2(200.0f, 30.f),
		DialogLayer - 1,
		L"Public: ?",
		L"Assets/textfield.dds",
		FColor(1.f, 1.f, 1.f, 1.f),
		FColor(1.f, 1.f, 1.f, 1.f),
		EAlignmentType::Left);
	IsPublicLabel->SetFont(DialogNormalFont);

	LevelNameLabel = std::make_shared<FTextLabelWidget>(
		MainPartPosition + Vector2(400.0f, 0.0f),
		Vector2(200.0f, 30.f),
		DialogLayer - 1,
		L"Level: ?",
		L"Assets/textfield.dds",
		FColor(1.f, 1.f, 1.f, 1.f),
		FColor(1.f, 1.f, 1.f, 1.f),
		EAlignmentType::Left);
	LevelNameLabel->SetFont(DialogNormalFont);

	FLobbyMemberTableRowData Labels;
	Labels.ValueColors.fill(Color::White);
	Labels.Values[FLobbyMemberTableRowData::EValue::DisplayName] = L"Name";
	Labels.Values[FLobbyMemberTableRowData::EValue::IsOwner] = L"Role";
	Labels.Values[FLobbyMemberTableRowData::EValue::Skin] = L"Skin";

	LobbyMembersList = std::make_shared<FLobbyMembersListWidget>(
		MainPartPosition + Vector2(0.0f, 30.0f),
		Vector2(GetSize().x - 10.0f, GetSize().y - (MainPartPosition.y - GetPosition().y) - 35.0f),
		DialogLayer,
		L"Members:", //Title text
		10.0f, //scroller width
		std::vector<FLobbyMemberTableRowData>(),
		Labels
	);
	LobbyMembersList->SetFonts(DialogTinyFont, DialogNormalFont);

	FLobbySearchResultTableRowData SearchLabels;
	SearchLabels.Values[FLobbySearchResultTableRowData::EValue::LevelName] = L"Level";
	SearchLabels.Values[FLobbySearchResultTableRowData::EValue::OwnerDisplayName] = L"Owner";
	SearchLabels.Values[FLobbySearchResultTableRowData::EValue::NumMembers] = L"Members";
	ResultLobbiesTable = std::make_shared<FSearchResultsLobbyTableWidget>(
		MainPartPosition,
		Vector2(GetSize().x - 10.0f, GetSize().y - (MainPartPosition.y - GetPosition().y) - 5.0f),
		DialogLayer,
		L"Search results:", //Title text
		10.0f, //scroller width
		std::vector<FLobbySearchResultTableRowData>(),
		SearchLabels
		);
	ResultLobbiesTable->SetFonts(DialogSmallFont, DialogNormalFont);
	ResultLobbiesTable->Hide();
}

void FLobbiesDialog::Update()
{
	if (!IsShown())
	{
		return;
	}

	const FLobbySearch& CurrentSearch = FGame::Get().GetLobbies()->GetCurrentSearch();
	if (CurrentSearch.IsValid())
	{
		OwnerLabel->Hide();
		IsPublicLabel->Hide();
		LevelNameLabel->Hide();
		LobbyMembersList->Hide();

		ResultLobbiesTable->Show();

		const FLobbySearch& CurrentSearch = FGame::Get().GetLobbies()->GetCurrentSearch();

		//copy data to vector
		const std::vector<FLobby>& LobbiesVector = CurrentSearch.GetResults();

		std::vector<FLobbySearchResultTableRowData> Rows;
		Rows.reserve(LobbiesVector.size());
		for (uint32_t SearchIndex = 0; SearchIndex < LobbiesVector.size(); ++SearchIndex)
		{
			Rows.push_back(BuildTableRowDataFromLobby(LobbiesVector[SearchIndex], SearchIndex));
		}

		ResultLobbiesTable->RefreshData(std::move(Rows));
	}
	else
	{
		OwnerLabel->Show();
		IsPublicLabel->Show();
		LevelNameLabel->Show();
		LobbyMembersList->Show();

		ResultLobbiesTable->Hide();

		const FLobby& CurrentLobby = FGame::Get().GetLobbies()->GetCurrentLobby();

		if (CurrentLobby.IsValid())
		{
			CreateLobbyButton->Hide();
			LeaveLobbyButton->Show();

			std::wstring OwnerName = L"?";
			if (!CurrentLobby.LobbyOwnerDisplayName.empty())
			{
				OwnerName = CurrentLobby.LobbyOwnerDisplayName;
			}

			OwnerLabel->SetText(L"Owner: " + OwnerName);
			IsPublicLabel->SetText(std::wstring(L"Public: ") + ((CurrentLobby.Permission == EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED) ? L"true" : L"false"));

			std::wstring LevelName = L"?";
			if (const FLobbyAttribute* LevelAttributePtr = CurrentLobby.GetAttribute("LEVEL"))
			{
				LevelName = FStringUtils::Widen(LevelAttributePtr->AsString.c_str());
			}
			LevelNameLabel->SetText(L"Level: " + LevelName);


			FProductUserId CurrentUser;
			if (PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser()))
			{
				CurrentUser = Player->GetProductUserID();
			}

			const bool bCurrentUserIsLobbyOwner = CurrentLobby.IsOwner(CurrentUser);

			if (bCurrentUserIsLobbyOwner)
			{
				ModifyLobbyButton->Show();
			}
			else
			{
				ModifyLobbyButton->Hide();
			}

			//copy data to vector
			std::vector<FLobbyMemberTableRowData> MemberRows;
			MemberRows.reserve(CurrentLobby.Members.size());
			for (const FLobbyMember& Member : CurrentLobby.Members)
			{
				MemberRows.push_back(BuildMemberRow(Member, CurrentLobby, bCurrentUserIsLobbyOwner, Member.ProductId == CurrentUser));
			}

			LobbyMembersList->RefreshData(std::move(MemberRows));
		}
		else
		{
			LeaveLobbyButton->Hide();
			ModifyLobbyButton->Hide();
			CreateLobbyButton->Show();

			OwnerLabel->SetText(L"Owner: ?");
			IsPublicLabel->SetText(std::wstring(L"Public: ?"));
			LevelNameLabel->SetText(L"Level: ?");

			std::vector<FLobbyMemberTableRowData> MemberRows;
			LobbyMembersList->RefreshData(std::move(MemberRows));
		}
	}

	FDialog::Update();
}

void FLobbiesDialog::Create()
{
	if (BackgroundImage) BackgroundImage->Create();
	if (TitleLabel) TitleLabel->Create();
	if (CreateLobbyButton) CreateLobbyButton->Create();
	if (LeaveLobbyButton) LeaveLobbyButton->Create();
	if (ModifyLobbyButton) ModifyLobbyButton->Create();
	if (SearchField) SearchField->Create();
	if (SearchButton) SearchButton->Create();
	if (OwnerLabel) OwnerLabel->Create();
	if (IsPublicLabel) IsPublicLabel->Create();
	if (LevelNameLabel) LevelNameLabel->Create();
	if (LobbyMembersList) LobbyMembersList->Create();
	if (CancelSearchButton) CancelSearchButton->Create();
	if (ResultLobbiesTable) ResultLobbiesTable->Create();
	if (SearchTypeDropDown)
	{
		SearchTypeDropDown->Create();
		SearchTypeDropDown->SelectEntry(0);
	}

	AddWidget(BackgroundImage);
	AddWidget(TitleLabel);
	AddWidget(CreateLobbyButton);
	AddWidget(LeaveLobbyButton);
	AddWidget(ModifyLobbyButton);
	AddWidget(SearchField);

	AddWidget(SearchButton);
	AddWidget(OwnerLabel);
	AddWidget(IsPublicLabel);
	AddWidget(LevelNameLabel);
	AddWidget(LobbyMembersList);

	AddWidget(CancelSearchButton);
	AddWidget(ResultLobbiesTable);

	AddWidget(SearchTypeDropDown);

	Disable();
}

void FLobbiesDialog::SetPosition(Vector2 Pos)
{
	IWidget::SetPosition(Pos);

	if (BackgroundImage) BackgroundImage->SetPosition(Pos);
	if (TitleLabel) TitleLabel->SetPosition(Pos);
	if (CreateLobbyButton) CreateLobbyButton->SetPosition(Pos + Vector2(5.0f, TitleLabel->GetSize().y + 5.0f));
	if (LeaveLobbyButton) LeaveLobbyButton->SetPosition(CreateLobbyButton->GetPosition());
	if (ModifyLobbyButton) ModifyLobbyButton->SetPosition(CreateLobbyButton->GetPosition() + Vector2(CreateLobbyButton->GetSize().x, 0.0f));
	if (SearchField) SearchField->SetPosition(CreateLobbyButton->GetPosition() + Vector2(GetSize().x - 200.0f, 0.0f));
	if (SearchTypeDropDown) SearchTypeDropDown->SetPosition(SearchField->GetPosition() - Vector2(175.0f, 0.0f));
	if (SearchButton) SearchButton->SetPosition(SearchField->GetPosition() + Vector2(SearchField->GetSize().x, 0.0f));
	if (CancelSearchButton) CancelSearchButton->SetPosition(SearchButton->GetPosition());

	const Vector2 MainPartPosition = CreateLobbyButton->GetPosition() + Vector2(0.0f, CreateLobbyButton->GetSize().y + 5.0f);

	if (OwnerLabel) OwnerLabel->SetPosition(MainPartPosition);
	if (IsPublicLabel) IsPublicLabel->SetPosition(MainPartPosition + Vector2(200.0f, 0.0f));
	if (LevelNameLabel) LevelNameLabel->SetPosition(MainPartPosition + Vector2(400.0f, 0.0f));
	if (LobbyMembersList) LobbyMembersList->SetPosition(MainPartPosition + Vector2(0.0f, 30.0f));

	if (ResultLobbiesTable) ResultLobbiesTable->SetPosition(MainPartPosition);
}

void FLobbiesDialog::SetSize(Vector2 NewSize)
{
	IWidget::SetSize(NewSize);
	if (BackgroundImage) BackgroundImage->SetSize(Vector2(NewSize.x, NewSize.y - 10.f));
	if (TitleLabel) TitleLabel->SetSize(Vector2(NewSize.x, 30.0f));
	if (CreateLobbyButton) CreateLobbyButton->SetSize(Vector2(LOBBY_DIALOG_UPPER_BUTTON_WIDTH, 25.f));
	if (LeaveLobbyButton) LeaveLobbyButton->SetSize(CreateLobbyButton->GetSize());
	if (ModifyLobbyButton) ModifyLobbyButton->SetSize(CreateLobbyButton->GetSize());
	if (SearchField) SearchField->SetSize(Vector2(175.0f, 25.0f));
	if (SearchTypeDropDown) SearchTypeDropDown->SetSize(Vector2(175.0f, 25.0f));
	if (SearchButton) SearchButton->SetSize(Vector2(20.0f, 20.0f));
	if (CancelSearchButton) CancelSearchButton->SetSize(SearchButton->GetSize());

	const Vector2 MainPartPosition = CreateLobbyButton->GetPosition() + Vector2(0.0f, CreateLobbyButton->GetSize().y + 5.0f);

	if (OwnerLabel) OwnerLabel->SetSize(Vector2(200.0f, 30.f));
	if (IsPublicLabel) IsPublicLabel->SetSize(Vector2(200.0f, 30.f));
	if (LevelNameLabel) LevelNameLabel->SetSize(Vector2(200.0f, 30.f));
	if (LobbyMembersList) LobbyMembersList->SetSize(Vector2(GetSize().x - 10.0f, GetSize().y - (MainPartPosition.y - GetPosition().y) - 35.0f));

	if (ResultLobbiesTable) ResultLobbiesTable->SetSize(Vector2(GetSize().x - 10.0f, GetSize().y - (MainPartPosition.y - GetPosition().y) - 5.0f));
}


void FLobbiesDialog::Clear()
{
	if (LobbyMembersList) LobbyMembersList->Clear();
	if (ResultLobbiesTable) ResultLobbiesTable->Clear();
}

void FLobbiesDialog::OnGameEvent(const FGameEvent& Event)
{
	if (Event.GetType() == EGameEventType::UserLoggedIn)
	{

	}
	else if (Event.GetType() == EGameEventType::UserLoginRequiresMFA)
	{
		Disable();
		SetFocused(false);
	}
	else if (Event.GetType() == EGameEventType::UserLoginEnteredMFA)
	{
		Disable();
	}
	else if (Event.GetType() == EGameEventType::UserLoggedOut)
	{
		if (FPlayerManager::Get().GetNumPlayers() == 0)
		{
			Disable();
			Clear();
		}
	}
	else if (Event.GetType() == EGameEventType::ShowPrevUser)
	{
		Clear();
	}
	else if (Event.GetType() == EGameEventType::ShowNextUser)
	{
		Clear();
	}
	else if (Event.GetType() == EGameEventType::NewUserLogin)
	{
		Disable();
		Clear();
	}
	else if (Event.GetType() == EGameEventType::CancelLogin)
	{
		Disable();
		Clear();
	}
	else if (Event.GetType() == EGameEventType::UserConnectLoggedIn)
	{
		Enable();
	}
	else if (Event.GetType() == EGameEventType::InviteFriendToLobby)
	{
		//get current session and invite friend
		FProductUserId FriendProductUserId = Event.GetProductUserId();
		FGame::Get().GetLobbies()->SendInvite(FriendProductUserId);
	}
	else if (Event.GetType() == EGameEventType::LobbyJoined)
	{
		//Clear search
		StopSearch();
	}
}

void FLobbiesDialog::SearchLobbyByLevel(const std::wstring& LevelName)
{
	std::string SearchLevelName =  FStringUtils::Narrow(FStringUtils::ToUpper(LevelName));

	FGame::Get().GetLobbies()->SearchLobbyByLevel(SearchLevelName);
	
	//change icon
	SearchButton->Hide();
	CancelSearchButton->Show();
}

void FLobbiesDialog::SearchLobbyByBucketId(const std::wstring& BucketId)
{
	std::string SearchBucketId = FStringUtils::Narrow(BucketId);

	FGame::Get().GetLobbies()->SearchLobbyByBucketId(SearchBucketId);

	//change icon
	SearchButton->Hide();
	CancelSearchButton->Show();
}


void FLobbiesDialog::SearchLobby(const std::wstring& SearchString)
{
	if (SearchTypeDropDown)
	{
		if (SearchTypeDropDown->GetCurrentSelection() == LOBBY_DIALOG_SEARCH_TYPE_LEVEL)
		{
			SearchLobbyByLevel(SearchString);
		}
		else if (SearchTypeDropDown->GetCurrentSelection() == LOBBY_DIALOG_SEARCH_TYPE_BUCKET)
		{
			SearchLobbyByBucketId(SearchString);
		}
	}
}

void FLobbiesDialog::StopSearch()
{
	//change icon
	SearchButton->Show();
	CancelSearchButton->Hide();
	SearchField->Clear();

	FGame::Get().GetLobbies()->ClearSearch();
}