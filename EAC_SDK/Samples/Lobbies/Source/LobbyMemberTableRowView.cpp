// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "LobbyMemberTableRowView.h"
#include "TextLabel.h"
#include "Button.h"

#include "Lobbies.h"
#include "Game.h"


FLobbyMemberTableRowView::FLobbyMemberTableRowView(Vector2 Pos, Vector2 Size, UILayer Layer, const std::wstring& InAssetFile, const FLobbyMemberTableRowData& InData, FColor InBackgroundColor, FColor InTextColor)
	: FDialog(Pos, Size, Layer), Data(InData), AssetFile(InAssetFile), BackgroundColor(InBackgroundColor), TextColor(InTextColor)
{
	static_assert(std::tuple_size<FLobbyMemberTableRowView::FRowWidgets>::value == std::tuple_size<FLobbyMemberTableRowData::FValues>::value, "FRowWidgets must be the same size as the number of values.");
	static_assert(std::tuple_size<FLobbyMemberTableRowView::FActionButtons>::value == std::tuple_size<FLobbyMemberTableRowData::FActionsAvailable>::value, "FActionButtons must be the same size as the number of actions.");

	Vector2 LabelSize;
	Vector2 ActionSize;
	CalcSizes(LabelSize, ActionSize);

	//create labels
	for (size_t i = 0; i < RowWidgets.size(); ++i)
	{
		std::shared_ptr<FTextLabelWidget> NextLabel = std::make_shared<FTextLabelWidget>(
			Vector2(Pos.x + LabelSize.x * i, Pos.y),
			LabelSize,
			Layer - 1,
			Data.Values[i],
			AssetFile,
			BackgroundColor,
			Data.ValueColors[i]);

		RowWidgets[i] = NextLabel;
		AddWidget(NextLabel);
	}

	//create action buttons
	std::array<std::wstring, FLobbyMemberTableRowData::EAction::Count> actionLabels;
	std::array<FColor, FLobbyMemberTableRowData::EAction::Count> actionColors;

	actionLabels[FLobbyMemberTableRowData::EAction::Kick] = L"KICK";
	actionColors[FLobbyMemberTableRowData::EAction::Kick] = Color::DarkRed;
	actionLabels[FLobbyMemberTableRowData::EAction::Promote] = L"PROMOTE";
	actionColors[FLobbyMemberTableRowData::EAction::Promote] = Color::DarkGreen;
	actionLabels[FLobbyMemberTableRowData::EAction::ShuffleSkin] = L"DRAW SKIN";
	actionColors[FLobbyMemberTableRowData::EAction::ShuffleSkin] = Color::DarkBlue;
	actionLabels[FLobbyMemberTableRowData::EAction::MuteAudio] = L"MUTE";
	actionColors[FLobbyMemberTableRowData::EAction::MuteAudio] = Color::ForestGreen;
	actionLabels[FLobbyMemberTableRowData::EAction::HardMuteAudio] = L"HARD MUTE";
	actionColors[FLobbyMemberTableRowData::EAction::HardMuteAudio] = Color::ForestGreen;
	actionLabels[FLobbyMemberTableRowData::EAction::ChangeColor] = L"COLOR";
	actionColors[FLobbyMemberTableRowData::EAction::ChangeColor] = Color::ForestGreen;

	for (size_t Ix = 0; Ix < ActionButtons.size(); ++Ix)
	{
		std::shared_ptr<FButtonWidget> NextButton = std::make_shared<FButtonWidget>(
			Vector2(Pos.x + Data.Values.size() * LabelSize.x + Ix * ActionSize.x, Pos.y),
			ActionSize,
			Layer - 1,
			actionLabels[Ix],
			assets::DefaultButtonAssets,
			nullptr,
			actionColors[Ix]);
		NextButton->SetOnPressedCallback([this, Ix]() { this->OnPressed(Ix); });
		if (!Data.bActionsAvailable[Ix].first)
		{
			NextButton->Disable();
			NextButton->Hide();
		}
		else
		{
			NextButton->Enable();
			NextButton->SetBackgroundColor(Data.bActionsAvailable[Ix].second);
			NextButton->Show();
		}

		ActionButtons[Ix] = NextButton;
		AddWidget(NextButton);
	}
}

void FLobbyMemberTableRowView::SetFocused(bool bValue)
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

void FLobbyMemberTableRowView::SetPosition(Vector2 Pos)
{
	FDialog::SetPosition(Pos);

	ReadjustLayout();
}

void FLobbyMemberTableRowView::SetSize(Vector2 NewSize)
{
	FDialog::SetSize(NewSize);

	ReadjustLayout();
}

void FLobbyMemberTableRowView::Enable()
{
	FDialog::Enable();

	//Disable action buttons
	for (size_t Ix = 0; Ix < FLobbyMemberTableRowData::EAction::Count; ++Ix)
	{
		if (ActionButtons.size() > Ix)
		{
			if (std::shared_ptr<FButtonWidget>& ActionButton = ActionButtons[Ix])
			{
				if (Data.bActionsAvailable[Ix].first)
				{
					ActionButton->Enable();
					ActionButton->SetBackgroundColor(Data.bActionsAvailable[Ix].second);
					ActionButton->Show();
				}
				else
				{
					ActionButton->Disable();
					ActionButton->Hide();
				}
			}
		}
	}
}

