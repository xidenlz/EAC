// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Dialog.h"
#include "Font.h"
#include "ListView.h"

class FPlayerDataStorageInfo : public FTextLabelWidget
{
public:
	FPlayerDataStorageInfo(Vector2 LabelPos,
		Vector2 LabelSize,
		UILayer LabelLayer,
		const std::wstring& LabelText,
		const std::wstring& LabelAssetFile) : FTextLabelWidget(LabelPos, LabelSize, LabelLayer, LabelText, LabelAssetFile, Color::White)
	{}

	void SetFocused(bool bValue) override;
};

template<>
std::shared_ptr<FPlayerDataStorageInfo> CreateListEntry(Vector2 Pos, Vector2 Size, UILayer Layer, const std::wstring& Data);



using FPlayerDataList = FListViewWidget<std::wstring, FPlayerDataStorageInfo>;

/**
 * Forward declarations
 */
class FGameEvent;
class FSpriteWidget;
class FTextEditorWidget;

/**
 * Player Data Storage dialog
 */
class FPlayerDataStorageDialog : public FDialog
{
public:
	/**
	 * Constructor
	 */
	FPlayerDataStorageDialog(Vector2 DialogPos,
		Vector2 DialogSize,
		UILayer DialogLayer,
		FontPtr DialogNormalFont,
		FontPtr DialogSmallFont,
		FontPtr DialogTinyFont);

	/**
	 * Destructor
	 */
	virtual ~FPlayerDataStorageDialog() {};

	/**
	* IGfxComponent Overrides
	*/
	virtual void Update() override;
	virtual void Create() override;

	/** IWidget */
	virtual void SetPosition(Vector2 Pos) override;
	virtual void SetSize(Vector2 NewSize) override;
	void SetWindowSize(Vector2 WindowSize);
	void SetWindowProportion(Vector2 InWindowProportion) { ConsoleWindowProportion = InWindowProportion; }
	void UpdateUserInfo();
	void ClearDataList();
	void ShowDataList();
	void HideDataList();

	void OnDataStorageEntrySelected(size_t Index);
	const std::wstring& GetCurrentSelection() const { return CurrentSelection; }
	std::wstring GetEditorContents() const;
	void ClearCurrentSelection();

	/**
	* Receives game event
	*
	* @param Event - Game event to act on
	*/
	void OnGameEvent(const FGameEvent& Event);

private:
	/** Header label */
	std::shared_ptr<FTextLabelWidget> HeaderLabel;

	/** Background Image */
	std::shared_ptr<FSpriteWidget> BackgroundImage;

	/** List of player data entries */
	std::shared_ptr<FPlayerDataList> PlayerDataList;

	/** File editor label */
	std::shared_ptr<FTextLabelWidget> TextEditorLabel;

	/** Text Editor to make changes in player data */
	std::shared_ptr<FTextEditorWidget> TextEditor;

	/** List of entries */
	std::vector<std::wstring> NameList;

	/** Button to refresh list */
	std::shared_ptr<FButtonWidget> RefreshListButton;

	/** Button to download file */
	std::shared_ptr<FButtonWidget> DownloadFileButton;

	/** Button to upload file */
	std::shared_ptr<FButtonWidget> SaveFileButton;

	/** Button to duplicate file */
	std::shared_ptr<FButtonWidget> DuplicateFileButton;

	/** Button to delete file */
	std::shared_ptr<FButtonWidget> DeleteFileButton;

	/** Currently selected entry */
	std::wstring CurrentSelection;

	/** Part of window that console is taking */
	Vector2 ConsoleWindowProportion;
};
