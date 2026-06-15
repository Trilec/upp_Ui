#include <Ui/UiMeasure.h>
#include <Ui/UiBoxLayout.h>
#include <Ui/UiGridLayout.h>
#include <Ui/UiTree.h>

namespace Upp {

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
