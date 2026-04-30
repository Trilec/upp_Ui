#include "UiColorPicker.h"

namespace Upp {

namespace {

Color BlendTowardPaper(Color c, bool dark, int alpha = 32)
{
    Color paper = dark ? Color(22, 28, 39) : Color(250, 252, 255);
    return Blend(c, paper, alpha);
}

String FormatHexColor(Color c)
{
    return Format("#%02X%02X%02X", c.GetR(), c.GetG(), c.GetB());
}

String FormatRgb8(Color c)
{
    return Format("%d, %d, %d", c.GetR(), c.GetG(), c.GetB());
}

String FormatRgbUnit(Color c)
{
    return Format("%.4f, %.4f, %.4f",
                  c.GetR() / 255.0,
                  c.GetG() / 255.0,
                  c.GetB() / 255.0);
}

String FormatHsvColor(Color c)
{
    int h = 0;
    int s = 0;
    int v = 0;

    int r = c.GetR();
    int g = c.GetG();
    int b = c.GetB();

    int mx = max(max(r, g), b);
    int mn = min(min(r, g), b);
    int d = mx - mn;

    v = int((mx / 255.0) * 100.0 + 0.5);
    s = mx == 0 ? 0 : int((d / (double)mx) * 100.0 + 0.5);

    if(d == 0)
        h = 0;
    else if(mx == r)
        h = int(60.0 * fmod(((g - b) / (double)d), 6.0));
    else if(mx == g)
        h = int(60.0 * (((b - r) / (double)d) + 2.0));
    else
        h = int(60.0 * (((r - g) / (double)d) + 4.0));

    if(h < 0)
        h += 360;

    return Format("%d, %d, %d", h, s, v);
}

static int ClampByte(int v)
{
    return max(0, min(255, v));
}

static int ClampPercent(int v)
{
    return max(0, min(100, v));
}

static void ColorToHsv(Color c, int& h, int& s, int& v)
{
    double hh = 0.0, ss = 0.0, vv = 0.0;
    RGBtoHSV(c.GetR() / 255.0, c.GetG() / 255.0, c.GetB() / 255.0, hh, ss, vv);
    h = int(hh * 360.0 + 0.5);
    s = int(ss * 100.0 + 0.5);
    v = int(vv * 100.0 + 0.5);
}

static Color HsvToColor(int h, int s, int v)
{
    double r = 0, g = 0, b = 0;
    HSVtoRGB(h / 360.0, s / 100.0, v / 100.0, r, g, b);
    return Color(ClampByte(int(r * 255.0 + 0.5)),
                 ClampByte(int(g * 255.0 + 0.5)),
                 ClampByte(int(b * 255.0 + 0.5)));
}

static void ColorToCmyk(Color c, int& cc, int& mm, int& yy, int& kk)
{
    double c1 = 0.0, m1 = 0.0, y1 = 0.0, k1 = 0.0;
    RGBtoCMYK(c.GetR() / 255.0, c.GetG() / 255.0, c.GetB() / 255.0, c1, m1, y1, k1);
    cc = ClampPercent(int(c1 * 100.0 + 0.5));
    mm = ClampPercent(int(m1 * 100.0 + 0.5));
    yy = ClampPercent(int(y1 * 100.0 + 0.5));
    kk = ClampPercent(int(k1 * 100.0 + 0.5));
}

static Color CmykToColor(int cc, int mm, int yy, int kk)
{
    double r = 0.0, g = 0.0, b = 0.0;
    CMYKtoRGB(ClampPercent(cc) / 100.0, ClampPercent(mm) / 100.0, ClampPercent(yy) / 100.0, ClampPercent(kk) / 100.0, r, g, b);
    return Color(ClampByte(int(r * 255.0 + 0.5)),
                 ClampByte(int(g * 255.0 + 0.5)),
                 ClampByte(int(b * 255.0 + 0.5)));
}

static VectorMap<String, Vector<Color> >& ColorLibraryPalettes_()
{
    static VectorMap<String, Vector<Color> > m;
    if(m.IsEmpty()) {
        m.Add("X-Rite ColorChecker", Vector<Color> {
            Color(0x73,0x52,0x44), Color(0xC2,0x96,0x82), Color(0x62,0x7A,0x9D), Color(0x57,0x6C,0x43),
            Color(0x85,0x80,0xB1), Color(0x67,0xBD,0xAA), Color(0xD6,0x7E,0x2C), Color(0x50,0x5B,0xA6),
            Color(0xC1,0x5A,0x63), Color(0x5E,0x3C,0x73), Color(0x9D,0xBC,0x40), Color(0xE0,0xA3,0x2E),
            Color(0x38,0x3D,0x88), Color(0x46,0x94,0x49), Color(0xAF,0x36,0x3C), Color(0xF3,0xC3,0x00),
            Color(0x9C,0x5A,0xA5), Color(0x00,0xA1,0xC7), Color(0xF3,0xF3,0xF2), Color(0xC8,0xC8,0xC8),
            Color(0xA0,0xA0,0xA0), Color(0x7A,0x7A,0x7A), Color(0x55,0x55,0x55), Color(0x34,0x34,0x34)
        });
        m.Add("Bright UI", Vector<Color> {
            Color(0xFF,0x2D,0x55), Color(0xFF,0x3B,0x30), Color(0xFF,0x95,0x00), Color(0xFF,0xCC,0x00),
            Color(0x34,0xC7,0x59), Color(0x00,0xC7,0xBE), Color(0x32,0xAD,0xE6), Color(0x00,0x7A,0xFF),
            Color(0x58,0x56,0xD6), Color(0xAF,0x52,0xDE), Color(0xFF,0x5A,0xC8), Color(0xFF,0xFF,0xFF),
            Color(0x11,0x11,0x11), Color(0x73,0x73,0x73), Color(0xD1,0xD1,0xD6), Color(0xF2,0xF2,0xF7)
        });
        m.Add("HDR Accents", Vector<Color> {
            Color(0x00,0xB3,0xFF), Color(0x00,0x80,0xFF), Color(0x00,0x48,0xFF), Color(0x6C,0x2C,0xFF),
            Color(0xFF,0x2B,0xD6), Color(0xFF,0x22,0x44), Color(0xFF,0x6A,0x00), Color(0xFF,0xD0,0x00),
            Color(0x9C,0xFF,0x00), Color(0x00,0xFF,0x80), Color(0x00,0xFF,0xF0), Color(0xFF,0xFF,0xFF)
        });
    }
    return m;
}

static void DrawHueTrack(Draw& w, const Rect& r)
{
    if(r.IsEmpty())
        return;
    for(int x = 0; x < r.GetWidth(); x++) {
        int hue = int((x / (double)max(1, r.GetWidth() - 1)) * 360.0 + 0.5);
        Color c = HsvToColor(hue, 100, 100);
        w.DrawRect(r.left + x, r.top, 1, r.GetHeight(), c);
    }
}

static void DrawSolidTrack(Draw& w, const Rect& r, Color c)
{
    if(!r.IsEmpty())
        w.DrawRect(r, c);
}

static void DrawGrayRampTrack(Draw& w, const Rect& r, Color a, Color b)
{
    if(r.IsEmpty())
        return;
    for(int x = 0; x < r.GetWidth(); x++) {
        int t = int((x / (double)max(1, r.GetWidth() - 1)) * 255.0 + 0.5);
        Color c = Blend(a, b, t);
        w.DrawRect(r.left + x, r.top, 1, r.GetHeight(), c);
    }
}

static Image MakeSolidSwatchImage(Color c, Size sz)
{
    if(sz.cx <= 0 || sz.cy <= 0)
        return Image();
    ImageBuffer ib(sz);
    for(int y = 0; y < sz.cy; y++) {
        RGBA *q = ib[y];
        for(int x = 0; x < sz.cx; x++)
            q[x] = RGBA(c);
    }
    return Image(ib);
}

static void DrawAlphaTrack(Draw& w, const Rect& r, Color base)
{
    if(r.IsEmpty())
        return;
    DrawGrayRampTrack(w, r, Color(0x32, 0x32, 0x32), Color(0x5C, 0x5C, 0x5C));
}

static void PrepValueEdit(UiFloatEdit& e, int precision, double mn, double mx)
{
    e.Precision(precision).MinMax(mn, mx).Step(1).ShowSpin(false);
    e.SetTextAlign(UiAlign::RIGHT);
}

struct ColorFieldCacheKey : Moveable<ColorFieldCacheKey> {
    Size sz;
    int  mode = 0;
    Color color;
    int  hue = 0;

    bool operator==(const ColorFieldCacheKey& b) const
    {
        return sz == b.sz && mode == b.mode && color == b.color && hue == b.hue;
    }
};

inline hash_t GetHashValue(const ColorFieldCacheKey& k)
{
    CombineHash h;
    h << k.sz.cx << k.sz.cy << k.mode << k.color.GetRaw() << k.hue;
    return h;
}


class ColorChip : public Ctrl {
public:
    typedef ColorChip CLASSNAME;

    void SetColor(Color c, bool active, const String& text)
    {
        color_ = c;
        active_ = active;
        text_ = text;
        Refresh();
    }

    virtual void Paint(Draw& w) override
    {
        const bool dark = UiThemeDetail::ResolveEffectiveMode(UiTheme::GetContext().mode) == UiThemeMode::Dark;
        Rect r(Point(0, 0), GetSize());
        w.DrawRect(r, dark ? Color(6, 6, 6) : SColorPaper());
        w.DrawRect(r.Deflated(1), color_);
        w.DrawRect(r.left, r.top, r.GetWidth(), 1, active_ ? SColorHighlight() : SColorShadow());
        w.DrawRect(r.left, r.bottom - 1, r.GetWidth(), 1, active_ ? SColorHighlight() : SColorShadow());
        w.DrawRect(r.left, r.top, 1, r.GetHeight(), active_ ? SColorHighlight() : SColorShadow());
        w.DrawRect(r.right - 1, r.top, 1, r.GetHeight(), active_ ? SColorHighlight() : SColorShadow());

        Font f = SansSerifZ(8).Bold();
        Size ts = GetTextSize(text_, f);
        int tx = r.left + (r.GetWidth() - ts.cx) / 2;
        int ty = r.bottom - ts.cy - DPI(2);
        Color ink = Grayscale(color_) < 128 ? White() : Black();
        w.DrawText(tx, ty, text_, f, ink);
    }

private:
    Color  color_ = Black();
    bool   active_ = false;
    String text_;
};

}

class ReadoutRow : public ParentCtrl {
public:
    typedef ReadoutRow CLASSNAME;

    ReadoutRow()
    {
        Add(card_.SizePos());
        Add(copy_);
        NoWantFocus();
        copy_.SetText("")
             .SetIcon(ICON_CONTENT_CONTENT_COPY_48())
             .SetIconSize(DPI(12), DPI(12))
             .NoWantFocus();
        copy_.WhenAction = [=] {
            if(!copy_text_.IsEmpty())
                WriteClipboardText(copy_text_);
        };
    }

    void SetText(const String& title, const String& value, bool compact = false)
    {
        title_ = title;
        value_ = value;
        compact_ = compact;
        card_.SetTitle(title_).SetSubTitle(value_);
        RefreshLayout();
        Refresh();
    }

    void SetInk(Color title_ink, Color value_ink)
    {
        UiTitleCard::Style s = card_.GetStyle();
        s.title_color = title_ink;
        s.subtitle_color = value_ink;
        card_.SetStyle(s);
        Refresh();
    }

