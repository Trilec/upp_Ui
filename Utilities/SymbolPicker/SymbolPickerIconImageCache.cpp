#include "SymbolPickerIconImageCache.h"

#include "SymbolPickerImageRender.h"

namespace Upp {

SymbolPickerIconImageCache::SymbolPickerIconImageCache()
{
}

void SymbolPickerIconImageCache::SetMaxEntries(int max_entries)
{
	max_entries_ = max(16, max_entries);
	Trim();
}

String SymbolPickerIconImageCache::MakeKey(const SymbolPickerIconEntry& entry, int pixel_size, Color tint) const
{
	return Format("%s|%d|%d|%d|%d|%d",
		entry.catalog_id,
		max(1, pixel_size),
		IsNull(tint) ? -1 : tint.GetR(),
		IsNull(tint) ? -1 : tint.GetG(),
		IsNull(tint) ? -1 : tint.GetB(),
		(int)entry.style);
}

Image SymbolPickerIconImageCache::RenderImage(const SymbolPickerIconEntry& entry, int pixel_size, Color tint) const
{
	return RenderSymbolPickerIconImage(entry, pixel_size, tint);
}

void SymbolPickerIconImageCache::Touch(int index)
{
	items_[index].stamp = ++stamp_;
}

void SymbolPickerIconImageCache::Trim()
{
	while(items_.GetCount() > max_entries_) {
		int oldest = 0;
		for(int i = 1; i < items_.GetCount(); ++i)
			if(items_[i].stamp < items_[oldest].stamp)
				oldest = i;
		lookup_.RemoveKey(items_[oldest].key);
		items_.Remove(oldest);
		for(int i = oldest; i < items_.GetCount(); ++i) {
			int q = lookup_.Find(items_[i].key);
			if(q >= 0)
				lookup_[q] = i;
		}
	}
}

Image SymbolPickerIconImageCache::GetImage(const SymbolPickerIconEntry& entry, int pixel_size, Color tint)
{
	String key = MakeKey(entry, pixel_size, tint);
	int q = lookup_.Find(key);
	if(q >= 0) {
		int i = lookup_[q];
		if(i >= 0 && i < items_.GetCount() && items_[i].key == key) {
			++hit_count_;
			Touch(i);
			return items_[i].image;
		}
	}

	++miss_count_;
	CacheItem& item = items_.Add();
	item.key = key;
	item.image = RenderImage(entry, pixel_size, tint);
	item.stamp = ++stamp_;
	lookup_.GetAdd(key, items_.GetCount() - 1) = items_.GetCount() - 1;
	Trim();
	return item.image;
}

void SymbolPickerIconImageCache::Clear()
{
	items_.Clear();
	lookup_.Clear();
	stamp_ = 0;
	hit_count_ = 0;
	miss_count_ = 0;
}

}
