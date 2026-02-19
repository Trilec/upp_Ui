#include <Ui/UiButton.h>
#include <Ui/UiDraw.h>

namespace Upp {

// ------------------------------------------------------------
// Helper: resolve icon for a given visual state
// ------------------------------------------------------------
Image UiButton::ResolveIconForState(StyledState st) const
{
    const Image* icons = style_.icon_images;

    const Image& normal = icons[ST_NORMAL];

    if(!IsNull(normal)) {
        const Image& specific = icons[st];
        return IsNull(specific) ? normal : specific;
    }

    for(int i = 0; i < 4; i++) {
        if(!IsNull(icons[i]))
            return icons[i];
    }

    return Image();
}

// Stable icon size across all states (prevents min-size/layout jitter)
Size UiButton::GetStableIconSize() const
{
    Size best(0, 0);
    for(int i = 0; i < 4; i++) {
        const Image& img = style_.icon_images[i];
        if(!IsNull(img)) {
            Size s = img.GetSize();
            best.cx = max(best.cx, s.cx);
            best.cy = max(best.cy, s.cy);
        }
    }
    return best;
}

// ------------------------------------------------------------
// StyleDefault: Chameleon-aware default look
// ------------------------------------------------------------

const UiButton::Style& UiButton::StyleDefault()
{
    static UiButton::Style s;
    ONCELOCK {
        Color face  = SColorFace();
        Color text  = SColorText();
        Color frame = SColorShadow();
        Color dis   = SColorDisabled();

        s.palette.face[ST_NORMAL]   = UiFill::Solid(DkColor(face, 2));
        s.palette.face[ST_HOT]      = UiFill::Solid(DkColor(face, 12));
        s.palette.face[ST_PRESSED]  = UiFill::Solid(DkColor(face, 20));
        s.palette.face[ST_DISABLED] = UiFill::Solid(DisabledColor(face));

        s.palette.frame[ST_NORMAL]   = frame;
        s.palette.frame[ST_HOT]      = LtColor(frame, 20);
        s.palette.frame[ST_PRESSED]  = DkColor(frame, 10);
        s.palette.frame[ST_DISABLED] = DisabledColor(frame);

        s.palette.ink[ST_NORMAL]   = text;
        s.palette.ink[ST_HOT]      = LtColor(text, 8);
        s.palette.ink[ST_PRESSED]  = DkColor(text, 8);
        s.palette.ink[ST_DISABLED] = dis;

        s.metrics.radius        = DPI(4);
        s.metrics.frame_width   = DPI(1);
        s.metrics.frame_enabled = true;
        s.metrics.face_enabled  = true;
        s.metrics.dashed        = false;

        s.metrics.text_font     = StdFont();
        s.metrics.use_text_font = false;

        s.skin.enabled        = false;
        s.skin.content_inset  = Rect(0, 0, 0, 0);

        s.press_offset = Point(1, 1);
        s.focus_margin = DPI(2);
        s.overpaint    = DPI(2);
        s.font         = StdFont();
        s.transparent  = false;

        s.align_h     = UiAlign::CENTER;
        s.align_v     = UiAlign::CENTER;
        s.icon_layout = UiAlign::LEFT;

        s.icon_margin = Rect(DPI(1), DPI(1), DPI(1), DPI(1));
        s.text_margin = Rect(DPI(2), 0, 0, 0);

        for(int i = 0; i < 4; i++)
            s.icon_images[i] = Image();

        s.underline        = false;
        s.underline_width  = DPI(1);
        s.underline_offset = DPI(1);
    }
    return s;
}

const UiButton::Style& UiButton::StyleAccent()
{
    static Style s;
    ONCELOCK {
        s = Style(StyleDefault());

        Color face  = LtColor(SColorHighlight(), 12);
        Color frame = DkColor(face, 20);
        Color ink   = SColorHighlightText();

        s.palette.face[ST_NORMAL]   = UiFill::Solid(DkColor(face, 2));
        s.palette.face[ST_HOT]      = UiFill::Solid(DkColor(face, 12));
        s.palette.face[ST_PRESSED]  = UiFill::Solid(DkColor(face, 20));
        s.palette.face[ST_DISABLED] = UiFill::Solid(DisabledColor(face));

        s.palette.frame[ST_NORMAL]   = frame;
        s.palette.frame[ST_HOT]      = LtColor(frame, 20);
        s.palette.frame[ST_PRESSED]  = DkColor(frame, 10);
        s.palette.frame[ST_DISABLED] = DisabledColor(frame);

        s.palette.ink[ST_NORMAL]   = ink;
        s.palette.ink[ST_HOT]      = ink;
        s.palette.ink[ST_PRESSED]  = ink;
        s.palette.ink[ST_DISABLED] = SColorDisabled();

        s.metrics.radius = DPI(4);
    }
    return s;
}

const UiButton::Style& UiButton::StyleSubtle()
{
    static Style s;
    ONCELOCK {
        s = Style(StyleDefault());

        Color frame = LtColor(SColorShadow(), 15);
        Color ink   = LtColor(SColorText(), 15);

        s.metrics.face_enabled = false;

        s.palette.frame[ST_NORMAL]   = frame;
        s.palette.frame[ST_HOT]      = DkColor(frame, 10);
        s.palette.frame[ST_PRESSED]  = DkColor(frame, 20);
        s.palette.frame[ST_DISABLED] = DisabledColor(frame);

        s.palette.ink[ST_NORMAL]   = ink;
        s.palette.ink[ST_HOT]      = LtColor(ink, 10);
        s.palette.ink[ST_PRESSED]  = DkColor(ink, 5);
        s.palette.ink[ST_DISABLED] = SColorDisabled();

        s.metrics.radius = DPI(4);
    }
    return s;
}

const UiButton::Style& UiButton::StyleIcon()
{
    static Style s;
    ONCELOCK {
        s = Style(StyleSubtle());

        s.metrics.frame_width = DPI(1);
        s.metrics.radius      = DPI(1);

        s.icon_margin = Rect(DPI(0), DPI(0), DPI(0), DPI(0));
        s.text_margin = Rect(0, 0, 0, 0);

        s.palette.ink[ST_NORMAL]   = White();
        s.palette.ink[ST_HOT]      = White();
        s.palette.ink[ST_PRESSED]  = White();
        s.palette.ink[ST_DISABLED] = SColorDisabled();

        s.metrics.face_enabled = false;

        s.align_h = UiAlign::CENTER;
        s.align_v = UiAlign::CENTER;
        s.icon_layout = UiAlign::LEFT;
    }
    return s;
}

// ------------------------------------------------------------
// Constructor & style wiring
// ------------------------------------------------------------

UiButton::UiButton()
    : style_(StyleDefault())
{
    BackPaint();
    WantFocus();

    user_min_size_ = Size(DPI(70), DPI(24));

    RebuildTextLines();
    minsize_dirty_  = true;
    layout_dirty_   = true;
    layout_content_ = Rect(0, 0, 0, 0);
}

UiButton& UiButton::SetStyle(const Style& s)
{
    style_ = Style(s);
    OnStyleChanged();
    return *this;
}

void UiButton::OnStyleChanged()
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

// ------------------------------------------------------------
// Multiline & layout helpers
// ------------------------------------------------------------

void UiButton::RebuildTextLines()
{
    lines_.Clear();
    line_sizes_.Clear();

    if(text_.IsEmpty())
        return;

    Font fnt = style_.metrics.use_text_font ? style_.metrics.text_font : style_.font;
    if(IsNull(fnt))
        fnt = StdFont();

    UiBuildStyledTextLines(text_, fnt, lines_, line_sizes_);
}

Size UiButton::GetTextBlockSize() const
{
    return UiMeasureStyledTextBlock(line_sizes_);
}

// Compute natural OUTER size (content + chrome), without user_min_size_
Size UiButton::ComputeNaturalSize() const
{
    Size text_block = GetTextBlockSize();
    bool have_text  = !lines_.IsEmpty();

    Size icon_sz   = GetStableIconSize();
    bool have_icon = icon_sz.cx > 0 && icon_sz.cy > 0;

    UiAlign stack_dir = style_.icon_layout;
    if(stack_dir != UiAlign::LEFT && stack_dir != UiAlign::RIGHT &&
       stack_dir != UiAlign::TOP  && stack_dir != UiAlign::BOTTOM)
        stack_dir = UiAlign::LEFT;

    Size content = UiMeasureBlocksContent(icon_sz,
                                          text_block,
                                          style_.icon_margin,
                                          style_.text_margin,
                                          stack_dir,
                                          have_icon,
                                          have_text,
                                          DPI(40), // empty_w
                                          DPI(20), // empty_h
                                          DPI(16)  // min_support_side
                                          );

    return UiStyledOuterSizeFromContent(content, style_.metrics, style_.skin);
}

void UiButton::UpdateLayout(const Rect& content) const
{
    layout_ = UiBlocksLayout();

    if(content.IsEmpty()) {
        layout_dirty_ = false;
        return;
    }

    Size text_block = GetTextBlockSize();
    bool have_text  = !lines_.IsEmpty();

    Size icon_sz   = GetStableIconSize();
    bool have_icon = icon_sz.cx > 0 && icon_sz.cy > 0;

    UiAlign stack_dir = style_.icon_layout;
    if(stack_dir != UiAlign::LEFT && stack_dir != UiAlign::RIGHT &&
       stack_dir != UiAlign::TOP  && stack_dir != UiAlign::BOTTOM)
        stack_dir = UiAlign::LEFT;

    // UiComputeBlocksLayout already applies margins via UiApplyMarginRect internally.
    layout_ = UiComputeBlocksLayout(content,
                                    have_icon ? icon_sz : Size(0, 0),
                                    have_text ? text_block : Size(0, 0),
                                    style_.align_h,
                                    style_.align_v,
                                    stack_dir,
                                    style_.icon_margin,
                                    style_.text_margin,
                                    DPI(16));

    layout_dirty_ = false;
}

// ------------------------------------------------------------
// API
// ------------------------------------------------------------

UiButton& UiButton::SetUnderline(bool on, int thickness, int offset)
{
    style_.underline        = on;
    style_.underline_width  = max(thickness, 0);
    style_.underline_offset = offset;
    Refresh();
    return *this;
}

UiButton& UiButton::SetIconLayout(UiAlign layout)
{
    if(layout != UiAlign::LEFT && layout != UiAlign::RIGHT &&
       layout != UiAlign::TOP  && layout != UiAlign::BOTTOM)
        layout = UiAlign::LEFT;

    style_.icon_layout = layout;
    minsize_dirty_ = true;
    layout_dirty_  = true;
    RefreshLayout();
    Refresh();
    return *this;
}

UiButton& UiButton::SetAlign(UiAlign h, UiAlign v)
{
    style_.align_h = h;
    style_.align_v = v;
    layout_dirty_  = true;
    RefreshLayout();
    Refresh();
    return *this;
}

UiButton& UiButton::SetAlignH(UiAlign h)
{
    style_.align_h = h;
    layout_dirty_  = true;
    RefreshLayout();
    Refresh();
    return *this;
}

UiButton& UiButton::SetAlignV(UiAlign v)
{
    style_.align_v = v;
    layout_dirty_  = true;
    RefreshLayout();
    Refresh();
    return *this;
}

UiButton& UiButton::SetIconMargin(const Rect& m)
{
    style_.icon_margin = m;
    minsize_dirty_ = true;
    layout_dirty_  = true;
    RefreshLayout();
    Refresh();
    return *this;
}

UiButton& UiButton::SetTextMargin(const Rect& m)
{
    style_.text_margin = m;
    minsize_dirty_ = true;
    layout_dirty_  = true;
    RefreshLayout();
    Refresh();
    return *this;
}

UiButton& UiButton::SetText(const String& text)
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
                raw.Cat('&');
                i++;
            }
            else {
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


// ------------------------------------------------------------
// Icon API (state-aware)
// ------------------------------------------------------------

UiButton& UiButton::SetIcon(const Image& img)
{
    for(int i = 0; i < 4; i++)
        style_.icon_images[i] = img;

    minsize_dirty_ = true;
    layout_dirty_  = true;
    RefreshLayout();
    Refresh();
    return *this;
}

UiButton& UiButton::SetIconState(const Image& img, StyledState state)
{
    if(state < ST_NORMAL || state > ST_DISABLED)
        return *this;

    style_.icon_images[state] = img;

    minsize_dirty_ = true;
    layout_dirty_  = true;
    RefreshLayout();
    Refresh();
    return *this;
}

UiButton& UiButton::SetIcons(const Image& normal,
                             const Image& hot,
                             const Image& pressed,
                             const Image& disabled)
{
    style_.icon_images[ST_NORMAL]   = normal;
    style_.icon_images[ST_HOT]      = IsNull(hot)      ? normal : hot;
    style_.icon_images[ST_PRESSED]  = IsNull(pressed)  ? normal : pressed;
    style_.icon_images[ST_DISABLED] = IsNull(disabled) ? normal : disabled;

    minsize_dirty_ = true;
    layout_dirty_  = true;
    RefreshLayout();
    Refresh();
    return *this;
}

UiButton& UiButton::ClearIcon()
{
    for(int i = 0; i < 4; i++)
        style_.icon_images[i] = Image();

    minsize_dirty_ = true;
    layout_dirty_  = true;
    RefreshLayout();
    Refresh();
    return *this;
}

// ------------------------------------------------------------
// State & layout
// ------------------------------------------------------------

void UiButton::UpdateVisualState()
{
    const bool enabled = IsEnabled();

    if(!enabled) {
        visual_state_ = ST_DISABLED;
        return;
    }

    if(pressed_) {
        visual_state_ = ST_PRESSED;
        return;
    }

    // Checked is a persistent "pressed" look.
    if(checkable_ && checked_) {
        visual_state_ = ST_PRESSED;
        return;
    }

    visual_state_ = mouse_over_ ? ST_HOT : ST_NORMAL;
}

UiButton& UiButton::SetCheckable(bool on)
{
    checkable_ = on;
    if(!checkable_)
        checked_ = false;
    UpdateVisualState();
    Refresh();
    return *this;
}

UiButton& UiButton::SetChecked(bool on)
{
    if(!checkable_)
        return *this;

    if(checked_ == on)
        return *this;

    checked_ = on;
    UpdateVisualState();
    Refresh();
    return *this;
}

void UiButton::Activate_()
{
    if(!IsEnabled())
        return;

    if(checkable_)
        checked_ = !checked_;

    UpdateVisualState();
    Refresh();

    WhenPush();
    WhenAction();
}

Size UiButton::GetMinSize() const
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

void UiButton::SetMinSize(Size sz)
{
    user_min_size_ = sz;
    minsize_dirty_ = true;
    RefreshLayout();
}

void UiButton::Layout()
{
    Rect outer   = GetSize();
    Rect content = UiStyledInnerRect(outer, style_.metrics, style_.skin);

    if(!layout_dirty_ && content == layout_content_)
        return;

    layout_content_ = content;
    UpdateLayout(content);
}

// ------------------------------------------------------------
// Event handling
// ------------------------------------------------------------

void UiButton::MouseEnter(Point, dword)
{
    mouse_over_ = true;
    UpdateVisualState();
    Refresh();
}

void UiButton::MouseLeave()
{
    mouse_over_ = false;

    if(pressed_) {
        pressed_ = false;
        ReleaseCapture();
    }

    UpdateVisualState();
    Refresh();
}

void UiButton::LeftDown(Point, dword)
{
    pressed_ = true;
    SetCapture();
    if(click_focus_)
        SetFocus();
    UpdateVisualState();
    Refresh();
}

void UiButton::LeftUp(Point p, dword)
{
    bool was_pressed = pressed_;
    pressed_ = false;
    ReleaseCapture();
    UpdateVisualState();
    Refresh();

    if(was_pressed && Rect(GetSize()).Contains(p))
        Activate_();
}

void UiButton::GotFocus()
{
    UpdateVisualState();
    Refresh();
}

void UiButton::LostFocus()
{
    UpdateVisualState();
    Refresh();
}

UiButton& UiButton::ClickFocus(bool on)
{
    click_focus_ = on;
    return *this;
}

void UiButton::CancelMode()
{
    if(pressed_ || mouse_over_) {
        pressed_    = false;
        mouse_over_ = false;
        UpdateVisualState();
        Refresh();
    }
    Ctrl::CancelMode();
}

bool UiButton::Key(dword key, int)
{
    switch(key) {
    case K_SPACE:
    case K_ENTER:
        Activate_();
        return true;
    default:
        break;
    }
    return Ctrl::Key(key, 1);
}

// ------------------------------------------------------------
// Paint
// ------------------------------------------------------------

void UiButton::Paint(Draw& w)
{
    Rect outer = GetSize();
    if(outer.IsEmpty())
        return;

    UpdateVisualState();

    bool        enabled   = IsEnabled();
    bool        has_focus = HasFocus();
    StyledState st        = visual_state_;

    StyledPalette& p = style_.palette;
    StyledMetrics& m = style_.metrics;
    StyledSkin&    s = style_.skin;

    if(WhenPaintBackground)
        WhenPaintBackground(w, outer, p, m, s, st, has_focus);
    else
        UiPaintStyledBackground(w, outer, p, m, s, st, has_focus);

    Font font = m.use_text_font ? m.text_font : style_.font;
    if(IsNull(font))
        font = StdFont();

    Color ink = AdjustInk(p.ink[st], st);
    Color icon_ink = UiResolveIconColor(p, st);
    if(IsNull(icon_ink))
        icon_ink = ink;

    Rect icon_r = layout_.support;
    Rect text_r = layout_.main;
    // Physical press offset only (checked should not shift content).
    if(pressed_) {
        icon_r.Offset(style_.press_offset);
        text_r.Offset(style_.press_offset);
    }

    Image icon_img = ResolveIconForState(st);
    if(!IsNull(icon_img) && !icon_r.IsEmpty()) {
        UiPaintStyledIcon(w,
                          icon_r,
                          icon_img,
                          icon_scale_,
                          style_.icon_tint_mono,
                          icon_ink,
                          enabled);
    }

    if(!lines_.IsEmpty() && !text_r.IsEmpty()) {
        UiPaintStyledText(w,
                          text_r,
                          lines_,
                          line_sizes_,
                          style_.align_h,
                          style_.align_v,
                          font,
                          ink,
                          has_access_mnemonic_ ? accesskey_ : 0,
                          style_.underline,
                          style_.underline_width,
                          style_.underline_offset);
    }

    if(WhenPaintForeground) {
        WhenPaintForeground(w, outer, p, m, s, st, has_focus);
    }
    else {
        UiPaintStyledForeground(w, outer, p, m, s, st, has_focus, style_.focus_margin, SColorHighlight());
    }
}

// ------------------------------------------------------------
// Accessibility & access keys
// ------------------------------------------------------------

String UiButton::GetDesc() const
{
    if(!text_.IsEmpty())
        return text_;
    return t_("Button");
}

dword UiButton::GetAccessKeys() const
{
    return accesskey_ ? Ctrl::AccessKeyBit(accesskey_) : 0;
}

void UiButton::AssignAccessKeys(dword used)
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


void UiButton::RebuildLook()
{
    // reserved
}

} // namespace Upp
