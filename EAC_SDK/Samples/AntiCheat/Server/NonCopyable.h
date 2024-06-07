// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

class FNonCopyable
{
public:
	FNonCopyable() = default;

	FNonCopyable(const FNonCopyable&) = delete;
	FNonCopyable& operator=(const FNonCopyable&) = delete;
};
