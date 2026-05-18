#include <Ui/UiStack.h>

namespace Upp {

UiStack::UiStack()
{
    Transparent();
}

int UiStack::AddPage(Ctrl& page, const String& key)
{
    for(int i = 0; i < pages_.GetCount(); i++)
        if(pages_[i].ctrl == &page)
            return i;

    Page& p = pages_.Add();
    p.key = key;
    p.ctrl = &page;
    Ctrl::Add(page);
    page.SetRect(GetSize());

    int old = active_;
    if(active_ < 0)
        active_ = pages_.GetCount() - 1;

    SyncVisibility();
    RefreshLayout();
    Refresh();
    if(old != active_)
        WhenPageChanged(old, active_);
    return pages_.GetCount() - 1;
}

UiStack& UiStack::RemovePage(int i)
{
    if(!IsValidPage(i))
        return *this;

    int old = active_;
    Ctrl *page = pages_[i].ctrl;

    removing_ = true;
    if(page && page->GetParent() == this)
        page->Remove();
    removing_ = false;

    pages_.Remove(i);

    if(pages_.IsEmpty())
        active_ = -1;
    else if(old == i)
        active_ = min(i, pages_.GetCount() - 1);
    else if(old > i)
        active_ = old - 1;
    else
        active_ = old;

    SyncVisibility();
    RefreshLayout();
    Refresh();
    WhenPageRemoved(i);
    if(old != active_)
        WhenPageChanged(old, active_);
    return *this;
}

UiStack& UiStack::ClearPages()
{
    if(pages_.IsEmpty())
        return *this;

    int old = active_;
    removing_ = true;
    for(Page& p : pages_)
        if(p.ctrl && p.ctrl->GetParent() == this)
            p.ctrl->Remove();
    removing_ = false;

    pages_.Clear();
    active_ = -1;
    RefreshLayout();
    Refresh();
    WhenPagesCleared();
    if(old != active_)
        WhenPageChanged(old, active_);
    return *this;
}

Ctrl& UiStack::GetPage(int i)
{
    ASSERT(IsValidPage(i) && pages_[i].ctrl);
    return *pages_[i].ctrl;
}

const Ctrl& UiStack::GetPage(int i) const
{
    ASSERT(IsValidPage(i) && pages_[i].ctrl);
    return *pages_[i].ctrl;
}

Ctrl* UiStack::FindPage(int i)
{
    return IsValidPage(i) ? ~pages_[i].ctrl : nullptr;
}

const Ctrl* UiStack::FindPage(int i) const
{
    return IsValidPage(i) ? ~pages_[i].ctrl : nullptr;
}

String UiStack::GetKey(int i) const
{
    return IsValidPage(i) ? pages_[i].key : String();
}

UiStack& UiStack::SetKey(int i, const String& key)
{
    if(IsValidPage(i))
        pages_[i].key = key;
    return *this;
}

UiStack& UiStack::SetActivePage(int i)
{
    if(!IsValidPage(i) || active_ == i)
        return *this;

    int old = active_;
    active_ = i;
    SyncVisibility();
    Layout();
    RefreshLayout();
    Refresh();
    WhenPageChanged(old, active_);
    return *this;
}

UiStack& UiStack::SetActiveKey(const String& key)
{
    int q = FindKey(key);
    if(q >= 0)
        SetActivePage(q);
    return *this;
}

String UiStack::GetActiveKey() const
{
    return IsValidPage(active_) ? pages_[active_].key : String();
}

Ctrl* UiStack::GetActiveCtrl()
{
    return FindPage(active_);
}

const Ctrl* UiStack::GetActiveCtrl() const
{
    return FindPage(active_);
}

UiStack& UiStack::MovePage(int from, int to)
{
    if(!IsValidPage(from) || !IsValidPage(to) || from == to)
        return *this;

    int old_active = active_;
    Ctrl *active_page = IsValidPage(active_) ? ~pages_[active_].ctrl : nullptr;
    Page page = pick(pages_[from]);
    pages_.Remove(from);
    pages_.Insert(to, pick(page));

    active_ = -1;
    for(int i = 0; i < pages_.GetCount(); i++) {
        if(pages_[i].ctrl == active_page) {
            active_ = i;
            break;
        }
    }

    SyncVisibility();
    RefreshLayout();
    Refresh();
    WhenPageMoved(from, to);
    if(old_active != active_)
        WhenPageChanged(old_active, active_);
    return *this;
}

UiStack& UiStack::MovePageUp(int i)
{
    if(i > 0)
        MovePage(i, i - 1);
    return *this;
}

UiStack& UiStack::MovePageDown(int i)
{
    if(i >= 0 && i + 1 < pages_.GetCount())
        MovePage(i, i + 1);
    return *this;
}

void UiStack::SetData(const Value& v)
{
    if(IsNumber(v)) {
        SetActivePage((int)v);
        return;
    }
    if(!IsNull(v))
        SetActiveKey(AsString(v));
}

Value UiStack::GetData() const
{
    return active_;
}

int UiStack::FindKey(const String& key) const
{
    for(int i = 0; i < pages_.GetCount(); i++)
        if(pages_[i].key == key)
            return i;
    return -1;
}

Size UiStack::MeasurePage(Ctrl *page) const
{
    return page ? page->GetMinSize() : Size(0, 0);
}

Size UiStack::GetMinSize() const
{
    if(pages_.IsEmpty())
        return Size(DPI(120), DPI(80));

    Size out(0, 0);
    for(const Page& p : pages_) {
        Size sz = MeasurePage(~p.ctrl);
        out.cx = max(out.cx, sz.cx);
        out.cy = max(out.cy, sz.cy);
    }
    return out;
}

Size UiStack::GetContentSize() const
{
    return GetMinSize();
}

void UiStack::Layout()
{
    Rect r = GetSize();
    for(int i = 0; i < pages_.GetCount(); i++) {
        Ctrl *page = pages_[i].ctrl;
        if(!page)
            continue;
        page->SetRect(r);
        page->Show(i == active_);
    }
}

void UiStack::ChildRemoved(Ctrl *child)
{
    ParentCtrl::ChildRemoved(child);
    if(removing_ || !child)
        return;

    int q = -1;
    for(int i = 0; i < pages_.GetCount(); i++) {
        if(pages_[i].ctrl == child) {
            q = i;
            break;
        }
    }
    if(q < 0)
        return;

    int old = active_;
    pages_.Remove(q);
    if(pages_.IsEmpty())
        active_ = -1;
    else if(old == q)
        active_ = min(q, pages_.GetCount() - 1);
    else if(old > q)
        active_ = old - 1;
    else
        active_ = old;

    SyncVisibility();
    RefreshLayout();
    Refresh();
    WhenPageRemoved(q);
    if(old != active_)
        WhenPageChanged(old, active_);
}

void UiStack::SyncVisibility()
{
    for(int i = 0; i < pages_.GetCount(); i++) {
        Ctrl *page = pages_[i].ctrl;
        if(page)
            page->Show(i == active_);
    }
}

}