void FLobbyMemberTableRowView::SetOnActionPressedCallback(std::function<void(size_t)> Callback)
{
	ActionPressedCallback = Callback;
}

void FLobbyMemberTableRowView::HideActions()
{
	for (std::shared_ptr<FButtonWidget>& ActionButton : ActionButtons)
	{
		if (ActionButton)
		{
			ActionButton->Disable();
			ActionButton->Hide();
		}
	}
}

void FLobbyMemberTableRowView::SetData(const FLobbyMemberTableRowData& InData)
{
	Data = InData;

	for (size_t i = 0; i < RowWidgets.size(); ++i)
	{
		RowWidgets[i]->SetText(i < InData.Values.size() ? InData.Values[i] : L"-");
		if (i < InData.ValueColors.size())
		{
			RowWidgets[i]->SetTextColor(InData.ValueColors[i]);
		}
	}

	for (size_t Ix = 0; Ix < ActionButtons.size(); ++Ix)
	{
		if (ActionButtons[Ix])
		{
			if (Data.bActionsAvailable[Ix].first)
			{
				ActionButtons[Ix]->Enable();
				ActionButtons[Ix]->SetBackgroundColor(Data.bActionsAvailable[Ix].second);
				ActionButtons[Ix]->Show();
			}
			else
			{
				ActionButtons[Ix]->Disable();
				ActionButtons[Ix]->Hide();
			}
		}
	}

	ReadjustLayout();
}

void FLobbyMemberTableRowView::SetFont(FontPtr InFont)
{
	Font = InFont;

	for (auto Widget : RowWidgets)
	{
		Widget->SetFont(InFont);
	}

	for (auto Button : ActionButtons)
	{
		Button->SetFont(InFont);
	}
}

void FLobbyMemberTableRowView::CalcSizes(Vector2& LabelSize, Vector2& ActionSize)
{
	// Fit the action buttons into "5" columns.
	float ActionSizeRatio = 5.0f / FLobbyMemberTableRowData::EAction::Count;
	float ColumnRatio = Data.Values.size() + ActionSizeRatio * FLobbyMemberTableRowData::EAction::Count;
	LabelSize = Vector2(Size.x / ColumnRatio, Size.y);
	ActionSize = Vector2(LabelSize.x * ActionSizeRatio, Size.y);
}

void FLobbyMemberTableRowView::ReadjustLayout()
{
	Vector2 LabelSize;
	Vector2 ActionSize;
	CalcSizes(LabelSize, ActionSize);

	//resize labels
	for (size_t i = 0; i < RowWidgets.size(); ++i)
	{
		RowWidgets[i]->SetPosition(Vector2(Position.x + LabelSize.x * i, Position.y));
		RowWidgets[i]->SetSize(LabelSize);
	}

	for (size_t i = 0; i < ActionButtons.size(); ++i)
	{
		ActionButtons[i]->SetPosition(Vector2(Position.x + Data.Values.size() * LabelSize.x + i * ActionSize.x, Position.y));
		ActionButtons[i]->SetSize(ActionSize);
	}
}

void FLobbyMemberTableRowView::OnPressed(size_t ActionIndex)
{
	if (ActionPressedCallback)
	{
		ActionPressedCallback(ActionIndex);
	}

	FProductUserId UserId  = Data.UserId;

	if (ActionIndex == FLobbyMemberTableRowData::EAction::Kick)
	{
		// Kick member
		FGame::Get().GetLobbies()->KickMember(UserId);
	}
	else if (ActionIndex == FLobbyMemberTableRowData::EAction::Promote)
	{
		// End lobby
		FGame::Get().GetLobbies()->PromoteMember(UserId);
	}
	else if (ActionIndex == FLobbyMemberTableRowData::EAction::ShuffleSkin)
	{
		// Leave and destroy lobby
		FGame::Get().GetLobbies()->ShuffleSkin();
	}
	else if (ActionIndex == FLobbyMemberTableRowData::EAction::MuteAudio)
	{
		// Locally mute the selected player, or disable our audio output if the player is ourselves
		FGame::Get().GetLobbies()->MuteAudio(UserId);
	}
	else if (ActionIndex == FLobbyMemberTableRowData::EAction::HardMuteAudio)
	{
		// Toggle hard mute for selected player, noone in the lobby will hear that player's audio
		FGame::Get().GetLobbies()->ToggleHardMuteMember(UserId);
	}
	else if (ActionIndex == FLobbyMemberTableRowData::EAction::ChangeColor)
	{
		// Toggle color the player
		FGame::Get().GetLobbies()->ShuffleColor();
	}
}

template<>
std::shared_ptr<FLobbyMemberTableRowView> CreateListEntry(Vector2 Pos, Vector2 Size, UILayer Layer, const FLobbyMemberTableRowData& Data)
{
	return std::make_shared<FLobbyMemberTableRowView>(Pos, Size, Layer, L"", Data, Color::DarkGray, Color::White);
}
