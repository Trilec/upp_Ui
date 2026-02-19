#include <Ui/UiMultiEdit.h>

namespace Upp {

UiMultiEdit::UiMultiEdit()
{
	// 1. Multi-line semantics
	SetAcceptsNewlines(true);
	SetAcceptsTabs(true);

	// 2. Configure ScrollBars
	// We enable them here. AutoHide() will automatically hide them
	// if the content fits, so we don't need to force them in Layout().
	sb_.ShowY(true);
	sb_.ShowX(true);
	sb_.AutoHide();
}

Size UiMultiEdit::GetMinSize() const
{
	// Start from base min size (includes frame/padding)
	Size sz = UiBaseEdit::GetMinSize();

	// Calculate height for ~3 lines
	int line_h = GetVisualLineHeight();
	Rect cp = UiNonNegativeThickness(style_.metrics.content_padding);
	int fw  = UiResolvedFrameWidth(style_.metrics, style_.skin);
	sz.cy = (line_h * 3) + cp.top + cp.bottom + fw * 2;

	// Calculate width for ~10 chars (approximate)
	sz.cx = (font_size_.cx * 10) + cp.left + cp.right + fw * 2;

	return sz;
}

void UiMultiEdit::Layout()
{
	// Careful of infinite recursion stack overflows If calling things that trigger refreshing
	// of the layout

	UiBaseEdit::Layout();
}

} // namespace Upp
