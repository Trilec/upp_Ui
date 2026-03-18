#include <Ui/UiToolButton.h>
#include <Ui/UiDraw.h>
#include <Ui/UiTheme.h>

namespace Upp {

Image UiToolButton::ResolveIconForState(StyledState st) const
{
    const Style& style = GetEffectiveStyle();
    const Image* icons = style.icon_images;

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

Size UiToolButton::GetStableIconSize() const
{
    const Style& style = GetEffectiveStyle();
    Size best(0, 0);
    for(int i = 0; i < 4; i++) {
        const Image& img = style.icon_images[i];
        if(!IsNull(img)) {
            Size s = img.GetSize();
            best.cx = max(best.cx, s.cx);
            best.cy = max(best.cy, s.cy);
        }
    }
    return best;
}

const UiToolButton::Style& UiToolButton::StyleDefault()
{
    static UiToolButton::Style s;
    ONCELOCK {
        const Color text_primary   = Color(48, 57, 71);
        const Color text_muted     = Color(107, 114, 128);
        const Color border_neutral = Null;
        const Color face_hot       = Color(242, 244, 247);
        const Color face_pressed   = Color(232, 236, 241);
        const Color accent         = Color(112, 122, 138);

        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::None();
            s.palette.frame[i] = Null;
            s.palette.ink[i] = text_primary;
            s.palette.icon[i] = Null;
        }

        s.palette.face[ST_HOT]      = UiFill::Solid(face_hot);
        s.palette.face[ST_PRESSED]  = UiFill::Solid(face_pressed);
        s.palette.face[ST_DISABLED] = UiFill::None();

        s.palette.frame[ST_HOT]      = Null;
        s.palette.frame[ST_PRESSED]  = Null;
        s.palette.frame[ST_DISABLED] = Null;

        s.palette.ink[ST_HOT]      = text_primary;
        s.palette.ink[ST_PRESSED]  = text_primary;
        s.palette.ink[ST_DISABLED] = text_muted;

        s.metrics.text_font = StdFont();
        s.metrics.use_text_font = false;
        s.metrics.content_padding = Rect(DPI(8), DPI(8), DPI(8), DPI(8));
        s.metrics.radius = DPI(4);
        s.metrics.frame_width = DPI(1);
        s.metrics.frame_enabled = false;
        s.metrics.face_enabled = true;
        s.metrics.dashed = false;
        s.metrics.high_contrast = false;
        s.metrics.shadow = StyledShadow();
        s.metrics.highlight = StyledHighlight();

        s.skin = StyledSkin();

        s.press_offset = Point(0, 0);
        s.metrics.focus_margin = DPI(2);
        s.overpaint = 0;
        s.font = StdFont();
        s.transparent = false;

        s.align_h = UiAlign::CENTER;
        s.align_v = UiAlign::CENTER;
        s.icon_layout = UiAlign::CENTER;
        s.icon_margin = Rect(0, 0, 0, 0);
        s.text_margin = Rect(0, 0, 0, 0);
        s.icon_tint_mono = true;

        for(int i = 0; i < 4; i++)
            s.icon_images[i] = Image();

        s.underline = false;
        s.underline_width = DPI(1);
        s.underline_offset = DPI(2);

        s.palette.frame[ST_DISABLED] = Null;
        s.palette.ink[ST_DISABLED] = Blend(text_muted, SColorPaper(), 40);
        s.palette.icon[ST_NORMAL] = accent;
        s.palette.icon[ST_HOT] = accent;
        s.palette.icon[ST_PRESSED] = DkColor(accent, 10);
        s.palette.icon[ST_DISABLED] = Blend(accent, SColorPaper(), 180);
    }
    return s;
}

UiToolButton::UiToolButton()
    : style_(StyleDefault())
    , themed_style_(StyleDefault())
{
    BackPaint();
    WantFocus();

    user_min_size_ = Size(DPI(28), DPI(28));
    SyncThemeStyle();
    RebuildTextLinesFromStyle(GetEffectiveStyle());
    minsize_dirty_ = true;
    layout_dirty_ = true;
    layout_content_ = Rect(0, 0, 0, 0);
}

void UiToolButton::InvalidateStyleCache()
{
    theme_revision_ = 0;
    minsize_dirty_ = true;
    layout_dirty_ = true;
    layout_content_ = Rect(0, 0, 0, 0);
}

UiToolButton::Style& UiToolButton::StyleEdit()
{
    if(!has_style_override_) {
        style_ = GetEffectiveStyle();
        has_style_override_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

void UiToolButton::SyncThemeStyle()
{
    if(has_style_override_)
        return;

    const uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;

    themed_style_ = UiTheme::ResolveToolButton();
    theme_revision_ = revision;
    RebuildTextLinesFromStyle(themed_style_);
    minsize_dirty_ = true;
    layout_dirty_ = true;
    layout_content_ = Rect(0, 0, 0, 0);
}

void UiToolButton::RebuildTextLines()
{
    RebuildTextLinesFromStyle(GetEffectiveStyle());
}

void UiToolButton::RebuildTextLinesFromStyle(const Style& st)
{
    lines_.Clear();
    line_sizes_.Clear();

    if(text_.IsEmpty())
        return;

    Font fnt = st.metrics.use_text_font ? st.metrics.text_font : st.font;
    if(IsNull(fnt))
        fnt = StdFont();

    UiBuildStyledTextLines(text_, fnt, lines_, line_sizes_);
}

UiToolButton& UiToolButton::SetStyle(const Style& s)
{
    style_ = Style(s);
    has_style_override_ = true;
    OnStyleChanged();
    return *this;
}

UiToolButton& UiToolButton::ClearStyleOverride()
{
    if(!has_style_override_)
        return *this;

    has_style_override_ = false;
    style_ = StyleDefault();
    InvalidateStyleCache();
    OnStyleChanged();
    return *this;
}

const UiToolButton::Style& UiToolButton::GetEffectiveStyle() const
{
    if(has_style_override_)
        return style_;

    const_cast<UiToolButton*>(this)->SyncThemeStyle();
    return themed_style_;
}

void UiToolButton::OnStyleChanged()
{
    const Style& style = GetEffectiveStyle();

    if(style.transparent)
        Transparent();
    else
        BackPaint();

    RebuildTextLinesFromStyle(style);
    InvalidateStyleCache();
    RefreshLayout();
    Refresh();
}

Size UiToolButton::GetTextBlockSize() const
{
    return UiMeasureStyledTextBlock(line_sizes_);
}

Size UiToolButton::ComputeNaturalSize() const
{
    const Style& style = GetEffectiveStyle();

    Size text_block = GetTextBlockSize();
    bool have_text = !lines_.IsEmpty();

    Size icon_sz = GetStableIconSize();
    bool have_icon = icon_sz.cx > 0 && icon_sz.cy > 0;

    UiAlign stack_dir = style.icon_layout;
    if(stack_dir != UiAlign::LEFT && stack_dir != UiAlign::RIGHT &&
       stack_dir != UiAlign::TOP  && stack_dir != UiAlign::BOTTOM)
        stack_dir = UiAlign::LEFT;

    Size content = UiMeasureBlocksContent(icon_sz,
                                          text_block,
                                          style.icon_margin,
                                          style.text_margin,
                                          stack_dir,
                                          have_icon,
                                          have_text,
                                          DPI(40),
                                          DPI(20),
                                          DPI(16));

    return UiStyledOuterSizeFromContent(content, style.metrics, style.skin);
}

void UiToolButton::UpdateLayout(const Rect& content) const
{
    const Style& style = GetEffectiveStyle();

    layout_ = UiBlocksLayout();
    if(content.IsEmpty()) {
        layout_dirty_ = false;
        return;
    }

    Size text_block = GetTextBlockSize();
    bool have_text = !lines_.IsEmpty();

    Size icon_sz = GetStableIconSize();
    bool have_icon = icon_sz.cx > 0 && icon_sz.cy > 0;

    UiAlign stack_dir = style.icon_layout;
    if(stack_dir != UiAlign::LEFT && stack_dir != UiAlign::RIGHT &&
       stack_dir != UiAlign::TOP  && stack_dir != UiAlign::BOTTOM)
        stack_dir = UiAlign::LEFT;

    layout_ = UiComputeBlocksLayout(content,
                                    have_icon ? icon_sz : Size(0, 0),
                                    have_text ? text_block : Size(0, 0),
                                    style.align_h,
                                    style.align_v,
                                    stack_dir,
                                    style.icon_margin,
                                    style.text_margin,
                                    DPI(16));

    layout_dirty_ = false;
}

UiToolButton& UiToolButton::SetUnderline(bool on, int thickness, int offset)
{
    Style& style = StyleEdit();
    style.underline = on;
    style.underline_width = max(thickness, 0);
    style.underline_offset = offset;
    OnStyleChanged();
    return *this;
}

UiToolButton& UiToolButton::SetIconLayout(UiAlign layout)
{
    if(layout != UiAlign::LEFT && layout != UiAlign::RIGHT &&
       layout != UiAlign::TOP  && layout != UiAlign::BOTTOM)
        layout = UiAlign::LEFT;

    StyleEdit().icon_layout = layout;
    OnStyleChanged();
    return *this;
}

UiToolButton& UiToolButton::SetAlign(UiAlign h, UiAlign v)
{
    Style& style = StyleEdit();
    style.align_h = h;
    style.align_v = v;
    OnStyleChanged();
    return *this;
}

UiToolButton& UiToolButton::SetAlignH(UiAlign h)
{
    StyleEdit().align_h = h;
    OnStyleChanged();
    return *this;
}

UiToolButton& UiToolButton::SetAlignV(UiAlign v)
{
    StyleEdit().align_v = v;
    OnStyleChanged();
    return *this;
}

UiToolButton& UiToolButton::SetIconMargin(const Rect& m)
{
    StyleEdit().icon_margin = m;
    OnStyleChanged();
    return *this;
}

UiToolButton& UiToolButton::SetTextMargin(const Rect& m)
{
    StyleEdit().text_margin = m;
    OnStyleChanged();
    return *this;
}

UiToolButton& UiToolButton::SetText(const String& text)
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
            for(int k = 0; k < 4; k++)
                text_.Cat(' ');
        }
        else {
            text_.Cat(c);
        }
    }

