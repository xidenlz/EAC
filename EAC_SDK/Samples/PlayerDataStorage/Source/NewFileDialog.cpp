// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "Game.h"
#include "Menu.h"
#include "Sprite.h"
#include "TextLabel.h"
#include "Button.h"
#include "TextField.h"
#include "NewFileDialog.h"
#include "PlayerDataStorage.h"

FNewFileDialog::FNewFileDialog(Vector2 DialogPos,
						 Vector2 DialogSize,
						 UILayer DialogLayer,
						 FontPtr DialogNormalFont,
						 FontPtr DialogSmallFont) :
	FDialog(DialogPos, DialogSize, DialogLayer)
{
	Background = std::make_shared<FSpriteWidget>(
		Position,
		Size,
		DialogLayer - 1,
		L"Assets/textfield.dds");
	AddWidget(Background);

	Label = std::make_shared<FTextLabelWidget>(
		Position,
		Vector2(150.f, 30.f),
		DialogLayer - 1,
		L"New file name:",
		L"");
	Label->SetFont(DialogNormalFont);
	AddWidget(Label);

	TextField = std::make_shared<FTextFieldWidget>(
		Vector2(Position + Vector2(0.0f, 30.0f)),
		Vector2(150.f, 30.0f),
		DialogLayer - 1,
		L"new_file",
		L"Assets/textfield.dds",
		DialogNormalFont,
		FTextFieldWidget::EInputType::Normal,
		EAlignmentType::Center
		);
	AddWidget(TextField);

	FColor ButtonCol = FColor(0.f, 0.47f, 0.95f, 1.f);

	OkButton = std::make_shared<FButtonWidget>(
		Vector2(Position.x + 30.f, Position.y + 70.f),
		Vector2(100.f, 30.f),
		DialogLayer - 1,
		L"OK",
		assets::DefaultButtonAssets,
		DialogSmallFont,
		ButtonCol);
	OkButton->SetOnPressedCallback([this]()
	{
		FGame::Get().GetPlayerDataStorage()->AddFile(this->GetFileName(), L"");
		Hide();
	});
	OkButton->SetBackgroundColors(assets::DefaultButtonColors);
	AddWidget(OkButton);

	CancelButton = std::make_shared<FButtonWidget>(
		Vector2(Position.x + 150.f, Position.y + 70.f),
		Vector2(100.f, 30.f),
		DialogLayer - 1,
		L"CANCEL",
		assets::DefaultButtonAssets,
		DialogSmallFont,
		ButtonCol);
	CancelButton->SetOnPressedCallback([this]()
	{
		Hide();
	});
	CancelButton->SetBackgroundColors(assets::DefaultButtonColors);
	AddWidget(CancelButton);
}

void FNewFileDialog::SetPosition(Vector2 Pos)
{
	IWidget::SetPosition(Pos);

	Background->SetPosition(Position);
	Label->SetPosition(Vector2(Position.x + 30.f, Position.y + 40.f));
	TextField->SetPosition(Vector2(Position.x + 30.f, Position.y + 70.f));
	OkButton->SetPosition(Vector2(Position.x + 50.f, Position.y + 100.f));
	CancelButton->SetPosition(Vector2(Position.x + 170.f, Position.y + 100.f));
}


void FNewFileDialog::OnEscapePressed()
{
	Hide();
}

std::wstring FNewFileDialog::GetFileName() const
{
	if (TextField)
	{
		return TextField->GetText();
	}

	return std::wstring();
}
