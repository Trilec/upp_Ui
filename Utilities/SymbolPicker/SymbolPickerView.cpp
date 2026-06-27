#include "SymbolPickerView.h"

namespace Upp {

static const char* SymbolPickerIconStyleText(SymbolPickerIconStyle style)
{
	switch(style) {
	case SymbolPickerIconStyle::Outlined: return "Outlined";
	case SymbolPickerIconStyle::Rounded:  return "Rounded";
	case SymbolPickerIconStyle::Sharp:    return "Sharp";
	}
	return "Outlined";
}

static String SafeAliasPart(const String& text)
{
	String out;
	for(int i = 0; i < text.GetCount(); ++i) {
		int c = (byte)text[i];
		if(IsAlNum(c))
			out.Cat(ToUpper((wchar)c));
		else if(out.IsEmpty() || out[out.GetCount() - 1] != '_')
			out.Cat('_');
	}
	while(!out.IsEmpty() && out[out.GetCount() - 1] == '_')
		out.Trim(out.GetCount() - 1);
	return out;
}

SymbolPickerTintCtrl::SymbolPickerTintCtrl()
{
	Add(label_.LeftPosZ(0, 32).VCenterPosZ(20));
	Add(swatch_.RightPosZ(0, 56).VCenterPosZ(22));
	label_.SetLabel("Tint");
	swatch_ <<= color_;
	swatch_.WhenAction = [=] {
		color_ = (Color)~swatch_;
		if(WhenAction)
			WhenAction();
	};
	SetMinSize(Size(DPI(96), DPI(24)));
}

void SymbolPickerTintCtrl::SetColor(Color color)
{
	color_ = color;
	swatch_ <<= color_;
}

Color SymbolPickerTintCtrl::GetColor() const
{
	return color_;
}

SymbolPickerView::SymbolPickerView()
{
	Title("Symbol Picker");
	Sizeable().Zoomable();
	SetRect(0, 0, DPI(1240), DPI(840));
	SetMinSize(Size(DPI(960), DPI(680)));
	BuildUi();
}

void SymbolPickerView::BuildUi()
{
	Add(main_box_.SizePos());
	main_box_.SetDirection(UiDirection::V).SetGap(DPI(8)).SetInset(DPI(8));

	BuildTopHeading();
	BuildCategoriesPanel();
	BuildLibraryPanel();
	BuildCollectionsPanel();

	main_box_.Add(top_heading_layout_).Fit().AlignSelf(UiBoxLayout::Align::Stretch);
	main_box_.Add(categories_panel_).Expand(1).MinMain(DPI(130)).AlignSelf(UiBoxLayout::Align::Stretch);
	main_box_.Add(library_panel_).Expand(2).MinMain(DPI(220)).AlignSelf(UiBoxLayout::Align::Stretch);
	main_box_.Add(collections_panel_).Expand(2).MinMain(DPI(220)).AlignSelf(UiBoxLayout::Align::Stretch);
}

void SymbolPickerView::BuildTopHeading()
{
	top_heading_layout_.SetDirection(UiDirection::H)
		.SetGap(DPI(8))
		.SetInset(0)
		.SetWrap(UiBoxWrap::Flow)
		.SetWrapAutoResize(true)
		.SetAlignItems(UiCrossAlign::Center);

	UiTitleCard::Style heading_style = UiTheme::ResolveTitleCard(UiRole::Accent);
	heading_style.metrics.face_enabled = false;
	heading_style.metrics.frame_enabled = false;
	heading_style.metrics.radius = 0;
	heading_style.card_line_side = UiAlign::RIGHT;
	heading_style.card_line_thickness = DPI(2);
	heading_style.card_line_gap = DPI(9);
	heading_card_.SetCustomStyle(heading_style);
	heading_card_.SetTitle("Symbols Picker")
		.SetSubTitle("Designer layout reconciliation shell")
		.SetContentInset(DPI(4))
		.SetMediaGap(DPI(9))
		.SetMediaReserve(DPI(0))
		.SetMediaMin(DPI(15))
		.SetMediaAutoFit(false)
		.SetMediaSide(UiAlign::LEFT)
		.SetMediaAlign(UiAlign::CENTER, UiAlign::CENTER);

	theme_preset_drop_.SetSizeMin(DPI(110), 0);
	theme_preset_drop_.UseInternalModel().Clear()
		.Add("Minimal", (int)UiThemePreset::Minimal)
		.Add("Pill", (int)UiThemePreset::Pill)
		.Add("Layered", (int)UiThemePreset::Layered);
	theme_preset_drop_.Select(0);

	icon_style_drop_.SetSizeMin(DPI(110), 0);
	icon_style_drop_.UseInternalModel().Clear()
		.Add("Outlined", (int)SymbolPickerIconStyle::Outlined)
		.Add("Rounded", (int)SymbolPickerIconStyle::Rounded)
		.Add("Sharp", (int)SymbolPickerIconStyle::Sharp);
	icon_style_drop_.Select(0);

	filter_edit_.SetMinSize(Size(DPI(180), 0));
	filter_edit_.SetPlaceholder("Filter library");

	new_collection_button_.SetText("New Collection").SetContentInset(DPI(6)).SetContentGap(DPI(8));
	clear_bin_button_.SetText("Clear Bin").SetContentInset(DPI(6)).SetContentGap(DPI(8));
	add_to_bin_button_.SetText("Add To Bin").SetContentInset(DPI(6)).SetContentGap(DPI(8));
	add_to_collection_button_.SetText("Add To Collection").SetContentInset(DPI(6)).SetContentGap(DPI(8));

	top_heading_layout_.Add(heading_card_).Expand(1).MinMain(DPI(240)).MinCross(DPI(56)).AlignSelf(UiBoxLayout::Align::Stretch);
	top_heading_layout_.Add(theme_preset_drop_).Fit().MinMain(DPI(110)).AlignSelf(UiBoxLayout::Align::Stretch);
	top_heading_layout_.Add(icon_style_drop_).Fit().MinMain(DPI(110)).AlignSelf(UiBoxLayout::Align::Stretch);
	top_heading_layout_.Add(filter_edit_).Fit().MinMain(DPI(180)).AlignSelf(UiBoxLayout::Align::Stretch);
	top_heading_layout_.Add(tint_ctrl_).Fixed(DPI(112)).AlignSelf(UiBoxLayout::Align::Center);
	top_heading_layout_.Add(add_to_bin_button_).Fit().AlignSelf(UiBoxLayout::Align::Stretch);
	top_heading_layout_.Add(add_to_collection_button_).Fit().AlignSelf(UiBoxLayout::Align::Stretch);
	top_heading_layout_.Add(new_collection_button_).Fit().AlignSelf(UiBoxLayout::Align::Stretch);
	top_heading_layout_.Add(clear_bin_button_).Fit().AlignSelf(UiBoxLayout::Align::Stretch);

	tint_ctrl_.WhenAction = [=] {
		if(model_ && commands_)
			commands_->Execute(MakeSymbolPickerSetTintCommand(tint_ctrl_.GetColor()), *model_);
	};
	theme_preset_drop_.WhenAction = [=] {
		if(model_ && commands_)
			commands_->Execute(MakeSymbolPickerSetThemePresetCommand((UiThemePreset)(int)~theme_preset_drop_), *model_);
	};
	icon_style_drop_.WhenAction = [=] {
		if(model_ && commands_)
			commands_->Execute(MakeSymbolPickerSetIconStyleCommand((SymbolPickerIconStyle)(int)~icon_style_drop_), *model_);
	};
	filter_edit_.WhenAction = [=] {
		if(model_ && commands_)
			commands_->Execute(MakeSymbolPickerSetFilterCommand(filter_edit_.GetTextUtf8()), *model_);
	};
	new_collection_button_.WhenAction = [=] {
		if(model_ && commands_)
			commands_->Execute(MakeSymbolPickerCreateCollectionCommand(Format("Collection %d", model_->GetCollections().GetCount() + 1)), *model_);
	};
	clear_bin_button_.WhenAction = [=] {
		if(model_ && commands_)
			commands_->Execute(MakeSymbolPickerClearBinCommand(), *model_);
	};
	add_to_bin_button_.WhenAction = [=] {
		if(model_ && commands_ && !selected_library_catalog_id_.IsEmpty())
			commands_->Execute(MakeSymbolPickerAddToBinCommand(selected_library_catalog_id_), *model_);
	};
	add_to_collection_button_.WhenAction = [=] {
		if(!model_ || !catalog_ || !commands_ || selected_library_catalog_id_.IsEmpty())
			return;
		if(model_->GetActiveCollectionIndex() < 0)
			return;
		const SymbolPickerIconEntry* entry = catalog_->FindByCatalogId(selected_library_catalog_id_);
		if(!entry)
			return;
		SymbolPickerIconRef ref;
		ref.catalog_id = entry->catalog_id;
		ref.source_id = entry->source_id;
		ref.alias = MakeCollectionAlias(*entry);
		ref.size = model_->GetExportSize();
		ref.tint = model_->GetTintColor();
		ref.unresolved = false;
		commands_->Execute(MakeSymbolPickerAddIconToCollectionCommand(model_->GetActiveCollectionIndex(), ref), *model_);
	};
}

void SymbolPickerView::BuildCategoriesPanel()
{
	categories_panel_.Add(category_base_layout_.SizePos());
	category_base_layout_.SetDirection(UiDirection::V).SetGap(DPI(8)).SetInset(DPI(8));
	category_header_shell_.Add(category_header_layout_.SizePos());
	category_header_layout_.SetDirection(UiDirection::H).SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
	categories_action_layout_.SetDirection(UiDirection::H).SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);

