// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "DebugLog.h"
#include "StringUtils.h"
#include "Console.h"
#include "Menu.h"
#include "Level.h"
#include "GameEvent.h"
#include "Platform.h"
#include "Main.h"
#include "Game.h"
#include "Voice.h"
#include "HTTPClient.h"

FGame::FGame() noexcept(false)
{
	Menu = std::make_unique<FMenu>(Console);
	Level = std::make_unique<FLevel>();
	Voice = std::make_unique<FVoice>();

	CreateConsoleCommands();
}

FGame::~FGame()
{
	
}

void FGame::Init()
{
	FBaseGame::Init();

	Voice->Init();
}

void FGame::CreateConsoleCommands()
{
	FBaseGame::CreateConsoleCommands();

	if (Console)
	{
		const std::vector<const wchar_t*> ExtraHelpMessageLines =
		{
			L" JOIN ROOM_NAME - to join a room, if ROOM_NAME is not supplied a new room will be created and joined;",
			L" LEAVE - to leave current room;",
			L" KICK USER_ID - to kick a user, will do nothing if you are not the room owner. USER_ID = ProductUserId;",
			L" REMOTEMUTE USER_ID MUTE - to remote mute a user, will do nothing if you are not the room owner.  USER_ID = ProductUserId, MUTE = 1 or 0;"
		};
		AppendHelpMessageLines(ExtraHelpMessageLines);

		Console->AddCommand(L"JOIN", [](const std::vector<std::wstring>& args)
		{
			if (FPlatform::IsInitialized())
			{
				PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
				if (Player)
				{
					if (args.size() == 1)
					{
						// Join room
						FGameEvent Event(EGameEventType::JoinRoom, Player->GetProductUserID(), args[0]);
						FGame::Get().OnGameEvent(Event);
					}
					else
					{
						// Create and join room
						FGameEvent Event(EGameEventType::JoinRoom, Player->GetProductUserID(), std::wstring());
						FGame::Get().OnGameEvent(Event);
					}
				}
			}
			else
			{
				FDebugLog::LogError(L"EOS SDK is not initialized!");
			}
		});

		Console->AddCommand(L"LEAVE", [](const std::vector<std::wstring>& args)
		{
			if (FPlatform::IsInitialized())
			{
				PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
				if (Player)
				{
					FGameEvent Event(EGameEventType::LeaveRoom, Player->GetProductUserID());
					FGame::Get().OnGameEvent(Event);
				}
			}
			else
			{
				FDebugLog::LogError(L"EOS SDK is not initialized!");
			}
		});

		Console->AddCommand(L"KICK", [](const std::vector<std::wstring>& args)
		{
			if (FPlatform::IsInitialized())
			{
				if (args.size() == 1)
				{
					std::string NarrowUserIdStr = FStringUtils::Narrow(args[0]);
					EOS_ProductUserId ProductUserId = FAccountHelpers::ProductUserIDFromString(NarrowUserIdStr.c_str());
					if (ProductUserId != nullptr)
					{
						FGameEvent Event(EGameEventType::Kick, ProductUserId);
						FGame::Get().OnGameEvent(Event);
					}
					else
					{
						FGame::Get().GetConsole()->AddLine(L"Kick command requires a valid user id");
					}
				}
				else
				{
					FGame::Get().GetConsole()->AddLine(L"Kick command requires a room name and user id parameter");
				}
			}
			else
			{
				FDebugLog::LogError(L"EOS SDK is not initialized!");
			}
		});

		Console->AddCommand(L"REMOTEMUTE", [](const std::vector<std::wstring>& args)
		{
			if (FPlatform::IsInitialized())
			{
				if (args.size() == 2)
				{
					std::string NarrowUserIdStr = FStringUtils::Narrow(args[0]);
					EOS_ProductUserId ProductUserId = FAccountHelpers::ProductUserIDFromString(NarrowUserIdStr.c_str());
					if (ProductUserId != nullptr)
					{
						try
						{
							FGameEvent Event(EGameEventType::RemoteMute, std::stoi(args[1]), ProductUserId);
							FGame::Get().OnGameEvent(Event);
						}
						catch (const std::invalid_argument&)
						{
							FGame::Get().GetConsole()->AddLine(L"Error: Can't remote mute - invalid argument");
						}
						catch (const std::out_of_range&)
						{
							FGame::Get().GetConsole()->AddLine(L"Error: Can't remote mute - out of range.");
						}
						catch (const std::exception&)
						{
							FGame::Get().GetConsole()->AddLine(L"Error: Can't remote mute, undefined error.");
						}
					}
					else
					{
						FGame::Get().GetConsole()->AddLine(L"Remote mute command requires a valid user id");
					}
				}
				else
				{
					FGame::Get().GetConsole()->AddLine(L"Remote mute command requires a user id parameter");
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

	Voice->Update();
	FHTTPClient::GetInstance().Update();
}

void FGame::OnGameEvent(const FGameEvent& Event)
{
	Voice->OnGameEvent(Event);

	FBaseGame::OnGameEvent(Event);
}

void FGame::OnShutdown()
{
	if (Voice)
	{
		Voice->OnShutdown();
	}

	FBaseGame::OnShutdown();

	FHTTPClient::ClearInstance();
}

const std::unique_ptr<FVoice>& FGame::GetVoice()
{
	return Voice;
}
