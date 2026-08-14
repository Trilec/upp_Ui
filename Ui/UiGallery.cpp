#include <Ui/UiGallery.h>
#include <Ui/UiGridLayout.h>
#include <Ui/UiList.h>
#include <Ui/UiTheme.h>

namespace Upp {

const UiGallery::Style& UiGallery::StyleDefault()
{
    static Style s;
    ONCELOCK {
        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(White());
            s.palette.frame[i] = Color(226, 232, 240);
            s.palette.ink[i] = Color(17, 24, 39);
            s.palette.icon[i] = Color(100, 116, 139);
        }
        s.palette.face[ST_DISABLED] = UiFill::Solid(Color(248, 250, 252));
        s.palette.ink[ST_DISABLED] = Color(148, 163, 184);
        s.palette.icon[ST_DISABLED] = Color(148, 163, 184);

        s.metrics = StyledMetrics();
        s.metrics.face_enabled = true;
        s.metrics.frame_enabled = true;
        s.metrics.frame_width = DPI(1);
        s.metrics.radius = 0;
        s.metrics.content_margin = Rect(0, 0, 0, 0);
        s.metrics.focus_enabled = true;
        s.metrics.focus_margin = DPI(2);
        s.metrics.focus_alpha = 180;
        s.metrics.focus_color = Color(65, 167, 248);
        s.skin = StyledSkin();

        s.marquee_fill = Color(219, 234, 254);
        s.marquee_frame = Color(59, 130, 246);
        s.marquee_frame_width = DPI(1);
    }
    return s;
}

UiGallery::UiGallery()
    : style_(StyleDefault())
    , themed_style_(StyleDefault())
    , model_(&internal_model_)
    , vscroll_(UiDirection::V)
{
    BackPaint();
    WantFocus();
    Add(vscroll_);
    vscroll_.EnableThinIdle(true);
    vscroll_.WhenScroll = [=] {
        if(updating_scrollbar_)
            return;
        scroll_y_ = vscroll_.GetPos();
        ClampScroll();
        PrepareItemRenders();
        UpdateVisibleRangeNotification();
        Refresh();
    };
    SyncThemeStyle();
    BindModel(internal_model_);
    SyncModel();
}

UiGallery::Style& UiGallery::StyleEdit()
{
    if(!has_custom_style_) {
        style_ = GetEffectiveStyle();
        has_custom_style_ = true;
    }
    theme_revision_ = 0;
    return style_;
}

const UiGallery::Style& UiGallery::GetEffectiveStyle() const
{
    if(has_custom_style_)
        return style_;
    const_cast<UiGallery*>(this)->SyncThemeStyle();
    return themed_style_;
}

void UiGallery::SyncThemeStyle()
{
    if(has_custom_style_)
        return;
    uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;

    themed_style_ = StyleDefault();
    UiList::Style list = UiTheme::ResolveList();
    themed_style_.palette = list.palette;
    themed_style_.metrics = list.metrics;
    themed_style_.skin = list.skin;
    themed_style_.metrics.content_margin = Rect(0, 0, 0, 0);
    themed_style_.marquee_fill = list.selected_face;
    themed_style_.marquee_frame = list.selected_frame;
    vscroll_.SetCustomStyle(UiTheme::ResolveScrollBar());
    theme_revision_ = revision;
}

UiGallery& UiGallery::SetCustomStyle(const Style& style)
{
    style_ = style;
    has_custom_style_ = true;
    OnStyleChanged();
    return *this;
}

UiGallery& UiGallery::ClearCustomStyle()
{
    if(!has_custom_style_)
        return *this;
    has_custom_style_ = false;
    style_ = StyleDefault();
    theme_revision_ = 0;
    OnStyleChanged();
    return *this;
}

void UiGallery::OnStyleChanged()
{
    InvalidateGeometry();
}

UiGallery& UiGallery::SetModel(UiListModel& model)
{
    if(model_ == &model)
        return *this;
    model_ = &model;
    BindModel(model);
    model_revision_ = -1;
    selected_.Clear();
    cursor_ = anchor_ = hot_ = pressed_ = -1;
    scroll_y_ = 0;
    notified_range_ = UiVisibleRange();
    EndMarquee(true);
    ResetItemRenderPool();
    SyncModel();
    InvalidateGeometry();
    return *this;
}

UiGallery& UiGallery::SetItemSize(Size size)
{
    size.cx = max(DPI(16), size.cx);
    size.cy = max(DPI(16), size.cy);
    if(base_item_size_ == size && zoom_ == 1.0)
        return *this;
    base_item_size_ = size;
    item_size_ = size;
    zoom_ = 1.0;
    InvalidateGeometry();
    return *this;
}

UiGallery& UiGallery::SetGap(int px)
{
    px = max(0, px);
    if(gap_ == px)
        return *this;
    gap_ = px;
    InvalidateGeometry();
    return *this;
}

UiGallery& UiGallery::SetInset(int all)
{
    return SetInset(all, all, all, all);
}

UiGallery& UiGallery::SetInset(int w, int h)
{
    return SetInset(w, h, w, h);
}

UiGallery& UiGallery::SetInset(int l, int t, int r, int b)
{
    Rect next(max(0, l), max(0, t), max(0, r), max(0, b));
    if(inset_ == next)
        return *this;
    inset_ = next;
    InvalidateGeometry();
    return *this;
}

UiGallery& UiGallery::SetOverscanRows(int rows)
{
    rows = max(0, rows);
    if(overscan_rows_ == rows)
        return *this;
    overscan_rows_ = rows;
    PrepareItemRenders();
    UpdateVisibleRangeNotification();
    return *this;
}