	category_card_.SetMinSize(Size(DPI(180), DPI(56)));
	category_card_.SetTitle("Library Categories")
		.SetSubTitle("Category filter host")
		.SetContentInset(DPI(0))
		.SetMediaGap(DPI(0))
		.SetMediaReserve(DPI(38))
		.SetMediaMin(DPI(15))
		.SetMediaAutoFit(false)
		.SetMediaSide(UiAlign::LEFT)
		.SetMediaAlign(UiAlign::CENTER, UiAlign::TOP)
		.SetTextAlign(UiAlign::LEFT, UiAlign::TOP);

	category_scroll_panel_.SetScrollMode(UIPANELSCROLL_AUTO);
	category_scroll_panel_.Content().Add(category_content_layout_.SizePos());
	category_content_layout_.SetDirection(UiDirection::V).SetGap(DPI(6)).SetInset(DPI(8));

	category_base_layout_.Add(category_header_shell_).Fit().AlignSelf(UiBoxLayout::Align::Stretch);
	category_header_layout_.Add(category_card_).Expand(1).MinMain(DPI(180)).MinCross(DPI(56));
	category_header_layout_.Add(categories_action_layout_).Fit().AlignSelf(UiBoxLayout::Align::Center);
	category_base_layout_.Add(category_scroll_panel_).Expand(1).AlignSelf(UiBoxLayout::Align::Stretch);
}