    void SetFonts(Font title_font, Font value_font)
    {
        UiTitleCard::Style s = card_.GetStyle();
        s.title_font = title_font;
        s.subtitle_font = value_font;
        card_.SetStyle(s);
        Refresh();
    }

    void SetCardStyle(const UiTitleCard::Style& s)
    {
        card_.SetStyle(s);
    }

    void SetCopyText(const String& s)
    {
        copy_text_ = s;
        copy_.Enable(!s.IsEmpty());
    }

    void SetCopyStyle(const UiButton::Style& s, Color icon)
    {
        copy_.SetStyle(s);
        copy_.SetIconColor(icon);
    }

    virtual void Layout() override
    {
        int bw = DPI(14);
        copy_.SetRect(GetSize().cx - bw - DPI(6), DPI(4), bw, bw);
    }

    virtual Size GetMinSize() const override
    {
        return card_.GetMinSize();
    }

private:
    String title_;
    String value_;
    bool   compact_ = false;
    String copy_text_;
    UiTitleCard card_;
    UiToolButton copy_;
};

class UiColorPicker::ColorField : public Ctrl {
public:
    typedef ColorField CLASSNAME;

    ColorField()
    {
        NoWantFocus();
    }

    void SetState(UiColorPicker::SpectrumMode mode, Color c, int hue)
    {
        if(mode_ != mode || color_ != c || hue_ != hue) {
            mode_ = mode;
            color_ = c;
            hue_ = hue;
            cache_ = Image();
            Refresh();
        }
    }

    Event<Point, bool> WhenPick;

    virtual void Paint(Draw& w) override
    {
        Rect r(Point(0, 0), GetSize());
        const Image& img = EnsureCache();
        w.DrawImage(r.left, r.top, img);

        Point marker = GetMarkerPos();
        Color frame = Color(0x22, 0x22, 0x22);
        w.DrawRect(r.left, r.top, r.GetWidth(), 1, frame);
        w.DrawRect(r.left, r.bottom - 1, r.GetWidth(), 1, frame);
        w.DrawRect(r.left, r.top, 1, r.GetHeight(), frame);
        w.DrawRect(r.right - 1, r.top, 1, r.GetHeight(), frame);

        Rect m = RectC(r.left + marker.x - DPI(3), r.top + marker.y - DPI(3), DPI(6), DPI(6));
        w.DrawRect(m, White());
        w.DrawRect(m.Deflated(1), Black());
    }

    virtual void LeftDown(Point p, dword) override
    {
        SetCapture();
        if(WhenPick)
            WhenPick(p, false);
    }

    virtual void MouseMove(Point p, dword flags) override
    {
        if(!HasCapture() || !(flags & K_MOUSELEFT))
            return;
        if(WhenPick)
            WhenPick(p, false);
    }

    virtual void LeftUp(Point p, dword) override
    {
        if(HasCapture())
            ReleaseCapture();
        if(WhenPick)
            WhenPick(p, true);
    }

    virtual Size GetMinSize() const override
    {
        return Size(DPI(180), DPI(180));
    }

private:
    const Image& EnsureCache() const
    {
        Size sz = GetSize();
        if(sz.IsEmpty())
            return cache_;

        ColorFieldCacheKey key;
        key.sz = sz;
        key.mode = (int)mode_;
        key.color = color_;
        key.hue = hue_;

        if(cache_.IsEmpty() || !(key == cache_key_)) {
            cache_ = UiGetCachedImage(key, [=] {
                ImageBuffer ib(sz);
                for(int y = 0; y < sz.cy; y++) {
                    RGBA *q = ib[y];
                    for(int x = 0; x < sz.cx; x++)
                        q[x] = RGBA(SampleAt(Point(x, y)));
                }
                return Image(ib);
            }, 32);
            cache_key_ = key;
        }
        return cache_;
    }

    Color SampleAt(Point p) const
    {
        int w = max(1, GetSize().cx - 1);
        int h = max(1, GetSize().cy - 1);
        int x = max(0, min(p.x, w));
        int y = max(0, min(p.y, h));

        switch(mode_) {
        case UiColorPicker::SPECTRUM_HUE_STRIP: {
            int hue = int((x / (double)w) * 360.0 + 0.5);
            int val = 100 - int((y / (double)h) * 100.0 + 0.5);
            return HsvToColor(hue, 100, val);
        }
        case UiColorPicker::SPECTRUM_RGB_SPECTRUM: {
            int r = int((x / (double)w) * 255.0 + 0.5);
            int g = int((y / (double)h) * 255.0 + 0.5);
            return Color(r, g, color_.GetB());
        }
        case UiColorPicker::SPECTRUM_HSV_RECT:
        default: {
            int sat = int((x / (double)w) * 100.0 + 0.5);
            int val = 100 - int((y / (double)h) * 100.0 + 0.5);
            return HsvToColor(hue_, sat, val);
        }
        }
    }

    Point GetMarkerPos() const
    {
        int w = max(1, GetSize().cx - 1);
        int h = max(1, GetSize().cy - 1);

        switch(mode_) {
        case UiColorPicker::SPECTRUM_HUE_STRIP: {
            int hue = 0, sat = 0, val = 0;
            ColorToHsv(color_, hue, sat, val);
            return Point(int((hue / 360.0) * w + 0.5), int((100 - val) / 100.0 * h + 0.5));
        }
        case UiColorPicker::SPECTRUM_RGB_SPECTRUM:
            return Point(int((color_.GetR() / 255.0) * w + 0.5), int((color_.GetG() / 255.0) * h + 0.5));
        case UiColorPicker::SPECTRUM_HSV_RECT:
        default: {
            int hue = 0, sat = 0, val = 0;
            ColorToHsv(color_, hue, sat, val);
            return Point(int((sat / 100.0) * w + 0.5), int((100 - val) / 100.0 * h + 0.5));
        }
        }
    }

    UiColorPicker::SpectrumMode mode_ = UiColorPicker::SPECTRUM_HSV_RECT;
    Color color_ = Color(0, 120, 212);
    int   hue_ = 0;
    mutable ColorFieldCacheKey cache_key_;
    mutable Image cache_;
};

class UiColorPicker::SwatchGrid : public Ctrl {
public:
    typedef SwatchGrid CLASSNAME;

    void SetGrid(int cols, int rows)
    {
        cols_ = max(1, cols);
        rows_ = max(1, rows);
        Refresh();
    }

    void SetColors(const Vector<Color>& c)
    {
        colors_ <<= c;
        Refresh();
    }

    void SetActive(Color c)
    {
        active_ = c;
        Refresh();
    }

    Event<Color> WhenPick;

    virtual void Paint(Draw& w) override
    {
        const bool dark = UiThemeDetail::ResolveEffectiveMode(UiTheme::GetContext().mode) == UiThemeMode::Dark;
        Rect r(Point(0, 0), GetSize());
        w.DrawRect(r, dark ? Color(12, 12, 12) : SColorPaper());

        int cell = DPI(20);
        int gap = DPI(4);
        for(int row = 0; row < rows_; row++) {
            for(int col = 0; col < cols_; col++) {
                int i = row * cols_ + col;
                Rect cellr = RectC(r.left + col * (cell + gap),
                                   r.top + row * (cell + gap),
                                   cell,
                                   cell);
                w.DrawRect(cellr, dark ? Color(30, 30, 30) : Blend(SColorShadow(), SColorPaper(), 220));
                w.DrawRect(cellr.Deflated(1), dark ? Color(18, 18, 18) : SColorFace());

                if(i < colors_.GetCount()) {
                    w.DrawRect(cellr.Deflated(2), colors_[i]);
                    bool active = colors_[i] == active_;
                    Color frame = active ? Color(0, 120, 212) : (dark ? Color(48, 48, 48) : Blend(SColorShadow(), SColorPaper(), 90));
                    w.DrawRect(cellr.left, cellr.top, cellr.GetWidth(), 1, frame);
                    w.DrawRect(cellr.left, cellr.bottom - 1, cellr.GetWidth(), 1, frame);
                    w.DrawRect(cellr.left, cellr.top, 1, cellr.GetHeight(), frame);
                    w.DrawRect(cellr.right - 1, cellr.top, 1, cellr.GetHeight(), frame);
                }
                else {
                    w.DrawRect(cellr.Deflated(2), dark ? Color(14, 14, 14) : Blend(SColorShadow(), SColorPaper(), 235));
                }
            }
        }
    }

    virtual void LeftDown(Point p, dword) override
    {
        int cell = DPI(20);
        int gap = DPI(4);
        int stride = cell + gap;
        int col = p.x / max(1, stride);
        int row = p.y / max(1, stride);
        if(col < 0 || col >= cols_ || row < 0 || row >= rows_)
            return;
        int i = row * cols_ + col;
        if(i >= 0 && i < colors_.GetCount())
            WhenPick(colors_[i]);
    }

    virtual Size GetMinSize() const override
    {
        int cell = DPI(20);
        int gap = DPI(4);
        return Size(cols_ * cell + max(0, cols_ - 1) * gap,
                    rows_ * cell + max(0, rows_ - 1) * gap);
    }

private:
    int cols_ = 10;
    int rows_ = 2;
    Vector<Color> colors_;
    Color active_ = Null;
};

const UiColorPicker::Style& UiColorPicker::StyleDefault()
{
    static Style s;
    static bool init = false;
    if(!init) {
        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(Color(12, 12, 12));
            s.palette.frame[i] = Color(34, 34, 34);
            s.palette.ink[i] = Color(208, 208, 208);
        }
        s.metrics.face_enabled = true;
        s.metrics.frame_enabled = true;
        s.metrics.frame_width = DPI(1);
        s.metrics.radius = DPI(4);
        s.metrics.content_margin = Rect(DPI(8), DPI(8), DPI(8), DPI(8));
        s.metrics.focus_enabled = false;
        init = true;
    }
    return s;
}

