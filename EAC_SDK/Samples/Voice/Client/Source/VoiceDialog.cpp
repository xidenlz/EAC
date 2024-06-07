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
#include "DebugLog.h"
#include "Checkbox.h"
#include "Users.h"
#include "VoiceRoomMemberTableRowView.h"
#include "VoiceDialog.h"

static FVoiceRoomMemberTableRowData BuildMemberRow(const FVoiceRoomMember& Member)
{
	FVoiceRoomMemberTableRowData Result;

	std::wstring MemberName = Member.Player->GetDisplayName().empty() ? L"?" : Member.Player->GetDisplayName();
	Result.DisplayName = MemberName;

	Result.UserId = Member.Player->GetProductUserID();
	Result.Values[FVoiceRoomMemberTableRowData::EValue::DisplayName] = MemberName;
	Result.Volume = Member.Volume;

	Result.bEditablesAvailable[FVoiceRoomMemberTableRowData::EEditableValue::Volume] = !Member.bIsLocal;

	Result.bActionsAvailable[FVoiceRoomMemberTableRowData::EAction::Status] = false;
	Result.bActionsAvailable[FVoiceRoomMemberTableRowData::EAction::Mute] = !Member.bIsRemoteMuted;
	Result.bActionsAvailable[FVoiceRoomMemberTableRowData::EAction::RemoteMute] = Member.bIsOwner && !Member.bIsLocal;
	Result.bActionsAvailable[FVoiceRoomMemberTableRowData::EAction::Kick] = Member.bIsOwner && !Member.bIsLocal;

	Result.bIsSpeaking = Member.bIsSpeaking;
	Result.bIsMuted = Member.bIsMuted;
	Result.bIsRemoteMuted = Member.bIsRemoteMuted;

	return Result;
}

FVoiceDialog::FVoiceDialog(
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
		L"ROOM MEMBERS",
		L"Assets/dialog_title.dds",
		FColor(1.f, 1.f, 1.f, 1.f),
		FColor(1.f, 1.f, 1.f, 1.f),
		EAlignmentType::Left);
	TitleLabel->SetBorderColor(Color::UIBorderGrey);
	TitleLabel->SetFont(DialogNormalFont);

	RoomNameText = std::make_shared<FTextViewWidget>(
		TitleLabel->GetPosition() + Vector2(410.0f, 0.0f),
		Vector2(100, 30),
		Layer - 1,
		L"",
		L"Assets/textfield.dds",
		DialogNormalFont,
		FColor(0.03f, 0.03f, 0.03f, 1.f));
	RoomNameText->SetBorderColor(Color::UIBorderGrey);
	RoomNameText->SetCanSelectText(true);
	RoomNameText->SetBorderOffsets(Vector2(1.0f, 1.0f));
	AddWidget(RoomNameText);

	JoinRoomButton = std::make_shared<FButtonWidget>(
		Vector2(DialogPos.x + 5.0f, DialogPos.y + TitleLabel->GetSize().y + 5.0f),
		Vector2(100.0f, 20.f),
		Layer - 1,
		L"JOIN",
		assets::DefaultButtonAssets,
		DialogNormalFont,
		Color::UIButtonBlue
		);
	JoinRoomButton->SetBackgroundColors(assets::DefaultButtonColors);
	JoinRoomButton->SetOnPressedCallback([this]()
	{
		if (VoiceRoomMembersList)
		{
			PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
			if (Player)
			{
				if (RoomNameText)
				{
					FGameEvent Event(EGameEventType::JoinRoom, Player->GetProductUserID(), RoomNameText->GetLine(0));
					FGame::Get().OnGameEvent(Event);
				}
				else
				{
					FGameEvent Event(EGameEventType::JoinRoom, Player->GetProductUserID(), std::wstring());
					FGame::Get().OnGameEvent(Event);
				}
			}
		}
	});
	JoinRoomButton->Disable();

	LeaveRoomButton = std::make_shared<FButtonWidget>(
		Vector2(DialogPos.x + 5.0f, DialogPos.y + TitleLabel->GetSize().y + 5.0f),
		Vector2(100.0f, 20.f),
		Layer - 1,
		L"LEAVE",
		assets::DefaultButtonAssets,
		DialogNormalFont,
		Color::UIButtonBlue
		);
	//LeaveRoomButton->SetBorderColor(Color::UIBorderGrey);
	LeaveRoomButton->SetBackgroundColors(assets::DefaultButtonColors);
	LeaveRoomButton->SetOnPressedCallback([this]()
	{
		PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
		if (Player)
		{
			FGameEvent Event(EGameEventType::LeaveRoom, Player->GetProductUserID());
			FGame::Get().OnGameEvent(Event);
		}
	});
	LeaveRoomButton->Disable();

	OutputVolumeLabel = std::make_shared<FTextLabelWidget>(
		DialogPos,
		Vector2(DialogSize.x, 30.f),
		DialogLayer - 1,
		L"Volume",
		L"",
		FColor(1.f, 1.f, 1.f, 1.f),
		FColor(1.f, 1.f, 1.f, 1.f),
		EAlignmentType::Left);
	OutputVolumeLabel->SetFont(DialogNormalFont);

	OutputVolumeText = std::make_shared<FTextFieldWidget>(
		Vector2(DialogPos.x + 5.0f, DialogPos.y + TitleLabel->GetSize().y + 5.0f),
		Vector2(100.0f, 20.f),
		Layer - 1,
		L"50",
		L"Assets/textfield.dds",
		DialogNormalFont,
		FTextFieldWidget::EInputType::Normal
		);

	OutputVolumeText->SetBorderColor(Color::UIBorderGrey);
	OutputVolumeText->SetOnEnterPressedCallback([](const std::wstring& Text) {
		if (!Text.empty())
		{
			PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
			if (Player)
			{
				FGameEvent Event(EGameEventType::UpdateReceivingVolume, Text);
				FGame::Get().OnGameEvent(Event);
			}
		}
	});

	OutputParticipantVolumeLabel = std::make_shared<FTextLabelWidget>(
		DialogPos,
		Vector2(DialogSize.x, 30.f),
		DialogLayer - 1,
		L"Part. vol.",
		L"",
		FColor(1.f, 1.f, 1.f, 1.f),
		FColor(1.f, 1.f, 1.f, 1.f),
		EAlignmentType::Left);
	OutputParticipantVolumeLabel->SetFont(DialogNormalFont);

	OutputParticipantVolumeText = std::make_shared<FTextFieldWidget>(
		Vector2(DialogPos.x + 5.0f, DialogPos.y + TitleLabel->GetSize().y + 5.0f),
		Vector2(100.0f, 20.f),
		Layer - 1,
		L"50",
		L"Assets/textfield.dds",
		DialogNormalFont,
		FTextFieldWidget::EInputType::Normal
		);

	OutputParticipantVolumeText->SetBorderColor(Color::UIBorderGrey);
	OutputParticipantVolumeText->SetOnEnterPressedCallback([](const std::wstring& Text) {
		if (!Text.empty())
		{
			PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
			if (Player)
			{
				FGameEvent Event(EGameEventType::UpdateParticipantVolume, FProductUserId(), Text);
				FGame::Get().OnGameEvent(Event);
			}
		}
	});

	Vector2 MainPartPosition = Vector2(DialogPos.x + 5.0f, DialogPos.y + TitleLabel->GetSize().y + 5.0f);

	FVoiceRoomMemberTableRowData Labels;
	Labels.Values[FVoiceRoomMemberTableRowData::EValue::DisplayName] = L"Member Name";
	Labels.Edits[FVoiceRoomMemberTableRowData::EEditableValue::Volume] = L"Volume";

	VoiceRoomMembersList = std::make_shared<FVoiceRoomMembersListWidget>(
		MainPartPosition + Vector2(0.0f, 30.0f),
		Vector2(GetSize().x - 10.0f, GetSize().y - (MainPartPosition.y - GetPosition().y) - 35.0f),
		Layer,
		L"Room:", //Title text
		10.0f, //scroller width
		std::vector<FVoiceRoomMemberTableRowData>(),
		Labels
		);
	VoiceRoomMembersList->SetFonts(DialogSmallFont, DialogNormalFont);
}

