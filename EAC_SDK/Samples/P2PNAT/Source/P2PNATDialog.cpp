// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "Game.h"
#include "Menu.h"
#include "TextLabel.h"
#include "Button.h"
#include "UIEvent.h"
#include "GameEvent.h"
#include "AccountHelpers.h"
#include "Player.h"
#include "Sprite.h"
#include "P2PNATDialog.h"
#include "P2PNAT.h"

FP2PNATDialog::FP2PNATDialog(
	Vector2 DialogPos,
	Vector2 DialogSize,
	UILayer DialogLayer,
	FontPtr DialogNormalFont,
	FontPtr DialogSmallFont,
	FontPtr DialogTinyFont,
	FontPtr DialogTitleFont) :
	FDialog(DialogPos, DialogSize, DialogLayer)
{
	HeaderLabel = std::make_shared<FTextLabelWidget>(
		DialogPos,
		Vector2(DialogSize.x, 25.0f),
		Layer - 1,
		std::wstring(L"Header"),
		L"Assets/wide_label.dds",
		FColor(1.f, 1.f, 1.f, 1.f),
		FColor(1.f, 1.f, 1.f, 1.f),
		EAlignmentType::Left
		);
	HeaderLabel->SetFont(DialogNormalFont);

	BackgroundImage = std::make_shared<FSpriteWidget>(
		DialogPos,
		DialogSize,
		DialogLayer,
		L"Assets/texteditor.dds");

	NATStatusLabel = std::make_shared<FTextLabelWidget>(
		DialogPos + Vector2(HeaderLabel->GetSize().x + HeaderLabel->GetSize().x - 170.0f, 0.0f),
		Vector2(170.0f, 25.0f),
		Layer - 1,
		L"NAT Status: Unknown",
		L"",
		FColor(1.f, 1.f, 1.f, 1.f),
		FColor(1.f, 1.f, 1.f, 1.f),
		EAlignmentType::Left
		);
	NATStatusLabel->SetFont(DialogNormalFont);

	Vector2 ChatInputFieldSize = Vector2(DialogSize.x, 30.0f);
	Vector2 ChatInputFieldPos = DialogPos + Vector2(0.0f, DialogSize.y) - Vector2(0.0f, ChatInputFieldSize.y);

	ChatInputField = std::make_shared<FTextFieldWidget>(
		ChatInputFieldPos,
		ChatInputFieldSize,
		Layer - 1,
		L"Chat...",
		L"Assets/textfield.dds",
		DialogNormalFont,
		FTextFieldWidget::EInputType::Normal,
		EAlignmentType::Left);
	ChatInputField->SetBorderColor(Color::UIBorderGrey);

	ChatInputField->SetOnEnterPressedCallback(
		[this](const std::wstring& Value)
	{
		this->OnSendMessage(Value);
	}
	);

	Vector2 ChatTextViewPos = HeaderLabel->GetPosition() + Vector2(0.0f, HeaderLabel->GetSize().y + 5.0f);
	Vector2 ChatTextViewSize = Vector2(DialogSize.x, DialogSize.y - ChatInputFieldSize.y - HeaderLabel->GetSize().y - 10.0f);

	ChatTextView = std::make_shared<FTextViewWidget>(
		ChatTextViewPos,
		ChatTextViewSize,
		Layer - 1,
		L"Choose friend to start chat...",
		L"Assets/textfield.dds",
		DialogNormalFont);

	ChatTextView->SetBorderOffsets(Vector2(10.0f, 10.0f));
	ChatTextView->SetScrollerOffsets(Vector2(5.f, 5.0f));
	ChatTextView->SetFont(DialogNormalFont);

	ChatInfoLabel = std::make_shared<FTextLabelWidget>(
		ChatTextViewPos,
		ChatTextViewSize,
		Layer - 2,
		L"",
		L"",
		FColor(1.f, 1.f, 1.f, 1.f),
		FColor(1.f, 1.f, 1.f, 1.f),
		EAlignmentType::Center
		);
	ChatInfoLabel->SetFont(DialogTitleFont);

	std::vector<std::wstring> EmptyButtonAssets = {};
	Vector2 CloseChatButtonSize = Vector2(150.0f, 35.0f);
	CloseCurrentChatButton = std::make_shared<FButtonWidget>(
		ChatTextViewPos + Vector2(ChatTextViewSize.x - CloseChatButtonSize.x - 10.0f, 10.0f),
		CloseChatButtonSize,
		Layer - 2,
		L"CLOSE CHAT",
		std::vector<std::wstring>({ L"Assets/solid_white.dds" }),
		DialogNormalFont,
		Color::UIHeaderGrey);
	CloseCurrentChatButton->SetFont(DialogNormalFont);
	CloseCurrentChatButton->SetBorderColor(Color::UIBorderGrey);
	CloseCurrentChatButton->SetOnPressedCallback([]()
	{
		static_cast<FMenu&>(*FGame::Get().GetMenu()).GetP2PNATDialog()->CloseCurrentChat();
	});
	CloseCurrentChatButton->Hide();
}

