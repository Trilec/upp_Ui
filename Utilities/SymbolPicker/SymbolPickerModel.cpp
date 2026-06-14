#include "SymbolPickerModel.h"

namespace Upp {

static int FindStringIndex(const Vector<String>& values, const String& value)
{
	for(int i = 0; i < values.GetCount(); i++)
		if(values[i] == value)
			return i;
	return -1;
}

void SymbolPickerModel::Changed()
{
	WhenChanged();
}

bool SymbolPickerModel::SetCurrentStyle(UiThemePreset preset)
{
	if(current_style_ == preset)
		return false;
	current_style_ = preset;
	Changed();
	return true;
}

bool SymbolPickerModel::SetCurrentCategory(const String& category)
{
	String next = TrimBoth(category);
	if(next.IsEmpty())
		next = "All";
	if(current_category_ == next)
		return false;
	current_category_ = next;
	Changed();
	return true;
}

bool SymbolPickerModel::SetFilterText(const String& text)
{
	String next = text;
	if(filter_text_ == next)
		return false;
	filter_text_ = next;
	Changed();
	return true;
}

bool SymbolPickerModel::SetTintColor(Color color)
{
	if(tint_color_ == color)
		return false;
	tint_color_ = color;
	Changed();
	return true;
}

bool SymbolPickerModel::SetExportType(SymbolPickerExportType type)
{
	if(export_type_ == type)
		return false;
	export_type_ = type;
	Changed();
	return true;
}

bool SymbolPickerModel::SetExportSize(int px)
{
	int next = max(1, px);
	if(export_size_ == next)
		return false;
	export_size_ = next;
	Changed();
	return true;
}

bool SymbolPickerModel::AddSelectedIconId(const String& id)
{
	String next = TrimBoth(id);
	if(next.IsEmpty() || FindStringIndex(selected_icon_ids_, next) >= 0)
		return false;
	selected_icon_ids_.Add(next);
	Changed();
	return true;
}

bool SymbolPickerModel::RemoveSelectedIconId(const String& id)
{
	int q = FindStringIndex(selected_icon_ids_, id);
	if(q < 0)
		return false;
	selected_icon_ids_.Remove(q);
	Changed();
	return true;
}

bool SymbolPickerModel::ClearSelectedIconIds()
{
	if(selected_icon_ids_.IsEmpty())
		return false;
	selected_icon_ids_.Clear();
	Changed();
	return true;
}

}
