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
#include "TitleStorage.h"

const size_t MaxChunkSize = 4 * 4 * 4096;

FTitleStorage::FTitleStorage()
{
}

FTitleStorage::~FTitleStorage()
{

}

void FTitleStorage::QueryList()
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player == nullptr || !Player->GetProductUserID().IsValid())
	{
		return;
	}

	if (CurrentTags.empty())
	{
		FGameEvent PopupEvent(EGameEventType::ShowPopupDialog, L"Please enter at least one tag and press 'Add tag'.");
		FGame::Get().OnGameEvent(PopupEvent);
		return;
	}

	EOS_HTitleStorage TitleStorageHandle = EOS_Platform_GetTitleStorageInterface(FPlatform::GetPlatformHandle());
	EOS_TitleStorage_QueryFileListOptions Options = {};
	Options.ApiVersion = EOS_TITLESTORAGE_QUERYFILELIST_API_LATEST;
	Options.LocalUserId = Player->GetProductUserID();
	
	Options.ListOfTagsCount = int32_t(CurrentTags.size());
	std::vector<const char*> Tags;
	for (const std::string& NextTag : CurrentTags)
	{
		Tags.push_back(NextTag.c_str());
	}
	Options.ListOfTags = Tags.data();

	EOS_TitleStorage_QueryFileList(TitleStorageHandle, &Options, nullptr, OnFileListRetrieved);
}

void FTitleStorage::StartFileDataDownload(const std::wstring& FileName)
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player == nullptr || !Player->GetProductUserID().IsValid())
	{
		return;
	}

	//TODO: make sure we are not transferring the same file atm
	EOS_HTitleStorage TitleStorageHandle = EOS_Platform_GetTitleStorageInterface(FPlatform::GetPlatformHandle());
	EOS_TitleStorage_ReadFileOptions Options = {};
	Options.ApiVersion = EOS_TITLESTORAGE_READFILE_API_LATEST;
	Options.LocalUserId = Player->GetProductUserID();
	std::string NarrowFileName = FStringUtils::Narrow(FileName);
	Options.Filename = NarrowFileName.c_str();
	Options.ReadChunkLengthBytes = MaxChunkSize;

	Options.ReadFileDataCallback = OnFileDataReceived;
	Options.FileTransferProgressCallback = OnFileTransferProgressUpdated;

	EOS_HTitleStorageFileTransferRequest Handle = EOS_TitleStorage_ReadFile(TitleStorageHandle, &Options, nullptr, OnFileReceived);
	if (!Handle)
	{
		FDebugLog::LogError(L"[EOS SDK] Title storage: can't start file download, bad handle returned '%ls'", FileName.c_str());
		return;
	}

	CancelCurrentTransfer();
	CurrentTransferHandle = Handle;

	FTransferInProgress NewTransfer;
	NewTransfer.bDownload = true;
	
	//Total file size will be set on first update

	TransfersInProgress[FileName] = NewTransfer;

	CurrentTransferProgress = 0.0f;
	CurrentTransferName = FileName;

	//Show dialog
	FGameEvent GameEvent(EGameEventType::FileTransferStarted, CurrentTransferName);
	FGame::Get().OnGameEvent(GameEvent);
}

std::vector<std::wstring> FTitleStorage::GetFileList() const
{
	std::vector<std::wstring> Names;
	Names.reserve(StorageData.size());

	for (auto& Entry : StorageData)
	{
		Names.push_back(Entry.first);
	}

	return Names;
}

void FTitleStorage::SetFileList(const std::vector<std::wstring>& FileNames)
{

	for (const std::wstring& NextName : FileNames)
	{
		auto Iter = StorageData.find(NextName);
		if (Iter == StorageData.end())
		{
			//data will be retrieved when requested explicitly
			StorageData[NextName] = std::make_pair<bool, std::wstring>(false, L"");
		}
	}

	//we need to remove files that are gone
	if (StorageData.size() != FileNames.size())
	{
		std::unordered_map<std::wstring, std::pair<bool, std::wstring>> NewStorageData;

		for (const std::wstring& NextName : FileNames)
		{
			NewStorageData[NextName] = StorageData[NextName];
		}

		StorageData.swap(NewStorageData);
	}
}

std::wstring FTitleStorage::GetLocalData(const std::wstring& EntryName, bool& NoData) const
{
	auto Iter = StorageData.find(EntryName);
	if (Iter != StorageData.end())
	{
		NoData = !Iter->second.first;
		return Iter->second.second;
	}

	NoData = true;
	return L"";
}

