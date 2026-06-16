#include "SymbolPickerCatalog.h"
#include "SymbolPickerCatalogSeed.h"

namespace Upp {

static SymbolPickerIconEntry CopyEntry(const SymbolPickerIconEntry& src)
{
	SymbolPickerIconEntry out;
	out.source_id = src.source_id;
	out.category = src.category;
	out.display_name = src.display_name;
	out.style = src.style;
	out.source_symbol = src.source_symbol;
	out.available = src.available;
	return out;
}

static String CategoryDisplayName(const String& id)
{
	String out;
	bool new_word = true;
	for(int i = 0; i < id.GetCount(); ++i) {
		int c = id[i];
		if(c == '_' || c == '/' || c == '-') {
			out.Cat(' ');
			new_word = true;
			continue;
		}
		out.Cat(new_word ? ToUpper((wchar)c) : c);
		new_word = false;
	}
	return out;
}

void SymbolPickerCatalog::Clear()
{
	icons_.Clear();
}

void SymbolPickerCatalog::Add(const SymbolPickerIconEntry& entry)
{
	icons_.Add(CopyEntry(entry));
}

const Vector<SymbolPickerIconEntry>& SymbolPickerCatalog::GetIcons() const
{
	return icons_;
}

Vector<SymbolPickerCategory> SymbolPickerCatalog::GetCategories() const
{
	VectorMap<String, int> counts;
	for(const auto& icon : icons_)
		counts.GetAdd(icon.category, 0)++;

	Vector<SymbolPickerCategory> out;
	for(int i = 0; i < counts.GetCount(); ++i) {
		SymbolPickerCategory& c = out.Add();
		c.id = counts.GetKey(i);
		c.display_name = CategoryDisplayName(c.id);
		c.icon_count = counts[i];
	}
	Sort(out, [](const SymbolPickerCategory& a, const SymbolPickerCategory& b) {
		return ToLower(a.display_name) < ToLower(b.display_name);
	});
	return out;
}

Vector<int> SymbolPickerCatalog::Filter(const String& category,
	const String& text,
	SymbolPickerIconStyle style) const
{
	String want_category = TrimBoth(category);
	String want_text = ToLower(TrimBoth(text));
	Vector<int> out;
	for(int i = 0; i < icons_.GetCount(); ++i) {
		const SymbolPickerIconEntry& icon = icons_[i];
		if(style != icon.style)
			continue;
		if(!want_category.IsEmpty() && want_category != "All" && icon.category != want_category)
			continue;
		if(!want_text.IsEmpty()) {
			String hay = ToLower(icon.display_name + " " + icon.source_id + " " + icon.category);
			if(hay.Find(want_text) < 0)
				continue;
		}
		out.Add(i);
	}
	return out;
}

const SymbolPickerIconEntry* SymbolPickerCatalog::FindBySourceId(const String& source_id) const
{
	for(const auto& icon : icons_)
		if(icon.source_id == source_id)
			return &icon;
	return nullptr;
}

bool RunSymbolPickerCatalogSmokeTests(String& error)
{
	auto Fail = [&](const String& msg) {
		error = msg;
		return false;
	};

	SymbolPickerCatalog catalog;
	SeedSymbolPickerCatalog(catalog);

	if(catalog.GetIcons().GetCount() != 24)
		return Fail("Catalog did not retain seeded icons.");

	Vector<SymbolPickerCategory> categories = catalog.GetCategories();
	if(categories.IsEmpty())
		return Fail("Catalog categories are empty.");
	bool has_action = false;
	bool has_navigation = false;
	bool has_alert = false;
	for(const auto& category : categories) {
		has_action |= category.id == "action";
		has_navigation |= category.id == "navigation";
		has_alert |= category.id == "alert";
	}
	if(!has_action || !has_navigation || !has_alert)
		return Fail("Catalog categories are missing expected seeded groups.");

	Vector<int> filtered_category = catalog.Filter("action", String(), SymbolPickerIconStyle::Outlined);
	if(filtered_category.GetCount() != 3)
		return Fail("Catalog category filtering failed.");

	Vector<int> filtered_text = catalog.Filter("All", "warn", SymbolPickerIconStyle::Sharp);
	if(filtered_text.GetCount() != 1)
		return Fail("Catalog text filtering failed.");

	Vector<int> filtered_style = catalog.Filter("All", String(), SymbolPickerIconStyle::Rounded);
	if(filtered_style.GetCount() != 8)
		return Fail("Catalog style filtering failed.");

	if(!catalog.FindBySourceId("action/save"))
		return Fail("FindBySourceId did not find existing id.");
	if(catalog.FindBySourceId("missing/id"))
		return Fail("FindBySourceId should return null for missing id.");

	return true;
}

}
