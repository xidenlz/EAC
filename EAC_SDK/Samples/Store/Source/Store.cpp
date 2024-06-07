// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "DebugLog.h"
#include "StringUtils.h"
#include "AccountHelpers.h"
#include "Game.h"
#include "GameEvent.h"
#include "Main.h"
#include "Platform.h"
#include "Users.h"
#include "Player.h"

#include "Store.h"
#include "eos_ecom.h"

FStore::FStore()
{
}

FStore::~FStore()
{
}


void FStore::Update()
{
	if (FPlayerManager::Get().GetNumPlayers() == 0)
	{
		return;
	}

	UpdateStoreTimer -= float(Main->GetTimer().GetElapsedSeconds());
	if (UpdateStoreTimer < 0.f)
	{
		if (CurrentUserId.IsValid())
		{
			QueryStore(CurrentUserId);
		}
		UpdateStoreTimer = UpdateStoreTimeoutSeconds;
	}
}

void FStore::Checkout()
{
	if (!FPlatform::IsInitialized())
	{
		FDebugLog::LogWarning(L"[EOS SDK] Can't Checkout - Platform Not Initialized");
		return;
	}

	if (GetCart().empty())
	{
		FDebugLog::LogWarning(L"Cart Empty!");
		return;
	}

	UpdateStoreTimer = UpdateStoreTimeoutSeconds;

	FDebugLog::Log(L"[EOS SDK] Checkout start");

	EOS_HEcom EcomHandle = EOS_Platform_GetEcomInterface(FPlatform::GetPlatformHandle());

	std::vector<EOS_Ecom_CheckoutEntry> CheckoutEntries;
	for (const auto& CartItem : GetCart())
	{
		EOS_Ecom_CheckoutEntry Entry;
		Entry.ApiVersion = EOS_ECOM_CHECKOUTENTRY_API_LATEST;
		Entry.OfferId = CartItem.Id.c_str();
		CheckoutEntries.push_back(Entry);
	}

	EOS_Ecom_CheckoutOptions CheckoutOptions{ 0 };
	CheckoutOptions.ApiVersion = EOS_ECOM_CHECKOUT_API_LATEST;
	CheckoutOptions.LocalUserId = CurrentUserId;
	CheckoutOptions.OverrideCatalogNamespace = nullptr;
	CheckoutOptions.EntryCount = static_cast<uint32_t>(CheckoutEntries.size());
	CheckoutOptions.Entries = &CheckoutEntries[0];

	EOS_Ecom_Checkout(EcomHandle, &CheckoutOptions, NULL, CheckoutCompleteCallbackFn);
}

void FStore::AddToCart(const FOfferData& OfferData)
{
	if (UserCart.Add(OfferData))
	{
		SetCartDirty(true);
	}
}

void FStore::RemoveFromCart(const FOfferData& OfferData)
{
	if (UserCart.Remove(OfferData))
	{
		SetCartDirty(true);
	}
}

const std::list<FOfferData>& FStore::GetCart() const
{
	return UserCart.CartItems;
}

void FStore::QueryStore(EOS_EpicAccountId LocalUserId)
{
	if (!FPlatform::IsInitialized())
	{
		FDebugLog::LogWarning(L"[EOS SDK] Can't Query Store - Platform Not Initialized");
		return;
	}

	UpdateStoreTimer = UpdateStoreTimeoutSeconds;

	FDebugLog::Log(L"[EOS SDK] Querying Store");

	EOS_HEcom EcomHandle = EOS_Platform_GetEcomInterface(FPlatform::GetPlatformHandle());

	EOS_Ecom_QueryOffersOptions QueryOptions{ 0 };
	QueryOptions.ApiVersion = EOS_ECOM_QUERYOFFERS_API_LATEST;
	QueryOptions.LocalUserId = LocalUserId;
	QueryOptions.OverrideCatalogNamespace = nullptr;

	EOS_Ecom_QueryOffers(EcomHandle, &QueryOptions, NULL, QueryStoreCompleteCallbackFn);
}

