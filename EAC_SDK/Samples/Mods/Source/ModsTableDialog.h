// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Dialog.h"
#include "Font.h"
#include "TableView.h"

/**
* Forward declarations
*/
class FGameEvent;
class FSpriteWidget;

/**
* Mods Table Dialog
*/
class FModsTableDialog : public FDialog
{
public:
	/**
	* Constructor
	*/
	FModsTableDialog(Vector2 DialogPos,
		Vector2 DialogSize,
		UILayer DialogLayer,
		FontPtr DialogNormalFont,
		FontPtr DialogSmallFont,
		FontPtr DialogTinyFont);

	/**
	* Destructor
	*/
	virtual ~FModsTableDialog() {};

	/**
	* IGfxComponent Overrides
	*/
	virtual void Update() override;
	virtual void Create() override;

	/** IWidget */
	virtual void SetPosition(Vector2 Pos) override;
	virtual void SetSize(Vector2 NewSize) override;
	void SetWindowSize(Vector2 WindowSize);
	void SetWindowProportion(Vector2 InWindowProportion) { ConsoleWindowProportion = InWindowProportion; }

	/**
	* Receives game event
	*
	* @param Event - Game event to act on
	*/
	void OnGameEvent(const FGameEvent& Event);

private:
	/** Header label */
	std::shared_ptr<FTextLabelWidget> HeaderLabel;

	/** Background Image */
	std::shared_ptr<FSpriteWidget> BackgroundImage;

	/** Part of window that console is taking */
	Vector2 ConsoleWindowProportion;

	/** Table view with the list of mods. */
	std::shared_ptr<FModsTableView> ModsTable;
};
