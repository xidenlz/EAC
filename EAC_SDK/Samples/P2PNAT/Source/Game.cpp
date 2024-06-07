// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "DebugLog.h"
#include "Console.h"
#include "Menu.h"
#include "Level.h"
#include "GameEvent.h"
#include "Game.h"
#include "P2PNAT.h"

FGame::FGame() noexcept(false)
{
	Menu = std::make_unique<FMenu>(Console);
	Level = std::make_unique<FLevel>();
	P2PNATComponent = std::make_unique<FP2PNAT>();

	CreateConsoleCommands();
}

FGame::~FGame()
{
}

void FGame::Update()
{
	P2PNATComponent->Update();

	FBaseGame::Update();
}

void FGame::OnGameEvent(const FGameEvent& Event)
{
	FBaseGame::OnGameEvent(Event);

	P2PNATComponent->OnGameEvent(Event);
}

const std::unique_ptr<FP2PNAT>& FGame::GetP2PNAT()
{
	return P2PNATComponent;
}
