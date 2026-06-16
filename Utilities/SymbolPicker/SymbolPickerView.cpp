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
	Add(root_.SizePos());
	root_.SetFrame(InsetFrame());

	BuildHeader();
	BuildCategoryStrip();
	BuildLibraryPanel();
	BuildCollectionsPanel();
	BuildBinPanel();

	center_split_.Horz();
	center_split_ << library_panel_ << collections_panel_;
	center_split_.SetPos(6000);
	center_host_.Add(center_split_.SizePos());

	main_split_.Vert();
	main_split_ << center_host_ << bin_panel_;
	main_split_.SetPos(7200);

	root_.Add(header_row_.HSizePosZ(0, 0).TopPosZ(0, 72));
	root_.Add(category_strip_.HSizePosZ(0, 0).TopPosZ(76, 60));
	root_.Add(main_split_.HSizePosZ(0, 0).VSizePosZ(140, 0));
}

void SymbolPickerView::BuildHeader()
{
	header_row_.Add(title_.LeftPosZ(12, 260).TopPosZ(8, 22));
	header_row_.Add(subtitle_.LeftPosZ(12, 420).TopPosZ(32, 18));
	header_row_.Add(theme_preset_drop_.LeftPosZ(420, 120).TopPosZ(14, 24));
	header_row_.Add(icon_style_drop_.LeftPosZ(548, 120).TopPosZ(14, 24));
	header_row_.Add(filter_label_.LeftPosZ(676, 60).TopPosZ(0, 12));
	header_row_.Add(filter_edit_.LeftPosZ(676, 180).TopPosZ(14, 24));
	header_row_.Add(tint_ctrl_.RightPosZ(148, 108).TopPosZ(14, 24));
	header_row_.Add(new_collection_button_.RightPosZ(12, 128).TopPosZ(14, 24));
	header_row_.Add(clear_bin_button_.RightPosZ(12, 96).TopPosZ(42, 22));

	title_.SetLabel("Symbol Picker");
	title_.SetFont(ArialZ(20).Bold());
	subtitle_.SetLabel("v0.2 layout and collection-model foundation");
	subtitle_.SetFont(StdFont());
	filter_label_.SetLabel("Filter");
	filter_label_.SetFont(StdFont());

	theme_preset_drop_.Add((int)UiThemePreset::Minimal, "Minimal");
	theme_preset_drop_.Add((int)UiThemePreset::Pill, "Pill");
	theme_preset_drop_.Add((int)UiThemePreset::Layered, "Layered");

	icon_style_drop_.Add((int)SymbolPickerIconStyle::Outlined, "Outlined");
	icon_style_drop_.Add((int)SymbolPickerIconStyle::Rounded,  "Rounded");
	icon_style_drop_.Add((int)SymbolPickerIconStyle::Sharp,    "Sharp");

	filter_edit_.SetText(String());

	new_collection_button_.SetLabel("New Collection");
	clear_bin_button_.SetLabel("Clear Bin");

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
			commands_->Execute(MakeSymbolPickerSetFilterCommand(~filter_edit_), *model_);
	};
	new_collection_button_.WhenAction = [=] {
		if(model_ && commands_)
			commands_->Execute(MakeSymbolPickerCreateCollectionCommand(Format("Collection %d", model_->GetCollections().GetCount() + 1)), *model_);
	};
	clear_bin_button_.WhenAction = [=] {
		if(model_ && commands_)
			commands_->Execute(MakeSymbolPickerClearBinCommand(), *model_);
	};
}

void SymbolPickerView::BuildCategoryStrip()
{
	category_strip_.Add(categories_list_.SizePos());
	categories_list_.AddColumn("Categories");
	categories_list_.WhenSel = [=] {
		if(model_ && commands_ && categories_list_.IsCursor())
			commands_->Execute(MakeSymbolPickerSetCategoryCommand(categories_list_.Get(categories_list_.GetCursor(), 1)), *model_);
	};
}

