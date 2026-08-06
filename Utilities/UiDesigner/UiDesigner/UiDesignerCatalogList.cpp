#include "UiDesignerWidgets.h"

namespace Upp {

UiDesignerCatalogList::UiDesignerCatalogList()
{
    BackPaint();
    Add(filter_edit_);
    Add(scope_label_);
    filter_edit_.SetPlaceholder("Filter controls...");
    scope_label_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
    UpdateScopeLabel();
    filter_edit_.WhenChange = [=] {
        filter_ = AsString(filter_edit_.GetData());
        RebuildMatches();
        WhenFilter(filter_);
    };
}

void UiDesignerCatalogList::SetCatalog(const UiDesignerCatalog *catalog)
{
    catalog_ = catalog;
    UpdateScopeLabel();
    RebuildMatches();
}

void UiDesignerCatalogList::SetCategory(const String& category)
{
    category_ = category;
    presets_ = false;
    UpdateScopeLabel();
    RebuildMatches();
}

void UiDesignerCatalogList::SetPresets(bool on)
{
    presets_ = on;
    UpdateScopeLabel();
    RebuildMatches();
}

void UiDesignerCatalogList::SetFilter(const String& filter)
{
    filter_ = filter;
    filter_edit_.SetData(filter);
    RebuildMatches();
}

void UiDesignerCatalogList::UpdateScopeLabel()
{
    const String scope = presets_
        ? "Presets"
        : (category_.IsEmpty() ? "All controls" : category_);
    scope_label_.SetText(scope);
    scope_label_.Tip("Current catalog scope");
}

void UiDesignerCatalogList::RebuildMatches()
{
    matches_.Clear();
    if(catalog_) {
        const String needle = ToLower(TrimBoth(filter_));
        if(presets_) {
            for(int i = 0; i < catalog_->GetPresets().GetCount(); i++) {
                const UiDesignerPreset& preset = catalog_->GetPresets()[i];
                if(needle.IsEmpty() ||
                   ToLower(preset.display_name).Find(needle) >= 0 ||
                   ToLower(preset.help).Find(needle) >= 0)
                    matches_.Add(i);
            }
        }
        else
            matches_ = catalog_->Search(filter_, category_.IsEmpty() ? "All" : category_);
    }
    hover_ = -1;
    selected_ = matches_.IsEmpty() ? -1 : minmax(selected_, 0, matches_.GetCount() - 1);
    scroll_ = 0;
    Refresh();
}

int UiDesignerCatalogList::Count() const
{
    return matches_.GetCount();
}

String UiDesignerCatalogList::ItemId(int index) const
{
    if(!catalog_ || index < 0 || index >= matches_.GetCount())
        return String();
    const int source = matches_[index];
    return presets_ ? "preset:" + catalog_->GetPresets()[source].id
                    : catalog_->GetControls()[source].type_id;
}

String UiDesignerCatalogList::ItemLabel(int index) const
{
    if(!catalog_ || index < 0 || index >= matches_.GetCount())
        return String();
    const int source = matches_[index];
    return presets_ ? catalog_->GetPresets()[source].display_name
                    : catalog_->GetControls()[source].display_name;
}

String UiDesignerCatalogList::ItemHelp(int index) const
{
    if(!catalog_ || index < 0 || index >= matches_.GetCount())
        return String();
    const int source = matches_[index];
    return presets_ ? catalog_->GetPresets()[source].help
                    : catalog_->GetControls()[source].help;
}

Image UiDesignerCatalogList::ItemIcon(int index) const
{
    if(!catalog_ || index < 0 || index >= matches_.GetCount())
        return Image();
    const int source = matches_[index];
    return presets_
        ? UiDesignerResolveCatalogIcon(catalog_->GetPresets()[source].icon_key)
        : UiDesignerResolveCatalogIcon(catalog_->GetControls()[source].icon_key);
}

Rect UiDesignerCatalogList::ItemRect(int index) const
{
    const int top = DPI(72);
    const int row = DPI(42);
    return RectC(0, top + index * row - scroll_, GetSize().cx, row);
}

int UiDesignerCatalogList::RowAt(Point p) const
{
    if(p.y < DPI(72))
        return -1;
    const int index = (p.y - DPI(72) + scroll_) / DPI(42);
    return index >= 0 && index < Count() ? index : -1;
}

int UiDesignerCatalogList::GetContentHeight() const
{
    return Count() * DPI(42);
}

void UiDesignerCatalogList::Layout()
{
    filter_edit_.SetRect(DPI(6), DPI(6), max(0, GetSize().cx - DPI(12)), DPI(34));
    scope_label_.SetRect(DPI(8), DPI(42), max(0, GetSize().cx - DPI(16)), DPI(16));
}

void UiDesignerCatalogList::Paint(Draw& w)
{
    w.DrawRect(GetSize(), SColorPaper());
    for(int i = 0; i < Count(); i++) {
        Rect r = ItemRect(i);
        if(r.bottom < DPI(40) || r.top > GetSize().cy)
            continue;
        const bool current = i == selected_;
        Color face = current
            ? Blend(SColorHighlight(), SColorPaper(), 75)
            : i == hover_ ? Blend(SColorHighlight(), SColorPaper(), 35)
            : (i & 1 ? Blend(SColorFace(), SColorPaper(), 70) : SColorPaper());
        w.DrawRect(r, face);
        Image icon = ItemIcon(i);
        if(!icon.IsEmpty())
            w.DrawImage(r.left + DPI(10), r.top + DPI(11), DPI(18), DPI(18), icon);
        w.DrawText(r.left + DPI(38), r.top + DPI(7), ItemLabel(i),
                   SansSerifZ(11).Bold(current), SColorText());
        const String help = ItemHelp(i);
        if(!help.IsEmpty())
            w.DrawText(r.left + DPI(38), r.top + DPI(23),
                       help.Left(54), SansSerifZ(8), SColorDisabled());
        w.DrawLine(r.left, r.bottom - 1, r.right, r.bottom - 1, 1, SColorShadow());
    }
    if(Count() == 0)
        w.DrawText(DPI(12), DPI(54), "No matching controls", SansSerifZ(10), SColorDisabled());
}

void UiDesignerCatalogList::Activate(int index)
{
    if(index >= 0 && index < Count())
        WhenActivate(ItemId(index));
}

void UiDesignerCatalogList::LeftDown(Point p, dword)
{
    pressed_ = selected_ = RowAt(p);
    drag_type_ = pressed_ >= 0 ? ItemId(pressed_) : String();
    drag_start_ = GetMousePos();
    drag_armed_ = pressed_ >= 0 && !drag_type_.IsEmpty();
    dragging_ = false;
    if(drag_armed_ && !HasCapture())
        SetCapture();
    SetFocus();
    Refresh();
}

void UiDesignerCatalogList::LeftUp(Point p, dword)
{
    const int pressed = pressed_;
    const String type = drag_type_;
    const bool was_dragging = dragging_;
    const bool was_armed = drag_armed_;
    const Point screen = GetMousePos();
    const int index = RowAt(p);
    pressed_ = -1;
    dragging_ = false;
    drag_armed_ = false;
    drag_type_.Clear();
    if(HasCapture())
        ReleaseCapture();
    if(was_dragging && was_armed && !type.IsEmpty())
        WhenToolDrop(type, screen);
    else if(pressed >= 0 && index == pressed)
        Activate(index);
}

void UiDesignerCatalogList::LeftDouble(Point p, dword)
{
    Activate(RowAt(p));
}

void UiDesignerCatalogList::LeftDrag(Point, dword)
{
    if(pressed_ < 0 || pressed_ >= Count())
        return;
    if(!drag_armed_ && !drag_type_.IsEmpty()) {
        drag_armed_ = true;
        if(!HasCapture())
            SetCapture();
    }
    MouseMove(GetMousePos() - GetScreenRect().TopLeft(), K_MOUSELEFT);
}

void UiDesignerCatalogList::MouseMove(Point p, dword)
{
    if(drag_armed_) {
        if(!HasCapture())
            SetCapture();
        if(!dragging_ && Length(GetMousePos() - drag_start_) >= DPI(5))
            dragging_ = true;
        if(dragging_ && WhenToolDrag)
            WhenToolDrag(drag_type_, GetMousePos());
        return;
    }
    const int next = RowAt(p);
    if(next != hover_) {
        hover_ = next;
        Tip(next >= 0 ? ItemHelp(next) : String());
        Refresh();
    }
}

void UiDesignerCatalogList::MouseLeave()
{
    hover_ = -1;
    Tip(String());
    Refresh();
}

Image UiDesignerCatalogList::CursorImage(Point p, dword flags)
{
    return dragging_ ? Image::SizeAll() : ParentCtrl::CursorImage(p, flags);
}

void UiDesignerCatalogList::CancelMode()
{
    const bool active = drag_armed_ || dragging_;
    pressed_ = -1;
    dragging_ = false;
    drag_armed_ = false;
    drag_type_.Clear();
    if(active && WhenToolCancel)
        WhenToolCancel();
    ParentCtrl::CancelMode();
}

void UiDesignerCatalogList::MouseWheel(Point, int zdelta, dword)
{
    const int list_height = max(0, GetSize().cy - DPI(40));
    const int maximum = max(0, GetContentHeight() - list_height);
    scroll_ = minmax(scroll_ - zdelta / 4, 0, maximum);
    Refresh();
}

bool UiDesignerCatalogList::Key(dword key, int)
{
    if(key == K_UP && Count()) {
        selected_ = max(0, selected_ - 1);
        Refresh();
        return true;
    }
    if(key == K_DOWN && Count()) {
        selected_ = min(Count() - 1, selected_ + 1);
        Refresh();
        return true;
    }
    if(key == K_ENTER && selected_ >= 0) {
        Activate(selected_);
        return true;
    }
    if(key == K_CTRL_F) {
        filter_edit_.SetFocus();
        return true;
    }
    return ParentCtrl::Key(key, 1);
}

}