void FStore::QueryEntitlements(EOS_EpicAccountId LocalUserId)
{
	if (!FPlatform::IsInitialized())
	{
		FDebugLog::LogWarning(L"[EOS SDK] Can't Query Entitlements - Platform Not Initialized");
		return;
	}

	UpdateStoreTimer = UpdateStoreTimeoutSeconds;

	FDebugLog::Log(L"[EOS SDK] Querying Entitlements");

	EOS_HEcom EcomHandle = EOS_Platform_GetEcomInterface(FPlatform::GetPlatformHandle());

	EOS_Ecom_QueryEntitlementsOptions QueryOptions{ 0 };
	QueryOptions.ApiVersion = EOS_ECOM_QUERYENTITLEMENTS_API_LATEST;
	QueryOptions.LocalUserId = LocalUserId;
	QueryOptions.bIncludeRedeemed = true;

	EOS_Ecom_QueryEntitlements(EcomHandle, &QueryOptions, NULL, QueryEntitlementsCompleteCallbackFn);
}

void FStore::SetCatalog(FEpicAccountId InUserId, std::vector<FOfferData>&& InCatalog)
{
	Catalog.swap(InCatalog);
	bDirty = true;
}

void FStore::SetEntitlements(FEpicAccountId InUserId, std::vector<FEntitlementData>&& InEntitlements)
{
	Entitlements.swap(InEntitlements);
	bDirty = true;
}

void FStore::OnLoggedIn(FEpicAccountId UserId)
{
	if (!CurrentUserId.IsValid())
	{
		SetCurrentUser(UserId);
	}
}

void FStore::OnLoggedOut(FEpicAccountId UserId)
{
	Catalog.clear();
	Entitlements.clear();
	UserCart.Clear();
	UpdateStoreTimer = 0.f;

	if (FPlayerManager::Get().GetNumPlayers() > 0)
	{
		if (GetCurrentUser() == UserId)
		{
			FGameEvent Event(EGameEventType::ShowNextUser);
			OnGameEvent(Event);
		}
	}
	else
	{
		SetCurrentUser(FEpicAccountId());
	}
}

void FStore::OnGameEvent(const FGameEvent& Event)
{
	if (Event.GetType() == EGameEventType::UserLoggedIn)
	{
		FEpicAccountId UserId = Event.GetUserId();
		OnLoggedIn(UserId);
	}
	else if (Event.GetType() == EGameEventType::UserLoggedOut)
	{
		FEpicAccountId UserId = Event.GetUserId();
		OnLoggedOut(UserId);
	}
	else if (Event.GetType() == EGameEventType::ShowPrevUser)
	{
		if (FPlayerManager::Get().GetNumPlayers() > 0)
		{
			FEpicAccountId PrevPlayerId = FPlayerManager::Get().GetPrevPlayerId(GetCurrentUser());
			if (PrevPlayerId.IsValid())
			{
				SetCurrentUser(PrevPlayerId);
				SetDirty(true);
				QueryStore(PrevPlayerId);

				UserCart.Clear();
				SetCartDirty(true);
			}
			else
			{
				SetCurrentUser(FEpicAccountId());
			}
		}
	}
	else if (Event.GetType() == EGameEventType::ShowNextUser)
	{
		if (FPlayerManager::Get().GetNumPlayers() > 0)
		{
			FEpicAccountId NextPlayerId = FPlayerManager::Get().GetNextPlayerId(GetCurrentUser());
			if (NextPlayerId.IsValid())
			{
				SetCurrentUser(NextPlayerId);
				SetDirty(true);
				QueryStore(NextPlayerId);

				UserCart.Clear();
				SetCartDirty(true);
			}
			else
			{
				SetCurrentUser(FEpicAccountId());
			}
		}
	}
	else if (Event.GetType() == EGameEventType::RefreshOffers)
	{
		FEpicAccountId CurrPlayerId = GetCurrentUser();
		if (CurrPlayerId.IsValid())
		{
			SetDirty(true);
			QueryStore(CurrPlayerId);
		}
	}
	else if (Event.GetType() == EGameEventType::CancelLogin)
	{
		SetDirty(true);
		SetCartDirty(true);
	}
}

