// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "Game.h"
#include "Menu.h"
#include "GameEvent.h"
#include "TextLabel.h"
#include "Button.h"
#include "DropDownList.h"
#include "StringUtils.h"
#include "Voice.h"
#include "VoiceSetupDialog.h"

FVoiceSetupDialog::FVoiceSetupDialog(Vector2 InPos,
	Vector2 InSize,
	UILayer InLayer,
	FontPtr InNormalFont,
	FontPtr InSmallFont) :
	FDialog(InPos, InSize, InLayer)
{
	Background = std::make_shared<FSpriteWidget>(
		Position,
		InSize,
		InLayer,
		L"Assets/textfield.dds");
	AddWidget(Background);

	HeaderLabel = std::make_shared<FTextLabelWidget>(
		Vector2(Position.x, Position.y),
		Vector2(InSize.x, 30.f),
		InLayer - 1,
		L"SETUP",
		L"Assets/dialog_title.dds",
		FColor(1.f, 1.f, 1.f, 1.f),
		FColor(1.f, 1.f, 1.f, 1.f),
		EAlignmentType::Left);
	HeaderLabel->SetFont(InNormalFont);
	HeaderLabel->SetBorderColor(Color::UIBorderGrey);
	AddWidget(HeaderLabel);

	const Vector2 UIPosition = Position + Vector2(0.0f, HeaderLabel->GetSize().y);
	const float UIWidth = InSize.x - 2.0f;
	const Vector2 DropDownListsSize = Vector2(UIWidth - 100.0f, 30.0f);

	OutputDeviceDropDown = std::make_shared<FDropDownList>(
		UIPosition,
		DropDownListsSize,
		DropDownListsSize + Vector2(0.0f, 80.0),
		Layer - 1,
		L"OUTPUT DEVICE: ",
		std::vector<std::wstring>({ L"No Device",}),
		InNormalFont,
		EAlignmentType::Left,
		Color::UIBackgroundGrey
		);
	OutputDeviceDropDown->SetBorderColor(Color::UIBorderGrey);
	OutputDeviceDropDown->SetOnSelectionCallback([this](const std::wstring& Selection)
	{
		OnOutputDeviceSelected(Selection);
	});
	OutputDeviceDropDown->SetOnExpandedCallback([this]()
	{
		OnOutputDeviceDropdownExpanded();
	});
	OutputDeviceDropDown->SetOnCollapsedCallback([this]()
	{
		OnOutputDeviceDropdownCollapsed();
	});
	//AddWidget(OutputDeviceDropDown);

	InputDeviceDropDown = std::make_shared<FDropDownList>(
		OutputDeviceDropDown->GetPosition() + Vector2(DropDownListsSize.x  + 10.0f, 0.0f),
		DropDownListsSize,
		DropDownListsSize + Vector2(0.0f, 60.0),
		Layer - 1,
		L"INPUT DEVICE: ",
		std::vector<std::wstring>({ L"No Device" }),
		InNormalFont,
		EAlignmentType::Left,
		Color::UIBackgroundGrey
		);
	InputDeviceDropDown->SetBorderColor(Color::UIBorderGrey);
	InputDeviceDropDown->SetOnSelectionCallback([this](const std::wstring& Selection)
	{
		OnInputDeviceSelected(Selection);
	});
	InputDeviceDropDown->SetOnExpandedCallback([this]()
	{
		OnInputDeviceDropdownExpanded();
	});
	InputDeviceDropDown->SetOnCollapsedCallback([this]()
	{
		OnInputDeviceDropdownCollapsed();
	});
	//AddWidget(InputDeviceDropDown);

	//Dropdown lists have to be last widgets in the list otherwise they will overlap with other widgets when expanded on DX platform.
	//This is temporary workaround (hack).
	//TODO: fix DX render to use UI Layer value correctly
	AddWidget(OutputDeviceDropDown);
	AddWidget(InputDeviceDropDown);
}

FVoiceSetupDialog::~FVoiceSetupDialog()
{

}

void FVoiceSetupDialog::SetPosition(Vector2 Pos)
{
	IWidget::SetPosition(Pos);

	if (Background) Background->SetPosition(Position);
	if (HeaderLabel) HeaderLabel->SetPosition(Position);

	const float UIWidth = GetSize().x - 2.0f;
	const float UIHeight = GetSize().y - 2.0f;
	const Vector2 UIPosition = Position + Vector2(5.0f, HeaderLabel->GetSize().y + 10.f);

	if (OutputDeviceDropDown) OutputDeviceDropDown->SetPosition(UIPosition);
	if (InputDeviceDropDown) InputDeviceDropDown->SetPosition(Vector2(UIPosition.x, OutputDeviceDropDown->GetPosition().y + 50.f));
}

