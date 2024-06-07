// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BaseGame.h"

class FMods;

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
	* Init
	*/
	virtual void Init() override;

	/**
	* Game event dispatcher
	*
	* @param Event - Game event to be dispatched
	*/
	virtual void OnGameEvent(const FGameEvent& Event) override;

	/**
	 * Getter for mods component.
	 */
	const std::unique_ptr<FMods>& GetMods();

protected:
	/**
	* Creates all console commands
	*/
	virtual void CreateConsoleCommands() override;

	/** Mods component */
	std::unique_ptr<FMods> Mods;

	/** Timestamp to know when shutdown was triggered */
	double ShutdownTriggeredTimestamp = 0.0;
};

