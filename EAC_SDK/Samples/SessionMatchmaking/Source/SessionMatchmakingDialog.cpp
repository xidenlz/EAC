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
#include "SessionMatchmakingDialog.h"
#include "DebugLog.h"
#include "Checkbox.h"
#include "TableView.h"
#include "NewSessionDialog.h"
#include "SessionsTableRowView.h"

namespace
{
	FSessionsTableRowData BuildTableRowDataFromSession(const FSession& Session, bool bSearchResult)
	{
		FSessionsTableRowData Result;
		Result.Values[FSessionsTableRowData::EValue::SessionName] = FStringUtils::Widen(Session.Name);

		std::wstring SessionStateString = L"None";
		if (Session.SessionState == EOS_EOnlineSessionState::EOS_OSS_Creating)
			SessionStateString = L"Creating";
		else if (Session.SessionState == EOS_EOnlineSessionState::EOS_OSS_Starting)
			SessionStateString = L"Starting";
		else if (Session.SessionState == EOS_EOnlineSessionState::EOS_OSS_Pending)
			SessionStateString = L"Pending";
		else if (Session.SessionState == EOS_EOnlineSessionState::EOS_OSS_InProgress)
			SessionStateString = L"InProgress";
		else if (Session.SessionState == EOS_EOnlineSessionState::EOS_OSS_Ending)
			SessionStateString = L"Ending";
		else if (Session.SessionState == EOS_EOnlineSessionState::EOS_OSS_Ended)
			SessionStateString = L"Ended";
		else if (Session.SessionState == EOS_EOnlineSessionState::EOS_OSS_NoSession)
			SessionStateString = L"NoSession";
		else if (Session.SessionState == EOS_EOnlineSessionState::EOS_OSS_Destroying)
			SessionStateString = L"Destroying";

		if (Session.bUpdateInProgress)
		{
			SessionStateString = L"*Updating*";
		}

		Result.Values[FSessionsTableRowData::EValue::Status] = SessionStateString;
		static wchar_t Buffer[20] = {};
		wsprintf(Buffer, L"%d/%d", Session.NumConnections, Session.MaxPlayers);
		Result.Values[FSessionsTableRowData::EValue::Players] = Buffer;

		const FSession::Attribute* LevelAttribute = Session.GetAttribute(SESSION_KEY_LEVEL);
		Result.Values[FSessionsTableRowData::EValue::Level] = FStringUtils::Widen((LevelAttribute) ? LevelAttribute->AsString : "None");

		Result.Values[FSessionsTableRowData::EValue::PresenceSession] = Session.bPresenceSession ? L"Yes" : L"No";
		Result.Values[FSessionsTableRowData::EValue::JoinInProgress] = Session.bAllowJoinInProgress ? L"Yes" : L"No";
		Result.Values[FSessionsTableRowData::EValue::Public] = (Session.PermissionLevel == EOS_EOnlineSessionPermissionLevel::EOS_OSPF_PublicAdvertised) ? 
			L"Public" : 
			(Session.PermissionLevel == EOS_EOnlineSessionPermissionLevel::EOS_OSPF_JoinViaPresence) ? L"Presence" : L"Invite Only";
		Result.Values[FSessionsTableRowData::EValue::AllowInvites] = Session.bInvitesAllowed ? L"On" : L"Off";

		Result.SessionId = Session.Id;

		Result.bActionsAvailable.fill(!bSearchResult);

		if (!bSearchResult)
		{
			if (Session.SessionState == EOS_EOnlineSessionState::EOS_OSS_InProgress || Session.SessionState == EOS_EOnlineSessionState::EOS_OSS_Starting)
			{
				Result.bActionsAvailable[FSessionsTableRowData::EAction::Start] = false;
			}
			else
			{
				Result.bActionsAvailable[FSessionsTableRowData::EAction::End] = false;
			}
		}

		return Result;
	}
}


