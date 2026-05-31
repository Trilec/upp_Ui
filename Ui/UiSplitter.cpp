#include <Ui/UiSplitter.h>
#include <Ui/UiTheme.h>

namespace Upp {

const UiSplitter::Style& UiSplitter::StyleDefault()
{
    static Style s;
    ONCELOCK {
        for(int i = 0; i < 4; i++) {
            s.track_palette.face[i] = UiFill::Solid(Color(203, 213, 225));
            s.track_palette.frame[i] = Null;
            s.track_palette.ink[i] = Color(100, 116, 139);

            s.thumb_palette.face[i] = UiFill::Solid(Color(241, 245, 249));
            s.thumb_palette.frame[i] = Color(203, 213, 225);
            s.thumb_palette.ink[i] = Color(100, 116, 139);

            s.background_palette.face[i] = UiFill::Solid(Color(255, 255, 255));
            s.background_palette.frame[i] = Null;
            s.background_palette.ink[i] = Color(15, 23, 42);
        }

        s.track_palette.face[ST_HOT] = UiFill::Solid(Color(148, 163, 184));
        s.track_palette.face[ST_PRESSED] = UiFill::Solid(Color(100, 116, 139));
        s.track_palette.ink[ST_DISABLED] = Color(148, 163, 184);

        s.thumb_palette.face[ST_HOT] = UiFill::Solid(Color(226, 232, 240));
        s.thumb_palette.face[ST_PRESSED] = UiFill::Solid(Color(203, 213, 225));
        s.thumb_palette.frame[ST_HOT] = Color(148, 163, 184);
        s.thumb_palette.frame[ST_PRESSED] = Color(100, 116, 139);
        s.thumb_palette.ink[ST_DISABLED] = Color(148, 163, 184);

        s.track_metrics.face_enabled = true;
        s.track_metrics.frame_enabled = false;
        s.track_metrics.frame_width = 0;
        s.track_metrics.radius = 0;
        s.track_metrics.focus_enabled = false;

        s.thumb_metrics.face_enabled = true;
        s.thumb_metrics.frame_enabled = true;
        s.thumb_metrics.frame_width = 1;
        s.thumb_metrics.radius = DPI(4);
        s.thumb_metrics.focus_enabled = false;

        s.background_metrics.face_enabled = true;
        s.background_metrics.frame_enabled = false;
        s.background_metrics.focus_enabled = false;

        s.hit_width = DPI(8);
        s.track_thickness = DPI(1);
        s.track_inset = Rect(0, 0, 0, 0);
        s.thumb_main = DPI(42);
        s.thumb_cross = DPI(8);
        s.thumb_inset = Rect(0, 0, 0, 0);
        s.paint_background = false;
        s.show_grip = true;
        s.grip_dot_count = 6;
        s.grip_dot_gap = DPI(3);
        s.grip_dot_size = DPI(2);
        s.grip_color = Null;
        s.label_font = SansSerifZ(11);
        s.label_color = Null;
        s.label_gap = DPI(6);
        s.thumb_icon_size = DPI(14);
        s.thumb_icon_render_mode = UiIconRenderMode::MonoTint;
    }
    return s;
}

UiSplitter::UiSplitter()
    : style_(StyleDefault())
    , themed_style_(StyleDefault())
{
    zoom_index_ = -1;
    vertical_ = false;
    drag_index_ = -1;
    hot_index_ = -1;
    pos_.Add(5000);
    SetFrame(NullFrame());
    NoWantFocus();
    BackPaint();
    HSizePos(0, 0).VSizePos(0, 0);
    SyncThemeStyle();
}

void UiSplitter::InvalidateStyleCache()
{
    theme_revision_ = 0;
}

UiSplitter::Style& UiSplitter::StyleEdit()
{
    if(!has_custom_style_) {
        style_ = GetEffectiveStyle();
        has_custom_style_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

void UiSplitter::SyncThemeStyle()
{
    if(has_custom_style_)
        return;

    uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;

    themed_style_ = UiTheme::ResolveSplitter();
    theme_revision_ = revision;
}

UiSplitter& UiSplitter::SetCustomStyle(const Style& s)
{
    style_ = Style(s);
    has_custom_style_ = true;
    OnStyleChanged();
    return *this;
}

UiSplitter& UiSplitter::ClearCustomStyle()
{
    if(!has_custom_style_)
        return *this;

    has_custom_style_ = false;
    style_ = StyleDefault();
    InvalidateStyleCache();
    OnStyleChanged();
    return *this;
}

const UiSplitter::Style& UiSplitter::GetEffectiveStyle() const
{
    if(has_custom_style_)
        return style_;

    const_cast<UiSplitter*>(this)->SyncThemeStyle();
    return themed_style_;
}

void UiSplitter::OnStyleChanged()
{
    RefreshLayout();
    Refresh();
}

int UiSplitter::ClientToPos(Point p) const
{
    Size sz = GetSize();
    int axis = vertical_ ? sz.cy : sz.cx;
    if(axis <= 0)
        return 0;
    return minmax((vertical_ ? p.y : p.x) * 10000 / axis, 0, 9999);
}

int UiSplitter::PosToClient(int pos) const
{
    Size sz = GetSize();
    int axis = vertical_ ? sz.cy : sz.cx;
    return axis * pos / 10000;
}

int UiSplitter::GetMins(int i) const
{
    int min_ratio = i < mins_.GetCount() ? mins_[i] : 0;
    int min_pixels = 0;
    int axis = vertical_ ? GetSize().cy : GetSize().cx;
    if(axis > 0)
        min_pixels = (i < minpx_.GetCount() ? minpx_[i] : 0) * 10000 / axis;
    return max(min_ratio, min_pixels);
}

void UiSplitter::SyncMin()
{
    for(int i = 0; i < pos_.GetCount(); i++)
        SetSplitUnits(pos_[i], i);
}

Rect UiSplitter::GetHitRect(int index) const
{
    Size sz = GetSize();
    int width = GetSplitWidth();
    int p = PosToClient(pos_[index]) - (width >> 1);
    return vertical_ ? RectC(0, p, sz.cx, width) : RectC(p, 0, width, sz.cy);
}

Rect UiSplitter::GetTrackRect(int index) const
{
    const Style& style = GetEffectiveStyle();
    Rect r = GetHitRect(index);
    r.Deflate(style.track_inset);
    int thick = max(1, style.track_thickness);
    int p = PosToClient(pos_[index]);
    if(vertical_) {
        int y = p - (thick >> 1);
        return RectC(r.left, y, r.GetWidth(), thick);
    }
    int x = p - (thick >> 1);
    return RectC(x, r.top, thick, r.GetHeight());
}

Rect UiSplitter::GetThumbRect(int index) const
{
    const Style& style = GetEffectiveStyle();
    Rect hit = GetHitRect(index);
    int main = max(DPI(8), style.thumb_main);
    int cross = max(1, style.thumb_cross);
    int p = PosToClient(pos_[index]);
    if(vertical_) {
        Rect r = RectC(hit.left + (hit.GetWidth() - main) / 2,
                       p - (cross >> 1),
                       min(main, hit.GetWidth()),
                       cross);
        r.Deflate(style.thumb_inset);
        return r;
    }
    Rect r = RectC(p - (cross >> 1),
                   hit.top + (hit.GetHeight() - main) / 2,
                   cross,
                   min(main, hit.GetHeight()));
    r.Deflate(style.thumb_inset);
    return r;
}

int UiSplitter::FindIndex(Point p) const
{
    int best = -1;
    int maxdist = GetSplitWidth();
    for(int i = 0; i < pos_.GetCount(); i++) {
        int dist = abs((vertical_ ? p.y : p.x) - PosToClient(pos_[i]));
        if(dist <= maxdist) {
            best = i;
            maxdist = dist;
        }
    }
    return best;
}

void UiSplitter::Layout()
{
    Size sz = GetSize();
    int count = GetViewChildCount();
    if(count == 0)
        return;

    while(mins_.GetCount() < count)
        mins_.Add(0);
    while(minpx_.GetCount() < count)
        minpx_.Add(0);
    if(mins_.GetCount() > count)
        mins_.SetCount(count);
    if(minpx_.GetCount() > count)
        minpx_.SetCount(count);
    count--;

    if(pos_.GetCount() < count) {
        pos_.SetCount(count, 0);
        for(int i = 0; i < count; i++)
            pos_[i] = (i + 1) * 10000 / (count + 1);
    }
    if(pos_.GetCount() > count)
        pos_.SetCount(count);

    if(zoom_index_ >= 0) {
        int i = 0;
        for(Ctrl* q = GetFirstChild(); q; q = q->GetNext()) {
            if(q->InFrame())
                continue;
            if(zoom_index_ == i)
                q->SizePos().Show();
            else
                q->Hide();
            i++;
        }
        return;
    }

    int width = GetSplitWidth();
    int lw = width >> 1;
    int rw = width - lw;

    int i = 0;
    for(Ctrl* q = GetFirstChild(); q; q = q->GetNext()) {
        if(q->InFrame())
            continue;

        int lo = i > 0 ? PosToClient(pos_[i - 1]) + rw : 0;
        int hi = i < count ? PosToClient(pos_[i]) - lw : (vertical_ ? sz.cy : sz.cx);
        q->Show();
        if(vertical_)
            q->SetRect(0, lo, sz.cx, max(0, hi - lo));
        else
            q->SetRect(lo, 0, max(0, hi - lo), sz.cy);
        i++;
    }
}

void UiSplitter::PaintGrip(Draw& w, const Rect& r, StyledState st) const
{
    const Style& style = GetEffectiveStyle();
    if(!style.show_grip || style.grip_dot_count <= 0 || style.grip_dot_size <= 0)
        return;

    Color c = IsNull(style.grip_color) ? style.thumb_palette.ink[st] : style.grip_color;
    if(IsNull(c))
        return;

    int count = max(1, style.grip_dot_count);
    int dot = max(1, style.grip_dot_size);
    int gap = max(1, style.grip_dot_gap);
    int total = count * dot + (count - 1) * gap;

    Size label_sz(0, 0);
    if(!style.label.IsEmpty())
        label_sz = GetTextSize(style.label, style.label_font);

    int icon = !IsNull(style.thumb_icon) ? max(1, style.thumb_icon_size) : 0;
    int reserve = (vertical_ ? label_sz.cx : label_sz.cy) + (icon ? icon + style.label_gap : 0);
    Rect grip_area = r;
    if(reserve > 0) {
        if(vertical_)
            grip_area.left += reserve / 2;
        else
            grip_area.top += reserve / 2;
    }

    int x = grip_area.left + (grip_area.GetWidth() - dot) / 2;
    int y = grip_area.top + (grip_area.GetHeight() - dot) / 2;
    if(vertical_)
        x = grip_area.left + (grip_area.GetWidth() - total) / 2;
    else
        y = grip_area.top + (grip_area.GetHeight() - total) / 2;

    for(int i = 0; i < count; i++) {
        w.DrawRect(RectC(x, y, dot, dot), c);
        if(vertical_)
            x += dot + gap;
        else
            y += dot + gap;
    }
}

void UiSplitter::Paint(Draw& w)
{
    const Style& style = GetEffectiveStyle();
    Rect outer = GetSize();
    if(outer.IsEmpty())
        return;

    if(style.paint_background)
        UiPaintStyledSurface(w, outer, style.background_palette, style.background_metrics, style.background_skin, IsEnabled() ? ST_NORMAL : ST_DISABLED, HasFocus(), false, false);

    if(zoom_index_ >= 0)
        return;

    for(int i = 0; i < pos_.GetCount(); i++) {
        StyledState st = IsEnabled() ? ST_NORMAL : ST_DISABLED;
        if(IsEnabled() && HasCapture() && i == drag_index_)
            st = ST_PRESSED;
        else if(IsEnabled() && i == hot_index_)
            st = ST_HOT;

        Rect track = GetTrackRect(i);
        UiPaintStyledSurface(w, track, style.track_palette, style.track_metrics, style.track_skin, st, false, false, false);

        Rect thumb = GetThumbRect(i);
        UiPaintStyledSurface(w, thumb, style.thumb_palette, style.thumb_metrics, style.thumb_skin, st, false, false, false);

        Color ink = IsNull(style.label_color) ? style.thumb_palette.ink[st] : style.label_color;
        int cursor = vertical_ ? thumb.left + style.label_gap : thumb.top + style.label_gap;
        if(!IsNull(style.thumb_icon)) {
            int icon = max(1, style.thumb_icon_size);
            Rect ir = vertical_
                    ? RectC(cursor, thumb.top + (thumb.GetHeight() - icon) / 2, icon, icon)
                    : RectC(thumb.left + (thumb.GetWidth() - icon) / 2, cursor, icon, icon);
            UiPaintStyledIcon(w, ir, style.thumb_icon, true, true, style.thumb_icon_render_mode, ink, IsEnabled());
            cursor += icon + style.label_gap;
        }
        if(!style.label.IsEmpty()) {
            Size tsz = GetTextSize(style.label, style.label_font);
            w.Clip(thumb);
            if(vertical_) {
                int y = thumb.top + (thumb.GetHeight() - tsz.cy) / 2;
                w.DrawText(cursor, y, style.label, style.label_font, ink);
            }
            else {
                int x = thumb.left + (thumb.GetWidth() - tsz.cx) / 2;
                w.DrawText(x, cursor, style.label, style.label_font, ink);
            }
            w.End();
        }
        PaintGrip(w, thumb, st);
    }
}

void UiSplitter::MouseMove(Point p, dword)
{
    if(HasCapture() && drag_index_ >= 0 && drag_index_ < pos_.GetCount()) {
        SetSplitUnits(ClientToPos(p), drag_index_);
        Refresh();
        WhenAction();
        return;
    }

    int h = FindIndex(p);
    if(h != hot_index_) {
        hot_index_ = h;
        Refresh();
    }
}

void UiSplitter::MouseLeave()
{
    if(HasCapture())
        return;
    if(hot_index_ >= 0) {
        hot_index_ = -1;
        Refresh();
    }
}

void UiSplitter::LeftDown(Point p, dword)
{
    drag_index_ = FindIndex(p);
    if(drag_index_ < 0)
        return;
    SetCapture();
    Refresh();
}

void UiSplitter::LeftUp(Point, dword)
{
    bool was_dragging = HasCapture() && drag_index_ >= 0;
    ReleaseCapture();
    drag_index_ = -1;
    if(was_dragging)
        WhenSplitFinish();
    Refresh();
}

Image UiSplitter::CursorImage(Point p, dword)
{
    return FindIndex(p) < 0 ? Image::Arrow() : vertical_ ? Image::SizeVert() : Image::SizeHorz();
}

Size UiSplitter::GetMinSize() const
{
    return GetContentSize();
}

Size UiSplitter::GetContentSize() const
{
    Size out(0, 0);
    int visible_count = 0;
    for(Ctrl* q = GetFirstChild(); q; q = q->GetNext()) {
        if(!q->IsShown())
            continue;
        Size sz = q->GetMinSize();
        if(vertical_) {
            out.cx = max(out.cx, sz.cx);
            out.cy += sz.cy;
        }
        else {
            out.cx += sz.cx;
            out.cy = max(out.cy, sz.cy);
        }
        visible_count++;
    }
    if(visible_count <= 0)
        return Size(DPI(80), DPI(80));
    int split = max(1, GetSplitWidth()) * max(0, visible_count - 1);
    if(vertical_)
        out.cy += split;
    else
        out.cx += split;
    return out;
}

UiSplitter& UiSplitter::SetSplitUnits(int p, int i)
{
    if(i < 0)
        return *this;
    int l = (i > 0 && i - 1 < pos_.GetCount() ? pos_[i - 1] : 0) + GetMins(i);
    int h = (i + 1 < pos_.GetCount() ? pos_[i + 1] : 10000) - GetMins(i + 1);
    pos_.At(i) = minmax(p, l, h);
    Layout();
    Refresh();
    return *this;
}

UiSplitter& UiSplitter::SetSplitPercent(double percent, int i)
{
    return SetSplitUnits((int)(minmax(percent, 0.0, 100.0) * 100.0 + 0.5), i);
}

double UiSplitter::GetSplitPercent(int i) const
{
    return GetSplitUnits(i) / 100.0;
}

void UiSplitter::Add(Ctrl& pane)
{
    Ctrl::Add(pane);
    pos_.Clear();
    Layout();
    Refresh();
}

void UiSplitter::Insert(int ii, Ctrl& pane)
{
    if(ii >= GetCount())
        Add(pane);
    else {
        Ctrl::AddChildBefore(&pane, GetIndexChild(ii));
        pos_.Clear();
        Layout();
        Refresh();
    }
}

void UiSplitter::Remove(Ctrl& ctrl)
{
    int n = 0;
    for(Ctrl* c = GetFirstChild(); c; c = c->GetNext()) {
        if(c == &ctrl) {
            if(c->GetNext())
                pos_.Remove(n);
            else if(n >= 1)
                pos_.Remove(n - 1);
            if(n < mins_.GetCount())
                mins_.Remove(n);
            if(n < minpx_.GetCount())
                minpx_.Remove(n);
            RemoveChild(c);
            break;
        }
        n++;
    }
    Layout();
    Refresh();
}

void UiSplitter::Swap(Ctrl& child, Ctrl& newctrl)
{
    newctrl.SetRect(child.GetRect());
    Ctrl::AddChildBefore(&newctrl, &child);
    Ctrl::RemoveChild(&child);
    Layout();
    Refresh();
}

void UiSplitter::Clear()
{
    while(GetFirstChild())
        RemoveChild(GetFirstChild());
    pos_.Clear();
    mins_.Clear();
    minpx_.Clear();
    hot_index_ = -1;
    drag_index_ = -1;
}

void UiSplitter::Reset()
{
    Clear();
    zoom_index_ = -1;
    vertical_ = false;
}

UiSplitter& UiSplitter::Set(Ctrl& a, Ctrl& b)
{
    Clear();
    *this << a << b;
    Layout();
    Refresh();
    return *this;
}

UiSplitter& UiSplitter::Horz(Ctrl& left, Ctrl& right)
{
    vertical_ = false;
    return Set(left, right);
}

UiSplitter& UiSplitter::Vert(Ctrl& top, Ctrl& bottom)
{
    vertical_ = true;
    return Set(top, bottom);
}

void UiSplitter::Zoom(int i)
{
    zoom_index_ = i;
    Layout();
    Refresh();
}

void UiSplitter::Serialize(Stream& s)
{
    int version = 1;
    s / version;
    s % pos_ % zoom_index_ % vertical_;
    if(s.IsLoading()) {
        for(int i = 0; i < pos_.GetCount(); i++) {
            if(pos_[i] < 0 || pos_[i] > 10000) {
                pos_.Clear();
                s.LoadError();
                break;
            }
        }
        if(zoom_index_ >= GetViewChildCount()) {
            zoom_index_ = -1;
            s.LoadError();
        }
        Layout();
        Refresh();
    }
}

UiSplitter& UiSplitter::SetLabel(const String& s)
{
    StyleEdit().label = s;
    OnStyleChanged();
    return *this;
}

UiSplitter& UiSplitter::SetThumbIcon(const Image& img)
{
    StyleEdit().thumb_icon = img;
    OnStyleChanged();
    return *this;
}

UiSplitter& UiSplitter::SetThumbSize(int main, int cross)
{
    Style& s = StyleEdit();
    s.thumb_main = max(0, main);
    s.thumb_cross = max(0, cross);
    OnStyleChanged();
    return *this;
}

UiSplitter& UiSplitter::SetThumbSizePixels(int width, int height)
{
    return vertical_ ? SetThumbSize(height, width) : SetThumbSize(width, height);
}

UiSplitter& UiSplitter::SetTrackThickness(int px)
{
    StyleEdit().track_thickness = max(1, px);
    OnStyleChanged();
    return *this;
}

} // namespace Upp
