// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Dialog.h"
#include "Font.h"
#include "ListView.h"
#include  "StringViewListEntry.h"

class FTitleStorageInfo : public FTextLabelWidget
{
public:
	FTitleStorageInfo(Vector2 LabelPos,
		Vector2 LabelSize,
		UILayer LabelLayer,
		const std::wstring& LabelText,
		const std::wstring& LabelAssetFile) : FTextLabelWidget(LabelPos, LabelSize, LabelLayer, LabelText, LabelAssetFile, Color::White)
	{}

	void SetFocused(bool bValue) override;
};

template<>
std::shared_ptr<FTitleStorageInfo> CreateListEntry(Vector2 Pos, Vector2 Size, UILayer Layer, const std::wstring& Data);

using FTitleDataList = FListViewWidget<std::wstring, FTitleStorageInfo>;
using FTitleTagList = FListViewWidget<std::wstring, FStringViewListEntry>;

/**
 * Forward declarations
 */
class FGameEvent;
class FSpriteWidget;
class FTextEditorWidget;
class FTextField;

/**
 * Title Storage dialog
 */
class FTitleStorageDialog : public FDialog
{
public:
	/**
	 * Constructor
	 */
	FTitleStorageDialog(Vector2 DialogPos,
		Vector2 DialogSize,
		UILayer DialogLayer,
		FontPtr DialogNormalFont,
		FontPtr DialogSmallFont,
		FontPtr DialogTinyFont);

	/**
	 * Destructor
	 */
	virtual ~FTitleStorageDialog() {};

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
	void ClearDataLists();
	void ShowDataLists();
	void HideDataLists();
	void ShowWidgets();
	void HideWidgets();

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

	/** List of title storage data entries */
	std::shared_ptr<FTitleDataList> TitleDataList;

	/** File editor label */
	std::shared_ptr<FTextLabelWidget> TextEditorLabel;

	/** Text Editor to view data in title storage */
	std::shared_ptr<FTextEditorWidget> TextEditor;

	/** List of entries */
	std::vector<std::wstring> NameList;

	/** List of currently selected tags */
	std::vector<std::wstring> Tags;

	/** Text field to enter file name */
	std::shared_ptr<FTextFieldWidget> FileNameTextField;

	/** Button to query list of file using tags */
	std::shared_ptr<FButtonWidget> QueryListButton;

	/** Button to download file */
	std::shared_ptr<FButtonWidget> DownloadFileButton;

	/** Text field to enter tag name */
	std::shared_ptr<FTextFieldWidget> TagNameTextField;

	/** Button to add tag */
	std::shared_ptr<FButtonWidget> AddTagButton;

	/** Button to clear tag list */
	std::shared_ptr<FButtonWidget> ClearTagsButton;

	/** List of title storage tags that are currently selected. */
	std::shared_ptr<FTitleTagList> TagList;

	/** Currently selected entry */
	std::wstring CurrentSelection;

	/** Part of window that console is taking */
	Vector2 ConsoleWindowProportion;
};
