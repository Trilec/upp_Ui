#include "SymbolPickerIconImageCache.h"

#include "SymbolPickerGeneratedCatalog.h"

namespace Upp {

static Image TintIconCached(const Image& src, Color col)
{
	if(src.IsEmpty() || IsNull(col))
		return src;

	Size sz = src.GetSize();
	ImageBuffer ib(sz);
	ib.SetKind(src.GetKind());

	for(int y = 0; y < sz.cy; ++y) {
		const RGBA* srow = src[y];
		RGBA* drow = ib[y];
		for(int x = 0; x < sz.cx; ++x) {
			const RGBA& s = srow[x];
			RGBA& d = drow[x];

			int lum = (int)((54 * s.r + 183 * s.g + 19 * s.b + 128) >> 8);
			int darkness = 255 - lum;
			int a = (s.a * darkness + 127) / 255;

			d.r = (byte)((col.GetR() * a + 127) / 255);
			d.g = (byte)((col.GetG() * a + 127) / 255);
			d.b = (byte)((col.GetB() * a + 127) / 255);
			d.a = a;
		}
	}

	return Image(ib);
}

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
	String svg_xml;
	if(!DecodeGeneratedSymbolPickerSvg(entry.catalog_id, svg_xml) || svg_xml.IsEmpty())
		return Null;

	Image base = RenderSVGImage(Size(pixel_size, pixel_size), svg_xml, Black());
	if(base.IsEmpty())
		return Null;
	return TintIconCached(base, tint);
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
