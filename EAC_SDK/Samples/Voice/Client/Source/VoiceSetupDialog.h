// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Dialog.h"
#include "Font.h"

/**
 * Forward declarations
 */
class FTextLabelWidget;
class FButtonWidget;
class FDropDownList;

/**
 * Voice setup dialog
 */
class FVoiceSetupDialog : public FDialog
{
public:
	/**
	 * Constructor
	 */
	FVoiceSetupDialog(Vector2 InPos,
		Vector2 InSize,
		UILayer InLayer,
		FontPtr InNormalFont,
		FontPtr InSmallFont);

	/**
	 * Destructor
	 */
	virtual ~FVoiceSetupDialog();

	/** IWidget */
	virtual void SetPosition(Vector2 Pos) override;
	virtual void SetSize(Vector2 NewSize) override;
	virtual void OnEscapePressed() override;

	virtual void Create() override;

	/**
	* Receives game event
	*
	* @param Event - Game event to act on
	*/
	void OnGameEvent(const FGameEvent& Event);

private:
	/** Update input devices */
	void UpdateInputDevices(const std::wstring& DefaultDeviceName);

	/** Update output devices */
	void UpdateOutputDevices(const std::wstring& DefaultDeviceName);

	/** A device option from output devices dropdown has been selected */
	void OnOutputDeviceSelected(const std::wstring& Selection);

	/** A device option from input devices dropdown has been selected */
	void OnInputDeviceSelected(const std::wstring& Selection);

	/** Output devices dropdown has been expanded */
	void OnOutputDeviceDropdownExpanded();

	/** Output devices dropdown has been collapsed */
	void OnOutputDeviceDropdownCollapsed();

	/** Input devices dropdown has been expanded */
	void OnInputDeviceDropdownExpanded();

	/** Input devices dropdown has been collapsed */
	void OnInputDeviceDropdownCollapsed();

	/** Background */
	WidgetPtr Background;

	/** Header Label */
	std::shared_ptr<FTextLabelWidget> HeaderLabel;

	/** Output Device Dropdown */
	std::shared_ptr<FDropDownList> OutputDeviceDropDown;

	/** Input Device Dropdown */
	std::shared_ptr<FDropDownList> InputDeviceDropDown;
};
