// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Widget.h"
#include "Font.h"
#include "ListView.h"
#include "Dialog.h"

#include <array>

class FTextLabelWidget;
class FButtonWidget;

struct FLobbyMemberTableRowData
{
	struct EValue final
	{
		// Not an `enum class` to allow easy `int` conversion.
		enum EData : int
		{
			DisplayName = 0,
			IsOwner,
			Skin,
			TalkingStatus,
			Count
		};
		EValue() = delete;
	};
	struct EAction final
	{
		// Not an `enum class` to allow easy `int` conversion.
		enum EData : int
		{
			Kick = 0,
			Promote,
			ShuffleSkin,
			MuteAudio,
			HardMuteAudio,
			ChangeColor,
			Count
		};
		EAction() = delete;
	};

	std::wstring DisplayName;
	FProductUserId UserId;

	using FValues = std::array<std::wstring, EValue::Count>;
	using FValueColors = std::array<FColor, EValue::Count>;
	using FActionsAvailable = std::array<std::pair<bool, FColor>, EAction::Count>;
	FValues Values;
	FValueColors ValueColors;
	FActionsAvailable bActionsAvailable;

	bool operator!=(const FLobbyMemberTableRowData& Other) const
	{
		return bActionsAvailable != Other.bActionsAvailable || Values != Other.Values;
	}

	FLobbyMemberTableRowData()
	{
		bActionsAvailable.fill(std::make_pair(false, Color::White));
		Values[EValue::DisplayName] = L"Name";
		Values[EValue::IsOwner] = L"Is Owner?";
		Values[EValue::Skin] = L"Skin";
		Values[EValue::TalkingStatus]= L"Talking?";
	}
};

class FLobbyMemberTableRowView : public FDialog
{
public:
	FLobbyMemberTableRowView(Vector2 Pos,
		Vector2 Size,
		UILayer Layer,
		const std::wstring& AssetFile,
		const FLobbyMemberTableRowData& InData,
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

	void SetData(const FLobbyMemberTableRowData& InData);
	void SetFont(FontPtr InFont);

protected:
	void CalcSizes(Vector2& LabelSize, Vector2& ActionSize);
	void ReadjustLayout();
	void OnPressed(size_t ActionIndex);

	FLobbyMemberTableRowData Data;

	using FRowWidgets = std::array<std::shared_ptr<FTextLabelWidget>, FLobbyMemberTableRowData::EValue::Count>;
	FRowWidgets RowWidgets;
	using FActionButtons = std::array<std::shared_ptr<FButtonWidget>, FLobbyMemberTableRowData::EAction::Count>;
	FActionButtons ActionButtons;
	std::wstring AssetFile;
	FColor BackgroundColor;
	FColor TextColor;
	FontPtr Font;

	std::function<void(size_t)> ActionPressedCallback;
};

template<>
std::shared_ptr<FLobbyMemberTableRowView> CreateListEntry(Vector2 Pos, Vector2 Size, UILayer Layer, const FLobbyMemberTableRowData& Data);