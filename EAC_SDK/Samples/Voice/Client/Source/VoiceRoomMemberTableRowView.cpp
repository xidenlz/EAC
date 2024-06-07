// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "VoiceRoomMemberTableRowView.h"
#include "TextLabel.h"
#include "Button.h"
#include "Checkbox.h"
#include "Voice.h"
#include "Game.h"
#include "GameEvent.h"

FVoiceRoomMemberTableRowView::FVoiceRoomMemberTableRowView(Vector2 Pos, Vector2 Size, UILayer Layer, const std::wstring& InAssetFile, const FVoiceRoomMemberTableRowData& InData, FColor InBackgroundColor, FColor InTextColor)
	: FDialog(Pos, Size, Layer), Data(InData), AssetFile(InAssetFile), BackgroundColor(InBackgroundColor), TextColor(InTextColor)
{
	static_assert(std::tuple_size<FVoiceRoomMemberTableRowView::FRowWidgets>::value == std::tuple_size<FVoiceRoomMemberTableRowData::FValues>::value, "FRowWidgets must be the same size as the number of values.");
	static_assert(std::tuple_size<FVoiceRoomMemberTableRowView::FActionButtons>::value == std::tuple_size<FVoiceRoomMemberTableRowData::FActionsAvailable>::value, "FActionButtons must be the same size as the number of actions.");
	static_assert(std::tuple_size<FVoiceRoomMemberTableRowView::FActionIcons>::value == std::tuple_size<FVoiceRoomMemberTableRowData::FActionsAvailable>::value, "FActionIcons must be the same size as the number of actions.");
	static_assert(std::tuple_size<FVoiceRoomMemberTableRowView::FEditWidgets>::value == std::tuple_size<FVoiceRoomMemberTableRowData::FEditableValues>::value, "FEditWidgets must be the same size as the number of editable values.");

	Vector2 LabelSize;
	Vector2 ActionSize;
	CalcSizes(LabelSize, ActionSize);

	//create labels
	for (size_t Ix = 0; Ix < RowWidgets.size(); ++Ix)
	{
		std::shared_ptr<FTextLabelWidget> NextLabel = std::make_shared<FTextLabelWidget>(
			Vector2(Pos.x + LabelSize.x * Ix, Pos.y),
			LabelSize,
			Layer - 1,
			Data.Values[Ix],
			AssetFile,
			BackgroundColor,
			TextColor);

		RowWidgets[Ix] = NextLabel;
		AddWidget(NextLabel);
	}

	// create edits
	for (size_t Ix = 0; Ix < Edits.size(); ++Ix)
	{
		std::shared_ptr<FTextFieldWidget> NextEdit = std::make_shared<FTextFieldWidget>(
			Vector2(Pos.x + LabelSize.x * Ix, Pos.y),
			LabelSize,
			Layer - 1,
			Data.Edits[Ix],
			L"Assets/textfield.dds",
			Font,
			FTextFieldWidget::EInputType::Normal
			);
		NextEdit->SetOnEnterPressedCallback([this, Ix](const std::wstring& FieldValue)
		{
			this->OnEditEnterPressed(Ix, FieldValue);
		});

		Edits[Ix] = NextEdit;
		NextEdit->Enable();
		AddWidget(NextEdit);
	}

	//create action buttons
	std::array<std::wstring, FVoiceRoomMemberTableRowData::EAction::Count> actionLabels;
	std::array<FColor, FVoiceRoomMemberTableRowData::EAction::Count> actionColors;
	std::array<std::wstring, FVoiceRoomMemberTableRowData::EAction::Count> actionIconEnabled;
	std::array<std::wstring, FVoiceRoomMemberTableRowData::EAction::Count> actionIconDisabled;

	actionLabels[FVoiceRoomMemberTableRowData::EAction::Status] = L"";
	actionColors[FVoiceRoomMemberTableRowData::EAction::Status] = Color::Black;
	actionIconEnabled[FVoiceRoomMemberTableRowData::EAction::Status] = L"Assets/audio_on.dds";
	actionIconDisabled[FVoiceRoomMemberTableRowData::EAction::Status] = L"Assets/audio_off.dds";

	actionLabels[FVoiceRoomMemberTableRowData::EAction::Mute] = L"MUTE";
	actionColors[FVoiceRoomMemberTableRowData::EAction::Mute] = Color::DarkGreen;

	actionLabels[FVoiceRoomMemberTableRowData::EAction::RemoteMute] = L"REMOTE MUTE";
	actionColors[FVoiceRoomMemberTableRowData::EAction::RemoteMute] = Color::DarkBlue;

	actionLabels[FVoiceRoomMemberTableRowData::EAction::Kick] = L"KICK";
	actionColors[FVoiceRoomMemberTableRowData::EAction::Kick] = Color::DarkRed;

	for (size_t Ix = 0; Ix < ActionButtons.size(); ++Ix)
	{
		if (Ix != FVoiceRoomMemberTableRowData::EAction::Status)
		{
			std::shared_ptr<FButtonWidget> NextButton = std::make_shared<FButtonWidget>(
				Vector2(Pos.x + Data.Values.size() * LabelSize.x + Ix * ActionSize.x, Pos.y),
				ActionSize,
				Layer - 1,
				actionLabels[Ix],
				assets::DefaultButtonAssets,
				Font,
				actionColors[Ix]);
			NextButton->SetOnPressedCallback([this, Ix]() { this->OnPressed(Ix); });
			if (!Data.bActionsAvailable[Ix])
			{
				NextButton->Disable();
			}

			ActionButtons[Ix] = NextButton;
			AddWidget(NextButton);
		}
	}

	for (size_t Ix = 0; Ix < ActionIcons.size(); ++Ix)
	{
		if (Ix == FVoiceRoomMemberTableRowData::EAction::Status)
		{
			std::shared_ptr<FCheckboxWidget> NextCheckbox = std::make_shared<FCheckboxWidget>(
				Vector2(Pos.x + Data.Values.size() * LabelSize.x + Ix * ActionSize.x, Pos.y),
				ActionSize,
				Layer - 1,
				actionLabels[Ix],
				L"",
				nullptr,
				actionIconDisabled[Ix],
				actionIconEnabled[Ix]);
			NextCheckbox->SetOnTickedCallback([this, Ix](bool bIsPressed) { this->OnPressed(Ix); });
			NextCheckbox->Disable();
			NextCheckbox->Show();
			ActionIcons[Ix] = NextCheckbox;
			AddWidget(NextCheckbox);
		}
	}
}