void SymbolPickerView::BuildLibraryPanel()
{
	library_panel_.Add(library_base_layout_.SizePos());
	library_base_layout_.SetDirection(UiDirection::V).SetGap(DPI(8)).SetInset(DPI(8));
	library_header_layout_.SetDirection(UiDirection::H).SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
	library_action_cluster_.SetDirection(UiDirection::H).SetGap(DPI(8)).SetInset(0).SetWrap(UiBoxWrap::Flow).SetWrapAutoResize(true);

	library_card_.SetMinSize(Size(DPI(180), DPI(56)));
	library_card_.SetTitle("Library Symbols/Icons")
		.SetSubTitle("Browse the filtered runtime catalog.")
		.SetContentInset(DPI(0))
		.SetMediaGap(DPI(0))
		.SetMediaReserve(DPI(38))
		.SetMediaMin(DPI(16))
		.SetMediaAutoFit(false)
		.SetMediaSide(UiAlign::LEFT)
		.SetMediaAlign(UiAlign::CENTER, UiAlign::TOP)
		.SetTextAlign(UiAlign::LEFT, UiAlign::TOP);

	library_scroll_panel_.SetScrollMode(UIPANELSCROLL_AUTO);
	library_scroll_panel_.Content().Add(library_content_layout_.SizePos());
	library_content_layout_.SetDirection(UiDirection::V).SetGap(DPI(6)).SetInset(DPI(8));

	library_base_layout_.Add(library_header_layout_).Fit().AlignSelf(UiBoxLayout::Align::Stretch);
	library_header_layout_.Add(library_card_).Expand(1).MinMain(DPI(180)).MinCross(DPI(56));
	library_header_layout_.Add(library_action_cluster_).Fit().AlignSelf(UiBoxLayout::Align::Center);
	library_base_layout_.Add(library_scroll_panel_).Expand(1).AlignSelf(UiBoxLayout::Align::Stretch);
}

