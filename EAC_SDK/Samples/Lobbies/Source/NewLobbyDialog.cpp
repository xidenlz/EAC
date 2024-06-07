// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "Game.h"
#include "Menu.h"
#include "GameEvent.h"
#include "TextLabel.h"
#include "Button.h"
#include "DropDownList.h"
#include "Checkbox.h"
#include "NewLobbyDialog.h"
#include "StringUtils.h"
#include "Lobbies.h"

static const float NewLobbyDialogUIElementsXOffset = 20.0f;
static const float NewLobbyDialogUIElementsYOffset = 20.0f;
static const int MaxUsersInLobby = 20;

FNewLobbyDialog::FNewLobbyDialog(Vector2 InPos,
	Vector2 InSize,
	UILayer InLayer,
	FontPtr InNormalFont,
	FontPtr InSmallFont) :
	FDialog(InPos, InSize, InLayer)
{
	Background = std::make_shared<FSpriteWidget>(
		Position,
		InSize,
		InLayer,
		L"Assets/textfield.dds",
		Color::Black);
	AddWidget(Background);


	HeaderLabel = std::make_shared<FTextLabelWidget>(
		Vector2(Position.x, Position.y),
		Vector2(InSize.x, 30.f),
		InLayer - 1,
		L"CREATE NEW LOBBY",
		L"Assets/dialog_title.dds",
		FColor(1.f, 1.f, 1.f, 1.f));
	HeaderLabel->SetFont(InNormalFont);
	HeaderLabel->SetBorderColor(Color::UIBorderGrey);
	AddWidget(HeaderLabel);

	const Vector2 UIPosition = Position + Vector2(NewLobbyDialogUIElementsXOffset, HeaderLabel->GetSize().y + NewLobbyDialogUIElementsYOffset);
	const float UIWidth = InSize.x - 2.0f * NewLobbyDialogUIElementsXOffset;
	const Vector2 DropDownListsSize = Vector2(UIWidth / 2.0f - 5.0f, 30.0f);

	BucketIdField = std::make_shared<FTextFieldWidget>(
		UIPosition,
		Vector2(150.0f, 30.0f),
		Layer - 1,
		L"BucketId",
		L"Assets/textfield.dds",
		InNormalFont,
		FTextFieldWidget::EInputType::Normal,
		EAlignmentType::Left,
		FColor(0.5f, 0.5f, 0.5f, 1.f)
		);
	BucketIdField->SetBorderColor(Color::UIBorderGrey);

	if (bEditingLobby)
	{
		BucketIdField->SetText(FStringUtils::Widen(FGame::Get().GetLobbies()->GetCurrentLobby().BucketId));
	}

	AddWidget(BucketIdField);

	LobbyLevelDropDown = std::make_shared<FDropDownList>(
		UIPosition + Vector2(0.0f, BucketIdField->GetSize().y + 10.0f),
		DropDownListsSize,
		DropDownListsSize + Vector2(0.0f, 85.0),
		Layer - 1,
		L"Level: ",
		std::vector<std::wstring>({ L"FOREST", L"CASTLE", L"DUNGEON", L"VILLAGE", L"VALLEY"}),
		InNormalFont,
		EAlignmentType::Left,
		Color::UIBackgroundGrey
		);
	LobbyLevelDropDown->SetBorderColor(Color::UIBorderGrey);
	//AddWidget(LobbyLevelDropDown);

	MaxPlayersDropDown = std::make_shared<FDropDownList>(
		LobbyLevelDropDown->GetPosition() + Vector2(DropDownListsSize.x  + 10.0f, 0.0f),
		DropDownListsSize,
		DropDownListsSize + Vector2(0.0f, 85.0),
		Layer - 1,
		L"Max Players: ",
		GetUsersOptionsList(2, MaxUsersInLobby),
		InNormalFont,
		EAlignmentType::Left,
		Color::UIBackgroundGrey
		);
	MaxPlayersDropDown->SetBorderColor(Color::UIBorderGrey);
	//AddWidget(MaxPlayersDropDown);

	PublicCheckbox = std::make_shared<FCheckboxWidget>(
		LobbyLevelDropDown->GetPosition() + Vector2(0.0f, DropDownListsSize.y + 10.0f),
		Vector2(75.0f, 30.0f),
		Layer - 1,
		L"Public",
		L"",
		InNormalFont
		);
	AddWidget(PublicCheckbox);

	InvitesAllowedCheckbox = std::make_shared<FCheckboxWidget>(
		PublicCheckbox->GetPosition() + Vector2(PublicCheckbox->GetSize().x + 10.f, 0.f),
		Vector2(75.0f, 30.0f),
		Layer - 1,
		L"Allow Invites",
		L"",
		InNormalFont
		);
	AddWidget(InvitesAllowedCheckbox);

	PresenceCheckbox = std::make_shared<FCheckboxWidget>(
		InvitesAllowedCheckbox->GetPosition() + Vector2(InvitesAllowedCheckbox->GetSize().x + 50.f, 0.f),
		Vector2(75.0f, 30.0f),
		Layer - 1,
		L"Presence",
		L"",
		InNormalFont
		);
	AddWidget(PresenceCheckbox);

	RTCRoomCheckbox = std::make_shared<FCheckboxWidget>(
		PublicCheckbox->GetPosition() + Vector2(0.0f, PublicCheckbox->GetSize().y + 10.f),
		Vector2(75.0f, 30.0f),
		Layer - 1,
		L"RTC Voice Room",
		L"",
		InNormalFont
		);
	AddWidget(RTCRoomCheckbox);

	CreateButton = std::make_shared<FButtonWidget>(
		Vector2(Position.x + InSize.x / 2.0f - 100.0f, Position.y + InSize.y - 60.0f),
		Vector2(200.f, 45.f),
		InLayer - 1,
		L"CREATE",
		assets::DefaultButtonAssets,
		InSmallFont,
		Color::UIButtonBlue
		);
	CreateButton->SetOnPressedCallback([this]()
	{
		if (bEditingLobby)
		{
			OnModifyLobbyPressed();
		}
		else
		{
			OnCreateLobbyPressed();
		}

		FinishLobbyModification();
	});
	CreateButton->SetBackgroundColors(assets::DefaultButtonColors);
	AddWidget(CreateButton);

	CloseButton = std::make_shared<FButtonWidget>(
		Vector2(Position.x + InSize.x - 30.0f, Position.y),
		Vector2(30.0f, 30.f),
		InLayer - 1,
		L"",
		std::vector<std::wstring>({ L"Assets/nobutton.dds" }),
		InSmallFont,
		FColor(1.0f, 1.0f, 1.0f, 1.0f)
		);
	CloseButton->SetOnPressedCallback([this]()
	{
		FinishLobbyModification();
	});
	AddWidget(CloseButton);

	//Dropdown lists have to be last widgets in the list otherwise they will overlap with other widgets when expanded on DX platform.
	//This is temporary workaround (hack).
	//TODO: fix DX render to use UI Layer value correctly
	AddWidget(LobbyLevelDropDown);
	AddWidget(MaxPlayersDropDown);
}

