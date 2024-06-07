// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include <eos_sdk.h>

class FEosSdk
{
public:
	FEosSdk() = default;

	FEosSdk(const FEosSdk&) = delete;
	FEosSdk& operator=(const FEosSdk&) = delete;

	EOS_Bool LoadAndInitSdk();
	EOS_Bool Shutdown();

	/** Processes queued requests */
	void Tick();

public:
	/** Handle to EOS SDK Platform */
	EOS_HPlatform PlatformHandle = nullptr;
};
