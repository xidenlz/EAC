// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "StringUtils.h"
#include "AccountHelpers.h"
#include "Input.h"
#include "Game.h"
#include "Main.h"
#include "Console.h"
#include "Store.h"
#include "TextLabel.h"
#include "TextField.h"
#include "TextView.h"
#include "Button.h"
#include "UIEvent.h"
#include "OfferInfo.h"
#include "OfferList.h"

namespace
{
	constexpr float LabelHeight = 25.f;
	constexpr float InputFieldHeight = LabelHeight;
	constexpr float OfferInfoHeight = 1.8f * InputFieldHeight;
	constexpr float ScrollerWidth = 10.f;
}

constexpr FColor FOfferListWidget::EnabledCol;
constexpr FColor FOfferListWidget::DisabledCol;

FOfferListWidget::FOfferListWidget(Vector2 OfferListPos,
									 Vector2 OfferLisSize,
									 UILayer OfferListLayer,
									 FontPtr OfferListNormalFont,
									 FontPtr OfferListTitleFont,
									 FontPtr OfferListSmallFont,
									 FontPtr OfferListTinyFont,
									 const std::wstring& Title,
									 const FOfferInfoButtonParams& InOfferInfoButtonParams) :
	IWidget(OfferListPos, OfferLisSize, OfferListLayer),
	OfferInfoButtonParams(InOfferInfoButtonParams),
	NormalFont(OfferListNormalFont),
	TitleFont(OfferListTitleFont),
	SmallFont(OfferListSmallFont),
	TinyFont(OfferListTinyFont)
{
	BackgroundImage = std::make_shared<FSpriteWidget>(Vector2(0.f, 0.f), Vector2(200.f, 100.f), OfferListLayer, L"Assets/friends.dds");

	TitleLabel = std::make_shared<FTextLabelWidget>(
		Vector2(Position.x, Position.y),
		Vector2(OfferLisSize.x, 30.f),
		OfferListLayer - 1,
		Title,
		L"Assets/dialog_title.dds",
		FColor(1.f, 1.f, 1.f, 1.f),
		FColor(1.f, 1.f, 1.f, 1.f),
		EAlignmentType::Left);
	TitleLabel->SetBorderColor(Color::UIBorderGrey);
}

void FOfferListWidget::Create()
{
	if (OfferInfoButtonParams.ButtonName.empty() || !OfferInfoButtonParams.OnButtonClicked)
	{
		return;
	}

	BackgroundImage->Create();

	// Title
	TitleLabel->Create();
	TitleLabel->SetFont(TitleFont);

	// Scroller
	Vector2 scrollerPosition = Vector2(Position.x + Size.x - ScrollerWidth - 4.f, Position.y + LabelHeight + InputFieldHeight + 20.f);
	Scroller = std::make_unique<FScroller>(std::static_pointer_cast<FOfferListWidget>(shared_from_this()),
		scrollerPosition,
		Vector2(ScrollerWidth, Size.y - InputFieldHeight - LabelHeight - BottomOffset - 50.0f),
		Layer - 1,
		L"Assets/scrollbar.dds");
	if (Scroller)
	{
		Scroller->Create();
		Scroller->Hide();
	}

	// Input Field
	SearchOfferWidget = std::make_shared<FTextFieldWidget>(
		Vector2(Position.x + 5.f, Position.y + LabelHeight + 15.f),
		Vector2(Size.x - 10.f, InputFieldHeight),
		Layer - 1,
		L"Search Offers...",
		L"Assets/textfield.dds",
		TitleFont,
		FTextFieldWidget::EInputType::Normal,
		EAlignmentType::Left);
	if (SearchOfferWidget)
	{
		SearchOfferWidget->Create();
		SearchOfferWidget->SetOnEnterPressedCallback([this](const std::wstring& value)
		{
			if (SearchOfferWidget->IsFocused())
			{
				this->SetFilter(value);
			}
		});
		SearchOfferWidget->SetBorderColor(Color::UIBorderGrey);
	}

	Vector2 SearchPosition = Vector2(Position.x + Size.x - 20.f, Position.y + 48.f);
	Vector2 SearchSize = Vector2(10.f, 10.f);

	SearchButtonWidget = std::make_shared<FButtonWidget>(SearchPosition, SearchSize, Layer - 1, L"", std::vector<std::wstring>({ L"Assets/search.dds" }), nullptr);
	if (SearchButtonWidget)
	{
		SearchButtonWidget->Create();
		SearchButtonWidget->SetOnPressedCallback([this]()
		{
			if (IsFocused() && this->SearchOfferWidget)
			{
				this->SetFilter(this->SearchOfferWidget->GetText());
			}
		});
	}

	CancelSearchButtonWidget = std::make_shared<FButtonWidget>(SearchPosition, SearchSize, Layer - 1, L"", std::vector<std::wstring>({ L"Assets/nobutton.dds" }), nullptr);
	if (CancelSearchButtonWidget)
	{
		CancelSearchButtonWidget->Create();
		CancelSearchButtonWidget->SetOnPressedCallback([this]()
		{
			if (IsFocused() && this->SearchOfferWidget)
			{
				ClearFilter();
				this->SearchOfferWidget->Clear();
			}
		});
		CancelSearchButtonWidget->Hide();
	}

	CreateOffers();
}