FNewLobbyDialog::~FNewLobbyDialog()
{

}

std::vector<std::wstring> FNewLobbyDialog::GetUsersOptionsList(int MinUsers, int MaxUsers)
{
	std::vector<std::wstring> MaxUsersOptions;
	MaxUsersOptions.reserve(MaxUsers);

	for (int Option = MinUsers; Option <= MaxUsers; Option++)
	{
		MaxUsersOptions.push_back(std::to_wstring(Option));
	}

	return std::move(MaxUsersOptions);
}

void FNewLobbyDialog::SetPosition(Vector2 Pos)
{
	IWidget::SetPosition(Pos);

	Background->SetPosition(Position);
	HeaderLabel->SetPosition(Position);

	const float UIWidth = GetSize().x - 2.0f * NewLobbyDialogUIElementsXOffset;
	const Vector2 DropDownListsSize = Vector2(UIWidth / 2.0f - 5.0f, 30.0f);
	const Vector2 UIPosition = Position + Vector2(NewLobbyDialogUIElementsXOffset, HeaderLabel->GetSize().y + NewLobbyDialogUIElementsYOffset);

	BucketIdField->SetPosition(UIPosition);

	LobbyLevelDropDown->SetPosition(UIPosition + Vector2(0.0f, BucketIdField->GetSize().y + 10.0f));
	MaxPlayersDropDown->SetPosition(LobbyLevelDropDown->GetPosition() + Vector2(DropDownListsSize.x + 10.0f, 0.0f));

	// Row 1 Check Options
	PublicCheckbox->SetPosition(LobbyLevelDropDown->GetPosition() + Vector2(0.0f, DropDownListsSize.y + 10.f));
	InvitesAllowedCheckbox->SetPosition(PublicCheckbox->GetPosition() + Vector2(PublicCheckbox->GetSize().x + 10.0f, 0.f));
	PresenceCheckbox->SetPosition(InvitesAllowedCheckbox->GetPosition() + Vector2(InvitesAllowedCheckbox->GetSize().x + 50.0f, 0.f));
	// Row 2 Check Options
	RTCRoomCheckbox->SetPosition(PublicCheckbox->GetPosition() + Vector2(0.0f, PublicCheckbox->GetSize().y + 10.f));

	CreateButton->SetPosition(Vector2(Position.x + GetSize().x / 2.0f - 100.0f, Position.y + GetSize().y - 60.0f));
	CloseButton->SetPosition(Vector2(Position.x + GetSize().x - 30.0f, Position.y));
}

