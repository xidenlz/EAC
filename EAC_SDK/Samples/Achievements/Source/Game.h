// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BaseGame.h"

class FAchievements;

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
	* Initialization
	*/
	virtual void Init() override;

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
	* Called just before shutting down the game. Allows to finish current operations.
	*/
	virtual void OnShutdown() override;

	/** 
	 * Getter for achievements component.
	 */
	const std::unique_ptr<FAchievements>& GetAchievements();

protected:
	/**
	* Creates all console commands
	*/
	virtual void CreateConsoleCommands() override;

	/** Achievements component */
	std::unique_ptr<FAchievements> Achievements;
};
