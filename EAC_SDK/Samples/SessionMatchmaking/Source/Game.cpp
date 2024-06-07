// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "DebugLog.h"
#include "Console.h"
#include "Menu.h"
#include "Level.h"
#include "GameEvent.h"
#include "Player.h"
#include "Main.h"
#include "Game.h"
#include "SessionMatchmaking.h"
#include "Platform.h"

const double MaxTimeToShutdown = 7.0; //7 seconds

FGame::FGame() noexcept(false)
{
	Menu = std::make_unique<FMenu>(Console);
	Level = std::make_unique<FLevel>();
	SessionMatchmaking = std::make_unique<FSessionMatchmaking>();

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
			L" REGISTERPLAYER - register a player with a given product id with the presence session;",
			L" UNREGISTERPLAYER - unregister a player with a given product id with the presence session;"
		};
		AppendHelpMessageLines(ExtraHelpMessageLines);

		Console->AddCommand(L"REGISTERPLAYER", [](const std::vector<std::wstring>& args)
		{
			if (FPlatform::IsInitialized())
			{
				if (FGame::Get().GetSessions())
				{
					if (const FSession* PresenceSession = FGame::Get().GetSessions()->GetPresenceSession())
					{
						if (args.size() == 1)
						{
							std::string NarrowUserIdStr = FStringUtils::Narrow(args[0]);
							EOS_ProductUserId ProductUserId = FAccountHelpers::ProductUserIDFromString(NarrowUserIdStr.c_str());
							if (ProductUserId != nullptr)
							{
								FGame::Get().GetSessions()->Register(PresenceSession->Name, ProductUserId);
							}
							else
							{
								FGame::Get().GetConsole()->AddLine(L"Register command requires a valid product user id");
							}

						}
						else
						{
							FDebugLog::LogError(L"Player product ID is required");
						}
					}
					else
					{
						FDebugLog::LogError(L"No presence session exists");
					}
				}
				else
				{
					FDebugLog::LogError(L"EOS SDK Sessions are not initialized!");
				}
			}
			else
			{
				FDebugLog::LogError(L"EOS SDK is not initialized!");
			}
		});
		Console->AddCommand(L"UNREGISTERPLAYER", [](const std::vector<std::wstring>& args)
		{
			if (FPlatform::IsInitialized())
			{
				if (FGame::Get().GetSessions())
				{
					if (const FSession* PresenceSession = FGame::Get().GetSessions()->GetPresenceSession())
					{
						if (args.size() == 1)
						{
							std::string NarrowUserIdStr = FStringUtils::Narrow(args[0]);
							EOS_ProductUserId ProductUserId = FAccountHelpers::ProductUserIDFromString(NarrowUserIdStr.c_str());
							if (ProductUserId != nullptr)
							{
								FGame::Get().GetSessions()->Unregister(PresenceSession->Name, ProductUserId);
							}
							else
							{
								FGame::Get().GetConsole()->AddLine(L"Unregister command requires a valid product user id");
							}

						}
						else
						{
							FDebugLog::LogError(L"Player product ID is required");
						}
					}
					else
					{
						FDebugLog::LogError(L"No presence session exists");
					}
				}
				else
				{
					FDebugLog::LogError(L"EOS SDK Sessions are not initialized!");
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
	SessionMatchmaking->Update();
	FBaseGame::Update();
}

void FGame::Create()
{
	FBaseGame::Create();

	SessionMatchmaking->SubscribeToGameInvites();
	SessionMatchmaking->SubscribeToLeaveSessionUI();
}

void FGame::OnGameEvent(const FGameEvent& Event)
{
	FBaseGame::OnGameEvent(Event);

	SessionMatchmaking->OnGameEvent(Event);
}

void FGame::OnShutdown()
{
	//SessionMatchmaking must be cleared before we destroy SDK platform.
	if (SessionMatchmaking)
	{
		SessionMatchmaking->OnShutdown();
	}

	FBaseGame::OnShutdown();

	ShutdownTriggeredTimestamp = Main->GetTimer().GetTotalSeconds();
}

bool FGame::IsShutdownDelayed()
{
	if (SessionMatchmaking && (Main->GetTimer().GetTotalSeconds() - ShutdownTriggeredTimestamp) < MaxTimeToShutdown)
	{
		return SessionMatchmaking->HasActiveLocalSessions();
	}

	return false;
}

const std::unique_ptr<FSessionMatchmaking>& FGame::GetSessions()
{
	return SessionMatchmaking;
}