void FP2PNATDialog::Update()
{
	FDialog::Update();

	//Update header label
	if (HeaderLabel)
	{
		HeaderLabel->SetText(L"CHAT");
		if (FPlayerManager::Get().GetCurrentUser().IsValid())
		{
			if (CurrentChat.IsValid())
			{
				HeaderLabel->SetText(std::wstring(L"Chatting with ") + FGame::Get().GetFriends()->GetFriendName(CurrentChat));
			}
		}
	}

	//Update info label
	if (ChatInfoLabel)
	{
		if (!FPlayerManager::Get().GetCurrentUser().IsValid())
		{
			ChatInfoLabel->SetText(L"Log in to proceed");
		}
		else if (!CurrentChat.IsValid())
		{
			ChatInfoLabel->SetText(L"Select a friend to start chatting");
		}
		else
		{
			ChatInfoLabel->SetText(L"");
		}
	}

	//Update NAT Status
	if (NATStatusLabel && FPlayerManager::Get().GetCurrentUser().IsValid())
	{
		EOS_ENATType NATType = FGame::Get().GetP2PNAT()->GetNATType();
		std::wstring NATString = L"?";
		FColor NATColor = Color::White;
		switch (NATType)
		{
		case EOS_ENATType::EOS_NAT_Unknown:
			NATString = L"Unknown";
			NATColor = Color::White;
			break;
		case EOS_ENATType::EOS_NAT_Open:
			NATString = L"Open";
			NATColor = Color::Green;
			break;
		case EOS_ENATType::EOS_NAT_Moderate:
			NATString = L"Moderate";
			NATColor = Color::Yellow;
			break;
		case EOS_ENATType::EOS_NAT_Strict:
			NATString = L"Strict";
			NATColor = Color::Red;
			break;
		}

		NATStatusLabel->SetText(std::wstring(L"NAT Status: ") + NATString);
		NATStatusLabel->SetTextColor(NATColor);
	}

	//Update chat view
	if (ChatTextView && bChatViewDirty)
	{
		bChatViewDirty = false;

		ChatTextView->Clear(true);

		FChatWithFriendData& Chat = GetChat(CurrentChat);
		for (const auto& Entry : Chat.ChatLines)
		{
			bool bSelf = Entry.first;
			ChatTextView->AddLine(Entry.second, (bSelf) ? Color::Gray : Color::White);
		}
		ChatTextView->ScrollToBottom();
	}

	//Update close chat button
	if (CloseCurrentChatButton)
	{
		if (CurrentChat.IsValid() && !CloseCurrentChatButton->IsShown())
		{
			CloseCurrentChatButton->Show();
		}
		else if (!CurrentChat.IsValid() && CloseCurrentChatButton->IsShown())
		{
			CloseCurrentChatButton->Hide();
		}
	}
}