void FNewLobbyDialog::Create()
{
	FDialog::Create();

	LobbyLevelDropDown->SetParent(std::weak_ptr<FDialog>(std::static_pointer_cast<FDialog>(shared_from_this())));
	MaxPlayersDropDown->SetParent(std::weak_ptr<FDialog>(std::static_pointer_cast<FDialog>(shared_from_this())));
}

void FNewLobbyDialog::Update()
{
	if (BucketIdField)
	{
		// Update CreateButton state:
		// It is only enabled when user enters a non-empty bucket id.
		std::wstring BucketIdString = BucketIdField->GetText();
		bool BucketIdPresent = !BucketIdString.empty() && BucketIdString != BucketIdField->GetInitialText();
		if (BucketIdPresent)
		{
			CreateButton->Enable();
		}
		else
		{
			CreateButton->Disable();
		}
	}

	FDialog::Update();
}

void FNewLobbyDialog::SetEditingMode(bool bValue)
{
	bEditingLobby = bValue;

	//Change label
	if (HeaderLabel)
	{
		if (bEditingLobby)
		{
			HeaderLabel->SetText(L"MODIFY LOBBY");
		}
		else
		{
			HeaderLabel->SetText(L"CREATE NEW LOBBY");
		}
	}

	//Create button label
	if (CreateButton)
	{
		if (bEditingLobby)
		{
			CreateButton->SetText(L"APPLY");
		}
		else
		{
			CreateButton->SetText(L"CREATE");
		}
	}

	// Change invite enabled accessibility.
	if (InvitesAllowedCheckbox)
	{
		// Assume only lobby owner ever sees this dialog, otherwise need to check ownership
		InvitesAllowedCheckbox->Enable();
		if (bEditingLobby)
		{
			const FLobby& Lobby = FGame::Get().GetLobbies()->GetCurrentLobby();
			InvitesAllowedCheckbox->SetTicked(Lobby.AreInvitesAllowed());
		}
		else
		{
			// Default to invites allowed
			InvitesAllowedCheckbox->SetTicked(true);
		}
	}

	// Change presence enabled accessibility.
	if (PresenceCheckbox)
	{
		if (bEditingLobby)
		{
			PresenceCheckbox->Disable();
			PresenceCheckbox->SetTicked(FGame::Get().GetLobbies()->GetCurrentLobby().IsPresenceEnabled());
			// Grey-out the text to further imply the checkbox is disabled
			PresenceCheckbox->SetTextColor(FColor(0.5, 0.5, 0.5, 1.0));
		}
		else
		{
			PresenceCheckbox->Enable();
			PresenceCheckbox->SetTicked(true);
			// Ensure the text is back to white when enabled
			PresenceCheckbox->SetTextColor(Color::White);
		}
	}

	// Change RTC Room enabled accessibility.
	if (RTCRoomCheckbox)
	{
		if (bEditingLobby)
		{
			RTCRoomCheckbox->Disable();
			RTCRoomCheckbox->SetTicked(FGame::Get().GetLobbies()->GetCurrentLobby().IsRTCRoomEnabled());
			// Grey-out the text to further imply the checkbox is disabled
			RTCRoomCheckbox->SetTextColor(FColor(0.5, 0.5, 0.5, 1.0));
		}
		else
		{
			RTCRoomCheckbox->Enable();
			RTCRoomCheckbox->SetTicked(true);
			// Ensure the text is back to white when enabled
			RTCRoomCheckbox->SetTextColor(Color::White);
		}
	}

	//Lower limit on players number.
	const int CurrentNumPlayers = (int)FGame::Get().GetLobbies()->GetCurrentLobby().Members.size();
	int LowerLimitNumPlayers = (bEditingLobby) ? CurrentNumPlayers : 2;
	if (LowerLimitNumPlayers < 2)
	{
		LowerLimitNumPlayers = 2;
	}

	MaxPlayersDropDown->UpdateOptionsList(GetUsersOptionsList(LowerLimitNumPlayers, MaxUsersInLobby));
}

