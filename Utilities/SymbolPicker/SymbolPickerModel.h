#ifndef _Utilities_SymbolPicker_SymbolPickerModel_h_
#define _Utilities_SymbolPicker_SymbolPickerModel_h_

#include <Ui/Ui.h>

namespace Upp {

enum class SymbolPickerExportType : byte {
	ImageCall,
	IconId,
	CppSnippet,
};

enum class SymbolPickerIconStyle : byte {
	Outlined,
	Rounded,
	Sharp,
};

struct SymbolPickerIconRef : Moveable<SymbolPickerIconRef> {
	String catalog_id;
	String source_id;
	String alias;
	int    size = 24;
	Color  tint = Null;
	bool   unresolved = false;
};

struct SymbolPickerCollection : Moveable<SymbolPickerCollection> {
	String                      name;
	String                      file_path;
	Vector<SymbolPickerIconRef> items;
	bool                        dirty = false;
};

class SymbolPickerModel {
public:
	bool SetThemePreset(UiThemePreset preset);
	bool SetIconStyle(SymbolPickerIconStyle style);
	bool SetCurrentCategory(const String& category);
	bool SetFilterText(const String& text);
	bool SetTintColor(Color color);
	bool SetExportType(SymbolPickerExportType type);
	bool SetExportSize(int px);
	bool AddIconToBin(const String& id);
	bool RemoveIconFromBin(const String& id);
	bool ClearBin();

	int  CreateCollection(const String& name, const String& file_path = String());
	bool RemoveCollection(int index);
	bool RenameCollection(int index, const String& name);
	bool SetActiveCollection(int index);
	bool AddIconToCollection(int collection_index, const SymbolPickerIconRef& ref);
	bool RemoveIconFromCollection(int collection_index, int item_index);
	bool ClearCollection(int collection_index);
	bool RenameCollectionIconAlias(int collection_index, int item_index, const String& alias);

	UiThemePreset GetThemePreset() const { return theme_preset_; }
	SymbolPickerIconStyle GetIconStyle() const { return icon_style_; }
	const String& GetCurrentCategory() const { return current_category_; }
	const String& GetFilterText() const { return filter_text_; }
	Color GetTintColor() const { return tint_color_; }
	SymbolPickerExportType GetExportType() const { return export_type_; }
	int GetExportSize() const { return export_size_; }
	const Vector<String>& GetBinIconIds() const { return bin_icon_ids_; }
	const Vector<SymbolPickerCollection>& GetCollections() const { return collections_; }
	int GetActiveCollectionIndex() const { return active_collection_index_; }
	const SymbolPickerCollection* GetActiveCollection() const;
	int FindBinIconIndex(const String& id) const;
	bool IsValidCollectionIndex(int index) const;
	bool IsValidItemIndex(int collection_index, int item_index) const;

	Event<> WhenChanged;

private:
	void Changed();

	UiThemePreset theme_preset_ = UiThemePreset::Minimal;
	SymbolPickerIconStyle icon_style_ = SymbolPickerIconStyle::Outlined;
	String current_category_ = "All";
	String filter_text_;
	Color tint_color_ = Null;
	SymbolPickerExportType export_type_ = SymbolPickerExportType::ImageCall;
	int export_size_ = 48;
	Vector<String> bin_icon_ids_;
	Vector<SymbolPickerCollection> collections_;
	int active_collection_index_ = -1;
};

}

#endif