    RebuildTextLinesFromStyle(GetEffectiveStyle());
    minsize_dirty_ = true;
    layout_dirty_ = true;
    layout_content_ = Rect(0, 0, 0, 0);
    RefreshLayout();
    Refresh();
    return *this;
}

UiToolButton& UiToolButton::SetIcon(const Image& img)
{
    Style& style = StyleEdit();
    for(int i = 0; i < 4; i++)
        style.icon_images[i] = img;
    OnStyleChanged();
    return *this;
}

UiToolButton& UiToolButton::SetIconState(const Image& img, StyledState state)
{
    if(state < ST_NORMAL || state > ST_DISABLED)
        return *this;

    StyleEdit().icon_images[state] = img;
    OnStyleChanged();
    return *this;
}

UiToolButton& UiToolButton::SetIcons(const Image& normal,
                             const Image& hot,
                             const Image& pressed,
                             const Image& disabled)
{
    Style& style = StyleEdit();
    style.icon_images[ST_NORMAL] = normal;
    style.icon_images[ST_HOT] = IsNull(hot) ? normal : hot;
    style.icon_images[ST_PRESSED] = IsNull(pressed) ? normal : pressed;
    style.icon_images[ST_DISABLED] = IsNull(disabled) ? normal : disabled;
    OnStyleChanged();
    return *this;
}

