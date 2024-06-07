// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "Game.h"
#include "TextLabel.h"
#include "Button.h"
#include "UIEvent.h"
#include "GameEvent.h"
#include "AccountHelpers.h"
#include "Player.h"
#include "OfferList.h"
#include "StoreDialog.h"

namespace
{
	constexpr float CheckoutButtonSizeY = 30.0f;
	constexpr float CheckoutButtonHorizontalOffset = 15.0f;
}

FStoreDialog::FStoreDialog(
	Vector2 DialogPos,
	Vector2 DialogSize,
	UILayer DialogLayer,
	FontPtr DialogNormalFont,
	FontPtr DialogSmallFont,
	FontPtr DialogTinyFont) :
	FDialog(DialogPos, DialogSize, DialogLayer),
	NormalFont(DialogNormalFont),
	SmallFont(DialogSmallFont),
	TinyFont(DialogTinyFont)
{
	CreateCatalogWidget();
	CreateCartWidget();
	CreateCheckoutButtonWidget();
}

void FStoreDialog::CreateCatalogWidget()
{
	FOfferInfoButtonParams CatalogOfferButtonParams;
	CatalogOfferButtonParams.ButtonName = L"Add To Cart";
	CatalogOfferButtonParams.OnButtonClicked = std::move([](const FOfferData& Offer)
	{
		FGame::Get().GetStore()->AddToCart(Offer);
	});
	CatalogWidget = CreateOfferListWidget(L"Catalog", CatalogOfferButtonParams, CatalogWidgetBottomOffset);
}

void FStoreDialog::CreateCartWidget()
{
	FOfferInfoButtonParams CartOfferButtonParams;
	CartOfferButtonParams.ButtonName = L"Remove";
	CartOfferButtonParams.OnButtonClicked = std::move([](const FOfferData& Offer)
	{
		FGame::Get().GetStore()->RemoveFromCart(Offer);
	});
	CartWidget = CreateOfferListWidget(L"Cart", CartOfferButtonParams, CartWidgetBottomOffset);
}

std::shared_ptr<FOfferListWidget> FStoreDialog::CreateOfferListWidget(const std::wstring& Title, const FOfferInfoButtonParams& OfferInfoButtonParams, float BottomOffset)
{
	std::shared_ptr<FOfferListWidget> OfferListWidget = std::make_shared<FOfferListWidget>(
		Position,
		Size,
		Layer,
		NormalFont,
		SmallFont,
		SmallFont,
		TinyFont,
		Title,
		OfferInfoButtonParams);

	OfferListWidget->SetBottomOffset(BottomOffset);
	OfferListWidget->SetBorderColor(Color::UIBorderGrey);

	OfferListWidget->Create();
	AddWidget(OfferListWidget);

	return OfferListWidget;
}

void FStoreDialog::CreateCheckoutButtonWidget()
{
	CheckoutButton = std::make_shared<FButtonWidget>(
		Position,
		Vector2(Size.x - CheckoutButtonHorizontalOffset, CheckoutButtonSizeY),
		Layer - 1,
		L"Checkout",
		assets::DefaultButtonAssets,
		SmallFont,
		assets::DefaultButtonColors[0]);

	CheckoutButton->SetBackgroundColors(assets::DefaultButtonColors);
	CheckoutButton->Create();
	CheckoutButton->SetOnPressedCallback([]()
	{
		if (FGame::Get().GetStore())
		{
			FGame::Get().GetStore()->Checkout();
		}
	});			

	CheckoutButton->Create();
	CheckoutButton->Disable();
	CheckoutButton->Hide();
	AddWidget(CheckoutButton);
}

void FStoreDialog::SetPosition(Vector2 Pos)
{
	IWidget::SetPosition(Pos);

	if (CatalogWidget)
	{
		CatalogWidget->SetPosition(Vector2(ConsoleDialogPosition.x, Pos.y));
	}

	if (CartWidget)
	{
		CartWidget->SetPosition(Pos);

		if (CheckoutButton)
		{
			const float CheckoutButtonX = CartWidget->GetPosition().x + CheckoutButtonHorizontalOffset;
			const float CheckoutButtonY = CartWidget->GetPosition().y + CartWidget->GetSize().y - CartWidgetBottomOffset;
			CheckoutButton->SetPosition(Vector2(CheckoutButtonX, CheckoutButtonY));
		}
	}
}

