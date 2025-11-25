#include <Ui/UiButton.h>
#include <Ui/UiDraw.h>

namespace Upp {

// ------------------------------------------------------------
// StyleDefault: Chameleon-aware default look
// ------------------------------------------------------------

const UiButton::Style& UiButton::StyleDefault()
{
    static UiButton::Style s;
    ONCELOCK {
        // Base semantic colors from Chameleon
        Color face  = SColorFace();
        Color text  = SColorText();
        Color frame = SColorShadow();
        Color dis   = SColorDisabled();

        // Face states – slightly stronger hover
        s.palette.face[ST_NORMAL]   = DkColor(face, 2);
        s.palette.face[ST_HOT]      = DkColor(face, 10); 
        s.palette.face[ST_PRESSED]  = DkColor(face, 12);
        s.palette.face[ST_DISABLED] = Blend(face, dis, 200);

        // Frame states
        s.palette.frame[ST_NORMAL]   = frame;
        s.palette.frame[ST_HOT]      = LtColor(frame, 20);
        s.palette.frame[ST_PRESSED]  = DkColor(frame, 10);
        s.palette.frame[ST_DISABLED] = DisabledColor(frame);

        // Ink states – small hover/pressed modulation
        s.palette.ink[ST_NORMAL]   = text;
        s.palette.ink[ST_HOT]      = LtColor(text, 8);   // slightly lighter on hover
        s.palette.ink[ST_PRESSED]  = DkColor(text, 8);   // slightly darker on press
        s.palette.ink[ST_DISABLED] = dis;

        // Metrics
        s.metrics.radius        = DPI(4);
        s.metrics.frame_width   = DPI(1);
        s.metrics.frame_enabled = true;
        s.metrics.face_enabled  = true;
        s.metrics.dashed        = false;

        s.metrics.text_font     = StdFont();
        s.metrics.use_text_font = false;

        // Skin off by default
        s.skin.enabled        = false;
        s.skin.includes_frame = false;
        s.skin.inset          = Rect(0, 0, 0, 0);

        // Control-specific extras
        s.press_offset = Point(1, 1);
        s.focus_margin = DPI(2);
        s.overpaint    = DPI(2);
        s.font         = StdFont();
        s.transparent  = false;
    }
    return s;
}

const UiButton::Style& UiButton::StyleAccent()
{
    static Style s;
    ONCELOCK {
        s = StyleDefault();

        // Slightly lighter than raw SColorHighlight for the base state,
        // but clearly distinct from the neutral default.
        Color face  = LtColor(SColorHighlight(), 12);
        Color frame = DkColor(face, 20);
        Color ink   = SColorHighlightText(); // usually white

        // Face
        s.palette.face[ST_NORMAL]   = DkColor(face, 2);
        s.palette.face[ST_HOT]      = DkColor(face, 12);
        s.palette.face[ST_PRESSED]  = DkColor(face, 20);
        s.palette.face[ST_DISABLED] = DisabledColor(face);

        // Frame
        s.palette.frame[ST_NORMAL]   = frame;
        s.palette.frame[ST_HOT]      = LtColor(frame, 20);
        s.palette.frame[ST_PRESSED]  = DkColor(frame, 10);
        s.palette.frame[ST_DISABLED] = DisabledColor(frame);

        // Ink
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
        s = StyleDefault();

        // Subtle: no fill, lighter border, slightly lighter text.
        Color frame = LtColor(SColorShadow(), 15);   // ~10–15% lighter
        Color ink  = LtColor(SColorText(), 15);     // softer text

        // No face fill
        s.metrics.face_enabled = false;

        // Light frame
        s.palette.frame[ST_NORMAL]   = frame;
        s.palette.frame[ST_HOT]      = DkColor(frame, 10);
        s.palette.frame[ST_PRESSED]  = DkColor(frame, 20);
        s.palette.frame[ST_DISABLED] = DisabledColor(frame);

        // Ink: soft by default, but still clearly clickable on hover
        s.palette.ink[ST_NORMAL]   = ink;
        s.palette.ink[ST_HOT]      = LtColor(ink, 10);
        s.palette.ink[ST_PRESSED]  = DkColor(ink, 5);
        s.palette.ink[ST_DISABLED] = SColorDisabled();

        s.metrics.radius = DPI(4);
    }
    return s;
}


const UiButton::Style& UiButton::StyleLink()
{
    static Style s;
    ONCELOCK {
        s = StyleSubtle();

        // Link: no frame, no fill, strong highlight ink.
        s.metrics.frame_enabled = false;
        s.metrics.face_enabled  = false;

        Color link = SColorHighlight();

        s.palette.ink[ST_NORMAL]   = link;
        s.palette.ink[ST_HOT]      = LtColor(link, 25);   // bright on hover
        s.palette.ink[ST_PRESSED]  = DkColor(link, 20);
        s.palette.ink[ST_DISABLED] = SColorDisabled();

        s.underline        = true;
        s.underline_width  = DPI(1);
        s.underline_offset = DPI(1);

        s.focus_margin = 1;
    }
    return s;
}


// ------------------------------------------------------------
// Constructor & style wiring
// ------------------------------------------------------------

UiButton::UiButton()
{
    style_ = StyleDefault();

    NoTransparent();      // we paint our own background
    WantFocus();          // tab-focusable

    // a sane default minimum size; overridden by user MinSize if set
    user_min_size_ = Size(DPI(70), DPI(24));
}

UiButton& UiButton::SetStyle(const Style& s)
{
    style_ = s;
    OnStyleChanged();
    return *this;
}

void UiButton::OnStyleChanged()
{
    // We keep everything internal – paint and layout read fonts from style_
    RefreshLayout();
    Refresh();
}

// ------------------------------------------------------------
// API
// ------------------------------------------------------------
UiButton& UiButton::SetUnderline(bool on, int thickness, int offset)
{
    style_.underline        = on;
    style_.underline_width  = max(thickness, 0);
    style_.underline_offset = offset;
    OnStyleChanged();
    return *this;
}


UiButton& UiButton::SetLabel(const String& text)
{
    // Extract '&X' access key and normalize label_ (similar to CtrlLib::Pusher)
    label_.Clear();
    accesskey_ = 0;

    for(int i = 0; i < text.GetCount(); i++) {
        int c = text[i];

        if(c == '&' && i + 1 < text.GetCount()) {
            int n = text[i + 1];

            if(n == '&') {
                // Escaped ampersand "&&" -> literal '&'
                label_.Cat('&');
                i++; // skip second '&'
            }
            else {
                // "&X" => set access key to X and omit '&' from label_
                accesskey_ = ToUpper((wchar)n);
                label_.Cat(n);
                i++; // skip the key char (already appended)
            }
        }
        else {
            label_.Cat(c);
        }
    }

    RefreshLayout();
    Refresh();
    return *this;
}


UiButton& UiButton::SetImage(const Image& img)
{
    image_ = img;
    mono_image_ = false;
    RefreshLayout();
    Refresh();
    return *this;
}

UiButton& UiButton::SetImageLayout(UiImageLayout layout)
{
    style_.image_layout = layout;
    OnStyleChanged();
    return *this;
}


UiButton& UiButton::SetMonoImage(const Image& img)
{
    image_ = img;
    mono_image_ = true; // reserved; currently drawn same as SetImage
    RefreshLayout();
    Refresh();
    return *this;
}

// ------------------------------------------------------------
// State & layout
// ------------------------------------------------------------

void UiButton::UpdateVisualState()
{
    bool enabled = IsEnabled();
    bool hot     = mouse_over_;   // hover only, not focus
    bool pressed = pressed_;

    visual_state_ = ResolveStyledState(enabled, hot, pressed);
}


Size UiButton::GetMinSize() const
{
    Font f = style_.metrics.use_text_font ? style_.metrics.text_font : style_.font;
    if(IsNull(f))
        f = StdFont();

    Size text_sz(0, 0);
    if(!label_.IsEmpty())
        text_sz = GetTextSize(label_, f);

    Size img_sz(0, 0);
    if(!IsNull(image_))
        img_sz = image_.GetSize();

    int gap = (!IsNull(image_) && !label_.IsEmpty()) ? style_.image_gap : 0;

    Size content(0, 0);

    switch(style_.image_layout) {
    case UIIMAGE_TOP:
    case UIIMAGE_BOTTOM:
        content.cx = max(img_sz.cx, text_sz.cx);
        content.cy = img_sz.cy + gap + text_sz.cy;
        break;

    case UIIMAGE_RIGHT:
    case UIIMAGE_LEFT:
    default:
        content.cx = img_sz.cx + gap + text_sz.cx;
        content.cy = max(img_sz.cy, text_sz.cy);
        break;
    }

    Size sz;
    sz.cx = content.cx + 2 * style_.padding_h + 2 * style_.metrics.frame_width;
    sz.cy = content.cy + 2 * style_.padding_v + 2 * style_.metrics.frame_width;

    // Baseline safety minima
    sz.cx = max(sz.cx, DPI(40));
    sz.cy = max(sz.cy, DPI(20));

    // Respect user-set min size
    sz.cx = max(sz.cx, user_min_size_.cx);
    sz.cy = max(sz.cy, user_min_size_.cy);

    return sz;
}


void UiButton::SetMinSize(Size sz)
{
    user_min_size_ = sz;
    RefreshLayout();
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
        ReleaseCapture(); // only if we were actually in pressed / captured state
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

    if(was_pressed && IsEnabled() && Rect(GetSize()).Contains(p)) {
        WhenPush();
        WhenAction();
    }
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
    // We are being asked to cancel current interaction (e.g. mouse capture lost).
    // DO NOT call ReleaseCapture() here ... will go recursive, an da shit we dont want
    
    if(pressed_ || mouse_over_) {
        pressed_    = false;
        mouse_over_ = false;
        UpdateVisualState();
        Refresh();
    }

    // well...Optional: call base version (currently Ctrl::CancelMode() is basically empty,
    // but this documents intent and keeps future compatibility)...(will see if anyone uses this )
    Ctrl::CancelMode();
}

bool UiButton::Key(dword key, int count)
{
    if(!IsEnabled())
        return false;

    switch(key) {
    case K_SPACE:
    case K_ENTER:
        // Simple, predictable behaviour: fire a click. :)
        WhenPush();
        WhenAction();
        return true;

    default:
        break;
    }

    // Do NOT intercept Tab / arrows / etc. – let Ctrl handle focus traversal.
    return Ctrl::Key(key, count);
}


// ------------------------------------------------------------
// Paint (follows our spec painting order, and 9-slice
// ------------------------------------------------------------
void UiButton::Paint(Draw& w)
{
    Rect r = GetSize();

    UpdateVisualState();
    StyledState st = visual_state_;

    StyledPalette& p = style_.palette;
    StyledMetrics& m = style_.metrics;
    StyledSkin&    s = style_.skin;

    Rect outer = r;

    // 1) Background hook
    if(WhenPaintBackground)
        WhenPaintBackground(w, outer, st);

    // 2) Skin (9-slice) or Face
    bool skin_drawn = false;
    if(s.enabled && !IsNull(s.base)) {
        if(s.inset.IsEmpty()) {
            // No inset => simple stretch
            w.DrawImage(outer, s.base);
        }
        else {
            // Use UiDraw9Slice (from UiStyle / Draw9Slice helper)
            UiDraw9Slice(w,
                         outer,
                         s.base,
                         s.inset.TopLeft(),
                         s.inset.BottomRight());
        }
        skin_drawn = true;
    }

    // 3) Face + Frame via shared helper, respecting skin's overrides
    StyledMetrics mm = m; // local copy so we can tweak flags

    if(skin_drawn) {
        mm.face_enabled = false;             // skin owns the background
        if(s.includes_frame)
            mm.frame_enabled = false;        // skin also owns the frame
    }

    UiPaintFaceFrameDash(w, outer, p, mm, st);

    // 4) Content (label + optional image)
    Rect content = outer;

    // Keep content inside padding + frame width, so icons/text never touch the frame.
    int fw = max(m.frame_width, 0);
    content.Deflate(style_.padding_h + fw,
                    style_.padding_v + fw);
    if(content.IsEmpty())
        return;

    Font f = m.use_text_font ? m.text_font : style_.font;
    if(IsNull(f))
        f = StdFont();

    String txt = label_;
    Size   text_sz(0, 0);
    if(!txt.IsEmpty())
        text_sz = GetTextSize(txt, f);

    Size img_sz(0, 0);
    if(!IsNull(image_))
        img_sz = image_.GetSize();

    int gap = (!IsNull(image_) && !txt.IsEmpty()) ? style_.image_gap : 0;

    // Block height: vertical centering is always done for the combined block
    int block_h = 0;
    switch(style_.image_layout) {
    case UIIMAGE_TOP:
    case UIIMAGE_BOTTOM:
        block_h = img_sz.cy + gap + text_sz.cy;
        break;
    case UIIMAGE_LEFT:
    case UIIMAGE_RIGHT:
    default:
        block_h = max(img_sz.cy, text_sz.cy);
        break;
    }

    Point offset = (st == ST_PRESSED ? style_.press_offset : Point(0, 0));

    // Vertically center the whole content block in 'content'
    int block_y0 = content.top + (content.GetHeight() - block_h) / 2 + offset.y;
    int cx       = content.CenterPoint().x; // used for top/bottom centering

    Color ink          = p.ink[st];
    bool  enabled      = IsEnabled();
    bool  hot_or_press = (st == ST_HOT || st == ST_PRESSED);

    // Web-like link behavior: visited links use a distinct color.
    // We only apply this to "linky" styles (underline on, no face/frame).
    if(visited_ && style_.underline && !style_.metrics.face_enabled ) {
        // A purple-ish "visited link" tone (similar to HTML default #551A8B)
        ink = Color(85, 26, 139);
    }
    
    int text_x = 0;
    int text_y = 0;
    int img_x  = 0;
    int img_y  = 0;

    // Extra inset so the icon's own border does not kiss the button frame
    const int icon_inset = DPI(1);

    // Compute text/image positions based on layout
    if(IsNull(image_)) {
        // No image -> just center text in content
        if(!txt.IsEmpty()) {
            int ty = content.top + (content.GetHeight() - text_sz.cy) / 2 + offset.y;
            int tx = content.left + (content.GetWidth() - text_sz.cx) / 2 + offset.x;
            text_x = tx;
            text_y = ty;
        }
    }
    else {
        switch(style_.image_layout) {
        case UIIMAGE_LEFT: {
            // Icon anchored to left padding; text flows right from it.
            img_x = content.left + offset.x + icon_inset;
            img_y = block_y0 + (block_h - img_sz.cy) / 2 + icon_inset;

            text_x = img_x + img_sz.cx + gap;
            text_y = block_y0 + (block_h - text_sz.cy) / 2;
            break;
        }
        case UIIMAGE_RIGHT: {
            // Icon anchored to right padding; text flows left towards it.
            img_x = content.right - img_sz.cx + offset.x - icon_inset;
            img_y = block_y0 + (block_h - img_sz.cy) / 2 + icon_inset;

            text_x = img_x - gap - text_sz.cx;
            text_y = block_y0 + (block_h - text_sz.cy) / 2;
            break;
        }
        case UIIMAGE_TOP: {
            // Icon + text block centered horizontally
            int block_w  = max(img_sz.cx, text_sz.cx);
            int block_x0 = content.left + (content.GetWidth() - block_w) / 2 + offset.x;

            img_x = block_x0 + (block_w - img_sz.cx) / 2;
            img_y = block_y0 + icon_inset;

            text_x = block_x0 + (block_w - text_sz.cx) / 2;
            text_y = img_y + img_sz.cy + gap;
            break;
        }
        case UIIMAGE_BOTTOM: {
            int block_w  = max(img_sz.cx, text_sz.cx);
            int block_x0 = content.left + (content.GetWidth() - block_w) / 2 + offset.x;

            text_x = block_x0 + (block_w - text_sz.cx) / 2;
            text_y = block_y0;

            img_x = block_x0 + (block_w - img_sz.cx) / 2;
            img_y = text_y + text_sz.cy + gap + icon_inset;
            break;
        }
        default:
            break;
        }
    }

    // Draw image (if any)
    if(!IsNull(image_)) {
        if(!enabled) {
            // Disabled glyph
            w.DrawImage(img_x, img_y, DisabledImage(image_, true));
        }
        else if(mono_image_) {
            // Monochrome: tint with current ink (state + theme aware)
            w.DrawImage(img_x, img_y, image_, ink);
        }
        else if(hot_or_press) {
            // Hover/pressed highlight
            DrawHighlightImage(w, img_x, img_y, image_, true, true);
        }
        else {
            w.DrawImage(img_x, img_y, image_);
        }
    }

    // Draw text (if any)
    if(!txt.IsEmpty()) {
        int ty = text_y;
        w.DrawText(text_x, ty, txt, f, ink);

        if(style_.underline) {
            // Baseline under the text
            int baseline = ty + text_sz.cy + style_.underline_offset;
            
            // ---- Thickness & color -----------------------------------------
            int   underline_h = max(style_.underline_width, DPI(1));
            Color line_ink    = ink;

            // ---- Horizontal extension (stacking focus + hover + pressed) ----
            int base_extend = DPI(3);   // “unit” extension per side

            int extend = 0;
			if(HasFocus()) {
				extend += base_extend;      // focused: +3
				line_ink = DkColor(line_ink, 11);
			}
            if(st == ST_PRESSED) {
                extend += base_extend;      // pressed: +3
                underline_h += 2;           // pressed: thicker
                line_ink    = DkColor(line_ink, 11);          // pressed: slightly lighter ink
            }
            if(st == ST_HOT) {
                extend += base_extend;      // hover: +3
                line_ink    = LtColor(line_ink, 12); 
            }
        
            // Start/end x (clamped to content so we don't hit the frame)
            int left  = text_x - extend;
            int right = text_x + text_sz.cx + extend;

            left  = max(left, content.left);
            right = min(right, content.right);

            if(right > left)
                w.DrawRect(left, baseline, right - left, underline_h, line_ink);
        }
    }



    // 5) Focus outline – rounded & AA via UiPaintFocusRing
    if(HasFocus() && style_.focus_margin > 0) {
        UiPaintFocusRing(w,
                         outer,
                         style_.palette,
                         style_.metrics,
                         st,
                         style_.focus_margin,
                         SColorHighlight()); // overide but could use Null to use palette.ink[st]
    }



    // 6) Foreground hook
    if(WhenPaintForeground)
        WhenPaintForeground(w, outer, st);
}


// ------------------------------------------------------------
// Accessibility & access keys
// ------------------------------------------------------------

static dword UiButtonAccessKeyMask(wchar chr)
{ 
    chr = ToUpper(chr);
    if(chr >= 'A' && chr <= 'Z')
        return 1u << (chr - 'A');
    return 0;
}

String UiButton::GetDesc() const
{
    if(!label_.IsEmpty())
        return label_;
    return t_("Button");
}


dword UiButton::GetAccessKeys() const
{
    // Mirror CtrlLib::Pusher: single access key → single bit mask
    return AccessKeyBit(accesskey_);
}

void UiButton::AssignAccessKeys(dword used)
{
    // If label already provided an explicit '&X', just mark it as used.
    if(accesskey_) {
        used |= AccessKeyBit(accesskey_);
        Ctrl::AssignAccessKeys(used);
        return;
    }

    // No explicit '&' → auto-choose first free key from label text.
    // This approximates ChooseAccessKey(label, used) using AccessKeyBit.
    WString w = label_.ToWString();
    for(int i = 0; i < w.GetCount(); i++) {
        wchar c = w[i];
        dword bit = AccessKeyBit(c);
        if(bit && !(used & bit)) {
            accesskey_ = ToUpper(c);
            used |= bit;
            Refresh();
            break;
        }
    }

    Ctrl::AssignAccessKeys(used);
}


} // namespace Upp