void FOfferListWidget::Release()
{
	if (BackgroundImage) BackgroundImage->Release();

	NormalFont.reset();
	TitleFont.reset();
	SmallFont.reset();
	TinyFont.reset();

	if (TitleLabel)
	{
		TitleLabel->Release();
		TitleLabel.reset();
	}

	if (Scroller)
	{
		Scroller->Release();
		Scroller.reset();
	}

	for (auto& OfferInfo : OfferWidgets)
	{
		if (OfferInfo)
		{
			OfferInfo->Release();
			OfferInfo.reset();
		}
	}

	if (SearchOfferWidget)
	{
		SearchOfferWidget->Release();
		SearchOfferWidget.reset();
	}

	if (SearchButtonWidget)
	{
		SearchButtonWidget->Release();
		SearchButtonWidget.reset();
	}

	if (CancelSearchButtonWidget)
	{
		CancelSearchButtonWidget->Release();
		CancelSearchButtonWidget.reset();
	}
}

void FOfferListWidget::Update()
{
	if (bEnabled)
	{
		if (!bOfferInfoVisible)
			return;

		if (!FGame::Get().GetStore()->GetCurrentUser().IsValid())
		{
			return;
		}

		if (BackgroundImage) BackgroundImage->Update();

		FilteredData = OfferData;
		if (SearchOfferWidget && SearchButtonWidget && CancelSearchButtonWidget)
		{
			if (bIsFilterSet)
			{
				FilteredData.resize(0);
				std::wstring NameFilterUpper = FStringUtils::ToUpper(NameFilter);
				for (size_t i = 0; i < OfferData.size(); ++i)
				{
					std::wstring NameUpper = FStringUtils::ToUpper(OfferData[i].Title);
					if (NameUpper.find(NameFilterUpper) != std::wstring::npos)
					{
						FilteredData.push_back(OfferData[i]);
					}
				}

				if (FStringUtils::ToUpper(SearchOfferWidget->GetText()) == NameFilterUpper)
				{
					//show cancel search icon
					CancelSearchButtonWidget->Show();
					SearchButtonWidget->Hide();
				}
				else
				{
					//text is different, we can perform a different search, show search icon
					CancelSearchButtonWidget->Hide();
					SearchButtonWidget->Show();
				}
			}
			else
			{
				//filter is unset, we can perform search
				CancelSearchButtonWidget->Hide();
				SearchButtonWidget->Show();
			}
		}

		if (bCanPerformSearch && FilteredData.empty())
		{
			bCanPerformSearch = false;
		}

		if (TitleLabel) TitleLabel->Update();

		if (FirstOfferToView > (FilteredData.size() - OfferWidgets.size()))
		{
			FirstOfferToView = (FilteredData.size() - OfferWidgets.size());
		}

		for (size_t i = 0; i < OfferWidgets.size(); ++i)
		{
			auto& Widget = OfferWidgets[i];
			if (Widget)
			{
				if (FirstOfferToView + i < FilteredData.size())
				{
					Widget->Show();
					Widget->SetOfferData(FilteredData[FirstOfferToView + i]);
				}
				else
				{
					Widget->SetOfferData(FOfferData());
					Widget->Hide();
				}
				Widget->Update();
			}
		}

		if (SearchOfferWidget) SearchOfferWidget->Update();

		if (Scroller)
		{
			if (FilteredData.size() <= OfferWidgets.size())
			{
				Scroller->Hide();
			}
			else
			{
				Scroller->Show();
			}
			Scroller->Update();
		}
	}
}

