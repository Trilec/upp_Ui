#include "SymbolPickerModel.h"

namespace Upp {

static int FindStringIndex(const Vector<String>& values, const String& value)
{
	for(int i = 0; i < values.GetCount(); i++)
		if(values[i] == value)
			return i;
	return -1;
}

static SymbolPickerIconRef CopyIconRef(const SymbolPickerIconRef& src)
{
	SymbolPickerIconRef out;
	out.catalog_id = src.catalog_id;
	out.source_id = src.source_id;
	out.alias = src.alias;
	out.size = src.size;
	out.tint = src.tint;
	out.unresolved = src.unresolved;
	return out;
}

void SymbolPickerModel::Changed()
{
	WhenChanged();
}

bool SymbolPickerModel::SetThemePreset(UiThemePreset preset)
{
	if(theme_preset_ == preset)
		return false;
	theme_preset_ = preset;
	Changed();
	return true;
}

bool SymbolPickerModel::SetIconStyle(SymbolPickerIconStyle style)
{
	if(icon_style_ == style)
		return false;
	icon_style_ = style;
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

bool SymbolPickerModel::AddIconToBin(const String& id)
{
	String next = TrimBoth(id);
	if(next.IsEmpty() || FindStringIndex(bin_icon_ids_, next) >= 0)
		return false;
	bin_icon_ids_.Add(next);
	Changed();
	return true;
}

bool SymbolPickerModel::RemoveIconFromBin(const String& id)
{
	int q = FindStringIndex(bin_icon_ids_, id);
	if(q < 0)
		return false;
	bin_icon_ids_.Remove(q);
	Changed();
	return true;
}

bool SymbolPickerModel::ClearBin()
{
	if(bin_icon_ids_.IsEmpty())
		return false;
	bin_icon_ids_.Clear();
	Changed();
	return true;
}

int SymbolPickerModel::CreateCollection(const String& name, const String& file_path)
{
	SymbolPickerCollection& collection = collections_.Add();
	collection.name = TrimBoth(name);
	if(collection.name.IsEmpty())
		collection.name = Format("Collection %d", collections_.GetCount());
	collection.file_path = file_path;
	collection.dirty = true;
	if(active_collection_index_ < 0)
		active_collection_index_ = collections_.GetCount() - 1;
	Changed();
	return collections_.GetCount() - 1;
}

bool SymbolPickerModel::RemoveCollection(int index)
{
	if(!IsValidCollectionIndex(index))
		return false;
	collections_.Remove(index);
	if(collections_.IsEmpty())
		active_collection_index_ = -1;
	else if(active_collection_index_ >= collections_.GetCount())
		active_collection_index_ = collections_.GetCount() - 1;
	else if(active_collection_index_ > index)
		--active_collection_index_;
	Changed();
	return true;
}

bool SymbolPickerModel::RenameCollection(int index, const String& name)
{
	if(!IsValidCollectionIndex(index))
		return false;
	String next = TrimBoth(name);
	if(next.IsEmpty())
		return false;
	if(collections_[index].name == next)
		return false;
	collections_[index].name = next;
	collections_[index].dirty = true;
	Changed();
	return true;
}

bool SymbolPickerModel::SetActiveCollection(int index)
{
	if(index == -1 && collections_.IsEmpty()) {
		if(active_collection_index_ == -1)
			return false;
		active_collection_index_ = -1;
		Changed();
		return true;
	}
	if(!IsValidCollectionIndex(index) || active_collection_index_ == index)
		return false;
	active_collection_index_ = index;
	Changed();
	return true;
}

bool SymbolPickerModel::AddIconToCollection(int collection_index, const SymbolPickerIconRef& ref)
{
	if(!IsValidCollectionIndex(collection_index))
		return false;
	SymbolPickerIconRef copy = CopyIconRef(ref);
	if(copy.catalog_id.IsEmpty() && !copy.source_id.IsEmpty())
		copy.unresolved = true;
	collections_[collection_index].items.Add(copy);
	collections_[collection_index].dirty = true;
	Changed();
	return true;
}

bool SymbolPickerModel::RemoveIconFromCollection(int collection_index, int item_index)
{
	if(!IsValidItemIndex(collection_index, item_index))
		return false;
	collections_[collection_index].items.Remove(item_index);
	collections_[collection_index].dirty = true;
	Changed();
	return true;
}

bool SymbolPickerModel::ClearCollection(int collection_index)
{
	if(!IsValidCollectionIndex(collection_index) || collections_[collection_index].items.IsEmpty())
		return false;
	collections_[collection_index].items.Clear();
	collections_[collection_index].dirty = true;
	Changed();
	return true;
}

bool SymbolPickerModel::RenameCollectionIconAlias(int collection_index, int item_index, const String& alias)
{
	if(!IsValidItemIndex(collection_index, item_index))
		return false;
	String next = alias;
	if(collections_[collection_index].items[item_index].alias == next)
		return false;
	collections_[collection_index].items[item_index].alias = next;
	collections_[collection_index].dirty = true;
	Changed();
	return true;
}

const SymbolPickerCollection* SymbolPickerModel::GetActiveCollection() const
{
	return IsValidCollectionIndex(active_collection_index_) ? &collections_[active_collection_index_] : nullptr;
}

int SymbolPickerModel::FindBinIconIndex(const String& id) const
{
	return FindStringIndex(bin_icon_ids_, id);
}

bool SymbolPickerModel::IsValidCollectionIndex(int index) const
{
	return index >= 0 && index < collections_.GetCount();
}

bool SymbolPickerModel::IsValidItemIndex(int collection_index, int item_index) const
{
	return IsValidCollectionIndex(collection_index)
		&& item_index >= 0
		&& item_index < collections_[collection_index].items.GetCount();
}

}