void FTitleStorage::CancelCurrentTransfer()
{
	if (CurrentTransferHandle)
	{
		auto Result = EOS_TitleStorageFileTransferRequest_CancelRequest(CurrentTransferHandle);
		EOS_TitleStorageFileTransferRequest_Release(CurrentTransferHandle);
		CurrentTransferHandle = nullptr;

		if (Result == EOS_EResult::EOS_Success)
		{
			//canceled with success
			auto Iter = TransfersInProgress.find(CurrentTransferName);
			if (Iter != TransfersInProgress.end())
			{
				FTransferInProgress& Transfer = Iter->second;
				if (Transfer.bDownload)
				{
					//Download is canceled - do nothing
				}
				else
				{
					//Upload is canceled - do nothing
				}

				TransfersInProgress.erase(Iter);
			}

			//Hide dialog
			FGameEvent GameEvent(EGameEventType::FileTransferFinished, CurrentTransferName);
			FGame::Get().OnGameEvent(GameEvent);
		}
	}

	ClearCurrentTransfer();
}


EOS_TitleStorage_EReadResult FTitleStorage::ReceiveData(const std::wstring& FileName, const void* Data, size_t NumBytes, size_t TotalSize)
{
	if (!Data)
	{
		FDebugLog::LogError(L"[EOS SDK] Title storage: could not receive data: Data pointer is null.");
		return EOS_TitleStorage_EReadResult::EOS_TS_RR_FailRequest;
	}

	auto Iter = TransfersInProgress.find(FileName);
	if (Iter != TransfersInProgress.end())
	{
		FTransferInProgress& Transfer = Iter->second;

		if (!Transfer.bDownload)
		{
			FDebugLog::LogError(L"[EOS SDK] Title storage: can't load file data: download/upload mismatch.");
			return EOS_TitleStorage_EReadResult::EOS_TS_RR_FailRequest;
		}

		//First update
		if (Transfer.CurrentIndex == 0 && Transfer.TotalSize == 0)
		{
			Transfer.TotalSize = TotalSize;

			if (Transfer.TotalSize == 0)
			{
				return EOS_TitleStorage_EReadResult::EOS_TS_RR_ContinueReading;
			}

			Transfer.Data.resize(TotalSize);
		}

		//Make sure we have enough space
		if (Transfer.TotalSize - Transfer.CurrentIndex >= NumBytes)
		{
			memcpy(static_cast<void*>(&Transfer.Data[Transfer.CurrentIndex]), Data, NumBytes);
			Transfer.CurrentIndex += NumBytes;

			return EOS_TitleStorage_EReadResult::EOS_TS_RR_ContinueReading;
		}
		else
		{
			FDebugLog::LogError(L"[EOS SDK] Title storage: could not receive data: too much of it.");
			return EOS_TitleStorage_EReadResult::EOS_TS_RR_FailRequest;
		}
	}

	return EOS_TitleStorage_EReadResult::EOS_TS_RR_CancelRequest;
}

void FTitleStorage::UpdateProgress(const std::wstring& FileName, float Progress)
{
	// Make sure the update is for our primary (current) transfer.
	if (FileName == CurrentTransferName)
	{
		CurrentTransferProgress = Progress;
	}
}

void FTitleStorage::FinishFileDownload(const std::wstring& FileName, bool bSuccess)
{
	auto Iter = TransfersInProgress.find(FileName);
	if (Iter != TransfersInProgress.end())
	{
		FTransferInProgress& Transfer = Iter->second;

		if (!Transfer.bDownload)
		{
			FDebugLog::LogError(L"[EOS SDK] Title storage: error while file read operation: can't finish because of download/upload mismatch.");
			return;
		}

		if (!Transfer.Done() || !bSuccess)
		{
			if (!Transfer.Done())
			{
				FDebugLog::LogError(L"[EOS SDK] Title storage: error while file read operation: expecting more data. File can be corrupted.");
			}
			TransfersInProgress.erase(Iter);
			if (FileName == CurrentTransferName)
			{
				ClearCurrentTransfer();
			}
			return;
		}

		std::string NarrowFileData;
		//Don't try to show files larger than 5 Mb (it will cause UI performance issues)
		if (Transfer.TotalSize > 5 * 1024 * 1024)
		{
			NarrowFileData = "*** File is too large to be viewed in this sample. ***";
		}
		else
		{
			NarrowFileData = (Transfer.TotalSize > 0) ? std::string(&Transfer.Data[0], Transfer.TotalSize) : std::string();
		}

		std::wstring WideFileData;
		//Data can be binary or corrupted.
		try
		{
			WideFileData = FStringUtils::Widen(NarrowFileData);
		}
		catch (...)
		{
			WideFileData = L"*** File data contains binary data that can't be viewed. ***";
		}
		StorageData[FileName] = std::make_pair<bool, std::wstring>(true, std::move(WideFileData));

		FDebugLog::Log(L"[EOS SDK] Title storage: file read finished: '%ls' Size: %d.", FileName.c_str(), NarrowFileData.size());

		TransfersInProgress.erase(Iter);

		FGameEvent GameEvent(EGameEventType::FileTransferFinished, CurrentTransferName);
		FGame::Get().OnGameEvent(GameEvent);

		if (FileName == CurrentTransferName)
		{
			ClearCurrentTransfer();
		}
	}
}

