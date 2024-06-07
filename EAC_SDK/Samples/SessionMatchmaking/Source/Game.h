// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BaseGame.h"

class FSessionMatchmaking;

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
	 * Creation
	 */
	virtual void Create() override;

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
	 * Called before application shutdown
	 */
	virtual void OnShutdown() override;

	/** 
	 * Called to check if shutdown needs to be delayed
	 */
	virtual bool IsShutdownDelayed() override;

	/**
	 * Getter for sessions component.
	 */
	const std::unique_ptr<FSessionMatchmaking>& GetSessions();

protected:
	virtual void CreateConsoleCommands() override;

	/** Leaderboard component */
	std::unique_ptr<FSessionMatchmaking> SessionMatchmaking;

	/** Timestamp to know when shutdown was triggered */
	double ShutdownTriggeredTimestamp = 0.0;
};