UiToolButton& UiToolButton::ClearIcon()
{
    Style& style = StyleEdit();
    for(int i = 0; i < 4; i++)
        style.icon_images[i] = Image();
    OnStyleChanged();
    return *this;
}

void UiToolButton::UpdateVisualState()
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

    if(checkable_ && checked_) {
        visual_state_ = ST_PRESSED;
        return;
    }

    visual_state_ = mouse_over_ ? ST_HOT : ST_NORMAL;
}

UiToolButton& UiToolButton::SetCheckable(bool on)
{
    checkable_ = on;
    if(!checkable_)
        checked_ = false;
    UpdateVisualState();
    Refresh();
    return *this;
}

UiToolButton& UiToolButton::SetChecked(bool on)
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

void UiToolButton::Activate_()
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

Size UiToolButton::GetMinSize() const
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
    minsize_dirty_ = false;
    return cached_minsize_;
}

void UiToolButton::SetMinSize(Size sz)
{
    user_min_size_ = sz;
    minsize_dirty_ = true;
    RefreshLayout();
}

void UiToolButton::Layout()
{
    const Style& style = GetEffectiveStyle();
    Rect outer = GetSize();
    Rect content = UiStyledInnerRect(outer, style.metrics, style.skin);

    if(!layout_dirty_ && content == layout_content_)
        return;

    layout_content_ = content;
    UpdateLayout(content);
}

