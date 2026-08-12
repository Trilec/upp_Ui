#include <Ui/UiMeasure.h>
#include <Ui/UiBoxLayout.h>
#include <Ui/UiGridLayout.h>
#include <Ui/UiPanel.h>
#include <Ui/UiTab.h>
#include <Ui/UiTree.h>

namespace Upp {

namespace {

Size MeasureUiTabForWidth(const UiTab& tab, int total_width)
{
	int active = tab.GetActiveTab();
	if(active < 0 || active >= tab.GetCount())
		return tab.GetMinSize();

	const UiTab::Style& style = tab.GetStyle();
	int width = max(1, total_width);

	// UiTab::Layout() first converts the outer rect to styled content, then
	// gives the active page the pane width. Mirror that geometry here so a
	// parent Fit item can ask how tall a wrapped active page is at this width.
	Rect probe = UiStyledInnerRect(RectC(0, 0, width, DPI(4096)), style.metrics, style.skin);
	int content_width = max(1, probe.GetWidth());
	int extent = max(DPI(24), style.tab_extent);
	int body_gap = max(0, style.body_gap);
	bool horizontal = tab.GetPlacement() == UiAlign::TOP || tab.GetPlacement() == UiAlign::BOTTOM;
	int page_width = horizontal ? content_width
	                            : max(1, content_width - extent - body_gap);

	UiLayoutMeasureResult page = UiMeasureLayout(tab.GetPage(active), { page_width });
	int page_height = max(0, page.width_dependent ? page.measured.cy : page.min.cy);
	int content_height = horizontal ? extent + body_gap + page_height : page_height;
	Size outer = UiStyledOuterSizeFromContent(Size(content_width, content_height), style.metrics, style.skin);
	return Size(width, max(0, outer.cy));
}

}

UiLayoutMeasureResult UiMeasureLayout(const Ctrl& c, const UiLayoutMeasureSpec& spec)
{
	UiLayoutMeasureResult out;
	out.preferred = c.GetMinSize();
	out.min = out.preferred;
	out.measured = out.preferred;

	if(const UiBoxLayout* box = dynamic_cast<const UiBoxLayout*>(&c)) {
		if(box->GetDirection() == UiDirection::H &&
		   box->GetWrapMode() != UiBoxWrap::None &&
		   box->IsWrapAutoResize()) {
			int preferred_w = max(1, box->GetPreferredSize().cx);
			int min_w = max(1, box->GetMinWrapWidth());
			int measure_w = spec.available_width >= 0 ? max(1, spec.available_width) : preferred_w;
			out.preferred = Size(preferred_w, max(0, box->MeasureHeightForWidth(preferred_w)));
			out.min = Size(min_w, max(0, box->MeasureHeightForWidth(min_w)));
			out.measured = Size(measure_w, max(0, box->MeasureHeightForWidth(measure_w)));
			out.width_dependent = true;
			return out;
		}
	}

	if(const UiGridLayout* grid = dynamic_cast<const UiGridLayout*>(&c)) {
		if(spec.available_width >= 0) {
			int measure_w = max(1, spec.available_width);
			out.measured = Size(measure_w, max(0, grid->MeasureHeightForWidth(measure_w)));
			out.width_dependent = true;
		}
		return out;
	}

	if(const UiPanel* panel = dynamic_cast<const UiPanel*>(&c)) {
		out.preferred = panel->GetMinSize();
		out.min = panel->GetMinWrapSize();
		int measure_w = spec.available_width >= 0 ? max(0, spec.available_width) : out.preferred.cx;
		out.measured = panel->MeasureSizeForWidth(measure_w);
		out.width_dependent = panel->HasWidthDependentContent();
		return out;
	}

	if(const UiTab* tab = dynamic_cast<const UiTab*>(&c)) {
		int active = tab->GetActiveTab();
		if(active >= 0 && active < tab->GetCount()) {
			UiLayoutMeasureResult page = UiMeasureLayout(tab->GetPage(active));
			out.width_dependent = page.width_dependent;
			if(spec.available_width >= 0)
				out.measured = MeasureUiTabForWidth(*tab, spec.available_width);
			else if(out.width_dependent)
				out.measured = MeasureUiTabForWidth(*tab, max(1, out.preferred.cx));
		}
		return out;
	}

	if(const UiTree* tree = dynamic_cast<const UiTree*>(&c)) {
		Size content = tree->GetContentSize();
		out.preferred = content;
		out.min = content;
		out.measured = content;
		out.width_dependent = false;
		return out;
	}

	return out;
}

}
