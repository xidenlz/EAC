// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Widget.h"
#include "Scroller.h"
#include "Font.h"
#include "Store.h"
#include "OfferInfo.h"


/** Forward declarations */
class FTextFieldWidget;
class FTextViewWidget;
class FTextLabelWidget;
class FUIEvent;
class FScroller;

/**
* A reusable widget used to display a list of offers in both the Catalog and the Cart.
*/
class FOfferListWidget : public IWidget, public IScrollable
{
public:
	/**
	 * Constructor
	 */
	FOfferListWidget(Vector2 OfferListPos,
		Vector2 OfferLisSize,
		UILayer OfferListLayer,
		FontPtr OfferListNormalFont,
		FontPtr OfferListTitleFont,
		FontPtr OfferListSmallFont,
		FontPtr OfferListTinyFont,
		const std::wstring& Title,
		const FOfferInfoButtonParams& OfferInfoButtonParams);

	FOfferListWidget(const FOfferListWidget&) = delete;
	FOfferListWidget& operator=(const FOfferListWidget&) = delete;

	/**
	 * Destructor
	 */
	virtual ~FOfferListWidget() {};

	/** IGfxComponent */
	virtual void Create() override;
	virtual void Release() override;
	virtual void Update() override;
	virtual void Render(FSpriteBatchPtr& Batch) override;
#ifdef _DEBUG
	virtual void DebugRender() override;
#endif

	/** IWidget */
	virtual void OnUIEvent(const FUIEvent& event) override;
	virtual void SetFocused(bool) override;
	virtual void SetPosition(Vector2 Pos) override;
	virtual void SetSize(Vector2 NewSize) override;

	/** IScrollable */
	virtual void ScrollUp(size_t length) override;
	virtual void ScrollDown(size_t length) override;
	virtual void ScrollToTop() override;
	virtual void ScrollToBottom() override;
	virtual size_t NumEntries() const override;
	virtual size_t GetNumLinesPerPage() const override;
	virtual size_t FirstViewedEntry() const override;
	virtual size_t LastViewedEntry() const override;

	/** Refreshes Offer data with given collection of Offers */
	void RefreshOfferData(const std::vector<FOfferData>& Offers);

	/** Sets visibility of Offer info */
	void SetOfferInfoVisible(bool bVisible) { bOfferInfoVisible = bVisible; }

	/** Sets filter to display a subset of available Offers */
	void SetFilter(const std::wstring& filter)
	{
		NameFilter = filter; 
		bIsFilterSet = !NameFilter.empty();
		if (bIsFilterSet)
		{
			bCanPerformSearch = true;
			FirstOfferToView = 0;
		}
	}

	/** Clears filter */
	void ClearFilter()
	{
		NameFilter.clear();
		bIsFilterSet	= false;
		bCanPerformSearch = false;
		FirstOfferToView = 0;
	}

	/** Clears Offer list */
	void Clear();

	/** Resets Offer list */
	void Reset();

	/** Sets offset from bottom. */
	void SetBottomOffset(float Value) { BottomOffset = Value; }

private:
	/** Creates widgets for Offers info */
	void CreateOffers();

	/** Offer data */
	std::vector<FOfferData> OfferData;

	/** Offer data once filter has been applied */
	std::vector<FOfferData> FilteredData;

	/** Value for filter */
	std::wstring NameFilter;

	/** True if filter has been set to a non-default value */
	bool bIsFilterSet = false;

	/** True if there's not a search active already */
	bool bCanPerformSearch = false;

	/** Background Image */
	std::shared_ptr<FSpriteWidget> BackgroundImage;
	
	/** Title Label */
	std::shared_ptr<FTextLabelWidget> TitleLabel;

	/** Collection of all Offer info widgets */
	std::vector<std::shared_ptr<FOfferInfoWidget>> OfferWidgets;

	/** Widget controlling Offer search */
	std::shared_ptr<FTextFieldWidget> SearchOfferWidget;

	/** Search button */
	std::shared_ptr<FButtonWidget> SearchButtonWidget;

	/** Cancel search button */
	std::shared_ptr<FButtonWidget> CancelSearchButtonWidget;

	/** Offer Info button name used when creating new offer info widgets. */
	FOfferInfoButtonParams OfferInfoButtonParams;

	/** Shared font */
	FontPtr NormalFont;

	/** Title font */
	FontPtr TitleFont;

	/** Small font */
	FontPtr SmallFont;

	/** Tiny font */
	FontPtr TinyFont;

	/** Scroller */
	std::shared_ptr<FScroller> Scroller;

	/** Index of first Offers to view */
	size_t FirstOfferToView = 0;

	/** Flag used to hide / show Offer info */
	bool bOfferInfoVisible = true;

	/** Offset from the bottom of widget that is not used. */
	float BottomOffset = 0.0f;

	/** Colors */
	static constexpr FColor EnabledCol = FColor(1.f, 1.f, 1.f, 1.f);
	static constexpr FColor DisabledCol = FColor(0.2f, 0.2f, 0.2f, 1.f);
};
