// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include <eos_sdk.h>
#include <eos_titlestorage_types.h>
#include <eos_titlestorage.h>

/**
* Manages player data storage for local user
*/
class FTitleStorage
{
public:
	/**
	* Constructor
	*/
	FTitleStorage() noexcept(false);

	/**
	* No copying or copy assignment allowed for this class.
	*/
	FTitleStorage(FTitleStorage const&) = delete;
	FTitleStorage& operator=(FTitleStorage const&) = delete;

	/**
	* Destructor
	*/
	virtual ~FTitleStorage();

	void Update();

	/** 
	* Remote operations follow. They trigger online requests to the backend services.
	*/

	/**
	* Refresh list of player data entries. Asynchronous operation. Takes currently selected tags into account.
	*/
	void QueryList();

	/**
	* Retrieve contents of specific file
	*/
	void StartFileDataDownload(const std::wstring& FileName);

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
	* Gets the name of file that is being transferred at the moment.
	*/
	const std::wstring& GetCurrentTransferName() const { return CurrentTransferName; }

	/** 
	* Gets current progress of the file transfer (in the range [0.0f, 1.0f]).
	*/
	float GetCurrentTransferProgress() const { return CurrentTransferProgress; }

	/**
	* Add one more tag to currently selected tags.
	*/
	void AddTag(const std::string& NewTag) { CurrentTags.insert(NewTag); }

	/** 
	* Clear all currently selected tags
	*/
	void ClearTags() { CurrentTags.clear(); }

	/** 
	* Get the list of currently selected tags.
	*/
	const std::set<std::string>& GetCurrentTags() const { return CurrentTags; }

	//Called on async operations progress
	EOS_TitleStorage_EReadResult ReceiveData(const std::wstring& FileName, const void* Data, size_t NumBytes, size_t TotalSize);
	void UpdateProgress(const std::wstring& FileName, float Progress);
	void FinishFileDownload(const std::wstring& FileName, bool bSuccess);

	/**
	* Receives game event
	*
	* @param Event - Game event to act on
	*/
	void OnGameEvent(const FGameEvent& Event);

	/**
	* Callback that is fired when the query file list async operation completes, either successfully or in error
	*
	* @param Data - Output parameters for the EOS_TitleStorage_QueryFileList Function
	*/
	static void EOS_CALL OnFileListRetrieved(const EOS_TitleStorage_QueryFileListCallbackInfo* Data);

	/**
	* Callback that is fired when file data is received.
	*
	* @param Data - Output parameters for the  Function
	*/
	static EOS_TitleStorage_EReadResult EOS_CALL OnFileDataReceived(const EOS_TitleStorage_ReadFileDataCallbackInfo* Data);

	/**
	* Callback that is fired when file transfer from cloud is completed.
	*
	* @param Data - Output parameters
	*/
	static void EOS_CALL OnFileReceived(const EOS_TitleStorage_ReadFileCallbackInfo* Data);

	/**
	* Callback that is fired when we get information about transfer progress
	*
	* @param Data - file transfer progress
	*/
	static void EOS_CALL OnFileTransferProgressUpdated(const EOS_TitleStorage_FileTransferProgressCallbackInfo* Data);

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
	EOS_HTitleStorageFileTransferRequest CurrentTransferHandle = nullptr;

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
	std::set<std::string> CurrentTags;
};
