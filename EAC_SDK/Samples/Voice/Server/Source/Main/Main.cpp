// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"

#include "Main.h"

#include "DebugLog.h"
#include "StringUtils.h"
#include "CommandLine.h"
#include "Settings.h"

#define CONSOLE_COL_WHITE "\033[1m\033[37m"		// White
#define CONSOLE_COL_RED "\033[1m\033[31m"		// Red
#define CONSOLE_COL_YELLOW "\033[1m\033[33m"	// Yellow
#define CONSOLE_COL_RESET "\033[0m"				// Reset

std::unique_ptr<FMain> Main;

FMain::FMain() noexcept(false)
{
	FDebugLog::Init();
#ifdef _WIN32
	FDebugLog::AddTarget(FDebugLog::ELogTarget::DebugOutput);
#endif // _WIN32
	FDebugLog::AddTarget(FDebugLog::ELogTarget::Console);
	FDebugLog::AddTarget(FDebugLog::ELogTarget::File);
}

FMain::~FMain()
{
	FDebugLog::Close();
}

void FMain::InitCommandLine()
{

}

void FMain::PrintToConsole(const std::wstring& Message)
{
	const std::string Msg = CONSOLE_COL_WHITE + FStringUtils::Narrow(Message) + CONSOLE_COL_RESET;
	puts(Msg.c_str());
}

void FMain::PrintWarningToConsole(const std::wstring& Message)
{
	const std::string Msg = CONSOLE_COL_YELLOW + FStringUtils::Narrow(Message) + CONSOLE_COL_RESET;
	puts(Msg.c_str());
}

void FMain::PrintErrorToConsole(const std::wstring& Message)
{
	const std::string Msg = CONSOLE_COL_RED + FStringUtils::Narrow(Message) + CONSOLE_COL_RESET;
	puts(Msg.c_str());
}