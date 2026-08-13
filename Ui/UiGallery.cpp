#include <Ui/UiGallery.h>
#include <Ui/UiGridLayout.h>
#include <Ui/UiList.h>
#include <Ui/UiTheme.h>

namespace Upp {

static StyledState UiGalleryStyledState(UiGalleryItemVisualState state)
{
    switch(state) {
    case UIGALLERYITEM_HOT:      return ST_HOT;
    case UIGALLERYITEM_SELECTED: return ST_PRESSED;
    case UIGALLERYITEM_DISABLED: return ST_DISABLED;
    case UIGALLERYITEM_NORMAL:
    default:                     return ST_NORMAL;
    }
}

static void DrawGalleryText(Draw& w, const Rect& r, const String& text, Font font, Color ink)
{
    if(r.IsEmpty() || text.IsEmpty())
        return;
    Size ts = GetTextSize(text, font);
    int x = r.left + max(0, (r.GetWidth() - ts.cx) / 2);
    int y = r.top + max(0, (r.GetHeight() - ts.cy) / 2);
    w.Clip(r);
    w.DrawText(x, y, text, font, ink);
    w.End();
}

const UiGallery::Style& UiGallery::StyleDefault()
{
    static Style s;
    ONCELOCK {
        const Color text = Color(17, 24, 39);
        const Color muted = Color(100, 116, 139);
        const Color disabled = Color(148, 163, 184);

        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(White());
            s.palette.frame[i] = Color(226, 232, 240);
            s.palette.ink[i] = text;
            s.palette.icon[i] = muted;

            s.item_palette.face[i] = UiFill::Solid(White());
            s.item_palette.frame[i] = Color(226, 232, 240);
            s.item_palette.ink[i] = text;
            s.item_palette.icon[i] = muted;
        }
        s.palette.face[ST_DISABLED] = UiFill::Solid(Color(248, 250, 252));
        s.palette.ink[ST_DISABLED] = disabled;
        s.palette.icon[ST_DISABLED] = disabled;

        s.item_palette.face[ST_HOT] = UiFill::Solid(Color(245, 247, 250));
        s.item_palette.frame[ST_HOT] = Color(203, 213, 225);
        s.item_palette.face[ST_PRESSED] = UiFill::Solid(Color(232, 242, 255));
        s.item_palette.frame[ST_PRESSED] = Color(65, 167, 248);
        s.item_palette.face[ST_DISABLED] = UiFill::Solid(Color(248, 250, 252));
        s.item_palette.frame[ST_DISABLED] = Color(226, 232, 240);
        s.item_palette.ink[ST_DISABLED] = disabled;
        s.item_palette.icon[ST_DISABLED] = disabled;

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

        s.item_metrics = StyledMetrics();
        s.item_metrics.face_enabled = true;
        s.item_metrics.frame_enabled = true;
        s.item_metrics.frame_width = DPI(1);
        s.item_metrics.radius = DPI(6);
        s.item_metrics.focus_enabled = false;

        s.title_font = StdFont();
        s.description_font = StdFont();
        s.description_font.Height(max(DPI(9), StdFont().GetHeight() - DPI(1)));
        s.icon_size = DPI(36);
        s.content_gap = DPI(6);
        s.text_gap = DPI(3);
        s.item_padding = DPI(8);
        s.metadata_size = DPI(8);
        s.metadata_inset = DPI(7);
        s.description_ink = muted;
        s.metadata_default = Color(65, 167, 248);
        s.show_icons = true;
        s.show_description = true;
        s.show_metadata_marker = true;
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
    themed_style_.title_font = list.font;
    themed_style_.description_font = list.font;
    themed_style_.description_ink = list.muted_ink;
    themed_style_.metadata_default = list.metadata_default;

    for(int i = 0; i < 4; i++) {
        themed_style_.item_palette.face[i] = list.palette.face[i];
        themed_style_.item_palette.frame[i] = list.palette.frame[i];
        themed_style_.item_palette.ink[i] = list.palette.ink[i];
        themed_style_.item_palette.icon[i] = list.palette.icon[i];
    }
    themed_style_.item_palette.face[ST_HOT] = UiFill::Solid(list.hot_face);
    themed_style_.item_palette.frame[ST_HOT] = list.hot_frame;
    themed_style_.item_palette.ink[ST_HOT] = list.hot_ink;
    themed_style_.item_palette.face[ST_PRESSED] = UiFill::Solid(list.selected_face);
    themed_style_.item_palette.frame[ST_PRESSED] = list.selected_frame;
    themed_style_.item_palette.ink[ST_PRESSED] = list.selected_ink;
    themed_style_.item_palette.face[ST_DISABLED] = list.palette.face[ST_DISABLED];
    themed_style_.item_palette.frame[ST_DISABLED] = list.palette.frame[ST_DISABLED];
    themed_style_.item_palette.ink[ST_DISABLED] = list.disabled_ink;
    themed_style_.item_palette.icon[ST_DISABLED] = list.palette.icon[ST_DISABLED];

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
    SyncModel();
    InvalidateGeometry();
    return *this;
}

UiGallery& UiGallery::SetItemSize(Size size)
{
    size.cx = max(DPI(16), size.cx);
    size.cy = max(DPI(16), size.cy);
    if(item_size_ == size)
        return *this;
    item_size_ = size;
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
    UpdateVisibleRangeNotification();
    return *this;
}

UiGallery& UiGallery::SetSelectionMode(UiGallerySelectionMode mode)
{
    if(selection_mode_ == mode)
        return *this;
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
