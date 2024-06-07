// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BaseMenu.h"

class FCustomInvitesDialog;

/**
* In-Game Menu
*/
class FMenu : public FBaseMenu
{
public:
	/**
	* Constructor
	*/
	FMenu(std::weak_ptr<FConsole> InConsole) noexcept(false);

	/**
	* No copying or copy assignment allowed for this class.
	*/
	FMenu(FMenu const&) = delete;
	FMenu& operator=(FMenu const&) = delete;

	/**
	* Destructor
	*/
	virtual ~FMenu();

	/** 
	 * Initialize
	 */
	virtual void Create() override; 
	
	virtual void Release() override;

	virtual void OnGameEvent(const FGameEvent& Event) override;

	/**
	 * Creates the Custom Invites dialogs
	 */
	virtual void CreateCustomInvitesDialog();

	virtual void UpdateLayout(int Width, int Height) override;

 	/** Custom Invites Dialog */
 	std::shared_ptr<FCustomInvitesDialog> CustomInvitesDialog;

	std::shared_ptr<FTextLabelWidget> TitleLabel;

	Vector2 WindowSize;

};
