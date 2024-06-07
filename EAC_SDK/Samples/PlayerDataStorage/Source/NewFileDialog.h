// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Dialog.h"
#include "Font.h"

/**
 * Forward declarations
 */
class FTextLabelWidget;
class FButtonWidget;

/**
 * Exit dialog
 */
class FNewFileDialog : public FDialog
{
public:
	/**
	 * Constructor
	 */
	FNewFileDialog(Vector2 DialogPos,
		Vector2 DialogSize,
		UILayer DialogLayer,
		FontPtr DialogNormalFont,
		FontPtr DialogSmallFont);

	/**
	 * Destructor
	 */
	virtual ~FNewFileDialog() {};

	/** IWidget */
	virtual void SetPosition(Vector2 Pos) override;

	virtual void OnEscapePressed() override;

	std::wstring GetFileName() const;

private:
	/** Dialog Background */
	WidgetPtr Background;

	/** Main Dialog Label */
	std::shared_ptr<FTextLabelWidget> Label;

	/** Text field to input new file name */
	std::shared_ptr<FTextFieldWidget> TextField;

	/** Ok Button */
	std::shared_ptr<FButtonWidget> OkButton;

	/** Cancel Button */
	std::shared_ptr<FButtonWidget> CancelButton;
};