FSessionMatchmakingDialog::FSessionMatchmakingDialog(
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
		L"SESSIONS",
		L"Assets/dialog_title.dds",
		FColor(1.f, 1.f, 1.f, 1.f),
		FColor(1.f, 1.f, 1.f, 1.f),
		EAlignmentType::Left);
	TitleLabel->SetBorderColor(Color::UIBorderGrey);
	TitleLabel->SetFont(DialogNormalFont);

	CreateSessionButton = std::make_shared<FButtonWidget>(
		Vector2(DialogPos.x + 5.0f, DialogPos.y + TitleLabel->GetSize().y + 5.0f),
		Vector2(200.0f, 25.f),
		Layer - 1,
		L"CREATE NEW SESSION",
		assets::DefaultButtonAssets,
		DialogNormalFont,
		FColor(0.5f, 0.5f, 0.5f, 1.f)
	);
	//CreateSessionButton->SetBorderColor(Color::UIBorderGrey);
	CreateSessionButton->SetBackgroundColors(assets::DefaultButtonColors);
	CreateSessionButton->SetOnPressedCallback([this]()
	{
		//Open New Session dialog
		FGameEvent Event(EGameEventType::NewSession);
		FGame::Get().OnGameEvent(Event);
	}
	);

	SearchByLevelNameField = std::make_shared<FTextFieldWidget>(
		CreateSessionButton->GetPosition() + Vector2(DialogSize.x - 200.0f, 0.0f),
		Vector2(175.0f, 25.0f),
		Layer - 1,
		L"Search by Level name...",
		L"Assets/textfield.dds",
		DialogNormalFont,
		FTextFieldWidget::EInputType::Normal,
		EAlignmentType::Left,
		FColor(0.5f, 0.5f, 0.5f, 1.f)
	);
	SearchByLevelNameField->SetOnEnterPressedCallback([this](const std::wstring& SearchString)
	{
		SearchSession(SearchString);
	});

	SearchButton = std::make_shared<FButtonWidget>(
		SearchByLevelNameField->GetPosition() + Vector2(SearchByLevelNameField->GetSize().x, 0.0f),
		Vector2(20.0f, 20.f),
		Layer - 1,
		L"",
		std::vector<std::wstring>({ L"Assets/search.dds" }),
		DialogSmallFont,
		FColor(0.5f, 0.5f, 0.5f, 1.f)
	);
	SearchButton->SetOnPressedCallback([this]()
	{
		SearchSession(SearchByLevelNameField->GetText());
	}
	);

	CancelSearchButton = std::make_shared<FButtonWidget>(
		SearchButton->GetPosition(),
		SearchButton->GetSize(),
		Layer - 1,
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

	Vector2 SessionTablePos = CreateSessionButton->GetPosition() + Vector2(0.0f, CreateSessionButton->GetSize().y + 5.0f);

	SessionsTable = std::make_shared<FSessionsTableView>(
		SessionTablePos,
		Vector2(GetSize().x - 10.0f, GetSize().y - (SessionTablePos.y - GetPosition().y) - 5.0f),
		DialogLayer - 1,
		L"Current sessions:",
		10.0f, //scroller width
		std::vector<FSessionsTableRowData>(),
		FSessionsTableRowData());

	SessionsTable->SetFonts(DialogSmallFont, DialogSmallFont);
	SessionsTable->SetOnSelectionCallback([this](const std::wstring& SelectedSessionName, const std::string& SessionId)
	{
		const bool bPresenceSession = false;
		SetSelectedSession(SelectedSessionName, SessionId, bPresenceSession);
	});
}

void FSessionMatchmakingDialog::Update()
{
	if (SessionsTable)
	{
		if (bShowingSearchResults)
		{
			const FSessionSearch& CurrentSearch = FGame::Get().GetSessions()->GetCurrentSearch();

			//copy data to vector
			std::vector<FSession> SessionsVector = CurrentSearch.GetResults();
			std::vector<FSessionsTableRowData> TableRows;
			TableRows.reserve(SessionsVector.size());
			for (const auto& NextSession : SessionsVector)
			{
				TableRows.push_back(BuildTableRowDataFromSession(NextSession, true));
			}

			SessionsTable->RefreshData(std::move(TableRows));
		}
		else
		{
			const auto& CurrentSessions = FGame::Get().GetSessions()->GetCurrentSessions();

			//copy data to vector
			std::vector<FSessionsTableRowData> TableRows;
			TableRows.reserve(CurrentSessions.size());
			for (const auto& Pair : CurrentSessions)
			{
				TableRows.push_back(BuildTableRowDataFromSession(Pair.second, false));
			}

			SessionsTable->RefreshData(std::move(TableRows));
		}
	}

	FDialog::Update();
}

void FSessionMatchmakingDialog::Create()
{
	if (BackgroundImage) BackgroundImage->Create();
	if (TitleLabel) TitleLabel->Create();
	if (CreateSessionButton) CreateSessionButton->Create();
	if (SearchByLevelNameField) SearchByLevelNameField->Create();
	if (SearchButton) SearchButton->Create();
	if (SessionsTable) SessionsTable->Create();
	if (CancelSearchButton) CancelSearchButton->Create();

	AddWidget(BackgroundImage);
	AddWidget(TitleLabel);
	AddWidget(CreateSessionButton);
	AddWidget(SearchByLevelNameField);
	AddWidget(SearchButton);
	AddWidget(SessionsTable);
	AddWidget(CancelSearchButton);

	Disable();
}

void FSessionMatchmakingDialog::SetPosition(Vector2 Pos)
{
	IWidget::SetPosition(Pos);

	if (BackgroundImage) BackgroundImage->SetPosition(Pos);
	if (TitleLabel) TitleLabel->SetPosition(Pos);
	if (CreateSessionButton) CreateSessionButton->SetPosition(Pos + Vector2(5.0f, TitleLabel->GetSize().y + 5.0f));
	if (SearchByLevelNameField) SearchByLevelNameField->SetPosition(CreateSessionButton->GetPosition() + Vector2(GetSize().x - 200.0f, 0.0f));
	if (SearchButton) SearchButton->SetPosition(SearchByLevelNameField->GetPosition() + Vector2(SearchByLevelNameField->GetSize().x, 0.0f));
	if (CancelSearchButton) CancelSearchButton->SetPosition(SearchButton->GetPosition());
	Vector2 SessionTablePos = CreateSessionButton->GetPosition() + Vector2(0.0f, CreateSessionButton->GetSize().y + 5.0f);
	if (SessionsTable) SessionsTable->SetPosition(SessionTablePos);
}

