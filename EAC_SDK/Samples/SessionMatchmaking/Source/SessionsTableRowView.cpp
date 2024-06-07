// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "SessionsTableRowView.h"
#include "TextLabel.h"
#include "Button.h"
#include "TableView.h"

#include "SessionMatchmaking.h"
#include "Game.h"


FSessionsTableRowView::FSessionsTableRowView(Vector2 Pos, Vector2 Size, UILayer Layer, const std::wstring& InAssetFile, const FSessionsTableRowData& InData, FColor InBackgroundColor, FColor InTextColor)
	: FDialog(Pos, Size, Layer), Data(InData), AssetFile(InAssetFile), BackgroundColor(InBackgroundColor), TextColor(InTextColor)
{
	static_assert(std::tuple_size<FSessionsTableRowView::FRowWidgets>::value == std::tuple_size<FSessionsTableRowData::FValues>::value, "FRowWidgets must be the same size as the number of values.");
	static_assert(std::tuple_size<FSessionsTableRowView::FActionButtons>::value == std::tuple_size<FSessionsTableRowData::FActionsAvailable>::value, "FActionButtons must be the same size as the number of actions.");

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
	std::array<std::wstring, FSessionsTableRowData::EAction::Count> actionLabels;
	std::array<FColor, FSessionsTableRowData::EAction::Count> actionColors;

	actionLabels[FSessionsTableRowData::EAction::Start] = L"START";
	actionColors[FSessionsTableRowData::EAction::Start] = Color::DarkGreen;
	actionLabels[FSessionsTableRowData::EAction::End] = L"END";
	actionColors[FSessionsTableRowData::EAction::End] = Color::DarkGoldenrod;
	actionLabels[FSessionsTableRowData::EAction::Mod] = L"MOD";
	actionColors[FSessionsTableRowData::EAction::Mod] = Color::DarkBlue;
	actionLabels[FSessionsTableRowData::EAction::Leave] = L"LEAVE";
	actionColors[FSessionsTableRowData::EAction::Leave] = Color::DarkRed;

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

void FSessionsTableRowView::SetFocused(bool bValue)
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

void FSessionsTableRowView::SetPosition(Vector2 Pos)
{
	FDialog::SetPosition(Pos);

	ReadjustLayout();
}

void FSessionsTableRowView::SetSize(Vector2 NewSize)
{
	FDialog::SetSize(NewSize);

	ReadjustLayout();
}

void FSessionsTableRowView::Enable()
{
	FDialog::Enable();

	//Disable action buttons
	for (size_t Ix = 0; Ix < FSessionsTableRowData::EAction::Count; ++Ix)
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

void FSessionsTableRowView::SetOnActionPressedCallback(std::function<void(size_t)> Callback)
{
	ActionPressedCallback = Callback;
}

void FSessionsTableRowView::HideActions()
{
	for (auto ActionButton : ActionButtons)
	{
		if (ActionButton)
		{
			ActionButton->Hide();
		}
	}
}

void FSessionsTableRowView::SetData(const FSessionsTableRowData& InData)
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

void FSessionsTableRowView::SetFont(FontPtr InFont)
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

void FSessionsTableRowView::CalcSizes(Vector2& LabelSize, Vector2& ActionSize)
{
	// Fit the action buttons into "2" columns.
	float ActionSizeRatio = 2.3f / FSessionsTableRowData::EAction::Count;
	float ColumnRatio = Data.Values.size() + ActionSizeRatio * FSessionsTableRowData::EAction::Count;
	LabelSize = Vector2(Size.x / ColumnRatio, Size.y);
	ActionSize = Vector2(LabelSize.x * ActionSizeRatio, Size.y);
}

void FSessionsTableRowView::ReadjustLayout()
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

void FSessionsTableRowView::OnPressed(size_t ActionIndex)
{
	if (ActionPressedCallback)
	{
		ActionPressedCallback(ActionIndex);
	}

	std::string SessionName = FStringUtils::Narrow(Data.Values[FSessionsTableRowData::EValue::SessionName]);

	if (ActionIndex == FSessionsTableRowData::EAction::Start)
	{
		//Start session
		FGame::Get().GetSessions()->StartSession(SessionName);
	}
	else if (ActionIndex == FSessionsTableRowData::EAction::End)
	{
		//End session
		FGame::Get().GetSessions()->EndSession(SessionName);
	}
	else if (ActionIndex == FSessionsTableRowData::EAction::Leave)
	{ 
		// Leave and destroy session
		FGame::Get().GetSessions()->DestroySession(SessionName);
	}
	else if (ActionIndex == FSessionsTableRowData::EAction::Mod)
	{
		//Modify session
		FSession CurrentSession = FGame::Get().GetSessions()->GetSession(SessionName);
		if (CurrentSession.IsValid())
		{
			CurrentSession.MaxPlayers = 10;

			for (FSession::Attribute& Attr : CurrentSession.Attributes)
			{
				if (Attr.Key == SESSION_KEY_LEVEL)
				{
					CurrentSession.Attributes[0].AsString = "Forest";
				}
			}

			FGame::Get().GetSessions()->ModifySession(CurrentSession);
		}
	}
}

template<>
std::shared_ptr<FSessionsTableRowView> CreateListEntry(Vector2 Pos, Vector2 Size, UILayer Layer, const FSessionsTableRowData& Data)
{
	return std::make_shared<FSessionsTableRowView>(Pos, Size, Layer, L"", Data, Color::DarkGray, Color::White);
}

template <>
void FTableView<FSessionsTableRowData, FSessionsTableRowView>::OnEntrySelected(size_t Index)
{
	if (Index < Data.size())
	{
		//save selection
		if (!Data[Index].Values.empty())
		{
			CurrentlySelectedOption = Data[Index].Values[0];
		}
		else
		{
			CurrentlySelectedOption.clear();
		}

		if (OnSelectionCallback)
		{
			OnSelectionCallback(CurrentlySelectedOption, Data[Index].SessionId);
		}
	}
}