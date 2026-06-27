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
	String MakeCollectionAlias(const SymbolPickerIconEntry& entry) const;
	void SelectLibraryCatalogId(const String& catalog_id);

	UiBoxLayout main_box_ { UiDirection::V };
	UiBoxLayout top_heading_layout_ { UiDirection::H };
	UiTitleCard heading_card_;
	UiDropdown  theme_preset_drop_;
	UiDropdown  icon_style_drop_;
	UiLineEdit  filter_edit_;
	SymbolPickerTintCtrl tint_ctrl_;
	UiButton    new_collection_button_;
	UiButton    clear_bin_button_;
	UiButton    add_to_bin_button_;
	UiButton    add_to_collection_button_;

	UiPanel categories_panel_;
	UiBoxLayout category_base_layout_ { UiDirection::V };
	UiPanel category_header_shell_;
	UiBoxLayout category_header_layout_ { UiDirection::H };
	UiBoxLayout categories_action_layout_ { UiDirection::H };
	UiTitleCard category_card_;
	UiScrollPanel category_scroll_panel_;

	UiPanel library_panel_;
	UiBoxLayout library_base_layout_ { UiDirection::V };
	UiBoxLayout library_header_layout_ { UiDirection::H };
	UiTitleCard library_card_;
	UiBoxLayout library_action_cluster_ { UiDirection::H };
	UiScrollPanel library_scroll_panel_;

	UiPanel collections_panel_;
	UiBoxLayout collections_base_layout_ { UiDirection::V };
	UiBoxLayout collections_header_layout_ { UiDirection::H };
	UiTitleCard collections_card_;
	UiBoxLayout collections_action_cluster_ { UiDirection::H };
	UiScrollPanel collections_scroll_panel_;

	UiBoxLayout category_content_layout_ { UiDirection::V };
	UiBoxLayout library_content_layout_ { UiDirection::V };
	UiBoxLayout collections_content_layout_ { UiDirection::V };
	UiDropdown  collections_selector_;
	UiLineEdit  collections_filter_edit_;

	Array<UiButton> category_buttons_;
	Array<UiButton> library_tiles_;
	Array<UiLabel>  collection_tiles_;
	String          selected_library_catalog_id_;

	SymbolPickerModel* model_ = nullptr;
	const SymbolPickerCatalog* catalog_ = nullptr;
	SymbolPickerCommandStack* commands_ = nullptr;
};

}

#endif