void SymbolPickerView::BuildCollectionsPanel()
{
	collections_panel_.Add(collections_base_layout_.SizePos());
	collections_base_layout_.SetDirection(UiDirection::V).SetGap(DPI(8)).SetInset(DPI(8));
	collections_header_layout_.SetDirection(UiDirection::H).SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
	collections_action_cluster_.SetDirection(UiDirection::H).SetGap(DPI(8)).SetInset(0).SetWrap(UiBoxWrap::Flow).SetWrapAutoResize(true);

	collections_card_.SetMinSize(Size(DPI(180), DPI(56)));
	collections_card_.SetTitle("Collections")
		.SetSubTitle("Saved sets and current bin summary.")
		.SetContentInset(DPI(0))
		.SetMediaGap(DPI(0))
		.SetMediaReserve(DPI(38))
		.SetMediaMin(DPI(16))
		.SetMediaAutoFit(false)
		.SetMediaSide(UiAlign::LEFT)
		.SetMediaAlign(UiAlign::CENTER, UiAlign::TOP)
		.SetTextAlign(UiAlign::LEFT, UiAlign::TOP);

	collections_selector_.SetSizeMin(DPI(180), 0);
	collections_filter_edit_.SetMinSize(Size(DPI(160), 0));
	collections_filter_edit_.SetPlaceholder("Filter placeholder");

	collections_scroll_panel_.SetScrollMode(UIPANELSCROLL_AUTO);
	collections_scroll_panel_.Content().Add(collections_content_layout_.SizePos());
	collections_content_layout_.SetDirection(UiDirection::V).SetGap(DPI(6)).SetInset(DPI(8));

	collections_base_layout_.Add(collections_header_layout_).Fit().AlignSelf(UiBoxLayout::Align::Stretch);
	collections_header_layout_.Add(collections_card_).Expand(1).MinMain(DPI(180)).MinCross(DPI(56));
	collections_header_layout_.Add(collections_action_cluster_).Fit().AlignSelf(UiBoxLayout::Align::Center);
	collections_action_cluster_.Add(collections_selector_).Fit().MinMain(DPI(180)).AlignSelf(UiBoxLayout::Align::Stretch);
	collections_action_cluster_.Add(collections_filter_edit_).Fit().MinMain(DPI(160)).AlignSelf(UiBoxLayout::Align::Stretch);
	collections_base_layout_.Add(collections_scroll_panel_).Expand(1).AlignSelf(UiBoxLayout::Align::Stretch);

	collections_selector_.WhenAction = [=] {
		if(!model_ || !commands_)
			return;
		int index = collections_selector_.GetData();
		if(index >= 0)
			commands_->Execute(MakeSymbolPickerSetActiveCollectionCommand(index), *model_);
	};
}

void SymbolPickerView::SetModel(SymbolPickerModel* model)
{
	model_ = model;
	RefreshFromModel();
}

void SymbolPickerView::SetCatalog(const SymbolPickerCatalog* catalog)
{
	catalog_ = catalog;
	RefreshFromModel();
}

void SymbolPickerView::SetCommands(SymbolPickerCommandStack* commands)
{
	commands_ = commands;
	RefreshFromModel();
}

void SymbolPickerView::RefreshCollections()
{
	collections_selector_.UseInternalModel().Clear();
	if(!model_)
		return;
	const Vector<SymbolPickerCollection>& collections = model_->GetCollections();
	for(int i = 0; i < collections.GetCount(); ++i) {
		const SymbolPickerCollection& collection = collections[i];
		String label = collection.name + (collection.dirty ? " *" : String());
		collections_selector_.Add(label, i);
	}
	if(model_->GetActiveCollectionIndex() >= 0 && model_->GetActiveCollectionIndex() < collections.GetCount())
		collections_selector_.SelectByData(model_->GetActiveCollectionIndex());
}

void SymbolPickerView::RefreshCollectionItems()
{
	RebuildCollectionTiles();
}

void SymbolPickerView::RefreshCategories()
{
	RebuildCategoryButtons();
}

void SymbolPickerView::RefreshLibrary()
{
	RebuildLibraryTiles();
}

void SymbolPickerView::RebuildCategoryButtons()
{
	category_content_layout_.ClearItems();
	category_buttons_.Clear();

	auto& all = category_buttons_.Add(new UiButton());
	all.SetText("All").SetContentInset(DPI(6)).SetContentGap(DPI(8));
	category_content_layout_.Add(all).Fit().AlignSelf(UiBoxLayout::Align::Stretch);
	all.WhenAction = [=] {
		if(model_ && commands_)
			commands_->Execute(MakeSymbolPickerSetCategoryCommand("All"), *model_);
	};

	if(!catalog_)
		return;

	Vector<SymbolPickerCategory> categories = catalog_->GetCategories();
	for(const auto& category : categories) {
		UiButton& button = category_buttons_.Add(new UiButton());
		button.SetText(Format("%s (%d)", category.display_name, category.icon_count))
			.SetContentInset(DPI(6))
			.SetContentGap(DPI(8));
		category_content_layout_.Add(button).Fit().AlignSelf(UiBoxLayout::Align::Stretch);
		String category_id = category.id;
		button.WhenAction = [=] {
			if(model_ && commands_)
				commands_->Execute(MakeSymbolPickerSetCategoryCommand(category_id), *model_);
		};
	}
}

