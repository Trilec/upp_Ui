#include <Ui/UiLabel.h>

namespace Upp {

// ============================================================================
// Styles
// ============================================================================

const UiLabel::Style& UiLabel::StyleDefault()
{
    static Style s;
    ONCELOCK {
        Color face  = SColorFace();
        Color text  = SColorText();
        Color frame = SColorShadow();
        Color dis   = SColorDisabled();
        
        for(int i = 0; i < 4; i++) {
            s.palette.face[i]  = face;
            s.palette.frame[i] = frame;
            s.palette.ink[i]   = text;
        }
        s.palette.ink[ST_DISABLED] = dis;

        s.metrics.radius        = 0;
        s.metrics.frame_width   = 0;
        s.metrics.frame_enabled = false;
        s.metrics.face_enabled  = false;
        s.metrics.dashed        = false;

        s.metrics.text_font     = StdFont();
        s.metrics.use_text_font = false;

        s.skin.base           = Image();
        s.skin.slice          = Rect(0, 0, 0, 0);
        s.skin.content_inset  = Rect(0, 0, 0, 0);
        s.skin.enabled        = false;

        s.align_h     = UiAlign::CENTER;
        s.align_v     = UiAlign::CENTER;
        s.icon_layout = UiAlign::LEFT;

        s.icon_margin = Rect(DPI(2), 0, DPI(4), 0);
        s.text_margin = Rect(0, 0, 0, 0);

        s.font        = StdFont();
        s.transparent = true;

        s.underline        = false;
        s.underline_width  = DPI(1);
        s.underline_offset = 0;

        s.nowrap = false;
    }
    return s;
}

const UiLabel::Style& UiLabel::StyleHeadline()
{
    static Style s;
    ONCELOCK {
        s = StyleDefault();
        s.font                  = SansSerifZ(24).Bold();
        s.metrics.text_font     = s.font;
        s.metrics.use_text_font = true;
        s.align_h               = UiAlign::LEFT;
        s.align_v               = UiAlign::TOP;
        s.text_margin           = Rect(0, 0, 0, DPI(4));
    }
    return s;
}

const UiLabel::Style& UiLabel::StyleSubheadline()
{
    static Style s;
    ONCELOCK {
        s = StyleDefault();
        s.font                  = SansSerifZ(18).Bold();
        s.metrics.text_font     = s.font;
        s.metrics.use_text_font = true;
        s.align_h               = UiAlign::LEFT;
        s.align_v               = UiAlign::CENTER;
        s.text_margin           = Rect(0, 0, 0, DPI(2));
    }
    return s;
}

const UiLabel::Style& UiLabel::StyleTitle()
{
    static Style s;
    ONCELOCK {
        s = StyleDefault();
        s.font                  = SansSerifZ(16).Bold();
        s.metrics.text_font     = s.font;
        s.metrics.use_text_font = true;
        s.align_h               = UiAlign::LEFT;
        s.align_v               = UiAlign::CENTER;
        s.transparent           = true;
    }
    return s;
}

const UiLabel::Style& UiLabel::StyleCaption()
{
    static Style s;
    ONCELOCK {
        s = StyleDefault();

        s.font                  = SansSerifZ(11);
        s.metrics.text_font     = s.font;
        s.metrics.use_text_font = true;

        Color base = SColorText();
        Color cap  = LtColor(base, 10);
        for(int i = 0; i < 4; i++)
            s.palette.ink[i] = cap;
        s.palette.ink[ST_DISABLED] = SColorDisabled();

        s.align_h     = UiAlign::LEFT;
        s.align_v     = UiAlign::CENTER;

        s.text_margin = Rect(0, DPI(1), 0, DPI(1));
    }
    return s;
}

const UiLabel::Style& UiLabel::StyleFootnote()
{
    static Style s;
    ONCELOCK {
        s = StyleDefault();

        s.font                  = SansSerifZ(DPI(9));
        s.metrics.text_font     = s.font;
        s.metrics.use_text_font = true;

        Color muted = Blend(SColorText(), SColorPaper(), 65);
        for(int i = 0; i < 4; i++)
            s.palette.ink[i] = muted;

        s.text_margin = Rect(0, DPI(2), 0, DPI(4));
        s.align_v     = UiAlign::TOP;
    }
    return s;
}

const UiLabel::Style& UiLabel::StyleBadge()
{
    static Style s;
    ONCELOCK {
        s = StyleDefault();

        Color face  = SColorHighlight();
        Color frame = DkColor(face, 20);
        Color ink   = SColorHighlightText();

        for(int i = 0; i < 4; i++) {
            s.palette.face[i]  = UiFill::Solid(face);
            s.palette.frame[i] = frame;
            s.palette.ink[i]   = ink;
        }
        s.palette.ink[ST_DISABLED] = DisabledColor(ink);

        s.metrics.face_enabled  = true;
        s.metrics.frame_enabled = true;
        s.metrics.frame_width   = DPI(1);
        s.metrics.radius        = DPI(999);

        s.align_h     = UiAlign::CENTER;
        s.align_v     = UiAlign::CENTER;
        s.icon_layout = UiAlign::LEFT;

        s.icon_margin = Rect(DPI(6), DPI(2), DPI(4), DPI(2));
        s.text_margin = Rect(DPI(8), DPI(2), DPI(8), DPI(2));

        s.transparent = false;
    }
    return s;
}

// ============================================================================
// Construction & styling
// ============================================================================

UiLabel::UiLabel()
    : style_(StyleDefault())
{
    Transparent();
    NoWantFocus();

    RebuildTextLines();
    minsize_dirty_ = true;
    layout_dirty_  = true;
    layout_content_ = Rect(0, 0, 0, 0);
}

UiLabel& UiLabel::SetStyle(const Style& s)
{
    style_ = s;
    OnStyleChanged();
    return *this;
}

void UiLabel::OnStyleChanged()
{
    if(style_.transparent)
        Transparent();
    else
        BackPaint();

    RebuildTextLines();

    minsize_dirty_ = true;
    layout_dirty_  = true;

    RefreshLayout();
    Refresh();
}

// ============================================================================
// Internal helpers (multiline text & layout)
// ============================================================================

void UiLabel::RebuildTextLines()
{
    lines_.Clear();
    line_sizes_.Clear();

    if(text_.IsEmpty())
        return;

    Font fnt = style_.metrics.use_text_font ? style_.metrics.text_font : style_.font;
    if(IsNull(fnt))
        fnt = StdFont();

    if(style_.nowrap) {
        String one = text_;
        one.Replace("\r", "");
        one.Replace("\n", " ");

        lines_.Add(one);
        line_sizes_.Add(one.IsEmpty() ? Size(0, GetTextSize(" ", fnt).cy) : GetTextSize(one, fnt));
        return;
    }

    UiBuildStyledTextLines(text_, fnt, lines_, line_sizes_);
}

Size UiLabel::GetTextBlockSize() const
{
    return UiMeasureStyledTextBlock(line_sizes_);
}

Size UiLabel::ComputeNaturalSize() const
{
    bool have_text = !lines_.IsEmpty();
    Size text_block = have_text ? GetTextBlockSize() : Size(0, 0);

    bool have_icon = !IsNull(icon_);
    Size icon_sz   = have_icon ? icon_.GetSize() : Size(0, 0);

    Size content = UiMeasureBlocksContent(icon_sz,
                                          text_block,
                                          style_.icon_margin,
                                          style_.text_margin,
                                          style_.icon_layout,
                                          have_icon,
                                          have_text,
                                          DPI(16),
                                          DPI(8),
                                          DPI(16));

    return UiStyledOuterSizeFromContent(content, style_.metrics, style_.skin);
}

void UiLabel::UpdateLayout(const Rect& content) const
{
    layout_ = UiBlocksLayout();

    if(content.IsEmpty()) {
        layout_dirty_ = false;
        return;
    }

    bool have_text = !lines_.IsEmpty();
    Size text_block = have_text ? GetTextBlockSize() : Size(0, 0);

    bool have_icon = !IsNull(icon_);
    Size icon_sz   = have_icon ? icon_.GetSize() : Size(0, 0);

    layout_ = UiComputeBlocksLayout(content,
                                   have_icon ? icon_sz : Size(0, 0),
                                   have_text ? text_block : Size(0, 0),
                                   style_.align_h,
                                   style_.align_v,
                                   style_.icon_layout,
                                   style_.icon_margin,
                                   style_.text_margin,
                                   DPI(16));

    layout_dirty_ = false;
}

// ============================================================================
// Content API
// ============================================================================

UiLabel& UiLabel::SetText(const String& text)
{
    text_.Clear();
    accesskey_ = 0;
    has_access_mnemonic_ = false;

    String raw;
    raw.Reserve(text.GetCount());

    for(int i = 0; i < text.GetCount(); i++) {
        int c = text[i];

        if(c == '&' && i + 1 < text.GetCount()) {
            int n = text[i + 1];

            if(n == '&') {
                // Escaped literal '&'
                raw.Cat('&');
                i++;
            }
            else {
                // Explicit mnemonic '&X'
                if(!has_access_mnemonic_) {
                    accesskey_ = ToUpper((wchar)n);
                    has_access_mnemonic_ = true;
                }
                raw.Cat(n);
                i++;
            }
        }
        else {
            raw.Cat(c);
        }
    }

    text_.Reserve(raw.GetCount());
    for(int i = 0; i < raw.GetCount(); i++) {
        int c = raw[i];
        if(c == '\t') {
            const int TAB_SPACES = 4;
            for(int k = 0; k < TAB_SPACES; k++)
                text_.Cat(' ');
        }
        else {
            text_.Cat(c);
        }
    }

    RebuildTextLines();
    minsize_dirty_ = true;
    layout_dirty_  = true;

    RefreshLayout();
    Refresh();
    return *this;
}

UiLabel& UiLabel::SetIcon(const Image& img)
{
    icon_          = img;
    mono_icon_     = false;
    minsize_dirty_ = true;
    layout_dirty_  = true;

    RefreshLayout();
    Refresh();
    return *this;
}

UiLabel& UiLabel::SetMonoIcon(const Image& img)
{
    icon_          = img;
    mono_icon_     = true;
    minsize_dirty_ = true;
    layout_dirty_  = true;

    RefreshLayout();
    Refresh();
    return *this;
}

UiLabel& UiLabel::ClearIcon()
{
    icon_          = Image();
    mono_icon_     = false;
    minsize_dirty_ = true;
    layout_dirty_  = true;

    RefreshLayout();
    Refresh();
    return *this;
}

// ============================================================================
// Layout / Alignment API
// ============================================================================

UiLabel& UiLabel::SetIconLayout(UiAlign where)
{
    style_.icon_layout = where;
    minsize_dirty_     = true;
    layout_dirty_      = true;

    RefreshLayout();
    Refresh();
    return *this;
}

UiLabel& UiLabel::SetAlign(UiAlign h, UiAlign v)
{
    style_.align_h = h;
    style_.align_v = v;
    layout_dirty_  = true;

    RefreshLayout();
    Refresh();
    return *this;
}

UiLabel& UiLabel::SetAlignH(UiAlign h)
{
    style_.align_h = h;
    layout_dirty_  = true;

    RefreshLayout();
    Refresh();
    return *this;
}

UiLabel& UiLabel::SetAlignV(UiAlign v)
{
    style_.align_v = v;
    layout_dirty_  = true;

    RefreshLayout();
    Refresh();
    return *this;
}

UiLabel& UiLabel::SetIconMargin(const Rect& m)
{
    style_.icon_margin = m;
    minsize_dirty_     = true;
    layout_dirty_      = true;

    RefreshLayout();
    Refresh();
    return *this;
}

UiLabel& UiLabel::SetTextMargin(const Rect& m)
{
    style_.text_margin = m;
    minsize_dirty_     = true;
    layout_dirty_      = true;

    RefreshLayout();
    Refresh();
    return *this;
}

UiLabel& UiLabel::SetUnderline(bool on, int thickness, int offset)
{
    style_.underline        = on;
    style_.underline_width  = max(thickness, 0);
    style_.underline_offset = offset;
    Refresh();
    return *this;
}

// ============================================================================
// Layout / Sizing
// ============================================================================

Size UiLabel::GetMinSize() const
{
    if(!minsize_dirty_)
        return cached_minsize_;

    Size natural = ComputeNaturalSize();

    int w = natural.cx;
    int h = natural.cy;

    if(user_min_size_.cx > 0)
        w = max(w, user_min_size_.cx);
    if(user_min_size_.cy > 0)
        h = max(h, user_min_size_.cy);

    cached_minsize_ = Size(w, h);
    minsize_dirty_  = false;
    return cached_minsize_;
}

void UiLabel::Layout()
{
    Rect outer = GetSize();
    Rect content = UiStyledInnerRect(outer, style_.metrics, style_.skin);

    if(!layout_dirty_ && content == layout_content_)
        return;

    layout_content_ = content;
    UpdateLayout(content);
}

UiLabel& UiLabel::SetSizeMin(Size sz)
{
    user_min_size_ = sz;
    Ctrl::SetMinSize(sz);

    minsize_dirty_ = true;
    layout_dirty_  = true;

    RefreshLayout();
    return *this;
}

// ============================================================================
// Paint
// ============================================================================

void UiLabel::Paint(Draw& w)
{
    Rect outer = GetSize();
    if(outer.IsEmpty())
        return;

    bool        enabled   = IsEnabled();
    bool        has_focus = HasFocus();
    StyledState st        = ResolveStyledState(enabled, false, false);

    StyledPalette& p = style_.palette;
    StyledMetrics& m = style_.metrics;
    StyledSkin&    s = style_.skin;

    if(WhenPaintBackground)
        WhenPaintBackground(w, outer, p, m, s, st, has_focus);
    else
        UiPaintStyledBackground(w, outer, p, m, s, st, has_focus);

    Font fnt = m.use_text_font ? m.text_font : style_.font;
    if(IsNull(fnt))
        fnt = StdFont();

    Color ink = p.ink[st];

    if(!IsNull(icon_) && !layout_.support.IsEmpty()) {
        UiPaintStyledIcon(w,
                          layout_.support,
                          icon_,
                          icon_scale_,
                          mono_icon_,
                          ink,
                          enabled);
    }

    if(!lines_.IsEmpty() && !layout_.main.IsEmpty()) {
        UiPaintStyledText(w,
                          layout_.main,
                          lines_,
                          line_sizes_,
                          style_.align_h,
                          style_.align_v,
                          fnt,
                          ink,
                          has_access_mnemonic_ ? accesskey_ : 0,
                          style_.underline,
                          style_.underline_width,
                          style_.underline_offset);
    }

    if(WhenPaintForeground)
        WhenPaintForeground(w, outer, p, m, s, st, has_focus);
    else
        UiPaintStyledForeground(w, outer, p, m, s, st, has_focus);
}

// ============================================================================
// Accessibility & access keys
// ============================================================================

String UiLabel::GetDesc() const
{
    if(!text_.IsEmpty())
        return text_;
    return t_("Label");
}

dword UiLabel::GetAccessKeys() const
{
    return accesskey_ ? AccessKeyBit(accesskey_) : 0;
}

void UiLabel::AssignAccessKeys(dword used)
{
    if(has_access_mnemonic_ && accesskey_) {
        used |= Ctrl::AccessKeyBit(accesskey_);
        Ctrl::AssignAccessKeys(used);
        return;
    }

    // Optional: auto-assign ONLY when there was no '&' markup.
    // This runs rarely, so scanning text_ here is fine.
    WString wtxt = text_.ToWString();
    for(int i = 0; i < wtxt.GetCount(); i++) {
        wchar c = wtxt[i];
        dword bit = Ctrl::AccessKeyBit(c);
        if(bit && !(used & bit)) {
            accesskey_ = ToUpper(c);
            used |= bit;
            break;
        }
    }

    Ctrl::AssignAccessKeys(used);
}


} // namespace Upp
