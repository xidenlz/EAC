// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

class FNonCopyable
{
public:
	FNonCopyable() {}

	FNonCopyable(const FNonCopyable&) = delete;
	FNonCopyable& operator=(const FNonCopyable&) = delete;
};
