// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"

#include "EosSdk.h"

#include "SampleConstants.h"

#include "DebugLog.h"
#include "Utils.h"
#include "StringUtils.h"
#include "CommandLine.h"

#if ALLOW_RESERVED_OPTIONS
#include "ReservedPlatformOptions.h"
#endif

#ifdef _WIN32
#include "Windows/eos_Windows.h"
#endif

#include <eos_logging.h>

constexpr char SampleConstants::ProductId[];
constexpr char SampleConstants::SandboxId[];
constexpr char SampleConstants::DeploymentId[];
constexpr char SampleConstants::ClientCredentialsId[];
constexpr char SampleConstants::ClientCredentialsSecret[];
constexpr char SampleConstants::EncryptionKey[];
constexpr char SampleConstants::GameName[];

EOS_Bool FEosSdk::LoadAndInitSdk()
{
	FDebugLog::Log(L"[EOS SDK] Initializing ...");

	// Init EOS SDK
	EOS_InitializeOptions SDKOptions = {};
	SDKOptions.ApiVersion = EOS_INITIALIZE_API_LATEST;
	SDKOptions.AllocateMemoryFunction = nullptr;
	SDKOptions.ReallocateMemoryFunction = nullptr;
	SDKOptions.ReleaseMemoryFunction = nullptr;
	SDKOptions.ProductName = SampleConstants::GameName;
	SDKOptions.ProductVersion = "1.0";
	SDKOptions.Reserved = nullptr;
	SDKOptions.SystemInitializeOptions = nullptr;
	SDKOptions.OverrideThreadAffinity = nullptr;

	EOS_EResult InitResult = EOS_Initialize(&SDKOptions);
	if (InitResult != EOS_EResult::EOS_Success)
	{
		FDebugLog::Log(L"[EOS SDK] Init Failed!");
		return EOS_FALSE;
	}

	FDebugLog::Log(L"[EOS SDK] Initialized. Setting Logging Callback ...");

	EOS_EResult SetLogCallbackResult = EOS_Logging_SetCallback([](const EOS_LogMessage* InMsg) {
		if (InMsg != nullptr && InMsg->Level != EOS_ELogLevel::EOS_LOG_Off)
		{
			if (InMsg->Level == EOS_ELogLevel::EOS_LOG_Error || InMsg->Level == EOS_ELogLevel::EOS_LOG_Fatal)
			{
				FDebugLog::Log(L"[EOS SDK] ERROR %ls: %ls", FStringUtils::Widen(InMsg->Category).c_str(), FStringUtils::Widen(InMsg->Message).c_str());
			}
			else if (InMsg->Level == EOS_ELogLevel::EOS_LOG_Warning)
			{
				FDebugLog::Log(L"[EOS SDK] WARNING %ls: %ls", FStringUtils::Widen(InMsg->Category).c_str(), FStringUtils::Widen(InMsg->Message).c_str());
			}
			else
			{
				FDebugLog::Log(L"[EOS SDK] %ls: %ls", FStringUtils::Widen(InMsg->Category).c_str(), FStringUtils::Widen(InMsg->Message).c_str());
			}
		}
	});

	if (SetLogCallbackResult != EOS_EResult::EOS_Success)
	{
		FDebugLog::Log(L"[EOS SDK] Set Logging Callback Failed!");
	}
	else
	{
		FDebugLog::Log(L"[EOS SDK] Logging Callback Set");
		EOS_Logging_SetLogLevel(EOS_ELogCategory::EOS_LC_ALL_CATEGORIES, EOS_ELogLevel::EOS_LOG_VeryVerbose);
	}

	// Create platform instance
	EOS_Platform_Options PlatformOptions = {};
	PlatformOptions.ApiVersion = EOS_PLATFORM_OPTIONS_API_LATEST;
	PlatformOptions.bIsServer = EOS_TRUE;
	PlatformOptions.EncryptionKey = SampleConstants::EncryptionKey;
	PlatformOptions.OverrideCountryCode = nullptr;
	PlatformOptions.OverrideLocaleCode = nullptr;
	PlatformOptions.Flags = EOS_PF_DISABLE_OVERLAY; // no overlay needed for the server app
	PlatformOptions.CacheDirectory = FUtils::GetTempDirectory();

	bool bHasInvalidParamProductId = false;
	bool bHasInvalidParamSandboxId = false;
	bool bHasInvalidParamDeploymentId = false;
	bool bHasInvalidParamClientCreds = false;

	std::string ProductId = SampleConstants::ProductId;
	std::string SandboxId = SampleConstants::SandboxId;
	std::string DeploymentId = SampleConstants::DeploymentId;

	// Use Command Line vars to populate vars if they exist
	std::wstring CmdProductID = FCommandLine::Get().GetParamValue(CommandLineConstants::ProductId);
	if (!CmdProductID.empty())
	{
		ProductId = FStringUtils::Narrow(CmdProductID);
	}
	bHasInvalidParamProductId = ProductId.empty() ? true : false;

	bHasInvalidParamSandboxId = false;
	std::wstring CmdSandboxID = FCommandLine::Get().GetParamValue(CommandLineConstants::SandboxId);
	if (!CmdSandboxID.empty())
	{
		SandboxId = FStringUtils::Narrow(CmdSandboxID);
	}
	bHasInvalidParamSandboxId = SandboxId.empty() ? true : false;

	std::wstring CmdDeploymentID = FCommandLine::Get().GetParamValue(CommandLineConstants::DeploymentId);
	if (!CmdDeploymentID.empty())
	{
		DeploymentId = FStringUtils::Narrow(CmdDeploymentID);
	}
	bHasInvalidParamDeploymentId = DeploymentId.empty() ? true : false;

	PlatformOptions.ProductId = ProductId.c_str();
	PlatformOptions.SandboxId = SandboxId.c_str();
	PlatformOptions.DeploymentId = DeploymentId.c_str();

	std::string ClientId = SampleConstants::ClientCredentialsId;
	std::string ClientSecret = SampleConstants::ClientCredentialsSecret;

	// Use Command Line vars to populate vars if they exist
	std::wstring CmdClientID = FCommandLine::Get().GetParamValue(CommandLineConstants::ClientId);
	if (!CmdClientID.empty())
	{
		ClientId = FStringUtils::Narrow(CmdClientID);
	}
	std::wstring CmdClientSecret = FCommandLine::Get().GetParamValue(CommandLineConstants::ClientSecret);
	if (!CmdClientSecret.empty())
	{
		ClientSecret = FStringUtils::Narrow(CmdClientSecret);
	}

	bHasInvalidParamClientCreds = false;
	if (!ClientId.empty() && !ClientSecret.empty())
	{
		PlatformOptions.ClientCredentials.ClientId = ClientId.c_str();
		PlatformOptions.ClientCredentials.ClientSecret = ClientSecret.c_str();
	}
	else if (!ClientId.empty() || !ClientSecret.empty())
	{
		bHasInvalidParamClientCreds = true;
	}
	else
	{
		PlatformOptions.ClientCredentials.ClientId = nullptr;
		PlatformOptions.ClientCredentials.ClientSecret = nullptr;
	}

	if (bHasInvalidParamProductId ||
		bHasInvalidParamSandboxId ||
		bHasInvalidParamDeploymentId ||
		bHasInvalidParamClientCreds)
	{
		return false;
	}

#if ALLOW_RESERVED_OPTIONS
	SetReservedPlatformOptions(PlatformOptions);
#else
	PlatformOptions.Reserved = NULL;
#endif // ALLOW_RESERVED_OPTIONS

	PlatformHandle = EOS_Platform_Create(&PlatformOptions);

	if (PlatformHandle == nullptr)
	{
		return false;
	}

	return EOS_TRUE;
}

EOS_Bool FEosSdk::Shutdown()
{
	if (PlatformHandle != nullptr)
	{
		EOS_Platform_Release(PlatformHandle);
		PlatformHandle = nullptr;
	}

	EOS_EResult ShutdownResult = EOS_Shutdown();

	if (ShutdownResult != EOS_EResult::EOS_Success)
	{
		FDebugLog::Log(L"SDK Failed to shutdown Err:%d\n", static_cast<int32_t>(ShutdownResult));
		return EOS_FALSE;
	}

	FDebugLog::Log(L"Attempting to unload the SDK...");

	return EOS_TRUE;
}

void FEosSdk::Tick()
{
	EOS_Platform_Tick(PlatformHandle);
}