UiColorPicker::UiColorPicker()
    : slider_hue_axis_(UiDirection::H)
    , slider_value_axis_(UiDirection::H)
    , slider_r_(UiDirection::H)
    , slider_g_(UiDirection::H)
    , slider_b_(UiDirection::H)
    , slider_a_(UiDirection::H)
    , slider_h_(UiDirection::H)
    , slider_s_(UiDirection::H)
    , slider_v_(UiDirection::H)
    , slider_c_(UiDirection::H)
    , slider_m_(UiDirection::H)
    , slider_y_(UiDirection::H)
    , slider_k_(UiDirection::H)
{
    slots_.SetCount(4);
    previous_slots_.SetCount(4);
    slots_[0].label = "C1";
    slots_[1].label = "C2";
    slots_[2].label = "C3";
    slots_[3].label = "C4";
    slots_[0].color = Color(0, 120, 212);
    slots_[1].color = White();
    slots_[2].color = Color(52, 52, 52);
    slots_[3].color = Color(255, 42, 24);
    previous_slots_ <<= slots_;

    BuildChildTree();

    header_title_.SetLabel("SYSTEM : COLOR : TECHNICAL");
    header_title_.SetAlign(ALIGN_LEFT);
    header_title_.SetFrame(NullFrame());

    tabs_.SetVisual(UITAB_UNDERLINE);
    tabs_.Add(picker_page_, "Visual Picker");
    tabs_.Add(swatches_page_, "Swatch Library");
    tabs_.SetActiveTab(0);
    {
        UiTab::Style ts = UiTheme::ResolveTab(UITAB_UNDERLINE);
        ts.fill_tabs = true;
        ts.item_spacing = DPI(6);
        tabs_.SetStyle(ts);
    }

    spectrum_mode_drop_.Add("Hue Cube", (int)SPECTRUM_HSV_RECT);
    spectrum_mode_drop_.Add("Hue Strip", (int)SPECTRUM_HUE_STRIP);
    spectrum_mode_drop_.Add("RGB Spectrum", (int)SPECTRUM_RGB_SPECTRUM);
    spectrum_mode_drop_.SelectByData((int)SPECTRUM_RGB_SPECTRUM);
    spectrum_mode_ = SPECTRUM_RGB_SPECTRUM;
    for(int i = 0; i < ColorLibraryPalettes_().GetCount(); i++)
        library_palette_drop_.Add(ColorLibraryPalettes_().GetKey(i), i);
    library_palette_drop_.Select(0);

    slider_r_.SetRange(0, 255).SetStep(1).SetValue(slots_[0].color.GetR());
    slider_g_.SetRange(0, 255).SetStep(1).SetValue(slots_[0].color.GetG());
    slider_b_.SetRange(0, 255).SetStep(1).SetValue(slots_[0].color.GetB());
    slider_a_.SetRange(0, 255).SetStep(1).SetValue(255);
    slider_hue_axis_.SetRange(0, 360).SetStep(1).SetValue(200);
    slider_value_axis_.SetRange(0, 100).SetStep(1).SetValue(83);
    slider_alpha_axis_.SetRange(0, 255).SetStep(1).SetValue(255);
    slider_h_.SetRange(0, 360).SetStep(1).SetValue(200);
    slider_s_.SetRange(0, 100).SetStep(1).SetValue(100);
    slider_v_.SetRange(0, 100).SetStep(1).SetValue(83);
    slider_c_.SetRange(0, 100).SetStep(1).SetValue(0);
    slider_m_.SetRange(0, 100).SetStep(1).SetValue(0);
    slider_y_.SetRange(0, 100).SetStep(1).SetValue(0);
    slider_k_.SetRange(0, 100).SetStep(1).SetValue(0);

    const Size wide_track(DPI(4096), DPI(4));
    const Size thumb_size(DPI(14), DPI(18));
    slider_hue_axis_.SetTrackSize(wide_track).SetThumbSize(thumb_size);
    slider_value_axis_.SetTrackSize(wide_track).SetThumbSize(thumb_size);
    slider_alpha_axis_.SetTrackSize(wide_track).SetThumbSize(thumb_size);
    slider_r_.SetTrackSize(wide_track).SetThumbSize(thumb_size);
    slider_g_.SetTrackSize(wide_track).SetThumbSize(thumb_size);
    slider_b_.SetTrackSize(wide_track).SetThumbSize(thumb_size);
    slider_a_.SetTrackSize(wide_track).SetThumbSize(thumb_size);
    slider_h_.SetTrackSize(wide_track).SetThumbSize(thumb_size);
    slider_s_.SetTrackSize(wide_track).SetThumbSize(thumb_size);
    slider_v_.SetTrackSize(wide_track).SetThumbSize(thumb_size);
    slider_c_.SetTrackSize(wide_track).SetThumbSize(thumb_size);
    slider_m_.SetTrackSize(wide_track).SetThumbSize(thumb_size);
    slider_y_.SetTrackSize(wide_track).SetThumbSize(thumb_size);
    slider_k_.SetTrackSize(wide_track).SetThumbSize(thumb_size);

    add_user_swatch_button_.SetText("Pull");
    transfer_to_active_button_.SetText("Push");
    push_user_swatch_button_.SetText("Store");

    recent_grid_->SetGrid(12, 2);
    user_grid_->SetGrid(12, 2);

    picker_section_title_.SetLabel("Spectrum");
    current_slot_title_.SetLabel("CURRENT SLOT");
    previous_slot_title_.SetLabel("PREVIOUS");
    hue_axis_title_.SetLabel("Hue");
    value_axis_title_.SetLabel("Gain");
    alpha_axis_title_.SetLabel("Alpha");
    rgb_section_title_.SetLabel("RGB Channels");
    hsv_section_title_.SetLabel("HSV Vectors");
    cmyk_section_title_.SetLabel("CMYK Process");
    live_section_title_.SetLabel("Live Selection");
    swatches_palette_title_.SetLabel("Library Palette");
    swatches_user_title_.SetLabel("User Stash");

    PrepValueEdit(hue_axis_value_, 0, 0, 360);
    PrepValueEdit(value_axis_value_, 0, 0, 100);
    PrepValueEdit(alpha_axis_value_, 0, 0, 255);
    PrepValueEdit(channel_r_value_, 0, 0, 255);
    PrepValueEdit(channel_g_value_, 0, 0, 255);
    PrepValueEdit(channel_b_value_, 0, 0, 255);
    PrepValueEdit(channel_a_value_, 0, 0, 255);
    PrepValueEdit(channel_h_value_, 0, 0, 360);
    PrepValueEdit(channel_s_value_, 0, 0, 100);
    PrepValueEdit(channel_v_value_, 0, 0, 100);
    PrepValueEdit(channel_ha_value_, 0, 0, 360);
    PrepValueEdit(channel_sa_value_, 0, 0, 100);
    PrepValueEdit(channel_va_value_, 0, 0, 100);
    PrepValueEdit(channel_aa_value_, 0, 0, 255);
    PrepValueEdit(channel_c_value_, 0, 0, 100);
    PrepValueEdit(channel_m_value_, 0, 0, 100);
    PrepValueEdit(channel_y_value_, 0, 0, 100);
    PrepValueEdit(channel_k_value_, 0, 0, 100);

    channel_r_.SetLabel("R");
    channel_g_.SetLabel("G");
    channel_b_.SetLabel("B");
    channel_a_.SetLabel("A");
    channel_h_.SetLabel("H");
    channel_s_.SetLabel("S");
    channel_v_.SetLabel("V");
    channel_ha_.SetLabel("H");
    channel_sa_.SetLabel("S");
    channel_va_.SetLabel("V");
    channel_aa_.SetLabel("A");
    channel_c_.SetLabel("C");
    channel_m_.SetLabel("M");
    channel_y_.SetLabel("Y");
    channel_k_.SetLabel("K");

    swatch_hint_.SetLabel("MODE: PULL TO ACTIVE SLOT");
    swatch_hint_.SetFrame(NullFrame());
    swatch_hint_.SetAlign(ALIGN_LEFT);

    mixer_placeholder_.SetLabel("Mixer page placeholder.\nThis is where gradient stop editing and channel groups can evolve next.");
    mixer_placeholder_.SetFrame(NullFrame());
    mixer_placeholder_.SetAlign(ALIGN_LEFT);

    for(int i = 0; i < 4; i++) {
        const int ii = i;
        slot_button_[i].WhenAction = [=] { HandleSlotButton(ii); };
    }

    tabs_.WhenAction = [=] { UpdateTabVisibility(); };
    spectrum_mode_drop_.WhenSelectData = [=](const Value& v) { SetSpectrumMode((SpectrumMode)(int)v); };
    library_palette_drop_.WhenSelect = [=](int i) {
        if(i >= 0 && i < ColorLibraryPalettes_().GetCount())
            recent_grid_->SetColors(ColorLibraryPalettes_()[i]);
    };

    slider_r_.WhenChanging = [=] { ApplySliderColor(false); };
    slider_g_.WhenChanging = [=] { ApplySliderColor(false); };
    slider_b_.WhenChanging = [=] { ApplySliderColor(false); };
    slider_a_.WhenChanging = [=] { CommitAlpha(false); };
    slider_hue_axis_.WhenChanging = [=] { CommitColor(HsvToColor((int)slider_hue_axis_.GetValue(), (int)slider_s_.GetValue(), (int)slider_v_.GetValue()), false); };
    slider_value_axis_.WhenChanging = [=] { CommitColor(HsvToColor((int)slider_hue_axis_.GetValue(), (int)slider_s_.GetValue(), (int)slider_value_axis_.GetValue()), false); };
    slider_alpha_axis_.WhenChanging = [=] { slider_a_.SetValue(slider_alpha_axis_.GetValue()); CommitAlpha(false); };
    slider_h_.WhenChanging = [=] { CommitColor(HsvToColor((int)slider_h_.GetValue(), (int)slider_s_.GetValue(), (int)slider_v_.GetValue()), false); };
    slider_s_.WhenChanging = [=] { CommitColor(HsvToColor((int)slider_h_.GetValue(), (int)slider_s_.GetValue(), (int)slider_v_.GetValue()), false); };
    slider_v_.WhenChanging = [=] { CommitColor(HsvToColor((int)slider_h_.GetValue(), (int)slider_s_.GetValue(), (int)slider_v_.GetValue()), false); };
    slider_c_.WhenChanging = [=] { CommitColor(CmykToColor((int)slider_c_.GetValue(), (int)slider_m_.GetValue(), (int)slider_y_.GetValue(), (int)slider_k_.GetValue()), false); };
    slider_m_.WhenChanging = [=] { CommitColor(CmykToColor((int)slider_c_.GetValue(), (int)slider_m_.GetValue(), (int)slider_y_.GetValue(), (int)slider_k_.GetValue()), false); };
    slider_y_.WhenChanging = [=] { CommitColor(CmykToColor((int)slider_c_.GetValue(), (int)slider_m_.GetValue(), (int)slider_y_.GetValue(), (int)slider_k_.GetValue()), false); };
    slider_k_.WhenChanging = [=] { CommitColor(CmykToColor((int)slider_c_.GetValue(), (int)slider_m_.GetValue(), (int)slider_y_.GetValue(), (int)slider_k_.GetValue()), false); };

    slider_r_.WhenAction = [=] { ApplySliderColor(true); };
    slider_g_.WhenAction = [=] { ApplySliderColor(true); };
    slider_b_.WhenAction = [=] { ApplySliderColor(true); };
    slider_a_.WhenAction = [=] { CommitAlpha(true); };
    slider_hue_axis_.WhenAction = [=] { CommitColor(HsvToColor((int)slider_hue_axis_.GetValue(), (int)slider_s_.GetValue(), (int)slider_v_.GetValue()), true); };
    slider_value_axis_.WhenAction = [=] { CommitColor(HsvToColor((int)slider_hue_axis_.GetValue(), (int)slider_s_.GetValue(), (int)slider_value_axis_.GetValue()), true); };
    slider_alpha_axis_.WhenAction = [=] { slider_a_.SetValue(slider_alpha_axis_.GetValue()); CommitAlpha(true); };
    slider_h_.WhenAction = [=] { CommitColor(HsvToColor((int)slider_h_.GetValue(), (int)slider_s_.GetValue(), (int)slider_v_.GetValue()), true); };
    slider_s_.WhenAction = [=] { CommitColor(HsvToColor((int)slider_h_.GetValue(), (int)slider_s_.GetValue(), (int)slider_v_.GetValue()), true); };
    slider_v_.WhenAction = [=] { CommitColor(HsvToColor((int)slider_h_.GetValue(), (int)slider_s_.GetValue(), (int)slider_v_.GetValue()), true); };
    slider_c_.WhenAction = [=] { CommitColor(CmykToColor((int)slider_c_.GetValue(), (int)slider_m_.GetValue(), (int)slider_y_.GetValue(), (int)slider_k_.GetValue()), true); };
    slider_m_.WhenAction = [=] { CommitColor(CmykToColor((int)slider_c_.GetValue(), (int)slider_m_.GetValue(), (int)slider_y_.GetValue(), (int)slider_k_.GetValue()), true); };
    slider_y_.WhenAction = [=] { CommitColor(CmykToColor((int)slider_c_.GetValue(), (int)slider_m_.GetValue(), (int)slider_y_.GetValue(), (int)slider_k_.GetValue()), true); };
    slider_k_.WhenAction = [=] { CommitColor(CmykToColor((int)slider_c_.GetValue(), (int)slider_m_.GetValue(), (int)slider_y_.GetValue(), (int)slider_k_.GetValue()), true); };

    add_user_swatch_button_.WhenAction = [=] { HandleAddUserSwatch(); };
    transfer_to_active_button_.WhenAction = [=] { HandleTransferRecentToActive(); };

    recent_grid_->WhenPick = [=](Color c) { HandleRecentPick(c); };
    user_grid_->WhenPick = [=](Color c) { HandleUserPick(c); };
    color_field_->WhenPick = [=](Point p, bool final_commit) {
        Rect rr = color_field_->GetRect();
        int x = max(0, min(p.x, rr.GetWidth() - 1));
        int y = max(0, min(p.y, rr.GetHeight() - 1));
        Color current = GetSlotColor(active_slot_);
        int h = 0, s = 0, v = 0;
        ColorToHsv(current, h, s, v);

        switch(spectrum_mode_) {
        case SPECTRUM_HUE_STRIP:
            h = int((x / (double)max(1, rr.GetWidth() - 1)) * 360.0 + 0.5);
            v = 100 - int((y / (double)max(1, rr.GetHeight() - 1)) * 100.0 + 0.5);
            current = HsvToColor(h, 100, v);
            break;
        case SPECTRUM_RGB_SPECTRUM:
            current = Color(int((x / (double)max(1, rr.GetWidth() - 1)) * 255.0 + 0.5),
                            int((y / (double)max(1, rr.GetHeight() - 1)) * 255.0 + 0.5),
                            current.GetB());
            break;
        case SPECTRUM_HSV_RECT:
        default:
            s = int((x / (double)max(1, rr.GetWidth() - 1)) * 100.0 + 0.5);
            v = 100 - int((y / (double)max(1, rr.GetHeight() - 1)) * 100.0 + 0.5);
            current = HsvToColor(h, s, v);
            break;
        }
        CommitColor(current, final_commit);
    };

    hue_axis_value_.WhenAction = [=] { slider_hue_axis_.SetValue(hue_axis_value_.GetValue()); CommitColor(HsvToColor((int)slider_hue_axis_.GetValue(), (int)slider_s_.GetValue(), (int)slider_v_.GetValue()), true); };
    value_axis_value_.WhenAction = [=] { slider_value_axis_.SetValue(value_axis_value_.GetValue()); CommitColor(HsvToColor((int)slider_hue_axis_.GetValue(), (int)slider_s_.GetValue(), (int)slider_value_axis_.GetValue()), true); };
    alpha_axis_value_.WhenAction = [=] { slider_alpha_axis_.SetValue(alpha_axis_value_.GetValue()); slider_a_.SetValue(alpha_axis_value_.GetValue()); CommitAlpha(true); };
    channel_r_value_.WhenAction = [=] { slider_r_.SetValue(channel_r_value_.GetValue()); ApplySliderColor(true); };
    channel_g_value_.WhenAction = [=] { slider_g_.SetValue(channel_g_value_.GetValue()); ApplySliderColor(true); };
    channel_b_value_.WhenAction = [=] { slider_b_.SetValue(channel_b_value_.GetValue()); ApplySliderColor(true); };
    channel_a_value_.WhenAction = [=] { slider_a_.SetValue(channel_a_value_.GetValue()); CommitAlpha(true); };
    channel_h_value_.WhenAction = [=] { slider_h_.SetValue(channel_h_value_.GetValue()); CommitColor(HsvToColor((int)slider_h_.GetValue(), (int)slider_s_.GetValue(), (int)slider_v_.GetValue()), true); };
    channel_s_value_.WhenAction = [=] { slider_s_.SetValue(channel_s_value_.GetValue()); CommitColor(HsvToColor((int)slider_h_.GetValue(), (int)slider_s_.GetValue(), (int)slider_v_.GetValue()), true); };
    channel_v_value_.WhenAction = [=] { slider_v_.SetValue(channel_v_value_.GetValue()); CommitColor(HsvToColor((int)slider_h_.GetValue(), (int)slider_s_.GetValue(), (int)slider_v_.GetValue()), true); };
    channel_ha_value_.WhenAction = [=] { slider_h_.SetValue(channel_ha_value_.GetValue()); CommitColor(HsvToColor((int)slider_h_.GetValue(), (int)slider_s_.GetValue(), (int)slider_v_.GetValue()), true); };
    channel_sa_value_.WhenAction = [=] { slider_s_.SetValue(channel_sa_value_.GetValue()); CommitColor(HsvToColor((int)slider_h_.GetValue(), (int)slider_s_.GetValue(), (int)slider_v_.GetValue()), true); };
    channel_va_value_.WhenAction = [=] { slider_v_.SetValue(channel_va_value_.GetValue()); CommitColor(HsvToColor((int)slider_h_.GetValue(), (int)slider_s_.GetValue(), (int)slider_v_.GetValue()), true); };
    channel_aa_value_.WhenAction = [=] { slider_a_.SetValue(channel_aa_value_.GetValue()); CommitAlpha(true); };
    channel_c_value_.WhenAction = [=] { slider_c_.SetValue(channel_c_value_.GetValue()); CommitColor(CmykToColor((int)slider_c_.GetValue(), (int)slider_m_.GetValue(), (int)slider_y_.GetValue(), (int)slider_k_.GetValue()), true); };
    channel_m_value_.WhenAction = [=] { slider_m_.SetValue(channel_m_value_.GetValue()); CommitColor(CmykToColor((int)slider_c_.GetValue(), (int)slider_m_.GetValue(), (int)slider_y_.GetValue(), (int)slider_k_.GetValue()), true); };
    channel_y_value_.WhenAction = [=] { slider_y_.SetValue(channel_y_value_.GetValue()); CommitColor(CmykToColor((int)slider_c_.GetValue(), (int)slider_m_.GetValue(), (int)slider_y_.GetValue(), (int)slider_k_.GetValue()), true); };
    channel_k_value_.WhenAction = [=] { slider_k_.SetValue(channel_k_value_.GetValue()); CommitColor(CmykToColor((int)slider_c_.GetValue(), (int)slider_m_.GetValue(), (int)slider_y_.GetValue(), (int)slider_k_.GetValue()), true); };

    slider_hue_axis_.WhenPaintTrack = [=](Draw& w, const UiSlider::PaintContext& ctx, bool& handled) {
        DrawHueTrack(w, ctx.track);
        handled = true;
    };
    slider_hue_axis_.WhenPaintActiveTrack = [=](Draw&, const UiSlider::PaintContext&, bool& handled) {
        handled = true;
    };
    slider_value_axis_.WhenPaintTrack = [=](Draw& w, const UiSlider::PaintContext& ctx, bool& handled) {
        DrawGrayRampTrack(w, ctx.track, Color(0x32, 0x32, 0x32), Color(0x5C, 0x5C, 0x5C));
        handled = true;
    };
    slider_value_axis_.WhenPaintActiveTrack = [=](Draw&, const UiSlider::PaintContext&, bool& handled) {
        handled = true;
    };
    slider_alpha_axis_.WhenPaintTrack = [=](Draw& w, const UiSlider::PaintContext& ctx, bool& handled) {
        DrawGrayRampTrack(w, ctx.track, Color(0x32, 0x32, 0x32), Color(0x5C, 0x5C, 0x5C));
        handled = true;
    };
    slider_alpha_axis_.WhenPaintActiveTrack = [=](Draw&, const UiSlider::PaintContext&, bool& handled) {
        handled = true;
    };

    recent_grid_->SetColors(ColorLibraryPalettes_()[0]);
    SyncFromActiveSlot(false);
    SyncThemeToChildren();
    UpdateTabVisibility();
}

