// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

/**
* Main class for Windows project
*/
class FMain
{
public:
	/**
	* Constructor
	*/
	FMain() noexcept(false);

	/**
	* No copying or copy assignment allowed for this class.
	*/
	FMain(FMain const&) = delete;
	FMain& operator=(FMain const&) = delete;

	/**
	* Destructor
	*/
	virtual ~FMain();

	/**
	* Initializes command line
	*/
	void InitCommandLine();

	/**
	* Utility function for printing a message to in-game console
	*
	* @param Message - Message to print to console
	*/
	void PrintToConsole(const std::wstring& Message);

	/**
	* Utility function for printing a warning message to in-game console
	*
	* @param Message - Message to print to console
	*/
	void PrintWarningToConsole(const std::wstring& Message);

	/**
	* Utility function for printing an error message to in-game console
	*
	* @param Message - Message to print to console
	*/
	void PrintErrorToConsole(const std::wstring& Message);
};

/** Global accessor for main */
extern std::unique_ptr<FMain> Main;