void FNewLobbyDialog::OnCreateLobbyPressed()
{
	FLobby Lobby;
	const std::wstring& MaxPlayersString = MaxPlayersDropDown->GetCurrentSelection();
	Lobby.MaxNumLobbyMembers = atoi(FStringUtils::Narrow(MaxPlayersString).data());
	Lobby.Permission = (PublicCheckbox->IsTicked()) ? EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED : EOS_ELobbyPermissionLevel::EOS_LPL_INVITEONLY;
	Lobby.bPresenceEnabled = PresenceCheckbox->IsTicked();
	Lobby.bRTCRoomEnabled = RTCRoomCheckbox->IsTicked();
	Lobby.bAllowInvites = InvitesAllowedCheckbox->IsTicked();
	Lobby.BucketId = FStringUtils::Narrow(BucketIdField->GetText());

	FLobbyAttribute Attribute;
	Attribute.Key = "LEVEL";
	Attribute.AsString = FStringUtils::Narrow(LobbyLevelDropDown->GetCurrentSelection());
	Attribute.ValueType = FLobbyAttribute::String;
	Attribute.Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC;

	Lobby.Attributes.push_back(Attribute);

	FGame::Get().GetLobbies()->CreateLobby(Lobby);
}

void FNewLobbyDialog::OnModifyLobbyPressed()
{
	FLobby Lobby = FGame::Get().GetLobbies()->GetCurrentLobby();
	const std::wstring& MaxPlayersString = MaxPlayersDropDown->GetCurrentSelection();
	Lobby.MaxNumLobbyMembers = atoi(FStringUtils::Narrow(MaxPlayersString).data());
	Lobby.Permission = (PublicCheckbox->IsTicked()) ? EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED : EOS_ELobbyPermissionLevel::EOS_LPL_INVITEONLY;
	Lobby.bAllowInvites = InvitesAllowedCheckbox->IsTicked();
	Lobby.BucketId = FStringUtils::Narrow(BucketIdField->GetText());

	FLobbyAttribute Attribute;
	Attribute.Key = "LEVEL";
	Attribute.AsString = FStringUtils::Narrow(LobbyLevelDropDown->GetCurrentSelection());
	Attribute.ValueType = FLobbyAttribute::String;
	Attribute.Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC;

	Lobby.Attributes.push_back(Attribute);

	FGame::Get().GetLobbies()->ModifyLobby(Lobby);
}


void FNewLobbyDialog::FinishLobbyModification()
{
	FGameEvent Event((bEditingLobby) ? EGameEventType::LobbyModificationFinished : EGameEventType::LobbyCreationFinished);
	FGame::Get().OnGameEvent(Event);
}

void FNewLobbyDialog::OnEscapePressed()
{
	FinishLobbyModification();
}