// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include <memory>
#include <mutex>
#include <vector>
#include <algorithm>
#include <string>
#include <map>
#include <fstream>
#include <cassert>
#include <future>
#include <iterator>
#include <queue>
#include <unordered_set>
#include <chrono>
#include <random>

#include <stdarg.h>

#ifdef _WIN32
#include <WinSDKVer.h>
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <SDKDDKVer.h>

// Use the C++ standard templated min/max
#define NOMINMAX

// DirectX apps don't need GDI
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP

// Include <mcx.h> if you need this
#define NOMCX

// Include <winsvc.h> if you need this
#define NOSERVICE

// WinHelp is deprecated
#define NOHELP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincodec.h>
#include <shellapi.h>
#endif //_WIN32

// Linux does not have _DEBUG in debug builds
#if !defined(NDEBUG) && !defined(_DEBUG)
#define _DEBUG
#endif

#if ALLOW_RESERVED_OPTIONS
#define DEV_BUILD
#endif

#ifndef _WIN32
typedef wchar_t WCHAR;
typedef wchar_t* LPWSTR, *PWSTR;
#define sprintf_s snprintf
#define __cdecl
#define wsprintf(BUF, FMT, ...) swprintf(BUF, sizeof(BUF)/sizeof(wchar_t), FMT, __VA_ARGS__)
#define OutputDebugStringW(STRING) wprintf(STRING)
#endif // !_WIN32

using uint16 = unsigned short;

class FVoiceUser;
using FVoiceUserPtr = std::shared_ptr<FVoiceUser>;

class FVoiceSession;
using FVoiceSessionPtr = std::shared_ptr<FVoiceSession>;

class FVoiceHost;
using FVoiceHostPtr = std::shared_ptr<FVoiceHost>;

class FVoiceSdk;
using FVoiceSdkPtr = std::shared_ptr<FVoiceSdk>;

using FScopedLock = std::lock_guard<std::mutex>;

using ServerTimePoint = std::chrono::time_point<std::chrono::steady_clock>;

// remove elements matching predicate, returns number of removed elements
template<class T, class A, class Predicate>
size_t erase_if(std::vector<T, A>& c, Predicate pred) {
	const size_t numBefore = c.size();
	c.erase(std::remove_if(c.begin(), c.end(), pred), c.end());
	return numBefore - c.size();
}