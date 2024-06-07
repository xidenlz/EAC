// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"

#include "VoiceApi.h"
#include "VoiceHost.h"
#include "VoiceSdk.h"

#include "Main.h"

#ifdef __APPLE__
#include "MacMain.h"
#endif

#include "DebugLog.h"
#include "StringUtils.h"
#include "CommandLine.h"
#include "Settings.h"
#include "SampleConstants.h"

using namespace std;

constexpr uint16 SampleConstants::ServerPort;

bool bIsRunning = true;

#ifdef _WIN32
BOOL WINAPI ConsoleHandler(DWORD CtrlType)
{
	static const WCHAR* EventNames[7] = {
		L"CTRL_C_EVENT",
		L"CTRL_BREAK_EVENT",
		L"CTRL_CLOSE_EVENT",
		L"", // Reserved
		L"", // Reserved
		L"CTRL_LOGOFF_EVENT",
		L"CTRL_SHUTDOWN_EVENT"
	};

	switch (CtrlType)
	{
		case CTRL_C_EVENT:
		case CTRL_BREAK_EVENT:
		case CTRL_CLOSE_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT:
			FDebugLog::Log(L"Exiting due to %ls...", EventNames[CtrlType]);
			bIsRunning = false;		
			return TRUE;

		default:
			return FALSE;
	}
}
#endif // _WIN32

int MasterMain(int Argc, const char* Args[])
{
	std::wstring CommandLine;
	for (int i = 0; i < Argc; ++i)
	{
		CommandLine += (FStringUtils::Widen(Args[i]) + L" ");
	}

	FCommandLine::Get().Init(const_cast<LPWSTR>(CommandLine.c_str()));
	FSettings::Get().Init();

	Main = std::make_unique<FMain>();
	Main->InitCommandLine();

	FDebugLog::Log(L"EOS Voice Server Sample");

	bIsRunning = true;

#ifdef _WIN32
	if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE))
	{
		FDebugLog::LogError(L"Could not set control handler!");
		return 1;
	}
#endif // _WIN32

	// parse listen port
	uint16 Port = SampleConstants::ServerPort;
	if (FCommandLine::Get().HasParam(CommandLineConstants::ServerPort))
	{
		try
		{
			Port = static_cast<uint16>(std::stoi(FCommandLine::Get().GetParamValue(CommandLineConstants::ServerPort)));
		}
		catch (const std::invalid_argument&)
		{
			FDebugLog::LogError(L"Error: Can't parse port - invalid argument");
		}
		catch (const std::out_of_range&)
		{
			FDebugLog::LogError(L"Error: Can't parse port - out of range.");
		}
		catch (const std::exception&)
		{
			FDebugLog::LogError(L"Error: Can't parse port, undefined error.");
		}
	}

	// create and load sdk
	FVoiceSdkPtr EosVoiceSdk = FVoiceSdkPtr(new FVoiceSdk());
	if (!EosVoiceSdk->LoadAndInitSdk())
	{
		FDebugLog::LogError(L"Unable to initialize sdk!");
		return 1;
	}

	// start voice host on its own thread
	FVoiceHostPtr VoiceHost = FVoiceHostPtr(new FVoiceHost());
	FVoiceApi Api(VoiceHost, EosVoiceSdk);
	if (Api.Listen(Port) == false)
	{
		FDebugLog::LogError(L"Unable to listen on port %d", Port);
		FDebugLog::Log(L"Exiting...");
		return 1;
	}

	FDebugLog::Log(L"Listening on port %d", Port);
	FDebugLog::Log(L"(ctrl - c) to exit");

	// main loop
	while (bIsRunning)
	{
		// update the sdk on the mainthread
		EosVoiceSdk->Tick();

		// remove any sessions that haven't received a heartbeat within the given timeout
		size_t NumRemoved = VoiceHost->RemoveExpiredSessions();
		if (NumRemoved > 0)
		{
			FDebugLog::Log(L"Removed %d expired sessions", NumRemoved);
		}
		
		// simulate other server activity
		std::this_thread::sleep_for(std::chrono::milliseconds(30));
	}

	// stop accepting requests
	Api.Stop();

	// then shutdown the sdk
	EosVoiceSdk->Shutdown();

	return 0;
}

int main(int argc, const char *argv[])
{

#if __APPLE__
	int returnValue = MainDriverMac(argc, argv, &MasterMain);
#else
	int returnValue = MasterMain(argc, argv);
#endif
	
	return returnValue;
}
