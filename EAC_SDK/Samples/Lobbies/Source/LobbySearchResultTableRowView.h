// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Widget.h"
#include "Font.h"
#include "ListView.h"
#include "Dialog.h"

#include <array>

class FTextLabelWidget;
class FButtonWidget;

struct FLobbySearchResultTableRowData
{
	struct EValue final
	{
		// Not an `enum class` to allow easy `int` conversion.
		enum EData : int
		{
			OwnerDisplayName = 0,
			NumMembers,
			LevelName,
			Count
		};
		EValue() = delete;
	};
	struct EAction final
	{
		// Not an `enum class` to allow easy `int` conversion.
		enum EData : int
		{
			Join = 0,
			Count
		};
		EAction() = delete;
	};

	uint32_t SearchResultIndex = 0;

	using FValues = std::array<std::wstring, EValue::Count>;
	using FActionsAvailable = std::array<bool, EAction::Count>;
	FValues Values;
	FActionsAvailable bActionsAvailable;

	bool operator!=(const FLobbySearchResultTableRowData& Other) const
	{
		return bActionsAvailable != Other.bActionsAvailable || Values != Other.Values;
	}

	FLobbySearchResultTableRowData()
	{
		bActionsAvailable.fill(false);
		Values[EValue::OwnerDisplayName] = L"Owner Name";
		Values[EValue::NumMembers] = L"Members";
		Values[EValue::LevelName] = L"Level";
	}
};

class FLobbySearchResultTableRowView : public FDialog
{
public:
	FLobbySearchResultTableRowView(Vector2 Pos,
		Vector2 Size,
		UILayer Layer,
		const std::wstring& AssetFile,
		const FLobbySearchResultTableRowData& InData,
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

	void SetData(const FLobbySearchResultTableRowData& InData);
	void SetFont(FontPtr InFont);

protected:
	void CalcSizes(Vector2& LabelSize, Vector2& ActionSize);
	void ReadjustLayout();
	void OnPressed(size_t ActionIndex);

	FLobbySearchResultTableRowData Data;

	using FRowWidgets = std::array<std::shared_ptr<FTextLabelWidget>, FLobbySearchResultTableRowData::EValue::Count>;
	FRowWidgets RowWidgets;
	using FActionButtons = std::array<std::shared_ptr<FButtonWidget>, FLobbySearchResultTableRowData::EAction::Count>;
	FActionButtons ActionButtons;
	std::wstring AssetFile;
	FColor BackgroundColor;
	FColor TextColor;
	FontPtr Font;

	std::function<void(size_t)> ActionPressedCallback;
};

template<>
std::shared_ptr<FLobbySearchResultTableRowView> CreateListEntry(Vector2 Pos, Vector2 Size, UILayer Layer, const FLobbySearchResultTableRowData& Data);