UiColorPicker::~UiColorPicker()
{
}

void UiColorPicker::BuildChildTree()
{
    Add(header_bar_);
    Add(tabs_);

    header_bar_.Add(header_title_);
    for(int i = 0; i < 4; i++)
        tabs_.Ctrl::Add(slot_button_[i]);

    color_field_.Create();
    recent_grid_.Create();
    user_grid_.Create();
    readout_hex_.Create();
    readout_rgb_unit_.Create();
    readout_hsv_.Create();
    readout_alpha_.Create();

    picker_page_.Add(picker_left_);
    picker_page_.Add(picker_right_);

    picker_left_.Add(picker_section_title_);
    picker_left_.Add(spectrum_mode_drop_);
    picker_left_.Add(*color_field_);
    picker_right_.Add(current_slot_card_);
    picker_right_.Add(previous_slot_card_);
    current_slot_card_.Add(current_slot_title_);
    current_slot_card_.Add(current_slot_preview_);
    previous_slot_card_.Add(previous_slot_title_);
    previous_slot_card_.Add(previous_slot_preview_);
    picker_left_.Add(hue_axis_title_);
    picker_left_.Add(value_axis_title_);
    picker_left_.Add(alpha_axis_title_);
    picker_left_.Add(hue_axis_value_);
    picker_left_.Add(value_axis_value_);
    picker_left_.Add(alpha_axis_value_);
    picker_left_.Add(slider_hue_axis_);
    picker_left_.Add(slider_value_axis_);
    picker_left_.Add(slider_alpha_axis_);
    picker_page_.Add(*readout_hex_);
    picker_page_.Add(*readout_rgb_unit_);
    picker_page_.Add(*readout_hsv_);
    picker_page_.Add(*readout_alpha_);

    picker_right_.Add(rgb_section_title_);
    picker_right_.Add(channel_r_);
    picker_right_.Add(channel_r_value_);
    picker_right_.Add(channel_g_);
    picker_right_.Add(channel_g_value_);
    picker_right_.Add(channel_b_);
    picker_right_.Add(channel_b_value_);
    picker_right_.Add(channel_a_);
    picker_right_.Add(channel_a_value_);
    picker_right_.Add(slider_r_);
    picker_right_.Add(slider_g_);
    picker_right_.Add(slider_b_);
    picker_right_.Add(slider_a_);
    picker_right_.Add(slider_h_);
    picker_right_.Add(slider_s_);
    picker_right_.Add(slider_v_);
    picker_right_.Add(hsv_section_title_);
    picker_right_.Add(channel_ha_);
    picker_right_.Add(channel_ha_value_);
    picker_right_.Add(channel_sa_);
    picker_right_.Add(channel_sa_value_);
    picker_right_.Add(channel_va_);
    picker_right_.Add(channel_va_value_);
    picker_right_.Add(channel_aa_);
    picker_right_.Add(channel_aa_value_);
    picker_right_.Add(channel_h_);
    picker_right_.Add(channel_h_value_);
    picker_right_.Add(channel_s_);
    picker_right_.Add(channel_s_value_);
    picker_right_.Add(channel_v_);
    picker_right_.Add(channel_v_value_);
    picker_right_.Add(cmyk_section_title_);
    picker_right_.Add(channel_c_);
    picker_right_.Add(channel_c_value_);
    picker_right_.Add(channel_m_);
    picker_right_.Add(channel_m_value_);
    picker_right_.Add(channel_y_);
    picker_right_.Add(channel_y_value_);
    picker_right_.Add(channel_k_);
    picker_right_.Add(channel_k_value_);
    picker_right_.Add(slider_c_);
    picker_right_.Add(slider_m_);
    picker_right_.Add(slider_y_);
    picker_right_.Add(slider_k_);
    picker_right_.Add(live_section_title_);

    swatches_page_.Add(swatches_palette_title_);
    swatches_page_.Add(library_palette_drop_);
    swatches_page_.Add(*recent_grid_);
    swatches_page_.Add(swatches_user_title_);
    swatches_page_.Add(*user_grid_);
    swatches_page_.Add(add_user_swatch_button_);
    swatches_page_.Add(transfer_to_active_button_);
    swatches_page_.Add(push_user_swatch_button_);
    swatches_page_.Add(swatch_hint_);

    mixer_page_.Add(mixer_placeholder_);
}

