// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BaseGame.h"

class FLeaderboard;

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
	 * Getter for leaderboard component.
	 */
	const std::unique_ptr<FLeaderboard>& GetLeaderboard();

protected:
	/**
	* Creates all console commands
	*/
	virtual void CreateConsoleCommands() override;

	/** Leaderboard component */
	std::unique_ptr<FLeaderboard> Leaderboard;
};