void FStoreDialog::SetSize(Vector2 NewSize)
{
	IWidget::SetSize(NewSize);

	if (CatalogWidget)
	{
		CatalogWidget->SetSize(Vector2(ConsoleDialogSize.x, ConsoleDialogPosition.y - Position.y - 10.0f));
	}

	if (CartWidget)
	{
		CartWidget->SetSize(NewSize);

		if (CheckoutButton)
		{
			CheckoutButton->SetSize(Vector2(CartWidget->GetSize().x - 2 * CheckoutButtonHorizontalOffset, CheckoutButtonSizeY));
		}
	}
}

void FStoreDialog::Update()
{
	FDialog::Update();

	FStore* StorePtr = FGame::Get().GetStore().get();
	if (StorePtr && StorePtr->GetCurrentUser().IsValid())
	{
		if (StorePtr->IsDirty())
		{
			if (CatalogWidget)
			{
				CatalogWidget->RefreshOfferData(StorePtr->GetCatalog());
			}
			StorePtr->SetDirty(false);
		}

		if (StorePtr->IsCartDirty())
		{
			if (CartWidget)
			{
				const std::list<FOfferData>& CartList = StorePtr->GetCart();
				CartWidget->RefreshOfferData(std::vector<FOfferData>(CartList.begin(), CartList.end()));

				if (CheckoutButton)
				{
					if (CartList.empty())
					{
						CheckoutButton->Disable();
					}
					else
					{
						CheckoutButton->Enable();
					}
				}
			}
			StorePtr->SetCartDirty(false);
		}
	}
}

void FStoreDialog::OnGameEvent(const FGameEvent& Event)
{
	if (Event.GetType() == EGameEventType::UserLoggedIn)
	{
		SetOfferInfoVisible(true);
	}
	else if (Event.GetType() == EGameEventType::UserLoginRequiresMFA)
	{
		SetOfferInfoVisible(false);
		SetFocused(false);
	}
	else if (Event.GetType() == EGameEventType::UserLoginEnteredMFA)
	{
		SetOfferInfoVisible(true);
	}
	else if (Event.GetType() == EGameEventType::UserLoggedOut)
	{
		if (FPlayerManager::Get().GetNumPlayers() == 0)
		{
			Clear();
			SetOfferInfoVisible(false);
		}
	}
	else if (Event.GetType() == EGameEventType::ShowPrevUser)
	{
		Clear();
		Reset();
	}
	else if (Event.GetType() == EGameEventType::ShowNextUser)
	{
		Clear();
		Reset();
	}
	else if (Event.GetType() == EGameEventType::NewUserLogin)
	{
		SetOfferInfoVisible(false);
		Clear();
	}
	else if (Event.GetType() == EGameEventType::CancelLogin)
	{
		Clear();
		Reset();
	}
}

void FStoreDialog::SetOfferInfoVisible(bool bVisible)
{
	if (CatalogWidget)
	{
		CatalogWidget->SetOfferInfoVisible(bVisible);
	}

	if (CartWidget)
	{
		CartWidget->SetOfferInfoVisible(bVisible);
	}

	if (CheckoutButton)
	{
		if (bVisible)
			CheckoutButton->Show();
		else
			CheckoutButton->Hide();
	}
}

void FStoreDialog::Reset()
{
	if (CatalogWidget)
	{
		CatalogWidget->SetOfferInfoVisible(true);
		CatalogWidget->Reset();
	}

	if (CartWidget)
	{
		CartWidget->SetOfferInfoVisible(true);
		CartWidget->Reset();
	}

	if (CheckoutButton)
	{
		CheckoutButton->Disable();
	}
}

void FStoreDialog::Clear()
{
	if (CatalogWidget)
	{
		CatalogWidget->RefreshOfferData(std::vector<FOfferData>());
		CatalogWidget->Reset();
	}

	if (CartWidget)
	{
		CartWidget->RefreshOfferData(std::vector<FOfferData>());
		CartWidget->Reset();
	}

	if (CheckoutButton)
	{
		CheckoutButton->Disable();
	}
}
