#ifndef _Utilities_SymbolPicker_SymbolPickerIconImageCache_h_
#define _Utilities_SymbolPicker_SymbolPickerIconImageCache_h_

#include "SymbolPickerCatalog.h"

namespace Upp {

class SymbolPickerIconImageCache {
public:
	SymbolPickerIconImageCache();

	void SetMaxEntries(int max_entries);
	Image GetImage(const SymbolPickerIconEntry& entry, int pixel_size, Color tint);
	void Clear();

private:
	struct CacheItem : Moveable<CacheItem> {
		String key;
		Image  image;
		int    stamp = 0;
	};

	String MakeKey(const SymbolPickerIconEntry& entry, int pixel_size, Color tint) const;
	Image RenderImage(const SymbolPickerIconEntry& entry, int pixel_size, Color tint) const;
	void Touch(int index);
	void Trim();

	Vector<CacheItem> items_;
	int max_entries_ = 256;
	int stamp_ = 0;
};

}

#endif
