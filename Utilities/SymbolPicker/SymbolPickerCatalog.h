#ifndef _Utilities_SymbolPicker_SymbolPickerCatalog_h_
#define _Utilities_SymbolPicker_SymbolPickerCatalog_h_

#include "SymbolPickerModel.h"

namespace Upp {

struct SymbolPickerIconEntry : Moveable<SymbolPickerIconEntry> {
	String                source_id;
	String                category;
	String                display_name;
	SymbolPickerIconStyle style = SymbolPickerIconStyle::Outlined;
	String                source_symbol;
	bool                  available = true;
};

struct SymbolPickerCategory : Moveable<SymbolPickerCategory> {
	String id;
	String display_name;
	int    icon_count = 0;
};

class SymbolPickerCatalog {
public:
	void Clear();
	void Add(const SymbolPickerIconEntry& entry);

	const Vector<SymbolPickerIconEntry>& GetIcons() const;
	Vector<SymbolPickerCategory> GetCategories() const;

	Vector<int> Filter(const String& category,
		const String& text,
		SymbolPickerIconStyle style) const;

	const SymbolPickerIconEntry* FindBySourceId(const String& source_id) const;

private:
	Vector<SymbolPickerIconEntry> icons_;
};

bool RunSymbolPickerCatalogSmokeTests(String& error);

}

#endif