void FVoiceSetupDialog::SetSize(Vector2 NewSize)
{
	IWidget::SetSize(NewSize);

	if (Background) Background->SetSize(Vector2(NewSize.x, NewSize.y));
	if (HeaderLabel) HeaderLabel->SetSize(Vector2(NewSize.x, 30.0f));
	if (OutputDeviceDropDown) OutputDeviceDropDown->SetSize(Vector2(NewSize.x - 10.f, 30.0f));
	if (InputDeviceDropDown) InputDeviceDropDown->SetSize(Vector2(NewSize.x - 10.f, 30.0f));
}

void FVoiceSetupDialog::Create()
{
	FDialog::Create();

	if (OutputDeviceDropDown) OutputDeviceDropDown->SetParent(std::weak_ptr<FDialog>(std::static_pointer_cast<FDialog>(shared_from_this())));
	if (InputDeviceDropDown) InputDeviceDropDown->SetParent(std::weak_ptr<FDialog>(std::static_pointer_cast<FDialog>(shared_from_this())));
}

void FVoiceSetupDialog::OnEscapePressed()
{

}

void FVoiceSetupDialog::OnGameEvent(const FGameEvent& Event)
{
	if (Event.GetType() == EGameEventType::AudioInputDevicesUpdated)
	{
		UpdateInputDevices(Event.GetFirstStr());
	}
	else if (Event.GetType() == EGameEventType::AudioOutputDevicesUpdated)
	{
		UpdateOutputDevices(Event.GetFirstStr());
	}
}

/**
 * We want to keep the currently selected device if possible when the list of available devices changes (e.g: The user plugged/unplugged a device)
 */
static int PickDeviceIndex(const std::vector<std::wstring>& DeviceNames, const std::wstring& DefaultDeviceName, const std::wstring& CurrentlySelected)
{
	// If the currently selected device still exists, we pick that
	for (int Index = 0; Index < static_cast<int>(DeviceNames.size()); Index++)
	{
		if (DeviceNames[Index] == CurrentlySelected)
		{
			return Index;
		}
	}

	// Find the address of the Default device
	for (int Index = 0; Index < static_cast<int>(DeviceNames.size()); Index++)
	{
		if (DeviceNames[Index] == DefaultDeviceName)
		{
			return Index;
		}
	}

	return 0;
}

void FVoiceSetupDialog::UpdateInputDevices(const std::wstring& DefaultDeviceName)
{
	if (InputDeviceDropDown)
	{
		const std::vector<std::wstring>& DeviceNames = FGame::Get().GetVoice()->GetAudioInputDeviceNames();
		int SelectedIndex = PickDeviceIndex(DeviceNames, DefaultDeviceName, InputDeviceDropDown->GetCurrentSelection());
		InputDeviceDropDown->UpdateOptionsList(DeviceNames, SelectedIndex);
	}
}

void FVoiceSetupDialog::UpdateOutputDevices(const std::wstring& DefaultDeviceName)
{
	if (OutputDeviceDropDown)
	{
		const std::vector<std::wstring>& DeviceNames = FGame::Get().GetVoice()->GetAudioOutputDeviceNames();
		int SelectedIndex = PickDeviceIndex(DeviceNames, DefaultDeviceName, OutputDeviceDropDown->GetCurrentSelection());
		OutputDeviceDropDown->UpdateOptionsList(DeviceNames, SelectedIndex);
	}
}

void FVoiceSetupDialog::OnOutputDeviceSelected(const std::wstring& Selection)
{
	FGame::Get().GetVoice()->SetAudioOutputDeviceFromName(Selection);
}

void FVoiceSetupDialog::OnInputDeviceSelected(const std::wstring& Selection)
{
	FGame::Get().GetVoice()->SetAudioInputDeviceFromName(Selection);
}

void FVoiceSetupDialog::OnOutputDeviceDropdownExpanded()
{
	if (InputDeviceDropDown)
	{
		InputDeviceDropDown->Hide();
	}
}

void FVoiceSetupDialog::OnOutputDeviceDropdownCollapsed()
{
	if (InputDeviceDropDown)
	{
		InputDeviceDropDown->Show();
	}
}

void FVoiceSetupDialog::OnInputDeviceDropdownExpanded()
{
	/*if (OutputDeviceDropDown)
	{
		OutputDeviceDropDown->Hide();
	}*/
}

void FVoiceSetupDialog::OnInputDeviceDropdownCollapsed()
{
	/*if (OutputDeviceDropDown)
	{
		OutputDeviceDropDown->Show();
	}*/
}