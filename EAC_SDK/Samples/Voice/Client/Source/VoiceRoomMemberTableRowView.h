// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Widget.h"
#include "Font.h"
#include "ListView.h"
#include "Dialog.h"

#include <array>

class FTextLabelWidget;
class FButtonWidget;
class FCheckboxWidget;

struct FVoiceRoomMemberTableRowData
{
	struct EValue final
	{
		// Not an `enum class` to allow easy `int` conversion.
		enum EData : int
		{
			DisplayName = 0,
			Count
		};
		EValue() = delete;
	};
	struct EAction final
	{
		// Not an `enum class` to allow easy `int` conversion.
		enum EData : int
		{
			Status = 0,
			Mute,
			RemoteMute,
			Kick,
			Count
		};
		EAction() = delete;
	};
	struct EEditableValue final
	{
		// Not an `enum class` to allow easy `int` conversion.
		enum EData : int
		{
			Volume = 0,
			Count
		};
		EEditableValue() = delete;
	};


	std::wstring DisplayName;
	FProductUserId UserId;
	float Volume;
	bool bIsSpeaking = false;
	bool bIsMuted = false;
	bool bIsRemoteMuted = false;

	using FValues = std::array<std::wstring, EValue::Count>;
	using FEditableValues = std::array<std::wstring, EEditableValue::Count>;
	using FActionsAvailable = std::array<bool, EAction::Count>;
	using FEditableAvailable = std::array<bool, EEditableValue::Count>;
	FValues Values;
	FEditableValues Edits;
	FActionsAvailable bActionsAvailable;
	FEditableAvailable bEditablesAvailable;

	bool operator!=(const FVoiceRoomMemberTableRowData& Other) const
	{
		return bActionsAvailable != Other.bActionsAvailable || Values != Other.Values;
	}

	FVoiceRoomMemberTableRowData()
	{
		bActionsAvailable.fill(false);
		bEditablesAvailable.fill(false);
		Values[EValue::DisplayName] = L"Name";
	}
};

class FVoiceRoomMemberTableRowView : public FDialog
{
public:
	FVoiceRoomMemberTableRowView(Vector2 Pos,
		Vector2 Size,
		UILayer Layer,
		const std::wstring& AssetFile,
		const FVoiceRoomMemberTableRowData& InData,
		FColor BackgroundColor,
		FColor TextColor);

	void SetFocused(bool bValue) override;

	/** Set Position */
	void SetPosition(Vector2 Pos) override;

	/** Set Size */
	void SetSize(Vector2 NewSize) override;

	void Enable() override;

	void SetOnActionPressedCallback(std::function<void(size_t)> Callback);

	void SetOnActionTickedCallback(std::function<void()> Callback);

	void HideActions();

	void SetData(const FVoiceRoomMemberTableRowData& InData);
	void SetFont(FontPtr InFont);

protected:
	void CalcSizes(Vector2& LabelSize, Vector2& ActionSize);
	void ReadjustLayout();
	void OnPressed(size_t ActionIndex);
	void OnTicked(bool bIsChecked);
	void OnEditEnterPressed(size_t EditIndex, const std::wstring& FieldValue);

	FVoiceRoomMemberTableRowData Data;

	using FRowWidgets = std::array<std::shared_ptr<FTextLabelWidget>, FVoiceRoomMemberTableRowData::EValue::Count>;
	FRowWidgets RowWidgets;
	using FActionButtons = std::array<std::shared_ptr<FButtonWidget>, FVoiceRoomMemberTableRowData::EAction::Count>;
	FActionButtons ActionButtons;
	using FActionIcons = std::array<std::shared_ptr<FCheckboxWidget>, FVoiceRoomMemberTableRowData::EAction::Count>;
	FActionIcons ActionIcons;
	using FEditWidgets = std::array<std::shared_ptr<FTextFieldWidget>, FVoiceRoomMemberTableRowData::EValue::Count>;
	FEditWidgets Edits;

	std::wstring AssetFile;
	FColor BackgroundColor;
	FColor TextColor;
	FontPtr Font;

	std::function<void(size_t)> ActionPressedCallback;
};

template<>
std::shared_ptr<FVoiceRoomMemberTableRowView> CreateListEntry(Vector2 Pos, Vector2 Size, UILayer Layer, const FVoiceRoomMemberTableRowData& Data);