void FStore::OnCheckoutSuccessful()
{
	UserCart.Clear();
	SetCartDirty(true);
}

void EOS_CALL FStore::QueryStoreCompleteCallbackFn(const EOS_Ecom_QueryOffersCallbackInfo* OfferData)
{
	assert(OfferData != NULL);

	if (OfferData->ResultCode != EOS_EResult::EOS_Success)
	{
		if (OfferData->ResultCode == EOS_EResult::EOS_Ecom_CatalogOfferPriceInvalid)
		{
			FDebugLog::LogWarning(L"[EOS SDK] Query offer warning: %ls", FStringUtils::Widen(EOS_EResult_ToString(OfferData->ResultCode)).c_str());
		}
		else
		{
			FDebugLog::LogError(L"[EOS SDK] Query offer error: %ls", FStringUtils::Widen(EOS_EResult_ToString(OfferData->ResultCode)).c_str());
			return;
		}
	}

	FDebugLog::Log(L"[EOS SDK] Query offer complete - User ID: %ls", FEpicAccountId(OfferData->LocalUserId).ToString().c_str());

	EOS_HEcom EcomHandle = EOS_Platform_GetEcomInterface(FPlatform::GetPlatformHandle());

	EOS_Ecom_GetOfferCountOptions CountOptions{ 0 };
	CountOptions.ApiVersion = EOS_ECOM_GETOFFERCOUNT_API_LATEST;
	CountOptions.LocalUserId = OfferData->LocalUserId;
	uint32_t OfferCount = EOS_Ecom_GetOfferCount(EcomHandle, &CountOptions);

	FDebugLog::Log(L"[EOS SDK] NumOffers: %d", OfferCount);

	std::vector<FOfferData> NewOffers;
	NewOffers.reserve(OfferCount);

	EOS_Ecom_CopyOfferByIndexOptions IndexOptions{ 0 };
	IndexOptions.ApiVersion = EOS_ECOM_COPYOFFERBYINDEX_API_LATEST;
	IndexOptions.LocalUserId = OfferData->LocalUserId;
	for (IndexOptions.OfferIndex = 0; IndexOptions.OfferIndex < OfferCount; ++IndexOptions.OfferIndex)
	{
		EOS_Ecom_CatalogOffer* Offer;
		EOS_EResult CopyResult = EOS_Ecom_CopyOfferByIndex(EcomHandle, &IndexOptions, &Offer);
		switch (CopyResult)
		{
		case EOS_EResult::EOS_Success:
		case EOS_EResult::EOS_Ecom_CatalogOfferPriceInvalid:
		case EOS_EResult::EOS_Ecom_CatalogOfferStale:
			FDebugLog::Log(L"[EOS SDK] Offer[%d] id(%ls) title(%ls) Price[Result(%d) Curr(%ull) Original(%ull) DecimalPoint(%ud)] Available?(%ls) Limit[%d]",
				IndexOptions.OfferIndex,
				FStringUtils::Widen(Offer->Id).c_str(),
				FStringUtils::Widen(Offer->TitleText).c_str(),
				Offer->PriceResult, Offer->CurrentPrice64, Offer->OriginalPrice64, Offer->DecimalPoint,
				Offer->bAvailableForPurchase ? L"true" : L"false",
				Offer->PurchaseLimit);

			NewOffers.push_back(FOfferData{
				OfferData->LocalUserId,
				std::string(Offer->Id),
				FStringUtils::Widen(Offer->TitleText),
				Offer->PriceResult == EOS_EResult::EOS_Success,
				Offer->CurrentPrice64,
				Offer->OriginalPrice64,
				Offer->DecimalPoint
			});

			EOS_Ecom_CatalogOffer_Release(Offer);
			break;
		default:
			FDebugLog::Log(L"[EOS SDK] Offer[%d] invalid : %d", IndexOptions.OfferIndex, CopyResult);
			break;
		}
	}

	if (FGame::Get().GetStore())
	{
		FGame::Get().GetStore()->SetCatalog(OfferData->LocalUserId, std::move(NewOffers));
	}

	FGame::Get().GetStore()->QueryEntitlements(OfferData->LocalUserId);
}

