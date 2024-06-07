// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Dialog.h"
#include "Font.h"

/**
 * Forward declarations
 */
struct FOfferInfoButtonParams;
class FOfferListWidget;
class FGameEvent;

/**
 * Store dialog
 */
class FStoreDialog : public FDialog
{
public:
	/**
	 * Constructor
	 */
	FStoreDialog(Vector2 DialogPos,
		Vector2 DialogSize,
		UILayer DialogLayer,
		FontPtr DialogNormalFont,
		FontPtr DialogSmallFont,
		FontPtr DialogTinyFont);

	/**
	 * Destructor
	 */
	virtual ~FStoreDialog() {};

	/** IWidget */
	virtual void SetPosition(Vector2 Pos) override;
	virtual void SetSize(Vector2 NewSize) override;
	virtual void Update() override;

	/**
	* Receives game event
	*
	* @param Event - Game event to act on
	*/
	void OnGameEvent(const FGameEvent& Event);

	/** Sets visibility of offer info */
	void SetOfferInfoVisible(bool bVisible);

	/** Reset */
	void Reset();

	/** Clear */
	void Clear();

	/** Set Console Dialog Size and Position */
	void SetConsoleDialogSizeAndPosition(Vector2 InConsoleDialogSize, Vector2 InConsoleDialogPosition)
	{
		ConsoleDialogSize = InConsoleDialogSize;
		ConsoleDialogPosition = InConsoleDialogPosition;
	}
	
private:
	
	/** Creates the store catalog widget to display available offers. */
	void CreateCatalogWidget();

	/** Creates the store cart widget to display the current offers added to cart */
	void CreateCartWidget();

	/** Creates a offer list widget using the given parameters. */
	std::shared_ptr<FOfferListWidget> CreateOfferListWidget(const std::wstring& Title, const FOfferInfoButtonParams& OfferInfoButtonParams, float BottomOffset);

	/** Creates the checkout widget */
	void CreateCheckoutButtonWidget();

	/** The Catalog widget */
	std::shared_ptr<FOfferListWidget> CatalogWidget;

	/** The Cart widget */
	std::shared_ptr<FOfferListWidget> CartWidget;

	/** The Checkout button */
	std::shared_ptr<FButtonWidget> CheckoutButton;

	/** Console Dialog Size*/
	Vector2 ConsoleDialogSize;

	/** Console Dialog Position */
	Vector2 ConsoleDialogPosition;

	/** Bottom offset for the catalog widget */
	float CatalogWidgetBottomOffset = 50.0f;

	/** Bottom offset for the cart widget */
	float CartWidgetBottomOffset = 100.0f;

	/** Normal Font */
	FontPtr NormalFont;

	/** Small Font */
	FontPtr SmallFont;

	/** TinyFont */
	FontPtr TinyFont;
};