void FVoiceRoomMemberTableRowView::SetFocused(bool bValue)
{
	FDialog::SetFocused(bValue);

	if (bValue)
	{
		SetBorderColor(Color::UIDarkGrey);
	}
	else
	{
		ClearBorderColor();
	}
}

void FVoiceRoomMemberTableRowView::SetPosition(Vector2 Pos)
{
	FDialog::SetPosition(Pos);

	ReadjustLayout();
}

void FVoiceRoomMemberTableRowView::SetSize(Vector2 NewSize)
{
	FDialog::SetSize(NewSize);

	ReadjustLayout();
}

void FVoiceRoomMemberTableRowView::Enable()
{
	FDialog::Enable();

	//Disable action buttons
	for (size_t Ix = 0; Ix < FVoiceRoomMemberTableRowData::EAction::Count; ++Ix)
	{
		if (!Data.bActionsAvailable[Ix] && ActionButtons.size() > Ix)
		{
			std::shared_ptr<FButtonWidget> ActionButton = ActionButtons[Ix];
			if (ActionButton)
			{
				ActionButton->Disable();
			}
		}
		if (!Data.bActionsAvailable[Ix] && ActionIcons.size() > Ix)
		{
			std::shared_ptr<FCheckboxWidget> ActionIcon = ActionIcons[Ix];
			if (ActionIcon)
			{
				ActionIcon->Disable();
			}
		}
	}

	for (size_t Ix = 0; Ix < FVoiceRoomMemberTableRowData::EEditableValue::Count; ++Ix)
	{
		if (!Data.bEditablesAvailable[Ix] && Edits.size() > Ix)
		{
			std::shared_ptr<FTextFieldWidget> Edit = Edits[Ix];
			if (Edit)
			{
				Edit->Disable();
			}
		}
	}
}

