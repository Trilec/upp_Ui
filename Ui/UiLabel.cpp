#include <Ui/UiLabel.h>
#include <Ui/UiTheme.h>

namespace Upp {

namespace {

struct RichPiece : Moveable<RichPiece> {
    UiLabel::Span::Kind kind = UiLabel::Span::TEXT;
    String text;
    Image icon;
    UiIconRenderMode icon_render_mode = UiIconRenderMode::PreserveColor;
    Color ink;
    Color bg;
    Font font;
    bool underline = false;
    int bullet_size = DPI(6);
    Color bullet_color;
    Size sz = Size(0, 0);
};

static Font ResolveSpanFont(const UiLabel::Span& s, Font base)
{
    Font f = IsNull(s.font) ? base : s.font;
    if(s.bold)
        f = f.Bold();
    if(s.italic)
        f = f.Italic();
    return f;
}

static void BuildRichLines(const Vector<UiLabel::Span>& spans,
                           Font base_font,
                           Color base_ink,
                           Vector<Vector<RichPiece>>& lines,
                           Vector<Size>& line_sizes)
{
    lines.Clear();
    line_sizes.Clear();
    lines.Add();

    for(const UiLabel::Span& s : spans) {
        if(s.kind == UiLabel::Span::NEWLINE) {
            lines.Add();
            continue;
        }

        if(s.kind == UiLabel::Span::ICON) {
            if(IsNull(s.icon))
                continue;
            RichPiece p;
            p.kind = s.kind;
            p.icon = s.icon;
            p.icon_render_mode = s.icon_render_mode;
            p.ink = IsNull(s.ink) ? base_ink : s.ink;
            p.sz = s.icon.GetSize();
            lines.Top().Add(p);
            continue;
        }

        if(s.kind == UiLabel::Span::BULLET) {
            RichPiece p;
            p.kind = s.kind;
            p.bullet_size = max(2, s.bullet_size);
            p.bullet_color = s.bullet_color;
            p.sz = Size(p.bullet_size, p.bullet_size);
            lines.Top().Add(p);
            continue;
        }

        String txt = s.text;
        int start = 0;
        while(start <= txt.GetCount()) {
            int nl = txt.Find('\n', start);
            String seg = (nl < 0) ? txt.Mid(start) : txt.Mid(start, nl - start);

            if(!seg.IsEmpty()) {
                RichPiece p;
                p.kind = UiLabel::Span::TEXT;
                p.text = seg;
                p.ink = IsNull(s.ink) ? base_ink : s.ink;
                p.bg = s.bg;
                p.font = ResolveSpanFont(s, base_font);
                p.underline = s.underline;
                p.sz = GetTextSize(seg, p.font);
                lines.Top().Add(p);
            }

            if(nl < 0)
                break;

            lines.Add();
            start = nl + 1;
        }
    }

    if(lines.IsEmpty())
        lines.Add();

    line_sizes.SetCount(lines.GetCount());
    for(int i = 0; i < lines.GetCount(); i++) {
        int w = 0;
        int h = max(1, base_font.GetCy());
        for(const RichPiece& p : lines[i]) {
            w += p.sz.cx;
            h = max(h, p.sz.cy);
        }
        line_sizes[i] = Size(w, h);
    }
}

static Vector<int> BuildCaretAdvances(const String& text, Font font)
{
    Vector<int> x;
    x.SetCount(text.GetCount() + 1);
    for(int i = 0; i <= text.GetCount(); i++)
        x[i] = GetTextSize(text.Left(i), font).cx;
    return x;
}

static Color AnsiColor(int code)
{
    switch(code) {
    case 30: return Color(0, 0, 0);
    case 31: return Color(220, 50, 47);
    case 32: return Color(38, 166, 91);
    case 33: return Color(234, 179, 8);
    case 34: return Color(59, 130, 246);
    case 35: return Color(168, 85, 247);
    case 36: return Color(20, 184, 166);
    case 37: return Color(230, 230, 230);
    case 90: return Color(120, 120, 120);
    case 91: return Color(248, 113, 113);
    case 92: return Color(74, 222, 128);
    case 93: return Color(250, 204, 21);
    case 94: return Color(96, 165, 250);
    case 95: return Color(216, 180, 254);
    case 96: return Color(45, 212, 191);
    case 97: return Color(255, 255, 255);
    default: return Null;
    }
}

}

const UiLabel::Style& UiLabel::StyleDefault()
{
    static Style s;
    ONCELOCK {
        const Color text_primary = Color(17, 24, 39);
        const Color text_muted = Color(107, 114, 128);
        const Color border_soft = Color(226, 232, 240);

        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::None();
            s.palette.frame[i] = border_soft;
            s.palette.ink[i] = text_primary;
            s.palette.icon[i] = Null;
        }
        s.palette.ink[ST_DISABLED] = text_muted;
        s.palette.icon[ST_NORMAL] = text_muted;
        s.palette.icon[ST_HOT] = text_primary;
        s.palette.icon[ST_PRESSED] = text_primary;
        s.palette.icon[ST_DISABLED] = Blend(text_muted, SColorPaper(), 60);

        s.metrics.text_font = StdFont();
        s.metrics.use_text_font = false;
        s.metrics.content_margin = Rect(0, 0, 0, 0);
        s.metrics.radius = 0;
        s.metrics.frame_width = 0;
        s.metrics.frame_enabled = false;
        s.metrics.face_enabled = false;
        s.metrics.dashed = false;
        s.metrics.high_contrast = false;
        s.metrics.shadow = StyledShadow();
        s.metrics.highlight = StyledHighlight();

        s.skin = StyledSkin();
        s.align_h = UiAlign::LEFT;
        s.align_v = UiAlign::CENTER;
        s.icon_side = UiAlign::LEFT;
        s.content_gap = DPI(6);
        s.font = StdFont();
        s.transparent = true;
        s.underline = false;
        s.underline_width = DPI(1);
        s.underline_offset = 0;
        s.nowrap = false;
    }
    return s;
}

