#ifndef _DesignerRecentDocuments_h_
#define _DesignerRecentDocuments_h_

#include <Core/Core.h>

namespace Upp {

class UiSplitButton;

class DesignerRecentDocuments {
public:
	static constexpr int MAX_RECENT = 10;

	void AddRecentDesignerDocument(const String& path);
	void LoadRecentDesignerDocuments(const Value& value);
	Value StoreRecentDesignerDocuments() const;
	void RemoveMissingDesignerDocuments();
	bool HasMissingDesignerDocuments() const;

	const Vector<String>& Get() const { return recent_; }
	void Clear() { recent_.Clear(); }

private:
	Vector<String> recent_;
};

void RefreshRecentDocumentMenus(UiSplitButton& save_button, UiSplitButton& load_button,
	                           const DesignerRecentDocuments& recents);

} // namespace Upp

#endif
