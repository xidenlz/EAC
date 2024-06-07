// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BaseGame.h"

class FLobbies;

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
	* Create Resources
	*/
	virtual void Create() override;

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
	* Called in the process of shutdown. Shutdown is delayed until this function returns false.
	* This allows to finish and cleanup something before shutting down.
	*/
	virtual bool IsShutdownDelayed() override;

	/**
	 * Getter for lobbies component.
	 */
	const std::unique_ptr<FLobbies>& GetLobbies();

protected:
	/**
	* Creates all console commands
	*/
	virtual void CreateConsoleCommands() override;

	/** Lobbies component */
	std::unique_ptr<FLobbies> Lobbies;

	/** Timestamp to know when shutdown was triggered */
	double ShutdownTriggeredTimestamp = 0.0;
};

