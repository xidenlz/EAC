// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "Game.h"
#include "Menu.h"
#include "Main.h"
#include "TextLabel.h"
#include "TextView.h"
#include "Button.h"
#include "UIEvent.h"
#include "OfferList.h"
#include "Sprite.h"
#include "OfferInfo.h"
#include "Store.h"
#include "GameEvent.h"
#include "StringUtils.h"

static const float PurchaseButtonSizeX = 100.0f;

FOfferInfoWidget::FOfferInfoWidget(Vector2 InfoPos,
									 Vector2 InfoSize,
									 UILayer InfoLayer,
									 FOfferData InOfferData,
									 FontPtr InfoLargeFont,
									 FontPtr InfoSmallFont,
									 const FOfferInfoButtonParams& InfoButtonParams) :
	IWidget(InfoPos, InfoSize, InfoLayer),
	OfferData(InOfferData),
	LargeFont(InfoLargeFont),
	SmallFont(InfoSmallFont),
	ButtonParams(InfoButtonParams)
{
	BackgroundImage = std::make_shared<FSpriteWidget>(Vector2(0.f, 0.f), Vector2(200.f, 100.f), InfoLayer, L"Assets/friendback.dds");
}

void FOfferInfoWidget::Create()
{
	if (ButtonParams.ButtonName.empty() || !ButtonParams.OnButtonClicked)
		return;

	if (BackgroundImage)
	{
		BackgroundImage->Create();
	}

	Button.reset();

	bool bHasPrice = true; //  OfferData.bPriceValid;

	Vector2 NameOffset = Vector2(1.f, Size.y * 0.25f);

	Vector2 NameSize = (bHasPrice) ?
		Vector2(Size.x, Size.y / 2.f) :
		Size;

	NameLabel = std::make_shared<FTextLabelWidget>(
		Position + NameOffset,
		NameSize,
		Layer - 1,
		OfferData.Title,
		L"",
		FColor(0.5f, 0.5f, 0.5f, 1.f),
		FColor(1.f, 1.f, 1.f, 1.f),
		EAlignmentType::Left);

	NameLabel->Create();
	NameLabel->SetFont(LargeFont);
	NameLabel->SetText(OfferData.Title);

	if (bHasPrice)
	{
		const Vector2 ButtonSize = Vector2(PurchaseButtonSizeX, Size.y / 2.0f);
		const Vector2 LeftButtonOffset((Size.x - ButtonSize.x - 5.0f) , Size.y * 0.25f);

		Button = std::make_shared<FButtonWidget>(
			Position + LeftButtonOffset,
			ButtonSize,
			Layer - 1,
			ButtonParams.ButtonName,
			assets::DefaultButtonAssets,
			SmallFont,
			assets::DefaultButtonColors[0]);
		Button->SetBackgroundColors(assets::DefaultButtonColors);
		Button->Create();
		Button->SetOnPressedCallback([this]() { ButtonParams.OnButtonClicked(OfferData); });
	}
}

void FOfferInfoWidget::Release()
{
	if (BackgroundImage)
	{
		BackgroundImage->Release();
		// Do not need to reset the pointer because we can reuse the same object.
	}

	if (NameLabel)
	{
		NameLabel->Release();
		NameLabel.reset();
	}

	if (Button)
	{
		Button->Release();
		Button.reset();
	}
}

void FOfferInfoWidget::Update()
{
	if (!bShown)
		return;

	if (BackgroundImage)
	{
		BackgroundImage->Update();
	}

	if (NameLabel)
	{
		NameLabel->Update();
	}

	if (Button)
	{
		Button->Update();
	}
}

void FOfferInfoWidget::Render(FSpriteBatchPtr& Batch)
{
	if (!bShown)
		return;

	IWidget::Render(Batch);

	if (BackgroundImage)
	{
		BackgroundImage->Render(Batch);
	}

	if (NameLabel)
	{
		NameLabel->Render(Batch);
	}

	if (Button)
	{
		Button->Render(Batch);
	}
}

#ifdef _DEBUG
void FOfferInfoWidget::DebugRender()
{
	IWidget::DebugRender();

	if (BackgroundImage) BackgroundImage->DebugRender();
	if (NameLabel) NameLabel->DebugRender();
	if (Button) Button->DebugRender();
}
#endif

void FOfferInfoWidget::OnUIEvent(const FUIEvent& event)
{
	if (!bShown)
		return;

	if (event.GetType() == EUIEventType::MousePressed || event.GetType() == EUIEventType::MouseReleased)
	{
		if (Button && Button->CheckCollision(event.GetVector()))
		{
			Button->OnUIEvent(event);
		}
	}
}

void FOfferInfoWidget::SetPosition(Vector2 Pos)
{
	IWidget::SetPosition(Pos);

	if (BackgroundImage)
	{
		BackgroundImage->SetPosition(Pos);
	}

	if (NameLabel)
	{
		Vector2 NameOffset = Vector2(1.f, Size.y * 0.25f);
		NameLabel->SetPosition(Pos + NameOffset);
	}

	const Vector2 ButtonSize = Vector2(PurchaseButtonSizeX, Size.y / 2.0f);
	const Vector2 LeftButtonOffset((Size.x - ButtonSize.x - 5.0f), Size.y * 0.25f);

	if (Button)
	{
		Button->SetPosition(Position + LeftButtonOffset);
	}
}

void FOfferInfoWidget::SetSize(Vector2 NewSize)
{
	IWidget::SetSize(NewSize);

	if (BackgroundImage) BackgroundImage->SetSize(NewSize);

	if (NameLabel) NameLabel->SetSize(Vector2(NewSize.x, NameLabel->GetSize().y));

	if (Button)
	{
		const Vector2 ButtonSize = Vector2(PurchaseButtonSizeX, Size.y / 2.0f);
		Button->SetSize(ButtonSize);
	}
}

void FOfferInfoWidget::SetOfferData(const FOfferData& Data)
{
	bool bNeedReset = false;
	bool bPriceChanged = (Data.bPriceValid != OfferData.bPriceValid);
	bool bDifferentOffer = (Data.UserId != OfferData.UserId) || (Data.Title != OfferData.Title);
	
	bNeedReset = (bPriceChanged || bDifferentOffer);

	OfferData = Data;

	if (bNeedReset)
	{
		Release();
		Create();
	}
}

void FOfferInfoWidget::SetFocused(bool bFocused)
{
	IWidget::SetFocused(bFocused);

	FColor Col = NameLabel->GetBackgroundColor();
	if (bFocused)
	{
		Col.R += 0.3f;
		if (Col.R > 1.0f)
		{
			Col.R = 1.0f;
		}
	}
	else
	{
		Col.R -= 0.3f;
		if (Col.R < 0.0f)
		{
			Col.R = 0.0f;
		}
	}
	NameLabel->SetBackgroundColor(Col);
}
