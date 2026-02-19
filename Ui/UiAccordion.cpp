#include <Ui/UiAccordion.h>

namespace Upp {

void UiAccordion::SectionHeader::LeftDown(Point p, dword keyflags)
{
    UiTitleCard::LeftDown(p, keyflags);
    down = true;
    SetCapture();
    if(owner && index >= 0)
        owner->BeginHeaderDrag(index, GetMousePos());
}

void UiAccordion::SectionHeader::MouseMove(Point p, dword keyflags)
{
    UiTitleCard::MouseMove(p, keyflags);
    if(down && owner)
        owner->ContinueHeaderDrag(GetMousePos());
}

void UiAccordion::SectionHeader::LeftUp(Point p, dword keyflags)
{
    UiTitleCard::LeftUp(p, keyflags);
    if(HasCapture())
        ReleaseCapture();

    bool moved = owner ? owner->drag_moved_ : false;
    if(owner)
        owner->EndHeaderDrag(false);

    Rect bounds = GetSize();
    if(owner && index >= 0 && down && !moved && bounds.Contains(p))
        owner->Toggle(index);
    down = false;
}

bool UiAccordion::SectionHeader::Key(dword key, int count)
{
    if(!owner || index < 0)
        return UiTitleCard::Key(key, count);

    switch(key) {
    case K_SPACE:
    case K_ENTER:
        owner->Toggle(index);
        return true;
    case K_UP:
    case K_LEFT:
        owner->FocusHeader(index - 1);
        return true;
    case K_DOWN:
    case K_RIGHT:
        owner->FocusHeader(index + 1);
        return true;
    case K_HOME:
        owner->FocusHeader(0);
        return true;
    case K_END:
        owner->FocusHeader(owner->GetCount() - 1);
        return true;
    default:
        break;
    }
    return UiTitleCard::Key(key, count);
}

const UiAccordion::Style& UiAccordion::StyleDefault()
{
    static Style s;
    ONCELOCK {
        Color face  = Blend(SColorFace(), White(), 12);
        Color frame = Blend(SColorShadow(), Black(), 18);
        Color ink   = SColorText();

        for(int i = 0; i < 4; i++) {
            s.palette.face[i]  = UiFill::Solid(face);
            s.palette.frame[i] = frame;
            s.palette.ink[i]   = ink;
        }
        s.palette.face[ST_HOT]      = UiFill::Solid(LtColor(face, 4));
        s.palette.face[ST_PRESSED]  = UiFill::Solid(DkColor(face, 3));
        s.palette.face[ST_DISABLED] = UiFill::Solid(Blend(face, SColorDisabled(), 50));

        s.metrics.radius        = DPI(8);
        s.metrics.frame_width   = DPI(1);
        s.metrics.frame_enabled = true;
        s.metrics.face_enabled  = true;
        s.metrics.content_padding = Rect(DPI(6), DPI(6), DPI(6), DPI(6));

        s.header_style = UiTitleCard::StyleDefault();
        s.header_style.metrics.content_padding = Rect(DPI(10), DPI(8), DPI(10), DPI(8));
        s.header_style.hover_enabled = true;
        s.header_style.show_focus = false;
        s.header_style.show_rule = true;

        s.body_style = UiPanel::StyleDefault();
        s.body_style.metrics.content_padding = Rect(DPI(6), DPI(6), DPI(6), DPI(6));

        s.chevron_side = UiAlign::RIGHT;
        s.glyph_open = CtrlsImg::DA();
        s.glyph_closed = CtrlsImg::RA();
        s.glyph_lock = Image();
        s.chevron_scale = false;
        s.chevron_size = 0;

        s.unified_section_frame = false;
        s.unified_section_radius = DPI(7);
        s.unified_section_frame_width = 1;

        s.body_line_extent = NONE;
        s.body_line_style = SOLID;
        s.body_line_thickness = 1;
        s.body_line_color = Null;

        s.animation_enabled = true;
        s.anim_open_ms = 120;
        s.anim_close_ms = 0;
    }
    return s;
}

UiAccordion::UiAccordion()
    : style_(StyleDefault())
{
    BackPaint();
    Add(drag_marker_);
    drag_marker_.Color(Color(56, 146, 255)).IgnoreMouse().Hide();
}

UiAccordion::~UiAccordion()
{
    EndHeaderDrag(true);
    StopAllAnimations();
}

UiAccordion& UiAccordion::SetStyle(const Style& s)
{
    style_ = s;
    OnStyleChanged();
    return *this;
}

void UiAccordion::OnStyleChanged()
{
    if(style_.transparent)
        Transparent();
    else
        BackPaint();

    for(int i = 0; i < sections_.GetCount(); i++)
        ApplySectionStyle(sections_[i], i);

    for(int i = 0; i < sections_.GetCount(); i++) {
        sections_[i].animating = false;
        sections_[i].current_body_cy = sections_[i].open ? max(style_.body_min_height, sections_[i].body_height) : 0;
        sections_[i].target_body_cy = sections_[i].current_body_cy;
    }

    RefreshLayout();
    Refresh();
}

int UiAccordion::AddSection(const String& title, const String& subtitle, const String& copy, bool open)
{
    int i = sections_.GetCount();
    Section& s = sections_.Add();

    s.title    = title;
    s.subtitle = subtitle;
    s.copy     = copy;
    s.open     = open;
    s.current_body_cy = 0;
    s.target_body_cy = 0;
    s.anim_from_cy = 0;
    s.anim_start_ms = 0;
    s.anim_ms = 0;
    s.animating = false;

    s.header.owner = this;
    s.header.index = i;

    Add(s.header);
    Add(s.body);
    s.body.Add(s.content.SizePos());
    s.content.Transparent();

    ApplySectionStyle(s, i);

    if(WhenAdded)
        WhenAdded(i);

    Layout();
    Refresh();
    return i;
}

int UiAccordion::AddSection(const String& title, bool open)
{
    return AddSection(title, String(), String(), open);
}

void UiAccordion::ApplySectionStyle(Section& s, int)
{
    s.header.SetStyle(style_.header_style)
            .SetTitle(s.title)
            .SetSubTitle(s.subtitle)
            .SetCopyText(s.copy)
            .EnableHover(style_.header_style.hover_enabled)
            .EnableFocusRing(style_.header_style.show_focus)
            .SetSelectable(true);

    s.body.SetStyle(style_.body_style);
    RefreshChevron(s);
}

void UiAccordion::RefreshChevron(Section& s)
{
    if(!style_.show_chevron) {
        s.header.ClearMedia();
        return;
    }

    Image arrow;
    if(s.lock != Lock::None && !IsNull(style_.glyph_lock))
        arrow = style_.glyph_lock;
    else
        arrow = s.open ? style_.glyph_open : style_.glyph_closed;

    auto PadImage = [&](const Image& src, int pad) {
        if(IsNull(src) || pad <= 0)
            return src;
        Size isz = src.GetSize();
        if(isz.cx <= 0 || isz.cy <= 0)
            return src;
        ImageBuffer ib(isz.cx + pad * 2, isz.cy + pad * 2);
        ib.SetKind(IMAGE_ALPHA);
        Fill(~ib, RGBAZero(), ib.GetLength());
        Copy(ib, Point(pad, pad), src, isz);
        return Image(ib);
    };

    Size pref = Size(DPI(14), DPI(14));
    if(!IsNull(arrow)) {
        if(style_.chevron_scale) {
            int px = style_.chevron_size > 0 ? style_.chevron_size : DPI(14);
            pref = Size(px, px);
            if(px <= DPI(20))
                arrow = PadImage(arrow, 2);
        }
        else {
            pref = arrow.GetSize();
            if(pref.cx <= 0 || pref.cy <= 0)
                pref = Size(DPI(14), DPI(14));
        }
    }

    int reserve = max(DPI(20), pref.cx + DPI(10));

    s.header.SetMedia(arrow, pref)
            .SetMediaSide(style_.chevron_side)
            .SetMediaReserve(reserve)
            .SetMediaSharePercent(0)
            .SetMediaAlign(UiAlign::CENTER, UiAlign::CENTER);
}

void UiAccordion::Clear()
{
    StopAllAnimations();
    for(int i = 0; i < sections_.GetCount(); i++) {
        sections_[i].header.Remove();
        sections_[i].body.Remove();
    }
    sections_.Clear();
    RefreshLayout();
    Refresh();
}

void UiAccordion::Remove(int i)
{
    if(i < 0 || i >= sections_.GetCount())
        return;

    StopAllAnimations();
    sections_[i].header.Remove();
    sections_[i].body.Remove();
    sections_.Remove(i);
    ReindexSections();

    if(WhenRemoved)
        WhenRemoved(i);

    NormalizePolicyAfterBulkChange();
    RefreshLayout();
    Refresh();
}

ParentCtrl& UiAccordion::Content(int i)
{
    ASSERT(i >= 0 && i < sections_.GetCount());
    return sections_[i].content;
}

UiTitleCard& UiAccordion::Header(int i)
{
    ASSERT(i >= 0 && i < sections_.GetCount());
    return sections_[i].header;
}

UiPanel& UiAccordion::Body(int i)
{
    ASSERT(i >= 0 && i < sections_.GetCount());
    return sections_[i].body;
}

UiAccordion& UiAccordion::SetSectionText(int i, const String& title, const String& subtitle, const String& copy)
{
    if(i < 0 || i >= sections_.GetCount())
        return *this;

    Section& s = sections_[i];
    s.title = title;
    s.subtitle = subtitle;
    s.copy = copy;
    s.header.SetTitle(title).SetSubTitle(subtitle).SetCopyText(copy);
    Refresh();
    return *this;
}

UiAccordion& UiAccordion::SetSectionBodyHeight(int i, int h)
{
    if(i < 0 || i >= sections_.GetCount())
        return *this;
    sections_[i].body_height = h;
    RefreshLayout();
    Refresh();
    return *this;
}

UiAccordion& UiAccordion::Open(int i, bool on)
{
    if(i < 0 || i >= sections_.GetCount())
        return *this;

    Section& item = sections_[i];
    if(item.lock == Lock::Open && !on)
        return *this;
    if(item.lock == Lock::Closed && on)
        return *this;

    if(item.open == on)
        return *this;

    item.open = on;

    if(on && style_.single_open) {
        for(int k = 0; k < sections_.GetCount(); k++) {
            if(k == i)
                continue;
            if(sections_[k].open && sections_[k].lock != Lock::Open) {
                sections_[k].open = false;
                StartSectionAnimation(k, false);
                RefreshChevron(sections_[k]);
                if(WhenSectionToggled)
                    WhenSectionToggled(k, false);
            }
        }
    }

    if(!on && style_.enforce_one) {
        bool any = false;
        for(int k = 0; k < sections_.GetCount(); k++) {
            if(sections_[k].open) {
                any = true;
                break;
            }
        }
        if(!any)
            item.open = true;
    }

    StartSectionAnimation(i, item.open);

    RefreshChevron(item);
    if(WhenSectionToggled)
        WhenSectionToggled(i, item.open);

    RefreshLayout();
    Refresh();
    return *this;
}

UiAccordion& UiAccordion::Toggle(int i)
{
    if(i < 0 || i >= sections_.GetCount())
        return *this;
    return Open(i, !sections_[i].open);
}

bool UiAccordion::IsOpen(int i) const
{
    if(i < 0 || i >= sections_.GetCount())
        return false;
    return sections_[i].open;
}

UiAccordion& UiAccordion::OpenAll(bool on)
{
    StopAllAnimations();

    for(int i = 0; i < sections_.GetCount(); i++) {
        Section& s = sections_[i];
        if(on) {
            s.open = (s.lock != Lock::Closed);
        }
        else {
            s.open = (s.lock == Lock::Open);
        }

        StartSectionAnimation(i, s.open);
        RefreshChevron(sections_[i]);
        if(WhenSectionToggled)
            WhenSectionToggled(i, s.open);
    }

    NormalizePolicyAfterBulkChange();

    RefreshLayout();
    Refresh();
    return *this;
}

UiAccordion& UiAccordion::SetSingleOpen(bool on)
{
    style_.single_open = on;
    NormalizePolicyAfterBulkChange();
    RefreshLayout();
    Refresh();
    return *this;
}

UiAccordion& UiAccordion::SetEnforceOne(bool on)
{
    style_.enforce_one = on;
    NormalizePolicyAfterBulkChange();
    RefreshLayout();
    Refresh();
    return *this;
}

UiAccordion& UiAccordion::ShowChevron(bool on)
{
    style_.show_chevron = on;
    for(int i = 0; i < sections_.GetCount(); i++)
        RefreshChevron(sections_[i]);
    Refresh();
    return *this;
}

UiAccordion& UiAccordion::SetChevronSide(UiAlign side)
{
    if(side == UiAlign::LEFT || side == UiAlign::RIGHT)
        style_.chevron_side = side;
    for(int i = 0; i < sections_.GetCount(); i++)
        RefreshChevron(sections_[i]);
    Refresh();
    return *this;
}

UiAccordion& UiAccordion::SetChevronGlyphs(const Image& open, const Image& closed, const Image& lock)
{
    style_.glyph_open = open;
    style_.glyph_closed = closed;
    style_.glyph_lock = lock;
    for(int i = 0; i < sections_.GetCount(); i++)
        RefreshChevron(sections_[i]);
    Refresh();
    return *this;
}

UiAccordion& UiAccordion::SetHeaderRuleExtent(UiSpan ex)
{
    style_.header_style.rule_extent = ex;
    for(int i = 0; i < sections_.GetCount(); i++)
        sections_[i].header.SetRuleExtent(ex);
    RefreshLayout();
    Refresh();
    return *this;
}

UiAccordion& UiAccordion::SetBodyLine(UiSpan ex, int thickness, UiLineStyle style, Color c)
{
    style_.body_line_extent = ex;
    style_.body_line_thickness = max(1, thickness);
    style_.body_line_style = style;
    style_.body_line_color = c;
    Refresh();
    return *this;
}

UiAccordion& UiAccordion::SetLockMode(int i, Lock mode)
{
    if(i < 0 || i >= sections_.GetCount())
        return *this;

    Section& it = sections_[i];
    it.lock = mode;

    if(mode == Lock::Open)
        it.open = true;
    else if(mode == Lock::Closed)
        it.open = false;
    else if(style_.single_open && it.open) {
        for(int j = 0; j < sections_.GetCount(); j++) {
            if(j == i)
                continue;
            if(sections_[j].open && sections_[j].lock == Lock::None)
                sections_[j].open = false;
        }
    }

    NormalizePolicyAfterBulkChange();

    StartSectionAnimation(i, it.open);

    RefreshLayout();
    Refresh();
    return *this;
}

UiAccordion::Lock UiAccordion::GetLockMode(int i) const
{
    if(i < 0 || i >= sections_.GetCount())
        return Lock::None;
    return sections_[i].lock;
}

UiAccordion& UiAccordion::SetAnimation(bool enabled, int open_ms, int close_ms)
{
    style_.animation_enabled = enabled;
    style_.anim_open_ms = max(0, open_ms);
    style_.anim_close_ms = max(0, close_ms);

    if(!enabled)
        StopAllAnimations();

    RefreshLayout();
    Refresh();
    return *this;
}

UiAccordion& UiAccordion::EnableDragReorder(bool on)
{
    drag_reorder_enabled_ = on;
    if(!on)
        EndHeaderDrag(true);
    return *this;
}

bool UiAccordion::FocusHeader(int i)
{
    if(i < 0 || i >= sections_.GetCount())
        return false;
    sections_[i].header.SetFocus();
    return true;
}

void UiAccordion::BeginHeaderDrag(int i, Point start_screen)
{
    if(!drag_reorder_enabled_ || i < 0 || i >= sections_.GetCount() || sections_.GetCount() < 2) {
        drag_candidate_ = false;
        return;
    }

    StopAllAnimations();

    drag_candidate_ = true;
    dragging_ = false;
    drag_moved_ = false;
    drag_from_ = i;
    drag_insert_before_ = i;
    drag_start_screen_ = start_screen;
    drag_marker_.Hide();
}

void UiAccordion::ContinueHeaderDrag(Point p_screen)
{
    if(!drag_candidate_)
        return;

    if(!dragging_) {
        int dx = p_screen.x - drag_start_screen_.x;
        int dy = p_screen.y - drag_start_screen_.y;
        if(abs(dy) < drag_threshold_px_ || abs(dy) < abs(dx))
            return;
        dragging_ = true;
        drag_moved_ = true;
        drag_marker_.Show();
        drag_marker_.Remove();
        Add(drag_marker_);
    }

    Rect self = GetScreenRect();
    int y = p_screen.y - self.top;

    int before = sections_.GetCount();
    for(int i = 0; i < sections_.GetCount(); i++) {
        Rect hr = sections_[i].header.GetRect();
        int mid = hr.top + hr.GetHeight() / 2;
        if(y < mid) {
            before = i;
            break;
        }
    }
    drag_insert_before_ = before;
    RefreshLayout();
    Refresh();
}

void UiAccordion::EndHeaderDrag(bool cancel)
{
    if(!drag_candidate_) {
        dragging_ = false;
        drag_moved_ = false;
        return;
    }

    if(!cancel && dragging_ && drag_from_ >= 0)
        MoveSectionTo(drag_from_, drag_insert_before_);

    drag_candidate_ = false;
    dragging_ = false;
    drag_moved_ = false;
    drag_from_ = -1;
    drag_insert_before_ = -1;
    drag_marker_.Hide();
    Refresh();
}

void UiAccordion::MoveSectionTo(int from, int before)
{
    if(from < 0 || from >= sections_.GetCount())
        return;
    if(before < 0 || before > sections_.GetCount())
        return;
    if(before == from || before == from + 1)
        return;

    const int original_before = before;
    if(before < from) {
        for(int i = from; i > before; --i)
            sections_.Swap(i, i - 1);
    }
    else {
        for(int i = from; i < before - 1; ++i)
            sections_.Swap(i, i + 1);
    }

    ReindexSections();
    if(WhenReordered)
        WhenReordered(from, original_before);

    RefreshLayout();
    Refresh();
}

void UiAccordion::ReindexSections()
{
    for(int i = 0; i < sections_.GetCount(); i++) {
        sections_[i].header.owner = this;
        sections_[i].header.index = i;
    }
}

void UiAccordion::StopAllAnimations()
{
    KillTimeCallback(ANIM_CB_ID);
    for(int i = 0; i < sections_.GetCount(); i++) {
        Section& s = sections_[i];
        s.animating = false;
        s.current_body_cy = s.open ? max(style_.body_min_height, s.body_height) : 0;
        s.target_body_cy = s.current_body_cy;
        if(!s.open && s.current_body_cy == 0)
            s.body.Hide();
    }
}

void UiAccordion::StartSectionAnimation(int i, bool opening)
{
    if(i < 0 || i >= sections_.GetCount())
        return;

    Section& s = sections_[i];
    Rect outer = GetSize();
    Rect content = UiStyledInnerRect(outer, style_.metrics, style_.skin);
    int target = opening ? MeasureSectionBodyHeight(s, max(1, content.GetWidth())) : 0;

    s.target_body_cy = target;
    if(opening)
        s.body.Show();

    int dur = opening ? style_.anim_open_ms : style_.anim_close_ms;
    if(!style_.animation_enabled || dur <= 0) {
        s.current_body_cy = target;
        s.animating = false;
        if(!opening && target == 0)
            s.body.Hide();
        return;
    }

    s.anim_from_cy = s.current_body_cy;
    s.anim_start_ms = msecs();
    s.anim_ms = max(1, dur);
    s.animating = true;
    SetTimeCallback(16, THISBACK(AnimationStep), ANIM_CB_ID);
}

void UiAccordion::AnimationStep()
{
    bool any = false;
    int now = msecs();

    for(int i = 0; i < sections_.GetCount(); i++) {
        Section& s = sections_[i];
        if(!s.animating)
            continue;

        any = true;
        int elapsed = now - s.anim_start_ms;
        int dur = max(1, s.anim_ms);
        if(elapsed >= dur) {
            s.current_body_cy = s.target_body_cy;
            s.animating = false;
            if(!s.open && s.current_body_cy == 0)
                s.body.Hide();
            continue;
        }

        int delta = s.target_body_cy - s.anim_from_cy;
        s.current_body_cy = s.anim_from_cy + (delta * elapsed) / dur;
        if(!s.open)
            s.current_body_cy = max(0, s.current_body_cy);
    }

    RefreshLayout();
    Refresh();

    if(any)
        SetTimeCallback(16, THISBACK(AnimationStep), ANIM_CB_ID);
}

void UiAccordion::NormalizePolicyAfterBulkChange()
{
    if(style_.single_open) {
        int keep_unlocked = -1;
        for(int i = 0; i < sections_.GetCount(); i++) {
            if(sections_[i].open && sections_[i].lock == Lock::None) {
                keep_unlocked = i;
                break;
            }
        }
        for(int i = 0; i < sections_.GetCount(); i++) {
            if(i == keep_unlocked)
                continue;
            if(sections_[i].open && sections_[i].lock == Lock::None)
                sections_[i].open = false;
        }
    }

    if(style_.enforce_one) {
        bool any = false;
        for(int i = 0; i < sections_.GetCount(); i++)
            if(sections_[i].open) { any = true; break; }
        if(!any) {
            for(int i = 0; i < sections_.GetCount(); i++)
                if(sections_[i].lock != Lock::Closed) { sections_[i].open = true; break; }
        }
    }

    for(int i = 0; i < sections_.GetCount(); i++)
        RefreshChevron(sections_[i]);

    int body_w = max(1, GetSize().cx);
    bool any_anim = false;
    for(int i = 0; i < sections_.GetCount(); i++) {
        Section& s = sections_[i];
        int desired = s.open ? MeasureSectionBodyHeight(s, body_w) : 0;

        if(s.animating && s.target_body_cy != desired) {
            s.animating = false;
            s.current_body_cy = desired;
        }
        else if(!s.animating) {
            s.current_body_cy = desired;
        }

        s.target_body_cy = desired;
        if(desired > 0)
            s.body.Show();
        else
            s.body.Hide();

        any_anim = any_anim || s.animating;
    }

    if(!any_anim)
        KillTimeCallback(ANIM_CB_ID);
}

int UiAccordion::MeasureSectionBodyHeight(const Section& s, int width) const
{
    if(s.body_height >= 0)
        return s.body_height;

    Rect b(0, 0, 0, 0);
    bool first = true;
    for(Ctrl* q = s.content.GetFirstChild(); q; q = q->GetNext()) {
        if(!q->IsShown())
            continue;
        Rect r = q->GetRect();
        if(first) {
            b = r;
            first = false;
        }
        else
            b |= r;
    }

    int measured = 0;
    if(!first)
        measured = b.GetHeight();

    measured = max(measured, s.content.GetMinSize().cy);
    measured = max(measured, style_.body_min_height);

    Size outer = UiStyledOuterSizeFromContent(Size(max(0, width), measured),
                                              style_.body_style.metrics,
                                              style_.body_style.skin);
    return max(style_.body_min_height, outer.cy);
}

Size UiAccordion::GetMinSize() const
{
    int w = DPI(200);
    int h = 0;

    for(int i = 0; i < sections_.GetCount(); i++) {
        const Section& s = sections_[i];
        w = max(w, s.header.GetMinSize().cx);
        h += max(DPI(24), style_.header_height);
        if(s.open)
            h += style_.header_body_gap + max(style_.body_min_height, s.body_height);
        if(i + 1 < sections_.GetCount())
            h += style_.section_gap;
    }

    Size out = UiStyledOuterSizeFromContent(Size(w, h), style_.metrics, style_.skin);
    return out;
}

void UiAccordion::Layout()
{
    Rect outer = GetSize();
    Rect content = UiStyledInnerRect(outer, style_.metrics, style_.skin);
    if(content.IsEmpty())
        return;

    int y = content.top;
    int w = content.GetWidth();

    for(int i = 0; i < sections_.GetCount(); i++) {
        Section& s = sections_[i];

        int hh = max(DPI(24), style_.header_height);
        s.header.SetRect(content.left, y, w, hh);
        y += hh;

        if(s.open || s.animating || s.current_body_cy > 0) {
            y += style_.header_body_gap;
            int bh = s.animating ? s.current_body_cy : (s.open ? MeasureSectionBodyHeight(s, w) : 0);
            if(!s.animating)
                s.current_body_cy = bh;
            s.body.SetRect(content.left, y, w, bh);
            if(bh > 0)
                s.body.Show();
            else
                s.body.Hide();
            y += bh;
        }
        else {
            s.body.Hide();
            s.body.SetRect(content.left, y, w, 0);
        }

        if(i + 1 < sections_.GetCount())
            y += style_.section_gap;
    }

    if(dragging_ && drag_from_ >= 0 && drag_from_ < sections_.GetCount()) {
        int line_y = content.top;
        if(drag_insert_before_ >= 0 && drag_insert_before_ < sections_.GetCount())
            line_y = sections_[drag_insert_before_].header.GetRect().top;
        else if(sections_.GetCount() > 0) {
            const Section& last = sections_.Top();
            line_y = max(last.header.GetRect().bottom, last.body.GetRect().bottom);
        }

        int x = content.left + DPI(6);
        int cx = max(0, content.GetWidth() - DPI(12));
        int cy = max(DPI(2), style_.metrics.frame_width + 2);
        drag_marker_.SetRect(x, line_y - cy / 2, cx, cy);
        drag_marker_.Show();
    }
    else {
        drag_marker_.Hide();
    }
}

void UiAccordion::Paint(Draw& w)
{
    Rect outer = GetSize();
    if(outer.IsEmpty())
        return;

    StyledState st = IsEnabled() ? ST_NORMAL : ST_DISABLED;
    bool has_focus = HasFocus();

    bool bg_handled = false;
    bool fg_handled = false;

    if(WhenPaintBackground) {
        WhenPaintBackground(w, outer, style_.palette, style_.metrics, style_.skin, st, has_focus);
        bg_handled = true;
    }

    if(WhenPaintForeground) {
        WhenPaintForeground(w, outer, style_.palette, style_.metrics, style_.skin, st, has_focus);
        fg_handled = true;
    }

    UiPaintStyledSurface(w, outer, style_.palette, style_.metrics, style_.skin,
                         st, has_focus, bg_handled, fg_handled, style_.show_focus);

    if(style_.unified_section_frame) {
        StyledPalette p;
        StyledMetrics m;
        m.face_enabled = false;
        m.frame_enabled = true;
        m.frame_width = max(1, style_.unified_section_frame_width);
        m.radius = max(0, style_.unified_section_radius);
        for(int i = 0; i < 4; i++) {
            p.frame[i] = style_.palette.frame[i];
            p.face[i] = UiFill::Solid(Null);
        }

        for(int i = 0; i < sections_.GetCount(); i++) {
            Rect rr = sections_[i].header.GetRect();
            if(sections_[i].open)
                rr |= sections_[i].body.GetRect();
            UiPaintFaceFrameDash(w, rr, p, m, ST_NORMAL);
        }
    }

    for(int i = 0; i < sections_.GetCount(); i++) {
        const Section& s = sections_[i];

        if(style_.body_line_extent != NONE && s.body.IsShown() && s.body.GetRect().GetHeight() > 0) {
            Rect br = s.body.GetRect();
            int lw = ResolveLineWidth(style_.body_line_extent, max(0, br.GetWidth() - DPI(16)));
            if(lw > 0) {
                int x = br.left + (br.GetWidth() - lw) / 2;
                int y = br.bottom - max(1, style_.body_line_thickness);
                Color c = IsNull(style_.body_line_color)
                              ? Blend(style_.palette.frame[ST_NORMAL], style_.palette.ink[ST_NORMAL], 72)
                              : style_.body_line_color;
                PaintRuleLine(w, x, y, lw, style_.body_line_thickness, style_.body_line_style, c);
            }
        }
    }

    if(dragging_ && drag_from_ >= 0 && drag_from_ < sections_.GetCount()) {
        Color base_face = style_.palette.face[ST_NORMAL].IsSolid() ? style_.palette.face[ST_NORMAL].color : SColorFace();
        Rect hr = sections_[drag_from_].header.GetRect();
        if(!hr.IsEmpty()) {
            Color shade = Blend(base_face, style_.palette.ink[ST_NORMAL], 12);
            w.DrawRect(hr, shade);
            w.DrawRect(hr.left, hr.top, hr.GetWidth(), 1, Blend(style_.palette.ink[ST_NORMAL], White(), 20));
            w.DrawRect(hr.left, hr.bottom - 1, hr.GetWidth(), 1, Blend(style_.palette.ink[ST_NORMAL], Black(), 20));
        }

        int y = 0;
        if(drag_insert_before_ >= 0 && drag_insert_before_ < sections_.GetCount())
            y = sections_[drag_insert_before_].header.GetRect().top;
        else if(sections_.GetCount() > 0) {
            const Section& last = sections_.Top();
            y = max(last.header.GetRect().bottom, last.body.GetRect().bottom);
        }

        (void)y;
    }
}

void UiAccordion::SetData(const Value& v)
{
    auto ToBool = [](const Value& x) -> bool {
        if(IsNull(x))
            return false;
        if(x.Is<bool>())
            return (bool)x;
        if(x.Is<int>())
            return (int)x != 0;
        if(x.Is<int64>())
            return (int64)x != 0;
        if(x.Is<double>())
            return (double)x != 0.0;
        if(x.Is<String>()) {
            String s = ToLower((String)x);
            return s == "1" || s == "true" || s == "yes" || s == "on";
        }
        return true;
    };

    StopAllAnimations();

    if(v.Is<ValueArray>()) {
        const ValueArray& va = v;
        for(int i = 0; i < sections_.GetCount(); i++) {
            bool open = i < va.GetCount() ? ToBool(va[i]) : false;
            if(sections_[i].lock == Lock::Open)
                open = true;
            else if(sections_[i].lock == Lock::Closed)
                open = false;
            sections_[i].open = open;
        }
    }
    else if(sections_.GetCount() == 1) {
        bool open = ToBool(v);
        if(sections_[0].lock == Lock::Open)
            open = true;
        else if(sections_[0].lock == Lock::Closed)
            open = false;
        sections_[0].open = open;
    }

    NormalizePolicyAfterBulkChange();

    int body_w = max(1, GetSize().cx);
    for(int i = 0; i < sections_.GetCount(); i++) {
        Section& s = sections_[i];
        int desired = s.open ? MeasureSectionBodyHeight(s, body_w) : 0;
        s.animating = false;
        s.anim_from_cy = desired;
        s.target_body_cy = desired;
        s.current_body_cy = desired;
        if(desired > 0)
            s.body.Show();
        else
            s.body.Hide();
        RefreshChevron(s);
    }

    RefreshLayout();
    Refresh();
}

Value UiAccordion::GetData() const
{
    ValueArray out;
    for(int i = 0; i < sections_.GetCount(); i++)
        out.Add(sections_[i].open);
    return out;
}

int UiAccordion::ResolveLineWidth(UiSpan ex, int avail) const
{
    avail = max(0, avail);
    switch(ex) {
    case NONE:   return 0;
    case SMALL:  return min(avail, DPI(40));
    case MEDIUM: return min(avail, (avail * 60) / 100);
    case LARGE:
    default:                   return avail;
    }
}

void UiAccordion::PaintRuleLine(Draw& w, int x, int y, int cx, int thickness, UiLineStyle style, Color c) const
{
    cx = max(0, cx);
    thickness = max(1, thickness);
    if(cx <= 0)
        return;

    if(style == SOLID) {
        w.DrawRect(x, y, cx, thickness, c);
        return;
    }

    int seg = style == DASHED ? DPI(8) : DPI(2);
    int gap = style == DASHED ? DPI(5) : DPI(4);
    if(seg <= 0)
        seg = 1;
    if(gap < 0)
        gap = 0;

    int p = x;
    int end = x + cx;
    while(p < end) {
        int run = min(seg, end - p);
        w.DrawRect(p, y, run, thickness, c);
        p += seg + gap;
    }
}

}
