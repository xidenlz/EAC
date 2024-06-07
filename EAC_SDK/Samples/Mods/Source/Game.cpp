// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "DebugLog.h"
#include "Console.h"
#include "Menu.h"
#include "Level.h"
#include "Mods.h"
#include "GameEvent.h"
#include "Platform.h"
#include "Game.h"

FGame::FGame() noexcept(false)
{
	Menu = std::make_unique<FMenu>(Console);
	Level = std::make_unique<FLevel>();
	Mods = std::make_unique<FMods>();

	CreateConsoleCommands();
}

FGame::~FGame()
{
}

void FGame::Init()
{
	FBaseGame::Init();

	Mods->Init();
}

void FGame::CreateConsoleCommands()
{
	FBaseGame::CreateConsoleCommands();

	if (Console)
	{
		const std::vector<const wchar_t*> ExtraHelpMessageLines =
		{
			L" SETREMOVEAFTEREXIT VALUE - set's default RemoveAfterExit behavior. VALUE is either true or false",
			L" INSTALL NamespaceId ItemId ArtifactId [RemoveAfterExit] - to install Mod (RemoveAfterExit is optional);",
			L" UNINSTALL NamespaceId ItemId ArtifactId - to install Mod;",
			L" UPDATE NamespaceId ItemId ArtifactId - to install Mod;",
		};

		AppendHelpMessageLines(ExtraHelpMessageLines);

		Console->AddCommand(L"SetRemoveAfterExit", [](const std::vector<std::wstring>& args)
		{
			if (FPlatform::IsInitialized())
			{
				if (args.size() == 1)
				{
					if (FGame::Get().GetMods())
					{
						const bool bRemoveAfterExit = args[0] == L"true";
						FGame::Get().GetMods()->SetRemoveAfterExit(bRemoveAfterExit);
					}
				}
				else
				{
					FGame::Get().GetConsole()->AddLine(L"SetRemoveAfterExit error: [trur | false] argument is required.");
				}
			}
			else
			{
				FDebugLog::LogError(L"EOS SDK is not initialized!");
			}
		});
		Console->AddCommand(L"INSTALL", [](const std::vector<std::wstring>& args)
		{
			if (FPlatform::IsInitialized())
			{

				if (args.size() == 3 || args.size() == 4)
				{
					FModId ModId;
					ModId.NamespaceId = FStringUtils::Narrow(args[0]);
					ModId.ItemId = FStringUtils::Narrow(args[1]);
					ModId.ArtifactId = FStringUtils::Narrow(args[2]);

					const bool bRemoveAfterExit = args.size() == 4 && args[3] == L"RemoveAfterExit";

					if (FGame::Get().GetMods())
					{
						FGame::Get().GetMods()->InstallMod(std::move(ModId), bRemoveAfterExit);
					}
				}
				else
				{
					FGame::Get().GetConsole()->AddLine(L"Install error: {NamespaceId, ItemId, ArtifactId} are required.");
				}
			}
			else
			{
				FDebugLog::LogError(L"EOS SDK is not initialized!");
			}
		});
		Console->AddCommand(L"UNINSTALL", [](const std::vector<std::wstring>& args)
		{
			if (FPlatform::IsInitialized())
			{

				if (args.size() == 3)
				{
					FModId ModId;
					ModId.NamespaceId = FStringUtils::Narrow(args[0]);
					ModId.ItemId = FStringUtils::Narrow(args[1]);
					ModId.ArtifactId = FStringUtils::Narrow(args[2]);

					if (FGame::Get().GetMods())
					{
						FGame::Get().GetMods()->UnInstallMod(std::move(ModId));
					}
				}
				else
				{
					FGame::Get().GetConsole()->AddLine(L"UnInstall error: {NamespaceId, ItemId, ArtifactId} are required.");
				}
			}
			else
			{
				FDebugLog::LogError(L"EOS SDK is not initialized!");
			}
		});
		Console->AddCommand(L"UPDATE", [](const std::vector<std::wstring>& args)
		{
			if (FPlatform::IsInitialized())
			{
				if (args.size() == 3)
				{
					FModId ModId;
					ModId.NamespaceId = FStringUtils::Narrow(args[0]);
					ModId.ItemId = FStringUtils::Narrow(args[1]);
					ModId.ArtifactId = FStringUtils::Narrow(args[2]);

					if (FGame::Get().GetMods())
					{
						FGame::Get().GetMods()->UpdateMod(std::move(ModId));
					}
				}
				else
				{
					FGame::Get().GetConsole()->AddLine(L"Update error: {NamespaceId, ItemId, ArtifactId} are required.");
				}
			}
			else
			{
				FDebugLog::LogError(L"EOS SDK is not initialized!");
			}
		});

		Console->AddCommand(L"ADDTESTDATA", [](const std::vector<std::wstring>& args)
		{
			if (FPlatform::IsInitialized())
			{
				if (FGame::Get().GetMods())
				{
					FGame::Get().GetMods()->AddTestData();
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
	Mods->Update();

	FBaseGame::Update();
}

void FGame::OnGameEvent(const FGameEvent& Event)
{
	FBaseGame::OnGameEvent(Event);
	Mods->OnGameEvent(Event);
}

std::unique_ptr<FMods> const& FGame::GetMods()
{
	return Mods;
}