UiColorPicker& UiColorPicker::SetStyle(const Style& s)
{
    style_ = s;
    has_style_override_ = true;
    OnStyleChanged();
    return *this;
}

UiColorPicker& UiColorPicker::ClearStyleOverride()
{
    if(!has_style_override_)
        return *this;

    has_style_override_ = false;
    OnStyleChanged();
    return *this;
}

void UiColorPicker::OnStyleChanged()
{
    InvalidateStyleCache();
    children_theme_revision_ = 0;
    SyncThemeToChildren();
    RefreshLayout();
    Refresh();
}

UiColorPicker::Style& UiColorPicker::StyleEdit()
{
    has_style_override_ = true;
    InvalidateStyleCache();
    children_theme_revision_ = 0;
    return style_;
}

void UiColorPicker::InvalidateStyleCache()
{
    theme_revision_ = 0;
    children_style_dirty_ = true;
}

void UiColorPicker::SyncThemeStyle()
{
    uint64 rev = UiTheme::GetRevision();
    if(theme_revision_ == rev)
        return;
    themed_style_ = has_style_override_ ? style_ : StyleDefault();
    if(!has_style_override_) {
        const bool dark = UiThemeDetail::ResolveEffectiveMode(UiTheme::GetContext().mode) == UiThemeMode::Dark;
        const Color face = dark ? Color(12, 12, 12) : Color(252, 253, 255);
        const Color header = dark ? Color(8, 8, 8) : Color(245, 248, 252);
        const Color frame = dark ? Color(34, 34, 34) : Blend(SColorShadow(), SColorPaper(), 150);
        const Color ink = dark ? Color(208, 208, 208) : SColorText();
        for(int i = 0; i < 4; i++) {
            themed_style_.palette.face[i] = UiFill::Solid(i == ST_NORMAL ? face : header);
            themed_style_.palette.frame[i] = frame;
            themed_style_.palette.ink[i] = ink;
        }
    }
    theme_revision_ = rev;
}

const UiColorPicker::Style& UiColorPicker::GetEffectiveStyle() const
{
    const_cast<UiColorPicker *>(this)->SyncThemeStyle();
    return themed_style_;
}

UiColorPicker& UiColorPicker::SetSlotCount(int n)
{
    slot_count_ = minmax(n, 1, 4);
    if(active_slot_ >= slot_count_)
        active_slot_ = slot_count_ - 1;
    SyncSlotButtons();
    RefreshLayout();
    Refresh();
    return *this;
}

UiColorPicker& UiColorPicker::SetActiveSlot(int i)
{
    if(i < 0 || i >= slot_count_ || i == active_slot_)
        return *this;
    active_slot_ = i;
    SyncFromActiveSlot(false);
    WhenSlotChanged(active_slot_);
    return *this;
}

UiColorPicker& UiColorPicker::SetSlotColor(int i, Color c, bool fire)
{
    if(i < 0 || i >= slots_.GetCount())
        return *this;
    if(slots_[i].color != c)
        previous_slots_[i].color = slots_[i].color;
    slots_[i].color = c;
    if(i == active_slot_)
        SyncFromActiveSlot(fire);
    else
        SyncSlotButtons();

    if(fire) {
        PushRecentColor(c);
        WhenAction();
    }
    return *this;
}

Color UiColorPicker::GetSlotColor(int i) const
{
    if(i < 0 || i >= slots_.GetCount())
        return Black();
    return slots_[i].color;
}

UiColorPicker& UiColorPicker::SetSlotLabel(int i, const String& s)
{
    if(i < 0 || i >= slots_.GetCount())
        return *this;
    slots_[i].label = s;
    SyncSlotButtons();
    return *this;
}

String UiColorPicker::GetSlotLabel(int i) const
{
    if(i < 0 || i >= slots_.GetCount())
        return String();
    return slots_[i].label;
}

UiColorPicker& UiColorPicker::SetAlphaEnabled(bool on)
{
    alpha_enabled_ = on;
    slider_a_.Show(on);
    SyncControlsFromColor(GetSlotColor(active_slot_));
    RefreshLayout();
    Refresh();
    return *this;
}

UiColorPicker& UiColorPicker::SetSpectrumMode(SpectrumMode m)
{
    spectrum_mode_ = m;
    SyncSpectrumMode();
    return *this;
}

UiColorPicker& UiColorPicker::AddUserSwatch(Color c)
{
    if(IsNull(c))
        return *this;
    if(user_swatches_.GetCount() >= 20)
        user_swatches_.Remove(0);
    user_swatches_.Add(c);
    user_grid_->SetColors(user_swatches_);
    return *this;
}

UiColorPicker& UiColorPicker::ClearUserSwatches()
{
    user_swatches_.Clear();
    user_grid_->SetColors(user_swatches_);
    return *this;
}

UiColorPicker& UiColorPicker::ClearRecentSwatches()
{
    recent_swatches_.Clear();
    recent_grid_->SetColors(recent_swatches_);
    return *this;
}

int UiColorPicker::GetUserSwatchCount() const
{
    return user_swatches_.GetCount();
}

int UiColorPicker::GetRecentSwatchCount() const
{
    return recent_swatches_.GetCount();
}

String UiColorPicker::FormatActiveHex() const
{
    return FormatHexColor(GetSlotColor(active_slot_));
}

String UiColorPicker::FormatActiveRgb8() const
{
    return FormatRgb8(GetSlotColor(active_slot_));
}

String UiColorPicker::FormatActiveRgbUnit() const
{
    return FormatRgbUnit(GetSlotColor(active_slot_));
}

String UiColorPicker::FormatActiveHsv() const
{
    return FormatHsvColor(GetSlotColor(active_slot_));
}

String UiColorPicker::FormatActiveAlpha() const
{
    return Format("%d", alpha_enabled_ ? slots_[active_slot_].alpha : 255);
}

void UiColorPicker::SyncFromActiveSlot(bool fire)
{
    Color c = slots_[active_slot_].color;
    SyncControlsFromColor(c);

    if(fire)
        WhenChanging();
}

void UiColorPicker::SyncReadouts()
{
    Color c = slots_[active_slot_].color;
    int h = 0, s = 0, v = 0;
    int cc = 0, mm = 0, yy = 0, kk = 0;
    ColorToHsv(c, h, s, v);
    ColorToCmyk(c, cc, mm, yy, kk);

    const int a = alpha_enabled_ ? slots_[active_slot_].alpha : 255;
    const String hex8 = Format("%s%02X", FormatHexColor(c), a);
    const String normalized = Format("%.4f, %.4f, %.4f, %.4f",
                                     c.GetR() / 255.0,
                                     c.GetG() / 255.0,
                                     c.GetB() / 255.0,
                                     a / 255.0);
    const String hsva = Format("%d, %d%%, %d%%, %.2f",
                               h, s, v, a / 255.0);
    const String cmyka = Format("%d, %d, %d, %d, %.2f",
                                cc, mm, yy, kk, a / 255.0);

    readout_hex_->SetText("RGBA (HEX8)", hex8);
    readout_rgb_unit_->SetText("NORMALIZED", normalized, true);
    readout_hsv_->SetText("HSV-A", hsva);
    readout_alpha_->SetText("CMYK-A", cmyka);
    readout_hex_->SetCopyText(hex8);
    readout_rgb_unit_->SetCopyText(normalized);
    readout_hsv_->SetCopyText(hsva);
    readout_alpha_->SetCopyText(cmyka);
}

void UiColorPicker::SyncControlsFromColor(Color c)
{
    int h = 0, s = 0, v = 0;
    int cc = 0, mm = 0, yy = 0, kk = 0;
    ColorToHsv(c, h, s, v);
    ColorToCmyk(c, cc, mm, yy, kk);

    slider_r_.SetValue(c.GetR());
    slider_g_.SetValue(c.GetG());
    slider_b_.SetValue(c.GetB());
    slider_a_.SetValue(slots_[active_slot_].alpha);
    slider_alpha_axis_.SetValue(slots_[active_slot_].alpha);

    slider_hue_axis_.SetValue(h);
    slider_value_axis_.SetValue(v);
    slider_h_.SetValue(h);
    slider_s_.SetValue(s);
    slider_v_.SetValue(v);
    slider_c_.SetValue(cc);
    slider_m_.SetValue(mm);
    slider_y_.SetValue(yy);
    slider_k_.SetValue(kk);

    hue_axis_value_.SetValue(h);
    value_axis_value_.SetValue(v);
    alpha_axis_value_.SetValue(slots_[active_slot_].alpha);

    channel_r_value_.SetValue(c.GetR());
    channel_g_value_.SetValue(c.GetG());
    channel_b_value_.SetValue(c.GetB());
    channel_a_value_.SetValue(slots_[active_slot_].alpha);
    channel_h_value_.SetValue(h);
    channel_s_value_.SetValue(s);
    channel_v_value_.SetValue(v);
    channel_ha_value_.SetValue(h);
    channel_sa_value_.SetValue(s);
    channel_va_value_.SetValue(v);
    channel_aa_value_.SetValue(slots_[active_slot_].alpha);
    channel_c_value_.SetValue(cc);
    channel_m_value_.SetValue(mm);
    channel_y_value_.SetValue(yy);
    channel_k_value_.SetValue(kk);

    SyncReadouts();

    auto PaintPreview = [&](UiButton& btn, Color color) {
        UiButton::Style bs = UiTheme::ResolveButton(UiButtonRole::Subtle);
        for(int i = 0; i < 4; i++) {
            bs.palette.face[i] = UiFill::Solid(color);
            bs.palette.frame[i] = IsDark(color) ? Blend(color, White(), 75)
                                                : Blend(color, Black(), 75);
            bs.palette.ink[i] = Null;
            bs.palette.icon[i] = Null;
        }
        bs.metrics.face_enabled = true;
        bs.metrics.frame_enabled = true;
        bs.metrics.frame_width = DPI(1);
        bs.metrics.radius = DPI(4);
        bs.metrics.content_margin = Rect(0, 0, 0, 0);
        bs.metrics.shadow.enabled = false;
        btn.SetText("");
        btn.SetStyle(bs);
    };
    PaintPreview(current_slot_preview_, slots_[active_slot_].color);
    PaintPreview(previous_slot_preview_, previous_slots_[active_slot_].color);

    color_field_->SetState(spectrum_mode_, c, h);
    recent_grid_->SetActive(c);
    user_grid_->SetActive(c);
    SyncSlotButtons();
    Refresh();
}
void UiColorPicker::CommitColor(Color c, bool final_commit)
{
    if(final_commit && slots_[active_slot_].color != c)
        previous_slots_[active_slot_].color = slots_[active_slot_].color;
    slots_[active_slot_].color = c;
    dword now = msecs();
    if(!final_commit && live_update_ms_ != 0 && now - live_update_ms_ < 50)
        return;
    live_update_ms_ = now;
    SyncControlsFromColor(c);
    if(final_commit) {
        PushRecentColor(c);
        WhenAction();
    }
    else {
        WhenChanging();
    }
}

