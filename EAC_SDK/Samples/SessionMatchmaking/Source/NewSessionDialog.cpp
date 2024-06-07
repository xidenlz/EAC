// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "Game.h"
#include "Menu.h"
#include "GameEvent.h"
#include "TextLabel.h"
#include "Button.h"
#include "DropDownList.h"
#include "Checkbox.h"
#include "NewSessionDialog.h"
#include "StringUtils.h"
#include "SessionMatchmaking.h"

static const float NewSessionDialogUIElementsXOffset = 20.0f;
static const float NewSessionDialogUIElementsYOffset = 20.0f;

const std::wstring PSNRestrictedPlatformString = L"PSN";
const std::wstring SwitchRestrictedPlatformString = L"Switch";
const std::wstring XboxLiveRestrictedPlatformString = L"XboxLive";

static ERestrictedPlatformType RestrictedPlatformTypeForString(const std::wstring& RestrictedPlatformString)
{
	if (RestrictedPlatformString == PSNRestrictedPlatformString)
	{
		return ERestrictedPlatformType::PSN;
	}
	else if (RestrictedPlatformString == SwitchRestrictedPlatformString)
	{
		return ERestrictedPlatformType::Switch;
	}
	else if (RestrictedPlatformString == XboxLiveRestrictedPlatformString)
	{
		return ERestrictedPlatformType::XboxLive;
	}

	return ERestrictedPlatformType::Unrestricted;
}