void FVoiceDialog::Update()
{
	if (!IsShown())
	{
		return;
	}

	VoiceRoomMembersList->Show();

	FDialog::Update();
}

void FVoiceDialog::Create()
{
	if (BackgroundImage) BackgroundImage->Create();
	if (TitleLabel) TitleLabel->Create();
	if (VoiceRoomMembersList) VoiceRoomMembersList->Create();
	if (RoomNameText) RoomNameText->Create();
	if (JoinRoomButton) JoinRoomButton->Create();
	if (LeaveRoomButton) LeaveRoomButton->Create();
	if (OutputVolumeLabel) OutputVolumeLabel->Create();
	if (OutputVolumeText) OutputVolumeText->Create();
	if (OutputParticipantVolumeLabel) OutputParticipantVolumeLabel->Create();
	if (OutputParticipantVolumeText) OutputParticipantVolumeText->Create();

	AddWidget(BackgroundImage);
	AddWidget(TitleLabel);
	AddWidget(VoiceRoomMembersList);
	AddWidget(RoomNameText);
	AddWidget(JoinRoomButton);
	AddWidget(LeaveRoomButton);
	AddWidget(OutputVolumeLabel);
	AddWidget(OutputVolumeText);
	AddWidget(OutputParticipantVolumeLabel);
	AddWidget(OutputParticipantVolumeText);

	Disable();
}

