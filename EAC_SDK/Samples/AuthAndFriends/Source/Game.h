// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BaseGame.h"
#include "CustomInvites.h"

class FCustomInvites;

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
	* Creates all console commands
	*/
	virtual void CreateConsoleCommands() override;

	/**
	* Game event dispatcher
	*
	* @param Event - Game event to be dispatched
	*/
	virtual void OnGameEvent(const FGameEvent& Event) override;

	/**
	 * Getter for custom invites component.
	 */
	const std::unique_ptr<FCustomInvites>& GetCustomInvites() { return CustomInvites; }

protected:
	/** CustomInvites component */
	std::unique_ptr<FCustomInvites> CustomInvites;
};
