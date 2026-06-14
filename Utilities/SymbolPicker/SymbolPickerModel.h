#ifndef _Utilities_SymbolPicker_SymbolPickerModel_h_
#define _Utilities_SymbolPicker_SymbolPickerModel_h_

#include <Ui/Ui.h>

namespace Upp {

enum class SymbolPickerExportType : byte {
	ImageCall,
	IconId,
	CppSnippet,
};

class SymbolPickerModel {
public:
	bool SetCurrentStyle(UiThemePreset preset);
	bool SetCurrentCategory(const String& category);
	bool SetFilterText(const String& text);
	bool SetTintColor(Color color);
	bool SetExportType(SymbolPickerExportType type);
	bool SetExportSize(int px);
	bool AddSelectedIconId(const String& id);
	bool RemoveSelectedIconId(const String& id);
	bool ClearSelectedIconIds();

	UiThemePreset GetCurrentStyle() const { return current_style_; }
	const String& GetCurrentCategory() const { return current_category_; }
	const String& GetFilterText() const { return filter_text_; }
	Color GetTintColor() const { return tint_color_; }
	SymbolPickerExportType GetExportType() const { return export_type_; }
	int GetExportSize() const { return export_size_; }
	const Vector<String>& GetSelectedIconIds() const { return selected_icon_ids_; }

	Event<> WhenChanged;

private:
	void Changed();

	UiThemePreset current_style_ = UiThemePreset::Minimal;
	String current_category_ = "All";
	String filter_text_;
	Color tint_color_ = Null;
	SymbolPickerExportType export_type_ = SymbolPickerExportType::ImageCall;
	int export_size_ = 48;
	Vector<String> selected_icon_ids_;
};

}

#endif