void EOS_CALL FStore::QueryEntitlementsCompleteCallbackFn(const EOS_Ecom_QueryEntitlementsCallbackInfo* EntitlementData)
{
	assert(EntitlementData != NULL);

	if (EntitlementData->ResultCode != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"[EOS SDK] Query entitlement error: %ls", FStringUtils::Widen(EOS_EResult_ToString(EntitlementData->ResultCode)).c_str());
		return;
	}

	FDebugLog::Log(L"[EOS SDK] Query entitlement complete - User ID: %ls", FEpicAccountId(EntitlementData->LocalUserId).ToString().c_str());

	EOS_HEcom EcomHandle = EOS_Platform_GetEcomInterface(FPlatform::GetPlatformHandle());

	EOS_Ecom_GetEntitlementsCountOptions CountOptions{ 0 };
	CountOptions.ApiVersion = EOS_ECOM_GETENTITLEMENTSCOUNT_API_LATEST;
	CountOptions.LocalUserId = EntitlementData->LocalUserId;
	uint32_t EntitlementCount = EOS_Ecom_GetEntitlementsCount(EcomHandle, &CountOptions);

	FDebugLog::Log(L"[EOS SDK] NumEntitlements: %d", EntitlementCount);

	std::vector<FEntitlementData> NewEntitlements;
	NewEntitlements.reserve(EntitlementCount);

	EOS_Ecom_CopyEntitlementByIndexOptions IndexOptions{ 0 };
	IndexOptions.ApiVersion = EOS_ECOM_COPYENTITLEMENTBYINDEX_API_LATEST;
	IndexOptions.LocalUserId = EntitlementData->LocalUserId;
	for (IndexOptions.EntitlementIndex = 0; IndexOptions.EntitlementIndex < EntitlementCount; ++IndexOptions.EntitlementIndex)
	{
		EOS_Ecom_Entitlement* Entitlement;
		EOS_EResult CopyResult = EOS_Ecom_CopyEntitlementByIndex(EcomHandle, &IndexOptions, &Entitlement);
		switch (CopyResult)
		{
		case EOS_EResult::EOS_Success:
		case EOS_EResult::EOS_Ecom_EntitlementStale:
			FDebugLog::Log(L"[EOS SDK] Entitlement[%d] : %ls : %ls : %ls",
				IndexOptions.EntitlementIndex,
				FStringUtils::Widen(Entitlement->EntitlementName).c_str(),
				FStringUtils::Widen(Entitlement->EntitlementId).c_str(),
				Entitlement->bRedeemed ? L"TRUE" : L"FALSE");

			NewEntitlements.push_back(FEntitlementData{
				EntitlementData->LocalUserId,
				std::string(Entitlement->EntitlementName),
				std::string(Entitlement->EntitlementId),
				Entitlement->bRedeemed == EOS_TRUE
			});

			EOS_Ecom_Entitlement_Release(Entitlement);
			break;
		default:
			FDebugLog::Log(L"[EOS SDK] Entitlement[%d] invalid : %d", IndexOptions.EntitlementIndex, CopyResult);
			break;
		}
	}

	if (FGame::Get().GetStore())
	{
		FGame::Get().GetStore()->SetEntitlements(EntitlementData->LocalUserId, std::move(NewEntitlements));
	}
}