FNewSessionDialog::FNewSessionDialog(Vector2 InPos,
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
		L"CREATE NEW SESSION",
		L"Assets/dialog_title.dds",
		FColor(1.f, 1.f, 1.f, 1.f));
	HeaderLabel->SetFont(InNormalFont);
	HeaderLabel->SetBorderColor(Color::UIBorderGrey);
	AddWidget(HeaderLabel);

	SessionNameField = std::make_shared<FTextFieldWidget>(
		Position + Vector2(NewSessionDialogUIElementsXOffset, HeaderLabel->GetSize().y + NewSessionDialogUIElementsYOffset),
		Vector2(InSize.x - 2.0f * NewSessionDialogUIElementsXOffset, 30.0f),
		Layer - 1,
		L"New session name...",
		L"Assets/textfield.dds",
		InNormalFont,
		FTextFieldWidget::EInputType::Normal,
		EAlignmentType::Left,
		FColor(0.5f, 0.5f, 0.5f, 1.f)
		);
	SessionNameField->SetBorderColor(Color::UIBorderGrey);
	AddWidget(SessionNameField);

	const Vector2 DropDownListsSize = Vector2(SessionNameField->GetSize().x / 2.0f - 5.0f, 30.0f);

	SessionLevelDropDown = std::make_shared<FDropDownList>(
		SessionNameField->GetPosition() + Vector2(0.0f, SessionNameField->GetSize().y + 10.0f),
		DropDownListsSize,
		DropDownListsSize + Vector2(0.0f, 85.0),
		Layer - 1,
		L"Level: ",
		std::vector<std::wstring>({ L"Forest", L"Castle", L"Dungeon", L"Village", L"Valley"}),
		InNormalFont,
		EAlignmentType::Left,
		Color::UIBackgroundGrey
		);
	SessionLevelDropDown->SetBorderColor(Color::UIBorderGrey);
	SessionLevelDropDown->SetOnSelectionCallback([this](const std::wstring& Selection)
	{
		OnSessionLevelSelected(Selection);
	});
	//AddWidget(SessionLevelDropDown);

	RestrictedPlatformsDropDown = std::make_shared<FDropDownList>(
		SessionLevelDropDown->GetPosition() + Vector2(DropDownListsSize.x  + 10.0f, 0.0f),
		DropDownListsSize + Vector2(65.0f, 0.0f),
		DropDownListsSize + Vector2(65.0f, 85.0),
		Layer - 1,
		L"Allowed Platform: ",
		std::vector<std::wstring>({ L"Unrestricted", PSNRestrictedPlatformString, SwitchRestrictedPlatformString, XboxLiveRestrictedPlatformString }),
		InNormalFont,
		EAlignmentType::Left,
		Color::UIBackgroundGrey
		);
	RestrictedPlatformsDropDown->SetBorderColor(Color::UIBorderGrey);
	RestrictedPlatformsDropDown->SetOnSelectionCallback([this](const std::wstring& Selection)
	{
		OnSessionRestrictedPlatformSelected(Selection);
	});

	MaxPlayersDropDown = std::make_shared<FDropDownList>(
		SessionLevelDropDown->GetPosition() + Vector2(DropDownListsSize.x  + 10.0f, 0.0f),
		DropDownListsSize,
		DropDownListsSize + Vector2(0.0f, 85.0),
		Layer - 1,
		L"Max Players: ",
		std::vector<std::wstring>({ L"2", L"3", L"4", L"5", L"6" }),
		InNormalFont,
		EAlignmentType::Left,
		Color::UIBackgroundGrey
		);
	MaxPlayersDropDown->SetBorderColor(Color::UIBorderGrey);
	MaxPlayersDropDown->SetOnSelectionCallback([this](const std::wstring& Selection)
	{
		OnSessionMaxPlayersNumSelected(Selection);
	});

	JoinInProgressCheckbox = std::make_shared<FCheckboxWidget>(
		SessionLevelDropDown->GetPosition() + Vector2(0.0f, SessionLevelDropDown->GetSize().y + 5.0f),
		Vector2(150.0f, 30.0f),
		Layer - 1,
		L"Join In Progress",
		L"",
		InNormalFont
		);
	AddWidget(JoinInProgressCheckbox);

	PublicCheckbox = std::make_shared<FCheckboxWidget>(
		JoinInProgressCheckbox->GetPosition() + Vector2(JoinInProgressCheckbox->GetSize().x + 10.f, 0.0f),
		Vector2(150.0f, 30.0f),
		Layer - 1,
		L"Public",
		L"",
		InNormalFont
		);
	AddWidget(PublicCheckbox);

	PresenceSessionCheckbox = std::make_shared<FCheckboxWidget>(
		JoinInProgressCheckbox->GetPosition() + Vector2(0.0f, JoinInProgressCheckbox->GetSize().y + 10.0f),
		Vector2(150.0f, 30.0f),
		Layer - 1,
		L"Presence?",
		L"",
		InNormalFont
		);

	AddWidget(PresenceSessionCheckbox);

	InvitesAllowedCheckbox = std::make_shared<FCheckboxWidget>(
		PresenceSessionCheckbox->GetPosition() + Vector2(PresenceSessionCheckbox->GetSize().x + 10.f, 0.0f),
		Vector2(150.0f, 30.0f),
		Layer - 1,
		L"Invites allowed?",
		L"",
		InNormalFont
		);
	InvitesAllowedCheckbox->SetTicked(true);
	AddWidget(InvitesAllowedCheckbox);

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
		CreateSessionPressed();

		FinishSessionModification();
	});
	CreateButton->SetBackgroundColors(assets::DefaultButtonColors);
	CreateButton->Disable();
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
		FinishSessionModification();
	});
	AddWidget(CloseButton);

	//Dropdown lists have to be last widgets in the list otherwise they will overlap with other widgets when expanded on DX platform.
	//This is temporary workaround (hack).
	//TODO: fix DX render to use UI Layer value correctly
	AddWidget(RestrictedPlatformsDropDown);
	AddWidget(SessionLevelDropDown);
	AddWidget(MaxPlayersDropDown);
}

FNewSessionDialog::~FNewSessionDialog()
{

}

void FNewSessionDialog::SetPosition(Vector2 Pos)
{
	IWidget::SetPosition(Pos);

	Background->SetPosition(Position);
	HeaderLabel->SetPosition(Position);
	SessionNameField->SetPosition(Position + Vector2(NewSessionDialogUIElementsXOffset, HeaderLabel->GetSize().y + NewSessionDialogUIElementsYOffset));

	const Vector2 DropDownListsSize = Vector2(SessionNameField->GetSize().x / 2.0f - 5.0f, 30.0f);

	SessionLevelDropDown->SetPosition(SessionNameField->GetPosition() + Vector2(0.0f, SessionNameField->GetSize().y + 10.0f));
	MaxPlayersDropDown->SetPosition(SessionLevelDropDown->GetPosition() + Vector2(DropDownListsSize.x + 10.0f, 0.0f));
	RestrictedPlatformsDropDown->SetPosition(SessionLevelDropDown->GetPosition() + Vector2(0.0f, SessionLevelDropDown->GetSize().y + 5.0f));

	JoinInProgressCheckbox->SetPosition(RestrictedPlatformsDropDown->GetPosition() + Vector2(0.0f, SessionLevelDropDown->GetSize().y + 5.0f));
	PublicCheckbox->SetPosition(JoinInProgressCheckbox->GetPosition() + Vector2(JoinInProgressCheckbox->GetSize().x + 10.f, 0.0f));
	PresenceSessionCheckbox->SetPosition(JoinInProgressCheckbox->GetPosition() + Vector2(0.0f, JoinInProgressCheckbox->GetSize().y + 10.0f));
	InvitesAllowedCheckbox->SetPosition(PresenceSessionCheckbox->GetPosition() + Vector2(PresenceSessionCheckbox->GetSize().x + 10.0f, 0.0f));

	CreateButton->SetPosition(Vector2(Position.x + GetSize().x / 2.0f - 100.0f, Position.y + GetSize().y - 60.0f));
	CloseButton->SetPosition(Vector2(Position.x + GetSize().x - 30.0f, Position.y));
}

