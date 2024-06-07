// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "DebugLog.h"
#include "Console.h"
#include "Menu.h"
#include "Level.h"
#include "Leaderboard.h"
#include "GameEvent.h"
#include "Platform.h"
#include "Game.h"

FGame::FGame() noexcept(false)
{
	Menu = std::make_unique<FMenu>(Console);
	Level = std::make_unique<FLevel>();
	Leaderboard = std::make_unique<FLeaderboard>();

	CreateConsoleCommands();
}

FGame::~FGame()
{

}

void FGame::Init()
{
	FBaseGame::Init();

	Leaderboard->Init();
}

void FGame::CreateConsoleCommands()
{
	FBaseGame::CreateConsoleCommands();

	if (Console)
	{
		const std::vector<const wchar_t*> ExtraHelpMessageLines =
		{
			L" GETDEFS - to request retrieval of leaderboard definitions;",
			L" INGEST - to ingest a stat for current logged in user;"
		};

		AppendHelpMessageLines(ExtraHelpMessageLines);

		// Leaderboards
		Console->AddCommand(L"GETDEFS", [](const std::vector<std::wstring>& args)
		{
			if (FPlatform::IsInitialized())
			{
				if (FGame::Get().GetLeaderboard())
				{
					FGame::Get().GetLeaderboard()->QueryDefinitions();
				}
			}
			else
			{
				FDebugLog::LogError(L"EOS SDK is not initialized!");
			}
		});

		Console->AddCommand(L"ADDDEBUGDEFS", [](const std::vector<std::wstring>& args)
		{
			if (FPlatform::IsInitialized())
			{
				if (FGame::Get().GetLeaderboard())
				{
					std::vector<std::shared_ptr<FLeaderboardsDefinitionData>> TestData;

					FLeaderboardsDefinitionData DefData;
					DefData.LeaderboardId = L"First";
					DefData.Aggregation = ELeaderboardAggregation::Max;
					DefData.StatName = L"Game Score";
					DefData.StartTime = 0;
					DefData.EndTime = 0;
					TestData.emplace_back(std::shared_ptr<FLeaderboardsDefinitionData>(new FLeaderboardsDefinitionData(DefData)));

					DefData.LeaderboardId = L"Second";
					TestData.emplace_back(std::shared_ptr<FLeaderboardsDefinitionData>(new FLeaderboardsDefinitionData(DefData)));

					DefData.LeaderboardId = L"Third";
					TestData.emplace_back(std::shared_ptr<FLeaderboardsDefinitionData>(new FLeaderboardsDefinitionData(DefData)));

					// Use the follow for testing scroller with a larger number of defs
					/*for (int i = 4; i < 20; ++i)
					{
						DefData.LeaderboardId = L"Definition" + std::to_wstring(i + 1);
						TestData.emplace_back(std::shared_ptr<FLeaderboardsDefinitionData>(new FLeaderboardsDefinitionData(DefData)));
					}*/

					//Existing data will be replaced
					FGame::Get().GetLeaderboard()->GetCachedDefinitions().swap(TestData);
				}
			}
			else
			{
				FDebugLog::LogError(L"EOS SDK is not initialized!");
			}
		});

		Console->AddCommand(L"INGEST", [](const std::vector<std::wstring>& args)
		{
			if (FPlatform::IsInitialized())
			{
				if (args.size() == 2)
				{
					if (FGame::Get().GetLeaderboard())
					{
						try
						{
							FGame::Get().GetLeaderboard()->IngestStat(args[0], std::stoi(args[1]));
						}
						catch (const std::invalid_argument&)
						{
							FGame::Get().GetConsole()->AddLine(L"Error: Can't ingest - invalid argument");
						}
						catch (const std::out_of_range&)
						{
							FGame::Get().GetConsole()->AddLine(L"Error: Can't ingest - out of range.");
						}
						catch (const std::exception&)
						{
							FGame::Get().GetConsole()->AddLine(L"Error: Can't ingest, undefined error.");
						}
					}
				}
				else
				{
					FGame::Get().GetConsole()->AddLine(L"Error: name of stat and ingest amount is required.");
				}
			}
			else
			{
				FDebugLog::LogError(L"EOS SDK is not initialized!");
			}
		});

	}
}

void FGame::Update()
{
	FBaseGame::Update();

	Leaderboard->Update();
}

void FGame::OnGameEvent(const FGameEvent& Event)
{
	FBaseGame::OnGameEvent(Event);

	Leaderboard->OnGameEvent(Event);
}

const std::unique_ptr<FLeaderboard>& FGame::GetLeaderboard()
{
	return Leaderboard;
}