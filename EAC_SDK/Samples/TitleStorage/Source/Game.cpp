// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "DebugLog.h"
#include "Console.h"
#include "Menu.h"
#include "Level.h"
#include "TitleStorage.h"
#include "GameEvent.h"
#include "Player.h"
#include "Game.h"

FGame::FGame() noexcept(false)
{
	Menu = std::make_unique<FMenu>(Console);
	Level = std::make_unique<FLevel>();
	TitleStorage = std::make_unique<FTitleStorage>();

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
			L" GETFILE FILE_NAME - to download file data from Title Storage;"
			L" TESTFILE FILE_NAME FILE_PATH - to test if data from Title Storage matches contents of file on disk;"
		};
		AppendHelpMessageLines(ExtraHelpMessageLines);

		Console->AddCommand(L"GETFILE", [](const std::vector<std::wstring>& Args)
		{
			if (Args.size() == 1)
			{
				if (FGame::Get().GetTitleStorage() && FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser()))
				{
					FGame::Get().GetTitleStorage()->StartFileDataDownload(Args[0]);
				}
				else
				{
					FGame::Get().GetConsole()->AddLine(L"GETFILE error: login to be able to interact with Title Storage.", Color::Red);
				}
			}
			else
			{
				FGame::Get().GetConsole()->AddLine(L"GETFILE error: file name is required.", Color::Red);
			}
		});

		Console->AddCommand(L"TESTFILE", [](const std::vector<std::wstring>& Args)
		{
			if (Args.size() == 2)
			{
				if (FGame::Get().GetTitleStorage() && FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser()))
				{
					bool NoLocalData = false;
					const std::wstring TSData = FGame::Get().GetTitleStorage()->GetLocalData(Args[0], NoLocalData);
					if (NoLocalData)
					{
						FGame::Get().GetConsole()->AddLine(L"TESTFILE error: no local data for file specified. File is missing in Title Storage or data is not downloaded yet (use download button or GETFILE command).", Color::Red);
					}
					else
					{
						//Load file data into memory
						std::wifstream FileStream(FStringUtils::Narrow(Args[1]), std::ios_base::binary | std::ios_base::in);

						if (FileStream.good())
						{
							std::wstring DataString((std::istreambuf_iterator<wchar_t>(FileStream)),
								std::istreambuf_iterator<wchar_t>());

							if (DataString == TSData)
							{
								FGame::Get().GetConsole()->AddLine(L"TESTFILE: files' data match!", Color::Green);
							}
							else
							{
								FGame::Get().GetConsole()->AddLine(L"TESTFILE: files' data is different!", Color::Red);
							}
						}
						else
						{
							FGame::Get().GetConsole()->AddLine(L"TESTFILE: could not open file specified as second parameter. Please provide full path.", Color::Red);
						}
					}
				}
				else
				{
					FGame::Get().GetConsole()->AddLine(L"TESTFILE error: login to be able to interact with Title Storage.", Color::Red);
				}
			}
			else
			{
				FGame::Get().GetConsole()->AddLine(L"TESTFILE error: data storage file name and full file path are required.", Color::Red);
			}
		});
	}
}

void FGame::Update()
{
	TitleStorage->Update();

	FBaseGame::Update();
}

void FGame::OnGameEvent(const FGameEvent& Event)
{
	FBaseGame::OnGameEvent(Event);
	TitleStorage->OnGameEvent(Event);
}

std::unique_ptr<FTitleStorage> const& FGame::GetTitleStorage()
{
	return TitleStorage;
}