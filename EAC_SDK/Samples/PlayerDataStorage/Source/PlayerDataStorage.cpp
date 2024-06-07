// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "DebugLog.h"
#include "StringUtils.h"
#include "Utils.h"
#include "AccountHelpers.h"
#include "Game.h"
#include "GameEvent.h"
#include "Main.h"
#include "Platform.h"
#include "Users.h"
#include "Player.h"
#include "PlayerDataStorage.h"

const size_t MaxChunkSize = 4096;

FPlayerDataStorage::FPlayerDataStorage()
{
}

FPlayerDataStorage::~FPlayerDataStorage()
{

}

void FPlayerDataStorage::QueryList()
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player == nullptr || !Player->GetProductUserID().IsValid())
	{
		return;
	}

	EOS_HPlayerDataStorage PlayerStorageHandle = EOS_Platform_GetPlayerDataStorageInterface(FPlatform::GetPlatformHandle());
	EOS_PlayerDataStorage_QueryFileListOptions Options = {};
	Options.ApiVersion = EOS_PLAYERDATASTORAGE_QUERYFILELIST_API_LATEST;
	Options.LocalUserId = Player->GetProductUserID();
	EOS_PlayerDataStorage_QueryFileList(PlayerStorageHandle, &Options, nullptr, OnFileListRetrieved);
}

void FPlayerDataStorage::StartFileDataDownload(const std::wstring& FileName)
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player == nullptr || !Player->GetProductUserID().IsValid())
	{
		return;
	}

	//TODO: make sure we are not transferring the same file atm
	EOS_HPlayerDataStorage PlayerStorageHandle = EOS_Platform_GetPlayerDataStorageInterface(FPlatform::GetPlatformHandle());
	EOS_PlayerDataStorage_ReadFileOptions Options = {};
	Options.ApiVersion = EOS_PLAYERDATASTORAGE_READFILE_API_LATEST;
	Options.LocalUserId = Player->GetProductUserID();
	std::string NarrowFileName = FStringUtils::Narrow(FileName);
	Options.Filename = NarrowFileName.c_str();
	Options.ReadChunkLengthBytes = MaxChunkSize;

	Options.ReadFileDataCallback = OnFileDataReceived;
	Options.FileTransferProgressCallback = OnFileTransferProgressUpdated;

	EOS_HPlayerDataStorageFileTransferRequest Handle = EOS_PlayerDataStorage_ReadFile(PlayerStorageHandle, &Options, nullptr, OnFileReceived);
	if (!Handle)
	{
		FDebugLog::LogError(L"[EOS SDK] Player data storage: can't start file download, bad handle returned '%ls'", FileName.c_str());
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

bool FPlayerDataStorage::StartFileDataUpload(const std::wstring& FileName)
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player == nullptr || !Player->GetProductUserID().IsValid())
	{
		return false;
	}

	//TODO: make sure we are not transferring the same file atm

	auto Iter = StorageData.find(FileName);
	const std::wstring& FileData = Iter->second.second;
	
	EOS_HPlayerDataStorage PlayerStorageHandle = EOS_Platform_GetPlayerDataStorageInterface(FPlatform::GetPlatformHandle());
	EOS_PlayerDataStorage_WriteFileOptions Options = {};
	Options.ApiVersion = EOS_PLAYERDATASTORAGE_WRITEFILE_API_LATEST;
	Options.LocalUserId = Player->GetProductUserID();
	std::string NarrowFileName = FStringUtils::Narrow(FileName);
	Options.Filename = NarrowFileName.c_str();
	Options.ChunkLengthBytes = MaxChunkSize;
	Options.WriteFileDataCallback = OnFileDataSend;
	Options.FileTransferProgressCallback = OnFileTransferProgressUpdated;

	EOS_HPlayerDataStorageFileTransferRequest Handle = EOS_PlayerDataStorage_WriteFile(PlayerStorageHandle, &Options, nullptr, OnFileSent);
	if(!Handle)
	{
		FDebugLog::LogError(L"[EOS SDK] Player data storage: can't start file upload, bad handle returned '%ls'", FileName.c_str());
		return false;
	}

	CancelCurrentTransfer();
	CurrentTransferHandle = Handle;

	FTransferInProgress NewTransfer;
	NewTransfer.bDownload = false;

	std::string NarrowFileData = FStringUtils::Narrow(FileData);
	NewTransfer.TotalSize = NarrowFileData.size();
	NewTransfer.Data.resize(NarrowFileData.size());
	if (NewTransfer.TotalSize > 0)
	{
		memcpy(static_cast<void*>(&NewTransfer.Data[0]), static_cast<const void*>(&NarrowFileData[0]), NarrowFileData.size());
	}
	NewTransfer.CurrentIndex = 0;

	TransfersInProgress[FileName] = NewTransfer;

	CurrentTransferProgress = 0.0f;
	CurrentTransferName = FileName;

	//Show dialog
	FGameEvent GameEvent(EGameEventType::FileTransferStarted, CurrentTransferName);
	FGame::Get().OnGameEvent(GameEvent);

	return true;
}

