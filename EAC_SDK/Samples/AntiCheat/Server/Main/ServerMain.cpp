// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"

#include "EosSdk.h"

#include "Main.h"

#ifdef __APPLE__
#include "MacMain.h"
#endif


#include "DebugLog.h"
#include "StringUtils.h"
#include "CommandLine.h"
#include "Settings.h"
#include "SampleConstants.h"
#include "AntiCheatNetworkTransport.h"
#include "AntiCheatServer.h"

using namespace std;

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

	FDebugLog::Log(L"EOS AntiCheat Server Sample");

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
	FEosSdkPtr EosSdk = std::make_shared<FEosSdk>();
	if (!EosSdk->LoadAndInitSdk())
	{
		FDebugLog::LogError(L"Unable to initialize sdk!");
		return 1;
	}

	FDebugLog::Log(L"Listening on port %d", Port);
	FDebugLog::Log(L"(ctrl - c) to exit");

	FAntiCheatServer Server;

	Server.Init(EosSdk->PlatformHandle);

	Server.BeginSession();

	FAntiCheatNetworkTransport::GetInstance().SetOnNewClientCallback([&Server](void* ClientHandle, FAntiCheatNetworkTransport::FRegistrationInfoMessage Message)
	{
		struct VerifyCallbackHelper
		{
			FAntiCheatServer* Server = nullptr;
			void* ClientHandle = nullptr;
			FAntiCheatNetworkTransport::FRegistrationInfoMessage Message;
		};
		auto VerifyCallback = [](const EOS_Connect_VerifyIdTokenCallbackInfo* Data)
		{
			// Called when the Connect ID Token validation operation is complete.
			bool bIsValidationOk = false;
			VerifyCallbackHelper* Helper = reinterpret_cast<VerifyCallbackHelper*>(Data->ClientData);
			if (Data->ResultCode == EOS_EResult::EOS_Success)
			{
				if (Data->bIsAccountInfoPresent)
				{
					FDebugLog::Log(L"EOS Connect ID Token DeviceType: %ls", FStringUtils::Widen(Data->DeviceType).c_str());

					bool bIsPlatformOk = true;
					switch (Helper->Message.ClientPlatform)
					{
						// The anti-cheat client does not support consoles or mobile devices, so we must handle
						// the anti-cheat ClientPlatform and related ClientType with care to prevent exploits.

						// If the user registration message claims to be on console, verify that an actual console device is being used.
						// See platform types here https://dev.epicgames.com/docs/game-services/eos-connect-interface#user-verification-using-an-id-token
						case EOS_EAntiCheatCommonClientPlatform::EOS_ACCCP_Nintendo:
							if (strcmp(Data->DeviceType, "Switch") != 0)
							{
								FDebugLog::LogError(L"Player claims to be on Nintendo but EOS Connect ID Token DeviceType doesn't match");
								bIsPlatformOk = false;
							}
							break;
						case EOS_EAntiCheatCommonClientPlatform::EOS_ACCCP_PlayStation:
							if (strcmp(Data->DeviceType, "PSVITA") != 0 && strcmp(Data->Platform, "PS3") != 0 && strcmp(Data->Platform, "PS4") != 0 && strcmp(Data->Platform, "PS5") != 0)
							{
								FDebugLog::LogError(L"Player claims to be on Playstation but EOS Connect ID Token DeviceType doesn't match");
								bIsPlatformOk = false;
							}
							break;
						case EOS_EAntiCheatCommonClientPlatform::EOS_ACCCP_Xbox:
							if (strcmp(Data->DeviceType, "Xbox360") != 0 && strcmp(Data->Platform, "XboxOne") != 0)
							{
								FDebugLog::LogError(L"Player claims to be on Xbox but EOS Connect ID Token DeviceType doesn't match");
								bIsPlatformOk = false;
							}
							break;

						// These platforms do not require verification because they have anti-cheat client support and we will 
						// require it by setting the anti-cheat ClientType to EOS_ACCCT_ProtectedClient in RegisterClient.
						case EOS_EAntiCheatCommonClientPlatform::EOS_ACCCP_Windows:
						case EOS_EAntiCheatCommonClientPlatform::EOS_ACCCP_Mac:
						case EOS_EAntiCheatCommonClientPlatform::EOS_ACCCP_Linux:
							break;
							
						// If the user registration message claims to be a mobile device and your game supports mobile clients,
						// you must must implement your own check to verify that players are really using the claimed device type.
						case EOS_EAntiCheatCommonClientPlatform::EOS_ACCCP_Android:
						case EOS_EAntiCheatCommonClientPlatform::EOS_ACCCP_iOS:
							// Your own platform check could go here. This sample does not support mobile clients.
							FDebugLog::LogError(L"Player claims to be on mobile but this sample does not support mobile clients");
							bIsPlatformOk = false;
							break;

						case EOS_EAntiCheatCommonClientPlatform::EOS_ACCCP_Unknown:
						default:
							FDebugLog::LogError(L"Player claims to be on an unknown platform");
							bIsPlatformOk = false;
							break;
					}

					// On success, continue to anti-cheat registration.
					if (bIsPlatformOk)
					{
						bIsValidationOk = true;
						Helper->Server->RegisterClient(Helper->ClientHandle, Helper->Message);
					}
				}
			}

			if (!bIsValidationOk)
			{
				// On failure, the player should not be allowed to continue connecting to the server.
				FDebugLog::LogError(L"EOS Connect ID Token verification failed, player will be removed");
				FAntiCheatNetworkTransport::GetInstance().CloseClientConnection(Helper->ClientHandle);
			}
			delete Helper;
		};

		// We must verify the the player's identity using the provided Connect ID Token before we allow them to continue connecting.
		Server.VerifyIdToken(EOS_ProductUserId_FromString(Message.ProductUserId), Message.EOSConnectIdTokenJWT, new VerifyCallbackHelper{&Server, ClientHandle, Message}, VerifyCallback);
	});
	FAntiCheatNetworkTransport::GetInstance().SetOnClientDisconnectedCallback([&Server](void* ClientHandle) 
	{
		// is active
		Server.UnregisterClient(ClientHandle);
	});
	FAntiCheatNetworkTransport::GetInstance().SetOnNewMessageCallback([&Server](void* ClientHandle, const void* Data, uint32_t Length)
	{
		Server.OnMessageFromClientReceived(ClientHandle, Data, Length);
	});

	// main loop
	while (bIsRunning)
	{
		FAntiCheatNetworkTransport::GetInstance().Update();

		// update the sdk on the mainthread
		EosSdk->Tick();
		
		// simulate other server activity
		std::this_thread::sleep_for(std::chrono::milliseconds(30));
	}

	Server.EndSession();

	// then shutdown the sdk
	EosSdk->Shutdown();

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
