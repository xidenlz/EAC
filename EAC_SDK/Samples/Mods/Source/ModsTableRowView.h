// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Widget.h"
#include "Font.h"
#include "ListView.h"
#include "Dialog.h"

class FTextViewWidget;
class FButtonWidget;
struct FModsTableRowData;
struct FModDetail;

/**
* Helper functions. Converting data to show on the table view.
*/
namespace ModsTableRowDataHelpers
{
	/** Adding newLine to InStr and to be able to show long lines. */
	std::wstring FormatString(const std::string& InStr);
	
	/** Casts back String which might contain newLine character. */
	std::string ToString(const std::wstring& InStr);
	
	/** Converts FModDetail to Show as Table rows. */
	FModsTableRowData FromModDetail(const FModDetail& ModDetail);
}

/** Holds single Mod related data and actions that can be applied to that Mod. */
struct FModsTableRowData
{
	/** Mod Data. */
	struct EValue final
	{
		enum EData : int
		{
			Title = 0,
			Version,
			NamespaceId,
			ItemId,
			ArtifactId,
			Count
		};
		EValue() = delete;
	};

	/** Actions that can be applied to the Mod. */
	struct EAction final
	{
		enum EData : int
		{
			Install = 0,
			Update,
			Uninstall,
			Count
		};
		EAction() = delete;
	};

	using FValues = std::array<std::wstring, EValue::Count>;
	using FActionsAvailable = std::array<bool, EAction::Count>;
	FValues Values;
	FActionsAvailable bActionsAvailable;

	bool operator!=(const FModsTableRowData& Other) const
	{
		return bActionsAvailable != Other.bActionsAvailable || Values != Other.Values;
	}

	/** default ctor */
	FModsTableRowData()
	{
		bActionsAvailable.fill(false);
		Values[EValue::Title] = L"Title";
		Values[EValue::Version] = L"Version";
		Values[EValue::NamespaceId] = L"Namespace Id";
		Values[EValue::ItemId] = L"Item Id";
		Values[EValue::ArtifactId] = L"Artifact Id";
	}
};

/** Class for representing view of the Mods data in a row. */
class FModsTableRowView : public FDialog
{
public:
	FModsTableRowView(Vector2 Pos,
		Vector2 Size,
		UILayer Layer,
		const std::wstring& AssetFile,
		const FModsTableRowData& InData,
		FColor BackgroundColor,
		FColor TextColor);

	void SetFocused(bool bValue) override;

	/** Set Position */
	void SetPosition(Vector2 Pos) override;

	/** Set Size */
	void SetSize(Vector2 NewSize) override;
	
	/* Enables Dialog and Updates Action button states.*/
	void Enable() override;

	void SetOnActionPressedCallback(std::function<void(size_t)> Callback);
	
	/** Hides Actions. */
	void HideActions();

	/** Sets row data to be displayed. */
	void SetData(const FModsTableRowData& InData);
	
	/** Sets Font to all Row elements*/
	void SetFont(FontPtr InFont);

protected:

	/** Calculates Label and Action sizes.*/
	void CalcSizes(Vector2& LabelSize, Vector2& ActionSize);
	
	/** Refreshes Layout */
	void ReadjustLayout();
	
	/** Calls Action Callback */
	void OnPressed(size_t ActionIndex);

	/** Row data that to which action will be applied */
	FModsTableRowData Data;

	using FRowWidgets = std::array<std::shared_ptr<FTextViewWidget>, FModsTableRowData::EValue::Count>;
	/** Row widgets to show data of the Mod. */
	FRowWidgets RowWidgets;
	using FActionButtons = std::array<std::shared_ptr<FButtonWidget>, FModsTableRowData::EAction::Count>;
	/** Actions available for the Mod. */
	FActionButtons ActionButtons;
	std::wstring AssetFile;
	FColor BackgroundColor;
	FColor TextColor;
	FontPtr Font;

	std::function<void(size_t)> ActionPressedCallback;
};

template<>
std::shared_ptr<FModsTableRowView> CreateListEntry(Vector2 Pos, Vector2 Size, UILayer Layer, const FModsTableRowData& Data);

#include "TableView.h"

template <>
void FTableView<FModsTableRowData, FModsTableRowView>::OnEntrySelected(size_t Index);