void UiColorPicker::CommitAlpha(bool final_commit)
{
    int new_alpha = ClampByte((int)slider_a_.GetValue());
    if(final_commit && previous_slots_[active_slot_].alpha != slots_[active_slot_].alpha)
        previous_slots_[active_slot_].alpha = slots_[active_slot_].alpha;
    slots_[active_slot_].alpha = new_alpha;
    dword now = msecs();
    if(!final_commit && live_update_ms_ != 0 && now - live_update_ms_ < 50)
        return;
    live_update_ms_ = now;
    SyncControlsFromColor(slots_[active_slot_].color);
    if(final_commit)
        WhenAction();
    else
        WhenChanging();
}
void UiColorPicker::SyncSlotButtons()
{
    const bool dark = UiThemeDetail::ResolveEffectiveMode(UiTheme::GetContext().mode) == UiThemeMode::Dark;
    const int inset = DPI(5);
    const int icon_side = max(DPI(12), GetEffectiveStyle().slot_size - inset * 2);
    Size icon_sz(icon_side, icon_side);
    for(int i = 0; i < 4; i++) {
        slot_button_[i].Show(i < slot_count_);
        slot_button_[i].SetText("");
        slot_button_[i].SetCheckable(true).SetChecked(i == active_slot_);
        UiButton::Style bs = UiTheme::ResolveButton(UiButtonRole::Subtle);
        for(int j = 0; j < 4; j++) {
            bs.palette.face[j] = UiFill::Solid(dark ? Color(12, 12, 12) : SColorPaper());
            bs.palette.frame[j] = dark ? Color(12, 12, 12) : SColorPaper();
            bs.palette.ink[j] = Null;
            bs.palette.icon[j] = Null;
        }
        bs.metrics.face_enabled = true;
        bs.metrics.frame_enabled = (i == active_slot_);
        bs.metrics.frame_width = DPI(1);
        bs.metrics.radius = DPI(3);
        bs.metrics.content_margin = Rect(inset, inset, inset, inset);
        bs.metrics.shadow.enabled = false;
        bs.metrics.focus_enabled = false;
        slot_button_[i].SetIcon(MakeSolidSwatchImage(slots_[i].color, icon_sz));
        slot_button_[i].SetIconSize(icon_sz);
        slot_button_[i].SetIconRenderMode(UiIconRenderMode::PreserveColor);
        slot_button_[i].SetAlign(UiAlign::CENTER, UiAlign::CENTER);
        if(i == active_slot_) {
            for(int j = 0; j < 4; j++)
                bs.palette.frame[j] = Color(0x32, 0x32, 0x32);
        }
        slot_button_[i].SetStyle(bs);
    }
}

void UiColorPicker::SyncSpectrumMode()
{
    spectrum_mode_drop_.SelectByData((int)spectrum_mode_);
    color_field_->SetState(spectrum_mode_, GetSlotColor(active_slot_), slider_hue_axis_.GetValue());
    Refresh();
}

void UiColorPicker::PushRecentColor(Color c)
{
    for(int i = 0; i < recent_swatches_.GetCount(); i++) {
        if(recent_swatches_[i] == c) {
            recent_swatches_.Remove(i);
            break;
        }
    }
    if(recent_swatches_.GetCount() >= 50)
        recent_swatches_.Remove(recent_swatches_.GetCount() - 1);
    recent_swatches_.Insert(0, c);
}
void UiColorPicker::ApplySliderColor(bool final_commit)
{
    CommitColor(Color((int)slider_r_.GetValue(),
                      (int)slider_g_.GetValue(),
                      (int)slider_b_.GetValue()),
                final_commit);
}

void UiColorPicker::HandleSlotButton(int index)
{
    SetActiveSlot(index);
}

void UiColorPicker::HandleRecentPick(Color c)
{
    pending_transfer_color_ = c;
}

void UiColorPicker::HandleUserPick(Color c)
{
    pending_transfer_color_ = c;
}

void UiColorPicker::HandleAddUserSwatch()
{
    if(!IsNull(pending_transfer_color_))
        SetSlotColor(active_slot_, pending_transfer_color_, true);
}

void UiColorPicker::HandleTransferRecentToActive()
{
    AddUserSwatch(GetSlotColor(active_slot_));
}