void SymbolPickerView::RebuildLibraryTiles()
{
	library_content_layout_.ClearItems();
	library_tiles_.Clear();

	if(!catalog_ || !model_)
		return;

	Vector<int> rows = catalog_->Filter(model_->GetCurrentCategory(), model_->GetFilterText(), model_->GetIconStyle());
	for(int row : rows) {
		const SymbolPickerIconEntry& entry = catalog_->GetIcons()[row];
		UiButton& tile = library_tiles_.Add(new UiButton());
		tile.SetText(Format("%s  |  %s  |  %s", entry.display_name, entry.category, SymbolPickerIconStyleText(entry.style)))
			.SetContentInset(DPI(6))
			.SetContentGap(DPI(8));
		library_content_layout_.Add(tile).Fit().AlignSelf(UiBoxLayout::Align::Stretch);

		String catalog_id = entry.catalog_id;
		tile.WhenAction = [=] {
			SelectLibraryCatalogId(catalog_id);
		};
	}
}

void SymbolPickerView::RebuildCollectionTiles()
{
	collections_content_layout_.ClearItems();
	collection_tiles_.Clear();

	UiLabel& bin_label = collection_tiles_.Add(new UiLabel());
	int bin_count = model_ ? model_->GetBinIconIds().GetCount() : 0;
	bin_label.SetText(Format("Bin items: %d", bin_count));
	collections_content_layout_.Add(bin_label).Fit().AlignSelf(UiBoxLayout::Align::Stretch);

	if(!model_)
		return;
	const SymbolPickerCollection* collection = model_->GetActiveCollection();
	if(!collection) {
		UiLabel& empty = collection_tiles_.Add(new UiLabel());
		empty.SetText("No active collection.");
		collections_content_layout_.Add(empty).Fit().AlignSelf(UiBoxLayout::Align::Stretch);
		return;
	}

	for(const auto& item : collection->items) {
		UiLabel& row = collection_tiles_.Add(new UiLabel());
		row.SetText(Format("%s  |  %s  |  %s",
			item.catalog_id.IsEmpty() ? String("(missing catalog_id)") : item.catalog_id,
			item.source_id,
			item.alias));
		collections_content_layout_.Add(row).Fit().AlignSelf(UiBoxLayout::Align::Stretch);
	}
}

void SymbolPickerView::SelectLibraryCatalogId(const String& catalog_id)
{
	selected_library_catalog_id_ = catalog_id;
	String subtitle = catalog_id.IsEmpty() ? "Browse the filtered runtime catalog." : "Selected: " + catalog_id;
	library_card_.SetSubTitle(subtitle);
}

String SymbolPickerView::MakeCollectionAlias(const SymbolPickerIconEntry& entry) const
{
	String alias = "ICON_" + SafeAliasPart(entry.category) + "_" + SafeAliasPart(entry.display_name) + "_" + SafeAliasPart(SymbolPickerIconStyleText(entry.style));
	return alias;
}

void SymbolPickerView::RefreshFromModel()
{
	if(!model_)
		return;

	theme_preset_drop_ <<= (int)model_->GetThemePreset();
	icon_style_drop_ <<= (int)model_->GetIconStyle();
	filter_edit_.SetTextUtf8(model_->GetFilterText());
	tint_ctrl_.SetColor(model_->GetTintColor());

	RefreshCategories();
	RefreshLibrary();
	RefreshCollections();
	RefreshCollectionItems();

	String heading_subtitle = Format("Theme: %d | Icon style: %s | Category: %s | Bin: %d",
		(int)model_->GetThemePreset(),
		SymbolPickerIconStyleText(model_->GetIconStyle()),
		model_->GetCurrentCategory(),
		model_->GetBinIconIds().GetCount());
	heading_card_.SetSubTitle(heading_subtitle);

	const SymbolPickerCollection* active = model_->GetActiveCollection();
	if(active)
		collections_card_.SetSubTitle(Format("%s | %d items", active->name, active->items.GetCount()));
	else
		collections_card_.SetSubTitle("No active collection.");

	if(catalog_ && !selected_library_catalog_id_.IsEmpty() && !catalog_->FindByCatalogId(selected_library_catalog_id_))
		selected_library_catalog_id_.Clear();
	SelectLibraryCatalogId(selected_library_catalog_id_);
}

}
