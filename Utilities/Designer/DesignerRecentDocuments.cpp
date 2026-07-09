#include "DesignerRecentDocuments.h"
#include <Ui/Ui.h>

namespace Upp {

void DesignerRecentDocuments::AddRecentDesignerDocument(const String& path)
{
	String p = NormalizePath(path);
	if(p.IsEmpty())
		return;
	for(int i = recent_.GetCount() - 1; i >= 0; i--)
		if(NormalizePath(recent_[i]) == p)
			recent_.Remove(i);
	recent_.Insert(0, p);
	if(recent_.GetCount() > MAX_RECENT)
		recent_.SetCount(MAX_RECENT);
}

void DesignerRecentDocuments::LoadRecentDesignerDocuments(const Value& value)
{
	if(!value.Is<ValueArray>())
		return;
	ValueArray items = value;
	for(int i = items.GetCount() - 1; i >= 0; i--)
		if(!IsNull(items[i]))
			AddRecentDesignerDocument(AsString(items[i]));
}

Value DesignerRecentDocuments::StoreRecentDesignerDocuments() const
{
	ValueArray out;
	for(const String& path : recent_)
		out.Add(path);
	return out;
}

void RefreshRecentDocumentMenus(UiSplitButton& save_button, UiSplitButton& load_button,
	                           const DesignerRecentDocuments& recents)
{
	auto fill = [&](UiSplitButton& button, bool is_save) {
		button.ClearItems();
		button.Add(is_save ? "Save As..." : "Open...", is_save ? "cmd:save_as" : "cmd:open");
		button.AddSeparator();
		if(is_save)
			button.Add("Open containing folder", "cmd:open_folder", false);
		button.AddGroupHeader("Recent documents");
		if(recents.Get().IsEmpty())
			button.Add("No recent documents", Value(), false);
		else {
			for(const String& path : recents.Get()) {
				bool ok = FileExists(path);
				String text = GetFileName(path);
				if(!ok)
					text << " (missing)";
				button.Add(text, path, ok);
				button.SetItemDescription(button.GetCount() - 1, path);
				button.SetItemEnabled(button.GetCount() - 1, ok);
			}
		}
	};
	fill(save_button, true);
	fill(load_button, false);
}

} // namespace Upp