void FOfferListWidget::Render(FSpriteBatchPtr& Batch)
{
	if (!bShown)
		return;

	IWidget::Render(Batch);

	if (BackgroundImage) BackgroundImage->Render(Batch);

	if (TitleLabel) TitleLabel->Render(Batch);

	if (bOfferInfoVisible && FGame::Get().GetStore()->GetCurrentUser().IsValid())
	{
		for (auto& widget : OfferWidgets)
		{
			widget->Render(Batch);
		}

		if (Scroller) Scroller->Render(Batch);

		if (SearchOfferWidget) SearchOfferWidget->SetTextColor(EnabledCol);

		if (SearchButtonWidget) SearchButtonWidget->SetBackgroundColor(EnabledCol);
		if (CancelSearchButtonWidget) CancelSearchButtonWidget->SetBackgroundColor(EnabledCol);
	}
	else
	{
		if (SearchOfferWidget) SearchOfferWidget->SetTextColor(DisabledCol);

		if (SearchButtonWidget) SearchButtonWidget->SetBackgroundColor(DisabledCol);
		if (CancelSearchButtonWidget) CancelSearchButtonWidget->SetBackgroundColor(EnabledCol);
	}

	if (SearchOfferWidget) SearchOfferWidget->Render(Batch);

	if (SearchButtonWidget) SearchButtonWidget->Render(Batch);
	if (CancelSearchButtonWidget) CancelSearchButtonWidget->Render(Batch);
}

#ifdef _DEBUG
void FOfferListWidget::DebugRender()
{
	IWidget::DebugRender();

	if (BackgroundImage) BackgroundImage->DebugRender();
	if (TitleLabel) TitleLabel->DebugRender();

	if (bOfferInfoVisible && FGame::Get().GetStore()->GetCurrentUser().IsValid())
	{
		for (auto& widget : OfferWidgets)
		{
			widget->DebugRender();
		}

		Scroller->DebugRender();
	}

	if (SearchOfferWidget) SearchOfferWidget->DebugRender();
	if (SearchButtonWidget) SearchButtonWidget->DebugRender();
	if (CancelSearchButtonWidget) CancelSearchButtonWidget->DebugRender();
}
#endif

void FOfferListWidget::SetPosition(Vector2 Pos)
{
	Vector2 OldPos = GetPosition();

	IWidget::SetPosition(Pos);

	if (BackgroundImage) BackgroundImage->SetPosition(Vector2(Position.x, Position.y));

	// Title Label
	if (TitleLabel) TitleLabel->SetPosition(Pos);

	if (Scroller && SearchOfferWidget && TitleLabel)
	{
		// Scroller
		Vector2 ScrollerPos = Vector2(Pos.x + Size.x - Scroller->GetSize().x - 5.f,
			Pos.y + TitleLabel->GetSize().y + SearchOfferWidget->GetSize().y + 15.f);
		Scroller->SetPosition(ScrollerPos);
	}

	// Offers
	for (auto& OfferInfo : OfferWidgets)
	{
		Vector2 OfferOldOffset = OfferInfo->GetPosition() - OldPos;
		OfferInfo->SetPosition(Pos + OfferOldOffset);
	}

	// Search Input Field
	if (SearchOfferWidget && TitleLabel)
	{
		Vector2 SearchFieldPos = Vector2(Pos.x + 5.f, Pos.y + TitleLabel->GetSize().y + (SearchOfferWidget->GetSize().y * 0.35f));
		SearchOfferWidget->SetPosition(SearchFieldPos);
	}

	Vector2 SearchPosition = Vector2(Position.x + Size.x - 20.f, Position.y + 48.f);
	if (SearchButtonWidget) SearchButtonWidget->SetPosition(SearchPosition);
	if (CancelSearchButtonWidget) CancelSearchButtonWidget->SetPosition(SearchPosition);
}

void FOfferListWidget::SetSize(Vector2 NewSize)
{
	IWidget::SetSize(NewSize);

	if (TitleLabel) TitleLabel->SetSize(Vector2(NewSize.x, 30.0f));

	if (BackgroundImage) BackgroundImage->SetSize(Vector2(NewSize.x, NewSize.y));

	if (SearchOfferWidget) SearchOfferWidget->SetSize(Vector2(NewSize.x - 8.f, 30.f));

	if (Scroller) Scroller->SetSize(Vector2(Scroller->GetSize().x, Size.y - InputFieldHeight - LabelHeight - BottomOffset - 50.0f));

	// Reset to add or remove Offers
	Reset();
}

