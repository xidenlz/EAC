// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include <eos_sdk.h>
#include <eos_playerdatastorage_types.h>
#include <eos_playerdatastorage.h>

/**
* Manages player data storage for local user
*/
class FPlayerDataStorage
{
public:
	/**
	* Constructor
	*/
	FPlayerDataStorage() noexcept(false);

	/**
	* No copying or copy assignment allowed for this class.
	*/
	FPlayerDataStorage(FPlayerDataStorage const&) = delete;
	FPlayerDataStorage& operator=(FPlayerDataStorage const&) = delete;

	/**
	* Destructor
	*/
	virtual ~FPlayerDataStorage();

	void Update();

	/** 
	* Remote operations follow. They trigger online requests to the backend services.
	*/

	/**
	* Refresh list of player data entries. Asynchronous operation.
	*/
	void QueryList();

	/**
	* Retrieve contents of specific file
	*/
	void StartFileDataDownload(const std::wstring& FileName);

	/**
	* Transmit contents of specific file to cloud
	*/
	bool StartFileDataUpload(const std::wstring& FileName);

	/** 
	* Add new file or overwrite existing one.
	*/
	void AddFile(const std::wstring& EntryName, const std::wstring& Data);

	/** 
	* Create a copy of existing file's data.
	*/
	void CopyFile(const std::wstring& FileSourceName, const std::wstring& FileDestName);

	/** 
	* Remove existing file and any cached data related to it.
	*/
	void RemoveFile(const std::wstring& EntryName);

	/** 
	* Cancel current file transfer (if any).
	*/
	void CancelCurrentTransfer();


	/** 
	* Functions that work with local data follow. These functions do not trigger any online queries but work with local data instead.
	*/

	/** 
	* Returns locally cached list of files.
	*/
	std::vector<std::wstring> GetFileList() const;

	/** 
	* Sets local list of files.
	*/
	void SetFileList(const std::vector<std::wstring>& FileNames);

	/** 
	* Returns locally cached data for file specified. NoData flag is set to true when there is no data available locally.
	*/
	std::wstring GetLocalData(const std::wstring& EntryName, bool& NoData) const;

	/** 
	* Sets local data for file specified.
	*/
	void SetLocalData(const std::wstring& EntryName, const std::wstring& Data);

	/** 
	* Erases local data for specified file (if any).
	*/
	void EraseLocalData(const std::wstring& EntryName);

	/** 
	* Gets the name of file that is being transferred at the moment.
	*/
	const std::wstring& GetCurrentTransferName() const { return CurrentTransferName; }

	/** 
	* Gets current progress of the file transfer (in the range [0.0f, 1.0f]).
	*/
	float GetCurrentTransferProgress() const { return CurrentTransferProgress; }

	//Called on async operations progress
	EOS_PlayerDataStorage_EReadResult ReceiveData(const std::wstring& FileName, const void* Data, size_t NumBytes, size_t TotalSize);
	EOS_PlayerDataStorage_EWriteResult SendData(const std::wstring& FileName, void* Data, uint32_t* BytesWritten);
	void UpdateProgress(const std::wstring& FileName, float Progress);
	void FinishFileDownload(const std::wstring& FileName, bool bSuccess);
	void FinishFileUpload(const std::wstring& FileName);

	/**
	* Receives game event
	*
	* @param Event - Game event to act on
	*/
	void OnGameEvent(const FGameEvent& Event);

	/**
	* Callback that is fired when the query file list async operation completes, either successfully or in error
	*
	* @param Data - Output parameters for the EOS_PlayerDataStorage_QueryFileList Function
	*/
	static void EOS_CALL OnFileListRetrieved(const EOS_PlayerDataStorage_QueryFileListCallbackInfo* Data);

	/**
	* Callback that is fired when file data is received.
	*
	* @param Data - Output parameters for the  Function
	*/
	static EOS_PlayerDataStorage_EReadResult EOS_CALL OnFileDataReceived(const EOS_PlayerDataStorage_ReadFileDataCallbackInfo* Data);

	/**
	* Callback that is fired when file transfer from cloud is completed.
	*
	* @param Data - Output parameters
	*/
	static void EOS_CALL OnFileReceived(const EOS_PlayerDataStorage_ReadFileCallbackInfo* Data);

	/**
	* Callback that is fired when file data is to be sent to cloud.
	*
	* @param Data - Output parameters for the  Function
	*/
	static EOS_PlayerDataStorage_EWriteResult EOS_CALL OnFileDataSend(const EOS_PlayerDataStorage_WriteFileDataCallbackInfo* Data, void* OutDataBuffer, uint32_t* OutDataWritten);

	/**
	* Callback that is fired when file transfer to cloud is completed.
	*
	* @param Data - Output parameters
	*/
	static void EOS_CALL OnFileSent(const EOS_PlayerDataStorage_WriteFileCallbackInfo* Data);

	/**
	* Callback that is fired when we get information about transfer progress
	*
	* @param Data - file transfer progress
	*/
	static void EOS_CALL OnFileTransferProgressUpdated(const EOS_PlayerDataStorage_FileTransferProgressCallbackInfo* Data);

	/**
	* Callback that is fired when we get confirmation that the file has been copied on the cloud (or error occurred).
	*
	* @param Data - out parameter
	*/
	static void EOS_CALL OnFileCopied(const EOS_PlayerDataStorage_DuplicateFileCallbackInfo* Data);
	

	/**
	* Callback that is fired when we get confirmation that the file has been removed from cloud (or error occurred).
	*
	* @param Data - out parameter
	*/
	static void EOS_CALL OnFileRemoved(const EOS_PlayerDataStorage_DeleteFileCallbackInfo* Data);

private:

	/**
	* Called when a user has logged in
	*/
	void OnLoggedIn(FProductUserId UserId);

	/**
	* Called when a user has logged out
	*/
	void OnLoggedOut(FProductUserId UserId);

	void ClearCurrentTransfer();

	// The name of file we are currently downloading or uploading
	std::wstring CurrentTransferName;

	//Current primary active transfer (upload or download).
	EOS_HPlayerDataStorageFileTransferRequest CurrentTransferHandle = nullptr;

	// The progress we made so far while transmitting file.
	float CurrentTransferProgress;

	/** Map of player data entries. Key - entry name, Value - pair (bool: is data present locally ; string: entry data). */
	std::unordered_map<std::wstring, std::pair<bool, std::wstring>> StorageData;

	struct FTransferInProgress
	{
		bool bDownload = true;
		size_t TotalSize = 0;
		size_t CurrentIndex = 0;
		std::vector<char> Data;

		bool Done() const { return TotalSize == CurrentIndex; }
	};

	std::unordered_map <std::wstring, FTransferInProgress> TransfersInProgress;
};
