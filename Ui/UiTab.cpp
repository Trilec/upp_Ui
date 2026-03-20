#include <Ui/UiTab.h>
#include <Ui/UiIcons.h>
#include <Ui/UiTheme.h>

namespace Upp {

const UiTab::Style& UiTab::StyleDefault()
{
    static Style s;
    static bool init = false;
    if(!init) {
        Color face = Color(248, 250, 252);
        Color frame = Color(226, 232, 240);
        Color ink = Color(71, 85, 105);

        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(face);
            s.palette.frame[i] = frame;
            s.palette.ink[i] = ink;

            s.tab_palette.face[i] = UiFill::Solid(Color(255, 255, 255));
            s.tab_palette.frame[i] = frame;
            s.tab_palette.ink[i] = ink;
        }

        s.tab_palette.face[ST_HOT] = UiFill::Solid(Color(241, 245, 249));
        s.tab_palette.face[ST_PRESSED] = UiFill::Solid(Color(255, 255, 255));
        s.tab_palette.frame[ST_PRESSED] = Color(148, 163, 184);
        s.tab_palette.ink[ST_HOT] = Color(30, 41, 59);
        s.tab_palette.ink[ST_PRESSED] = Color(15, 23, 42);
        s.tab_palette.ink[ST_DISABLED] = Color(148, 163, 184);

        s.metrics.face_enabled = true;
        s.metrics.frame_enabled = true;
        s.metrics.frame_width = DPI(1);
        s.metrics.radius = DPI(10);
        s.metrics.content_padding = Rect(DPI(6), DPI(6), DPI(6), DPI(6));

        s.tab_metrics.face_enabled = true;
        s.tab_metrics.frame_enabled = true;
        s.tab_metrics.frame_width = DPI(1);
        s.tab_metrics.radius = DPI(8);
        s.tab_metrics.content_padding = Rect(0, 0, 0, 0);

        s.tab_extent = DPI(34);
        s.tab_gap = DPI(6);
        s.body_gap = DPI(6);
        s.tab_padding = Rect(DPI(12), DPI(7), DPI(12), DPI(7));
        s.strip_inset = Rect(0, 0, 0, 0);
        s.icon_text_gap = DPI(6);
        s.affordance_gap = DPI(4);
        s.affordance_size = DPI(12);
        s.min_tab_main = DPI(72);
        s.indicator_thickness = DPI(3);
        s.indicator_span = LARGE;
        s.fill_tabs = false;
        s.metrics.focus_enabled = false;
        s.visual = UITAB_CLASSIC;
        init = true;
    }
    return s;
}

UiTab::UiTab()
{
    Ctrl::Add(pane_.SizePos());
    pane_.Transparent();
    BackPaint();
    WantFocus();
    SyncThemeStyle();
    OnStyleChanged();
}

void UiTab::InvalidateStyleCache()
{
    theme_revision_ = 0;
}

