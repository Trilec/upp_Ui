#ifndef _Utilities_SymbolPicker_SymbolPickerModel_h_
#define _Utilities_SymbolPicker_SymbolPickerModel_h_

#include <Ui/Ui.h>

namespace Upp {

enum class SymbolPickerExportType : byte {
	ImageCall,
	IconId,
	CppSnippet,
	PngFiles,
	SvgFiles,
	UppRawHeader,
	UppRleHeader,
	UppIml,
	UppImlLibrary,
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
	String comment;
	String category_override;
	SymbolPickerIconStyle style_override = SymbolPickerIconStyle::Outlined;
	bool   has_style_override = false;
	bool   unresolved = false;
};

struct SymbolPickerCollection : Moveable<SymbolPickerCollection> {
	String                      name;
	String                      comment;
	String                      file_path;
	Vector<SymbolPickerIconRef> items;
	bool                        dirty = false;
};

struct SymbolPickerProject : Moveable<SymbolPickerProject> {
	String                        project_name;
	String                        comment;
	String                        file_path;
	String                        output_base_name;
	String                        symbol_prefix;
	int                           default_size = 48;
	Color                         default_tint = Null;
	SymbolPickerIconStyle         default_style = SymbolPickerIconStyle::Outlined;
	Vector<SymbolPickerCollection> collections;
	int                           active_collection_index = -1;
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
	bool MoveIconInCollection(int collection_index, int from_index, int to_index);
	bool ClearCollection(int collection_index);
	bool RenameCollectionIconAlias(int collection_index, int item_index, const String& alias);
	bool SetProjectName(const String& name);
	bool SetProjectComment(const String& comment);
	bool SetProjectFilePath(const String& path);
	bool SetOutputBaseName(const String& name);
	bool SetSymbolPrefix(const String& prefix);
	void MarkCollectionsSaved();
	SymbolPickerProject ExportProject() const;
	bool LoadProject(const SymbolPickerProject& project);

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
	const String& GetProjectName() const { return project_name_; }
	const String& GetProjectComment() const { return project_comment_; }
	const String& GetProjectFilePath() const { return project_file_path_; }
	const String& GetOutputBaseName() const { return output_base_name_; }
	const String& GetSymbolPrefix() const { return symbol_prefix_; }
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
	String project_name_;
	String project_comment_;
	String project_file_path_;
	String output_base_name_ = "symbols";
	String symbol_prefix_ = "ICON_";
};

}

#endif