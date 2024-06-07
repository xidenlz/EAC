// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "DebugLog.h"
#include "Console.h"
#include "Menu.h"
#include "Level.h"
#include "GameEvent.h"
#include "Platform.h"
#include "Main.h"
#include "Game.h"
#include "Lobbies.h"

const double MaxTimeToShutdown = 7.0; //7 seconds


FGame::FGame() noexcept(false)
{
	Menu = std::make_unique<FMenu>(Console);
	Level = std::make_unique<FLevel>();
	Lobbies = std::make_unique<FLobbies>();

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
			L" CURRENTLOBBY - to print out current lobby info;",
			L" FINDLOBBY lobby_id - to perform a lobby search;",
			L" FINDLOBBYBYBUCKETID bucket_id - to perform a lobby search by bucketid;",
			L" FINDLOBBYBYLEVEL level - to perform a lobby search by type;",
			L" CREATELOBBY bucketid level maxusers [public] [rtcenable] - to create lobby;"
		};
		AppendHelpMessageLines(ExtraHelpMessageLines);

		Console->AddCommand(L"CURRENTLOBBY", [](const std::vector<std::wstring>&)
		{
			if (FPlatform::IsInitialized())
			{
				if (FGame::Get().GetLobbies())
				{
					if (FGame::Get().GetLobbies()->GetCurrentLobby().IsValid())
					{
						FDebugLog::Log(L"Current lobby id: %ls", FStringUtils::Widen(FGame::Get().GetLobbies()->GetCurrentLobby().Id).c_str());
					}
					else
					{
						FDebugLog::LogError(L"No current lobby.");
					}
				}
				else
				{
					FDebugLog::LogError(L"EOS SDK Lobbies are not initialized!");
				}
			}
			else
			{
				FDebugLog::LogError(L"EOS SDK is not initialized!");
			}
		});
		Console->AddCommand(L"FINDLOBBY", [](const std::vector<std::wstring>& args)
		{
			if (FPlatform::IsInitialized())
			{
				if (FGame::Get().GetLobbies())
				{
					if (args.size() == 1)
					{
						FGame::Get().GetLobbies()->Search(FStringUtils::Narrow(args[0]), 1);
					}
					else
					{
						FDebugLog::LogError(L"Lobby id is required as the only argument.");
					}
				}
				else
				{
					FDebugLog::LogError(L"EOS SDK Lobbies are not initialized!");
				}
			}
			else
			{
				FDebugLog::LogError(L"EOS SDK is not initialized!");
			}
		});
		Console->AddCommand(L"FINDLOBBYBYBUCKETID", [](const std::vector<std::wstring>& args)
		{
			if (FPlatform::IsInitialized())
			{
				if (FGame::Get().GetLobbies())
				{
					if (args.size() == 1)
					{
						FGame::Get().GetLobbies()->SearchLobbyByBucketId(FStringUtils::Narrow(args[0]));
					}
					else
					{
						FDebugLog::LogError(L"BacketId is required as the only argument.");
					}
				}
				else
				{
					FDebugLog::LogError(L"EOS SDK Lobbies are not initialized!");
				}
			}
			else
			{
				FDebugLog::LogError(L"EOS SDK is not initialized!");
			}
		});
		Console->AddCommand(L"FINDLOBBYBYLEVEL", [](const std::vector<std::wstring>& args)
		{
			if (FPlatform::IsInitialized())
			{
				if (FGame::Get().GetLobbies())
				{
					if (args.size() == 1)
					{
						FGame::Get().GetLobbies()->SearchLobbyByLevel(FStringUtils::Narrow(FStringUtils::ToUpper(args[0])));
					}
					else
					{
						FDebugLog::LogError(L"Lobby's type is required as the only argument.");
					}
				}
				else
				{
					FDebugLog::LogError(L"EOS SDK Lobbies are not initialized!");
				}
			}
			else
			{
				FDebugLog::LogError(L"EOS SDK is not initialized!");
			}
		});
		Console->AddCommand(L"CREATELOBBY", [](const std::vector<std::wstring>& args)
		{
			if (FPlatform::IsInitialized())
			{
				if (FGame::Get().GetLobbies())
				{
					if (FGame::Get().GetLobbies()->GetCurrentLobby().IsValid())
					{
						FDebugLog::LogError(L"You already in lobby!");
					}
					else if (args.size() >= 3)
					{
						bool bPublicPermission = false;
						bool bRTCEnable = false;
						if (args.size() > 3)
						{
							for (int i = 3; i < (int)args.size(); i++)
							{
								if (FStringUtils::ToUpper(args[i]) == L"PUBLIC")
								{
									bPublicPermission = true;
								}
								else if (FStringUtils::ToUpper(args[i]) == L"RTCENABLE")
								{
									bRTCEnable = true;
								}
							}
						}
						FLobby Lobby;
						Lobby.MaxNumLobbyMembers = atoi(FStringUtils::Narrow(args[2]).c_str());
						Lobby.Permission = (bPublicPermission ? EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED : EOS_ELobbyPermissionLevel::EOS_LPL_INVITEONLY);
						Lobby.bPresenceEnabled = true;
						Lobby.bRTCRoomEnabled = bRTCEnable;
						Lobby.bAllowInvites = true;
						Lobby.BucketId = FStringUtils::Narrow(args[0]);

						FLobbyAttribute Attribute;
						Attribute.Key = "LEVEL";
						Attribute.AsString = FStringUtils::Narrow(args[1]);
						Attribute.ValueType = FLobbyAttribute::String;
						Attribute.Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC;

						Lobby.Attributes.push_back(Attribute);

						FGame::Get().GetLobbies()->CreateLobby(Lobby);
					}
					else
					{
						FDebugLog::LogError(L"Additional parameters required. Use help command");
					}
				}
				else
				{
					FDebugLog::LogError(L"EOS SDK Lobbies are not initialized!");
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

	Lobbies->Update();
}



void FGame::Create()
{
	FBaseGame::Create();

	Lobbies->SubscribeToLobbyInvites();
	Lobbies->SubscribeToLobbyUpdates();
	Lobbies->SubscribeToLeaveLobbyUI();
}

void FGame::OnGameEvent(const FGameEvent& Event)
{
	FBaseGame::OnGameEvent(Event);
	Lobbies->OnGameEvent(Event);
}

void FGame::OnShutdown()
{
	//Lobbies must be cleared before we destroy SDK platform.
	if (Lobbies)
	{
		Lobbies->OnShutdown();
	}

	FBaseGame::OnShutdown();

	ShutdownTriggeredTimestamp = Main->GetTimer().GetTotalSeconds();
}

bool FGame::IsShutdownDelayed()
{
	if (Lobbies && (Main->GetTimer().GetTotalSeconds() - ShutdownTriggeredTimestamp) < MaxTimeToShutdown)
	{
		return !Lobbies->IsReadyToShutdown();
	}

	return false;
}

const std::unique_ptr<FLobbies>& FGame::GetLobbies()
{
	return Lobbies;
}
