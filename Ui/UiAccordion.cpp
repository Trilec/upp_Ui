#include <Ui/UiAccordion.h>
#include <Ui/UiMeasure.h>
#include <Ui/UiTheme.h>
#include <Ui/UiScrollPanel.h>
#include <Ui/UiTree.h>
#include <Ui/UiLabel.h>
#include <Ui/UiPanel.h>

namespace Upp {

static void RefreshAccordionExtent(UiAccordion& acc)
{
    acc.RefreshLayout();
    acc.RefreshParentLayout();

    for(Ctrl* p = acc.GetParent(); p; p = p->GetParent()) {
        p->RefreshLayout();
        if(UiScrollPanel* sp = dynamic_cast<UiScrollPanel*>(p)) {
            sp->RefreshLayout();
            sp->Refresh();
        }
    }

    acc.Refresh();
}

static int MeasureAccordionChildHeight(Ctrl* q, int width)
{
    if(!q)
        return 0;

    width = max(0, width);

    UiLayoutMeasureResult measure = UiMeasureLayout(*q, {width > 0 ? width : -1});
    if(measure.width_dependent) {
        if(width > 0)
            return max(measure.min.cy, measure.measured.cy);
        return max(measure.min.cy, measure.preferred.cy);
    }
    if(UiTree* tree = dynamic_cast<UiTree*>(q))
        return max(tree->GetMinSize().cy, tree->GetContentSize().cy);
    if(UiLabel* label = dynamic_cast<UiLabel*>(q))
        return max(label->GetMinSize().cy, label->GetContentSize().cy);
    if(UiScrollPanel* sp = dynamic_cast<UiScrollPanel*>(q))
        return max(sp->GetMinSize().cy, sp->GetContentSize().cy);
    if(UiPanel* panel = dynamic_cast<UiPanel*>(q))
        return max(panel->GetMinSize().cy, panel->GetContentSize().cy);

    return q->GetMinSize().cy;
}

void UiAccordion::SectionHeader::LeftDown(Point p, dword keyflags)
{
    UiTitleCard::LeftDown(p, keyflags);
    down = true;
    down_on_drag = owner && index >= 0 && owner->CanBeginHeaderDrag(index, p);
    SetCapture();
    if(down_on_drag)
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
    if(owner && index >= 0 && down && !moved && bounds.Contains(p) && !down_on_drag)
        owner->Toggle(index);
    down = false;
    down_on_drag = false;
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

void UiAccordion::SectionHeader::Paint(Draw& w)
{
    UiTitleCard::Paint(w);
    if(!owner || index < 0)
        return;

    const UiAccordion::Style& style = owner->GetStyle();
    if(!owner->drag_reorder_enabled_ || !style.show_drag_handle || owner->GetCount() < 2)
        return;

    Rect dr = owner->GetHeaderDragRect(index);
    if(dr.IsEmpty())
        return;

    StyledState st = !IsEnabled() ? ST_DISABLED : (down ? ST_PRESSED : (HasMouse() ? ST_HOT : ST_NORMAL));
    Color ink = style.header_style.palette.icon[st];
    if(IsNull(ink))
        ink = style.header_style.palette.ink[st];
    if(IsNull(ink))
        ink = style.palette.ink[st];
    if(owner->dragging_ && owner->drag_from_ == index)
        ink = Color(56, 146, 255);

    UiPaintStyledIcon(w, dr,
                      IsNull(style.drag_glyph) ? ICON_DESIGN_DRAG_INDICATOR_48() : style.drag_glyph,
                      true, true, UiIconRenderMode::MonoTint, ink, IsEnabled());
}

const UiAccordion::Style& UiAccordion::StyleDefault()
{
    static Style s;
    ONCELOCK {
        Color face = Color(248, 250, 252);
        Color frame = Color(226, 232, 240);
        Color ink = Color(15, 23, 42);

        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(face);
            s.palette.frame[i] = frame;
            s.palette.ink[i] = ink;
        }
        s.palette.face[ST_HOT] = UiFill::Solid(Color(241, 245, 249));
        s.palette.face[ST_PRESSED] = UiFill::Solid(Color(226, 232, 240));
        s.palette.face[ST_DISABLED] = UiFill::Solid(Color(248, 250, 252));
        s.palette.ink[ST_DISABLED] = Color(148, 163, 184);

        s.metrics.radius = DPI(8);
        s.metrics.frame_width = DPI(1);
        s.metrics.frame_enabled = true;
        s.metrics.face_enabled = true;
        s.metrics.content_margin = Rect(DPI(6), DPI(6), DPI(6), DPI(6));

        s.header_style = UiTitleCard::StyleDefault();
        s.header_style.metrics.radius = DPI(8);
        s.header_style.metrics.content_margin = Rect(DPI(10), DPI(6), DPI(10), DPI(6));
        s.header_style.hover_enabled = false;
        s.header_style.metrics.focus_enabled = false;
        s.header_style.title_line = false;
        s.header_style.card_line = true;
        s.header_style.card_line_color = Color(215, 219, 226);
        s.header_style.media_tint_mono = true;
        s.header_style.title_font = SansSerifZ(11).Bold();
        s.header_style.subtitle_font = SansSerifZ(8);
        for(int i = 0; i < 4; i++) {
            s.header_style.palette.ink[i] = Color(0, 120, 212);
            s.header_style.palette.icon[i] = Color(0, 120, 212);
        }

        s.body_style = UiPanel::StyleDefault();
        s.body_style.transparent = true;
        s.body_style.metrics.face_enabled = false;
        s.body_style.metrics.frame_enabled = false;
        s.body_style.metrics.frame_width = 0;
        s.body_style.metrics.radius = 0;
        s.body_style.metrics.focus_enabled = false;
        s.body_style.metrics.content_margin = Rect(0, 0, 0, 0);
        s.body_style.metrics.shadow.enabled = false;

        s.chevron_side = UiAlign::RIGHT;
        s.glyph_open = CtrlsImg::DA();
        s.glyph_closed = CtrlsImg::RA();
        s.glyph_lock = Image();
        s.chevron_scale = false;
        s.chevron_size = 0;
        s.show_drag_handle = true;
        s.drag_side = UiAlign::RIGHT;
        s.drag_glyph = ICON_DESIGN_DRAG_INDICATOR_48();
        s.drag_size = DPI(14);
        s.drag_gap = DPI(8);

        s.unified_section_frame = false;
        s.unified_section_radius = DPI(8);
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
    SyncThemeStyle();
    Add(drag_marker_);
    drag_marker_.Color(Color(56, 146, 255)).IgnoreMouse().Hide();
}

UiAccordion::~UiAccordion()
{
    EndHeaderDrag(true);
    StopAllAnimations();
}

void UiAccordion::InvalidateStyleCache()
{
    theme_revision_ = 0;
}

UiAccordion::Style& UiAccordion::StyleEdit()
{
    if(!has_custom_style_) {
        style_ = GetEffectiveStyle();
        has_custom_style_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

void UiAccordion::SyncThemeStyle()
{
    if(has_custom_style_)
        return;
    const uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;

    Style resolved = StyleDefault();
    UiPanel::Style panel = UiTheme::ResolvePanel(UiPanelRole::Surface);
    resolved.palette = panel.palette;
    resolved.metrics.radius = max(DPI(8), panel.metrics.radius);
    resolved.transparent = true;
    resolved.metrics.frame_width = 0;
    resolved.metrics.frame_enabled = false;
    resolved.metrics.face_enabled = false;
    resolved.metrics.shadow.enabled = false;
    resolved.body_style = UiTheme::ResolvePanel(UiPanelRole::Surface);
    resolved.body_style.transparent = true;
    resolved.body_style.metrics.face_enabled = false;
    resolved.body_style.metrics.frame_enabled = false;
    resolved.body_style.metrics.frame_width = 0;
    resolved.body_style.metrics.radius = 0;
    resolved.body_style.metrics.focus_enabled = false;
    resolved.body_style.metrics.content_margin = Rect(0, 0, 0, 0);
    resolved.body_style.metrics.shadow.enabled = false;
    resolved.header_style = UiTheme::ResolveTitleCard(UiRole::Accent);
    resolved.header_style.metrics.content_margin = Rect(DPI(10), DPI(6), DPI(10), DPI(6));
    resolved.header_style.hover_enabled = false;
    resolved.header_style.metrics.focus_enabled = false;
    resolved.header_style.title_line = false;
    resolved.header_style.card_line = true;
    resolved.header_style.media_tint_mono = true;
    resolved.header_style.title_font = SansSerifZ(11).Bold();
    resolved.header_style.subtitle_font = SansSerifZ(8);
    style_ = resolved;
    theme_revision_ = revision;

    for(int i = 0; i < sections_.GetCount(); i++)
        ApplySectionStyle(sections_[i], i);
    RefreshLayout();
    Refresh();
}

const UiAccordion::Style& UiAccordion::GetEffectiveStyle() const
{
    const_cast<UiAccordion*>(this)->SyncThemeStyle();
    return style_;
}

UiAccordion& UiAccordion::SetCustomStyle(const Style& s)
{
    style_ = s;
    has_custom_style_ = true;
    OnStyleChanged();
    return *this;
}

UiAccordion& UiAccordion::ClearCustomStyle()
{
    if(!has_custom_style_)
        return *this;
    has_custom_style_ = false;
    style_ = StyleDefault();
    InvalidateStyleCache();
    OnStyleChanged();
    return *this;
}

void UiAccordion::OnStyleChanged()
{
    const Style& style = GetEffectiveStyle();
    if(style.transparent)
        Transparent();
    else
        BackPaint();

    for(int i = 0; i < sections_.GetCount(); i++)
        ApplySectionStyle(sections_[i], i);

    for(int i = 0; i < sections_.GetCount(); i++) {
        sections_[i].animating = false;
        int body_w = max(1, UiStyledInnerRect(GetSize(), style.metrics, style.skin).GetWidth());
        sections_[i].current_body_cy = sections_[i].open ? MeasureSectionBodyHeight(sections_[i], body_w) : 0;
        sections_[i].target_body_cy = sections_[i].current_body_cy;
    }

    RefreshAccordionExtent(*this);
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
    const Style& style = GetEffectiveStyle();
    UiTitleCard::Style hs = style.header_style;
    hs.media_gap = style.chevron_gap;
    if(drag_reorder_enabled_ && style.show_drag_handle && sections_.GetCount() > 1) {
        int reserve = GetDragReserve();
        if(style.drag_side == UiAlign::LEFT)
            hs.metrics.content_margin.left += reserve;
        else
            hs.metrics.content_margin.right += reserve;
    }
    s.header.SetCustomStyle(hs)
            .SetTitle(s.title)
            .SetSubTitle(s.subtitle)
            .SetCopyText(s.copy)
            .EnableHover(style.header_style.hover_enabled)
            .SetShowFocus(style.header_style.metrics.focus_enabled)
            .SetSelectable(true);

    s.body.SetCustomStyle(style.body_style);
    RefreshChevron(s);
}

void UiAccordion::RefreshChevron(Section& s)
{
    const Style& style = GetEffectiveStyle();
    if(!style.show_chevron) {
        s.header.ClearMedia();
        return;
    }

    Image arrow;
    if(s.lock != Lock::None && !IsNull(style.glyph_lock))
        arrow = style.glyph_lock;
    else
        arrow = s.open ? style.glyph_open : style.glyph_closed;

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
        if(style.chevron_scale) {
            int px = style.chevron_size > 0 ? style.chevron_size : DPI(14);
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

    int reserve = max(DPI(20), pref.cx + max(DPI(6), style.chevron_gap));
    if(drag_reorder_enabled_ && style.show_drag_handle && sections_.GetCount() > 1 && style.drag_side == style.chevron_side)
        reserve += GetDragReserve();

    s.header.SetMedia(arrow, pref)
            .SetMediaSide(style.chevron_side)
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
    RefreshAccordionExtent(*this);
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
    RefreshAccordionExtent(*this);
}

ParentCtrl& UiAccordion::GetSectionContent(int i)
{
    ASSERT(i >= 0 && i < sections_.GetCount());
    return sections_[i].content;
}

UiTitleCard& UiAccordion::GetSectionHeader(int i)
{
    ASSERT(i >= 0 && i < sections_.GetCount());
    return sections_[i].header;
}

UiPanel& UiAccordion::GetSectionBody(int i)
{
    ASSERT(i >= 0 && i < sections_.GetCount());
    return sections_[i].body;
}

Rect UiAccordion::GetSectionHeaderRect(int i) const
{
    return i >= 0 && i < sections_.GetCount() ? sections_[i].header.GetRect() : Rect();
}

Rect UiAccordion::GetSectionBodyRect(int i) const
{
    return i >= 0 && i < sections_.GetCount() ? sections_[i].body.GetRect() : Rect();
}

Rect UiAccordion::GetSectionContentRect(int i) const
{
    return GetSectionBodyRect(i);
}

int UiAccordion::GetSectionBodyHeight(int i) const
{
    return i >= 0 && i < sections_.GetCount() ? max(0, sections_[i].current_body_cy) : 0;
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
    h = max(0, h);
    Section& s = sections_[i];
    if(s.body_height == h) {
        if(s.open && s.current_body_cy != h) {
            s.animating = false;
            s.current_body_cy = h;
            s.target_body_cy = h;
            if(h > 0) {
                s.body.Show();
                s.content.Show();
            }
            RefreshAccordionExtent(*this);
        }
        return *this;
    }
    s.body_height = h;
    if(s.open) {
        s.animating = false;
        s.current_body_cy = h;
        s.target_body_cy = h;
        if(h > 0) {
            s.body.Show();
            s.content.Show();
        }
    }
    RefreshAccordionExtent(*this);
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

    const Style& style = GetEffectiveStyle();
    if(on && style.single_open) {
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

    if(!on && style.enforce_one) {
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
    RefreshAccordionExtent(*this);

    if(WhenSectionToggled)
        WhenSectionToggled(i, item.open);
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
    }

    NormalizePolicyAfterBulkChange();

    RefreshAccordionExtent(*this);

    for(int i = 0; i < sections_.GetCount(); i++) {
        if(WhenSectionToggled)
            WhenSectionToggled(i, sections_[i].open);
    }
    return *this;
}

UiAccordion& UiAccordion::SetSingleOpen(bool on)
{
    StyleEdit().single_open = on;
    NormalizePolicyAfterBulkChange();
    RefreshAccordionExtent(*this);
    return *this;
}

UiAccordion& UiAccordion::SetEnforceOne(bool on)
{
    StyleEdit().enforce_one = on;
    NormalizePolicyAfterBulkChange();
    RefreshAccordionExtent(*this);
    return *this;
}

UiAccordion& UiAccordion::ShowChevron(bool on)
{
    StyleEdit().show_chevron = on;
    for(int i = 0; i < sections_.GetCount(); i++)
        RefreshChevron(sections_[i]);
    Refresh();
    return *this;
}

UiAccordion& UiAccordion::SetChevronSide(UiAlign side)
{
    if(side == UiAlign::LEFT || side == UiAlign::RIGHT)
        StyleEdit().chevron_side = side;
    for(int i = 0; i < sections_.GetCount(); i++)
        RefreshChevron(sections_[i]);
    Refresh();
    return *this;
}

UiAccordion& UiAccordion::SetChevronGlyphs(const Image& open, const Image& closed, const Image& lock)
{
    StyleEdit().glyph_open = open;
    StyleEdit().glyph_closed = closed;
    StyleEdit().glyph_lock = lock;
    for(int i = 0; i < sections_.GetCount(); i++)
        RefreshChevron(sections_[i]);
    Refresh();
    return *this;
}


UiAccordion& UiAccordion::SetChevronSize(int px)
{
    StyleEdit().chevron_scale = true;
    StyleEdit().chevron_size = max(0, px);
    for(int i = 0; i < sections_.GetCount(); i++)
        RefreshChevron(sections_[i]);
    Refresh();
    return *this;
}

UiAccordion& UiAccordion::SetChevronGap(int px)
{
    StyleEdit().chevron_gap = max(0, px);
    for(int i = 0; i < sections_.GetCount(); i++)
        RefreshChevron(sections_[i]);
    Refresh();
    return *this;
}
UiAccordion& UiAccordion::SetHeaderTitleLineLength(UiSpan ex)
{
    StyleEdit().header_style.title_line_length = ex;
    for(int i = 0; i < sections_.GetCount(); i++)
        sections_[i].header.SetTitleLine(ex,
                                         StyleEdit().header_style.title_line_thickness,
                                         StyleEdit().header_style.title_line_style,
                                         StyleEdit().header_style.title_line_color);
    RefreshAccordionExtent(*this);
    return *this;
}

UiAccordion& UiAccordion::SetBodyLine(UiSpan ex, int thickness, UiLineStyle style, Color c)
{
    StyleEdit().body_line_extent = ex;
    StyleEdit().body_line_thickness = max(1, thickness);
    StyleEdit().body_line_style = style;
    StyleEdit().body_line_color = c;
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
    else if(GetEffectiveStyle().single_open && it.open) {
        for(int j = 0; j < sections_.GetCount(); j++) {
            if(j == i)
                continue;
            if(sections_[j].open && sections_[j].lock == Lock::None)
                sections_[j].open = false;
        }
    }

    NormalizePolicyAfterBulkChange();

    StartSectionAnimation(i, it.open);
    RefreshAccordionExtent(*this);
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
    StyleEdit().animation_enabled = enabled;
    StyleEdit().anim_open_ms = max(0, open_ms);
    StyleEdit().anim_close_ms = max(0, close_ms);

    if(!enabled)
        StopAllAnimations();

    RefreshAccordionExtent(*this);
    return *this;
}

UiAccordion& UiAccordion::EnableDragReorder(bool on)
{
    drag_reorder_enabled_ = on;
    if(!on)
        EndHeaderDrag(true);
    OnStyleChanged();
    return *this;
}

UiAccordion& UiAccordion::ShowDragHandle(bool on)
{
    StyleEdit().show_drag_handle = on;
    OnStyleChanged();
    return *this;
}

UiAccordion& UiAccordion::SetDragSide(UiAlign side)
{
    if(side != UiAlign::LEFT && side != UiAlign::RIGHT)
        side = UiAlign::RIGHT;
    StyleEdit().drag_side = side;
    OnStyleChanged();
    return *this;
}

UiAccordion& UiAccordion::SetDragGlyph(const Image& glyph)
{
    StyleEdit().drag_glyph = glyph;
    Refresh();
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
    RefreshAccordionExtent(*this);
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


int UiAccordion::GetChevronReserve(const Section& s) const
{
    const Style& style = GetEffectiveStyle();
    if(!style.show_chevron)
        return 0;

    Size pref = Size(DPI(14), DPI(14));
    if(style.chevron_scale) {
        int px = style.chevron_size > 0 ? style.chevron_size : DPI(14);
        pref = Size(px, px);
    }
    else {
        Image arrow;
        if(s.lock != Lock::None && !IsNull(style.glyph_lock))
            arrow = style.glyph_lock;
        else
            arrow = s.open ? style.glyph_open : style.glyph_closed;
        if(!IsNull(arrow) && arrow.GetSize().cx > 0 && arrow.GetSize().cy > 0)
            pref = arrow.GetSize();
    }
    return max(DPI(20), pref.cx + max(DPI(6), style.chevron_gap));
}

int UiAccordion::GetDragReserve() const
{
    const Style& style = GetEffectiveStyle();
    int size = max(DPI(10), style.drag_size);
    return size + max(0, style.drag_gap);
}

Rect UiAccordion::GetHeaderDragRect(int i) const
{
    if(i < 0 || i >= sections_.GetCount())
        return Rect(0, 0, 0, 0);

    const Style& style = GetEffectiveStyle();
    if(!drag_reorder_enabled_ || !style.show_drag_handle || sections_.GetCount() < 2)
        return Rect(0, 0, 0, 0);

    const Section& s = sections_[i];
    Size sz = s.header.GetSize();
    if(sz.cx <= 0 || sz.cy <= 0)
        return Rect(0, 0, 0, 0);

    Rect hr(Point(0, 0), sz);
    int side = min(style.drag_size, max(0, hr.GetHeight() - DPI(8)));
    side = max(DPI(10), side);
    int y = hr.top + (hr.GetHeight() - side) / 2;

    Rect cm = style.header_style.metrics.content_margin;
    if(style.drag_side == UiAlign::LEFT) {
        int x = hr.left + cm.left;
        return RectC(x, y, side, side);
    }
    else {
        int x = hr.right - cm.right - side;
        return RectC(x, y, side, side);
    }
}

bool UiAccordion::CanBeginHeaderDrag(int i, Point p_local) const
{
    if(!drag_reorder_enabled_ || i < 0 || i >= sections_.GetCount() || sections_.GetCount() < 2)
        return false;
    const Style& style = GetEffectiveStyle();
    if(!style.show_drag_handle)
        return true;
    return GetHeaderDragRect(i).Contains(p_local);
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

    RefreshAccordionExtent(*this);
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
        int body_w = max(1, UiStyledInnerRect(GetSize(), GetEffectiveStyle().metrics, GetEffectiveStyle().skin).GetWidth());
        s.current_body_cy = s.open ? MeasureSectionBodyHeight(s, body_w) : 0;
        s.target_body_cy = s.current_body_cy;
        if(!s.open && s.current_body_cy == 0)
            s.body.Hide();
    }
}

void UiAccordion::StartSectionAnimation(int i, bool opening)
{
    if(i < 0 || i >= sections_.GetCount())
        return;
    const Style& style = GetEffectiveStyle();

    Section& s = sections_[i];
    Rect outer = GetSize();
    Rect content = UiStyledInnerRect(outer, style.metrics, style.skin);
    int target = opening ? MeasureSectionBodyHeight(s, max(1, content.GetWidth())) : 0;

    s.target_body_cy = target;
    if(opening) {
        s.body.Show();
        s.content.Show();
    }

    int dur = opening ? style.anim_open_ms : style.anim_close_ms;
    if(!style.animation_enabled || dur <= 0) {
        s.current_body_cy = target;
        s.animating = false;
        if(!opening && target == 0) {
            s.content.Hide();
            s.body.Hide();
        }
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
              if(!s.open && s.current_body_cy == 0) {
                  s.content.Hide();
                  s.body.Hide();
              }
              continue;
        }

        int delta = s.target_body_cy - s.anim_from_cy;
        s.current_body_cy = s.anim_from_cy + (delta * elapsed) / dur;
        if(!s.open)
            s.current_body_cy = max(0, s.current_body_cy);
    }

    RefreshAccordionExtent(*this);

    if(any)
        SetTimeCallback(16, THISBACK(AnimationStep), ANIM_CB_ID);
}

void UiAccordion::NormalizePolicyAfterBulkChange()
{
    const Style& style = GetEffectiveStyle();
    if(style.single_open) {
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

    if(style.enforce_one) {
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
        if(desired > 0) {
            s.body.Show();
            s.content.Show();
        }
        else {
            s.content.Hide();
            s.body.Hide();
        }

        any_anim = any_anim || s.animating;
    }

    if(!any_anim)
        KillTimeCallback(ANIM_CB_ID);
}

int UiAccordion::MeasureSectionBodyHeight(const Section& s, int width) const
{
    const Style& style = GetEffectiveStyle();
    if(s.body_height >= 0)
        return s.body_height;

    Rect b(0, 0, 0, 0);
    bool first = true;
    int measured = 0;
    bool has_content = false;
    for(Ctrl* q = s.content.GetFirstChild(); q; q = q->GetNext()) {
        has_content = true;
        Size ms = q->GetMinSize();
        int body_h = MeasureAccordionChildHeight(q, width);
        measured = max(measured, max(ms.cy, body_h));

        Rect r = q->GetRect();
        bool fill_host = q->GetFirstChild()
                      && r.left == 0
                      && r.top == 0
                      && abs(r.GetWidth() - max(0, width)) <= 1;
        if(r.GetWidth() <= 0 || r.GetHeight() <= 0 || fill_host)
            r = RectC(0, 0, max(0, width), max(0, max(ms.cy, body_h)));
        else
            r.bottom = max(r.bottom, r.top + max(ms.cy, body_h));

        if(first) {
            b = r;
            first = false;
        }
        else {
            b |= r;
        }
    }

    if(!first)
        measured = max(measured, b.GetHeight());

    if(!has_content)
        return 0;
    measured = max(measured, style.body_min_height);

    Size outer = UiStyledOuterSizeFromContent(Size(max(0, width), measured),
                                              GetEffectiveStyle().body_style.metrics,
                                              GetEffectiveStyle().body_style.skin);
    return max(GetEffectiveStyle().body_min_height, outer.cy);
}

Size UiAccordion::GetMinSize() const
{
    const Style& style = GetEffectiveStyle();
    int w = 0;
    int h = 0;

    for(int i = 0; i < sections_.GetCount(); i++) {
        const Section& s = sections_[i];
        w = max(w, s.header.GetMinSize().cx);
        h += max(max(DPI(24), style.header_height), s.header.GetMinSize().cy);
        const int body = s.open ? MeasureSectionBodyHeight(s, max(1, w)) : 0;
        if(body > 0)
            h += style.header_body_gap + body;
        if(i + 1 < sections_.GetCount())
            h += style.item_spacing;
    }

    Size out = UiStyledOuterSizeFromContent(Size(w, h), style.metrics, style.skin);
    return out;
}

void UiAccordion::Layout()
{
    const Style& style = GetEffectiveStyle();
    Rect outer = GetSize();
    Rect content = UiStyledInnerRect(outer, style.metrics, style.skin);
    if(content.IsEmpty())
        return;

    const int w = content.GetWidth();
    const int count = sections_.GetCount();
    Vector<int> header_height, body_height, preferred;
    header_height.SetCount(count, 0);
    body_height.SetCount(count, 0);
    preferred.SetCount(count, 0);
    int remaining = max(0, content.GetHeight() - max(0, count - 1) * style.item_spacing);
    for(int i = 0; i < count; i++) {
        Section& s = sections_[i];
        const int desired = max(max(DPI(24), style.header_height), s.header.GetMinSize().cy);
        header_height[i] = min(desired, remaining);
        remaining -= header_height[i];
    }
    int open_gaps = 0;
    int preferred_total = 0;
    int last_preferred = -1;
    for(int i = 0; i < count; i++) {
        Section& s = sections_[i];
        if(!s.open && !s.animating && s.current_body_cy <= 0)
            continue;
        preferred[i] = max(0, MeasureSectionBodyHeight(s, w));
        if(preferred[i] > 0) {
            open_gaps++;
            preferred_total += preferred[i];
            last_preferred = i;
        }
    }
    remaining = max(0, remaining - open_gaps * max(0, style.header_body_gap));
    int body_remaining = remaining;
    if(preferred_total > 0) {
        int assigned = 0;
        for(int i = 0; i < count; i++) {
            if(preferred[i] <= 0)
                continue;
            const int allocation = i == last_preferred
                ? max(0, body_remaining - assigned)
                : min(preferred[i], (int)((int64)body_remaining * preferred[i] / preferred_total));
            body_height[i] = min(preferred[i], max(0, allocation));
            assigned += body_height[i];
        }
    }

    int y = content.top;
    for(int i = 0; i < count; i++) {
        Section& s = sections_[i];
        const int hh = min(header_height[i], max(0, content.bottom - y));
        s.header.SetRect(content.left, y, w, hh);
        y += hh;

        const int allocated_body = body_height[i];
        if(preferred[i] > 0 || s.animating) {
            y += style.header_body_gap;
            y = min(y, content.bottom);
            int bh = s.animating ? min(max(0, s.current_body_cy), allocated_body) : allocated_body;
            bh = min(bh, max(0, content.bottom - y));
            if(!s.animating)
                s.current_body_cy = bh;
            s.body.SetRect(content.left, y, w, bh);
            if(bh > 0) {
                s.body.Show();
                s.content.Show();
                s.content.SetRect(0, 0, s.body.GetSize().cx, s.body.GetSize().cy);
                s.content.Layout();
            }
            else {
                s.content.Hide();
                s.body.Hide();
            }
            y += bh;
        }
        else {
            s.content.Hide();
            s.body.Hide();
            s.body.SetRect(content.left, y, w, 0);
        }

        if(i + 1 < count)
            y += style.item_spacing;
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
        int cy = max(DPI(2), style.metrics.frame_width + 2);
        drag_marker_.SetRect(x, line_y - cy / 2, cx, cy);
        drag_marker_.Show();
    }
    else {
        drag_marker_.Hide();
    }
}

void UiAccordion::Paint(Draw& w)
{
    const Style& style = GetEffectiveStyle();
    Rect outer = GetSize();
    if(outer.IsEmpty())
        return;

    StyledState st = IsEnabled() ? ST_NORMAL : ST_DISABLED;
    bool has_focus = HasFocus();

    bool bg_handled = false;
    bool fg_handled = false;

    if(WhenPaintBackground) {
        WhenPaintBackground(w, outer, style.palette, style.metrics, style.skin, st, has_focus);
        bg_handled = true;
    }

    if(WhenPaintForeground) {
        WhenPaintForeground(w, outer, style.palette, style.metrics, style.skin, st, has_focus);
        fg_handled = true;
    }

    UiPaintStyledSurface(w, outer, style.palette, style.metrics, style.skin,
                         st, has_focus, bg_handled, fg_handled);

    if(style.unified_section_frame) {
        StyledPalette p;
        StyledMetrics m;
        m.face_enabled = false;
        m.frame_enabled = true;
        m.frame_width = max(1, style.unified_section_frame_width);
        m.radius = max(0, style.unified_section_radius);
        for(int i = 0; i < 4; i++) {
            p.frame[i] = style.palette.frame[i];
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

        if(style.body_line_extent != NONE && s.body.IsShown() && s.body.GetRect().GetHeight() > 0) {
            Rect br = s.body.GetRect();
            int lw = ResolveLineWidth(style.body_line_extent, max(0, br.GetWidth() - DPI(16)));
            if(lw > 0) {
                int x = br.left + (br.GetWidth() - lw) / 2;
                int y = br.bottom - max(1, style.body_line_thickness);
                Color c = IsNull(style.body_line_color)
                              ? Blend(style.palette.frame[ST_NORMAL], style.palette.ink[ST_NORMAL], 72)
                              : style.body_line_color;
                PaintRuleLine(w, x, y, lw, style.body_line_thickness, style.body_line_style, c);
            }
        }
    }

    if(dragging_ && drag_from_ >= 0 && drag_from_ < sections_.GetCount()) {
        Color base_face = style.palette.face[ST_NORMAL].IsSolid() ? style.palette.face[ST_NORMAL].color : SColorFace();
        Rect hr = sections_[drag_from_].header.GetRect();
        if(!hr.IsEmpty()) {
            Color shade = Blend(base_face, style.palette.ink[ST_NORMAL], 12);
            w.DrawRect(hr, shade);
            w.DrawRect(hr.left, hr.top, hr.GetWidth(), 1, Blend(style.palette.ink[ST_NORMAL], White(), 20));
            w.DrawRect(hr.left, hr.bottom - 1, hr.GetWidth(), 1, Blend(style.palette.ink[ST_NORMAL], Black(), 20));
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

    RefreshAccordionExtent(*this);
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