void UiColorPicker::UpdateTabVisibility()
{
    bool picker = tabs_.GetActiveTab() == PAGE_PICKER;
    bool swatches = tabs_.GetActiveTab() == PAGE_SWATCHES;
    bool mixer = tabs_.GetActiveTab() == PAGE_MIXER;
    bool show_readouts = picker || swatches;

    picker_page_.Show(picker);
    swatches_page_.Show(swatches);
    mixer_page_.Show(mixer);

    color_field_->Show(picker);
    spectrum_mode_drop_.Show(picker);
    slider_hue_axis_.Show(picker);
    slider_value_axis_.Show(picker);
    slider_alpha_axis_.Show(picker && alpha_enabled_);
    slider_r_.Show(picker);
    slider_g_.Show(picker);
    slider_b_.Show(picker);
    slider_a_.Show(picker && alpha_enabled_);
    slider_h_.Show(picker);
    slider_s_.Show(picker);
    slider_v_.Show(picker);
    slider_c_.Show(picker);
    slider_m_.Show(picker);
    slider_y_.Show(picker);
    slider_k_.Show(picker);
    readout_hex_->Show(show_readouts);
    readout_rgb_unit_->Show(show_readouts);
    readout_hsv_->Show(show_readouts);
    readout_alpha_->Show(show_readouts && alpha_enabled_);
    current_slot_card_.Show(picker);
    previous_slot_card_.Show(picker);
    hue_axis_title_.Show(picker);
    value_axis_title_.Show(picker);
    alpha_axis_title_.Show(picker && alpha_enabled_);
    hue_axis_value_.Show(picker);
    value_axis_value_.Show(picker);
    alpha_axis_value_.Show(picker && alpha_enabled_);

    library_palette_drop_.Show(swatches);
    recent_grid_->Show(swatches);
    user_grid_->Show(swatches);
    add_user_swatch_button_.Show(swatches);
    transfer_to_active_button_.Show(swatches);
    push_user_swatch_button_.Show(false);
    swatch_hint_.Show(swatches);

    mixer_placeholder_.Show(mixer);

    RefreshLayout();
}
void UiColorPicker::SyncThemeToChildren()
{
    const Style& s = GetEffectiveStyle();
    Color face = s.palette.face[ST_NORMAL].IsSolid() ? s.palette.face[ST_NORMAL].color : SColorPaper();
    bool dark_surface = IsDark(face);
    Color heading_ink = dark_surface ? Blend(face, White(), 210) : Blend(face, Black(), 185);
    Color label_ink = dark_surface ? Blend(face, White(), 180) : Blend(face, Black(), 150);
    Color value_ink = dark_surface ? Blend(face, White(), 135) : Blend(face, Black(), 105);

    Font heading_font = SansSerifZ(9).Bold();
    Font label_font = SansSerifZ(8);
    Font value_font = SansSerifZ(7);
    Font readout_value_font = SansSerifZ(8);
    Font readout_compact_font = SansSerifZ(7);
    Font mono_small = MonospaceZ(8);

    auto Prep = [&](Label& lbl, Color c, Font f, int align = ALIGN_LEFT) {
        lbl.SetInk(c);
        lbl.SetFont(f);
        lbl.SetFrame(NullFrame());
        lbl.SetAlign(align);
    };
    auto PrepEditField = [&](UiFloatEdit& e, Color ink) {
        UiBaseEdit::Style es = UiTheme::ResolveEdit();
        for(int i = 0; i < 4; i++) {
            es.palette.face[i] = UiFill::Solid(face);
            es.palette.frame[i] = Null;
            es.palette.ink[i] = ink;
        }
        es.selection_color = Color(33, 98, 227);
        es.selection_ink = White();
        es.metrics.face_enabled = false;
        es.metrics.frame_enabled = false;
        es.metrics.content_margin = Rect(0, 0, 0, 0);
        es.font = mono_small;
        e.SetStyle(es);
    };

    Prep(header_title_, heading_ink, heading_font);
    Prep(picker_section_title_, heading_ink, heading_font);
    Prep(current_slot_title_, label_ink, SansSerifZ(8).Bold());
    Prep(previous_slot_title_, label_ink, SansSerifZ(8).Bold());
    Prep(hue_axis_title_, label_ink, label_font, ALIGN_RIGHT);
    Prep(value_axis_title_, label_ink, label_font, ALIGN_RIGHT);
    Prep(alpha_axis_title_, label_ink, label_font, ALIGN_RIGHT);
    Prep(rgb_section_title_, heading_ink, heading_font);
    Prep(hsv_section_title_, heading_ink, heading_font);
    Prep(cmyk_section_title_, heading_ink, heading_font);
    Prep(live_section_title_, heading_ink, heading_font);
    Prep(swatches_palette_title_, heading_ink, heading_font);
    Prep(swatches_user_title_, heading_ink, heading_font);
    Prep(channel_r_, label_ink, label_font, ALIGN_RIGHT);
    Prep(channel_g_, label_ink, label_font, ALIGN_RIGHT);
    Prep(channel_b_, label_ink, label_font, ALIGN_RIGHT);
    Prep(channel_a_, label_ink, label_font, ALIGN_RIGHT);
    Prep(channel_h_, label_ink, label_font, ALIGN_RIGHT);
    Prep(channel_s_, label_ink, label_font, ALIGN_RIGHT);
    Prep(channel_v_, label_ink, label_font, ALIGN_RIGHT);
    Prep(channel_ha_, label_ink, label_font, ALIGN_RIGHT);
    Prep(channel_sa_, label_ink, label_font, ALIGN_RIGHT);
    Prep(channel_va_, label_ink, label_font, ALIGN_RIGHT);
    Prep(channel_aa_, label_ink, label_font, ALIGN_RIGHT);
    Prep(channel_c_, label_ink, label_font, ALIGN_RIGHT);
    Prep(channel_m_, label_ink, label_font, ALIGN_RIGHT);
    Prep(channel_y_, label_ink, label_font, ALIGN_RIGHT);
    Prep(channel_k_, label_ink, label_font, ALIGN_RIGHT);
    Prep(swatch_hint_, label_ink, label_font);
    Prep(mixer_placeholder_, label_ink, label_font);
    PrepEditField(hue_axis_value_, value_ink);
    PrepEditField(value_axis_value_, value_ink);
    PrepEditField(alpha_axis_value_, value_ink);
    PrepEditField(channel_r_value_, value_ink);
    PrepEditField(channel_g_value_, value_ink);
    PrepEditField(channel_b_value_, value_ink);
    PrepEditField(channel_a_value_, value_ink);
    PrepEditField(channel_h_value_, value_ink);
    PrepEditField(channel_s_value_, value_ink);
    PrepEditField(channel_v_value_, value_ink);
    PrepEditField(channel_ha_value_, value_ink);
    PrepEditField(channel_sa_value_, value_ink);
    PrepEditField(channel_va_value_, value_ink);
    PrepEditField(channel_aa_value_, value_ink);
    PrepEditField(channel_c_value_, value_ink);
    PrepEditField(channel_m_value_, value_ink);
    PrepEditField(channel_y_value_, value_ink);
    PrepEditField(channel_k_value_, value_ink);

    Color readout_title = heading_ink;
    Color readout_value = value_ink;
    UiTitleCard::Style info_style = UiTheme::ResolveTitleCard();
    for(int i = 0; i < 4; i++) {
        info_style.palette.face[i] = UiFill::Solid(face);
        info_style.palette.frame[i] = Color(0x22, 0x22, 0x22);
        info_style.palette.ink[i] = heading_ink;
    }
    info_style.metrics.face_enabled = false;
    info_style.metrics.frame_enabled = true;
    info_style.metrics.frame_width = DPI(1);
    info_style.metrics.radius = DPI(4);
    info_style.metrics.content_margin = Rect(DPI(6), DPI(4), DPI(22), DPI(4));
    info_style.metrics.shadow.enabled = false;
    info_style.show_rule = false;
    info_style.show_bottom_line = false;
    info_style.transparent = false;
    info_style.hover_enabled = false;
    info_style.title_subtitle_gap = 0;
    info_style.subtitle_copy_gap = 0;
    info_style.title_font = heading_font;
    info_style.subtitle_font = readout_value_font;
    info_style.title_color = readout_title;
    info_style.subtitle_color = readout_value;

    if(readout_hex_)
        readout_hex_->SetCardStyle(info_style);
    if(readout_rgb_unit_)
        readout_rgb_unit_->SetCardStyle(info_style);
    if(readout_hsv_)
        readout_hsv_->SetCardStyle(info_style);
    if(readout_alpha_)
        readout_alpha_->SetCardStyle(info_style);

    if(readout_hex_)
        readout_hex_->SetInk(readout_title, readout_value);
    if(readout_rgb_unit_)
        readout_rgb_unit_->SetInk(readout_title, readout_value);
    if(readout_hsv_)
        readout_hsv_->SetInk(readout_title, readout_value);
    if(readout_alpha_)
        readout_alpha_->SetInk(readout_title, readout_value);

    UiButton::Style copy_style = UiTheme::ResolveButton(UiButtonRole::Subtle);
    for(int i = 0; i < 4; i++) {
        copy_style.palette.face[i] = UiFill::Solid(face);
        copy_style.palette.frame[i] = face;
        copy_style.palette.ink[i] = value_ink;
        copy_style.palette.icon[i] = value_ink;
    }
    copy_style.metrics.face_enabled = false;
    copy_style.metrics.frame_enabled = false;
    copy_style.metrics.shadow.enabled = false;
    copy_style.metrics.content_margin = Rect(0, 0, 0, 0);
    copy_style.metrics.radius = 0;

    if(readout_hex_)
        readout_hex_->SetCopyStyle(copy_style, readout_value);
    if(readout_rgb_unit_)
        readout_rgb_unit_->SetCopyStyle(copy_style, readout_value);
    if(readout_hsv_)
        readout_hsv_->SetCopyStyle(copy_style, readout_value);
    if(readout_alpha_)
        readout_alpha_->SetCopyStyle(copy_style, readout_value);

    if(readout_hex_)
        readout_hex_->SetFonts(heading_font, readout_value_font);
    if(readout_rgb_unit_)
        readout_rgb_unit_->SetFonts(heading_font, readout_compact_font);
    if(readout_hsv_)
        readout_hsv_->SetFonts(heading_font, readout_value_font);
    if(readout_alpha_)
        readout_alpha_->SetFonts(heading_font, readout_value_font);

    UiDropdown::Style dd = UiTheme::ResolveDropdown();
    dd.font = SansSerifZ(8);

    Color dd_face     = dark_surface ? Color(12, 12, 12) : Color(252, 253, 255);
    Color dd_hot      = dark_surface ? Color(20, 22, 26) : Color(242, 246, 252);
    Color dd_pressed  = dark_surface ? Color(24, 28, 34) : Color(232, 240, 252);
    Color dd_frame    = dark_surface ? Color(34, 34, 34) : Color(202, 210, 222);
    Color dd_disabled = dark_surface ? Color(18, 18, 18) : Color(244, 246, 250);

    dd.palette.face[ST_NORMAL]   = UiFill::Solid(dd_face);
    dd.palette.face[ST_HOT]      = UiFill::Solid(dd_hot);
    dd.palette.face[ST_PRESSED]  = UiFill::Solid(dd_pressed);
    dd.palette.face[ST_DISABLED] = UiFill::Solid(dd_disabled);

    for(int i = 0; i < 4; i++) {
        dd.palette.frame[i] = dd_frame;
        dd.palette.ink[i]   = readout_value;
        dd.palette.icon[i]  = readout_value;
    }

    dd.popup_item_style.font = dd.font;
    dd.popup_item_style.transparent = true;
    dd.popup_item_style.palette.face[ST_NORMAL]   = UiFill::None();
    dd.popup_item_style.palette.face[ST_HOT]      = UiFill::Solid(dd_hot);
    dd.popup_item_style.palette.face[ST_PRESSED]  = UiFill::Solid(dd_pressed);
    dd.popup_item_style.palette.face[ST_DISABLED] = UiFill::Solid(Blend(dd_face, dd_frame, 35));
    for(int i = 0; i < 4; i++) {
        dd.popup_item_style.palette.frame[i] = Null;
        dd.popup_item_style.palette.ink[i]   = readout_value;
        dd.popup_item_style.palette.icon[i]  = readout_value;
    }

    dd.popup_background_color = dd_face;
    dd.popup_frame_color = dd_frame;
    dd.popup_use_main_skin = false;
    dd.popup_frame_width = DPI(1);
    dd.popup_radius = DPI(5);

    dd.skin.enabled = false;
    dd.metrics.frame_enabled = true;
    dd.metrics.frame_width = DPI(1);
    dd.metrics.face_enabled = true;
    dd.metrics.radius = DPI(5);
    dd.metrics.content_margin = Rect(DPI(8), DPI(6), DPI(8), DPI(6));
    dd.transparent = false;
    dd.indicator_size = DPI(11);

    spectrum_mode_drop_.SetStyle(dd);
    library_palette_drop_.SetStyle(dd);
    spectrum_mode_drop_.EnableDragReorder(true);
    library_palette_drop_.EnableDragReorder(true);

    UiButton::Style action_style = UiTheme::ResolveButton(UiButtonRole::Accent);
    for(int i = 0; i < 4; i++) {
        action_style.palette.frame[i] = Blend(face, Color(33, 98, 227), dark_surface ? 120 : 80);
        action_style.palette.face[i] = UiFill::Solid(i == ST_NORMAL ? Color(33, 98, 227)
                                                                    : Blend(Color(33, 98, 227), White(), 20));
        action_style.palette.ink[i] = White();
        action_style.palette.icon[i] = White();
    }
    action_style.metrics.radius = DPI(2);
    action_style.metrics.frame_enabled = true;
    action_style.metrics.face_enabled = true;
    add_user_swatch_button_.SetStyle(action_style);

    UiButton::Style secondary_action = UiTheme::ResolveButton(UiButtonRole::Subtle);
    for(int i = 0; i < 4; i++) {
        secondary_action.palette.face[i] = UiFill::Solid(face);
        secondary_action.palette.frame[i] = Blend(face, heading_ink, dark_surface ? 70 : 40);
        secondary_action.palette.ink[i] = label_ink;
        secondary_action.palette.icon[i] = label_ink;
    }
    secondary_action.metrics.radius = DPI(2);
    secondary_action.metrics.frame_enabled = true;
    secondary_action.metrics.face_enabled = true;
    transfer_to_active_button_.SetStyle(secondary_action);

    UiButton::Style slot_preview_style = UiTheme::ResolveButton(UiButtonRole::Subtle);
    for(int i = 0; i < 4; i++) {
        slot_preview_style.palette.frame[i] = Blend(face, heading_ink, dark_surface ? 65 : 35);
        slot_preview_style.palette.ink[i] = Null;
        slot_preview_style.palette.icon[i] = Null;
    }
    slot_preview_style.metrics.face_enabled = true;
    slot_preview_style.metrics.frame_enabled = true;
    slot_preview_style.metrics.frame_width = DPI(1);
    slot_preview_style.metrics.radius = DPI(4);
    slot_preview_style.metrics.content_margin = Rect(0, 0, 0, 0);
    current_slot_preview_.SetStyle(slot_preview_style);
    previous_slot_preview_.SetStyle(slot_preview_style);

    UiTab::Style ts = UiTheme::ResolveTab(UITAB_UNDERLINE);
    ts.visual = UITAB_UNDERLINE;
    ts.fill_tabs = false;
    ts.item_spacing = DPI(18);
    ts.indicator_thickness = DPI(3);
    ts.indicator_span = LARGE;
    ts.body_gap = 0;
    ts.tab_padding = Rect(DPI(0), DPI(6), DPI(0), DPI(6));
    ts.tab_metrics.face_enabled = false;
    ts.tab_metrics.frame_enabled = false;
    for(int i = 0; i < 4; i++) {
        ts.tab_palette.face[i] = UiFill::None();
        ts.tab_palette.frame[i] = Null;
        ts.tab_palette.ink[i] = dark_surface
                                ? ((i == ST_NORMAL || i == ST_DISABLED) ? Color(128, 137, 148) : Color(186, 192, 200))
                                : ((i == ST_NORMAL || i == ST_DISABLED) ? Color(88, 98, 112) : Color(120, 128, 136));
        ts.tab_palette.frame[i] = Color(0x22, 0x22, 0x22);
    }
    ts.tab_palette.frame[ST_PRESSED] = Color(0, 120, 212);
    tabs_.SetStyle(ts);

    children_theme_revision_ = theme_revision_;
    children_style_dirty_ = false;
}

void UiColorPicker::Paint(Draw& w)
{
    const Style& s = GetEffectiveStyle();
    Rect r(Point(0, 0), GetSize());
    Color face = s.palette.face[ST_NORMAL].IsSolid() ? s.palette.face[ST_NORMAL].color : SColorPaper();
    w.DrawRect(r, face);
    w.DrawRect(r.left, r.top, r.GetWidth(), s.header_height, Blend(face, Black(), 28));
    if(s.metrics.frame_enabled) {
        Color frame = IsNull(s.palette.frame[ST_NORMAL]) ? SColorShadow() : s.palette.frame[ST_NORMAL];
        w.DrawRect(r.left, r.top, r.GetWidth(), 1, frame);
        w.DrawRect(r.left, r.bottom - 1, r.GetWidth(), 1, frame);
        w.DrawRect(r.left, r.top, 1, r.GetHeight(), frame);
        w.DrawRect(r.right - 1, r.top, 1, r.GetHeight(), frame);
        w.DrawRect(r.left, s.header_height, r.GetWidth(), 1, frame);
    }
}

