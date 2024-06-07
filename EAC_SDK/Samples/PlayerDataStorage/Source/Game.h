// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BaseGame.h"

class FPlayerDataStorage;

/**
* Main game class
*/
class FGame : public FBaseGame
{
public:
	/**
	 * Constructor
	 */
	FGame() noexcept(false);

	/**
	 * Destructor
	 */
	virtual ~FGame() override;

	/**
	* Singleton's getter
	*/
	static FGame& Get()
	{
		return static_cast<FGame&>(GetBase());
	}

	/**
	* Main update game loop
	*/
	virtual void Update() override;

	/**
	* Game event dispatcher
	*
	* @param Event - Game event to be dispatched
	*/
	virtual void OnGameEvent(const FGameEvent& Event) override;

	/**
	 * Getter for PDS component.
	 */
	const std::unique_ptr<FPlayerDataStorage>& GetPlayerDataStorage();

protected:
	/**
	* Creates all console commands
	*/
	virtual void CreateConsoleCommands() override;

	/** PDS component */
	std::unique_ptr<FPlayerDataStorage> PlayerDataStorage;
};

