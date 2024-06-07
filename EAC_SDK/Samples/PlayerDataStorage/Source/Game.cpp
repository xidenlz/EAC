// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "DebugLog.h"
#include "Console.h"
#include "Menu.h"
#include "Level.h"
#include "PlayerDataStorage.h"
#include "Player.h"
#include "GameEvent.h"
#include "Game.h"

FGame::FGame() noexcept(false)
{
	Menu = std::make_unique<FMenu>(Console);
	Level = std::make_unique<FLevel>();
	PlayerDataStorage = std::make_unique<FPlayerDataStorage>();

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
			L" ADDFILE FILE_NAME FILE_DATA_PATH (opt) - to create (and upload) empty file for Player Data Storage\n"
			L"                                          or to upload file data from disk (when second parameter is present);",
			L" GETFILE FILE_NAME - to download file data from Player Data Storage;",
			L" DUPFILE SOURCE_FILE_NAME DESTINATION_FILE_NAME - to create a copy of existing file in Player Data Storage;",
			L" TESTFILE FILE_NAME FILE_PATH - to test if data from Player Data Storage matches contents of file on disk;"
		};
		AppendHelpMessageLines(ExtraHelpMessageLines);

		Console->AddCommand(L"ADDFILE", [](const std::vector<std::wstring>& Args)
		{
			if (Args.size() == 1)
			{
				if (FGame::Get().GetPlayerDataStorage() && FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser()))
				{
					FGame::Get().GetPlayerDataStorage()->AddFile(Args[0], L"");
				}
				else
				{
					FGame::Get().GetConsole()->AddLine(L"ADDFILE error: login to be able to interact with Player Data Storage.", Color::Red);
				}
			}
			else if (Args.size() == 2)
			{
				//Simply load the whole file into memory.
				std::wifstream FileStream(FStringUtils::Narrow(Args[1]), std::ios_base::binary | std::ios_base::in);

				if (FileStream.good())
				{
					std::wstring DataString((std::istreambuf_iterator<wchar_t>(FileStream)),
						std::istreambuf_iterator<wchar_t>());

					if (FGame::Get().GetPlayerDataStorage() && FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser()))
					{
						FGame::Get().GetPlayerDataStorage()->AddFile(Args[0], DataString);
					}
					else
					{
						FGame::Get().GetConsole()->AddLine(L"ADDFILE error: login to be able to interact with Player Data Storage.", Color::Red);
					}
				}
				else
				{
					FGame::Get().GetConsole()->AddLine(std::wstring(L"ADDFILE error: could not open file: ") + Args[1], Color::Red);
				}
			}
			else
			{
				FGame::Get().GetConsole()->AddLine(L"ADDFILE error: file name is required. Filepath is an optional second parameter.", Color::Red);
			}
		});

		Console->AddCommand(L"GETFILE", [](const std::vector<std::wstring>& Args)
		{
			if (Args.size() == 1)
			{
				if (FGame::Get().GetPlayerDataStorage() && FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser()))
				{
					FGame::Get().GetPlayerDataStorage()->StartFileDataDownload(Args[0]);
				}
				else
				{
					FGame::Get().GetConsole()->AddLine(L"GETFILE error: login to be able to interact with Player Data Storage.", Color::Red);
				}
			}
			else
			{
				FGame::Get().GetConsole()->AddLine(L"GETFILE error: file name is required.", Color::Red);
			}
		});

		Console->AddCommand(L"DUPFILE", [](const std::vector<std::wstring>& Args)
		{
			if (Args.size() == 2)
			{
				if (FGame::Get().GetPlayerDataStorage() && FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser()))
				{
					FGame::Get().GetPlayerDataStorage()->CopyFile(Args[0], Args[1]);
				}
				else
				{
					FGame::Get().GetConsole()->AddLine(L"DUPFILE error: login to be able to interact with Player Data Storage.", Color::Red);
				}
			}
			else
			{
				FGame::Get().GetConsole()->AddLine(L"DUPFILE error: source file name and destination file name are required.", Color::Red);
			}
		});

		Console->AddCommand(L"TESTFILE", [](const std::vector<std::wstring>& Args)
		{
			if (Args.size() == 2)
			{
				if (FGame::Get().GetPlayerDataStorage() && FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser()))
				{
					bool NoLocalData = false;
					const std::wstring PDSData = FGame::Get().GetPlayerDataStorage()->GetLocalData(Args[0], NoLocalData);
					if (NoLocalData)
					{
						FGame::Get().GetConsole()->AddLine(L"TESTFILE error: no local data for file specified. File is missing in PDS or data is not downloaded yet (use download button or GETFILE command).", Color::Red);
					}
					else
					{
						//Load file data into memory
						std::wifstream FileStream(FStringUtils::Narrow(Args[1]), std::ios_base::binary | std::ios_base::in);

						if (FileStream.good())
						{
							std::wstring DataString((std::istreambuf_iterator<wchar_t>(FileStream)),
								std::istreambuf_iterator<wchar_t>());

							if (DataString == PDSData)
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
					FGame::Get().GetConsole()->AddLine(L"TESTFILE error: login to be able to interact with Player Data Storage.", Color::Red);
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
	PlayerDataStorage->Update();

	FBaseGame::Update();
}

void FGame::OnGameEvent(const FGameEvent& Event)
{
	FBaseGame::OnGameEvent(Event);
	PlayerDataStorage->OnGameEvent(Event);
}

std::unique_ptr<FPlayerDataStorage> const& FGame::GetPlayerDataStorage()
{
	return PlayerDataStorage;
}
