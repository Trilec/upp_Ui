#include <Ui/UiDirectContentHost.h>

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

Size UiDirectContentHost::GetMinSize() const
{
    if(!content_)
        return min_;
    Size natural = content_->GetMinSize();
    int cx = h_mode_ == UIDIRECT_FIXED ? fixed_.cx : natural.cx;
    int cy = v_mode_ == UIDIRECT_FIXED ? fixed_.cy : natural.cy;
    return Size(max(cx, min_.cx), max(cy, min_.cy));
}

static int UiDirectAlignedPos(int start, int available, int size, UiAlign align)
{
    if(align == UiAlign::RIGHT || align == UiAlign::BOTTOM)
        return start + max(0, available - size);
    if(align == UiAlign::CENTER)
        return start + max(0, (available - size) / 2);
    return start;
}

void UiDirectContentHost::Layout()
{
    if(!content_)
        return;
    Rect r = GetSize();
    Size natural = content_->GetMinSize();
    int w = h_mode_ == UIDIRECT_EXPAND ? r.GetWidth()
          : h_mode_ == UIDIRECT_FIXED ? fixed_.cx
          : natural.cx;
    int h = v_mode_ == UIDIRECT_EXPAND ? r.GetHeight()
          : v_mode_ == UIDIRECT_FIXED ? fixed_.cy
          : natural.cy;
    w = min(max(w, min_.cx), r.GetWidth());
    h = min(max(h, min_.cy), r.GetHeight());
    int x = UiDirectAlignedPos(r.left, r.GetWidth(), w, align_h_);
    int y = UiDirectAlignedPos(r.top, r.GetHeight(), h, align_v_);
    content_->SetRect(RectC(x, y, w, h));
}

}
