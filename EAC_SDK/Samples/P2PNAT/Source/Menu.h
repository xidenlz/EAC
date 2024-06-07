// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BaseMenu.h"

/**
* Forward declarations
*/
class FUIEvent;
class FConsole;
class FGameEvent;
class FFont;
class FConsoleDialog;
class FFriendsDialog;
class FExitDialog;
class FAuthDialogs;
class FSpriteWidget;
class FTextLabelWidget;
class FButtonWidget;
class FP2PNATDialog;
class FPopupDialog;

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
	virtual ~FMenu() {};

	/**
	* IGfxComponent Overrides
	*/
	virtual void Update() override;
	virtual void Render(FSpriteBatchPtr& Batch) override;
#ifdef _DEBUG
	virtual void DebugRender() override;
#endif
	virtual void Create() override;
	virtual void Release() override;

	/**
	* Updates layout of elements
	*/
	void UpdateLayout(int Width, int Height) override;

	/**
	* Event Callback
	*
	* @param Event - UI event
	*/
	void OnUIEvent(const FUIEvent& Event) override;

	/**
	* Receives game event
	*
	* @param Event - Game event to act on
	*/
	void OnGameEvent(const FGameEvent& Event) override;

	/** 
	 * Get P2P NAT Dialog
	 */
	std::shared_ptr<FP2PNATDialog> GetP2PNATDialog() const { return P2PNATDialog; }

private:
	/**
	 * Creates P2P NAT traversal dialog
	 */
	void CreateP2PNATDialog();

	/** 
	 * Creates docs button 
	 */
	void CreateNATDocsButton();

	/** P2P NAT Traversal dialog */
	std::shared_ptr<FP2PNATDialog> P2PNATDialog;

	/** NAT docs button */
	std::shared_ptr<FButtonWidget> NATDocsButton;
};
