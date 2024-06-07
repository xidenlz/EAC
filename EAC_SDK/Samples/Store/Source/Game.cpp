// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "DebugLog.h"
#include "Console.h"
#include "Menu.h"
#include "Level.h"
#include "Store.h"
#include "GameEvent.h"
#include "Game.h"

FGame::FGame() noexcept(false)
{
	Menu = std::make_unique<FMenu>(Console);
	Level = std::make_unique<FLevel>();
	Store = std::make_unique<FStore>();

	CreateConsoleCommands();
}

FGame::~FGame()
{
}

void FGame::CreateConsoleCommands()
{
	FBaseGame::CreateConsoleCommands();

	if (Console)
	{
		const std::vector<const wchar_t*> ExtraHelpMessageLines =
		{
			L" OFFERS - to refresh the catalog;",
		};
		AppendHelpMessageLines(ExtraHelpMessageLines);

		Console->AddCommand(L"OFFERS", [](const std::vector<std::wstring>&)
		{
			FGameEvent Event(EGameEventType::RefreshOffers);
			FGame::Get().OnGameEvent(Event);
		});

		Console->AddCommand(L"LOCALE", [](const std::vector<std::wstring>& args)
		{
			if (args.size() == 1)
			{
				FGameEvent Event(EGameEventType::SetLocale, args[0]);
				FGame::Get().OnGameEvent(Event);
			}
			else
			{
				FGame::Get().GetConsole()->AddLine(L"error: locale required.");
			}
		});
	}
}

void FGame::Update()
{
	Store->Update();

	FBaseGame::Update();
}

void FGame::OnGameEvent(const FGameEvent& Event)
{
	FBaseGame::OnGameEvent(Event);
	Store->OnGameEvent(Event);
}

std::unique_ptr<FStore> const& FGame::GetStore()
{
	return Store;
}