void FTitleStorage::Update()
{

}

void FTitleStorage::OnLoggedIn(FProductUserId UserId)
{

}

void FTitleStorage::OnLoggedOut(FProductUserId UserId)
{
	StorageData.clear();
	ClearCurrentTransfer();
}

void FTitleStorage::ClearCurrentTransfer()
{
	CurrentTransferName.clear();
	CurrentTransferProgress = 0.0f;

	if (CurrentTransferHandle)
	{
		EOS_TitleStorageFileTransferRequest_Release(CurrentTransferHandle);
		CurrentTransferHandle = nullptr;
	}
}

void FTitleStorage::OnGameEvent(const FGameEvent& Event)
{
	if (Event.GetType() == EGameEventType::UserConnectLoggedIn)
	{
		FProductUserId UserId = Event.GetProductUserId();
		OnLoggedIn(UserId);
	}
	else if (Event.GetType() == EGameEventType::UserConnectAuthExpiration)
	{
		FProductUserId UserId = Event.GetProductUserId();
		OnLoggedOut(UserId);
	}
}


void EOS_CALL FTitleStorage::OnFileListRetrieved(const EOS_TitleStorage_QueryFileListCallbackInfo* Data)
{
	if (Data)
	{
		if (Data->ResultCode == EOS_EResult::EOS_Success)
		{
			PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
			if (Player == nullptr || !Player->GetProductUserID().IsValid())
			{
				return;
			}

			FDebugLog::Log(L"[EOS SDK] Title storage file list is successfully retrieved.");

			const size_t FileCount = Data->FileCount;
			std::vector<std::wstring> FileNames;
			EOS_HTitleStorage TitleStorageHandle = EOS_Platform_GetTitleStorageInterface(FPlatform::GetPlatformHandle());
			for (size_t FileIndex = 0; FileIndex < FileCount; ++FileIndex)
			{
				EOS_TitleStorage_CopyFileMetadataAtIndexOptions Options = {};
				Options.ApiVersion = EOS_TITLESTORAGE_COPYFILEMETADATAATINDEX_API_LATEST;

				Options.LocalUserId = Player->GetProductUserID();
				Options.Index = static_cast<uint32_t>(FileIndex);

				EOS_TitleStorage_FileMetadata* FileMetadata = nullptr;

				EOS_TitleStorage_CopyFileMetadataAtIndex(TitleStorageHandle, &Options, &FileMetadata);

				if (FileMetadata)
				{
					if (FileMetadata->Filename)
					{
						std::wstring FileName = FStringUtils::Widen(FileMetadata->Filename);

						FileNames.push_back(FileName);
					}

					EOS_TitleStorage_FileMetadata_Release(FileMetadata);
				}
			}

			FGame::Get().GetTitleStorage()->SetFileList(FileNames);
		}
		else
		{
			FDebugLog::LogError(L"[EOS SDK] Title storage: file list retrieval error: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
	}
}



EOS_TitleStorage_EReadResult EOS_CALL FTitleStorage::OnFileDataReceived(const EOS_TitleStorage_ReadFileDataCallbackInfo* Data)
{
	if (Data)
	{
		return FGame::Get().GetTitleStorage()->ReceiveData(FStringUtils::Widen(Data->Filename), Data->DataChunk, Data->DataChunkLengthBytes, Data->TotalFileSizeBytes);
	}
	return EOS_TitleStorage_EReadResult::EOS_TS_RR_FailRequest;
}

void EOS_CALL FTitleStorage::OnFileReceived(const EOS_TitleStorage_ReadFileCallbackInfo* Data)
{
	if (Data)
	{
		if (Data->ResultCode == EOS_EResult::EOS_Success)
		{
			FGame::Get().GetTitleStorage()->FinishFileDownload(FStringUtils::Widen(Data->Filename), true);
		}
		else
		{
			FDebugLog::LogError(L"[EOS SDK] Title storage: could not download file: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
			FGame::Get().GetTitleStorage()->FinishFileDownload(FStringUtils::Widen(Data->Filename), false);
		}
	}
}


void EOS_CALL FTitleStorage::OnFileTransferProgressUpdated(const EOS_TitleStorage_FileTransferProgressCallbackInfo* Data)
{
	if (Data)
	{
		if (Data->TotalFileSizeBytes > 0)
		{
			FGame::Get().GetTitleStorage()->UpdateProgress(FStringUtils::Widen(Data->Filename), float(Data->BytesTransferred) / Data->TotalFileSizeBytes);
			FDebugLog::Log(L"[EOS SDK] Title storage: transfer progress %d / %d.", Data->BytesTransferred, Data->TotalFileSizeBytes);
		}
	}
}
