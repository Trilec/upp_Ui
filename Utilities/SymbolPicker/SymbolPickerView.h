#ifndef _Utilities_SymbolPicker_SymbolPickerView_h_
#define _Utilities_SymbolPicker_SymbolPickerView_h_

#include "SymbolPickerModel.h"
#include "SymbolPickerCatalog.h"
#include "SymbolPickerCommands.h"

#include <Ui/Ui.h>

namespace Upp {

class SymbolPickerTintCtrl : public ParentCtrl {
public:
	typedef SymbolPickerTintCtrl CLASSNAME;

	SymbolPickerTintCtrl();

	void SetColor(Color color);
	Color GetColor() const;

	Event<> WhenAction;

private:
	Label       label_;
	ColorPusher swatch_;
	Color       color_ = Null;
};

class SymbolPickerIconTile : public ParentCtrl {
public:
	typedef SymbolPickerIconTile CLASSNAME;

	SymbolPickerIconTile();

	void SetEntry(const SymbolPickerIconEntry& entry);
	const SymbolPickerIconEntry& GetEntry() const { return entry_; }
	String GetCatalogId() const;
	String GetSourceId() const;
	void SetSelected(bool selected);

	Event<> WhenSelected;
	Event<> WhenActivated;

	virtual void LeftDown(Point p, dword keyflags) override;
	virtual void LeftDrag(Point p, dword keyflags) override;
	virtual void LeftDouble(Point p, dword keyflags) override;
	virtual void Paint(Draw& w) override;
	virtual void Layout() override;
	virtual Size GetMinSize() const override;

private:
	void SyncLabels();

	SymbolPickerIconEntry entry_;
	Label title_;
	Label meta_;
	Label ids_;
	bool  selected_ = false;
};

class SymbolPickerCollectionTile : public ParentCtrl {
public:
	typedef SymbolPickerCollectionTile CLASSNAME;

	SymbolPickerCollectionTile();

	void SetItem(const SymbolPickerIconRef& item, int index);
	int GetItemIndex() const { return item_index_; }

	virtual void Paint(Draw& w) override;
	virtual void Layout() override;
	virtual Size GetMinSize() const override;
	virtual void LeftDrag(Point p, dword keyflags) override;

	Event<> WhenDragStart;

private:
	Label title_;
	Label meta_;
	Label ids_;
	bool  unresolved_ = false;
	int   item_index_ = -1;
};

class SymbolPickerDropScrollPanel : public UiScrollPanel {
public:
	typedef SymbolPickerDropScrollPanel CLASSNAME;

	enum DropVisualState {
		DROP_NORMAL,
		DROP_DRAG_OVER,
		DROP_ACCEPTED,
		DROP_REJECTED,
	};

	SymbolPickerDropScrollPanel();

	void SetDropState(DropVisualState state);
	DropVisualState GetDropState() const { return drop_state_; }
	Point GetLastDragPoint() const { return last_drag_point_; }

	Event<PasteClip&> WhenDropTest;
	Event<PasteClip&> WhenDropPerform;

	virtual void Paint(Draw& w) override;
	virtual void DragEnter() override;
	virtual void DragAndDrop(Point p, PasteClip& d) override;
	virtual void DragLeave() override;

private:
	DropVisualState drop_state_ = DROP_NORMAL;
	Point last_drag_point_ = Point(0, 0);
};

class SymbolPickerView : public TopWindow {
public:
	typedef SymbolPickerView CLASSNAME;

	SymbolPickerView();

	void SetModel(SymbolPickerModel* model);
	void SetCatalog(const SymbolPickerCatalog* catalog);
	void SetCommands(SymbolPickerCommandStack* commands);
	void RefreshFromModel();

private:
	void BuildUi();
	void BuildTopHeading();
	void BuildCategoriesPanel();
	void BuildLibraryPanel();
	void BuildCollectionsPanel();
	void RefreshCollections();
	void RefreshCollectionItems();
	void RefreshCategories();
	void RefreshLibrary();
	void RebuildCategoryButtons();
	void RebuildLibraryTiles();
	void RebuildCollectionTiles();
	void HandleCollectionsDropTest(PasteClip& d);
	void HandleCollectionsDropPerform(PasteClip& d);
	int GetCollectionDropInsertIndex(Point p) const;
	String MakeCollectionAlias(const SymbolPickerIconEntry& entry) const;
	void SelectLibraryCatalogId(const String& catalog_id);
	void UpdateLibraryTileSelection();

	UiBoxLayout main_box_ { UiDirection::V };
	UiBoxLayout top_heading_layout_ { UiDirection::H };
	UiTitleCard heading_card_;
	UiLabel     version_label_;
	UiToolButton dark_theme_tool_;
	UiToolButton help_tool_;
	UiToolButton setup_tool_;
	UiButton    exit_button_;

	UiPanel categories_panel_;
	UiBoxLayout category_base_layout_ { UiDirection::V };
	UiPanel category_header_shell_;
	UiBoxLayout category_header_layout_ { UiDirection::H };
	UiBoxLayout categories_action_layout_ { UiDirection::H };
	UiTitleCard category_card_;
	UiToolButton categories_filter_icon_;
	UiLineEdit  categories_filter_edit_;
	UiScrollPanel category_scroll_panel_;

	UiPanel library_panel_;
	UiBoxLayout library_base_layout_ { UiDirection::V };
	UiBoxLayout library_header_layout_ { UiDirection::H };
	UiTitleCard library_card_;
	UiBoxLayout library_action_cluster_ { UiDirection::H };
	UiDropdown  library_style_selector_;
	UiToolButton library_refresh_button_;
	UiLineEdit  library_filter_edit_;
	UiScrollPanel library_scroll_panel_;

	UiPanel collections_panel_;
	UiBoxLayout collections_base_layout_ { UiDirection::V };
	UiBoxLayout collections_header_layout_ { UiDirection::H };
	UiTitleCard collections_card_;
	UiBoxLayout collections_action_cluster_ { UiDirection::H };
	UiToolButton new_collection_tool_;
	UiToolButton remove_collection_tool_;
	UiSplitButton save_and_save_as_button_;
	UiSplitButton load_and_history_button_;
	UiSplitButton export_and_type_button_;
	UiDropdown  output_pixel_size_;
	UiDropdown  output_export_type_;
	UiToolButton copy_button_;
	UiToolButton collections_filter_icon_;
	UiLineEdit  collections_filter_edit_;
	SymbolPickerDropScrollPanel collections_scroll_panel_;

	UiBoxLayout category_content_layout_ { UiDirection::H };
	UiBoxLayout library_content_layout_ { UiDirection::H };
	UiBoxLayout collections_content_layout_ { UiDirection::H };
	UiDropdown  collections_selector_;

	Array<UiButton> category_buttons_;
	Array<SymbolPickerIconTile> library_tiles_;
	Array<SymbolPickerCollectionTile> collection_tiles_;
	String          selected_library_catalog_id_;

	SymbolPickerModel* model_ = nullptr;
	const SymbolPickerCatalog* catalog_ = nullptr;
	SymbolPickerCommandStack* commands_ = nullptr;
};

}

#endif