void UiColorPicker::Layout()
{
    const Style& s = GetEffectiveStyle();
    const uint64 rev = UiTheme::GetRevision();
    if(children_style_dirty_ || children_theme_revision_ != rev)
        SyncThemeToChildren();

    Rect body = UiStyledInnerRect(GetSize(), s.metrics, s.skin);
    Rect header = RectC(body.left, body.top, body.GetWidth(), s.header_height);
    header_bar_.SetRect(header);
    header_title_.SetRect(header.left + DPI(12), header.top + DPI(10), max(DPI(180), header.GetWidth() / 2), DPI(18));

    Rect tabsr = RectC(body.left, header.bottom, body.GetWidth(), max(0, body.GetHeight() - s.header_height));
    tabs_.SetRect(tabsr);
    Rect page = picker_page_.GetSize();
    if(page.IsEmpty())
        return;

    int slot_x = tabs_.GetSize().cx - DPI(8);
    int slot_y = DPI(2);
    for(int i = slot_count_ - 1; i >= 0; i--) {
        slot_x -= s.slot_size;
        slot_button_[i].SetRect(slot_x, slot_y, s.slot_size, s.slot_size);
        slot_x -= s.slot_gap;
    }
    for(int i = slot_count_; i < 4; i++)
        slot_button_[i].SetRect(0, 0, 0, 0);

    auto LayoutReadouts = [&](Rect area) {
        int gap = DPI(8);
        int h = DPI(52);
        int w = max(DPI(120), (area.GetWidth() - gap * 3) / 4);
        readout_hex_->SetRect(area.left, area.top, w, h);
        readout_rgb_unit_->SetRect(area.left + w + gap, area.top, w, h);
        readout_hsv_->SetRect(area.left + (w + gap) * 2, area.top, w, h);
        readout_alpha_->SetRect(area.left + (w + gap) * 3, area.top, area.GetWidth() - (w + gap) * 3, h);
    };

    if(tabs_.GetActiveTab() == PAGE_PICKER) {
        int right_w = min(max(DPI(320), s.right_panel_width - DPI(12)), max(DPI(320), page.GetWidth() * 44 / 100));
        Rect left = page;
        left.right -= right_w + s.page_gap;
        Rect right = RectC(left.right + s.page_gap, page.top, right_w, page.GetHeight());
        Rect left_local(Point(0, 0), left.GetSize());
        Rect right_local(Point(0, 0), right.GetSize());

        picker_left_.SetRect(left);
        picker_right_.SetRect(right);

        picker_section_title_.SetRect(0, 0, 0, 0);
        int readout_h = DPI(52);
        int bottom_margin = DPI(8);
        Rect readout_area(page.left, page.bottom - readout_h - bottom_margin, page.right, page.bottom - bottom_margin);
        LayoutReadouts(readout_area);

        int top_y = left_local.top + DPI(4);
        spectrum_mode_drop_.SetRect(left_local.left, top_y, left_local.GetWidth(), DPI(26));
        color_field_->SetRect(left_local.left, top_y + DPI(34), left_local.GetWidth(), max(DPI(180), left_local.GetHeight() - readout_h - DPI(160)));

        int axis_y = color_field_->GetRect().bottom + DPI(14);
        const int axis_label_w = DPI(24);
        const int axis_value_w = DPI(58);
        const int axis_gap = DPI(8);
        const int axis_row_h = DPI(24);
        const int axis_spacing = DPI(28);
        const int axis_slider_x = left_local.left + axis_label_w + axis_gap;
        const int axis_slider_w = left_local.GetWidth() - axis_label_w - axis_value_w - axis_gap * 2;

        hue_axis_title_.SetRect(left_local.left, axis_y + DPI(3), axis_label_w, axis_row_h);
        slider_hue_axis_.SetRect(axis_slider_x, axis_y, axis_slider_w, axis_row_h);
        hue_axis_value_.SetRect(left_local.right - axis_value_w, axis_y + DPI(2), axis_value_w, axis_row_h);

        value_axis_title_.SetRect(left_local.left, axis_y + axis_spacing + DPI(3), axis_label_w, axis_row_h);
        slider_value_axis_.SetRect(axis_slider_x, axis_y + axis_spacing, axis_slider_w, axis_row_h);
        value_axis_value_.SetRect(left_local.right - axis_value_w, axis_y + axis_spacing + DPI(2), axis_value_w, axis_row_h);

        alpha_axis_title_.SetRect(left_local.left, axis_y + axis_spacing * 2 + DPI(3), axis_label_w, axis_row_h);
        slider_alpha_axis_.SetRect(axis_slider_x, axis_y + axis_spacing * 2, axis_slider_w, axis_row_h);
        alpha_axis_value_.SetRect(left_local.right - axis_value_w, axis_y + axis_spacing * 2 + DPI(2), axis_value_w, axis_row_h);

        int y = right_local.top + DPI(4);
        int card_gap = DPI(10);
        int card_w = (right_local.GetWidth() - card_gap) / 2;
        current_slot_card_.SetRect(right_local.left, y, card_w, DPI(58));
        previous_slot_card_.SetRect(right_local.left + card_w + card_gap, y, right_local.GetWidth() - card_w - card_gap, DPI(58));
        current_slot_title_.SetRect(DPI(8), DPI(6), card_w - DPI(16), DPI(12));
        current_slot_preview_.SetRect(DPI(8), DPI(22), card_w - DPI(16), DPI(26));
        previous_slot_title_.SetRect(DPI(8), DPI(6), previous_slot_card_.GetRect().GetWidth() - DPI(16), DPI(12));
        previous_slot_preview_.SetRect(DPI(8), DPI(22), previous_slot_card_.GetRect().GetWidth() - DPI(16), DPI(26));
        y += DPI(72);

        const int chan_label_w = DPI(22);
        const int chan_value_w = DPI(44);
        const int chan_slider_x = DPI(26);
        const int chan_slider_w = right_local.GetWidth() - chan_slider_x - chan_value_w;
        const int row_step = DPI(20);
        int val_w = DPI(44);

        rgb_section_title_.SetRect(right_local.left, y, right_local.GetWidth(), s.section_title_height); y += DPI(18);
        channel_r_.SetRect(right_local.left, y, chan_label_w, s.readout_row_height);
        channel_r_value_.SetRect(right_local.right - chan_value_w, y, chan_value_w, s.readout_row_height);
        slider_r_.SetRect(right_local.left + chan_slider_x, y, chan_slider_w, DPI(18)); y += row_step;
        channel_g_.SetRect(right_local.left, y, chan_label_w, s.readout_row_height);
        channel_g_value_.SetRect(right_local.right - chan_value_w, y, chan_value_w, s.readout_row_height);
        slider_g_.SetRect(right_local.left + chan_slider_x, y, chan_slider_w, DPI(18)); y += row_step;
        channel_b_.SetRect(right_local.left, y, chan_label_w, s.readout_row_height);
        channel_b_value_.SetRect(right_local.right - chan_value_w, y, chan_value_w, s.readout_row_height);
        slider_b_.SetRect(right_local.left + chan_slider_x, y, chan_slider_w, DPI(18)); y += row_step;
        channel_a_.SetRect(right_local.left, y, chan_label_w, s.readout_row_height);
        channel_a_value_.SetRect(right_local.right - chan_value_w, y, chan_value_w, s.readout_row_height);
        slider_a_.SetRect(right_local.left + chan_slider_x, y, chan_slider_w, DPI(18)); y += DPI(28);

        hsv_section_title_.SetRect(right_local.left, y, right_local.GetWidth(), s.section_title_height); y += DPI(18);
        channel_ha_.SetRect(right_local.left, y, chan_label_w, s.readout_row_height);
        channel_ha_value_.SetRect(right_local.right - val_w, y, val_w, s.readout_row_height);
        slider_h_.SetRect(right_local.left + chan_slider_x, y, chan_slider_w, DPI(18)); y += row_step;
        channel_sa_.SetRect(right_local.left, y, chan_label_w, s.readout_row_height);
        channel_sa_value_.SetRect(right_local.right - val_w, y, val_w, s.readout_row_height);
        slider_s_.SetRect(right_local.left + chan_slider_x, y, chan_slider_w, DPI(18)); y += row_step;
        channel_va_.SetRect(right_local.left, y, chan_label_w, s.readout_row_height);
        channel_va_value_.SetRect(right_local.right - val_w, y, val_w, s.readout_row_height);
        slider_v_.SetRect(right_local.left + chan_slider_x, y, chan_slider_w, DPI(18)); y += DPI(24);
        channel_aa_.SetRect(0, 0, 0, 0);
        channel_aa_value_.SetRect(0, 0, 0, 0);

        cmyk_section_title_.SetRect(right_local.left, y, right_local.GetWidth(), s.section_title_height); y += DPI(18);
        channel_c_.SetRect(right_local.left, y, chan_label_w, s.readout_row_height);
        channel_c_value_.SetRect(right_local.right - val_w, y, val_w, s.readout_row_height);
        slider_c_.SetRect(right_local.left + chan_slider_x, y, chan_slider_w, DPI(18)); y += row_step;
        channel_m_.SetRect(right_local.left, y, chan_label_w, s.readout_row_height);
        channel_m_value_.SetRect(right_local.right - val_w, y, val_w, s.readout_row_height);
        slider_m_.SetRect(right_local.left + chan_slider_x, y, chan_slider_w, DPI(18)); y += row_step;
        channel_y_.SetRect(right_local.left, y, chan_label_w, s.readout_row_height);
        channel_y_value_.SetRect(right_local.right - val_w, y, val_w, s.readout_row_height);
        slider_y_.SetRect(right_local.left + chan_slider_x, y, chan_slider_w, DPI(18)); y += row_step;
        channel_k_.SetRect(right_local.left, y, chan_label_w, s.readout_row_height);
        channel_k_value_.SetRect(right_local.right - val_w, y, val_w, s.readout_row_height);
        slider_k_.SetRect(right_local.left + chan_slider_x, y, chan_slider_w, DPI(18));

        live_section_title_.SetRect(0, 0, 0, 0);
    }
    else if(tabs_.GetActiveTab() == PAGE_SWATCHES) {
        int readout_h = DPI(52);
        int bottom_margin = DPI(8);
        Rect readout_area(page.left, page.bottom - readout_h - bottom_margin, page.right, page.bottom - bottom_margin);
        LayoutReadouts(readout_area);

        int dropdown_w = DPI(220);
        swatches_palette_title_.SetRect(page.left, page.top + DPI(6), page.GetWidth() - dropdown_w - DPI(12), s.section_title_height);
        library_palette_drop_.SetRect(page.right - dropdown_w, page.top + DPI(2), dropdown_w, DPI(26));

        int palette_y = page.top + DPI(42);
        recent_grid_->SetRect(page.left, palette_y, page.GetWidth(), DPI(78));

        int user_y = palette_y + recent_grid_->GetRect().GetHeight() + DPI(18);
        swatches_user_title_.SetRect(page.left, user_y, page.GetWidth() - DPI(140), s.section_title_height);
        add_user_swatch_button_.SetRect(page.right - DPI(136), user_y - DPI(4), DPI(64), DPI(24));
        transfer_to_active_button_.SetRect(page.right - DPI(68), user_y - DPI(4), DPI(64), DPI(24));
        swatch_hint_.SetRect(page.left, user_y + DPI(16), page.GetWidth() - DPI(140), DPI(16));

        int stash_y = user_y + DPI(40);
        user_grid_->SetRect(page.left, stash_y, page.GetWidth(), max(DPI(76), readout_area.top - stash_y - DPI(12)));
        push_user_swatch_button_.SetRect(0, 0, 0, 0);

        current_slot_card_.SetRect(0, 0, 0, 0);
        previous_slot_card_.SetRect(0, 0, 0, 0);
    }
    else {
        mixer_placeholder_.SetRect(page.left, page.top, page.GetWidth(), DPI(80));
        readout_hex_->SetRect(0, 0, 0, 0);
        readout_rgb_unit_->SetRect(0, 0, 0, 0);
        readout_hsv_->SetRect(0, 0, 0, 0);
        readout_alpha_->SetRect(0, 0, 0, 0);
    }
}
Size UiColorPicker::GetMinSize() const
{
    return Size(DPI(620), DPI(460));
}

void UiColorPicker::SetData(const Value& v)
{
    if(v.Is<Color>()) {
        SetSlotColor(active_slot_, v);
        return;
    }
    if(v.Is<ValueArray>()) {
        ValueArray a = v;
        SetSlotCount(min(4, a.GetCount()));
        for(int i = 0; i < a.GetCount() && i < 4; i++) {
            if(a[i].Is<Color>())
                slots_[i].color = a[i];
        }
        SyncFromActiveSlot(false);
    }
}

Value UiColorPicker::GetData() const
{
    if(slot_count_ == 1)
        return slots_[0].color;

    ValueArray a;
    for(int i = 0; i < slot_count_; i++)
        a.Add(slots_[i].color);
    return a;
}

}

