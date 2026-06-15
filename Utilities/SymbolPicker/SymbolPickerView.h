#ifndef _Utilities_SymbolPicker_SymbolPickerView_h_
#define _Utilities_SymbolPicker_SymbolPickerView_h_

#include "SymbolPickerModel.h"
#include "SymbolPickerCommands.h"

#include <CtrlLib/CtrlLib.h>

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
	void SetCommands(SymbolPickerCommandStack* commands);
	void RefreshFromModel();

private:
	void BuildUi();
	void BuildHeader();
	void BuildCategoryStrip();
	void BuildLibraryPanel();
	void BuildCollectionsPanel();
	void BuildBinPanel();
	void RefreshCollections();
	void RefreshCollectionItems();
	void RefreshBin();

	ParentCtrl root_;
	StaticRect header_row_;
	StaticRect category_strip_;
	StaticRect center_host_;
	Splitter   center_split_;
	Splitter   main_split_;

	StaticRect library_panel_;
	StaticRect collections_panel_;
	StaticRect bin_panel_;

	Label      title_;
	Label      subtitle_;
	Label      filter_label_;
	DropList   theme_preset_drop_;
	DropList   icon_style_drop_;
	EditString filter_edit_;
	Option     outlined_option_;
	Option     rounded_option_;
	Option     sharp_option_;
	SymbolPickerTintCtrl tint_ctrl_;
	Button     new_collection_button_;
	Button     clear_bin_button_;

	ArrayCtrl  categories_list_;
	ArrayCtrl  library_placeholder_list_;
	ArrayCtrl  collections_list_;
	TabCtrl    collections_tabs_;
	ParentCtrl collection_items_page_;
	ArrayCtrl  collection_items_list_;
	ArrayCtrl  bin_list_;
	Label      library_title_;
	Label      library_subtitle_;
	Label      collections_title_;
	Label      collections_subtitle_;
	Label      bin_title_;
	Label      bin_subtitle_;
	bool       refreshing_collections_ = false;

	SymbolPickerModel* model_ = nullptr;
	SymbolPickerCommandStack* commands_ = nullptr;
};

}

#endif