void FP2PNATDialog::Create()
{
	if (HeaderLabel) HeaderLabel->Create();
	if (BackgroundImage) BackgroundImage->Create();
	if (NATStatusLabel) NATStatusLabel->Create();
	if (ChatTextView)	ChatTextView->Create();
	if (ChatInputField)	ChatTextView->Create();
	if (ChatInfoLabel) ChatInfoLabel->Create();
	if (CloseCurrentChatButton) CloseCurrentChatButton->Create();

	AddWidget(HeaderLabel);
	AddWidget(BackgroundImage);
	AddWidget(NATStatusLabel);
	AddWidget(ChatTextView);
	AddWidget(ChatInputField);
	AddWidget(ChatInfoLabel);
	AddWidget(CloseCurrentChatButton);
}

void FP2PNATDialog::SetPosition(Vector2 Pos)
{
	IWidget::SetPosition(Pos);

	if (HeaderLabel) HeaderLabel->SetPosition(Pos);
	if (BackgroundImage) BackgroundImage->SetPosition(Vector2(Position.x, Position.y + 20.f));
	if (NATStatusLabel) NATStatusLabel->SetPosition(HeaderLabel->GetPosition() + Vector2(HeaderLabel->GetSize().x - NATStatusLabel->GetSize().x, 0.0f));
	if (ChatTextView)	ChatTextView->SetPosition(HeaderLabel->GetPosition() + Vector2(0.0f, HeaderLabel->GetSize().y + 5.0f));
	if (ChatInputField)	ChatInputField->SetPosition(Position + Vector2(0.0f, Size.y) - Vector2(0.0f, ChatInputField->GetSize().y));
	if (ChatInfoLabel) ChatInfoLabel->SetPosition(ChatTextView->GetPosition());

	Vector2 CloseChatButtonSize = Vector2(150.0f, 35.0f);
	if (CloseCurrentChatButton) CloseCurrentChatButton->SetPosition(ChatTextView->GetPosition() + Vector2(ChatTextView->GetSize().x - CloseChatButtonSize.x - 10.0f, 10.0f));
}

void FP2PNATDialog::SetSize(Vector2 NewSize)
{
	IWidget::SetSize(NewSize);

	if (HeaderLabel) HeaderLabel->SetSize(Vector2(NewSize.x, 25.0f));
	if (BackgroundImage) BackgroundImage->SetSize(NewSize);
	if (NATStatusLabel) NATStatusLabel->SetSize(Vector2(170.0f, 25.0f));

	Vector2 ChatInputFieldSize = Vector2(NewSize.x, 30.0f);
	Vector2 ChatTextViewSize = Vector2(NewSize.x, NewSize.y - ChatInputFieldSize.y - HeaderLabel->GetSize().y - 10.0f);

	if (ChatTextView)	ChatTextView->SetSize(ChatTextViewSize);
	if (ChatInputField)	ChatInputField->SetSize(ChatInputFieldSize);
	if (ChatInfoLabel) ChatInfoLabel->SetSize(ChatTextViewSize);
	Vector2 CloseChatButtonSize = Vector2(150.0f, 35.0f);
	if (CloseCurrentChatButton) CloseCurrentChatButton->SetSize(CloseChatButtonSize);
}


void FP2PNATDialog::OnUIEvent(const FUIEvent& Event)
{
	FDialog::OnUIEvent(Event);

	//We want to always pass mouse release event to text view to make sure it will never get stuck in "text selection" mode.
	if (ChatTextView && Event.GetType() == EUIEventType::MouseReleased)
	{
		ChatTextView->OnUIEvent(Event);
	}
}

void FP2PNATDialog::UpdateNATStatus()
{
	if (NATStatusLabel)
	{
		FGame::Get().GetP2PNAT()->RefreshNATType();
	}
}

