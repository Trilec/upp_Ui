#include <Ui/UiButton.h>
#include <Ui/UiDraw.h>
#include <Ui/UiTheme.h>

namespace Upp {

Image UiButton::ResolveIconForState(StyledState st) const
{
    const Image& assigned = assigned_icon_images_[st];
    if(!IsNull(assigned))
        return assigned;

    for(int i = 0; i < 4; i++) {
        if(!IsNull(assigned_icon_images_[i]))
            return assigned_icon_images_[i];
    }

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

Size UiButton::GetStableIconSize() const
{
    if(icon_size_.cx > 0 && icon_size_.cy > 0)
        return icon_size_;

    Size best(0, 0);
    for(int i = 0; i < 4; i++) {
        const Image& img = !IsNull(assigned_icon_images_[i]) ? assigned_icon_images_[i]
                                                             : GetEffectiveStyle().icon_images[i];
        if(!IsNull(img)) {
            Size s = img.GetSize();
            best.cx = max(best.cx, s.cx);
            best.cy = max(best.cy, s.cy);
        }
    }
    return best;
}

bool UiButton::HasResolvedIcon() const
{
    for(int i = 0; i < 4; i++) {
        if(!IsNull(assigned_icon_images_[i]))
            return true;
    }

    const Style& style = GetEffectiveStyle();
    for(int i = 0; i < 4; i++) {
        if(!IsNull(style.icon_images[i]))
            return true;
    }

    return false;
}

const UiButton::Style& UiButton::StyleDefault()
{
    static UiButton::Style s;
    ONCELOCK {
        const Color text_primary   = Color(17, 24, 39);
        const Color text_muted     = Color(107, 114, 128);
        const Color border_neutral = Color(215, 219, 226);
        const Color face_hot       = Color(247, 248, 250);
        const Color face_pressed   = Color(239, 243, 247);
        const Color accent         = Color(0, 120, 212);

        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::None();
            s.palette.frame[i] = border_neutral;
            s.palette.ink[i] = text_primary;
            s.palette.icon[i] = Null;
        }

        s.palette.face[ST_HOT]      = UiFill::Solid(face_hot);
        s.palette.face[ST_PRESSED]  = UiFill::Solid(face_pressed);
        s.palette.face[ST_DISABLED] = UiFill::None();

        s.palette.frame[ST_HOT]      = DkColor(border_neutral, 8);
        s.palette.frame[ST_PRESSED]  = DkColor(border_neutral, 14);
        s.palette.frame[ST_DISABLED] = Color(229, 231, 235);

        s.palette.ink[ST_HOT]      = text_primary;
        s.palette.ink[ST_PRESSED]  = text_primary;
        s.palette.ink[ST_DISABLED] = text_muted;

        s.metrics.text_font = StdFont();
        s.metrics.use_text_font = false;
        s.metrics.content_margin = Rect(DPI(14), DPI(8), DPI(14), DPI(8));
        s.metrics.radius = DPI(8);
        s.metrics.frame_width = DPI(1);
        s.metrics.frame_enabled = true;
        s.metrics.face_enabled = true;
        s.metrics.dashed = false;
        s.metrics.high_contrast = false;
        s.metrics.shadow = StyledShadow();
        s.metrics.highlight = StyledHighlight();

        s.skin = StyledSkin();

        s.press_offset = Point(0, 0);
        s.metrics.focus_margin = DPI(2);
        s.overpaint = 0;
        s.font = SansSerifZ(13);
        s.transparent = false;

        s.align_h = UiAlign::CENTER;
        s.align_v = UiAlign::CENTER;
        s.icon_side = UiAlign::LEFT;
        s.content_gap = DPI(4);
        s.icon_render_mode = UiIconRenderMode::MonoTint;

        for(int i = 0; i < 4; i++)
            s.icon_images[i] = Image();

        s.underline = false;
        s.underline_width = DPI(1);
        s.underline_offset = DPI(2);

        s.palette.frame[ST_DISABLED] = Blend(border_neutral, SColorPaper(), 150);
        s.palette.ink[ST_DISABLED] = Blend(text_muted, SColorPaper(), 40);
        s.palette.icon[ST_NORMAL] = accent;
        s.palette.icon[ST_HOT] = accent;
        s.palette.icon[ST_PRESSED] = DkColor(accent, 10);
        s.palette.icon[ST_DISABLED] = Blend(accent, SColorPaper(), 180);
    }
    return s;
}

UiButton::UiButton()
    : style_(StyleDefault())
    , themed_style_(StyleDefault())
{
    BackPaint();
    WantFocus();

    user_min_size_ = Size(DPI(70), DPI(24));
    SyncThemeStyle();
    RebuildTextLinesFromStyle(GetEffectiveStyle());
    minsize_dirty_ = true;
    layout_dirty_ = true;
    layout_content_ = Rect(0, 0, 0, 0);
}

UiButton::Style UiButton::ResolveThemeStyle() const
{
    return UiTheme::ResolveButton();
}

void UiButton::InvalidateStyleCache()
{
    theme_revision_ = 0;
    minsize_dirty_ = true;
    layout_dirty_ = true;
    layout_content_ = Rect(0, 0, 0, 0);
}

UiButton::Style& UiButton::StyleEdit()
{
    if(!has_custom_style_) {
        style_ = GetEffectiveStyle();
        has_custom_style_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

void UiButton::SyncThemeStyle()
{
    if(has_custom_style_)
        return;

    const uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;

    themed_style_ = ResolveThemeStyle();
    theme_revision_ = revision;
    RebuildTextLinesFromStyle(themed_style_);
    minsize_dirty_ = true;
    layout_dirty_ = true;
    layout_content_ = Rect(0, 0, 0, 0);
}

void UiButton::RebuildTextLines()
{
    RebuildTextLinesFromStyle(GetEffectiveStyle());
}

void UiButton::RebuildTextLinesFromStyle(const Style& st)
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

UiButton& UiButton::SetCustomStyle(const Style& s)
{
    style_ = Style(s);
    has_custom_style_ = true;
    OnStyleChanged();
    return *this;
}

UiButton& UiButton::ClearCustomStyle()
{
    if(!has_custom_style_)
        return *this;

    has_custom_style_ = false;
    style_ = StyleDefault();
    InvalidateStyleCache();
    OnStyleChanged();
    return *this;
}

const UiButton::Style& UiButton::GetEffectiveStyle() const
{
    if(has_custom_style_)
        return style_;

    const_cast<UiButton*>(this)->SyncThemeStyle();
    return themed_style_;
}

void UiButton::OnStyleChanged()
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

Size UiButton::GetTextBlockSize() const
{
    return UiMeasureStyledTextBlock(line_sizes_);
}

Size UiButton::ComputeNaturalSize() const
{
    const Style& style = GetEffectiveStyle();

    Size text_block = GetTextBlockSize();
    bool have_text = !lines_.IsEmpty();

    bool have_icon = HasResolvedIcon();
    Size icon_sz = have_icon ? GetStableIconSize() : Size(0, 0);
    const bool explicit_icon_size = icon_size_.cx > 0 && icon_size_.cy > 0;

    UiAlign stack_dir = style.icon_side;
    if(stack_dir != UiAlign::LEFT && stack_dir != UiAlign::RIGHT &&
       stack_dir != UiAlign::TOP  && stack_dir != UiAlign::BOTTOM)
        stack_dir = UiAlign::LEFT;

    Size content = UiMeasureBlocksContent(icon_sz,
                                          text_block,
                                          stack_dir,
                                          have_icon,
                                          have_text,
                                          DPI(40),
                                          DPI(20),
                                          explicit_icon_size ? 0 : DPI(16),
                                          have_icon && have_text ? style.content_gap : 0);

    return UiStyledOuterSizeFromContent(content, style.metrics, style.skin);
}

void UiButton::UpdateLayout(const Rect& content) const
{
    const Style& style = GetEffectiveStyle();

    layout_ = UiBlocksLayout();
    if(content.IsEmpty()) {
        layout_dirty_ = false;
        return;
    }

    Size text_block = GetTextBlockSize();
    bool have_text = !lines_.IsEmpty();

    bool have_icon = HasResolvedIcon();
    Size icon_sz = have_icon ? GetStableIconSize() : Size(0, 0);
    const bool explicit_icon_size = icon_size_.cx > 0 && icon_size_.cy > 0;

    UiAlign stack_dir = style.icon_side;
    if(stack_dir != UiAlign::LEFT && stack_dir != UiAlign::RIGHT &&
       stack_dir != UiAlign::TOP  && stack_dir != UiAlign::BOTTOM)
        stack_dir = UiAlign::LEFT;

    layout_ = UiComputeBlocksLayout(content,
                                    have_icon ? icon_sz : Size(0, 0),
                                    have_text ? text_block : Size(0, 0),
                                    style.align_h,
                                    style.align_v,
                                    stack_dir,
                                    explicit_icon_size ? 0 : DPI(16),
                                    have_icon && have_text ? style.content_gap : 0);

    layout_dirty_ = false;
}

UiButton& UiButton::SetUnderline(bool on, int thickness, int offset)
{
    Style& style = StyleEdit();
    style.underline = on;
    style.underline_width = max(thickness, 0);
    style.underline_offset = offset;
    OnStyleChanged();
    return *this;
}

UiButton& UiButton::SetIconSide(UiAlign side)
{
    if(side != UiAlign::LEFT && side != UiAlign::RIGHT &&
       side != UiAlign::TOP  && side != UiAlign::BOTTOM)
        side = UiAlign::LEFT;

    StyleEdit().icon_side = side;
    OnStyleChanged();
    return *this;
}

UiButton& UiButton::SetAlign(UiAlign h, UiAlign v)
{
    Style& style = StyleEdit();
    style.align_h = h;
    style.align_v = v;
    OnStyleChanged();
    return *this;
}

UiButton& UiButton::SetAlignH(UiAlign h)
{
    StyleEdit().align_h = h;
    OnStyleChanged();
    return *this;
}

UiButton& UiButton::SetAlignV(UiAlign v)
{
    StyleEdit().align_v = v;
    OnStyleChanged();
    return *this;
}

UiButton& UiButton::SetContentGap(int gap)
{
    StyleEdit().content_gap = max(0, gap);
    OnStyleChanged();
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

UiButton& UiButton::SetIcon(const Image& img)
{
    for(int i = 0; i < 4; i++)
        assigned_icon_images_[i] = img;
    OnStyleChanged();
    return *this;
}

UiButton& UiButton::SetIconState(const Image& img, StyledState state)
{
    if(state < ST_NORMAL || state > ST_DISABLED)
        return *this;

    assigned_icon_images_[state] = img;
    OnStyleChanged();
    return *this;
}

UiButton& UiButton::SetIcons(const Image& normal,
                             const Image& hot,
                             const Image& pressed,
                             const Image& disabled)
{
    assigned_icon_images_[ST_NORMAL] = normal;
    assigned_icon_images_[ST_HOT] = IsNull(hot) ? normal : hot;
    assigned_icon_images_[ST_PRESSED] = IsNull(pressed) ? normal : pressed;
    assigned_icon_images_[ST_DISABLED] = IsNull(disabled) ? normal : disabled;
    OnStyleChanged();
    return *this;
}

UiButton& UiButton::ClearIcon()
{
    for(int i = 0; i < 4; i++)
        assigned_icon_images_[i] = Image();
    OnStyleChanged();
    return *this;
}

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
    minsize_dirty_ = false;
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
    const Style& style = GetEffectiveStyle();
    Rect outer = GetSize();
    Rect content = UiStyledInnerRect(outer, style.metrics, style.skin);

    if(!layout_dirty_ && content == layout_content_)
        return;

    layout_content_ = content;
    UpdateLayout(content);
}

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
        pressed_ = false;
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

void UiButton::Paint(Draw& w)
{
    const Style& style = GetEffectiveStyle();
    Rect outer = GetSize();
    if(outer.IsEmpty())
        return;

    Rect content = UiStyledInnerRect(outer, style.metrics, style.skin);
    if(layout_dirty_ || content != layout_content_) {
        layout_content_ = content;
        UpdateLayout(content);
    }

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
    Color icon_ink = has_assigned_icon_colors_ ? assigned_icon_colors_[st] : UiResolveIconColor(p, st);
    UiIconRenderMode icon_render_mode = has_assigned_icon_render_mode_ ? assigned_icon_render_mode_ : style.icon_render_mode;
    const bool explicit_icon_size = icon_size_.cx > 0 && icon_size_.cy > 0;
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
                          true,
                          !explicit_icon_size,
                          icon_render_mode,
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
}

} // namespace Upp
