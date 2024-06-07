// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BaseGame.h"

class FAntiCheatClient;
class FAntiCheatNetworkTransport;

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
	* Called just before shutting down the game. Allows to finish current operations.
	*/
	virtual void OnShutdown() override;

	/**
	* Game event dispatcher
	*
	* @param Event - Game event to be dispatched
	*/
	virtual void OnGameEvent(const FGameEvent& Event) override;

	/** 
	 * Getter for Anti-Cheat Client component.
	 */
	const std::unique_ptr<FAntiCheatClient>& GetAntiCheatClient();

	/** 
	 * Getter for Anti-Cheat Network Transport component.
	 */
	const std::unique_ptr<FAntiCheatNetworkTransport>& GetAntiCheatNetworkTransport();

protected:
	/** Anti-Cheat Client component */
	std::unique_ptr<FAntiCheatClient> AntiCheatClient;

	/** Anti-Cheat Network Transport component */
	std::unique_ptr<FAntiCheatNetworkTransport> AntiCheatNetworkTransport;
};
