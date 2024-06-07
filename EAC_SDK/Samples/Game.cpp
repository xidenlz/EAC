// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "DebugLog.h"
#include "Game.h"
#include "Menu.h"
#include "Level.h"
#include "Achievements.h"
#include "HTTPClient.h"
#include "Console.h"
#include "Platform.h"

FGame::FGame() noexcept(false)
{
	Menu = std::make_unique<FMenu>(Console);
	Level = std::make_unique<FLevel>();
	Achievements = std::make_unique<FAchievements>();

	CreateConsoleCommands();
}

FGame::~FGame()
{

}

void FGame::Init()
{
	FBaseGame::Init();

	Achievements->Init();
}

void FGame::CreateConsoleCommands()
{
	FBaseGame::CreateConsoleCommands();

	if (Console)
	{
		const std::vector<const wchar_t*> ExtraHelpMessageLines =
		{
			L" GETDEFS - to request retrieval of achievement definitions;",
			L" GETPLAYER - to request retrieval of current user's achievements data;",
			L" UNLOCK - to unlock a named achievement;",
			L" INGEST - to ingest a stat;",
			L" QUERYSTATS - to query stats;",
			L" DEBUGNOTIFY - to toggle the debug notify flag;",
			L" URL - to make an HTTP lookup."
		};

		AppendHelpMessageLines(ExtraHelpMessageLines);

		// Achievements
		Console->AddCommand(L"GETDEFS", [](const std::vector<std::wstring>& args)
		{
			if (FPlatform::IsInitialized())
			{
				if (FGame::Get().GetAchievements())
				{
					FGame::Get().GetAchievements()->QueryDefinitions();
				}
			}
			else
			{
				FDebugLog::LogError(L"EOS SDK is not initialized!");
			}
		});

		Console->AddCommand(L"GETPLAYER", [](const std::vector<std::wstring>& args)
		{
			if (FPlatform::IsInitialized())
			{
				if (FGame::Get().GetAchievements())
				{
					if (args.size() == 1)
					{
						std::string NarrowUserIdStr = FStringUtils::Narrow(args[0]);
						EOS_ProductUserId ProductUserId = FAccountHelpers::ProductUserIDFromString(NarrowUserIdStr.c_str());
						if (ProductUserId != nullptr)
						{
							// Get player's achievements
							FGame::Get().GetAchievements()->QueryPlayerAchievements(ProductUserId);
						}
						else
						{
							FGame::Get().GetConsole()->AddLine(L"GETPLAYER called with an invalid user id");
						}
					}
					else
					{
						// Get local player's achievements
						FGame::Get().GetAchievements()->QueryPlayerAchievements(FProductUserId());
					}
				}
			}
			else
			{
				FDebugLog::LogError(L"EOS SDK is not initialized!");
			}
		});

		Console->AddCommand(L"UNLOCK", [](const std::vector<std::wstring>& args)
		{
			if (FPlatform::IsInitialized())
			{
				if (args.size() == 1)
				{
					if (FGame::Get().GetAchievements())
					{
						std::vector<std::wstring> AchievementIds;
						AchievementIds.emplace_back(args[0]);

						FGame::Get().GetAchievements()->UnlockAchievements(AchievementIds);
					}
				}
				else
				{
					FGame::Get().GetConsole()->AddLine(L"Error: name of achievement is required.");
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
					if (FGame::Get().GetAchievements())
					{
						try
						{
							FGame::Get().GetAchievements()->IngestStat(args[0], std::stoi(args[1]));
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
				else if (args.size() == 3)
				{
					if (FGame::Get().GetAchievements())
					{
						std::string NarrowUserIdStr = FStringUtils::Narrow(args[2]);
						EOS_ProductUserId TargetUserId = FAccountHelpers::ProductUserIDFromString(NarrowUserIdStr.c_str());
						if (TargetUserId != nullptr)
						{
							try
							{
								FGame::Get().GetAchievements()->IngestStat(args[0], std::stoi(args[1]), TargetUserId);
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
						else
						{
							FGame::Get().GetConsole()->AddLine(L"INGEST called with an invalid user id");
						}
					}
				}
				else
				{
					FGame::Get().GetConsole()->AddLine(L"Error: name of stat and ingest amount is required, target user id is optional");
				}
			}
			else
			{
				FDebugLog::LogError(L"EOS SDK is not initialized!");
			}
		});

		Console->AddCommand(L"DEBUGNOTIFY", [](const std::vector<std::wstring>& args)
		{
			if (FPlatform::IsInitialized())
			{
				if (FGame::Get().GetAchievements())
				{
					if (FGame::Get().GetAchievements()->ToggleDebugNotify()) {
						FDebugLog::LogWarning(L"Achievements query disabled to allow debugging of notification.");
					} else {
						FDebugLog::LogWarning(L"Achievements query re-enabled.");
					}
				}
			}
			else
			{
				FDebugLog::LogError(L"EOS SDK is not initialized!");
			}
		});

		Console->AddCommand(L"QUERYSTATS", [](const std::vector<std::wstring>& args)
		{
			if (FPlatform::IsInitialized())
			{
				if (FGame::Get().GetAchievements())
				{
					FGame::Get().GetAchievements()->QueryStats();
				}
			}
			else
			{
				FDebugLog::LogError(L"EOS SDK is not initialized!");
			}
		});

		Console->AddCommand(L"URL", [](const std::vector<std::wstring>& args)
		{
			if (args.size() == 1)
			{
				FHTTPClient::GetInstance().PerformHTTPRequest(FStringUtils::Narrow(args[0]), FHTTPClient::EHttpRequestMethod::GET, std::string(), [](FHTTPClient::HTTPErrorCode ErrorCode, const std::vector<char>& Data)
				{
					if (ErrorCode == 200)
					{
						FGame::Get().GetConsole()->AddLine(L"HTTP Request success: ");
						std::string ResponseString(Data.data(), Data.size());
						FGame::Get().GetConsole()->AddLine(FStringUtils::Widen(ResponseString));
					}
					else
					{
						std::string ErrorString(Data.data(), Data.size());
						FDebugLog::LogError(L"HTTP Request failed: %ls", FStringUtils::Widen(ErrorString).c_str());
					}
				}
				);
			}
		});

	}
}

void FGame::Update()
{
	FBaseGame::Update();

	Achievements->Update();
	FHTTPClient::GetInstance().Update();
}

void FGame::OnGameEvent(const FGameEvent& Event)
{
	FBaseGame::OnGameEvent(Event);

	Achievements->OnGameEvent(Event);
}

void FGame::OnShutdown()
{
	//Achievements must be cleared before we destroy SDK platform.
	if (Achievements)
	{
		Achievements->OnShutdown();
	}

	FBaseGame::OnShutdown();

	FHTTPClient::ClearInstance();
}

const std::unique_ptr<FAchievements>& FGame::GetAchievements()
{
	return Achievements;
}