std::vector<std::wstring> FPlayerDataStorage::GetFileList() const
{
	std::vector<std::wstring> Names;
	Names.reserve(StorageData.size());

	for (auto& Entry : StorageData)
	{
		Names.push_back(Entry.first);
	}

	return Names;
}

void FPlayerDataStorage::SetFileList(const std::vector<std::wstring>& FileNames)
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

void FPlayerDataStorage::AddFile(const std::wstring& EntryName, const std::wstring& Data)
{
	SetLocalData(EntryName, Data);
	if (!StartFileDataUpload(EntryName))
	{
		EraseLocalData(EntryName);
	}
}

void FPlayerDataStorage::CopyFile(const std::wstring& SourceFileName, const std::wstring& DestinationFileName)
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player == nullptr || !Player->GetProductUserID().IsValid())
	{
		return;
	}

	EOS_HPlayerDataStorage PlayerStorageHandle = EOS_Platform_GetPlayerDataStorageInterface(FPlatform::GetPlatformHandle());
	EOS_PlayerDataStorage_DuplicateFileOptions Options = {};
	Options.ApiVersion = EOS_PLAYERDATASTORAGE_DUPLICATEFILE_API_LATEST;
	Options.LocalUserId = Player->GetProductUserID();
	std::string NarrowSourceFileName = FStringUtils::Narrow(SourceFileName);
	Options.SourceFilename = NarrowSourceFileName.c_str();

	std::string NarrowDestinationFileName = FStringUtils::Narrow(DestinationFileName);
	Options.DestinationFilename = NarrowDestinationFileName.c_str();

	EOS_PlayerDataStorage_DuplicateFile(PlayerStorageHandle, &Options, nullptr, OnFileCopied);
}

void FPlayerDataStorage::RemoveFile(const std::wstring& FileName)
{
	PlayerPtr Player = FPlayerManager::Get().GetPlayer(FPlayerManager::Get().GetCurrentUser());
	if (Player == nullptr || !Player->GetProductUserID().IsValid())
	{
		return;
	}

	//remove local data (if any)
	EraseLocalData(FileName);

	EOS_HPlayerDataStorage PlayerStorageHandle = EOS_Platform_GetPlayerDataStorageInterface(FPlatform::GetPlatformHandle());
	EOS_PlayerDataStorage_DeleteFileOptions Options = {};
	Options.ApiVersion = EOS_PLAYERDATASTORAGE_DELETEFILE_API_LATEST;
	Options.LocalUserId = Player->GetProductUserID();
	std::string NarrowFileName = FStringUtils::Narrow(FileName);
	Options.Filename = NarrowFileName.c_str();

	EOS_PlayerDataStorage_DeleteFile(PlayerStorageHandle, &Options, nullptr, OnFileRemoved);
}

std::wstring FPlayerDataStorage::GetLocalData(const std::wstring& EntryName, bool& NoData) const
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

void FPlayerDataStorage::SetLocalData(const std::wstring& EntryName, const std::wstring& Data)
{
	StorageData[EntryName] = std::make_pair<bool, std::wstring>(true, std::wstring(Data));
}

