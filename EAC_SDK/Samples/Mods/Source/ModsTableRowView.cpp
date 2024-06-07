// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "ModsTableRowView.h"
#include "TextLabel.h"
#include "Button.h"
#include "TableView.h"

#include "Mods.h"
#include "Game.h"

namespace ModsTableRowDataHelpers
{
	std::wstring FormatString(const std::string& InStr)
	{
		std::string Str = InStr;
		if (!Str.size())
		{
			Str = "-";
		}

		std::wstring WStr = FStringUtils::Widen(Str);

#ifdef DXTK
		if (WStr.size() > 16)
		{
			WStr.insert(Str.size() / 2, L"\n");
		}
#endif
		return WStr;
	}

	std::string ToString(const std::wstring& InStr)
	{
		const std::string NewLine("\n");
		std::string Str = FStringUtils::Narrow(InStr);
		size_t StartPos = Str.find(NewLine);
		if (StartPos != std::string::npos)
		{
			Str.replace(StartPos, NewLine.length(), "");
		}
		return Str;
	}

	FModsTableRowData FromModDetail(const FModDetail& ModDetail)
	{
		FModsTableRowData Tmp;
		Tmp.Values[FModsTableRowData::EValue::Title] = FormatString(ModDetail.Title);
		Tmp.Values[FModsTableRowData::EValue::Version] = FormatString(ModDetail.Version);
		Tmp.Values[FModsTableRowData::EValue::NamespaceId] = FormatString(ModDetail.Id.NamespaceId);
		Tmp.Values[FModsTableRowData::EValue::ItemId] = FormatString(ModDetail.Id.ItemId);
		Tmp.Values[FModsTableRowData::EValue::ArtifactId] = FormatString(ModDetail.Id.ArtifactId);
		Tmp.bActionsAvailable[FModsTableRowData::EAction::Install] = ModDetail.State == EModState::Available;
		Tmp.bActionsAvailable[FModsTableRowData::EAction::Update] = ModDetail.State == EModState::Installed;
		Tmp.bActionsAvailable[FModsTableRowData::EAction::Uninstall] = ModDetail.State == EModState::Installed;
		return Tmp;
	}
}

FModsTableRowView::FModsTableRowView(Vector2 Pos, Vector2 Size, UILayer Layer, const std::wstring& InAssetFile, const FModsTableRowData& InData, FColor InBackgroundColor, FColor InTextColor)
	: FDialog(Pos, Size, Layer), Data(InData), AssetFile(InAssetFile), BackgroundColor(InBackgroundColor), TextColor(InTextColor)
{
	static_assert(std::tuple_size<FModsTableRowView::FRowWidgets>::value == std::tuple_size<FModsTableRowData::FValues>::value, "FRowWidgets must be the same size as the number of values.");

	Vector2 LabelSize;
	Vector2 ActionSize;
	CalcSizes(LabelSize, ActionSize);

	// create text view widgets
	for (size_t i = 0; i < RowWidgets.size(); ++i)
	{
		std::shared_ptr<FTextViewWidget> NextTextView = std::make_shared<FTextViewWidget>(
			Vector2(Pos.x + LabelSize.x * i, Pos.y),
			LabelSize,
			Layer - 1,
			Data.Values[i],
			AssetFile,
			Font,
			BackgroundColor,
			TextColor);

		// No scroller required
		NextTextView->DisableScroller();

		RowWidgets[i] = NextTextView;
		AddWidget(NextTextView);
	}

	// Offset Title a little from LHS
	RowWidgets[0]->SetBorderOffsets(Vector2(5.f, 0.f));

	//create action buttons
	std::array<std::wstring, FModsTableRowData::EAction::Count> actionLabels;
	std::array<FColor, FModsTableRowData::EAction::Count> actionColors;

	actionLabels[FModsTableRowData::EAction::Install] = L"INSTALL";
	actionColors[FModsTableRowData::EAction::Install] = Color::DarkGreen;
	actionLabels[FModsTableRowData::EAction::Update] = L"UPDATE";
	actionColors[FModsTableRowData::EAction::Update ] = Color::DarkBlue;
	actionLabels[FModsTableRowData::EAction::Uninstall] = L"UNINSTALL";
	actionColors[FModsTableRowData::EAction::Uninstall] = Color::DarkRed;

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

void FModsTableRowView::SetFocused(bool bValue)
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

void FModsTableRowView::SetPosition(Vector2 Pos)
{
	FDialog::SetPosition(Pos);

	ReadjustLayout();
}

void FModsTableRowView::SetSize(Vector2 NewSize)
{
	FDialog::SetSize(NewSize);

	ReadjustLayout();
}

void FModsTableRowView::Enable()
{
	FDialog::Enable();

	//Disable action buttons
	for (size_t Ix = 0; Ix < FModsTableRowData::EAction::Count; ++Ix)
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

void FModsTableRowView::SetOnActionPressedCallback(std::function<void(size_t)> Callback)
{
	ActionPressedCallback = Callback;
}

void FModsTableRowView::HideActions()
{
	for (auto ActionButton : ActionButtons)
	{
		if (ActionButton)
		{
			ActionButton->Hide();
		}
	}
}

void FModsTableRowView::SetData(const FModsTableRowData& InData)
{
	Data = InData;

	for (size_t i = 0; i < RowWidgets.size(); ++i)
	{
		std::wstring DataString = (i < InData.Values.size()) ? InData.Values[i] : L"-";
		RowWidgets[i]->Clear();
		RowWidgets[i]->AddLine(DataString);
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

void FModsTableRowView::SetFont(FontPtr InFont)
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

void FModsTableRowView::CalcSizes(Vector2& LabelSize, Vector2& ActionSize)
{
	// Fit the action buttons into "2" columns.
	float ActionSizeRatio = 2.3f / FModsTableRowData::EAction::Count;
	float ColumnRatio = Data.Values.size() + ActionSizeRatio * FModsTableRowData::EAction::Count;
	LabelSize = Vector2(Size.x / ColumnRatio, Size.y);
	ActionSize = Vector2(LabelSize.x * ActionSizeRatio, Size.y);
}

void FModsTableRowView::ReadjustLayout()
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

void FModsTableRowView::OnPressed(size_t ActionIndex)
{
	if (ActionPressedCallback)
	{
		ActionPressedCallback(ActionIndex);
	}

	FModId ModId;
	ModId.NamespaceId = ModsTableRowDataHelpers::ToString(Data.Values[FModsTableRowData::EValue::NamespaceId]);
	ModId.ItemId = ModsTableRowDataHelpers::ToString(Data.Values[FModsTableRowData::EValue::ItemId]);
	ModId.ArtifactId = ModsTableRowDataHelpers::ToString(Data.Values[FModsTableRowData::EValue::ArtifactId]);

	if (ActionIndex == FModsTableRowData::EAction::Install)
	{
		FGame::Get().GetMods()->InstallMod(ModId);
	}
	else if (ActionIndex == FModsTableRowData::EAction::Update)
	{
		FGame::Get().GetMods()->UpdateMod(ModId);
	}
	else if (ActionIndex == FModsTableRowData::EAction::Uninstall)
	{
		FGame::Get().GetMods()->UnInstallMod(ModId);
	}
}

template<>
std::shared_ptr<FModsTableRowView> CreateListEntry(Vector2 Pos, Vector2 Size, UILayer Layer, const FModsTableRowData& Data)
{
	return std::make_shared<FModsTableRowView>(Pos, Size, Layer, L"", Data, Color::DarkGray, Color::White);
}

template <>
void FTableView<FModsTableRowData, FModsTableRowView>::OnEntrySelected(size_t Index)
{
}