// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "LobbySearchResultTableRowView.h"
#include "TextLabel.h"
#include "Button.h"

#include "Lobbies.h"
#include "Game.h"


FLobbySearchResultTableRowView::FLobbySearchResultTableRowView(Vector2 Pos, Vector2 Size, UILayer Layer, const std::wstring& InAssetFile, const FLobbySearchResultTableRowData& InData, FColor InBackgroundColor, FColor InTextColor)
	: FDialog(Pos, Size, Layer), Data(InData), AssetFile(InAssetFile), BackgroundColor(InBackgroundColor), TextColor(InTextColor)
{
	static_assert(std::tuple_size<FLobbySearchResultTableRowView::FRowWidgets>::value == std::tuple_size<FLobbySearchResultTableRowData::FValues>::value, "FRowWidgets must be the same size as the number of values.");
	static_assert(std::tuple_size<FLobbySearchResultTableRowView::FActionButtons>::value == std::tuple_size<FLobbySearchResultTableRowData::FActionsAvailable>::value, "FActionButtons must be the same size as the number of actions.");

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
			TextColor);

		RowWidgets[i] = NextLabel;
		AddWidget(NextLabel);
	}

	//create action buttons
	std::array<std::wstring, FLobbySearchResultTableRowData::EAction::Count> actionLabels;
	std::array<FColor, FLobbySearchResultTableRowData::EAction::Count> actionColors;

	actionLabels[FLobbySearchResultTableRowData::EAction::Join] = L"JOIN";
	actionColors[FLobbySearchResultTableRowData::EAction::Join] = Color::Green;

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
		if (!Data.bActionsAvailable[Ix])
		{
			NextButton->Disable();
		}

		ActionButtons[Ix] = NextButton;
		AddWidget(NextButton);
	}
}

void FLobbySearchResultTableRowView::SetFocused(bool bValue)
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

void FLobbySearchResultTableRowView::SetPosition(Vector2 Pos)
{
	FDialog::SetPosition(Pos);

	ReadjustLayout();
}

void FLobbySearchResultTableRowView::SetSize(Vector2 NewSize)
{
	FDialog::SetSize(NewSize);

	ReadjustLayout();
}

void FLobbySearchResultTableRowView::Enable()
{
	FDialog::Enable();

	//Disable action buttons
	for (size_t Ix = 0; Ix < FLobbySearchResultTableRowData::EAction::Count; ++Ix)
	{
		if (!Data.bActionsAvailable[Ix] && ActionButtons.size() > Ix)
		{
			std::shared_ptr<FButtonWidget> ActionButton = ActionButtons[Ix];
			if (ActionButton)
			{
				ActionButton->Disable();
			}
		}
	}
}

void FLobbySearchResultTableRowView::SetOnActionPressedCallback(std::function<void(size_t)> Callback)
{
	ActionPressedCallback = Callback;
}

void FLobbySearchResultTableRowView::HideActions()
{
	for (auto ActionButton : ActionButtons)
	{
		if (ActionButton)
		{
			ActionButton->Hide();
		}
	}
}

void FLobbySearchResultTableRowView::SetData(const FLobbySearchResultTableRowData& InData)
{
	Data = InData;

	for (size_t i = 0; i < RowWidgets.size(); ++i)
	{
		std::wstring DataString = (i < InData.Values.size()) ? InData.Values[i] : L"-";
		RowWidgets[i]->SetText(DataString);
	}

	for (size_t Ix = 0; Ix < ActionButtons.size(); ++Ix)
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

	ReadjustLayout();
}

void FLobbySearchResultTableRowView::SetFont(FontPtr InFont)
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

void FLobbySearchResultTableRowView::CalcSizes(Vector2& LabelSize, Vector2& ActionSize)
{
	// Fit the action buttons into "2" columns.
	float ActionSizeRatio = 2.3f / FLobbySearchResultTableRowData::EAction::Count;
	float ColumnRatio = Data.Values.size() + ActionSizeRatio * FLobbySearchResultTableRowData::EAction::Count;
	LabelSize = Vector2(Size.x / ColumnRatio, Size.y);
	ActionSize = Vector2(LabelSize.x * ActionSizeRatio, Size.y);
}

void FLobbySearchResultTableRowView::ReadjustLayout()
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

void FLobbySearchResultTableRowView::OnPressed(size_t ActionIndex)
{
	if (ActionPressedCallback)
	{
		ActionPressedCallback(ActionIndex);
	}	

	if (ActionIndex == FLobbySearchResultTableRowData::EAction::Join)
	{
		FGame::Get().GetLobbies()->JoinSearchResult(Data.SearchResultIndex);
	}
}

template<>
std::shared_ptr<FLobbySearchResultTableRowView> CreateListEntry(Vector2 Pos, Vector2 Size, UILayer Layer, const FLobbySearchResultTableRowData& Data)
{
	return std::make_shared<FLobbySearchResultTableRowView>(Pos, Size, Layer, L"", Data, Color::DarkGray, Color::White);
}