void FVoiceDialog::SetPosition(Vector2 Pos)
{
	IWidget::SetPosition(Pos);

	if (BackgroundImage) BackgroundImage->SetPosition(Pos);
	if (TitleLabel) TitleLabel->SetPosition(Pos);
	if (RoomNameText) RoomNameText->SetPosition(Pos + Vector2(65.f, 38.f));
	if (JoinRoomButton) JoinRoomButton->SetPosition(RoomNameText->GetPosition() + Vector2(RoomNameText->GetSize().x + 5.f, 0.f));
	if (LeaveRoomButton) LeaveRoomButton->SetPosition(JoinRoomButton->GetPosition() + Vector2(JoinRoomButton->GetSize().x + 2.f, 0.f));
	if (OutputVolumeLabel) OutputVolumeLabel->SetPosition(LeaveRoomButton->GetPosition() + Vector2(LeaveRoomButton->GetSize().x + 2.f, 0.f));
	if (OutputVolumeText) OutputVolumeText->SetPosition(OutputVolumeLabel->GetPosition() + Vector2(OutputVolumeLabel->GetSize().x + 2.f, 0.f));
	if (OutputParticipantVolumeLabel) OutputParticipantVolumeLabel->SetPosition(OutputVolumeText->GetPosition() + Vector2(OutputVolumeText->GetSize().x + 2.f, 0.f));
	if (OutputParticipantVolumeText) OutputParticipantVolumeText->SetPosition(OutputParticipantVolumeLabel->GetPosition() + Vector2(OutputParticipantVolumeLabel->GetSize().x + 2.f, 0.f));

	const Vector2 MainPartPosition = GetPosition() + Vector2(0.0f, 5.0f);

	if (VoiceRoomMembersList) VoiceRoomMembersList->SetPosition(MainPartPosition + Vector2(0.0f, 30.0f));
}

void FVoiceDialog::SetSize(Vector2 NewSize)
{
	IWidget::SetSize(NewSize);

	if (BackgroundImage) BackgroundImage->SetSize(Vector2(NewSize.x, NewSize.y - 10.f));
	if (TitleLabel) TitleLabel->SetSize(Vector2(NewSize.x, 30.0f));
	if (RoomNameText) RoomNameText->SetSize(Vector2(180.0f, 25.0f));
	if (JoinRoomButton) JoinRoomButton->SetSize(Vector2(80.0f, 20.f));
	if (LeaveRoomButton) LeaveRoomButton->SetSize(Vector2(80.0f, 20.f));
	if (OutputVolumeLabel) OutputVolumeLabel->SetSize(Vector2(60.0f, 20.0f));
	if (OutputVolumeText) OutputVolumeText->SetSize(Vector2(50.0f, 20.0f));
	if (OutputParticipantVolumeLabel) OutputParticipantVolumeLabel->SetSize(Vector2(60.0f, 20.0f));
	if (OutputParticipantVolumeText) OutputParticipantVolumeText->SetSize(Vector2(50.0f, 20.0f));

	const Vector2 MainPartPosition = GetPosition() + Vector2(0.0f, 5.0f);

	if (VoiceRoomMembersList) VoiceRoomMembersList->SetSize(Vector2(GetSize().x - 10.0f, GetSize().y - (MainPartPosition.y - GetPosition().y) - 35.0f));
}


void FVoiceDialog::Clear()
{
	if (VoiceRoomMembersList) VoiceRoomMembersList->Clear();
}

void FVoiceDialog::OnGameEvent(const FGameEvent& Event)
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
		if (LeaveRoomButton) LeaveRoomButton->Disable();
	}
	else if (Event.GetType() == EGameEventType::RoomJoined)
	{
		PlayerPtr LocalPlayer = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
		if (LocalPlayer)
		{
			if (Event.GetProductUserId() == LocalPlayer->GetProductUserID())
			{
				if (JoinRoomButton) JoinRoomButton->Disable();
				if (LeaveRoomButton) LeaveRoomButton->Enable();
			}
			UpdateMemberListTable();
			if (!Event.GetFirstStr().empty())
			{
				if (RoomNameText)
				{
					RoomNameText->Clear();
					RoomNameText->AddLine(Event.GetFirstStr());
				}
			}
		}
	}
	else if (Event.GetType() == EGameEventType::RoomLeft)
	{
		PlayerPtr LocalPlayer = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
		if (LocalPlayer)
		{
			if (Event.GetProductUserId() == LocalPlayer->GetProductUserID())
			{
				if (JoinRoomButton) JoinRoomButton->Enable();
				if (LeaveRoomButton) LeaveRoomButton->Disable();
				if (RoomNameText)
				{
					RoomNameText->Clear();
				}
			}
		}
		UpdateMemberListTable();
	}
	else if (Event.GetType() == EGameEventType::RoomDataUpdated)
	{
		UpdateMemberListTable();
	}
	else if (Event.GetType() == EGameEventType::NoUserLoggedIn)
	{
		UpdateMemberListTable();
	}
}

void FVoiceDialog::UpdateMemberListTable()
{
	std::map<EOS_ProductUserId, FVoiceRoomMember> RoomMembers = FGame::Get().GetVoice()->GetRoomMembers();

	// Copy data to vector to populate table
	std::vector<FVoiceRoomMemberTableRowData> MemberRows;
	MemberRows.reserve(RoomMembers.size());
	for (std::map<EOS_ProductUserId, FVoiceRoomMember>::iterator Itr = RoomMembers.begin(); Itr != RoomMembers.end(); ++Itr)
	{
		MemberRows.push_back(BuildMemberRow(Itr->second));
	}
	VoiceRoomMembersList->RefreshData(std::move(MemberRows));
}