void SymbolPickerView::BuildLibraryPanel()
{
	library_panel_.Add(library_title_.LeftPosZ(12, 140).TopPosZ(8, 20));
	library_panel_.Add(library_subtitle_.LeftPosZ(12, 240).TopPosZ(28, 16));
	library_panel_.Add(add_to_bin_button_.RightPosZ(128, 110).TopPosZ(8, 22));
	library_panel_.Add(add_to_collection_button_.RightPosZ(12, 110).TopPosZ(8, 22));
	library_panel_.Add(library_list_.HSizePosZ(12, 12).VSizePosZ(52, 12));

	library_title_.SetLabel("Library");
	library_title_.SetFont(ArialZ(16).Bold());
	library_subtitle_.SetLabel("Browse the full icon library.");
	add_to_bin_button_.SetLabel("Add To Bin");
	add_to_collection_button_.SetLabel("Add To Collection");

	library_list_.AddColumn("Name");
	library_list_.AddColumn("Category");
	library_list_.AddColumn("Style");
	library_list_.AddColumn("Catalog Id");
	library_list_.AddColumn("Source Id");
	library_list_.WhenLeftDouble = [=] {
		if(!model_ || !catalog_ || !commands_ || !library_list_.IsCursor())
			return;
		String catalog_id = library_list_.Get(library_list_.GetCursor(), 3);
		commands_->Execute(MakeSymbolPickerAddToBinCommand(catalog_id), *model_);
	};
	add_to_bin_button_.WhenAction = [=] {
		if(!model_ || !catalog_ || !commands_ || !library_list_.IsCursor())
			return;
		String catalog_id = library_list_.Get(library_list_.GetCursor(), 3);
		commands_->Execute(MakeSymbolPickerAddToBinCommand(catalog_id), *model_);
	};
	add_to_collection_button_.WhenAction = [=] {
		if(!model_ || !catalog_ || !commands_ || !library_list_.IsCursor())
			return;
		if(model_->GetActiveCollectionIndex() < 0)
			return;
		String catalog_id = library_list_.Get(library_list_.GetCursor(), 3);
		const SymbolPickerIconEntry* entry = catalog_->FindByCatalogId(catalog_id);
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

void SymbolPickerView::BuildCollectionsPanel()
{
	collections_panel_.Add(collections_title_.LeftPosZ(12, 140).TopPosZ(8, 20));
	collections_panel_.Add(collections_subtitle_.LeftPosZ(12, 220).TopPosZ(28, 16));
	collections_panel_.Add(collections_list_.LeftPosZ(12, 220).VSizePosZ(52, 12));
	collections_panel_.Add(collections_tabs_.HSizePosZ(240, 12).VSizePosZ(52, 12));

	collections_title_.SetLabel("Collections");
	collections_title_.SetFont(ArialZ(16).Bold());
	collections_subtitle_.SetLabel("Manage saved icon sets.");

	collections_list_.AddColumn("Collections");
	collections_list_.WhenSel = [=] {
		if(refreshing_collections_ || !model_ || !commands_ || !collections_list_.IsCursor())
			return;
		commands_->Execute(MakeSymbolPickerSetActiveCollectionCommand(collections_list_.GetCursor()), *model_);
	};

	collection_items_list_.AddColumn("Source Id");
	collection_items_list_.AddColumn("Alias");
	collection_items_list_.AddColumn("Size");
	collection_items_list_.AddColumn("Unresolved");
	collection_items_page_.Add(collection_items_list_.SizePos());
	collections_tabs_.Add(collection_items_page_.SizePos(), "Items");
}

void SymbolPickerView::BuildBinPanel()
{
	bin_panel_.Add(bin_title_.LeftPosZ(12, 80).TopPosZ(8, 20));
	bin_panel_.Add(bin_subtitle_.LeftPosZ(12, 240).TopPosZ(28, 16));
	bin_panel_.Add(bin_list_.HSizePosZ(12, 12).VSizePosZ(52, 12));

	bin_title_.SetLabel("Bin");
	bin_title_.SetFont(ArialZ(16).Bold());
	bin_subtitle_.SetLabel("Gather icons for export or reuse.");

	bin_list_.AddColumn("Bin Icons");
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
	refreshing_collections_ = true;
	collections_list_.Clear();
	if(model_) {
		const Vector<SymbolPickerCollection>& collections = model_->GetCollections();
		for(const auto& collection : collections)
			collections_list_.Add(collection.name + (collection.dirty ? " *" : String()));
		if(model_->GetActiveCollectionIndex() >= 0 && model_->GetActiveCollectionIndex() < collections.GetCount())
			collections_list_.SetCursor(model_->GetActiveCollectionIndex());
	}
	refreshing_collections_ = false;
}

void SymbolPickerView::RefreshCollectionItems()
{
	collection_items_list_.Clear();
	if(!model_)
		return;
	const SymbolPickerCollection* collection = model_->GetActiveCollection();
	if(!collection)
		return;
	for(const auto& item : collection->items)
		collection_items_list_.Add(item.catalog_id.IsEmpty() ? item.source_id : item.catalog_id, item.alias, item.size, item.unresolved ? "Yes" : "No");
}

void SymbolPickerView::RefreshBin()
{
	bin_list_.Clear();
	if(!model_)
		return;
	for(const String& id : model_->GetBinIconIds())
		bin_list_.Add(id);
}

void SymbolPickerView::RefreshCategories()
{
	categories_list_.Clear();
	categories_list_.Add("All", "All");
	if(!catalog_)
		return;
	Vector<SymbolPickerCategory> categories = catalog_->GetCategories();
	for(const auto& category : categories)
		categories_list_.Add(Format("%s (%d)", category.display_name, category.icon_count), category.id);

	String current = model_ ? model_->GetCurrentCategory() : String("All");
	for(int i = 0; i < categories_list_.GetCount(); ++i) {
		if(categories_list_.Get(i, 1) == current) {
			categories_list_.SetCursor(i);
			return;
		}
	}
	categories_list_.SetCursor(0);
}

void SymbolPickerView::RefreshLibrary()
{
	library_list_.Clear();
	if(!catalog_ || !model_)
		return;
	Vector<int> rows = catalog_->Filter(model_->GetCurrentCategory(), model_->GetFilterText(), model_->GetIconStyle());
	for(int row : rows) {
		const SymbolPickerIconEntry& entry = catalog_->GetIcons()[row];
		library_list_.Add(entry.display_name, entry.category, SymbolPickerIconStyleText(entry.style), entry.catalog_id, entry.source_id);
	}
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
	filter_edit_.SetText(model_->GetFilterText());
	tint_ctrl_.SetColor(model_->GetTintColor());
	RefreshCategories();
	RefreshLibrary();
	RefreshCollections();
	RefreshCollectionItems();
	RefreshBin();
	String sub = Format("Theme: %d | Icon style: %s | Category: %s",
		(int)model_->GetThemePreset(), SymbolPickerIconStyleText(model_->GetIconStyle()), model_->GetCurrentCategory());
	subtitle_.SetLabel(sub);
}

}
