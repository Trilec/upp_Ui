#include "UiColorPicker.h"
#include "UiColorPickerPaletteLab.h"

#include <cmath>

#ifdef PLATFORM_WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace Upp {

using namespace UiColorPickerPaletteLab;

namespace {

static int ClampByte_(int value)
{
    return minmax(value, 0, 255);
}

static Color AlphaComposite_(Color foreground, int alpha, Color background)
{
    int a = ClampByte_(alpha);
    int ia = 255 - a;
    return Color((foreground.GetR() * a + background.GetR() * ia + 127) / 255,
                 (foreground.GetG() * a + background.GetG() * ia + 127) / 255,
                 (foreground.GetB() * a + background.GetB() * ia + 127) / 255);
}

static Color CheckerColor_(int x, int y, int tile = 4)
{
    bool alternate = ((x / max(1, tile)) + (y / max(1, tile))) & 1;
    Color paper = SColorPaper();
    Color ink = SColorText();
    return alternate ? Blend(paper, ink, 34) : Blend(paper, ink, 18);
}

static Color SurfaceColor_(UiRole role = UiRole::Subtle)
{
    const UiPanel::Style& style = UiTheme::ResolvePanel(role);
    return style.palette.face[ST_NORMAL].IsSolid() ? style.palette.face[ST_NORMAL].color
                                                   : SColorPaper();
}

static Color FrameColor_(UiRole role = UiRole::Subtle)
{
    const UiPanel::Style& style = UiTheme::ResolvePanel(role);
    return IsNull(style.palette.frame[ST_NORMAL])
         ? Blend(SurfaceColor_(role), SColorText(), 36)
         : style.palette.frame[ST_NORMAL];
}

static void DrawFrame_(Draw& draw, const Rect& rect, Color color, int width = 1)
{
    if(rect.IsEmpty() || IsNull(color))
        return;
    width = max(1, width);
    draw.DrawRect(rect.left, rect.top, rect.GetWidth(), width, color);
    draw.DrawRect(rect.left, rect.bottom - width, rect.GetWidth(), width, color);
    draw.DrawRect(rect.left, rect.top, width, rect.GetHeight(), color);
    draw.DrawRect(rect.right - width, rect.top, width, rect.GetHeight(), color);
}

static void DrawAlphaSwatch_(Draw& draw, const Rect& rect, Color color, int alpha)
{
    if(rect.IsEmpty() || IsNull(color))
        return;
    alpha = ClampByte_(alpha);
    int tile = max(2, DPI(4));
    for(int y = rect.top; y < rect.bottom; y += tile) {
        for(int x = rect.left; x < rect.right; x += tile) {
            Color background = CheckerColor_(x - rect.left, y - rect.top, tile);
            Color output = alpha >= 255 ? color : AlphaComposite_(color, alpha, background);
            draw.DrawRect(x, y,
                          min(tile, rect.right - x),
                          min(tile, rect.bottom - y), output);
        }
    }
}

static Image MakeAlphaSwatchImage_(Color color, int alpha, Size size, bool split = false)
{
    if(size.IsEmpty() || IsNull(color))
        return Image();
    ImageBuffer buffer(size);
    int tile = max(2, DPI(4));
    for(int y = 0; y < size.cy; y++) {
        RGBA *row = buffer[y];
        for(int x = 0; x < size.cx; x++) {
            Color background = CheckerColor_(x, y, tile);
            Color output = split && x < size.cx / 2
                         ? color
                         : AlphaComposite_(color, alpha, background);
            row[x] = RGBA(output);
            row[x].a = 255;
        }
    }
    return Image(buffer);
}

static bool ReadScreenColor_(Color& color)
{
#ifdef PLATFORM_WIN32
    POINT point;
    if(!::GetCursorPos(&point))
        return false;
    HDC dc = ::GetDC(nullptr);
    if(!dc)
        return false;
    COLORREF pixel = ::GetPixel(dc, point.x, point.y);
    ::ReleaseDC(nullptr, dc);
    if(pixel == CLR_INVALID)
        return false;
    color = Color(GetRValue(pixel), GetGValue(pixel), GetBValue(pixel));
    return true;
#else
    (void)color;
    return false;
#endif
}

static Color SampleImage_(const Image& image, Pointf normalized, int radius = 2)
{
    if(image.IsEmpty())
        return Null;
    Size size = image.GetSize();
    int cx = minmax(int(normalized.x * size.cx), 0, size.cx - 1);
    int cy = minmax(int(normalized.y * size.cy), 0, size.cy - 1);
    int64 sr = 0, sg = 0, sb = 0, weight = 0;
    Color centre(image[cy][cx].r, image[cy][cx].g, image[cy][cx].b);
    double cl = 0.0, ca = 0.0, cb = 0.0;
    ColorToLab(centre, cl, ca, cb);
    for(int y = max(0, cy - radius); y <= min(size.cy - 1, cy + radius); y++) {
        const RGBA *row = image[y];
        for(int x = max(0, cx - radius); x <= min(size.cx - 1, cx + radius); x++) {
            if(row[x].a < 16)
                continue;
            Color sample(row[x].r, row[x].g, row[x].b);
            double l = 0.0, a = 0.0, b = 0.0;
            ColorToLab(sample, l, a, b);
            double distance = sqrt((l - cl) * (l - cl) + (a - ca) * (a - ca) + (b - cb) * (b - cb));
            if(distance > 18.0)
                continue;
            int w = radius + 2 - max(abs(x - cx), abs(y - cy));
            sr += row[x].r * w;
            sg += row[x].g * w;
            sb += row[x].b * w;
            weight += w;
        }
    }
    return weight > 0 ? Color(int(sr / weight), int(sg / weight), int(sb / weight)) : centre;
}

static Point WheelPoint_(int hue, double saturation, const Rect& wheel)
{
    double angle = (NormalizeHue(hue) - 90.0) * M_PI / 180.0;
    double radius = minmax(saturation, 0.0, 100.0) / 100.0 * wheel.GetWidth() * 0.5;
    Point centre = wheel.CenterPoint();
    return Point(centre.x + int(cos(angle) * radius + 0.5),
                 centre.y + int(sin(angle) * radius + 0.5));
}

static void PointToWheel_(Point point, const Rect& wheel, int& hue, int& saturation)
{
    Point centre = wheel.CenterPoint();
    double dx = point.x - centre.x;
    double dy = point.y - centre.y;
    double radius = max(1.0, wheel.GetWidth() * 0.5);
    hue = NormalizeHue(int(atan2(dy, dx) * 180.0 / M_PI + 90.0 + 0.5));
    saturation = minmax(int(sqrt(dx * dx + dy * dy) / radius * 100.0 + 0.5), 0, 100);
}

class CommitLineEdit_ : public UiLineEdit {
public:
    typedef CommitLineEdit_ CLASSNAME;
    Event<> WhenCommit;