void FVoiceRoomMemberTableRowView::SetOnActionPressedCallback(std::function<void(size_t)> Callback)
{
	ActionPressedCallback = Callback;
}

void FVoiceRoomMemberTableRowView::HideActions()
{
	for (std::shared_ptr<FButtonWidget> ActionButton : ActionButtons)
	{
		if (ActionButton)
		{
			ActionButton->Hide();
		}
	}
	for (std::shared_ptr<FCheckboxWidget> ActionIcon : ActionIcons)
	{
		if (ActionIcon)
		{
			ActionIcon->Hide();
		}
	}
}

void FVoiceRoomMemberTableRowView::SetData(const FVoiceRoomMemberTableRowData& InData)
{
	Data = InData;
	if (!InData.UserId)  // empty data
	{
		return;
	}

	for (size_t Ix = 0; Ix < RowWidgets.size(); ++Ix)
	{
		std::wstring DataString = (Ix < InData.Values.size()) ? InData.Values[Ix] : L"-";
		RowWidgets[Ix]->SetText(DataString);
	}

	for (size_t Ix = 0; Ix < Edits.size(); ++Ix)
	{
		if (Ix == FVoiceRoomMemberTableRowData::EEditableValue::Volume)
		{
			if (Edits[Ix])
			{
				if (!Edits[Ix]->IsFocused())
				{
					std::wstringstream StreamBuffer;
					StreamBuffer << InData.Volume;
					Edits[Ix]->SetText(StreamBuffer.str());
				}

				if (Data.bEditablesAvailable[Ix])
				{
					Edits[Ix]->Enable();
				}
				else
				{
					Edits[Ix]->Disable();
				}
			}
		}
	}

	for (size_t Ix = 0; Ix < ActionButtons.size(); ++Ix)
	{
		if (ActionButtons[Ix])
		{
			if (Data.bActionsAvailable[Ix])
			{
				ActionButtons[Ix]->Enable();
			}
			else
			{
				ActionButtons[Ix]->Disable();
			}
		}
	}

	for (size_t Ix = 0; Ix < ActionIcons.size(); ++Ix)
	{
		if (ActionIcons[Ix])
		{
			if (Ix == FVoiceRoomMemberTableRowData::EAction::Status)
			{
				bool bTicked = Data.bIsMuted || !Data.bIsSpeaking;
				ActionIcons[Ix]->SetTicked(bTicked);
				if (bTicked)
				{
					ActionIcons[Ix]->SetTickedColor(Data.bIsMuted ? Color::Red : Color::White);
				}
				else
				{
					ActionIcons[Ix]->SetUntickedColor(Data.bIsSpeaking ? Color::Green : Color::White);
				}
			}
		}
	}

	ReadjustLayout();
}

void FVoiceRoomMemberTableRowView::SetFont(FontPtr InFont)
{
	Font = InFont;

	for (std::shared_ptr<FTextLabelWidget> Widget : RowWidgets)
	{
		if (Widget)
		{
			Widget->SetFont(InFont);
		}
	}

	for (std::shared_ptr<FTextFieldWidget> Edit : Edits)
	{
		if (Edit)
		{
			Edit->SetFont(InFont);
		}
	}

	for (std::shared_ptr<FButtonWidget> Button : ActionButtons)
	{
		if (Button)
		{
			Button->SetFont(InFont);
		}
	}

	for (std::shared_ptr<FCheckboxWidget> Icon : ActionIcons)
	{
		if (Icon)
		{
			Icon->SetFont(InFont);
		}
	}
}

void FVoiceRoomMemberTableRowView::CalcSizes(Vector2& LabelSize, Vector2& ActionSize)
{
	// Fit the action buttons into "2" columns.
	float ActionSizeRatio = 2.3f / FVoiceRoomMemberTableRowData::EAction::Count;
	float ColumnRatio = Data.Values.size() + Data.Edits.size() + ActionSizeRatio * FVoiceRoomMemberTableRowData::EAction::Count;
	LabelSize = Vector2(Size.x / ColumnRatio, Size.y);
	ActionSize = Vector2(LabelSize.x * ActionSizeRatio, Size.y);
}