UiGallery& UiGallery::SetZoomRange(double minimum, double maximum, double step)
{
    minimum = max(0.10, minimum);
    maximum = max(minimum, maximum);
    step = max(1.01, step);
    min_zoom_ = minimum;
    max_zoom_ = maximum;
    zoom_step_ = step;
    if(zoom_ < min_zoom_ || zoom_ > max_zoom_)
        SetZoom(zoom_);
    return *this;
}

UiGallery& UiGallery::SetZoom(double zoom, Point anchor)
{
    zoom = minmax(zoom, min_zoom_, max_zoom_);
    if(fabs(zoom - zoom_) < 0.0001)
        return *this;
    if(!geometry_valid_)
        UpdateGeometry();

    if(anchor.x < 0 || anchor.y < 0)
        anchor = viewport_.CenterPoint();
    int anchor_index = HitTestItem(anchor);
    double anchor_fraction_y = 0.5;
    if(anchor_index >= 0) {
        Rect before = GetItemRect(anchor_index);
        if(before.GetHeight() > 0)
            anchor_fraction_y = minmax(double(anchor.y - before.top) / before.GetHeight(), 0.0, 1.0);
    }

    zoom_ = zoom;
    item_size_ = Size(max(DPI(16), int(base_item_size_.cx * zoom_ + 0.5)),
                      max(DPI(16), int(base_item_size_.cy * zoom_ + 0.5)));
    geometry_valid_ = false;
    UpdateGeometry();

    if(anchor_index >= 0 && model_ && anchor_index < model_->GetCount()) {
        Rect after = GetItemRect(anchor_index);
        int item_anchor_y = after.top + int(after.GetHeight() * anchor_fraction_y + 0.5);
        scroll_y_ += item_anchor_y - anchor.y;
        ClampScroll();
        if(vscroll_.IsShown()) {
            updating_scrollbar_ = true;
            vscroll_.SetPos(scroll_y_);
            updating_scrollbar_ = false;
        }
    }

    InvalidateItemRenderData();
    PrepareItemRenders();
    UpdateVisibleRangeNotification();
    RefreshLayout();
    Refresh();
    return *this;
}

UiGallery& UiGallery::ZoomBy(double factor, Point anchor)
{
    if(factor <= 0)
        return *this;
    return SetZoom(zoom_ * factor, anchor);
}

UiGallery& UiGallery::SetSelectionMode(UiGallerySelectionMode mode)
{
    if(selection_mode_ == mode)
        return *this;
    EndMarquee(true);
    selection_mode_ = mode;
    if(mode == UIGALLERYSEL_SINGLE && selected_.GetCount() > 1) {
        int keep = IsSelectableIndex(cursor_) ? cursor_ : selected_[0];
        selected_.Clear();
        if(IsSelectableIndex(keep))
            selected_.FindAdd(keep);
    }
    NotifySelectionChange();
    return *this;
}

UiGallery& UiGallery::ClearSelection()
{
    if(selected_.IsEmpty())
        return *this;
    selected_.Clear();
    cursor_ = anchor_ = -1;
    NotifySelectionChange();
    return *this;
}

UiGallery& UiGallery::Select(int index, bool additive)
{
    SyncModel();
    if(!IsSelectableIndex(index))
        return *this;
    if(selection_mode_ != UIGALLERYSEL_MULTI || !additive)
        SelectSingle(index);
    else
        ToggleSelection(index);
    return *this;
}

UiGallery& UiGallery::SelectAll()
{
    SyncModel();
    if(selection_mode_ != UIGALLERYSEL_MULTI || !model_)
        return *this;
    selected_.Clear();
    for(int i = 0; i < model_->GetCount(); i++)
        if(IsSelectableIndex(i))
            selected_.FindAdd(i);
    if(!selected_.IsEmpty()) {
        cursor_ = selected_[0];
        anchor_ = cursor_;
    }
    NotifySelectionChange();
    return *this;
}

bool UiGallery::IsSelected(int index) const
{
    return selected_.Find(index) >= 0;
}

Vector<int> UiGallery::GetSelection() const
{
    Vector<int> out;
    out.Reserve(selected_.GetCount());
    for(int i = 0; i < selected_.GetCount(); i++)
        out.Add(selected_[i]);
    Sort(out);
    return out;
}

UiGallery& UiGallery::SetCursor(int index)
{
    SyncModel();
    if(!IsSelectableIndex(index))
        return *this;
    SelectSingle(index);
    ScrollTo(index);
    return *this;
}

UiGallery& UiGallery::SetScrollPos(int y)
{
    if(!geometry_valid_)
        UpdateGeometry();
    int next = clamp(y, 0, GetMaxScroll());
    if(next == scroll_y_)
        return *this;
    scroll_y_ = next;
    if(vscroll_.IsShown()) {
        updating_scrollbar_ = true;
        vscroll_.SetPos(scroll_y_);
        updating_scrollbar_ = false;
    }
    PrepareItemRenders();
    UpdateVisibleRangeNotification();
    Refresh();
    return *this;
}

void UiGallery::ScrollTo(int index)
{
    SyncModel();
    if(!model_ || index < 0 || index >= model_->GetCount())
        return;
    if(!geometry_valid_)
        UpdateGeometry();
    int row = index / max(1, columns_);
    int top = inset_.top + row * (item_size_.cy + gap_);
    int bottom = top + item_size_.cy;
    int page = max(0, viewport_.GetHeight());
    int next = scroll_y_;
    if(top < scroll_y_)
        next = top;
    else if(bottom > scroll_y_ + page)
        next = bottom - page;
    SetScrollPos(next);
}

void UiGallery::ScrollToSelection()
{
    if(cursor_ >= 0)
        ScrollTo(cursor_);
}

} // namespace Upp