    virtual void LostFocus() override
    {
        UiLineEdit::LostFocus();
        if(WhenCommit)
            WhenCommit();
    }
};

struct DisplaySwatch_ : Moveable<DisplaySwatch_> {
    UiColorPicker::SlotValue value;
    int family_id = -1;
    int hero_number = 0;
    bool hero = false;
    bool gamut_mapped = false;
};

class PaletteDragSource_ {
public:
    virtual ~PaletteDragSource_() {}
    virtual Vector<UiColorPicker::SlotValue> GetPaletteDragValues() const = 0;
};

class SwatchFlow_;

class SwatchTile_ : public UiToolButton {
public:
    typedef SwatchTile_ CLASSNAME;

    void Configure(SwatchFlow_& owner, int index)
    {
        owner_ = &owner;
        index_ = index;
        SetText("");
        SetContentInset(0);
        SetContentGap(0);
        WantFocus();
    }

    void SetDisplay(const DisplaySwatch_& display, bool selected, bool active, bool compact)
    {
        display_ = display;
        selected_ = selected;
        active_ = active;
        compact_ = compact;
        if(IsNull(display.value.color))
            Tip("Empty User Stash cell");
        else
            Tip(display.value.label.IsEmpty()
                ? FormatHex8(display.value.color, display.value.alpha)
                : Format("%s — %s", display.value.label, FormatHex8(display.value.color, display.value.alpha)));
        Refresh();
    }

    virtual Size GetMinSize() const override
    {
        return Size(DPI(26), DPI(26));
    }

    virtual void Paint(Draw& draw) override
    {