void FOfferListWidget::OnUIEvent(const FUIEvent& Event)
{
	if (!bEnabled)
		return;

	if (!bOfferInfoVisible)
		return;

	if (!FGame::Get().GetStore()->GetCurrentUser().IsValid())
		return;

	if (Event.GetType() == EUIEventType::MousePressed || Event.GetType() == EUIEventType::MouseReleased)
	{
		for (auto& OfferInfo : OfferWidgets)
		{
			if (OfferInfo && OfferInfo->CheckCollision(Event.GetVector()))
			{
				OfferInfo->OnUIEvent(Event);
			}
		}

		if (Scroller && Scroller->CheckCollision(Event.GetVector()))
		{
			Scroller->OnUIEvent(Event);
		}

		bool bSearchButtonClicked = false;
		if (SearchButtonWidget && SearchButtonWidget->CheckCollision(Event.GetVector()))
		{
			SearchButtonWidget->OnUIEvent(Event);
			bSearchButtonClicked = true;
		}

		if (CancelSearchButtonWidget && CancelSearchButtonWidget->CheckCollision(Event.GetVector()))
		{
			CancelSearchButtonWidget->OnUIEvent(Event);
			bSearchButtonClicked = true;
		}

		if (!bSearchButtonClicked && SearchOfferWidget && SearchOfferWidget->CheckCollision(Event.GetVector()))
		{
			SearchOfferWidget->Clear();
			SearchOfferWidget->OnUIEvent(Event);
		}
	}
	else if (Event.GetType() == EUIEventType::KeyPressed ||
		Event.GetType() == EUIEventType::MouseWheelScrolled ||
		Event.GetType() == EUIEventType::TextInput)
	{
		if (SearchOfferWidget) SearchOfferWidget->OnUIEvent(Event);
		if (Scroller) Scroller->OnUIEvent(Event);
	}
	else if (Event.GetType() == EUIEventType::Last)
	{
		ClearFilter();
		if (SearchOfferWidget) SearchOfferWidget->Clear();
	}
	else
	{
		for (auto& OfferInfo : OfferWidgets)
		{
			OfferInfo->OnUIEvent(Event);
		}
	}
}

void FOfferListWidget::SetFocused(bool bValue)
{
	IWidget::SetFocused(bValue);

	if (!bValue)
	{
		if (TitleLabel) TitleLabel->SetFocused(false);
		if (SearchOfferWidget) SearchOfferWidget->SetFocused(false);
		for (auto NextOfferWidget : OfferWidgets)
		{
			if (NextOfferWidget)
			{
				NextOfferWidget->SetFocused(false);
			}
		}
	}
}

void FOfferListWidget::ScrollUp(size_t length)
{
	if (FirstOfferToView < length)
	{
		FirstOfferToView = 0;
	}
	else
	{
		FirstOfferToView -= length;
	}
}

void FOfferListWidget::ScrollDown(size_t length)
{
	FirstOfferToView += length;
	if (OfferWidgets.size() > FilteredData.size())
	{
		FirstOfferToView = 0;
		return;
	}

	if (FirstOfferToView > (FilteredData.size() - OfferWidgets.size()))
	{
		FirstOfferToView = (FilteredData.size() - OfferWidgets.size());
	}
}

void FOfferListWidget::ScrollToTop() 
{
	ScrollUp(FilteredData.size());
}

void FOfferListWidget::ScrollToBottom()
{
	ScrollDown(FilteredData.size());
}

size_t FOfferListWidget::NumEntries() const
{
	return FilteredData.size();
}


size_t FOfferListWidget::GetNumLinesPerPage() const
{
	return OfferWidgets.size();
}

size_t FOfferListWidget::FirstViewedEntry() const
{
	return FirstOfferToView;
}

size_t FOfferListWidget::LastViewedEntry() const
{
	return FirstOfferToView + OfferWidgets.size() - 1;
}

void FOfferListWidget::RefreshOfferData(const std::vector<FOfferData>& Offers)
{
	OfferData = Offers;

	if (Offers.empty())
	{
		for (auto& NextWidget : OfferWidgets)
		{
			NextWidget->SetOfferData(FOfferData());
		}
	}
}

void FOfferListWidget::Clear()
{
	for (auto& OfferInfo : OfferWidgets)
	{
		if (OfferInfo)
		{
			OfferInfo->Release();
			OfferInfo.reset();
		}
	}

	OfferWidgets.clear();
}

void FOfferListWidget::Reset()
{
	ClearFilter();
	Clear();
	CreateOffers();
}

void FOfferListWidget::CreateOffers()
{
	// Offers
	const size_t NumOffersOnScreen = size_t((Size.y - LabelHeight - InputFieldHeight - BottomOffset) / OfferInfoHeight);

	OfferWidgets.resize(NumOffersOnScreen);
	for (size_t i = 0; i < NumOffersOnScreen; ++i)
	{
		OfferWidgets[i] = std::make_shared<FOfferInfoWidget>(
			Vector2(Position.x, Position.y + LabelHeight + InputFieldHeight + (OfferInfoHeight * i) + 20.f),
			Vector2(Size.x - ScrollerWidth - 2.f, OfferInfoHeight),
			Layer - 1,
			FOfferData(),
			SmallFont,
			TinyFont,
			OfferInfoButtonParams);

		if (OfferWidgets[i])
		{
			OfferWidgets[i]->Create();
			OfferWidgets[i]->Hide();

			if (Scroller)
			{
				Vector2 OfferSize = Vector2(Size.x - Scroller->GetSize().x - 5.f, OfferWidgets[i]->GetSize().y);
				OfferWidgets[i]->SetSize(OfferSize);
			}
		}
	}
}