void FVoiceRoomMemberTableRowView::ReadjustLayout()
{
	Vector2 LabelSize;
	Vector2 ActionSize;
	CalcSizes(LabelSize, ActionSize);

	//resize labels
	for (size_t Ix = 0; Ix < RowWidgets.size(); ++Ix)
	{
		if (RowWidgets[Ix])
		{
			RowWidgets[Ix]->SetPosition(Vector2(Position.x + LabelSize.x * Ix, Position.y));
			RowWidgets[Ix]->SetSize(LabelSize);
		}
	}

	for (size_t Ix = 0; Ix < Edits.size(); ++Ix)
	{
		if (Edits[Ix])
		{
			Edits[Ix]->SetPosition(Vector2(Position.x + Data.Values.size() * LabelSize.x + ActionSize.x * Ix, Position.y));
			Edits[Ix]->SetSize(ActionSize);
		}
	}

	for (size_t Ix = 0; Ix < ActionButtons.size(); ++Ix)
	{
		if (ActionButtons[Ix])
		{
			ActionButtons[Ix]->SetPosition(Vector2(Position.x + Data.Edits.size() * ActionSize.x + Data.Values.size() * LabelSize.x + ActionSize.x * Ix, Position.y));
			ActionButtons[Ix]->SetSize(ActionSize);
		}
	}

	for (size_t Ix = 0; Ix < ActionIcons.size(); ++Ix)
	{
		if (ActionIcons[Ix])
		{
			ActionIcons[Ix]->SetPosition(Vector2(Position.x + Data.Values.size() * LabelSize.x + (Ix + 1) * ActionSize.x, Position.y)); // after edits
			ActionIcons[Ix]->SetSize(ActionSize);
		}
	}
}

void FVoiceRoomMemberTableRowView::OnPressed(size_t ActionIndex)
{
	if (ActionPressedCallback)
	{
		ActionPressedCallback(ActionIndex);
	}

	FProductUserId UserId  = Data.UserId;

	if (ActionIndex == FVoiceRoomMemberTableRowData::EAction::Mute)
	{
		FGame::Get().GetVoice()->LocalToggleMuteMember(UserId);
	}
	else if (ActionIndex == FVoiceRoomMemberTableRowData::EAction::RemoteMute)
	{
		FGame::Get().GetVoice()->RemoteToggleMuteMember(UserId);
	}
	else if (ActionIndex == FVoiceRoomMemberTableRowData::EAction::Kick)
	{
		FGame::Get().GetVoice()->KickMember(UserId);
	}
}

void FVoiceRoomMemberTableRowView::OnEditEnterPressed(size_t EditIndex, const std::wstring& FieldValue)
{
	if (EditIndex == FVoiceRoomMemberTableRowData::EEditableValue::Volume)
	{
		if (FieldValue.size() <= 0)
		{
			Edits[EditIndex]->SetText(L"0");
		}
		else
		{
			float Volume;
			try
			{
				Volume = std::stof(FieldValue);
			}
			catch (...)
			{
				return; // wrong format, range, etc.
			}

			if (Volume < 0.f)
			{
				Edits[EditIndex]->SetText(L"0");
			}
			else if (Volume > 100.f)
			{
				Edits[EditIndex]->SetText(L"100");
			}
		}

		FGameEvent Event(EGameEventType::UpdateParticipantVolume, Data.UserId, FieldValue);
		FGame::Get().OnGameEvent(Event);
	}
}

template<>
std::shared_ptr<FVoiceRoomMemberTableRowView> CreateListEntry(Vector2 Pos, Vector2 Size, UILayer Layer, const FVoiceRoomMemberTableRowData& Data)
{
	return std::make_shared<FVoiceRoomMemberTableRowView>(Pos, Size, Layer, L"", Data, Color::DarkGray, Color::White);
}