void FPlayerDataStorage::EraseLocalData(const std::wstring& EntryName)
{
	auto iter = StorageData.find(EntryName);
	if (iter != StorageData.end())
	{
		StorageData.erase(iter);
	}
}

void FPlayerDataStorage::CancelCurrentTransfer()
{
	if (CurrentTransferHandle)
	{
		auto Result = EOS_PlayerDataStorageFileTransferRequest_CancelRequest(CurrentTransferHandle);
		EOS_PlayerDataStorageFileTransferRequest_Release(CurrentTransferHandle);
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


EOS_PlayerDataStorage_EReadResult FPlayerDataStorage::ReceiveData(const std::wstring& FileName, const void* Data, size_t NumBytes, size_t TotalSize)
{
	if (!Data)
	{
		FDebugLog::LogError(L"[EOS SDK] Player data storage: could not receive data: Data pointer is null.");
		return EOS_PlayerDataStorage_EReadResult::EOS_RR_FailRequest;
	}

	auto Iter = TransfersInProgress.find(FileName);
	if (Iter != TransfersInProgress.end())
	{
		FTransferInProgress& Transfer = Iter->second;

		if (!Transfer.bDownload)
		{
			FDebugLog::LogError(L"[EOS SDK] Player data storage: can't load file data: download/upload mismatch.");
			return EOS_PlayerDataStorage_EReadResult::EOS_RR_FailRequest;
		}

		//First update
		if (Transfer.CurrentIndex == 0 && Transfer.TotalSize == 0)
		{
			Transfer.TotalSize = TotalSize;

			if (Transfer.TotalSize == 0)
			{
				return EOS_PlayerDataStorage_EReadResult::EOS_RR_ContinueReading;
			}

			Transfer.Data.resize(TotalSize);
		}

		//Make sure we have enough space
		if (Transfer.TotalSize - Transfer.CurrentIndex >= NumBytes)
		{
			memcpy(static_cast<void*>(&Transfer.Data[Transfer.CurrentIndex]), Data, NumBytes);
			Transfer.CurrentIndex += NumBytes;

			return EOS_PlayerDataStorage_EReadResult::EOS_RR_ContinueReading;
		}
		else
		{
			FDebugLog::LogError(L"[EOS SDK] Player data storage: could not receive data: too much of it.");
			return EOS_PlayerDataStorage_EReadResult::EOS_RR_FailRequest;
		}
	}

	return EOS_PlayerDataStorage_EReadResult::EOS_RR_CancelRequest;
}

EOS_PlayerDataStorage_EWriteResult FPlayerDataStorage::SendData(const std::wstring& FileName, void* Data, uint32_t* BytesWritten)
{
	if (!Data || !BytesWritten)
	{
		FDebugLog::LogError(L"[EOS SDK] Player data storage: could not send data: pointer is null.");
		return EOS_PlayerDataStorage_EWriteResult::EOS_WR_FailRequest;
	}

	auto Iter = TransfersInProgress.find(FileName);
	if (Iter != TransfersInProgress.end())
	{
		FTransferInProgress& Transfer = Iter->second;

		if (Transfer.bDownload)
		{
			FDebugLog::LogError(L"[EOS SDK] Player data storage: can't send file data: download/upload mismatch.");
			return EOS_PlayerDataStorage_EWriteResult::EOS_WR_FailRequest;
		}

		if (Transfer.Done())
		{
			*BytesWritten = 0;
			return EOS_PlayerDataStorage_EWriteResult::EOS_WR_CompleteRequest;
		}

		size_t BytesToWrite = std::min(MaxChunkSize, Transfer.TotalSize - Transfer.CurrentIndex);
		
		if (BytesToWrite > 0)
		{
			memcpy(Data, static_cast<const void*>(&Transfer.Data[Transfer.CurrentIndex]), BytesToWrite);
		}
		*BytesWritten = static_cast<uint32_t>(BytesToWrite);

		Transfer.CurrentIndex += BytesToWrite;

		if (Transfer.Done())
		{
			return EOS_PlayerDataStorage_EWriteResult::EOS_WR_CompleteRequest;
		}
		else
		{
			return EOS_PlayerDataStorage_EWriteResult::EOS_WR_ContinueWriting;
		}
	}
	else
	{
		FDebugLog::LogError(L"[EOS SDK] Player data storage: could not send data as this file is not being uploaded at the moment.");
		*BytesWritten = 0;
		return EOS_PlayerDataStorage_EWriteResult::EOS_WR_CancelRequest;
	}
}


void FPlayerDataStorage::UpdateProgress(const std::wstring& FileName, float Progress)
{
	// Make sure the update is for our primary (current) transfer.
	if (FileName == CurrentTransferName)
	{
		CurrentTransferProgress = Progress;
	}
}

void FPlayerDataStorage::FinishFileDownload(const std::wstring& FileName, bool bSuccess)
{
	auto Iter = TransfersInProgress.find(FileName);
	if (Iter != TransfersInProgress.end())
	{
		FTransferInProgress& Transfer = Iter->second;

		if (!Transfer.bDownload)
		{
			FDebugLog::LogError(L"[EOS SDK] Player data storage: error while file read operation: can't finish because of download/upload mismatch.");
			return;
		}

		if (!Transfer.Done() || !bSuccess)
		{
			if (!Transfer.Done())
			{
				FDebugLog::LogError(L"[EOS SDK] Player data storage: error while file read operation: expecting more data. File can be corrupted.");
			}
			TransfersInProgress.erase(Iter);
			if (FileName == CurrentTransferName)
			{
				ClearCurrentTransfer();
			}
			return;
		}

		std::string NarrowFileData;
		//Don't try to show files larger than 5 megs (it will cause UI performance issues)
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

		FDebugLog::Log(L"[EOS SDK] Player data storage: file read finished: '%ls' Size: %d.", FileName.c_str(), NarrowFileData.size());

		TransfersInProgress.erase(Iter);

		FGameEvent GameEvent(EGameEventType::FileTransferFinished, CurrentTransferName);
		FGame::Get().OnGameEvent(GameEvent);

		if (FileName == CurrentTransferName)
		{
			ClearCurrentTransfer();
		}
	}
}

void FPlayerDataStorage::FinishFileUpload(const std::wstring& FileName)
{
	auto Iter = TransfersInProgress.find(FileName);
	if (Iter != TransfersInProgress.end())
	{
		FTransferInProgress& Transfer = Iter->second;

		if (Transfer.bDownload)
		{
			FDebugLog::LogError(L"[EOS SDK] Player data storage: error while file write operation: can't finish because of download/upload mismatch.");
			return;
		}

		if (!Transfer.Done())
		{
			FDebugLog::LogError(L"[EOS SDK] Player data storage: error while file write operation: unexpected end of transfer.");
		}

		TransfersInProgress.erase(Iter);

		FGameEvent GameEvent(EGameEventType::FileTransferFinished, CurrentTransferName);
		FGame::Get().OnGameEvent(GameEvent);

		if (FileName == CurrentTransferName)
		{
			ClearCurrentTransfer();
		}
	}
}

void FPlayerDataStorage::Update()
{

}

void FPlayerDataStorage::OnLoggedIn(FProductUserId UserId)
{
	QueryList();
}

void FPlayerDataStorage::OnLoggedOut(FProductUserId UserId)
{
	StorageData.clear();
	ClearCurrentTransfer();
}

void FPlayerDataStorage::ClearCurrentTransfer()
{
	CurrentTransferName.clear();
	CurrentTransferProgress = 0.0f;

	if (CurrentTransferHandle)
	{
		EOS_PlayerDataStorageFileTransferRequest_Release(CurrentTransferHandle);
		CurrentTransferHandle = nullptr;
	}
}

void FPlayerDataStorage::OnGameEvent(const FGameEvent& Event)
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


void EOS_CALL FPlayerDataStorage::OnFileListRetrieved(const EOS_PlayerDataStorage_QueryFileListCallbackInfo* Data)
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

			FDebugLog::Log(L"[EOS SDK] Player data storage file list is successfully retrieved.");

			const size_t FileCount = Data->FileCount;
			std::vector<std::wstring> FileNames;
			EOS_HPlayerDataStorage PlayerStorageHandle = EOS_Platform_GetPlayerDataStorageInterface(FPlatform::GetPlatformHandle());
			for (size_t FileIndex = 0; FileIndex < FileCount; ++FileIndex)
			{
				EOS_PlayerDataStorage_CopyFileMetadataAtIndexOptions Options = {};
				Options.ApiVersion = EOS_PLAYERDATASTORAGE_COPYFILEMETADATAATINDEX_API_LATEST;

				Options.LocalUserId = Player->GetProductUserID();
				Options.Index = static_cast<uint32_t>(FileIndex);

				EOS_PlayerDataStorage_FileMetadata* FileMetadata = nullptr;

				EOS_PlayerDataStorage_CopyFileMetadataAtIndex(PlayerStorageHandle, &Options, &FileMetadata);

				if (FileMetadata)
				{
					if (FileMetadata->Filename)
					{
						std::wstring FileName = FStringUtils::Widen(FileMetadata->Filename);

						FileNames.push_back(FileName);

						if (FileMetadata->MD5Hash)
						{
							std::wstring MD5Hash = FStringUtils::Widen(FileMetadata->MD5Hash);

							std::wstring LastModifiedTimeStrW = L"Undefined";
							if (FileMetadata->LastModifiedTime != EOS_PLAYERDATASTORAGE_TIME_UNDEFINED)
							{
								LastModifiedTimeStrW = FUtils::ConvertUnixTimestampToUTCString(FileMetadata->LastModifiedTime);
							}

							FDebugLog::Log(L"[EOS SDK] File List Retrieved - Filename: %ls, FileSizeBytes: %u, UnencryptedDataSizeBytes: %u, MD5Hash: %ls, LastModifiedTime: %ls",
								FileName.c_str(), FileMetadata->FileSizeBytes, FileMetadata->UnencryptedDataSizeBytes, MD5Hash.c_str(), LastModifiedTimeStrW.c_str());
						}
					}

					EOS_PlayerDataStorage_FileMetadata_Release(FileMetadata);
				}
			}

			FGame::Get().GetPlayerDataStorage()->SetFileList(FileNames);
		}
		else
		{
			FDebugLog::LogError(L"[EOS SDK] Player data storage: file list retrieval error: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
	}
}



EOS_PlayerDataStorage_EReadResult EOS_CALL FPlayerDataStorage::OnFileDataReceived(const EOS_PlayerDataStorage_ReadFileDataCallbackInfo* Data)
{
	if (Data)
	{
		return FGame::Get().GetPlayerDataStorage()->ReceiveData(FStringUtils::Widen(Data->Filename), Data->DataChunk, Data->DataChunkLengthBytes, Data->TotalFileSizeBytes);
	}
	return EOS_PlayerDataStorage_EReadResult::EOS_RR_FailRequest;
}

void EOS_CALL FPlayerDataStorage::OnFileReceived(const EOS_PlayerDataStorage_ReadFileCallbackInfo* Data)
{
	if (Data)
	{
		if (Data->ResultCode == EOS_EResult::EOS_Success)
		{
			FGame::Get().GetPlayerDataStorage()->FinishFileDownload(FStringUtils::Widen(Data->Filename), true);
		}
		else
		{
			FDebugLog::LogError(L"[EOS SDK] Player data storage: could not download file: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
			FGame::Get().GetPlayerDataStorage()->FinishFileDownload(FStringUtils::Widen(Data->Filename), false);
		}
	}
}

EOS_PlayerDataStorage_EWriteResult EOS_CALL FPlayerDataStorage::OnFileDataSend(const EOS_PlayerDataStorage_WriteFileDataCallbackInfo* Data, void* OutDataBuffer, uint32_t* OutDataWritten)
{
	if (Data)
	{
		return FGame::Get().GetPlayerDataStorage()->SendData(FStringUtils::Widen(Data->Filename), OutDataBuffer, OutDataWritten);
	}
	return EOS_PlayerDataStorage_EWriteResult::EOS_WR_FailRequest;
}

void EOS_CALL FPlayerDataStorage::OnFileSent(const EOS_PlayerDataStorage_WriteFileCallbackInfo* Data)
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

			EOS_HPlayerDataStorage PlayerStorageHandle = EOS_Platform_GetPlayerDataStorageInterface(FPlatform::GetPlatformHandle());
			EOS_PlayerDataStorage_CopyFileMetadataByFilenameOptions Options = {};
			Options.ApiVersion = EOS_PLAYERDATASTORAGE_COPYFILEMETADATABYFILENAMEOPTIONS_API_LATEST;
			Options.LocalUserId = Player->GetProductUserID();
			Options.Filename = Data->Filename;

			EOS_PlayerDataStorage_FileMetadata* FileMetadata = nullptr;
			EOS_PlayerDataStorage_CopyFileMetadataByFilename(PlayerStorageHandle, &Options, &FileMetadata);

			if (FileMetadata)
			{
				if (FileMetadata->Filename && FileMetadata->MD5Hash)
				{
					std::wstring FileName = FStringUtils::Widen(FileMetadata->Filename);
					std::wstring MD5Hash = FStringUtils::Widen(FileMetadata->MD5Hash);

					std::wstring LastModifiedTimeStrW = L"Undefined";
					if (FileMetadata->LastModifiedTime != EOS_PLAYERDATASTORAGE_TIME_UNDEFINED)
					{
						LastModifiedTimeStrW = FUtils::ConvertUnixTimestampToUTCString(FileMetadata->LastModifiedTime);
					}

					FDebugLog::Log(L"[EOS SDK] File Sent - Filename: %ls, FileSizeBytes: %u, UnencryptedDataSizeBytes: %u, MD5Hash: %ls, LastModifiedTime: %ls",
						FileName.c_str(), FileMetadata->FileSizeBytes, FileMetadata->UnencryptedDataSizeBytes, MD5Hash.c_str(), LastModifiedTimeStrW.c_str());
				}

				EOS_PlayerDataStorage_FileMetadata_Release(FileMetadata);
			}
		}
		else
		{
			FDebugLog::LogError(L"[EOS SDK] Player data storage: could not upload file: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		FGame::Get().GetPlayerDataStorage()->FinishFileUpload(FStringUtils::Widen(Data->Filename));
	}
}

void EOS_CALL FPlayerDataStorage::OnFileTransferProgressUpdated(const EOS_PlayerDataStorage_FileTransferProgressCallbackInfo* Data)
{
	if (Data)
	{
		if (Data->TotalFileSizeBytes > 0)
		{
			FGame::Get().GetPlayerDataStorage()->UpdateProgress(FStringUtils::Widen(Data->Filename), float(Data->BytesTransferred) / Data->TotalFileSizeBytes);
			FDebugLog::Log(L"[EOS SDK] Player data storage: transfer progress %d / %d.", Data->BytesTransferred, Data->TotalFileSizeBytes);
		}
	}
}

void EOS_CALL FPlayerDataStorage::OnFileCopied(const EOS_PlayerDataStorage_DuplicateFileCallbackInfo* Data)
{
	if (Data)
	{
		if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
		{
			// Operation is retrying so it is not complete yet
			return;
		}

		if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"[EOS SDK] Player data storage: error while copying the file: %ls.", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else
		{
			FGame::Get().GetPlayerDataStorage()->QueryList();
		}
	}
}

void EOS_CALL FPlayerDataStorage::OnFileRemoved(const EOS_PlayerDataStorage_DeleteFileCallbackInfo* Data)
{
	if (Data)
	{
		if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
		{
			// Operation is retrying so it is not complete yet
			return;
		}

		if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			FDebugLog::LogError(L"[EOS SDK] Player data storage: error while removing file: %ls", FStringUtils::Widen(EOS_EResult_ToString(Data->ResultCode)).c_str());
		}
		else
		{
			FGame::Get().GetPlayerDataStorage()->QueryList();
		}
	}
}
