// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Widget.h"
#include "Font.h"
#include "ListView.h"
#include "Dialog.h"

#include <array>

class FTextLabelWidget;
class FButtonWidget;

struct FSessionsTableRowData
{
	struct EValue final
	{
		// Not an `enum class` to allow easy `int` conversion.
		enum EData : int
		{
			SessionName = 0,
			Status,
			Players,
			Level,
			PresenceSession,
			JoinInProgress,
			Public,
			AllowInvites,
			Count
		};
		EValue() = delete;
	};
	struct EAction final
	{
		// Not an `enum class` to allow easy `int` conversion.
		enum EData : int
		{
			Start = 0,
			End,
			Mod,
			Leave,
			Count
		};
		EAction() = delete;
	};

	std::string SessionId;

	using FValues = std::array<std::wstring, EValue::Count>;
	using FActionsAvailable = std::array<bool, EAction::Count>;
	FValues Values;
	FActionsAvailable bActionsAvailable;

	bool operator!=(const FSessionsTableRowData& Other) const
	{
		return bActionsAvailable != Other.bActionsAvailable || Values != Other.Values;
	}

	FSessionsTableRowData()
	{
		bActionsAvailable.fill(false);
		Values[EValue::SessionName] = L"Name";
		Values[EValue::Status] = L"Status";
		Values[EValue::Players] = L"Players";
		Values[EValue::Level] = L"Level";
		Values[EValue::PresenceSession] = L"Presence?";
		Values[EValue::JoinInProgress] = L"JIP";
		Values[EValue::Public] = L"Public";
		Values[EValue::AllowInvites] = L"Invites";
	}
};

class FSessionsTableRowView : public FDialog
{
public:
	FSessionsTableRowView(Vector2 Pos,
		Vector2 Size,
		UILayer Layer,
		const std::wstring& AssetFile,
		const FSessionsTableRowData& InData,
		FColor BackgroundColor,
		FColor TextColor);

	void SetFocused(bool bValue) override;

	/** Set Position */
	void SetPosition(Vector2 Pos) override;

	/** Set Size */
	void SetSize(Vector2 NewSize) override;

	void Enable() override;

	void SetOnActionPressedCallback(std::function<void(size_t)> Callback);

	void HideActions();

	void SetData(const FSessionsTableRowData& InData);
	void SetFont(FontPtr InFont);

protected:
	void CalcSizes(Vector2& LabelSize, Vector2& ActionSize);
	void ReadjustLayout();
	void OnPressed(size_t ActionIndex);

	FSessionsTableRowData Data;

	using FRowWidgets = std::array<std::shared_ptr<FTextLabelWidget>, FSessionsTableRowData::EValue::Count>;
	FRowWidgets RowWidgets;
	using FActionButtons = std::array<std::shared_ptr<FButtonWidget>, FSessionsTableRowData::EAction::Count>;
	FActionButtons ActionButtons;
	std::wstring AssetFile;
	FColor BackgroundColor;
	FColor TextColor;
	FontPtr Font;

	std::function<void(size_t)> ActionPressedCallback;
};

template<>
std::shared_ptr<FSessionsTableRowView> CreateListEntry(Vector2 Pos, Vector2 Size, UILayer Layer, const FSessionsTableRowData& Data);

#include "TableView.h"

template <>
void FTableView<FSessionsTableRowData, FSessionsTableRowView>::OnEntrySelected(size_t Index);