void FP2PNATDialog::OnGameEvent(const FGameEvent& Event)
{
	if (Event.GetType() == EGameEventType::UserLoggedIn)
	{
		UpdateNATStatus();
	}
	else if (Event.GetType() == EGameEventType::UserLoginRequiresMFA)
	{
		UpdateNATStatus();
		SetFocused(false);
	}
	else if (Event.GetType() == EGameEventType::UserLoginEnteredMFA)
	{
		UpdateNATStatus();
	}
	else if (Event.GetType() == EGameEventType::UserLoggedOut)
	{
		UpdateNATStatus();
	}
	else if (Event.GetType() == EGameEventType::ShowPrevUser)
	{
		UpdateNATStatus();
	}
	else if (Event.GetType() == EGameEventType::ShowNextUser)
	{
		UpdateNATStatus();
	}
	else if (Event.GetType() == EGameEventType::NewUserLogin)
	{
		UpdateNATStatus();
	}
	else if (Event.GetType() == EGameEventType::CancelLogin)
	{
		UpdateNATStatus();
	}
	else if (Event.GetType() == EGameEventType::StartChatWithFriend)
	{
		// Open chat with friend
		CurrentChat = Event.GetProductUserId();

		bChatViewDirty = true;
		ChatTextView->Clear();
	}
}

FChatWithFriendData& FP2PNATDialog::GetChat(FProductUserId FriendId)
{
	auto Iter = std::find_if(ChatData.begin(), ChatData.end(), [FriendId](const FChatWithFriendData& Next) { return Next.FriendId == FriendId; });
	if (Iter != ChatData.end())
	{
		return *Iter;
	}

	ChatData.push_back(FChatWithFriendData(FriendId));
	return ChatData.back();
}

void FP2PNATDialog::CloseCurrentChat()
{
	CurrentChat = FProductUserId();

	if (ChatTextView)
	{
		ChatTextView->Clear();
	}
}

void FP2PNATDialog::OnSendMessage(const std::wstring& Value)
{
	if (CurrentChat.IsValid())
	{
		FChatWithFriendData& Chat = GetChat(CurrentChat);

		std::vector<std::wstring> NewLines;
		SplitMessageIntoLines(Value, NewLines);
		for (const std::wstring& NextLine : NewLines)
		{
			if (!NextLine.empty())
			{
				Chat.ChatLines.push_back(std::make_pair(true, NextLine));
			}
		}
		
		bChatViewDirty = true;

		FGame::Get().GetP2PNAT()->SendMessage(CurrentChat, Value);
	}

	ChatInputField->Clear();
}

void FP2PNATDialog::OnMessageReceived(const std::wstring& Message, FProductUserId FriendId)
{
	if (FriendId.IsValid())
	{
		FChatWithFriendData& Chat = GetChat(FriendId);

		std::vector<std::wstring> NewLines;
		SplitMessageIntoLines(Message, NewLines);
		for (const std::wstring& NextLine : NewLines)
		{
			if (!NextLine.empty())
			{
				Chat.ChatLines.push_back(std::make_pair(false, NextLine));
			}
		}

		if (CurrentChat == FriendId)
		{
			bChatViewDirty = true;
		}

		if (!CurrentChat.IsValid())
		{
			CurrentChat = FriendId;
			bChatViewDirty = true;
		}
	}
}

void FP2PNATDialog::SplitMessageIntoLines(const std::wstring& Message, std::vector<std::wstring>& OutLines)
{
	//In case the message is multi-line (e.g. because of pasting) we want to split it
	std::wstring::size_type TokenizeStartPos = 0;
	std::wstring::size_type TokenizeEndPos = Message.find_first_of(L'\n', TokenizeStartPos);

	while (TokenizeStartPos != std::wstring::npos && TokenizeEndPos != std::wstring::npos)
	{
		OutLines.push_back(Message.substr(TokenizeStartPos, TokenizeEndPos - TokenizeStartPos));
		TokenizeStartPos = Message.find_first_of(L'\n', TokenizeEndPos) + 1;
		TokenizeEndPos = Message.find_first_of(L'\n', TokenizeStartPos);
	}

	OutLines.push_back(Message.substr(TokenizeStartPos));
}