UiLabel::UiLabel()
    : style_(StyleDefault())
    , themed_style_(StyleDefault())
{
    Transparent();
    NoWantFocus();

    SyncThemeStyle();
    RebuildTextLinesFromStyle(GetEffectiveStyle());
    minsize_dirty_ = true;
    layout_dirty_ = true;
    layout_content_ = Rect(0, 0, 0, 0);
}

void UiLabel::InvalidateStyleCache()
{
    theme_revision_ = 0;
    minsize_dirty_ = true;
    layout_dirty_ = true;
    layout_content_ = Rect(0, 0, 0, 0);
}

UiLabel::Style& UiLabel::StyleEdit()
{
    if(!has_style_override_) {
        style_ = GetEffectiveStyle();
        has_style_override_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

void UiLabel::SyncThemeStyle()
{
    if(has_style_override_)
        return;

    const uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;

    themed_style_ = UiTheme::ResolveLabel();
    theme_revision_ = revision;
    RebuildTextLinesFromStyle(themed_style_);
    minsize_dirty_ = true;
    layout_dirty_ = true;
    layout_content_ = Rect(0, 0, 0, 0);
}

UiLabel& UiLabel::SetStyle(const Style& s)
{
    style_ = s;
    has_style_override_ = true;
    OnStyleChanged();
    return *this;
}

UiLabel& UiLabel::ClearStyleOverride()
{
    if(!has_style_override_)
        return *this;

    has_style_override_ = false;
    style_ = StyleDefault();
    InvalidateStyleCache();
    OnStyleChanged();
    return *this;
}

const UiLabel::Style& UiLabel::GetEffectiveStyle() const
{
    if(has_style_override_)
        return style_;

    const_cast<UiLabel*>(this)->SyncThemeStyle();
    return themed_style_;
}

void UiLabel::OnStyleChanged()
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

void UiLabel::RebuildTextLines()
{
    RebuildTextLinesFromStyle(GetEffectiveStyle());
}

void UiLabel::RebuildTextLinesFromStyle(const Style& style)
{
    lines_.Clear();
    line_sizes_.Clear();

    if(text_.IsEmpty())
        return;

    Font fnt = style.metrics.use_text_font ? style.metrics.text_font : style.font;
    if(IsNull(fnt))
        fnt = StdFont();

    if(style.nowrap) {
        String one = text_;
        one.Replace("\r", "");
        one.Replace("\n", " ");
        lines_.Add(one);
        line_sizes_.Add(one.IsEmpty() ? Size(0, GetTextSize(" ", fnt).cy) : GetTextSize(one, fnt));
        return;
    }

    UiBuildStyledTextLines(text_, fnt, lines_, line_sizes_);
}

String UiLabel::BuildPlainTextFromSpans() const
{
    String out;
    for(const Span& s : spans_) {
        switch(s.kind) {
        case Span::TEXT:
            out << s.text;
            break;
        case Span::NEWLINE:
            out.Cat('\n');
            break;
        case Span::BULLET:
            out << "* ";
            break;
        case Span::ICON:
        default:
            break;
        }
    }
    return out;
}

Size UiLabel::GetStableIconSize() const
{
    // Explicit icon sizing is the label contract now; a zero size simply
    // falls back to the source image size so layout and paint stay aligned.
    if(icon_size_.cx > 0 && icon_size_.cy > 0)
        return icon_size_;

    return IsNull(icon_) ? Size(0, 0) : icon_.GetSize();
}

bool UiLabel::HasSelection() const
{
    return selectable_text_ && sel_anchor_ >= 0 && sel_caret_ >= 0 && sel_anchor_ != sel_caret_;
}

int UiLabel::SelFrom() const
{
    return min(sel_anchor_, sel_caret_);
}

int UiLabel::SelTo() const
{
    return max(sel_anchor_, sel_caret_);
}

int UiLabel::HitTestTextPos(Point p) const
{
    const Style& style = GetEffectiveStyle();

    if(text_.IsEmpty())
        return 0;

    if(layout_dirty_)
        const_cast<UiLabel*>(this)->Layout();

    Rect text_r = layout_.main;
    if(text_r.IsEmpty())
        return 0;

    Font fnt = style.metrics.use_text_font ? style.metrics.text_font : style.font;
    if(IsNull(fnt))
        fnt = StdFont();

    const int gap = UiStyledTextLineGap();

    if(rich_enabled_ && !spans_.IsEmpty()) {
        Vector<Vector<RichPiece>> rich_lines;
        Vector<Size> rich_line_sizes;
        BuildRichLines(spans_, fnt, style.palette.ink[ST_NORMAL], rich_lines, rich_line_sizes);

        const int count = rich_lines.GetCount();
        if(count <= 0)
            return 0;

        int total_h = 0;
        for(int i = 0; i < count; i++)
            total_h += rich_line_sizes[i].cy;
        if(count > 1)
            total_h += gap * (count - 1);

        int start_y;
        switch(style.align_v) {
        case UiAlign::BOTTOM:
            start_y = text_r.bottom - total_h;
            break;
        case UiAlign::CENTER:
            start_y = text_r.top + (text_r.GetHeight() - total_h) / 2;
            break;
        case UiAlign::TOP:
        default:
            start_y = text_r.top;
            break;
        }

        int y = start_y;
        int text_ofs = 0;
        int best_pos = text_.GetCount();

        for(int i = 0; i < count; i++) {
            const Vector<RichPiece>& line = rich_lines[i];
            const Size& sz = rich_line_sizes[i];

            int line_x;
            switch(style.align_h) {
            case UiAlign::CENTER:
                line_x = text_r.left + (text_r.GetWidth() - sz.cx) / 2;
                break;
            case UiAlign::RIGHT:
                line_x = text_r.right - sz.cx;
                break;
            case UiAlign::LEFT:
            default:
                line_x = text_r.left;
                break;
            }

            int line_top = y;
            int line_bottom = y + sz.cy;
            int line_text_len = 0;
            for(const RichPiece& rp : line)
                if(rp.kind == Span::TEXT)
                    line_text_len += rp.text.GetCount();

            if(p.y < line_top)
                return text_ofs;

            if(p.y <= line_bottom || i + 1 == count) {
                if(p.x <= line_x)
                    return text_ofs;
                if(p.x >= line_x + sz.cx)
                    return text_ofs + line_text_len;

                int x = line_x;
                for(const RichPiece& rp : line) {
                    int next_x = x + rp.sz.cx;
                    if(p.x <= next_x) {
                        if(rp.kind != Span::TEXT || rp.text.IsEmpty())
                            return text_ofs;

                        int relx = max(0, p.x - x);
                        Vector<int> caret_x = BuildCaretAdvances(rp.text, rp.font);
                        for(int k = 1; k <= rp.text.GetCount(); k++) {
                            int w = caret_x[k];
                            if(w >= relx) {
                                int prev = caret_x[k - 1];
                                int pos = (relx - prev <= w - relx) ? (k - 1) : k;
                                return text_ofs + pos;
                            }
                        }
                        return text_ofs + rp.text.GetCount();
                    }

                    if(rp.kind == Span::TEXT)
                        text_ofs += rp.text.GetCount();
                    x = next_x;
                }
                return text_ofs;
            }

            best_pos = text_ofs + line_text_len;
            text_ofs += line_text_len;
            if(i + 1 < count)
                text_ofs += 1;
            y = line_bottom + gap;
        }

        return best_pos;
    }

    const int count = lines_.GetCount();
    if(count <= 0)
        return 0;

    int total_h = 0;
    for(int i = 0; i < count; i++)
        total_h += line_sizes_[i].cy;
    if(count > 1)
        total_h += gap * (count - 1);

    int start_y;
    switch(style.align_v) {
    case UiAlign::BOTTOM:
        start_y = text_r.bottom - total_h;
        break;
    case UiAlign::CENTER:
        start_y = text_r.top + (text_r.GetHeight() - total_h) / 2;
        break;
    case UiAlign::TOP:
    default:
        start_y = text_r.top;
        break;
    }

    int y = start_y;
    int text_ofs = 0;
    int best_pos = text_.GetCount();

    for(int i = 0; i < count; i++) {
        const String& line = lines_[i];
        const Size& sz = line_sizes_[i];

        int line_x;
        switch(style.align_h) {
        case UiAlign::CENTER:
            line_x = text_r.left + (text_r.GetWidth() - sz.cx) / 2;
            break;
        case UiAlign::RIGHT:
            line_x = text_r.right - sz.cx;
            break;
        case UiAlign::LEFT:
        default:
            line_x = text_r.left;
            break;
        }

        int line_top = y;
        int line_bottom = y + sz.cy;
        int line_len = line.GetCount();
        Vector<int> caret_x = BuildCaretAdvances(line, fnt);
        int line_w = caret_x.IsEmpty() ? 0 : caret_x.Top();

        if(p.y < line_top)
            return text_ofs;

        if(p.y <= line_bottom || i + 1 == count) {
            if(p.x <= line_x)
                return text_ofs;
            if(p.x >= line_x + line_w)
                return text_ofs + line_len;
            int relx = p.x - line_x;
            for(int k = 1; k <= line_len; k++) {
                int w = caret_x[k];
                if(w >= relx) {
                    int prev = caret_x[k - 1];
                    int pos = (relx - prev <= w - relx) ? (k - 1) : k;
                    return text_ofs + pos;
                }
            }
            return text_ofs + line_len;
        }

        best_pos = text_ofs + line_len;
        text_ofs += line_len;
        if(i + 1 < count)
            text_ofs += 1;
        y = line_bottom + gap;
    }

    return best_pos;
}
Size UiLabel::GetTextBlockSize() const
{
    const Style& style = GetEffectiveStyle();

    if(rich_enabled_ && !spans_.IsEmpty()) {
        Font fnt = style.metrics.use_text_font ? style.metrics.text_font : style.font;
        if(IsNull(fnt))
            fnt = StdFont();
        Vector<Vector<RichPiece>> rl;
        Vector<Size> ls;
        BuildRichLines(spans_, fnt, style.palette.ink[ST_NORMAL], rl, ls);
        return UiMeasureStyledTextBlock(ls);
    }
    return UiMeasureStyledTextBlock(line_sizes_);
}

Size UiLabel::ComputeNaturalSize() const
{
    const Style& style = GetEffectiveStyle();

    bool have_text = !lines_.IsEmpty();
    Size text_block = have_text ? GetTextBlockSize() : Size(0, 0);

    bool have_icon = !IsNull(icon_);
    Size icon_sz = have_icon ? GetStableIconSize() : Size(0, 0);
    const bool explicit_icon_size = icon_size_.cx > 0 && icon_size_.cy > 0;

    Size content = UiMeasureBlocksContent(icon_sz,
                                          text_block,
                                          style.icon_side,
                                          have_icon,
                                          have_text,
                                          DPI(16),
                                          DPI(8),
                                          explicit_icon_size ? 0 : DPI(16),
                                          have_icon && have_text ? style.content_gap : 0);

    return UiStyledOuterSizeFromContent(content, style.metrics, style.skin);
}

void UiLabel::UpdateLayout(const Rect& content) const
{
    const Style& style = GetEffectiveStyle();

    layout_ = UiBlocksLayout();
    if(content.IsEmpty()) {
        layout_dirty_ = false;
        return;
    }

    bool have_text = !lines_.IsEmpty();
    Size text_block = have_text ? GetTextBlockSize() : Size(0, 0);
    bool have_icon = !IsNull(icon_);
    Size icon_sz = have_icon ? GetStableIconSize() : Size(0, 0);
    const bool explicit_icon_size = icon_size_.cx > 0 && icon_size_.cy > 0;

    layout_ = UiComputeBlocksLayout(content,
                                    have_icon ? icon_sz : Size(0, 0),
                                    have_text ? text_block : Size(0, 0),
                                    style.align_h,
                                    style.align_v,
                                    style.icon_side,
                                    explicit_icon_size ? 0 : DPI(16),
                                    have_icon && have_text ? style.content_gap : 0);

    layout_dirty_ = false;
}
UiLabel& UiLabel::SetText(const String& text)
{
    rich_enabled_ = false;
    spans_.Clear();

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
    if(text_.IsEmpty()) {
        sel_anchor_ = -1;
        sel_caret_ = -1;
    }
    minsize_dirty_ = true;
    layout_dirty_ = true;
    layout_content_ = Rect(0, 0, 0, 0);
    RefreshLayout();
    Refresh();
    return *this;
}

UiLabel& UiLabel::SetSelectable(bool on)
{
    selectable_text_ = on;
    if(on)
        WantFocus();
    else {
        NoWantFocus();
        sel_anchor_ = -1;
        sel_caret_ = -1;
        selecting_drag_ = false;
    }
    Refresh();
    return *this;
}

UiLabel& UiLabel::SetIconSize(Size sz)
{
    icon_size_ = Size(max(0, sz.cx), max(0, sz.cy));
    minsize_dirty_ = true;
    layout_dirty_ = true;
    layout_content_ = Rect(0, 0, 0, 0);
    RefreshLayout();
    Refresh();
    return *this;
}

UiLabel& UiLabel::EnableRich(bool on)
{
    rich_enabled_ = on;
    text_ = rich_enabled_ ? BuildPlainTextFromSpans() : text_;
    RebuildTextLinesFromStyle(GetEffectiveStyle());
    minsize_dirty_ = true;
    layout_dirty_ = true;
    layout_content_ = Rect(0, 0, 0, 0);
    RefreshLayout();
    Refresh();
    return *this;
}

UiLabel& UiLabel::SetSpans(const Vector<Span>& spans)
{
    spans_ = clone(spans);
    rich_enabled_ = true;
    text_ = BuildPlainTextFromSpans();
    RebuildTextLinesFromStyle(GetEffectiveStyle());
    minsize_dirty_ = true;
    layout_dirty_ = true;
    layout_content_ = Rect(0, 0, 0, 0);
    RefreshLayout();
    Refresh();
    return *this;
}

UiLabel& UiLabel::ClearSpans()
{
    spans_.Clear();
    rich_enabled_ = false;
    text_.Clear();
    RebuildTextLinesFromStyle(GetEffectiveStyle());
    minsize_dirty_ = true;
    layout_dirty_ = true;
    layout_content_ = Rect(0, 0, 0, 0);
    RefreshLayout();
    Refresh();
    return *this;
}

UiLabel& UiLabel::AddSpan(const Span& span)
{
    spans_.Add(span);
    rich_enabled_ = true;
    text_ = BuildPlainTextFromSpans();
    RebuildTextLinesFromStyle(GetEffectiveStyle());
    minsize_dirty_ = true;
    layout_dirty_ = true;
    layout_content_ = Rect(0, 0, 0, 0);
    RefreshLayout();
    Refresh();
    return *this;
}

UiLabel& UiLabel::SetSpan(int i, const Span& span)
{
    if(i < 0 || i >= spans_.GetCount())
        return *this;
    spans_[i] = span;
    rich_enabled_ = true;
    text_ = BuildPlainTextFromSpans();
    RebuildTextLinesFromStyle(GetEffectiveStyle());
    minsize_dirty_ = true;
    layout_dirty_ = true;
    layout_content_ = Rect(0, 0, 0, 0);
    RefreshLayout();
    Refresh();
    return *this;
}

UiLabel& UiLabel::AddTextSpan(const String& text, Color ink, bool bold, bool italic, bool underline)
{
    Span s;
    s.kind = Span::TEXT;
    s.text = text;
    s.ink = ink;
    s.bold = bold;
    s.italic = italic;
    s.underline = underline;
    return AddSpan(s);
}

UiLabel& UiLabel::AddIconSpan(const Image& icon, UiIconRenderMode render_mode)
{
    Span s;
    s.kind = Span::ICON;
    s.icon = icon;
    s.icon_render_mode = render_mode;
    return AddSpan(s);
}

UiLabel& UiLabel::AddBulletSpan(Color color, int size)
{
    Span s;
    s.kind = Span::BULLET;
    s.bullet_color = color;
    s.bullet_size = size;
    return AddSpan(s);
}

UiLabel& UiLabel::AddNewlineSpan()
{
    Span s;
    s.kind = Span::NEWLINE;
    return AddSpan(s);
}

UiLabel& UiLabel::SetAnsiText(const String& ansi)
{
    spans_.Clear();
    rich_enabled_ = true;
    return AppendAnsiText(ansi);
}

UiLabel& UiLabel::AppendAnsiText(const String& ansi)
{
    struct State {
        Color ink;
        bool bold = false;
        bool italic = false;
        bool underline = false;
    } st;

    auto emit_text = [&](const String& chunk) {
        if(chunk.IsEmpty())
            return;
        Span s;
        s.kind = Span::TEXT;
        s.text = chunk;
        s.ink = st.ink;
        s.bold = st.bold;
        s.italic = st.italic;
        s.underline = st.underline;
        spans_.Add(s);
    };

    String chunk;
    for(int i = 0; i < ansi.GetCount(); i++) {
        int c = (byte)ansi[i];
        if(c == '\n') {
            emit_text(chunk);
            chunk.Clear();
            Span nl;
            nl.kind = Span::NEWLINE;
            spans_.Add(nl);
            continue;
        }

        if(c == 0x1b && i + 1 < ansi.GetCount() && ansi[i + 1] == '[') {
            emit_text(chunk);
            chunk.Clear();

            i += 2;
            Vector<int> codes;
            int acc = 0;
            bool have = false;
            while(i < ansi.GetCount()) {
                int cc = (byte)ansi[i];
                if(cc >= '0' && cc <= '9') {
                    acc = acc * 10 + (cc - '0');
                    have = true;
                }
                else if(cc == ';') {
                    codes.Add(have ? acc : 0);
                    acc = 0;
                    have = false;
                }
                else if(cc == 'm') {
                    codes.Add(have ? acc : 0);
                    break;
                }
                else {
                    break;
                }
                i++;
            }

            for(int k = 0; k < codes.GetCount(); k++) {
                int code = codes[k];
                if(code == 0) {
                    st = State();
                }
                else if(code == 1) st.bold = true;
                else if(code == 3) st.italic = true;
                else if(code == 4) st.underline = true;
                else if(code == 22) st.bold = false;
                else if(code == 23) st.italic = false;
                else if(code == 24) st.underline = false;
                else if(code == 39) st.ink = Null;
                else if((code >= 30 && code <= 37) || (code >= 90 && code <= 97)) {
                    st.ink = AnsiColor(code);
                }
                else if(code == 38 && k + 4 < codes.GetCount() && codes[k + 1] == 2) {
                    st.ink = Color(min(max(codes[k + 2], 0), 255),
                                   min(max(codes[k + 3], 0), 255),
                                   min(max(codes[k + 4], 0), 255));
                    k += 4;
                }
            }
            continue;
        }

        chunk.Cat((char)c);
    }
    emit_text(chunk);

    rich_enabled_ = true;
    text_ = BuildPlainTextFromSpans();
    RebuildTextLinesFromStyle(GetEffectiveStyle());
    minsize_dirty_ = true;
    layout_dirty_ = true;
    layout_content_ = Rect(0, 0, 0, 0);
    RefreshLayout();
    Refresh();
    return *this;
}

void UiLabel::CopySelectionToClipboard() const
{
    if(!HasSelection() || text_.IsEmpty())
        return;
    int from = SelFrom();
    int to = SelTo();
    String sel = text_.Mid(from, to - from);
    WString w = sel.ToWString();
    ClearClipboard();
    AppendClipboardUnicodeText(w);
    AppendClipboardText(sel);
}

UiLabel& UiLabel::SetIcon(const Image& img)
{
    icon_ = img;
    icon_render_mode_ = UiIconRenderMode::PreserveColor;
    minsize_dirty_ = true;
    layout_dirty_ = true;
    layout_content_ = Rect(0, 0, 0, 0);
    RefreshLayout();
    Refresh();
    return *this;
}

UiLabel& UiLabel::SetIcon(const Image& img, UiIconRenderMode render_mode)
{
    icon_ = img;
    icon_render_mode_ = render_mode;
    minsize_dirty_ = true;
    layout_dirty_ = true;
    layout_content_ = Rect(0, 0, 0, 0);
    RefreshLayout();
    Refresh();
    return *this;
}

UiLabel& UiLabel::SetIconRenderMode(UiIconRenderMode render_mode)
{
    icon_render_mode_ = render_mode;
    Refresh();
    return *this;
}

UiLabel& UiLabel::ClearIcon()
{
    icon_ = Image();
    icon_render_mode_ = UiIconRenderMode::PreserveColor;
    minsize_dirty_ = true;
    layout_dirty_ = true;
    layout_content_ = Rect(0, 0, 0, 0);
    RefreshLayout();
    Refresh();
    return *this;
}

UiLabel& UiLabel::SetIconSide(UiAlign where)
{
    if(where != UiAlign::LEFT && where != UiAlign::RIGHT &&
       where != UiAlign::TOP  && where != UiAlign::BOTTOM)
        where = UiAlign::LEFT;
    StyleEdit().icon_side = where;
    OnStyleChanged();
    return *this;
}

UiLabel& UiLabel::SetAlign(UiAlign h, UiAlign v)
{
    Style& style = StyleEdit();
    style.align_h = h;
    style.align_v = v;
    OnStyleChanged();
    return *this;
}

UiLabel& UiLabel::SetAlignH(UiAlign h)
{
    StyleEdit().align_h = h;
    OnStyleChanged();
    return *this;
}

UiLabel& UiLabel::SetAlignV(UiAlign v)
{
    StyleEdit().align_v = v;
    OnStyleChanged();
    return *this;
}

UiLabel& UiLabel::SetContentGap(int gap)
{
    StyleEdit().content_gap = max(0, gap);
    OnStyleChanged();
    return *this;
}

UiLabel& UiLabel::SetUnderline(bool on, int thickness, int offset)
{
    Style& style = StyleEdit();
    style.underline = on;
    style.underline_width = max(thickness, 0);
    style.underline_offset = offset;
    OnStyleChanged();
    return *this;
}

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
    minsize_dirty_ = false;
    return cached_minsize_;
}

Size UiLabel::GetContentSize() const
{
    return ComputeNaturalSize();
}

void UiLabel::Layout()
{
    const Style& style = GetEffectiveStyle();
    Rect outer = GetSize();
    Rect content = UiStyledInnerRect(outer, style.metrics, style.skin);

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
    layout_dirty_ = true;
    layout_content_ = Rect(0, 0, 0, 0);
    RefreshLayout();
    return *this;
}
void UiLabel::Paint(Draw& w)
{
    const Style& style = GetEffectiveStyle();
    Rect outer = GetSize();
    if(outer.IsEmpty())
        return;

    const bool enabled = IsEnabled();
    const bool has_focus = HasFocus();
    const StyledState state = ResolveStyledState(enabled, false, false);

    const StyledPalette& p = style.palette;
    const StyledMetrics& m = style.metrics;
    const StyledSkin& s = style.skin;

    if(WhenPaintBackground)
        WhenPaintBackground(w, outer, p, m, s, state, has_focus);
    else
        UiPaintStyledBackground(w, outer, p, m, s, state, has_focus);

    Font fnt = m.use_text_font ? m.text_font : style.font;
    if(IsNull(fnt))
        fnt = StdFont();

    Color ink = p.ink[state];
    Color icon_ink = UiResolveIconColor(p, state);
    if(IsNull(icon_ink))
        icon_ink = ink;

    if(rich_enabled_ && !spans_.IsEmpty() && !layout_.main.IsEmpty()) {
        Vector<Vector<RichPiece>> rich_lines;
        Vector<Size> rich_line_sizes;
        BuildRichLines(spans_, fnt, ink, rich_lines, rich_line_sizes);
        bool draw_sel = HasSelection() && !text_.IsEmpty();
        int sf = draw_sel ? SelFrom() : -1;
        int st = draw_sel ? SelTo() : -1;

        int gap = UiStyledTextLineGap();
        int total_h = 0;
        for(int i = 0; i < rich_line_sizes.GetCount(); i++)
            total_h += rich_line_sizes[i].cy;
        if(rich_line_sizes.GetCount() > 1)
            total_h += gap * (rich_line_sizes.GetCount() - 1);

        int start_y;
        switch(style.align_v) {
        case UiAlign::BOTTOM:
            start_y = layout_.main.bottom - total_h;
            break;
        case UiAlign::CENTER:
            start_y = layout_.main.top + (layout_.main.GetHeight() - total_h) / 2;
            break;
        case UiAlign::TOP:
        default:
            start_y = layout_.main.top;
            break;
        }

        int y = start_y;
        int text_ofs = 0;
        for(int i = 0; i < rich_lines.GetCount(); i++) {
            const Vector<RichPiece>& ln = rich_lines[i];
            Size lsz = rich_line_sizes[i];

            int x;
            switch(style.align_h) {
            case UiAlign::CENTER: x = layout_.main.left + (layout_.main.GetWidth() - lsz.cx) / 2; break;
            case UiAlign::RIGHT:  x = layout_.main.right - lsz.cx; break;
            case UiAlign::LEFT:
            default: x = layout_.main.left; break;
            }

            for(const RichPiece& rp : ln) {
                Rect pr(x, y + (lsz.cy - rp.sz.cy) / 2, x + rp.sz.cx, y + (lsz.cy - rp.sz.cy) / 2 + rp.sz.cy);

                if(!IsNull(rp.bg))
                    w.DrawRect(pr, rp.bg);

                if(rp.kind == Span::TEXT) {
                    Color rk = IsNull(rp.ink) ? ink : rp.ink;
                    int piece_len = rp.text.GetCount();
                    if(draw_sel && piece_len > 0) {
                        int hs0 = max(sf, text_ofs);
                        int hs1 = min(st, text_ofs + piece_len);
                        if(hs0 < hs1) {
                            int a = hs0 - text_ofs;
                            int b = hs1 - text_ofs;
                            String pre = rp.text.Left(a);
                            String mid = rp.text.Mid(a, b - a);
                            String post = rp.text.Mid(b);
                            int x0 = pr.left;
                            int x1 = pr.right;
                            if(a > 0)
                                x0 = pr.left + GetTextSize(pre, rp.font).cx;
                            if(b < piece_len)
                                x1 = pr.left + GetTextSize(rp.text.Left(b), rp.font).cx;
                            if(x1 > x0)
                                w.DrawRect(x0, pr.top, x1 - x0, rp.sz.cy, SColorHighlight());

                            int tx = pr.left;
                            if(!pre.IsEmpty()) {
                                w.DrawText(tx, pr.top, pre, rp.font, rk);
                                tx += GetTextSize(pre, rp.font).cx;
                            }
                            if(!mid.IsEmpty()) {
                                w.DrawText(tx, pr.top, mid, rp.font, SColorHighlightText());
                                tx += GetTextSize(mid, rp.font).cx;
                            }
                            if(!post.IsEmpty())
                                w.DrawText(tx, pr.top, post, rp.font, rk);
                        }
                        else
                            w.DrawText(pr.left, pr.top, rp.text, rp.font, rk);
                    }
                    else
                        w.DrawText(pr.left, pr.top, rp.text, rp.font, rk);
                    if(rp.underline) {
                        int uy = pr.bottom - 1;
                        w.DrawRect(pr.left, uy, rp.sz.cx, 1, rk);
                    }
                    text_ofs += piece_len;
                }
                else if(rp.kind == Span::ICON) {
                    Color rk = IsNull(rp.ink) ? icon_ink : rp.ink;
                    UiPaintStyledIcon(w, pr, rp.icon, true, false, rp.icon_render_mode, rk, enabled);
                }
                else if(rp.kind == Span::BULLET) {
                    int d = max(2, min(pr.GetWidth(), pr.GetHeight()));
                    Rect br(pr.left, pr.top + (pr.GetHeight() - d) / 2, pr.left + d, pr.top + (pr.GetHeight() - d) / 2 + d);
                    w.DrawEllipse(br, IsNull(rp.bullet_color) ? ink : rp.bullet_color);
                }

                x += rp.sz.cx;
            }

            if(i + 1 < rich_lines.GetCount())
                text_ofs += 1;
            y += lsz.cy;
            if(i + 1 < rich_lines.GetCount())
                y += gap;
        }
    }
    else {
        if(!IsNull(icon_) && !layout_.support.IsEmpty()) {
            const bool explicit_icon_size = icon_size_.cx > 0 && icon_size_.cy > 0;
            UiPaintStyledIcon(w,
                              layout_.support,
                              icon_,
                              true,
                              !explicit_icon_size,
                              icon_render_mode_,
                              icon_ink,
                              enabled);
        }

        bool draw_sel = HasSelection() && !layout_.main.IsEmpty() && !text_.IsEmpty();

        if(!draw_sel && !lines_.IsEmpty() && !layout_.main.IsEmpty()) {
            UiPaintStyledText(w,
                              layout_.main,
                              lines_,
                              line_sizes_,
                              style.align_h,
                              style.align_v,
                              fnt,
                              ink,
                              has_access_mnemonic_ ? accesskey_ : 0,
                              style.underline,
                              style.underline_width,
                              style.underline_offset);
        }
        else if(draw_sel && !lines_.IsEmpty() && !layout_.main.IsEmpty()) {
            const int gap = UiStyledTextLineGap();
            int total_h = 0;
            for(int i = 0; i < lines_.GetCount(); i++)
                total_h += line_sizes_[i].cy;
            if(lines_.GetCount() > 1)
                total_h += gap * (lines_.GetCount() - 1);

            int start_y;
            switch(style.align_v) {
            case UiAlign::BOTTOM:
                start_y = layout_.main.bottom - total_h;
                break;
            case UiAlign::CENTER:
                start_y = layout_.main.top + (layout_.main.GetHeight() - total_h) / 2;
                break;
            case UiAlign::TOP:
            default:
                start_y = layout_.main.top;
                break;
            }

            int y = start_y;
            int text_ofs = 0;
            int sf = SelFrom();
            int st = SelTo();

            for(int i = 0; i < lines_.GetCount(); i++) {
                const String& line = lines_[i];
                const Size& sz = line_sizes_[i];
                int line_len = line.GetCount();

                int line_x;
                switch(style.align_h) {
                case UiAlign::CENTER:
                    line_x = layout_.main.left + (layout_.main.GetWidth() - sz.cx) / 2;
                    break;
                case UiAlign::RIGHT:
                    line_x = layout_.main.right - sz.cx;
                    break;
                case UiAlign::LEFT:
                default:
                    line_x = layout_.main.left;
                    break;
                }

                int lf = text_ofs;
                int lt = text_ofs + line_len;
                int hs0 = max(sf, lf);
                int hs1 = min(st, lt);
                Vector<int> caret_x = BuildCaretAdvances(line, fnt);
                if(hs0 < hs1) {
                    int a = hs0 - lf;
                    int b = hs1 - lf;
                    int x0 = line_x + caret_x[a];
                    int x1 = line_x + caret_x[b];
                    if(x1 > x0)
                        w.DrawRect(x0, y, x1 - x0, sz.cy, SColorHighlight());
                    String pre = line.Left(a);
                    String mid = line.Mid(a, b - a);
                    String post = line.Mid(b);
                    int x = line_x;
                    if(!pre.IsEmpty()) {
                        w.DrawText(x, y, pre, fnt, ink);
                        x += GetTextSize(pre, fnt).cx;
                    }
                    if(!mid.IsEmpty()) {
                        w.DrawText(x, y, mid, fnt, SColorHighlightText());
                        x += GetTextSize(mid, fnt).cx;
                    }
                    if(!post.IsEmpty())
                        w.DrawText(x, y, post, fnt, ink);
                }
                else if(!line.IsEmpty()) {
                    w.DrawText(line_x, y, line, fnt, ink);
                }

                text_ofs += line_len;
                if(i + 1 < lines_.GetCount())
                    text_ofs += 1;
                y += sz.cy;
                if(i + 1 < lines_.GetCount())
                    y += gap;
            }
        }
    }

    if(WhenPaintForeground)
        WhenPaintForeground(w, outer, p, m, s, state, has_focus);
    else
        UiPaintStyledForeground(w, outer, p, m, s, state, has_focus);
}

void UiLabel::LeftDown(Point p, dword)
{
    if(!selectable_text_ || text_.IsEmpty())
        return;
    SetFocus();
    sel_anchor_ = HitTestTextPos(p);
    sel_caret_ = sel_anchor_;
    selecting_drag_ = true;
    SetCapture();
    Refresh();
}

void UiLabel::MouseMove(Point p, dword)
{
    if(!selectable_text_ || !selecting_drag_)
        return;
    int pos = HitTestTextPos(p);
    if(sel_caret_ != pos) {
        sel_caret_ = pos;
        Refresh();
    }
}

void UiLabel::LeftUp(Point p, dword)
{
    if(!selectable_text_)
        return;
    if(selecting_drag_) {
        sel_caret_ = HitTestTextPos(p);
        selecting_drag_ = false;
        ReleaseCapture();
        Refresh();
    }
}

bool UiLabel::Key(dword key, int count)
{
    if(!selectable_text_)
        return Ctrl::Key(key, count);

    if(key == K_CTRL_A) {
        if(!text_.IsEmpty()) {
            sel_anchor_ = 0;
            sel_caret_ = text_.GetCount();
            Refresh();
        }
        return true;
    }
    if(key == K_CTRL_C || key == K_CTRL_INSERT) {
        CopySelectionToClipboard();
        return true;
    }
    if(key == K_ESCAPE) {
        if(HasSelection()) {
            sel_anchor_ = -1;
            sel_caret_ = -1;
            Refresh();
        }
        return true;
    }
    return Ctrl::Key(key, count);
}

void UiLabel::GotFocus()
{
    Refresh();
}

void UiLabel::LostFocus()
{
    selecting_drag_ = false;
    if(HasSelection()) {
        sel_anchor_ = -1;
        sel_caret_ = -1;
        Refresh();
    }
}

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




