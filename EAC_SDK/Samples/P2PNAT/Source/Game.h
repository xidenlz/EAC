// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BaseGame.h"

class FP2PNAT;

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
	 * Getter for P2PNAT component.
	 */
	const std::unique_ptr<FP2PNAT>& GetP2PNAT();

protected:
	/** Peer to peer NAT component */
	std::unique_ptr<FP2PNAT> P2PNATComponent;

	/** Timestamp to know when shutdown was triggered */
	double ShutdownTriggeredTimestamp = 0.0;
};

