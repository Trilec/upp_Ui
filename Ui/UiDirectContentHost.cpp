#include <Ui/UiDirectContentHost.h>
#include <Ui/UiAxis.h>

namespace Upp {

UiDirectContentHost& UiDirectContentHost::SetContent(Ctrl& ctrl)
{
    if(content_ == &ctrl)
        return *this;
    if(content_)
        content_->Remove();
    content_ = &ctrl;
    Add(ctrl);
    RefreshLayout();
    return *this;
}

UiDirectContentHost& UiDirectContentHost::ClearContent()
{
    if(content_) {
        content_->Remove();
        content_ = nullptr;
        RefreshLayout();
    }
    return *this;
}

UiDirectContentHost& UiDirectContentHost::SetSizing(UiDirectSizeMode h, UiDirectSizeMode v)
{
    h_mode_ = h;
    v_mode_ = v;
    RefreshLayout();
    return *this;
}

UiDirectContentHost& UiDirectContentHost::SetFixedSize(Size sz)
{
    fixed_ = Size(max(0, sz.cx), max(0, sz.cy));
    RefreshLayout();
    return *this;
}

UiDirectContentHost& UiDirectContentHost::SetMinimumSize(Size sz)
{
    min_ = Size(max(0, sz.cx), max(0, sz.cy));
    if(max_.cx > 0 && min_.cx > max_.cx)
        max_.cx = min_.cx;
    if(max_.cy > 0 && min_.cy > max_.cy)
        max_.cy = min_.cy;
    RefreshLayout();
    return *this;
}

UiDirectContentHost& UiDirectContentHost::SetMaximumSize(Size sz)
{
    max_ = Size(max(0, sz.cx), max(0, sz.cy));
    if(max_.cx > 0 && max_.cx < min_.cx)
        max_.cx = min_.cx;
    if(max_.cy > 0 && max_.cy < min_.cy)
        max_.cy = min_.cy;
    RefreshLayout();
    return *this;
}

UiDirectContentHost& UiDirectContentHost::SetAlign(UiAlign h, UiAlign v)
{
    align_h_ = h;
    align_v_ = v;
    RefreshLayout();
    return *this;
}

static UiAxisMode UiDirectAxisMode(UiDirectSizeMode mode);
static UiResolvedAxis UiDirectResolveAxis(UiDirectSizeMode mode, int minimum, int maximum,
                                          int fixed, int preferred, int available);

Size UiDirectContentHost::GetMinSize() const
{
    Size natural = content_ ? content_->GetMinSize() : Size(0, 0);
    UiResolvedAxis hw = UiDirectResolveAxis(h_mode_, min_.cx, max_.cx, fixed_.cx,
                                            natural.cx, h_mode_ == UIDIRECT_EXPAND ? min_.cx : natural.cx);
    UiResolvedAxis hv = UiDirectResolveAxis(v_mode_, min_.cy, max_.cy, fixed_.cy,
                                            natural.cy, v_mode_ == UIDIRECT_EXPAND ? min_.cy : natural.cy);
    return Size(hw.final_size, hv.final_size);
}

static int UiDirectAlignedPos(int start, int available, int size, UiAlign align)
{
    if(align == UiAlign::RIGHT || align == UiAlign::BOTTOM)
        return start + max(0, available - size);
    if(align == UiAlign::CENTER)
        return start + max(0, (available - size) / 2);
    return start;
}

static UiAxisMode UiDirectAxisMode(UiDirectSizeMode mode)
{
	switch(mode) {
	case UIDIRECT_FIXED:  return UiAxisMode::Fixed;
	case UIDIRECT_EXPAND: return UiAxisMode::Expand;
	default:              return UiAxisMode::Fit;
	}
}

static UiResolvedAxis UiDirectResolveAxis(UiDirectSizeMode mode, int minimum, int maximum,
                                          int fixed, int preferred, int available)
{
	UiAxisConstraints c;
	c.mode = UiDirectAxisMode(mode);
	c.minimum = minimum;
	c.maximum = maximum;
	c.fixed = fixed;
	c.preferred = preferred;
	c.available = available;
	return UiResolveAxis(c);
}

void UiDirectContentHost::Layout()
{
    if(!content_)
        return;
    Rect r = GetSize();
    Size natural = content_->GetMinSize();
    UiResolvedAxis hw = UiDirectResolveAxis(h_mode_, min_.cx, max_.cx, fixed_.cx, natural.cx, r.GetWidth());
    UiResolvedAxis hv = UiDirectResolveAxis(v_mode_, min_.cy, max_.cy, fixed_.cy, natural.cy, r.GetHeight());
    int w = hw.final_size;
    int h = hv.final_size;
    int x = UiDirectAlignedPos(r.left, r.GetWidth(), w, align_h_);
    int y = UiDirectAlignedPos(r.top, r.GetHeight(), h, align_v_);
    content_->SetRect(RectC(x, y, w, h));
}

}