void UiToolButton::MouseEnter(Point, dword)
{
    mouse_over_ = true;
    UpdateVisualState();
    Refresh();
}

void UiToolButton::MouseLeave()
{
    mouse_over_ = false;

    if(pressed_) {
        pressed_ = false;
        ReleaseCapture();
    }

    UpdateVisualState();
    Refresh();
}

void UiToolButton::LeftDown(Point, dword)
{
    pressed_ = true;
    SetCapture();
    if(click_focus_)
        SetFocus();
    UpdateVisualState();
    Refresh();
}

void UiToolButton::LeftUp(Point p, dword)
{
    bool was_pressed = pressed_;
    pressed_ = false;
    ReleaseCapture();
    UpdateVisualState();
    Refresh();

    if(was_pressed && Rect(GetSize()).Contains(p))
        Activate_();
}

void UiToolButton::GotFocus()
{
    UpdateVisualState();
    Refresh();
}

void UiToolButton::LostFocus()
{
    UpdateVisualState();
    Refresh();
}

UiToolButton& UiToolButton::ClickFocus(bool on)
{
    click_focus_ = on;
    return *this;
}

void UiToolButton::CancelMode()
{
    if(pressed_ || mouse_over_) {
        pressed_ = false;
        mouse_over_ = false;
        UpdateVisualState();
        Refresh();
    }
    Ctrl::CancelMode();
}

bool UiToolButton::Key(dword key, int)
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

void UiToolButton::Paint(Draw& w)
{
    const Style& style = GetEffectiveStyle();
    Rect outer = GetSize();
    if(outer.IsEmpty())
        return;

    UpdateVisualState();

    const bool has_focus = HasFocus();
    const StyledState st = visual_state_;
    const StyledPalette& p = style.palette;
    const StyledMetrics& m = style.metrics;
    const StyledSkin& s = style.skin;

    if(WhenPaintBackground)
        WhenPaintBackground(w, outer, p, m, s, st, has_focus);
    else
        UiPaintStyledBackground(w, outer, p, m, s, st, has_focus);

    Font font = m.use_text_font ? m.text_font : style.font;
    if(IsNull(font))
        font = StdFont();

    Color ink = AdjustInk(p.ink[st], st);
    Color icon_ink = UiResolveIconColor(p, st);
    if(IsNull(icon_ink))
        icon_ink = ink;

    Rect icon_r = layout_.support;
    Rect text_r = layout_.main;
    if(pressed_) {
        icon_r.Offset(style.press_offset);
        text_r.Offset(style.press_offset);
    }

    Image icon_img = ResolveIconForState(st);
    if(!IsNull(icon_img) && !icon_r.IsEmpty()) {
        UiPaintStyledIcon(w,
                          icon_r,
                          icon_img,
                          icon_scale_,
                          style.icon_tint_mono,
                          icon_ink,
                          IsEnabled());
    }

    if(!lines_.IsEmpty() && !text_r.IsEmpty()) {
        UiPaintStyledText(w,
                          text_r,
                          lines_,
                          line_sizes_,
                          style.align_h,
                          style.align_v,
                          font,
                          ink,
                          has_access_mnemonic_ ? accesskey_ : 0,
                          style.underline,
                          style.underline_width,
                          style.underline_offset);
    }

    if(WhenPaintForeground)
        WhenPaintForeground(w, outer, p, m, s, st, has_focus);
    else
        UiPaintStyledForeground(w, outer, p, m, s, st, has_focus);
}

String UiToolButton::GetDesc() const
{
    if(!text_.IsEmpty())
        return text_;
    return t_("Button");
}

dword UiToolButton::GetAccessKeys() const
{
    return accesskey_ ? Ctrl::AccessKeyBit(accesskey_) : 0;
}

void UiToolButton::AssignAccessKeys(dword used)
{
    if(has_access_mnemonic_ && accesskey_) {
        used |= Ctrl::AccessKeyBit(accesskey_);
        Ctrl::AssignAccessKeys(used);
        return;
    }

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

void UiToolButton::RebuildLook()
{
}

} // namespace Upp



