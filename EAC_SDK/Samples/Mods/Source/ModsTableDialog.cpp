// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"

#include "ModsTableDialog.h"

#include "Game.h"
#include "TextLabel.h"
#include "Button.h"
#include "UIEvent.h"
#include "GameEvent.h"
#include "AccountHelpers.h"
#include "Player.h"
#include "Sprite.h"
#include "Mods.h"
#include "TextEditor.h"
#include "TableView.h"

const float HeaderLabelHeight = 20.0f;

FModsTableDialog::FModsTableDialog(
	Vector2 DialogPos,
	Vector2 DialogSize,
	UILayer DialogLayer,
	FontPtr DialogNormalFont,
	FontPtr DialogSmallFont,
	FontPtr DialogTinyFont) :
	FDialog(DialogPos, DialogSize, DialogLayer)
{
	HeaderLabel = std::make_shared<FTextLabelWidget>(
		DialogPos,
		Size,
		Layer - 1,
		std::wstring(L"Mods: "),
		L"Assets/wide_label.dds",
		FColor(1.f, 1.f, 1.f, 1.f),
		FColor(1.f, 1.f, 1.f, 1.f),
		EAlignmentType::Left
		);
	HeaderLabel->SetFont(DialogNormalFont);

	BackgroundImage = std::make_shared<FSpriteWidget>(
		DialogPos,
		DialogSize,
		DialogLayer,
		L"Assets/texteditor.dds");

	Vector2 ModsTablePos = Vector2(0.0f, 0.0f);

	ModsTable = std::make_shared<FModsTableView>(
		ModsTablePos + Vector2(0.0f, HeaderLabelHeight + 25.f),
		Vector2(500, 500),
		DialogLayer - 1,
		L"All mods:",
		10.0f, //scroller width
		std::vector<FModsTableRowData>(),
		FModsTableRowData(),
		35.f/*TableViewEntryHeight*/);

	ModsTable->SetFonts(DialogSmallFont, DialogSmallFont);
}

void FModsTableDialog::Update()
{
	if (ModsTable && FGame::Get().GetMods()->IsDirty())
	{
		ModsTable->ScrollToTop();

		std::vector<FModsTableRowData> InstalledRows, OtherRows;
		const std::map<FModId, FModDetail>& Mods = FGame::Get().GetMods()->GetModDetails();

		std::for_each(Mods.begin(), Mods.end(), [&InstalledRows, &OtherRows](std::pair<FModId, FModDetail> Mod) {
			FModsTableRowData RowData = ModsTableRowDataHelpers::FromModDetail(Mod.second);
			if (RowData.bActionsAvailable[FModsTableRowData::EAction::Install])
			{
				InstalledRows.push_back(ModsTableRowDataHelpers::FromModDetail(Mod.second));
			}
			else
			{
				OtherRows.push_back(ModsTableRowDataHelpers::FromModDetail(Mod.second));
			}
		});

		OtherRows.reserve(InstalledRows.size() + OtherRows.size());
		OtherRows.insert(OtherRows.end(), InstalledRows.begin(), InstalledRows.end());
		ModsTable->RefreshData(std::move(OtherRows));

		FGame::Get().GetMods()->SetIsDirty(false);
	}

	FDialog::Update();
}

void FModsTableDialog::Create()
{
	if (HeaderLabel) HeaderLabel->Create();
	if (BackgroundImage) BackgroundImage->Create();
	if (ModsTable) ModsTable->Create();

	AddWidget(HeaderLabel);
	AddWidget(BackgroundImage);
	AddWidget(ModsTable);
}

void FModsTableDialog::SetPosition(Vector2 Pos)
{
	IWidget::SetPosition(Pos);

	if (HeaderLabel) HeaderLabel->SetPosition(Pos);
	if (BackgroundImage) BackgroundImage->SetPosition(Vector2(Position.x, Position.y + 20.f));
	if (ModsTable) ModsTable->SetPosition(Vector2(Position.x, Position.y + 20.f));
}

void FModsTableDialog::SetSize(Vector2 NewSize)
{
	IWidget::SetSize(NewSize);
}

void FModsTableDialog::SetWindowSize(Vector2 WindowSize)
{
	if (BackgroundImage) BackgroundImage->SetSize(Vector2(WindowSize.x * ConsoleWindowProportion.x, WindowSize.y * 0.455f));
	if (HeaderLabel) HeaderLabel->SetSize(Vector2(WindowSize.x * ConsoleWindowProportion.x, HeaderLabelHeight));
	if (ModsTable) ModsTable->SetSize(Vector2(WindowSize.x * ConsoleWindowProportion.x, WindowSize.y * 0.455f));
}

void FModsTableDialog::OnGameEvent(const FGameEvent& Event)
{
}