UiTab::Style& UiTab::StyleEdit()
{
    if(!has_style_override_) {
        style_ = GetEffectiveStyle();
        has_style_override_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

void UiTab::SyncThemeStyle()
{
    if(has_style_override_)
        return;

    const uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;

    themed_style_ = UiTheme::ResolveTab(visual_);
    theme_revision_ = revision;
}

const UiTab::Style& UiTab::GetEffectiveStyle() const
{
    if(has_style_override_)
        return style_;

    const_cast<UiTab*>(this)->SyncThemeStyle();
    return themed_style_;
}

UiTab& UiTab::SetStyle(const Style& s)
{
    style_ = s;
    has_style_override_ = true;
    OnStyleChanged();
    return *this;
}

UiTab& UiTab::ClearStyleOverride()
{
    if(!has_style_override_)
        return *this;

    has_style_override_ = false;
    style_ = StyleDefault();
    InvalidateStyleCache();
    OnStyleChanged();
    return *this;
}

void UiTab::OnStyleChanged()
{
    RebuildAllTextSizes();
    RefreshLayout();
    Refresh();
}

UiTab& UiTab::SetPlacement(UiAlign side)
{
    if(side != UiAlign::TOP && side != UiAlign::BOTTOM && side != UiAlign::LEFT && side != UiAlign::RIGHT)
        side = UiAlign::TOP;
    if(placement_ == side)
        return *this;
    placement_ = side;
    RefreshLayout();
    Refresh();
    return *this;
}

UiTab& UiTab::SetVisual(UiTabVisual visual)
{
    if(visual < UITAB_CLASSIC || visual > UITAB_DOCUMENT)
        visual = UITAB_CLASSIC;
    if(visual_ == visual)
        return *this;
    visual_ = visual;
    InvalidateStyleCache();
    OnStyleChanged();
    return *this;
}

bool UiTab::IsHorizontal() const
{
    return placement_ == UiAlign::TOP || placement_ == UiAlign::BOTTOM;
}

void UiTab::RebuildTextSize(TabItem& t)
{
    const Style& style = GetEffectiveStyle();
    Font f = style.tab_font;
    if(IsNull(f))
        f = StdFont();
    t.text_size = t.text.IsEmpty() ? Size(0, 0) : GetTextSize(t.text, f);
}

void UiTab::RebuildAllTextSizes()
{
    for(int i = 0; i < tabs_.GetCount(); i++)
        RebuildTextSize(tabs_[i]);
}

int UiTab::Add(Ctrl& page, const String& title, const Image& icon)
{
    TabItem& t = tabs_.Add();
    t.text = title;
    t.icon = icon;
    t.page = &page;
    t.enabled = true;
    RebuildTextSize(t);

    pane_.Add(page.SizePos());
    page.Hide();

    if(active_ < 0)
        active_ = tabs_.GetCount() - 1;

    SyncPages();
    RefreshLayout();
    Refresh();
    return tabs_.GetCount() - 1;
}

void UiTab::Remove(int i)
{
    if(i < 0 || i >= tabs_.GetCount())
        return;

    Ctrl* pg = tabs_[i].page;
    if(pg)
        pg->Remove();

    tabs_.Remove(i);

    if(tabs_.IsEmpty())
        active_ = -1;
    else if(active_ >= tabs_.GetCount())
        active_ = tabs_.GetCount() - 1;
    else if(i < active_)
        active_--;

    hot_ = -1;
    SyncPages();
    RefreshLayout();
    Refresh();
}

void UiTab::Clear()
{
    for(int i = 0; i < tabs_.GetCount(); i++) {
        if(tabs_[i].page)
            tabs_[i].page->Remove();
    }
    tabs_.Clear();
    active_ = -1;
    hot_ = -1;
    RefreshLayout();
    Refresh();
}

UiTab& UiTab::SetTabText(int i, const String& text)
{
    if(i < 0 || i >= tabs_.GetCount())
        return *this;
    tabs_[i].text = text;
    RebuildTextSize(tabs_[i]);
    RefreshLayout();
    Refresh();
    return *this;
}

UiTab& UiTab::SetTabIcon(int i, const Image& icon)
{
    if(i < 0 || i >= tabs_.GetCount())
        return *this;
    tabs_[i].icon = icon;
    RefreshLayout();
    Refresh();
    return *this;
}

UiTab& UiTab::EnableTab(int i, bool on)
{
    if(i < 0 || i >= tabs_.GetCount())
        return *this;
    tabs_[i].enabled = on;
    if(!on && i == active_)
        active_ = FindEnabled(i, +1);
    SyncPages();
    Refresh();
    return *this;
}

UiTab& UiTab::SetTabClosable(int i, bool on)
{
    if(i < 0 || i >= tabs_.GetCount())
        return *this;
    tabs_[i].closable = on;
    RefreshLayout();
    Refresh();
    return *this;
}

UiTab& UiTab::SetTabDraggable(int i, bool on)
{
    if(i < 0 || i >= tabs_.GetCount())
        return *this;
    tabs_[i].draggable = on;
    RefreshLayout();
    Refresh();
    return *this;
}

bool UiTab::IsTabEnabled(int i) const
{
    return i >= 0 && i < tabs_.GetCount() && tabs_[i].enabled;
}

Ctrl& UiTab::GetPage(int i)
{
    ASSERT(i >= 0 && i < tabs_.GetCount() && tabs_[i].page);
    return *tabs_[i].page;
}

const Ctrl& UiTab::GetPage(int i) const
{
    ASSERT(i >= 0 && i < tabs_.GetCount() && tabs_[i].page);
    return *tabs_[i].page;
}

int UiTab::FindEnabled(int start, int step) const
{
    if(tabs_.IsEmpty())
        return -1;
    int i = start;
    for(int n = 0; n < tabs_.GetCount(); n++) {
        i += step;
        if(i < 0)
            i = tabs_.GetCount() - 1;
        else if(i >= tabs_.GetCount())
            i = 0;
        if(tabs_[i].enabled)
            return i;
    }
    return -1;
}

UiTab& UiTab::SetActiveTab(int i)
{
    if(i < 0 || i >= tabs_.GetCount() || !tabs_[i].enabled)
        return *this;
    if(active_ == i)
        return *this;
    active_ = i;
    SyncPages();
    Refresh();
    if(WhenAction)
        WhenAction();
    return *this;
}

void UiTab::SetData(const Value& v)
{
    if(IsNull(v))
        return;
    SetActiveTab((int)v);
}

Value UiTab::GetData() const
{
    return active_;
}

void UiTab::SyncPages()
{
    for(int i = 0; i < tabs_.GetCount(); i++) {
        Ctrl* pg = tabs_[i].page;
        if(!pg)
            continue;
        if(i == active_)
            pg->Show();
        else
            pg->Hide();
    }
}

Size UiTab::GetMinSize() const
{
    int w = DPI(220);
    int h = DPI(140);
    for(int i = 0; i < tabs_.GetCount(); i++) {
        if(tabs_[i].page) {
            Size ms = tabs_[i].page->GetMinSize();
            w = max(w, ms.cx);
            h = max(h, ms.cy);
        }
    }
    const Style& style = GetEffectiveStyle();
    int ex = max(DPI(24), style.tab_extent) + style.tab_gap;
    if(IsHorizontal())
        h += ex;
    else
        w += ex;
    return UiStyledOuterSizeFromContent(Size(w, h), style.metrics, style.skin);
}

void UiTab::Layout()
{
    Rect outer = GetSize();
    const Style& style = GetEffectiveStyle();
    Rect content = UiStyledInnerRect(outer, style.metrics, style.skin);
    if(content.IsEmpty()) {
        strip_rect_ = Rect(0, 0, 0, 0);
        pane_.SetRect(0, 0, 0, 0);
        return;
    }

    int ex = max(DPI(24), style.tab_extent);
    Rect pane_r = content;
    int body_gap = max(0, style.body_gap);

    switch(placement_) {
    case UiAlign::BOTTOM:
        strip_rect_ = Rect(content.left, content.bottom - ex, content.right, content.bottom);
        pane_r.bottom = strip_rect_.top - body_gap;
        break;
    case UiAlign::LEFT:
        strip_rect_ = Rect(content.left, content.top, content.left + ex, content.bottom);
        pane_r.left = strip_rect_.right + body_gap;
        break;
    case UiAlign::RIGHT:
        strip_rect_ = Rect(content.right - ex, content.top, content.right, content.bottom);
        pane_r.right = strip_rect_.left - body_gap;
        break;
    case UiAlign::TOP:
    default:
        strip_rect_ = Rect(content.left, content.top, content.right, content.top + ex);
        pane_r.top = strip_rect_.bottom + body_gap;
        break;
    }

    pane_.SetRect(pane_r);
    tabs_rect_ = UiApplyThicknessRect(strip_rect_, UiNonNegativeThickness(style.strip_inset));

    int n = tabs_.GetCount();
    if(n <= 0)
        return;

    int gap = max(0, style.tab_gap);
    bool horz = IsHorizontal();
    int avail = horz ? tabs_rect_.GetWidth() : tabs_rect_.GetHeight();
    Vector<int> pref;
    pref.SetCount(n);
    int pref_sum = 0;

    for(int i = 0; i < n; i++) {
        const TabItem& t = tabs_[i];
        int icon_w = IsNull(t.icon) ? 0 : (horz ? style.tab_extent - DPI(14) : style.tab_extent - DPI(14));
        int aff_count = 0;
        if(show_drag_handles_ && t.draggable)
            aff_count++;
        if(show_close_buttons_ && t.closable)
            aff_count++;
        int aff_w = aff_count > 0 ? (aff_count * style.affordance_size + max(0, aff_count - 1) * style.affordance_gap + style.affordance_gap) : 0;
        int main = (horz ? t.text_size.cx : t.text_size.cy) + style.tab_padding.left + style.tab_padding.right;
        if(icon_w > 0)
            main += icon_w + style.icon_text_gap;
        main += aff_w;
        main = max(main, style.min_tab_main);
        pref[i] = main;
        pref_sum += main;
    }

    int fixed_gap = gap * max(0, n - 1);
    Vector<int> tab_main;
    tab_main.SetCount(n);

    if(style.fill_tabs || pref_sum + fixed_gap > avail) {
        int each = max(1, (avail - fixed_gap) / n);
        for(int i = 0; i < n; i++)
            tab_main[i] = each;
    }
    else {
        for(int i = 0; i < n; i++)
            tab_main[i] = pref[i];
    }

    int cursor = horz ? tabs_rect_.left : tabs_rect_.top;
    for(int i = 0; i < n; i++) {
        if(horz)
            tabs_[i].tab_rect = Rect(cursor, tabs_rect_.top, cursor + tab_main[i], tabs_rect_.bottom);
        else
            tabs_[i].tab_rect = Rect(tabs_rect_.left, cursor, tabs_rect_.right, cursor + tab_main[i]);

        if(style.visual == UITAB_CLASSIC && (style.tab_metrics.face_enabled || style.tab_metrics.frame_enabled)) {
            Rect rr = tabs_[i].tab_rect;
            int lift = DPI(3);
            bool active = (i == active_);
            switch(placement_) {
            case UiAlign::TOP:
                if(!active) {
                    rr.top += lift;
                }
                break;
            case UiAlign::BOTTOM:
                if(!active) {
                    rr.bottom -= lift;
                }
                break;
            case UiAlign::LEFT:
                if(!active) {
                    rr.left += lift;
                }
                break;
            case UiAlign::RIGHT:
                if(!active) {
                    rr.right -= lift;
                }
                break;
            default:
                break;
            }
            tabs_[i].tab_rect = rr;
        }

        tabs_[i].close_rect = Rect(0, 0, 0, 0);
        tabs_[i].drag_rect = Rect(0, 0, 0, 0);
        Rect ir = tabs_[i].tab_rect;
        ir.left += style.tab_padding.left;
        ir.right -= style.tab_padding.right;
        ir.top += style.tab_padding.top;
        ir.bottom -= style.tab_padding.bottom;
        int a = max(DPI(10), style.affordance_size);
        int y = ir.top + (ir.GetHeight() - a) / 2;
        int right = ir.right;
        if(show_close_buttons_ && tabs_[i].closable) {
            tabs_[i].close_rect = RectC(right - a, y, a, a);
            right = tabs_[i].close_rect.left - style.affordance_gap;
        }
        if(show_drag_handles_ && tabs_[i].draggable) {
            tabs_[i].drag_rect = RectC(right - a, y, a, a);
        }

        cursor += tab_main[i] + gap;
    }

    SyncPages();
}

int UiTab::FindTabAt(Point p) const
{
    for(int i = 0; i < tabs_.GetCount(); i++) {
        if(tabs_[i].tab_rect.Contains(p))
            return i;
    }
    return -1;
}

int UiTab::FindCloseAt(Point p) const
{
    for(int i = 0; i < tabs_.GetCount(); i++)
        if(!tabs_[i].close_rect.IsEmpty() && tabs_[i].close_rect.Contains(p))
            return i;
    return -1;
}

int UiTab::FindDragAt(Point p) const
{
    for(int i = 0; i < tabs_.GetCount(); i++)
        if(!tabs_[i].drag_rect.IsEmpty() && tabs_[i].drag_rect.Contains(p))
            return i;
    return -1;
}

void UiTab::MoveTab(int from, int to)
{
    if(from < 0 || from >= tabs_.GetCount() || to < 0 || to >= tabs_.GetCount() || from == to)
        return;

    int old_active = active_;

    if(from < to) {
        for(int i = from; i < to; i++)
            Swap(tabs_[i], tabs_[i + 1]);
    }
    else {
        for(int i = from; i > to; i--)
            Swap(tabs_[i], tabs_[i - 1]);
    }

    if(old_active == from)
        active_ = to;
    else if(from < old_active && to >= old_active)
        active_--;
    else if(from > old_active && to <= old_active)
        active_++;

    Layout();
    Refresh();
    if(WhenReorder)
        WhenReorder(from, to);
}

void UiTab::Paint(Draw& w)
{
    Rect outer = GetSize();
    if(outer.IsEmpty())
        return;

    StyledState st = IsEnabled() ? ST_NORMAL : ST_DISABLED;
    bool has_focus = HasFocus();

    const Style& style = GetEffectiveStyle();
    UiPaintStyledBackground(w, outer, style.palette, style.metrics, style.skin, st, has_focus);

    if(!strip_rect_.IsEmpty() && style.metrics.face_enabled && style.palette.face[st].IsSolid()) {
        Color strip_face = style.palette.face[st].color;
        w.DrawRect(strip_rect_, Blend(strip_face, SColorPaper(), 220));
    }

    Font f = style.tab_font;
    if(IsNull(f))
        f = StdFont();

    Color accent = style.tab_palette.frame[ST_PRESSED];
    if(IsNull(accent))
        accent = SColorHighlight();
    Color pane_face = style.palette.face[st].IsSolid() ? style.palette.face[st].color : SColorFace();
    Color seam_edge = style.palette.frame[st];
    if(IsNull(seam_edge))
        seam_edge = Blend(SColorShadow(), Black(), 10);

    auto DrawTabContent = [&](const TabItem& t, StyledState ts) {
        Rect ir = t.tab_rect;
        ir.left += style.tab_padding.left;
        ir.right -= style.tab_padding.right;
        ir.top += style.tab_padding.top;
        ir.bottom -= style.tab_padding.bottom;

        Color ink = style.tab_palette.ink[ts];
        if(IsNull(ink))
            ink = SColorText();
        if(ts == ST_HOT)
            ink = LtColor(ink, 6);
        Color icon_ink = UiResolveIconColor(style.tab_palette, ts);
        if(IsNull(icon_ink))
            icon_ink = ink;

        int text_right = ir.right;

        if(!t.close_rect.IsEmpty())
            text_right = min(text_right, t.close_rect.left - style.affordance_gap);
        if(!t.drag_rect.IsEmpty())
            text_right = min(text_right, t.drag_rect.left - style.affordance_gap);

        if(!IsNull(t.icon)) {
            int ico = max(DPI(12), min(ir.GetWidth(), ir.GetHeight()));
            int iw = min(ico, max(DPI(12), style.tab_extent - DPI(14)));
            Rect icon_r = RectC(ir.left, ir.top + (ir.GetHeight() - iw) / 2, iw, iw);
            UiPaintStyledIcon(w, icon_r, t.icon, true, true, icon_ink, ts != ST_DISABLED);
            ir.left = icon_r.right + style.icon_text_gap;
        }

        if(!t.drag_rect.IsEmpty()) {
            UiPaintStyledIcon(w, t.drag_rect, ICON_DESIGN_DRAG_INDICATOR_48(), true, true, icon_ink, ts != ST_DISABLED);
        }

        if(!t.close_rect.IsEmpty()) {
            Color c = (hot_close_ >= 0 && &t == &tabs_[hot_close_]) ? DkColor(icon_ink, 20) : icon_ink;
            int l = t.close_rect.left;
            int t0 = t.close_rect.top;
            int r0 = t.close_rect.right - 1;
            int b0 = t.close_rect.bottom - 1;
            w.DrawLine(l, t0, r0, b0, max(1, DPI(1)), c);
            w.DrawLine(l, b0, r0, t0, max(1, DPI(1)), c);
        }

        if(!t.text.IsEmpty() && !ir.IsEmpty()) {
            int tx = ir.left;
            int ty = ir.top + (ir.GetHeight() - t.text_size.cy) / 2;
            int maxw = max(1, text_right - tx);
            Font tf = f;
            if(ts == ST_HOT || ts == ST_PRESSED)
                tf = tf.Bold();
            DrawSmartText(w, tx, ty, maxw, t.text, tf, ink);
        }
    };

    if(style.visual == UITAB_SEGMENTED && style.tab_metrics.face_enabled && style.tab_palette.face[ST_NORMAL].IsSolid()) {
        Rect seg = tabs_rect_.Deflated(DPI(1), DPI(1));
        UiPaintStyledCap(w, seg, style.tab_palette, style.tab_metrics, ST_NORMAL, placement_, UICAP_FLAT_CLOSED);
    }

    if(style.visual == UITAB_DOCUMENT) {
        int th = 1;
        switch(UiCapOpenSide(placement_)) {
        case UiAlign::BOTTOM: w.DrawRect(strip_rect_.left, strip_rect_.bottom - th, strip_rect_.GetWidth(), th, seam_edge); break;
        case UiAlign::TOP:    w.DrawRect(strip_rect_.left, strip_rect_.top, strip_rect_.GetWidth(), th, seam_edge); break;
        case UiAlign::LEFT:   w.DrawRect(strip_rect_.left, strip_rect_.top, th, strip_rect_.GetHeight(), seam_edge); break;
        case UiAlign::RIGHT:  w.DrawRect(strip_rect_.right - th, strip_rect_.top, th, strip_rect_.GetHeight(), seam_edge); break;
        default: break;
        }
    }

    if(style.visual == UITAB_CLASSIC && (style.tab_metrics.face_enabled || style.tab_metrics.frame_enabled)) {
        int th = 1;
        int rr = max(2, style.tab_metrics.radius);
        Rect seam = strip_rect_;
        if(active_ >= 0 && active_ < tabs_.GetCount()) {
            Rect ar = tabs_[active_].tab_rect;
            if(placement_ == UiAlign::TOP) {
                int y = seam.bottom - th;
                int x1 = ar.left - min(rr, ar.GetWidth() / 2);
                int x2 = ar.right + min(rr, ar.GetWidth() / 2);
                w.DrawRect(seam.left, y, max(0, x1 - seam.left), th, seam_edge);
                w.DrawRect(x2, y, max(0, seam.right - x2), th, seam_edge);
            }
            else if(placement_ == UiAlign::BOTTOM) {
                int y = seam.top;
                int x1 = ar.left - min(rr, ar.GetWidth() / 2);
                int x2 = ar.right + min(rr, ar.GetWidth() / 2);
                w.DrawRect(seam.left, y, max(0, x1 - seam.left), th, seam_edge);
                w.DrawRect(x2, y, max(0, seam.right - x2), th, seam_edge);
            }
            else if(placement_ == UiAlign::LEFT) {
                int x = seam.right - th;
                int y1 = ar.top - min(rr, ar.GetHeight() / 2);
                int y2 = ar.bottom + min(rr, ar.GetHeight() / 2);
                w.DrawRect(x, seam.top, th, max(0, y1 - seam.top), seam_edge);
                w.DrawRect(x, y2, th, max(0, seam.bottom - y2), seam_edge);
            }
            else if(placement_ == UiAlign::RIGHT) {
                int x = seam.left;
                int y1 = ar.top - min(rr, ar.GetHeight() / 2);
                int y2 = ar.bottom + min(rr, ar.GetHeight() / 2);
                w.DrawRect(x, seam.top, th, max(0, y1 - seam.top), seam_edge);
                w.DrawRect(x, y2, th, max(0, seam.bottom - y2), seam_edge);
            }
        }
        else {
            switch(UiCapOpenSide(placement_)) {
            case UiAlign::BOTTOM: w.DrawRect(seam.left, seam.bottom - th, seam.GetWidth(), th, seam_edge); break;
            case UiAlign::TOP:    w.DrawRect(seam.left, seam.top, seam.GetWidth(), th, seam_edge); break;
            case UiAlign::LEFT:   w.DrawRect(seam.left, seam.top, th, seam.GetHeight(), seam_edge); break;
            case UiAlign::RIGHT:  w.DrawRect(seam.right - th, seam.top, th, seam.GetHeight(), seam_edge); break;
            default: break;
            }
        }
    }

    int pass_count = (style.visual == UITAB_CLASSIC || style.visual == UITAB_DOCUMENT) ? 2 : 1;
    for(int pass = 0; pass < pass_count; pass++) {
        for(int i = 0; i < tabs_.GetCount(); i++) {
            if(pass_count == 2) {
                bool active_pass = (pass == 1);
                if((i == active_) != active_pass)
                    continue;
            }

            const TabItem& t = tabs_[i];
            StyledState ts = !IsEnabled() || !t.enabled ? ST_DISABLED : (i == active_ ? ST_PRESSED : (i == hot_ ? ST_HOT : ST_NORMAL));
            UiCapShape shape = UICAP_NONE;
            StyledMetrics cap_metrics = style.tab_metrics;

            if(dragging_ && i == drag_to_) {
                shape = UICAP_LINE;
                cap_metrics.frame_width = max(cap_metrics.frame_width, DPI(3));
                ts = ST_PRESSED;
            }
            else {
                switch(style.visual) {
                case UITAB_CLASSIC:
                    shape = (style.tab_metrics.face_enabled || style.tab_metrics.frame_enabled) && i == active_ ? UICAP_OPEN : UICAP_FLAT_CLOSED;
                    break;
                case UITAB_DOCUMENT:
                    shape = (style.tab_metrics.face_enabled || style.tab_metrics.frame_enabled) && i == active_ ? UICAP_OPEN : UICAP_FLAT_CLOSED;
                    break;
                case UITAB_SEGMENTED:
                    shape = UICAP_CLOSED;
                    break;
                case UITAB_UNDERLINE:
                    shape = UICAP_NONE;
                    break;
                case UITAB_RAIL:
                    shape = (i == active_) ? UICAP_LINE_OPPOSITE : UICAP_NONE;
                    break;
                default:
                    shape = UICAP_NONE;
                    break;
                }
            }

            if(shape != UICAP_NONE) {
                StyledPalette cap_palette = style.tab_palette;

                if(ts == ST_HOT) {
                    Color base = Null;
                    if(cap_palette.face[ST_NORMAL].IsSolid())
                        base = cap_palette.face[ST_NORMAL].color;
                    else if(cap_palette.face[ST_HOT].IsSolid())
                        base = cap_palette.face[ST_HOT].color;

                    if(!IsNull(base)) {
                        int lum = (77 * base.GetR() + 150 * base.GetG() + 29 * base.GetB()) >> 8;
                        Color hot = (lum >= 170) ? Blend(base, Black(), 18) : Blend(base, White(), 18);
                        cap_palette.face[ST_HOT] = UiFill::Solid(hot);
                    }
                }

                if(shape == UICAP_LINE || shape == UICAP_LINE_OPPOSITE) {
                    for(int sidx = 0; sidx < 4; sidx++)
                        cap_palette.frame[sidx] = accent;
                }
                UiPaintStyledCap(w, t.tab_rect, cap_palette, cap_metrics, ts, placement_, shape);
            }

            if(style.visual == UITAB_UNDERLINE && i == active_) {
                int icon_w = IsNull(t.icon) ? 0 : max(DPI(12), style.tab_extent - DPI(14));
                int content_w = t.text_size.cx + (icon_w > 0 ? (icon_w + style.icon_text_gap) : 0) + style.tab_padding.left + style.tab_padding.right;
                int maxw = max(DPI(14), t.tab_rect.GetWidth() - DPI(8));
                int wline = maxw;
                if(style.indicator_span == SMALL)
                    wline = max(DPI(12), maxw / 3);
                else if(style.indicator_span == MEDIUM)
                    wline = max(DPI(16), maxw * 2 / 3);
                else
                    wline = min(maxw, max(DPI(16), content_w));
                int th = max(1, style.indicator_thickness);
                int x = t.tab_rect.left + (t.tab_rect.GetWidth() - wline) / 2;
                int y = IsHorizontal() ? (t.tab_rect.bottom - th) : (t.tab_rect.top + (t.tab_rect.GetHeight() - wline) / 2);
                if(IsHorizontal())
                    w.DrawRect(x, y, wline, th, accent);
                else
                    w.DrawRect(t.tab_rect.left, y, th, wline, accent);
            }

            DrawTabContent(t, ts);
        }
    }

    UiPaintStyledForeground(w, outer, style.palette, style.metrics, style.skin, st, has_focus);
}

void UiTab::LeftDown(Point p, dword)
{
    SetFocus();

    int close_hit = FindCloseAt(p);
    if(close_hit >= 0 && tabs_[close_hit].enabled && tabs_[close_hit].closable) {
        if(WhenClose)
            WhenClose(close_hit);
        Remove(close_hit);
        return;
    }

    int drag_hit = FindDragAt(p);
    int hit = FindTabAt(p);
    if(hit >= 0) {
        SetActiveTab(hit);
        drag_candidate_ = drag_reorder_enabled_ && (drag_hit == hit || (!show_drag_handles_ && tabs_[hit].draggable));
        dragging_ = false;
        drag_from_ = hit;
        drag_to_ = hit;
        drag_start_ = p;
        if(drag_candidate_)
            SetCapture();
    }
}

void UiTab::MouseMove(Point p, dword)
{
    int hclose = FindCloseAt(p);
    int hdrag = FindDragAt(p);

    if(hclose != hot_close_ || hdrag != hot_drag_) {
        hot_close_ = hclose;
        hot_drag_ = hdrag;
        Refresh();
    }

    if(drag_candidate_) {
        if(!dragging_) {
            int dx = abs(p.x - drag_start_.x);
            int dy = abs(p.y - drag_start_.y);
            if(dx >= DPI(5) || dy >= DPI(5))
                dragging_ = true;
        }
        if(dragging_) {
            int hit = FindTabAt(p);
            if(hit >= 0 && hit != drag_to_) {
                int from = drag_to_;
                drag_to_ = hit;
                MoveTab(from, drag_to_);
            }
            return;
        }
    }

    int hit = FindTabAt(p);
    if(hit != hot_) {
        hot_ = hit;
        Refresh();
    }
}

void UiTab::LeftUp(Point, dword)
{
    if(drag_candidate_) {
        drag_candidate_ = false;
        dragging_ = false;
        drag_from_ = -1;
        drag_to_ = -1;
        if(HasCapture())
            ReleaseCapture();
        hot_drag_ = -1;
    }
}

void UiTab::MouseLeave()
{
    if(drag_candidate_)
        return;
    hot_close_ = -1;
    hot_drag_ = -1;
    if(hot_ >= 0) {
        hot_ = -1;
        Refresh();
    }
}

bool UiTab::Key(dword key, int count)
{
    (void)count;
    if(tabs_.IsEmpty())
        return Ctrl::Key(key, count);

    bool horz = IsHorizontal();
    if((horz && key == K_LEFT) || (!horz && key == K_UP)) {
        int ni = FindEnabled(active_, -1);
        if(ni >= 0) {
            SetActiveTab(ni);
            return true;
        }
    }
    if((horz && key == K_RIGHT) || (!horz && key == K_DOWN)) {
        int ni = FindEnabled(active_, +1);
        if(ni >= 0) {
            SetActiveTab(ni);
            return true;
        }
    }
    if(key == K_HOME) {
        for(int i = 0; i < tabs_.GetCount(); i++)
            if(tabs_[i].enabled) {
                SetActiveTab(i);
                return true;
            }
    }
    if(key == K_END) {
        for(int i = tabs_.GetCount() - 1; i >= 0; i--)
            if(tabs_[i].enabled) {
                SetActiveTab(i);
                return true;
            }
    }
    return Ctrl::Key(key, count);
}

void UiTab::GotFocus()
{
    Refresh();
}

void UiTab::LostFocus()
{
    Refresh();
}

}












