#include "SymbolPickerView.h"

namespace Upp {

void SymbolPickerView::Layout()
{
	// Layout can be requested while the TopWindow is still being constructed.
	// Wait until BuildUi() has populated the section rows before touching them.
	if(category_header_layout_.GetItemCount() < 3 ||
	   library_header_layout_.GetItemCount() < 3 ||
	   collections_header_layout_.GetItemCount() < 3)
		return;

	if(!responsive_layout_initialized_) {
		responsive_layout_initialized_ = true;
		version_label_.SetText("v0.3.8");

		// Categories do not need the 45px section-heading reserve used by the
		// symbol/collection headers. Keep the heading close to its natural
		// text/media height so the category flow starts directly underneath it.
		category_base_layout_.SetGap(DPI(4));
		category_card_.SetMinSize(Size(DPI(180), DPI(32)));
		category_header_layout_.ItemAt(0).MinCross(DPI(32));
	}

	UpdateResponsiveHeaderHeights();
}

void SymbolPickerView::UpdateResponsiveHeaderHeights()
{
	// main_box_ contributes 8px on each side and each section base layout
	// contributes another 8px on each side. The resulting width is the width
	// available to the section header row.
	const int header_width = max(DPI(1), GetSize().cx - DPI(32));

	auto RequiredHeaderHeight = [&](UiTitleCard& card, UiBoxLayout& actions) {
		const int card_width = max(DPI(180), card.GetMinSize().cx);
		// Header row = title card + expanding spacer(min 8) + action cluster,
		// with two 8px inter-item gaps. The outer row itself must stay on one
		// line; only the action cluster is allowed to Flow internally.
		int action_width = max(DPI(1), header_width - card_width - DPI(24));
		action_width = max(action_width, actions.GetMinWrapWidth());
		return max(card.GetMinSize().cy, actions.MeasureHeightForWidth(action_width));
	};

	const int library_height = RequiredHeaderHeight(library_card_, library_action_cluster_);
	if(library_height != library_header_min_height_) {
		library_header_min_height_ = library_height;
		library_base_layout_.ItemAt(0).MinMain(library_height);
	}

	const int collections_height = RequiredHeaderHeight(collections_card_, collections_action_cluster_);
	if(collections_height != collections_header_min_height_) {
		collections_header_min_height_ = collections_height;
		collections_base_layout_.ItemAt(0).MinMain(collections_height);
	}
}

} // namespace Upp