void FSessionMatchmakingDialog::SetSize(Vector2 NewSize)
{
	IWidget::SetSize(NewSize);
	if (BackgroundImage) BackgroundImage->SetSize(Vector2(NewSize.x, NewSize.y - 10.f));
	if (TitleLabel) TitleLabel->SetSize(Vector2(NewSize.x, 30.0f));
	if (CreateSessionButton) CreateSessionButton->SetSize(Vector2(200.0f, 25.f));
	if (SearchByLevelNameField) SearchByLevelNameField->SetSize(Vector2(175.0f, 25.0f));
	if (SearchButton) SearchButton->SetSize(Vector2(20.0f, 20.0f));
	if (CancelSearchButton) CancelSearchButton->SetSize(SearchButton->GetSize());
	if (SessionsTable) SessionsTable->SetSize(Vector2(GetSize().x - 10.0f, GetSize().y - (SessionsTable->GetPosition().y - GetPosition().y) - 5.0f));
}


void FSessionMatchmakingDialog::OnGameEvent(const FGameEvent& Event)
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
			SessionsTable->Clear();
		}
	}
	else if (Event.GetType() == EGameEventType::ShowPrevUser)
	{
		SessionsTable->Clear();
	}
	else if (Event.GetType() == EGameEventType::ShowNextUser)
	{
		SessionsTable->Clear();
	}
	else if (Event.GetType() == EGameEventType::NewUserLogin)
	{
		Disable();
		SessionsTable->Clear();
	}
	else if (Event.GetType() == EGameEventType::CancelLogin)
	{
		Disable();
		SessionsTable->Clear();
	}
	else if (Event.GetType() == EGameEventType::UserConnectLoggedIn)
	{
		Enable();
	}
	else if (Event.GetType() == EGameEventType::InviteFriendToSession)
	{
		//get selected session and invite friend
		if (!SelectedSessionName.empty())
		{
			FGame::Get().GetSessions()->InviteToSession(FStringUtils::Narrow(SelectedSessionName), Event.GetProductUserId());
		}
		else
		{
			FDebugLog::LogError(L"Sessions: can't invite to selected session. Selected session is invalid.");
		}
		
	}
	else if (Event.GetType() == EGameEventType::RegisterFriendWithSession)
	{
		//get selected session and invite friend
		if (!SelectedSessionName.empty())
		{
			FProductUserId FriendProductUserId = Event.GetProductUserId();

			FGame::Get().GetSessions()->Register(FStringUtils::Narrow(SelectedSessionName), FriendProductUserId);
		}
		else
		{
			FDebugLog::LogError(L"Sessions: can't invite to selected session. Selected session is invalid.");
		}

	}
	else if (Event.GetType() == EGameEventType::SessionJoined)
	{
		StopSearch();
	}
	else if (Event.GetType() == EGameEventType::RequestToJoinFriendSession)
	{
		FGame::Get().GetSessions()->RequestToJoinSession(Event.GetProductUserId());
	}
}

void FSessionMatchmakingDialog::SetSelectedSession(const std::wstring& InSessionName, const std::string& SessionId, bool bPresenceSession)
{
	SelectedSessionName = InSessionName;

	if (bShowingSearchResults && !SessionId.empty())
	{
		auto Handle = FGame::Get().GetSessions()->GetSessionHandleFromSearch(SessionId);
		FGame::Get().GetSessions()->JoinSession(Handle, bPresenceSession);
	}
}

void FSessionMatchmakingDialog::SearchSession(const std::wstring& Pattern)
{
	std::string SearchLevelName = FStringUtils::Narrow(Pattern);

	std::vector<FSession::Attribute> Attributes;

	FSession::Attribute LevelAttribute;
	LevelAttribute.Key = SESSION_KEY_LEVEL;
	LevelAttribute.ValueType = FSession::Attribute::String;
	LevelAttribute.AsString = SearchLevelName;
	LevelAttribute.Advertisement = EOS_ESessionAttributeAdvertisementType::EOS_SAAT_Advertise;

	Attributes.push_back(LevelAttribute);

	FGame::Get().GetSessions()->Search(Attributes);

	bShowingSearchResults = true;
	
	//change icon
	SearchButton->Hide();
	CancelSearchButton->Show();
}

void FSessionMatchmakingDialog::StopSearch()
{
	//change icon
	SearchButton->Show();
	CancelSearchButton->Hide();
	SearchByLevelNameField->Clear();

	bShowingSearchResults = false;
}