#include <Ui/UiAbsoluteLayout.h>

namespace Upp {

UiAbsoluteLayout::UiAbsoluteLayout()
{
    Transparent();
}

Rect UiAbsoluteLayout::NormalizeRect(const Rect& rect)
{
    return RectC(rect.left, rect.top,
                 max(0, rect.GetWidth()), max(0, rect.GetHeight()));
}

bool UiAbsoluteLayout::ItemRef::IsValid() const
{
    return owner && index >= 0 && index < owner->GetItemCount();
}

UiAbsoluteLayout::ItemRef& UiAbsoluteLayout::ItemRef::SetRect(const Rect& rect)
{
    if(IsValid())
        owner->SetItemRect(index, rect);
    return *this;
}

Rect UiAbsoluteLayout::ItemRef::GetRect() const
{
    return IsValid() ? owner->GetItemRect(index) : Rect(0, 0, 0, 0);
}

UiAbsoluteLayout::ItemRef UiAbsoluteLayout::Add(Ctrl& child, const Rect& rect)
{
    const int existing = Find(child);
    if(existing >= 0) {
        SetItemRect(existing, rect);
        return ItemRef(this, existing);
    }

    Item& item = items_.Add();
    item.child = &child;
    item.rect = NormalizeRect(rect);
    Ctrl::Add(child);
    Relayout();
    return ItemRef(this, items_.GetCount() - 1);
}

UiAbsoluteLayout& UiAbsoluteLayout::SetItemRect(int index, const Rect& rect)
{
    if(index < 0 || index >= items_.GetCount())
        return *this;
    const Rect normalized = NormalizeRect(rect);
    if(items_[index].rect == normalized)
        return *this;
    items_[index].rect = normalized;
    Relayout();
    return *this;
}

int UiAbsoluteLayout::Find(const Ctrl& child) const
{
    for(int i = 0; i < items_.GetCount(); i++)
        if(items_[i].child == &child)
            return i;
    return -1;
}

Ctrl* UiAbsoluteLayout::GetItemCtrl(int index) const
{
    return index >= 0 && index < items_.GetCount() ? items_[index].child : nullptr;
}

Rect UiAbsoluteLayout::GetItemRect(int index) const
{
    return index >= 0 && index < items_.GetCount()
        ? items_[index].rect : Rect(0, 0, 0, 0);
}

bool UiAbsoluteLayout::Remove(int index)
{
    if(index < 0 || index >= items_.GetCount())
        return false;
    if(items_[index].child)
        items_[index].child->Remove();
    items_.Remove(index);
    Relayout();
    return true;
}

bool UiAbsoluteLayout::Remove(Ctrl& child)
{
    const int index = Find(child);
    return index >= 0 && Remove(index);
}

void UiAbsoluteLayout::Clear()
{
    for(Item& item : items_)
        if(item.child)
            item.child->Remove();
    items_.Clear();
    Relayout();
}

Size UiAbsoluteLayout::GetContentSize() const
{
    int right = 0;
    int bottom = 0;
    for(const Item& item : items_) {
        right = max(right, item.rect.right);
        bottom = max(bottom, item.rect.bottom);
    }
    return Size(max(0, right), max(0, bottom));
}

Size UiAbsoluteLayout::GetMinSize() const
{
    return GetContentSize();
}

void UiAbsoluteLayout::Layout()
{
    for(const Item& item : items_)
        if(item.child)
            item.child->SetRect(item.rect);
}

void UiAbsoluteLayout::Relayout()
{
    Layout();
    RefreshLayout();
}

} // namespace Upp