void EOS_CALL FStore::CheckoutCompleteCallbackFn(const EOS_Ecom_CheckoutCallbackInfo* CheckoutData)
{
	assert(CheckoutData != NULL);

	if (CheckoutData->ResultCode != EOS_EResult::EOS_Success)
	{
		FDebugLog::LogError(L"[EOS SDK] Checkout error: %ls", FStringUtils::Widen(EOS_EResult_ToString(CheckoutData->ResultCode)).c_str());

		std::wstring Msg = L"Checkout Failed";
		FGameEvent Event(EGameEventType::AddNotification, Msg);
		FGame::Get().OnGameEvent(Event);

		return;
	}

	FDebugLog::Log(L"[EOS SDK] Checkout complete - User ID: %ls", FEpicAccountId(CheckoutData->LocalUserId).ToString().c_str());
	if (CheckoutData->TransactionId)
	{
		EOS_Ecom_HTransaction TransactionHandle;

		EOS_Ecom_CopyTransactionByIdOptions CopyTransactionOptions{ 0 };
		CopyTransactionOptions.ApiVersion = EOS_ECOM_COPYTRANSACTIONBYID_API_LATEST;
		CopyTransactionOptions.LocalUserId = CheckoutData->LocalUserId;
		CopyTransactionOptions.TransactionId = CheckoutData->TransactionId;

		EOS_HEcom EcomHandle = EOS_Platform_GetEcomInterface(FPlatform::GetPlatformHandle());
		if (EOS_Ecom_CopyTransactionById(EcomHandle, &CopyTransactionOptions, &TransactionHandle) == EOS_EResult::EOS_Success)
		{
			EOS_Ecom_Transaction_GetEntitlementsCountOptions CountOptions{ 0 };
			CountOptions.ApiVersion = EOS_ECOM_TRANSACTION_GETENTITLEMENTSCOUNT_API_LATEST;
			uint32_t EntitlementCount = EOS_Ecom_Transaction_GetEntitlementsCount(TransactionHandle, &CountOptions);

			FDebugLog::Log(L"[EOS SDK] New Entitlements: %d", EntitlementCount);

			std::vector<FEntitlementData> NewEntitlements;
			NewEntitlements.reserve(EntitlementCount);

			EOS_Ecom_Transaction_CopyEntitlementByIndexOptions IndexOptions{ 0 };
			IndexOptions.ApiVersion = EOS_ECOM_TRANSACTION_COPYENTITLEMENTBYINDEX_API_LATEST;
			for (IndexOptions.EntitlementIndex = 0; IndexOptions.EntitlementIndex < EntitlementCount; ++IndexOptions.EntitlementIndex)
			{
				EOS_Ecom_Entitlement* Entitlement;
				EOS_EResult CopyResult = EOS_Ecom_Transaction_CopyEntitlementByIndex(TransactionHandle, &IndexOptions, &Entitlement);
				switch (CopyResult)
				{
				case EOS_EResult::EOS_Success:
				case EOS_EResult::EOS_Ecom_EntitlementStale:
					FDebugLog::Log(L"[EOS SDK] New Entitlement[%d] : %ls : %ls : %ls",
						IndexOptions.EntitlementIndex,
						FStringUtils::Widen(Entitlement->EntitlementName).c_str(),
						FStringUtils::Widen(Entitlement->EntitlementId).c_str(),
						Entitlement->bRedeemed ? L"TRUE" : L"FALSE");

					NewEntitlements.push_back(FEntitlementData{
						CheckoutData->LocalUserId,
						std::string(Entitlement->EntitlementName),
						std::string(Entitlement->EntitlementId),
						Entitlement->bRedeemed == EOS_TRUE
						});

					EOS_Ecom_Entitlement_Release(Entitlement);
					break;
				default:
					FDebugLog::Log(L"[EOS SDK] New Entitlement[%d] invalid : %d", IndexOptions.EntitlementIndex, CopyResult);
					break;
				}
			}

			if (FGame::Get().GetStore())
			{
				FGame::Get().GetStore()->SetEntitlements(CheckoutData->LocalUserId, std::move(NewEntitlements));
			}

			EOS_Ecom_Transaction_Release(TransactionHandle);
		}
	}

	std::wstring Msg = L"Checkout Successful";
	FGameEvent Event(EGameEventType::AddNotification, Msg);
	FGame::Get().OnGameEvent(Event);
	FGame::Get().GetStore()->OnCheckoutSuccessful();
}