void FNewSessionDialog::Create()
{
	FDialog::Create();

	SessionLevelDropDown->SetParent(std::weak_ptr<FDialog>(std::static_pointer_cast<FDialog>(shared_from_this())));
	MaxPlayersDropDown->SetParent(std::weak_ptr<FDialog>(std::static_pointer_cast<FDialog>(shared_from_this())));
	RestrictedPlatformsDropDown->SetParent(std::weak_ptr<FDialog>(std::static_pointer_cast<FDialog>(shared_from_this())));
}

void FNewSessionDialog::Show()
{
	FDialog::Show();
	UpdateValuesOnShow();
}

void FNewSessionDialog::Toggle()
{
	FDialog::Toggle();
	UpdateValuesOnShow();
}

void FNewSessionDialog::Update()
{
	FDialog::Update();

	bool bEnableCreateButton = bSessionLevelSelected && bSessionMaxPlayersSelected &&
		SessionNameField && SessionNameField->GetInitialText() != SessionNameField->GetText();
	
	if (CreateButton)
	{
		if (bEnableCreateButton)
		{
			CreateButton->Enable();
		}
		else
		{
			CreateButton->Disable();
		}
	}
}

void FNewSessionDialog::UpdateValuesOnShow()
{
	if (FGame::Get().GetSessions()->HasPresenceSession())
	{
		PresenceSessionCheckbox->Disable();
		PresenceSessionCheckbox->SetTicked(false);
		InvitesAllowedCheckbox->Disable();
		InvitesAllowedCheckbox->SetTicked(false);
	}
	else
	{
		PresenceSessionCheckbox->Enable();
		PresenceSessionCheckbox->SetTicked(true);
		InvitesAllowedCheckbox->Enable();
		InvitesAllowedCheckbox->SetTicked(true);
	}
}

void FNewSessionDialog::CreateSessionPressed()
{
	std::wstring SessionName = SessionNameField->GetText();

	FSession Session;
	Session.bAllowJoinInProgress = JoinInProgressCheckbox->IsTicked();
	Session.bPresenceSession = PresenceSessionCheckbox->IsTicked();
	Session.bInvitesAllowed = InvitesAllowedCheckbox->IsTicked();
	const std::wstring& MaxPlayersString = MaxPlayersDropDown->GetCurrentSelection();
	Session.MaxPlayers = atoi(FStringUtils::Narrow(MaxPlayersString).data());
	Session.Name = FStringUtils::Narrow(SessionName);
	Session.PermissionLevel = (PublicCheckbox->IsTicked()) ? EOS_EOnlineSessionPermissionLevel::EOS_OSPF_PublicAdvertised : EOS_EOnlineSessionPermissionLevel::EOS_OSPF_InviteOnly;

	if (bSessionRestrictedPlatformSelected)
	{
		Session.RestrictedPlatform = RestrictedPlatformTypeForString(RestrictedPlatformsDropDown->GetCurrentSelection());
	}

	FSession::Attribute Attribute;
	Attribute.Key = SESSION_KEY_LEVEL;
	Attribute.AsString = FStringUtils::Narrow(SessionLevelDropDown->GetCurrentSelection());
	Attribute.ValueType = FSession::Attribute::String;
	Attribute.Advertisement = EOS_ESessionAttributeAdvertisementType::EOS_SAAT_Advertise;

	Session.Attributes.push_back(Attribute);

	FGame::Get().GetSessions()->CreateSession(Session);
}

void FNewSessionDialog::FinishSessionModification()
{
	FGameEvent Event(EGameEventType::SessionCreationFinished);
	FGame::Get().OnGameEvent(Event);
}

void FNewSessionDialog::OnEscapePressed()
{
	FinishSessionModification();
}