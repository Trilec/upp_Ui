#include "UiColorPicker.h"

#include <cmath>
#include <cstdlib>
#include <cfloat>

#ifdef PLATFORM_WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace Upp {

namespace {

static int ClampByte_(int value)
{
    return minmax(value, 0, 255);
}

static int ClampPercent_(int value)
{
    return minmax(value, 0, 100);
}

static double ClampUnit_(double value)
{
    return minmax(value, 0.0, 1.0);
}

static int NormalizeHue_(int hue)
{
    hue %= 360;
    if(hue < 0)
        hue += 360;
    return hue;
}

static int ClampHue_(int hue)
{
    return minmax(hue, 0, 359);
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

static Image MakeAlphaSwatchImage_(Color color, int alpha, Size size, bool split_preview = true)
{
    if(size.cx <= 0 || size.cy <= 0)
        return Image();

    alpha = ClampByte_(alpha);
    int tile = max(2, DPI(4));
    int split_x = split_preview ? size.cx / 2 : 0;

    ImageBuffer buffer(size);
    Fill(~buffer, RGBAZero(), buffer.GetLength());

    for(int y = 0; y < size.cy; y++) {
        RGBA *row = buffer[y];
        for(int x = 0; x < size.cx; x++) {
            Color background = CheckerColor_(x, y, tile);
            Color output = split_preview && x < split_x
                         ? color
                         : AlphaComposite_(color, alpha, background);
            row[x] = RGBA(output);
            row[x].a = 255;
        }
    }

    return Image(buffer);
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

static void DrawCheckerboard_(Draw& draw, const Rect& rect)
{
    if(rect.IsEmpty())
        return;

    int tile = max(2, DPI(4));
    for(int y = rect.top; y < rect.bottom; y += tile) {
        for(int x = rect.left; x < rect.right; x += tile) {
            draw.DrawRect(x, y,
                          min(tile, rect.right - x),
                          min(tile, rect.bottom - y),
                          CheckerColor_(x - rect.left, y - rect.top, tile));
        }
    }
}

static void DrawAlphaSwatch_(Draw& draw, const Rect& rect, Color color, int alpha)
{
    if(rect.IsEmpty())
        return;
    alpha = ClampByte_(alpha);
    if(alpha >= 255) {
        draw.DrawRect(rect, color);
        return;
    }

    int tile = max(2, DPI(4));
    for(int y = rect.top; y < rect.bottom; y += tile) {
        for(int x = rect.left; x < rect.right; x += tile) {
            Color background = CheckerColor_(x - rect.left, y - rect.top, tile);
            draw.DrawRect(x, y,
                          min(tile, rect.right - x),
                          min(tile, rect.bottom - y),
                          AlphaComposite_(color, alpha, background));
        }
    }
}

static void ColorToHsv_(Color color, int& hue, int& saturation, int& value)
{
    double h = 0.0;
    double s = 0.0;
    double v = 0.0;
    RGBtoHSV(color.GetR() / 255.0,
             color.GetG() / 255.0,
             color.GetB() / 255.0,
             h, s, v);
    hue = ClampHue_(int(h * 360.0 + 0.5));
    saturation = ClampPercent_(int(s * 100.0 + 0.5));
    value = ClampPercent_(int(v * 100.0 + 0.5));
}

static Color HsvToColor_(double hue, double saturation, double value)
{
    hue = NormalizeHue_(int(std::floor(hue + 0.5)));
    saturation = minmax(saturation, 0.0, 100.0);
    value = minmax(value, 0.0, 100.0);

    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
    HSVtoRGB(hue / 360.0, saturation / 100.0, value / 100.0, r, g, b);
    return Color(ClampByte_(int(r * 255.0 + 0.5)),
                 ClampByte_(int(g * 255.0 + 0.5)),
                 ClampByte_(int(b * 255.0 + 0.5)));
}

static void ColorToHsl_(Color color, int& hue, int& saturation, int& lightness)
{
    double r = color.GetR() / 255.0;
    double g = color.GetG() / 255.0;
    double b = color.GetB() / 255.0;
    double maximum = max(r, max(g, b));
    double minimum = min(r, min(g, b));
    double delta = maximum - minimum;

    double h = 0.0;
    double l = (maximum + minimum) * 0.5;
    double s = 0.0;

    if(delta > 1e-12) {
        s = delta / (1.0 - fabs(2.0 * l - 1.0));
        if(maximum == r)
            h = 60.0 * fmod((g - b) / delta, 6.0);
        else if(maximum == g)
            h = 60.0 * (((b - r) / delta) + 2.0);
        else
            h = 60.0 * (((r - g) / delta) + 4.0);
        if(h < 0.0)
            h += 360.0;
    }

    hue = ClampHue_(int(h + 0.5));
    saturation = ClampPercent_(int(s * 100.0 + 0.5));
    lightness = ClampPercent_(int(l * 100.0 + 0.5));
}

static Color HslToColor_(double hue, double saturation, double lightness)
{
    hue = NormalizeHue_(int(std::floor(hue + 0.5))) / 360.0;
    saturation = minmax(saturation / 100.0, 0.0, 1.0);
    lightness = minmax(lightness / 100.0, 0.0, 1.0);

    auto hue_to_rgb = [](double p, double q, double t) {
        if(t < 0.0)
            t += 1.0;
        if(t > 1.0)
            t -= 1.0;
        if(t < 1.0 / 6.0)
            return p + (q - p) * 6.0 * t;
        if(t < 1.0 / 2.0)
            return q;
        if(t < 2.0 / 3.0)
            return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
        return p;
    };

    double r = lightness;
    double g = lightness;
    double b = lightness;

    if(saturation > 1e-12) {
        double q = lightness < 0.5
                 ? lightness * (1.0 + saturation)
                 : lightness + saturation - lightness * saturation;
        double p = 2.0 * lightness - q;
        r = hue_to_rgb(p, q, hue + 1.0 / 3.0);
        g = hue_to_rgb(p, q, hue);
        b = hue_to_rgb(p, q, hue - 1.0 / 3.0);
    }

    return Color(ClampByte_(int(r * 255.0 + 0.5)),
                 ClampByte_(int(g * 255.0 + 0.5)),
                 ClampByte_(int(b * 255.0 + 0.5)));
}

static void ColorToCmyk_(Color color, int& cyan, int& magenta, int& yellow, int& black)
{
    double c = 0.0;
    double m = 0.0;
    double y = 0.0;
    double k = 0.0;
    RGBtoCMYK(color.GetR() / 255.0,
              color.GetG() / 255.0,
              color.GetB() / 255.0,
              c, m, y, k);
    cyan = ClampPercent_(int(c * 100.0 + 0.5));
    magenta = ClampPercent_(int(m * 100.0 + 0.5));
    yellow = ClampPercent_(int(y * 100.0 + 0.5));
    black = ClampPercent_(int(k * 100.0 + 0.5));
}

static Color CmykToColor_(double cyan, double magenta, double yellow, double black)
{
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
    CMYKtoRGB(minmax(cyan / 100.0, 0.0, 1.0),
              minmax(magenta / 100.0, 0.0, 1.0),
              minmax(yellow / 100.0, 0.0, 1.0),
              minmax(black / 100.0, 0.0, 1.0),
              r, g, b);
    return Color(ClampByte_(int(r * 255.0 + 0.5)),
                 ClampByte_(int(g * 255.0 + 0.5)),
                 ClampByte_(int(b * 255.0 + 0.5)));
}

static void ColorToLab_(Color color, double& l, double& a, double& b)
{
    auto linear = [](double value) {
        return value <= 0.04045 ? value / 12.92
                               : pow((value + 0.055) / 1.055, 2.4);
    };
    double r = linear(color.GetR() / 255.0);
    double g = linear(color.GetG() / 255.0);
    double blue = linear(color.GetB() / 255.0);
    double x = (r * 0.4124564 + g * 0.3575761 + blue * 0.1804375) / 0.95047;
    double y = (r * 0.2126729 + g * 0.7151522 + blue * 0.0721750);
    double z = (r * 0.0193339 + g * 0.1191920 + blue * 0.9503041) / 1.08883;
    auto pivot = [](double value) {
        return value > 0.008856 ? pow(value, 1.0 / 3.0)
                                : 7.787 * value + 16.0 / 116.0;
    };
    double fx = pivot(x), fy = pivot(y), fz = pivot(z);
    l = 116.0 * fy - 16.0;
    a = 500.0 * (fx - fy);
    b = 200.0 * (fy - fz);
}

static Color LabToColor_(double l, double a, double b)
{
    l = minmax(l, 0.0, 100.0);
    a = minmax(a, -128.0, 127.0);
    b = minmax(b, -128.0, 127.0);
    auto inverse = [](double value) {
        double cube = value * value * value;
        return cube > 0.008856 ? cube : (value - 16.0 / 116.0) / 7.787;
    };
    auto linear_rgb = [&](double chroma_scale, double& r, double& g, double& blue) {
        double fy = (l + 16.0) / 116.0;
        double fx = fy + a * chroma_scale / 500.0;
        double fz = fy - b * chroma_scale / 200.0;
        double x = 0.95047 * inverse(fx);
        double y = inverse(fy);
        double z = 1.08883 * inverse(fz);
        r = x * 3.2404542 + y * -1.5371385 + z * -0.4985314;
        g = x * -0.9692660 + y * 1.8760108 + z * 0.0415560;
        blue = x * 0.0556434 + y * -0.2040259 + z * 1.0572252;
    };
    double r = 0.0, g = 0.0, blue = 0.0;
    linear_rgb(1.0, r, g, blue);
    auto in_gamut = [](double value) { return value >= 0.0 && value <= 1.0; };
    if(!in_gamut(r) || !in_gamut(g) || !in_gamut(blue)) {
        double low = 0.0, high = 1.0;
        for(int i = 0; i < 18; i++) {
            double scale = (low + high) * 0.5;
            linear_rgb(scale, r, g, blue);
            if(in_gamut(r) && in_gamut(g) && in_gamut(blue))
                low = scale;
            else
                high = scale;
        }
        linear_rgb(low, r, g, blue);
    }
    auto encode = [](double value) {
        value = value <= 0.0031308 ? 12.92 * value
                                  : 1.055 * pow(max(0.0, value), 1.0 / 2.4) - 0.055;
        return ClampByte_(int(minmax(value, 0.0, 1.0) * 255.0 + 0.5));
    };
    return Color(encode(r), encode(g), encode(blue));
}

// TMI is a grading-oriented model: Temperature (blue/orange),
// Magenta/Green, and Intensity. This is deliberately an intuitive,
// bounded display transform rather than a spectral colour-temperature model.
static void ColorToTmi_(Color color, double& temperature, double& magenta, double& intensity)
{
    double r = color.GetR() / 255.0;
    double g = color.GetG() / 255.0;
    double b = color.GetB() / 255.0;
    intensity = minmax((r + g + b) / 3.0 * 100.0, 0.0, 100.0);
    temperature = minmax((r - b) * 100.0, -100.0, 100.0);
    magenta = minmax((((r + b) * 0.5) - g) * 100.0, -100.0, 100.0);
}

static Color TmiToColor_(double temperature, double magenta, double intensity)
{
    double base = minmax(intensity, 0.0, 100.0) / 100.0;
    double t = minmax(temperature, -100.0, 100.0) / 100.0;
    double m = minmax(magenta, -100.0, 100.0) / 100.0;

    double r = base + t * 0.35 + m * 0.24;
    double b = base - t * 0.35 + m * 0.24;
    double g = base - m * 0.48;

    double maximum = max(r, max(g, b));
    double minimum = min(r, min(g, b));
    if(maximum > 1.0) {
        r /= maximum;
        g /= maximum;
        b /= maximum;
    }
    if(minimum < 0.0) {
        r -= minimum;
        g -= minimum;
        b -= minimum;
        maximum = max(r, max(g, b));
        if(maximum > 1.0) {
            r /= maximum;
            g /= maximum;
            b /= maximum;
        }
    }

    return Color(ClampByte_(int(ClampUnit_(r) * 255.0 + 0.5)),
                 ClampByte_(int(ClampUnit_(g) * 255.0 + 0.5)),
                 ClampByte_(int(ClampUnit_(b) * 255.0 + 0.5)));
}

static String FormatHex_(Color color)
{
    return Format("#%02X%02X%02X", color.GetR(), color.GetG(), color.GetB());
}

static String FormatHex8_(Color color, int alpha)
{
    return Format("#%02X%02X%02X%02X",
                  color.GetR(), color.GetG(), color.GetB(), ClampByte_(alpha));
}

static bool IsHexDigit_(int character)
{
    return (character >= '0' && character <= '9')
        || (character >= 'a' && character <= 'f')
        || (character >= 'A' && character <= 'F');
}

static int HexValue_(int character)
{
    if(character >= '0' && character <= '9')
        return character - '0';
    if(character >= 'a' && character <= 'f')
        return character - 'a' + 10;
    return character - 'A' + 10;
}

struct ParsedNumber_ : Moveable<ParsedNumber_> {
    double value = 0.0;
    bool percent = false;
};

struct ParsedColor_ {
    Color color = Black();
    int alpha = 255;
    bool has_alpha = false;
};

static Vector<ParsedNumber_> ExtractNumbers_(const String& text)
{
    Vector<ParsedNumber_> values;
    const char *cursor = text.Begin();

    while(cursor && *cursor) {
        bool number_start = (*cursor >= '0' && *cursor <= '9')
                         || *cursor == '+' || *cursor == '-'
                         || *cursor == '.';
        if(!number_start) {
            cursor++;
            continue;
        }

        char *end = nullptr;
        double value = std::strtod(cursor, &end);
        if(end == cursor) {
            cursor++;
            continue;
        }

        while(*end == ' ' || *end == '\t')
            end++;

        ParsedNumber_& parsed = values.Add();
        parsed.value = value;
        parsed.percent = *end == '%';
        if(parsed.percent)
            end++;
        cursor = end;
    }

    return values;
}

static bool ParseHexColor_(String text, ParsedColor_& output)
{
    text = TrimBoth(text);
    if(text.StartsWith("#"))
        text = text.Mid(1);
    else if(text.StartsWith("0x") || text.StartsWith("0X"))
        text = text.Mid(2);
    else
        return false;

    int count = text.GetCount();
    if(count != 3 && count != 4 && count != 6 && count != 8)
        return false;

    for(int i = 0; i < count; i++)
        if(!IsHexDigit_((byte)text[i]))
            return false;

    int r = 0;
    int g = 0;
    int b = 0;
    int a = 255;

    if(count == 3 || count == 4) {
        r = HexValue_(text[0]) * 17;
        g = HexValue_(text[1]) * 17;
        b = HexValue_(text[2]) * 17;
        if(count == 4) {
            a = HexValue_(text[3]) * 17;
            output.has_alpha = true;
        }
    }
    else {
        r = HexValue_(text[0]) * 16 + HexValue_(text[1]);
        g = HexValue_(text[2]) * 16 + HexValue_(text[3]);
        b = HexValue_(text[4]) * 16 + HexValue_(text[5]);
        if(count == 8) {
            a = HexValue_(text[6]) * 16 + HexValue_(text[7]);
            output.has_alpha = true;
        }
    }

    output.color = Color(r, g, b);
    output.alpha = a;
    return true;
}

static double PercentOrUnit_(const ParsedNumber_& value)
{
    if(value.percent)
        return value.value;
    if(fabs(value.value) <= 1.0)
        return value.value * 100.0;
    return value.value;
}

static int ParseAlpha_(const ParsedNumber_& value)
{
    if(value.percent)
        return ClampByte_(int(value.value * 2.55 + 0.5));
    if(value.value >= 0.0 && value.value <= 1.0)
        return ClampByte_(int(value.value * 255.0 + 0.5));
    return ClampByte_(int(value.value + 0.5));
}

static bool ParseColorText_(const String& source, ParsedColor_& output)
{
    String text = TrimBoth(source);
    if(text.IsEmpty())
        return false;

    if(ParseHexColor_(text, output))
        return true;

    if(text.GetCount() == 6 || text.GetCount() == 8) {
        bool all_hex = true;
        bool has_alpha_hex = false;
        for(int i = 0; i < text.GetCount(); i++) {
            all_hex = all_hex && IsHexDigit_((byte)text[i]);
            has_alpha_hex = has_alpha_hex || ((text[i] >= 'A' && text[i] <= 'F') || (text[i] >= 'a' && text[i] <= 'f'));
        }
        if(all_hex && has_alpha_hex) {
            String candidate = "#";
            candidate << text;
            if(ParseHexColor_(candidate, output))
                return true;
        }
    }

    String lower = ToLower(text);
    Vector<ParsedNumber_> numbers = ExtractNumbers_(text);

    bool is_rgba = lower.StartsWith("rgba");
    bool is_rgb = is_rgba || lower.StartsWith("rgb");
    bool is_hsva = lower.StartsWith("hsva");
    bool is_hsv = is_hsva || lower.StartsWith("hsv");
    bool is_hsla = lower.StartsWith("hsla") || lower.StartsWith("hlsa");
    bool is_hsl = is_hsla || lower.StartsWith("hsl") || lower.StartsWith("hls");
    bool is_cmyka = lower.StartsWith("cmyka");
    bool is_cmyk = is_cmyka || lower.StartsWith("cmyk");
    bool is_tmia = lower.StartsWith("tmia");
    bool is_tmi = is_tmia || lower.StartsWith("tmi");

    if(is_rgb) {
        if(numbers.GetCount() < 3 || numbers.GetCount() > 4)
            return false;
        bool normalized = !numbers[0].percent && !numbers[1].percent && !numbers[2].percent
                       && fabs(numbers[0].value) <= 1.0
                       && fabs(numbers[1].value) <= 1.0
                       && fabs(numbers[2].value) <= 1.0;
        auto rgb_component = [&](const ParsedNumber_& component) {
            if(component.percent)
                return int(component.value * 2.55 + 0.5);
            if(normalized)
                return int(component.value * 255.0 + 0.5);
            return int(component.value + 0.5);
        };
        int r = rgb_component(numbers[0]);
        int g = rgb_component(numbers[1]);
        int b = rgb_component(numbers[2]);
        output.color = Color(ClampByte_(r), ClampByte_(g), ClampByte_(b));
        if(numbers.GetCount() == 4) {
            output.alpha = ParseAlpha_(numbers[3]);
            output.has_alpha = true;
        }
        return true;
    }

    if(is_hsv) {
        if(numbers.GetCount() < 3 || numbers.GetCount() > 4)
            return false;
        output.color = HsvToColor_(numbers[0].value,
                                   PercentOrUnit_(numbers[1]),
                                   PercentOrUnit_(numbers[2]));
        if(numbers.GetCount() == 4) {
            output.alpha = ParseAlpha_(numbers[3]);
            output.has_alpha = true;
        }
        return true;
    }

    if(is_hsl) {
        if(numbers.GetCount() < 3 || numbers.GetCount() > 4)
            return false;
        output.color = HslToColor_(numbers[0].value,
                                   PercentOrUnit_(numbers[1]),
                                   PercentOrUnit_(numbers[2]));
        if(numbers.GetCount() == 4) {
            output.alpha = ParseAlpha_(numbers[3]);
            output.has_alpha = true;
        }
        return true;
    }

    if(is_cmyk) {
        if(numbers.GetCount() < 4 || numbers.GetCount() > 5)
            return false;
        output.color = CmykToColor_(PercentOrUnit_(numbers[0]),
                                    PercentOrUnit_(numbers[1]),
                                    PercentOrUnit_(numbers[2]),
                                    PercentOrUnit_(numbers[3]));
        if(numbers.GetCount() == 5) {
            output.alpha = ParseAlpha_(numbers[4]);
            output.has_alpha = true;
        }
        return true;
    }

    if(is_tmi) {
        if(numbers.GetCount() < 3 || numbers.GetCount() > 4)
            return false;
        output.color = TmiToColor_(numbers[0].value,
                                   numbers[1].value,
                                   PercentOrUnit_(numbers[2]));
        if(numbers.GetCount() == 4) {
            output.alpha = ParseAlpha_(numbers[3]);
            output.has_alpha = true;
        }
        return true;
    }

    // Bare comma/space separated tuples are accepted as RGB/RGBA.
    if(numbers.GetCount() == 3 || numbers.GetCount() == 4) {
        bool normalized = true;
        for(int i = 0; i < 3; i++)
            normalized = normalized && !numbers[i].percent
                                  && numbers[i].value >= 0.0
                                  && numbers[i].value <= 1.0;
        int r = normalized ? int(numbers[0].value * 255.0 + 0.5) : int(numbers[0].value + 0.5);
        int g = normalized ? int(numbers[1].value * 255.0 + 0.5) : int(numbers[1].value + 0.5);
        int b = normalized ? int(numbers[2].value * 255.0 + 0.5) : int(numbers[2].value + 0.5);
        output.color = Color(ClampByte_(r), ClampByte_(g), ClampByte_(b));
        if(numbers.GetCount() == 4) {
            output.alpha = ParseAlpha_(numbers[3]);
            output.has_alpha = true;
        }
        return true;
    }

    return false;
}

static bool LooksLikeColorExpression_(const String& text)
{
    String value = TrimBoth(text);
    String lower = ToLower(value);
    return value.StartsWith("#")
        || value.StartsWith("0x") || value.StartsWith("0X")
        || value.Find(',') >= 0
        || value.Find('(') >= 0
        || lower.StartsWith("rgb")
        || lower.StartsWith("hsv")
        || lower.StartsWith("hsl") || lower.StartsWith("hls")
        || lower.StartsWith("cmyk")
        || lower.StartsWith("tmi");
}

static bool ParseSingleNumber_(const String& text, double& value)
{
    String trimmed = TrimBoth(text);
    if(trimmed.IsEmpty() || LooksLikeColorExpression_(trimmed))
        return false;

    const char *begin = trimmed.Begin();
    char *end = nullptr;
    value = std::strtod(begin, &end);
    if(end == begin)
        return false;
    while(*end == ' ' || *end == '\t')
        end++;
    return *end == '\0';
}

static UiColorPicker::SlotValue MakeSlot_(Color color, int alpha = 255, const String& label = String())
{
    UiColorPicker::SlotValue value;
    value.color = color;
    value.alpha = ClampByte_(alpha);
    value.label = label;
    return value;
}

struct PaletteDefinition_ : Moveable<PaletteDefinition_> {
    String name;
    String category;
    int columns = 1;
    int rows = 1;
    String badge;
    Vector<UiColorPicker::SlotValue> colors;
};

static void AddPaletteColor_(PaletteDefinition_& palette, Color color, const String& label = String())
{
    palette.colors.Add(MakeSlot_(color, 255, label));
}

static PaletteDefinition_ MakeSpectrumPalette_()
{
    PaletteDefinition_ palette;
    palette.name = "Full Spectrum";
    palette.category = "General";
    palette.columns = 12;
    palette.rows = 8;
    palette.badge = "sRGB";

    for(int row = 0; row < palette.rows; row++) {
        double value = 100.0 - row * 9.0;
        double saturation = 96.0 - row * 2.0;
        for(int column = 0; column < palette.columns; column++) {
            int hue = column * 30;
            AddPaletteColor_(palette,
                             HsvToColor_(hue, saturation, value),
                             Format("H%03d V%02d", hue, int(value)));
        }
    }
    return palette;
}

static PaletteDefinition_ MakeStrongPalette_()
{
    PaletteDefinition_ palette;
    palette.name = "Strong / Vivid";
    palette.category = "General";
    palette.columns = 12;
    palette.rows = 6;
    palette.badge = "sRGB";

    for(int row = 0; row < palette.rows; row++) {
        double value = 100.0 - row * 11.0;
        double saturation = 100.0 - row * 4.0;
        for(int column = 0; column < palette.columns; column++) {
            int hue = column * 30;
            AddPaletteColor_(palette, HsvToColor_(hue, saturation, value));
        }
    }
    return palette;
}

static PaletteDefinition_ MakePastelPalette_()
{
    PaletteDefinition_ palette;
    palette.name = "Pastel / Soft Light";
    palette.category = "General";
    palette.columns = 12;
    palette.rows = 6;
    palette.badge = "sRGB";

    for(int row = 0; row < palette.rows; row++) {
        double saturation = 48.0 - row * 3.0;
        double lightness = 92.0 - row * 7.0;
        for(int column = 0; column < palette.columns; column++)
            AddPaletteColor_(palette, HslToColor_(column * 30, saturation, lightness));
    }
    return palette;
}

static PaletteDefinition_ MakeEarthPalette_()
{
    PaletteDefinition_ palette;
    palette.name = "Earth / Muted";
    palette.category = "General";
    palette.columns = 12;
    palette.rows = 6;
    palette.badge = "sRGB";

    const int hues[12] = { 12, 22, 32, 42, 54, 68, 84, 102, 122, 148, 178, 205 };
    for(int row = 0; row < palette.rows; row++) {
        double saturation = 58.0 - row * 5.0;
        double lightness = 27.0 + row * 10.0;
        for(int column = 0; column < palette.columns; column++)
            AddPaletteColor_(palette, HslToColor_(hues[column], saturation, lightness));
    }
    return palette;
}

static PaletteDefinition_ MakeBrightUiPalette_()
{
    PaletteDefinition_ palette;
    palette.name = "Bright UI / Accents";
    palette.category = "UI & Web";
    palette.columns = 5;
    palette.rows = 5;
    palette.badge = "Semantic";

    const Color bases[5] = {
        Color(34, 197, 94),   // success
        Color(245, 158, 11),  // warning
        Color(239, 68, 68),   // error
        Color(59, 130, 246),  // information
        Color(168, 85, 247)   // brand/accent
    };
    const int white_mix[5] = { 180, 105, 0, -55, -115 };

    for(int row = 0; row < 5; row++) {
        for(int column = 0; column < 5; column++) {
            Color color = white_mix[row] >= 0
                        ? Blend(bases[column], White(), white_mix[row])
                        : Blend(bases[column], Black(), -white_mix[row]);
            AddPaletteColor_(palette, color);
        }
    }
    return palette;
}

static PaletteDefinition_ MakeGreyPalette_()
{
    PaletteDefinition_ palette;
    palette.name = "Grey / Scale";
    palette.category = "General";
    palette.columns = 1;
    palette.rows = 12;
    palette.badge = "Achromatic";

    for(int row = 0; row < palette.rows; row++) {
        double t = palette.rows <= 1 ? 0.0 : double(row) / double(palette.rows - 1);
        int level = ClampByte_(int((1.0 - t) * 255.0 + 0.5));
        AddPaletteColor_(palette, Color(level, level, level), Format("Grey %d", level));
    }
    return palette;
}

static PaletteDefinition_ MakeWebSafePalette_()
{
    PaletteDefinition_ palette;
    palette.name = "WebSafe Subset";
    palette.category = "UI & Web";
    palette.columns = 6;
    palette.rows = 6;
    palette.badge = "36 / 216";

    const int hue_components[6][3] = {
        { 255,   0,   0 },
        { 255, 255,   0 },
        {   0, 255,   0 },
        {   0, 255, 255 },
        {   0,   0, 255 },
        { 255,   0, 255 }
    };

    for(int row = 0; row < 6; row++) {
        for(int column = 0; column < 6; column++) {
            int level = (column + 1) * 51;
            Color color(hue_components[row][0] ? level : 0,
                        hue_components[row][1] ? level : 0,
                        hue_components[row][2] ? level : 0);
            AddPaletteColor_(palette, color);
        }
    }
    return palette;
}

static PaletteDefinition_ MakeHdriPalette_()
{
    PaletteDefinition_ palette;
    palette.name = "HDRI / Wide Gamut";
    palette.category = "UI & Web";
    palette.columns = 12;
    palette.rows = 6;
    palette.badge = "P3 preview";

    for(int row = 0; row < palette.rows; row++) {
        double saturation = 100.0 - row * 3.0;
        double value = 100.0 - row * 7.0;
        for(int column = 0; column < palette.columns; column++) {
            int hue = NormalizeHue_(column * 30 + (row & 1 ? 8 : 0));
            AddPaletteColor_(palette, HsvToColor_(hue, saturation, value));
        }
    }
    return palette;
}

static PaletteDefinition_ MakeXritePalette_()
{
    PaletteDefinition_ palette;
    palette.name = "X-Rite ColorChecker";
    palette.category = "Reference";
    palette.columns = 6;
    palette.rows = 4;
    palette.badge = "24 patch";

    const Color values[24] = {
        Color(0x73,0x52,0x44), Color(0xC2,0x96,0x82), Color(0x62,0x7A,0x9D), Color(0x57,0x6C,0x43),
        Color(0x85,0x80,0xB1), Color(0x67,0xBD,0xAA), Color(0xD6,0x7E,0x2C), Color(0x50,0x5B,0xA6),
        Color(0xC1,0x5A,0x63), Color(0x5E,0x3C,0x73), Color(0x9D,0xBC,0x40), Color(0xE0,0xA3,0x2E),
        Color(0x38,0x3D,0x88), Color(0x46,0x94,0x49), Color(0xAF,0x36,0x3C), Color(0xF3,0xC3,0x00),
        Color(0x9C,0x5A,0xA5), Color(0x00,0xA1,0xC7), Color(0xF3,0xF3,0xF2), Color(0xC8,0xC8,0xC8),
        Color(0xA0,0xA0,0xA0), Color(0x7A,0x7A,0x7A), Color(0x55,0x55,0x55), Color(0x34,0x34,0x34)
    };
    for(int i = 0; i < 24; i++)
        AddPaletteColor_(palette, values[i], Format("Patch %02d", i + 1));
    return palette;
}

static PaletteDefinition_ MakeSkinPalette_()
{
    PaletteDefinition_ palette;
    palette.name = "Skin Tones";
    palette.category = "Reference";
    palette.columns = 1;
    palette.rows = 11;
    palette.badge = "Monk scale";

    // Screen approximations of the eleven-point Monk Skin Tone scale.
    const Color values[11] = {
        Color(246, 237, 228), Color(243, 231, 219), Color(247, 234, 208),
        Color(234, 218, 186), Color(215, 189, 150), Color(181, 151, 111),
        Color(160, 126, 86),  Color(130, 92, 67),   Color(96, 65, 52),
        Color(62, 43, 35),    Color(41, 30, 25)
    };
    for(int i = 0; i < 11; i++)
        AddPaletteColor_(palette, values[i], Format("MST %d", i + 1));
    return palette;
}

static PaletteDefinition_ MakeIndustryPalette_()
{
    PaletteDefinition_ palette;
    palette.name = "Industry Ref";
    palette.category = "Reference";
    palette.columns = 10;
    palette.rows = 10;
    palette.badge = "Screen ref";

    const Color bases[10] = {
        Color(186, 12, 47), Color(224, 82, 6), Color(239, 179, 0), Color(173, 165, 0),
        Color(0, 137, 85), Color(0, 143, 156), Color(0, 102, 161), Color(52, 63, 139),
        Color(112, 48, 160), Color(173, 34, 109)
    };
    const int mix[10] = { 190, 150, 110, 70, 30, 0, -28, -56, -84, -116 };

    for(int row = 0; row < 10; row++) {
        for(int column = 0; column < 10; column++) {
            Color color = mix[row] >= 0
                        ? Blend(bases[column], White(), mix[row])
                        : Blend(bases[column], Black(), -mix[row]);
            AddPaletteColor_(palette, color, Format("REF %02d-%02d", row + 1, column + 1));
        }
    }
    return palette;
}

static const Vector<PaletteDefinition_>& PaletteRegistry_()
{
    static Vector<PaletteDefinition_> palettes;
    if(palettes.IsEmpty()) {
        palettes.Add(MakeSpectrumPalette_());
        palettes.Add(MakeStrongPalette_());
        palettes.Add(MakePastelPalette_());
        palettes.Add(MakeEarthPalette_());
        palettes.Add(MakeBrightUiPalette_());
        palettes.Add(MakeGreyPalette_());
        palettes.Add(MakeWebSafePalette_());
        palettes.Add(MakeHdriPalette_());
        palettes.Add(MakeXritePalette_());
        palettes.Add(MakeSkinPalette_());
        palettes.Add(MakeIndustryPalette_());

#ifdef _DEBUG
        for(const PaletteDefinition_& palette : palettes) {
            ASSERT(palette.colors.GetCount() == palette.columns * palette.rows);
            for(int i = 0; i < palette.colors.GetCount(); i++)
                for(int j = i + 1; j < palette.colors.GetCount(); j++)
                    ASSERT(palette.colors[i].color != palette.colors[j].color);
        }
#endif
    }
    return palettes;
}

static int FindPaletteCategory_(const String& category)
{
    if(category == "General")
        return 0;
    if(category == "UI & Web")
        return 1;
    return 2;
}

static Vector<UiColorPicker::SlotValue> BuildHarmonyPalette_(Color base,
                                                             UiColorPicker::HarmonyMode mode,
                                                             const Vector<UiColorPicker::SlotValue>& slots)
{
    Vector<UiColorPicker::SlotValue> output;
    int hue = 0;
    int saturation = 0;
    int value = 0;
    ColorToHsv_(base, hue, saturation, value);

    auto add_hue = [&](double hue_offset, double saturation_scale, double value_scale, const String& label) {
        output.Add(MakeSlot_(HsvToColor_(hue + hue_offset,
                                        minmax(saturation * saturation_scale, 0.0, 100.0),
                                        minmax(value * value_scale, 0.0, 100.0)),
                             255,
                             label));
    };

    switch(mode) {
    case UiColorPicker::HARMONY_CUSTOM:
        for(const UiColorPicker::SlotValue& slot : slots)
            output.Add(slot);
        break;

    case UiColorPicker::HARMONY_ANALOGOUS:
        for(int i = -2; i <= 2; i++)
            add_hue(i * 30.0, 1.0, 1.0 - fabs(i) * 0.05, "Analogous");
        break;

    case UiColorPicker::HARMONY_COMPLEMENTARY:
        for(int row = 0; row < 3; row++) {
            double scale = 1.0 - row * 0.16;
            add_hue(0, 1.0, scale, "Primary");
            add_hue(180, 1.0, scale, "Complement");
        }
        break;

    case UiColorPicker::HARMONY_SPLIT_COMPLEMENTARY:
        for(int row = 0; row < 3; row++) {
            double scale = 1.0 - row * 0.14;
            add_hue(0, 1.0, scale, "Primary");
            add_hue(150, 1.0, scale, "Split A");
            add_hue(210, 1.0, scale, "Split B");
        }
        break;

    case UiColorPicker::HARMONY_TRIAD:
        for(int row = 0; row < 3; row++) {
            double scale = 1.0 - row * 0.14;
            add_hue(0, 1.0, scale, "Triad A");
            add_hue(120, 1.0, scale, "Triad B");
            add_hue(240, 1.0, scale, "Triad C");
        }
        break;

    case UiColorPicker::HARMONY_SQUARE:
        for(int row = 0; row < 2; row++) {
            double scale = 1.0 - row * 0.18;
            add_hue(0, 1.0, scale, "Square A");
            add_hue(90, 1.0, scale, "Square B");
            add_hue(180, 1.0, scale, "Square C");
            add_hue(270, 1.0, scale, "Square D");
        }
        break;

    case UiColorPicker::HARMONY_COMPOUND: {
        const int offsets[6] = { 0, 30, 150, 180, 210, 330 };
        for(int offset : offsets)
            add_hue(offset, offset == 0 ? 1.0 : 0.84, 1.0, "Compound");
        break;
    }

    case UiColorPicker::HARMONY_SHADES:
        for(int i = 0; i < 12; i++)
            output.Add(MakeSlot_(HsvToColor_(hue, saturation, 100.0 - i * 7.0), 255, "Shade"));
        break;

    case UiColorPicker::HARMONY_MONOCHROMATIC:
        for(int i = 0; i < 12; i++) {
            double s = 25.0 + (i % 4) * 20.0;
            double v = 100.0 - (i / 4) * 22.0;
            output.Add(MakeSlot_(HsvToColor_(hue, s, v), 255, "Monochrome"));
        }
        break;

    case UiColorPicker::HARMONY_IMAGE_EXTRACT:
        break;
    }

    return output;
}

struct RgbSample_ : Moveable<RgbSample_> {
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
};

static Vector<UiColorPicker::SlotValue> ExtractImagePalette_(const Image& image, int requested_count)
{
    Vector<UiColorPicker::SlotValue> output;
    if(image.IsEmpty())
        return output;

    requested_count = minmax(requested_count, 2, 24);
    Size size = image.GetSize();
    int total = max(1, size.cx * size.cy);
    int step = max(1, int(sqrt(total / 4096.0)));

    Vector<RgbSample_> samples;
    for(int y = 0; y < size.cy; y += step) {
        const RGBA *row = image[y];
        for(int x = 0; x < size.cx; x += step) {
            const RGBA& pixel = row[x];
            if(pixel.a < 16)
                continue;
            RgbSample_& sample = samples.Add();
            sample.r = pixel.r;
            sample.g = pixel.g;
            sample.b = pixel.b;
        }
    }

    if(samples.IsEmpty())
        return output;

    requested_count = min(requested_count, samples.GetCount());
    Vector<RgbSample_> centroids;
    centroids.SetCount(requested_count);
    for(int i = 0; i < requested_count; i++)
        centroids[i] = samples[(i * samples.GetCount()) / requested_count];

    Vector<int> assignment;
    assignment.SetCount(samples.GetCount(), 0);
    Vector<int> population;
    population.SetCount(requested_count, 0);

    for(int iteration = 0; iteration < 12; iteration++) {
        Vector<double> sum_r, sum_g, sum_b;
        sum_r.SetCount(requested_count, 0.0);
        sum_g.SetCount(requested_count, 0.0);
        sum_b.SetCount(requested_count, 0.0);
        population.SetCount(requested_count, 0);

        for(int i = 0; i < samples.GetCount(); i++) {
            int best = 0;
            double best_distance = DBL_MAX;
            for(int c = 0; c < requested_count; c++) {
                double dr = samples[i].r - centroids[c].r;
                double dg = samples[i].g - centroids[c].g;
                double db = samples[i].b - centroids[c].b;
                double distance = dr * dr + dg * dg + db * db;
                if(distance < best_distance) {
                    best_distance = distance;
                    best = c;
                }
            }
            assignment[i] = best;
            sum_r[best] += samples[i].r;
            sum_g[best] += samples[i].g;
            sum_b[best] += samples[i].b;
            population[best]++;
        }

        for(int c = 0; c < requested_count; c++) {
            if(population[c] > 0) {
                centroids[c].r = sum_r[c] / population[c];
                centroids[c].g = sum_g[c] / population[c];
                centroids[c].b = sum_b[c] / population[c];
            }
            else
                centroids[c] = samples[(c * 997) % samples.GetCount()];
        }
    }

    Vector<int> order;
    for(int i = 0; i < requested_count; i++)
        order.Add(i);
    for(int i = 0; i < order.GetCount(); i++) {
        int best = i;
        for(int j = i + 1; j < order.GetCount(); j++)
            if(population[order[j]] > population[order[best]])
                best = j;
        Swap(order[i], order[best]);
    }

    for(int rank = 0; rank < order.GetCount(); rank++) {
        int i = order[rank];
        Color color(ClampByte_(int(centroids[i].r + 0.5)),
                    ClampByte_(int(centroids[i].g + 0.5)),
                    ClampByte_(int(centroids[i].b + 0.5)));
        bool duplicate = false;
        for(const UiColorPicker::SlotValue& existing : output)
            if(existing.color == color) {
                duplicate = true;
                break;
            }
        if(!duplicate)
            output.Add(MakeSlot_(color, 255, Format("Image %d", rank + 1)));
    }

    return output;
}

struct SharedColorPickerSession_ {
    bool initialized = false;
    Vector<UiColorPicker::SlotValue> slots;
    Vector<UiColorPicker::SlotValue> previous;
    Vector<UiColorPicker::SlotValue> recent;
    Vector<UiColorPicker::SlotValue> stash;
    int active_slot = 0;
    bool alpha_enabled = true;
    int page_mode = UiColorPicker::PAGE_COLOR;
    int spectrum_mode = UiColorPicker::SPECTRUM_HUE_STRIP;
    int channel_mode = UiColorPicker::CHANNEL_RGB_FLOAT;
    int harmony_mode = UiColorPicker::HARMONY_TRIAD;
    int palette_category = 0;
    int palette_index = 0;
    int selected_curve_channel = 0;
    int generator_mode = 2;
    int generator_count = 3;
    int generator_gain = 100;
    Vector<Point> generator_handles;
    Vector<int> generator_values;
    Image generator_image;
    ShadowCurve curves[4];

    SharedColorPickerSession_()
    {
        for(int i = 0; i < 4; i++)
            curves[i] = ShadowLinear();
    }
};

static SharedColorPickerSession_& SharedSession_()
{
    static SharedColorPickerSession_ session;
    return session;
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

static void DrawHueTrack_(Draw& draw, const Rect& rect)
{
    if(rect.IsEmpty())
        return;
    int denominator = max(1, rect.GetWidth() - 1);
    for(int x = 0; x < rect.GetWidth(); x++) {
        int hue = ClampHue_(int(x / double(denominator) * 359.0 + 0.5));
        draw.DrawRect(rect.left + x, rect.top, 1, rect.GetHeight(),
                      HsvToColor_(hue, 100, 100));
    }
}

static void DrawValueTrack_(Draw& draw, const Rect& rect, int hue, int saturation)
{
    if(rect.IsEmpty())
        return;
    int denominator = max(1, rect.GetWidth() - 1);
    for(int x = 0; x < rect.GetWidth(); x++) {
        double value = x / double(denominator) * 100.0;
        draw.DrawRect(rect.left + x, rect.top, 1, rect.GetHeight(),
                      HsvToColor_(hue, saturation, value));
    }
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

} // namespace

class ColorDragSource_ {
public:
    virtual ~ColorDragSource_() {}
    virtual UiColorPicker::SlotValue GetColorDragValue() const = 0;
};

class UiColorPicker::ReadoutRow : public ParentCtrl {
public:
    typedef ReadoutRow CLASSNAME;

    ReadoutRow()
    {
        Add(title_);
        Add(edit_);
        Add(copy_);

        title_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        edit_.SetTextAlign(UiAlign::LEFT);
        edit_.SetAcceptsDrop(true);
        UiLineEdit::Style edit_style = UiTheme::ResolveEdit(UiRole::Subtle);
        edit_style.font = Monospace().Height(DPI(10));
        edit_.SetCustomStyle(edit_style);
        title_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
        copy_.SetText("")
             .SetIcon(ICON_CONTENT_CONTENT_COPY_48())
             .SetIconSize(DPI(14), DPI(14))
             .SetIconRenderMode(UiIconRenderMode::MonoTint);

        edit_.WhenChange = [=] {
            if(syncing_)
                return;
            if(WhenLiveText)
                WhenLiveText(edit_.GetTextUtf8());
        };
        edit_.WhenAction = [=] { Commit(); };
        edit_.WhenCommit = [=] { Commit(); };
        copy_.WhenAction = [=] {
            if(!edit_.IsEmpty())
                WriteClipboardText(edit_.GetTextUtf8());
        };
    }

    void SetTitle(const String& text)
    {
        title_.SetText(text);
    }

    void SetValue(const String& text)
    {
        if(edit_.HasFocus() && edit_.IsDirty())
            return;
        if(edit_.GetTextUtf8() == text)
            return;
        syncing_ = true;
        edit_.SetTextUtf8(text);
        edit_.ClearDirty();
        syncing_ = false;
    }

    String GetValue() const
    {
        return edit_.GetTextUtf8();
    }

    void SetPlaceholder(const String& text)
    {
        edit_.SetPlaceholder(text);
    }

    Event<String> WhenLiveText;
    Event<String> WhenCommitText;

    virtual void Layout() override
    {
        Rect rect(Point(0, 0), GetSize());
        int copy_width = DPI(26);
        int title_height = DPI(16);
        title_.SetRect(rect.left + DPI(4), rect.top, max(0, rect.GetWidth() - DPI(8)), title_height);
        copy_.SetRect(rect.right - copy_width, rect.top + title_height,
                      copy_width, max(0, rect.GetHeight() - title_height));
        edit_.SetRect(rect.left, rect.top + title_height,
                      max(0, rect.GetWidth() - copy_width - DPI(2)),
                      max(0, rect.GetHeight() - title_height));
    }

    virtual Size GetMinSize() const override
    {
        return Size(DPI(150), DPI(40));
    }

private:
    void Commit()
    {
        if(syncing_)
            return;
        String text = edit_.GetTextUtf8();
        edit_.ClearDirty();
        if(WhenCommitText)
            WhenCommitText(text);
    }

    UiLabel         title_;
    CommitLineEdit_ edit_;
    UiToolButton    copy_;
    bool            syncing_ = false;
};

class UiColorPicker::ColorField : public Ctrl {
public:
    typedef ColorField CLASSNAME;

    ColorField()
    {
        NoWantFocus();
    }

    void SetState(SpectrumMode mode, Color color, int hue, int gain)
    {
        bool cache_changed = mode_ != mode;
        if(mode == SPECTRUM_HSV_RECT)
            cache_changed = cache_changed || hue_ != hue;
        else if(mode == SPECTRUM_RGB_SPECTRUM)
            cache_changed = cache_changed || color_.GetB() != color.GetB();
        else if(mode == SPECTRUM_HUE_STRIP)
            cache_changed = cache_changed || gain_ != gain;

        mode_ = mode;
        color_ = color;
        hue_ = hue;
        gain_ = minmax(gain, 0, 100);

        if(cache_changed) {
            cache_ = Image();
            cache_key_ = UiRasterCacheKey();
        }
        Refresh();
    }

    Event<Point, bool> WhenPick;

    virtual void Paint(Draw& draw) override
    {
        Rect rect(Point(0, 0), GetSize());
        if(rect.IsEmpty())
            return;

        const Image& image = EnsureCache();
        if(!image.IsEmpty())
            draw.DrawImage(rect.left, rect.top, image);

        Color frame = Blend(SColorShadow(), SColorPaper(), 90);
        DrawFrame_(draw, rect, frame);

        Point marker = GetMarkerPosition();
        Rect outer = RectC(rect.left + marker.x - DPI(5),
                           rect.top + marker.y - DPI(5),
                           DPI(10), DPI(10));
        DrawFrame_(draw, outer, White(), DPI(2));
        DrawFrame_(draw, outer.Deflated(DPI(2)), Black(), DPI(1));
    }

    virtual void LeftDown(Point point, dword) override
    {
        SetCapture();
        if(WhenPick)
            WhenPick(point, false);
    }

    virtual void MouseMove(Point point, dword flags) override
    {
        if(!HasCapture() || !(flags & K_MOUSELEFT))
            return;
        if(WhenPick)
            WhenPick(point, false);
    }

    virtual void LeftUp(Point point, dword) override
    {
        if(HasCapture())
            ReleaseCapture();
        if(WhenPick)
            WhenPick(point, true);
    }

    virtual Size GetMinSize() const override
    {
        return Size(DPI(260), DPI(260));
    }

private:
    const Image& EnsureCache() const
    {
        Size size = GetSize();
        if(size.IsEmpty())
            return cache_;

        UiRasterCacheKeyBuilder key_builder("uicolor-field");
        key_builder.Add(size).Add((int)mode_);
        if(mode_ == SPECTRUM_HSV_RECT)
            key_builder.Add(hue_);
        else if(mode_ == SPECTRUM_HUE_STRIP)
            key_builder.Add(gain_);
        else if(mode_ == SPECTRUM_RGB_SPECTRUM)
            key_builder.Add(color_.GetB());

        UiRasterCacheKey key = key_builder.Build();
        if(cache_.IsEmpty() || !(key == cache_key_)) {
            UiRasterCachePolicy policy = UiRasterPolicyAA("uicolor-field");
            policy.allow_scale_from_bucket = false;
            policy.max_single_image_bytes = 2 * 1024 * 1024;
            cache_ = UiRasterCache::Get(key, policy, [=] {
                ImageBuffer buffer(size);
                Fill(~buffer, RGBAZero(), buffer.GetLength());
                for(int y = 0; y < size.cy; y++) {
                    RGBA *row = buffer[y];
                    for(int x = 0; x < size.cx; x++) {
                        row[x] = RGBA(SampleAt(Point(x, y)));
                        row[x].a = 255;
                    }
                }
                return Image(buffer);
            });
            cache_key_ = key;
        }
        return cache_;
    }

    Color SampleAt(Point point) const
    {
        int width = max(1, GetSize().cx - 1);
        int height = max(1, GetSize().cy - 1);
        int x = minmax(point.x, 0, width);
        int y = minmax(point.y, 0, height);

        switch(mode_) {
        case SPECTRUM_HUE_STRIP: {
            int hue = ClampHue_(int(x / double(width) * 359.0 + 0.5));
            int saturation = 100 - int(y / double(height) * 100.0 + 0.5);
            return HsvToColor_(hue, saturation, gain_);
        }

        case SPECTRUM_RGB_SPECTRUM: {
            int red = int(x / double(width) * 255.0 + 0.5);
            int green = 255 - int(y / double(height) * 255.0 + 0.5);
            return Color(ClampByte_(red), ClampByte_(green), color_.GetB());
        }

        case SPECTRUM_HSV_RECT:
        default: {
            int saturation = int(x / double(width) * 100.0 + 0.5);
            int value = 100 - int(y / double(height) * 100.0 + 0.5);
            return HsvToColor_(hue_, saturation, value);
        }
        }
    }

    Point GetMarkerPosition() const
    {
        int width = max(1, GetSize().cx - 1);
        int height = max(1, GetSize().cy - 1);
        int hue = 0;
        int saturation = 0;
        int value = 0;
        ColorToHsv_(color_, hue, saturation, value);

        switch(mode_) {
        case SPECTRUM_HUE_STRIP:
            return Point(int(hue / 359.0 * width + 0.5),
                         int((100 - saturation) / 100.0 * height + 0.5));

        case SPECTRUM_RGB_SPECTRUM:
            return Point(int(color_.GetR() / 255.0 * width + 0.5),
                         int((255 - color_.GetG()) / 255.0 * height + 0.5));

        case SPECTRUM_HSV_RECT:
        default:
            return Point(int(saturation / 100.0 * width + 0.5),
                         int((100 - value) / 100.0 * height + 0.5));
        }
    }

    SpectrumMode mode_ = SPECTRUM_HUE_STRIP;
    Color color_ = Color(0, 120, 212);
    int hue_ = 200;
    int gain_ = 100;
    mutable UiRasterCacheKey cache_key_;
    mutable Image cache_;
};

class UiColorPicker::SwatchGrid : public Ctrl, public ColorDragSource_ {
public:
    typedef SwatchGrid CLASSNAME;

    SwatchGrid()
    {
        WantFocus();
    }

    void SetGrid(int columns, int rows)
    {
        columns = max(1, columns);
        rows = max(1, rows);
        if(columns_ == columns && rows_ == rows)
            return;
        columns_ = columns;
        rows_ = rows;
        RefreshLayout();
        Refresh();
    }

    void SetFlow(int columns, bool fit_rows)
    {
        forced_columns_ = max(0, columns);
        fit_rows_ = fit_rows;
        Refresh();
    }

    void SetItems(const Vector<SlotValue>& items)
    {
        items_ = clone(items);
        selected_index_ = min(selected_index_, items_.GetCount() - 1);
        Refresh();
    }

    const Vector<SlotValue>& GetItems() const
    {
        return items_;
    }

    void SetActive(const SlotValue& value)
    {
        if(active_.color == value.color && active_.alpha == value.alpha)
            return;
        active_ = value;
        Refresh();
    }

    void SetSelectedIndex(int index)
    {
        index = index >= 0 && index < items_.GetCount() ? index : -1;
        if(selected_index_ == index)
            return;
        selected_index_ = index;
        Refresh();
    }

    int GetSelectedIndex() const
    {
        return selected_index_;
    }

    SlotValue GetSelectedValue() const
    {
        if(selected_index_ >= 0 && selected_index_ < items_.GetCount())
            return items_[selected_index_];
        SlotValue empty;
        empty.color = Null;
        return empty;
    }

    void EnableDropTarget(bool on = true)
    {
        accept_drop_ = on;
    }

    SlotValue GetDragValue() const
    {
        if(drag_index_ >= 0 && drag_index_ < items_.GetCount())
            return items_[drag_index_];
        SlotValue empty;
        empty.color = Null;
        return empty;
    }

    virtual SlotValue GetColorDragValue() const override { return drag_payload_; }

    Event<int, SlotValue> WhenPick;
    Event<int, SlotValue> WhenDoublePick;
    Event<SlotValue>      WhenDropValue;

    virtual void Paint(Draw& draw) override
    {
        Rect rect(Point(0, 0), GetSize());
        if(rect.IsEmpty())
            return;

        draw.DrawRect(rect, SColorPaper());

        int gap = DPI(4);
        int cell = DPI(30);
        int columns = forced_columns_ > 0 ? forced_columns_
                                         : max(1, (rect.GetWidth() + gap) / (cell + gap));
        int rows = max(1, (items_.GetCount() + columns - 1) / columns);
        if(fit_rows_)
            cell = max(DPI(18), min(cell, (rect.GetHeight() - max(0, rows - 1) * gap) / rows));
        int start_x = rect.left;
        int start_y = rect.top;

        for(int row = 0; row < rows; row++) {
            for(int column = 0; column < columns; column++) {
                int index = row * columns + column;
                Rect cell_rect = RectC(start_x + column * (cell + gap),
                                       start_y + row * (cell + gap),
                                       cell, cell);
                Color empty_face = Blend(SColorPaper(), SColorText(), 12);
                draw.DrawRect(cell_rect, empty_face);

                if(index >= items_.GetCount())
                    continue;

                const SlotValue& value = items_[index];
                DrawAlphaSwatch_(draw, cell_rect.Deflated(DPI(2)),
                                  value.color, value.alpha);

                bool selected = index == selected_index_;
                bool active = value.color == active_.color && value.alpha == active_.alpha;
                Color frame = selected ? SColorHighlight()
                            : active ? Blend(SColorHighlight(), SColorPaper(), 85)
                                     : Blend(SColorShadow(), SColorPaper(), 110);
                DrawFrame_(draw, cell_rect, frame, selected ? DPI(2) : DPI(1));

            }
        }

        if(drop_hot_ && accept_drop_)
            DrawFrame_(draw, rect.Deflated(DPI(1)), SColorHighlight(), DPI(2));
    }

    virtual void LeftDown(Point point, dword) override
    {
        SetFocus();
        drag_index_ = HitTest(point);
        if(drag_index_ >= 0) {
            selected_index_ = drag_index_;
            Refresh();
            if(WhenPick)
                WhenPick(drag_index_, items_[drag_index_]);
        }
    }

    virtual void LeftUp(Point, dword) override
    {
        if(!dragging_)
            drag_index_ = -1;
    }

    virtual void LeftDouble(Point point, dword) override
    {
        int index = HitTest(point);
        if(index >= 0 && WhenDoublePick)
            WhenDoublePick(index, items_[index]);
    }

    virtual void LeftDrag(Point, dword) override
    {
        if(drag_index_ < 0 || drag_index_ >= items_.GetCount())
            return;

        dragging_ = true;
        const SlotValue value = items_[drag_index_];
        drag_payload_ = value;
        VectorMap<String, ClipData> payload = InternalClip<ColorDragSource_>(*this, "uicolor-value");
        payload.Add("application/x-upp-uicolor-swatch-v1",
                    ClipData(FormatHex8_(value.color, value.alpha)));
        Append(payload, FormatHex8_(value.color, value.alpha));
        if(HasCapture())
            ReleaseCapture();
        DoDragAndDrop(payload,
                      MakeAlphaSwatchImage_(value.color,
                                            value.alpha,
                                            Size(DPI(34), DPI(34)),
                                            true),
                      DND_COPY);
        dragging_ = false;
        drag_index_ = -1;
    }

    virtual void DragEnter() override
    {
        if(accept_drop_) {
            drop_hot_ = true;
            Refresh();
        }
    }

    virtual void DragAndDrop(Point, PasteClip& clip) override
    {
        bool internal = accept_drop_ && IsAvailableInternal<ColorDragSource_>(clip, "uicolor-value");
        bool text = accept_drop_ && clip.IsAnyAvailable(ClipFmtsText());
        if(!internal && !text) {
            clip.Reject();
            drop_hot_ = false;
            Refresh();
            return;
        }
        if(internal)
            AcceptInternal<ColorDragSource_>(clip, "uicolor-value");
        else
            AcceptText(clip);
        clip.SetAction(DND_COPY);
        drop_hot_ = true;
        Refresh();

        if(clip.IsPaste()) {
            SlotValue value;
            if(internal) {
                const ColorDragSource_ *source = GetInternalPtr<ColorDragSource_>(clip, "uicolor-value");
                if(source)
                    value = source->GetColorDragValue();
            }
            else {
                Color color;
                int alpha = 255;
                if(UiColorPicker::ParseColorText(GetString(clip), color, alpha)) {
                    value.color = color;
                    value.alpha = alpha;
                }
            }
            if(!IsNull(value.color) && WhenDropValue)
                WhenDropValue(value);
            drop_hot_ = false;
            Refresh();
        }
    }

    virtual void DragLeave() override
    {
        if(drop_hot_) {
            drop_hot_ = false;
            Refresh();
        }
    }

    virtual Size GetMinSize() const override
    {
        int cell = DPI(24);
        int gap = DPI(4);
        return Size(columns_ * cell + max(0, columns_ - 1) * gap,
                    rows_ * cell + max(0, rows_ - 1) * gap);
    }

private:
    int HitTest(Point point) const
    {
        Rect rect(Point(0, 0), GetSize());
        int gap = DPI(4);
        int cell = DPI(30);
        int columns = forced_columns_ > 0 ? forced_columns_
                                         : max(1, (rect.GetWidth() + gap) / (cell + gap));
        int rows = max(1, (items_.GetCount() + columns - 1) / columns);
        if(fit_rows_)
            cell = max(DPI(18), min(cell, (rect.GetHeight() - max(0, rows - 1) * gap) / rows));
        int start_x = rect.left;
        int start_y = rect.top;

        int local_x = point.x - start_x;
        int local_y = point.y - start_y;
        if(local_x < 0 || local_y < 0)
            return -1;

        int stride = cell + gap;
        int column = local_x / max(1, stride);
        int row = local_y / max(1, stride);
        if(column < 0 || column >= columns || row < 0)
            return -1;
        if(local_x % stride >= cell || local_y % stride >= cell)
            return -1;

        int index = row * columns + column;
        return index >= 0 && index < items_.GetCount() ? index : -1;
    }

    int columns_ = 12;
    int rows_ = 1;
    Vector<SlotValue> items_;
    SlotValue active_;
    int selected_index_ = -1;
    int drag_index_ = -1;
    bool dragging_ = false;
    bool accept_drop_ = false;
    bool drop_hot_ = false;
    int forced_columns_ = 0;
    bool fit_rows_ = false;
    SlotValue drag_payload_;
};

class UiColorPicker::ColorSlotButton : public UiToolButton, public ColorDragSource_ {
public:
    typedef ColorSlotButton CLASSNAME;

    enum Kind {
        CURRENT,
        PRIMARY,
        PREVIOUS
    };

    void Configure(UiColorPicker& owner, int slot, Kind kind)
    {
        owner_ = &owner;
        slot_ = slot;
        kind_ = kind;
        accept_drop_ = kind != PREVIOUS;
        SetText("");
        SetContentInset(0);
        SetContentGap(0);
        SetIconRenderMode(UiIconRenderMode::PreserveColor);
        SetAlign(UiAlign::CENTER, UiAlign::CENTER);
    }

    void SetValue(const SlotValue& value, bool active, bool alpha_enabled)
    {
        if(value_ == value && active_ == active && alpha_enabled_ == alpha_enabled)
            return;
        value_ = value;
        active_ = active;
        alpha_enabled_ = alpha_enabled;
        Size swatch_size = kind_ == PREVIOUS ? Size(DPI(38), DPI(6))
                                             : Size(DPI(38), DPI(22));
        SetIcon(MakeAlphaSwatchImage_(value.color,
                                      alpha_enabled ? value.alpha : 255,
                                      swatch_size,
                                      alpha_enabled));
        SetIconSize(swatch_size.cx, swatch_size.cy);
        SetCheckable(kind_ == PRIMARY);
        SetChecked(kind_ == PRIMARY && active);

        if(kind_ == CURRENT)
            Tip(Format("Current: %s", FormatHex8_(value.color, value.alpha)));
        else if(kind_ == PRIMARY)
            Tip(Format("%s: %s", value.label, FormatHex8_(value.color, value.alpha)));
        else
            Tip(Format("Previous %s: %s", value.label, FormatHex8_(value.color, value.alpha)));

        Refresh();
    }

    virtual void Paint(Draw& draw) override
    {
        UiToolButton::Paint(draw);
        if(kind_ == PRIMARY && active_)
            DrawFrame_(draw, Rect(Point(0, 0), GetSize()).Deflated(1),
                       Blend(SColorShadow(), SColorPaper(), 55), DPI(2));
        if(drop_hot_)
            DrawFrame_(draw, Rect(Point(0, 0), GetSize()).Deflated(1),
                       SColorHighlight(), DPI(2));
    }

    virtual void LeftDrag(Point, dword) override
    {
        if(kind_ == CURRENT || IsNull(value_.color))
            return;
        drag_payload_ = value_;
        VectorMap<String, ClipData> payload = InternalClip<ColorDragSource_>(*this, "uicolor-value");
        payload.Add("application/x-upp-uicolor-swatch-v1",
                    ClipData(FormatHex8_(value_.color, value_.alpha)));
        Append(payload, FormatHex8_(value_.color, value_.alpha));
        if(HasCapture())
            ReleaseCapture();
        DoDragAndDrop(payload,
                      MakeAlphaSwatchImage_(value_.color, value_.alpha,
                                            Size(DPI(34), DPI(34)), true),
                      DND_COPY);
    }

    virtual void DragEnter() override
    {
        if(accept_drop_) {
            drop_hot_ = true;
            Refresh();
        }
    }

    virtual void DragAndDrop(Point, PasteClip& clip) override
    {
        bool internal = accept_drop_ && owner_ && IsAvailableInternal<ColorDragSource_>(clip, "uicolor-value");
        bool text = accept_drop_ && owner_ && clip.IsAnyAvailable(ClipFmtsText());
        if(!internal && !text) {
            clip.Reject();
            drop_hot_ = false;
            Refresh();
            return;
        }
        if(internal)
            AcceptInternal<ColorDragSource_>(clip, "uicolor-value");
        else
            AcceptText(clip);
        clip.SetAction(DND_COPY);
        drop_hot_ = true;
        Refresh();

        if(clip.IsPaste()) {
            SlotValue value;
            if(internal) {
                const ColorDragSource_ *source = GetInternalPtr<ColorDragSource_>(clip, "uicolor-value");
                if(source)
                    value = source->GetColorDragValue();
            }
            else {
                Color color;
                int alpha = 255;
                if(UiColorPicker::ParseColorText(GetString(clip), color, alpha)) {
                    value.color = color;
                    value.alpha = alpha;
                }
            }
            if(!IsNull(value.color)) {
                int target = kind_ == CURRENT ? owner_->GetActiveSlot() : slot_;
                owner_->HandleColorDrop(target, value);
            }
            drop_hot_ = false;
            Refresh();
        }
    }

    virtual void DragLeave() override
    {
        if(drop_hot_) {
            drop_hot_ = false;
            Refresh();
        }
    }

private:
    virtual SlotValue GetColorDragValue() const override { return drag_payload_; }
    UiColorPicker *owner_ = nullptr;
    int slot_ = 0;
    Kind kind_ = PRIMARY;
    SlotValue value_;
    bool active_ = false;
    bool alpha_enabled_ = true;
    bool accept_drop_ = true;
    bool drop_hot_ = false;
    SlotValue drag_payload_;
};

class UiColorPicker::ChannelGroup : public ParentCtrl {
public:
    typedef ChannelGroup CLASSNAME;

    struct Row {
        UiLabel label;
        UiSlider slider { UiDirection::H };
        CommitLineEdit_ edit;
        double minimum = 0.0;
        double maximum = 1.0;
        int precision = 4;
        int slider_steps = 10000;
        bool alpha = false;
    };

    ChannelGroup()
    {
        for(int i = 0; i < 5; i++) {
            Add(row_[i].label);
            Add(row_[i].slider);
            Add(row_[i].edit);
            UiLineEdit::Style edit_style = UiTheme::ResolveEdit(UiRole::Subtle);
            edit_style.font = Monospace().Height(DPI(10));
            row_[i].edit.SetCustomStyle(edit_style);
            UiLabel::Style label_style = UiTheme::ResolveLabel(UiRole::Subtle);
            label_style.font = Monospace().Height(DPI(10));
            row_[i].label.SetCustomStyle(label_style);

            const int index = i;
            row_[i].slider.WhenChanging = [=] {
                if(!syncing_ && index < row_count_ && WhenValue)
                    WhenValue(index, SliderToValue(index), false);
            };
            row_[i].slider.WhenAction = [=] {
                if(!syncing_ && index < row_count_ && WhenValue)
                    WhenValue(index, SliderToValue(index), true);
            };
            row_[i].edit.WhenChange = [=] {
                if(syncing_ || index >= row_count_)
                    return;
                String text = row_[index].edit.GetTextUtf8();
                if(LooksLikeColorExpression_(text) && WhenColorText)
                    WhenColorText(text, false);
            };
            row_[i].edit.WhenAction = [=] { CommitEdit(index); };
            row_[i].edit.WhenCommit = [=] { CommitEdit(index); };
        }
    }

    void ConfigureRow(int index, const String& label,
                      double minimum, double maximum,
                      int precision, int slider_steps,
                      bool alpha = false)
    {
        if(index < 0 || index >= 5)
            return;

        Row& row = row_[index];
        row.label.SetText(label);
        row.minimum = minimum;
        row.maximum = maximum;
        row.precision = max(0, precision);
        row.slider_steps = max(1, slider_steps);
        row.alpha = alpha;
        row.slider.SetRange(0, row.slider_steps).SetStep(1);
        row.edit.SetPlaceholder(precision > 0 ? "0.0000" : "000");
        RefreshLayout();
    }

    void SetRowCount(int count)
    {
        row_count_ = minmax(count, 1, 5);
        for(int i = 0; i < 5; i++)
            row_[i].label.Show(i < row_count_),
            row_[i].slider.Show(i < row_count_),
            row_[i].edit.Show(i < row_count_);
        RefreshLayout();
    }

    int GetRowCount() const
    {
        return row_count_;
    }

    void SetValue(int index, double value)
    {
        if(index < 0 || index >= row_count_)
            return;

        Row& row = row_[index];
        value = minmax(value, row.minimum, row.maximum);
        int slider_value = int((value - row.minimum)
                             / max(1e-12, row.maximum - row.minimum)
                             * row.slider_steps + 0.5);

        String text = row.precision > 0
                    ? Format("%.*f", row.precision, value)
                    : AsString(int(value + (value >= 0 ? 0.5 : -0.5)));

        syncing_ = true;
        row.slider.SetValue(slider_value);
        if(!(row.edit.HasFocus() && row.edit.IsDirty())) {
            row.edit.SetTextUtf8(text);
            row.edit.ClearDirty();
        }
        syncing_ = false;
    }

    double GetValue(int index) const
    {
        if(index < 0 || index >= row_count_)
            return 0.0;
        double value = 0.0;
        if(ParseSingleNumber_(row_[index].edit.GetTextUtf8(), value))
            return minmax(value, row_[index].minimum, row_[index].maximum);
        return SliderToValue(index);
    }

    void EnableAlpha(bool enabled)
    {
        for(int i = 0; i < row_count_; i++) {
            if(!row_[i].alpha)
                continue;
            row_[i].label.Enable(enabled);
            row_[i].slider.Enable(enabled);
            row_[i].edit.Enable(enabled);
        }
    }

    Event<int, double, bool> WhenValue;
    Event<String, bool> WhenColorText;

    virtual void Layout() override
    {
        Rect rect(Point(0, 0), GetSize());
        int row_height = min(DPI(25), max(DPI(16), rect.GetHeight() / max(1, row_count_)));
        int label_width = DPI(24);
        int edit_width = DPI(74);
        int gap = DPI(4);

        for(int i = 0; i < row_count_; i++) {
            int y = rect.top + i * row_height;
            Rect row_rect(rect.left, y, rect.right,
                          min(rect.bottom, y + row_height));
            row_[i].label.SetRect(row_rect.left, row_rect.top,
                                  label_width, row_rect.GetHeight());
            row_[i].edit.SetRect(row_rect.right - edit_width, row_rect.top + DPI(1),
                                  edit_width, max(DPI(19), row_rect.GetHeight() - DPI(2)));
            int slider_left = row_[i].label.GetRect().right + gap;
            int slider_right = row_[i].edit.GetRect().left - gap;
            row_[i].slider.SetRect(slider_left, row_rect.top,
                                   max(0, slider_right - slider_left),
                                   row_rect.GetHeight());
        }
    }

    virtual Size GetMinSize() const override
    {
        return Size(DPI(300), row_count_ * DPI(28));
    }

private:
    double SliderToValue(int index) const
    {
        const Row& row = row_[index];
        double t = row.slider.GetValue() / double(max(1, row.slider_steps));
        return row.minimum + (row.maximum - row.minimum) * t;
    }

    void CommitEdit(int index)
    {
        if(syncing_ || index < 0 || index >= row_count_)
            return;

        String text = row_[index].edit.GetTextUtf8();
        if(LooksLikeColorExpression_(text)) {
            if(WhenColorText)
                WhenColorText(text, true);
            return;
        }

        double value = 0.0;
        if(!ParseSingleNumber_(text, value)) {
            SetValue(index, SliderToValue(index));
            return;
        }

        value = minmax(value, row_[index].minimum, row_[index].maximum);
        SetValue(index, value);
        if(WhenValue)
            WhenValue(index, value, true);
        row_[index].edit.ClearDirty();
    }

    Row row_[5];
    int row_count_ = 4;
    bool syncing_ = false;
};

class UiColorPicker::ImagePreview : public Ctrl {
public:
    typedef ImagePreview CLASSNAME;

    void SetImage(const Image& image)
    {
        image_ = image;
        points_.Clear();
        if(!image_.IsEmpty()) {
            points_.Add(Pointf(0.25, 0.25));
            points_.Add(Pointf(0.65, 0.20));
            points_.Add(Pointf(0.50, 0.55));
            points_.Add(Pointf(0.78, 0.72));
        }
        Refresh();
    }

    Vector<Color> GetSamples() const
    {
        Vector<Color> colors;
        Size source = image_.GetSize();
        if(source.IsEmpty())
            return colors;
        for(const Pointf& point : points_) {
            RGBA pixel = image_[minmax(int(point.y * source.cy), 0, source.cy - 1)]
                               [minmax(int(point.x * source.cx), 0, source.cx - 1)];
            colors.Add(Color(pixel.r, pixel.g, pixel.b));
        }
        return colors;
    }

    void ClearSamples()
    {
        points_.Clear();
        selected_ = -1;
        Refresh();
    }

    void SetSampleCount(int count)
    {
        if(image_.IsEmpty())
            return;
        count = minmax(count, 2, 9);
        while(points_.GetCount() > count)
            points_.Remove(points_.GetCount() - 1);
        while(points_.GetCount() < count) {
            int i = points_.GetCount();
            points_.Add(Pointf(((i % 3) + 0.5) / 3.0,
                               ((i / 3) + 0.5) / 3.0));
        }
        selected_ = min(selected_, points_.GetCount() - 1);
        Refresh();
    }

    Event<> WhenSamplesChanged;

    virtual void Paint(Draw& draw) override
    {
        Rect rect(Point(0, 0), GetSize());
        draw.DrawRect(rect, Blend(SColorPaper(), SColorText(), 8));
        DrawFrame_(draw, rect, Blend(SColorShadow(), SColorPaper(), 120));

        if(image_.IsEmpty()) {
            String text = "No image loaded";
            Font font = StdFont();
            Size size = GetTextSize(text, font);
            draw.DrawText(rect.left + (rect.GetWidth() - size.cx) / 2,
                          rect.top + (rect.GetHeight() - size.cy) / 2,
                          text, font, SColorDisabled());
            return;
        }

        Size source = image_.GetSize();
        double scale = min(rect.GetWidth() / double(max(1, source.cx)),
                           rect.GetHeight() / double(max(1, source.cy)));
        Size target(max(1, int(source.cx * scale + 0.5)),
                    max(1, int(source.cy * scale + 0.5)));
        int x = rect.left + (rect.GetWidth() - target.cx) / 2;
        int y = rect.top + (rect.GetHeight() - target.cy) / 2;
        draw.DrawImage(x, y, target.cx, target.cy, image_);
        for(const Pointf& point : points_) {
            int index = int(&point - points_.Begin());
            Point p(x + int(point.x * target.cx + 0.5),
                    y + int(point.y * target.cy + 0.5));
            RGBA pixel = image_[minmax(int(point.y * source.cy), 0, source.cy - 1)]
                               [minmax(int(point.x * source.cx), 0, source.cx - 1)];
            Color color(pixel.r, pixel.g, pixel.b);
            Rect marker = RectC(p.x - DPI(9), p.y - DPI(9), DPI(18), DPI(18));
            draw.DrawEllipse(marker, color, DPI(2), White());
            draw.DrawEllipse(marker.Inflated(DPI(2)), Null,
                             index == selected_ ? DPI(2) : DPI(1),
                             index == selected_ ? SColorHighlight() : SColorShadow());
        }
    }

    virtual void LeftDown(Point point, dword) override
    {
        if(image_.IsEmpty())
            return;
        Rect rect(Point(0, 0), GetSize());
        Size source = image_.GetSize();
        double scale = min(rect.GetWidth() / double(max(1, source.cx)),
                           rect.GetHeight() / double(max(1, source.cy)));
        Size target(max(1, int(source.cx * scale + 0.5)),
                    max(1, int(source.cy * scale + 0.5)));
        Point origin((rect.GetWidth() - target.cx) / 2,
                     (rect.GetHeight() - target.cy) / 2);
        Rect image_rect(origin, target);
        if(!image_rect.Contains(point))
            return;
        Pointf normalized((point.x - origin.x) / double(max(1, target.cx)),
                          (point.y - origin.y) / double(max(1, target.cy)));
        selected_ = HitSample(point, image_rect);
        if(selected_ < 0 && points_.GetCount() < 9) {
            selected_ = points_.GetCount();
            points_.Add(normalized);
        }
        else if(selected_ >= 0)
            points_[selected_] = normalized;
        if(selected_ >= 0)
            SetCapture();
        Refresh();
        if(WhenSamplesChanged)
            WhenSamplesChanged();
    }

    virtual void MouseMove(Point point, dword flags) override
    {
        if(!HasCapture() || !(flags & K_MOUSELEFT) || selected_ < 0)
            return;
        Rect image_rect = GetImageRect();
        Point constrained(minmax(point.x, image_rect.left, image_rect.right - 1),
                          minmax(point.y, image_rect.top, image_rect.bottom - 1));
        points_[selected_] = Pointf((constrained.x - image_rect.left) / double(max(1, image_rect.GetWidth())),
                                    (constrained.y - image_rect.top) / double(max(1, image_rect.GetHeight())));
        Refresh();
        if(WhenSamplesChanged)
            WhenSamplesChanged();
    }

    virtual void LeftUp(Point, dword) override
    {
        if(HasCapture())
            ReleaseCapture();
    }

    virtual bool Key(dword key, int) override
    {
        if((key == K_DELETE || key == K_BACKSPACE) && selected_ >= 0 && selected_ < points_.GetCount()) {
            points_.Remove(selected_);
            selected_ = min(selected_, points_.GetCount() - 1);
            Refresh();
            if(WhenSamplesChanged)
                WhenSamplesChanged();
            return true;
        }
        return false;
    }

    virtual Size GetMinSize() const override
    {
        return Size(DPI(180), DPI(150));
    }

private:
    Rect GetImageRect() const
    {
        Rect rect(Point(0, 0), GetSize());
        Size source = image_.GetSize();
        if(source.IsEmpty())
            return Rect(0, 0, 0, 0);
        double scale = min(rect.GetWidth() / double(max(1, source.cx)),
                           rect.GetHeight() / double(max(1, source.cy)));
        Size target(max(1, int(source.cx * scale + 0.5)),
                    max(1, int(source.cy * scale + 0.5)));
        return RectC((rect.GetWidth() - target.cx) / 2,
                     (rect.GetHeight() - target.cy) / 2, target.cx, target.cy);
    }

    int HitSample(Point point, const Rect& image_rect) const
    {
        for(int i = points_.GetCount() - 1; i >= 0; i--) {
            Point p(image_rect.left + int(points_[i].x * image_rect.GetWidth() + 0.5),
                    image_rect.top + int(points_[i].y * image_rect.GetHeight() + 0.5));
            int dx = point.x - p.x, dy = point.y - p.y;
            if(dx * dx + dy * dy <= DPI(13) * DPI(13))
                return i;
        }
        return -1;
    }

    Image image_;
    Vector<Pointf> points_;
    int selected_ = -1;
};

class UiColorPicker::HarmonyWheel : public Ctrl {
public:
    typedef HarmonyWheel CLASSNAME;

    HarmonyWheel()
    {
        WantFocus();
        RebuildHandles();
    }

    void SetBase(Color color)
    {
        int hue = 0, saturation = 0, value = 0;
        ColorToHsv_(color, hue, saturation, value);
        if(saturation > 0 && value > 0)
            base_hue_ = hue;
        saturation_ = saturation;
        value_ = value;
        manipulator_ = Point(base_hue_, 50);
        RebuildHandles();
        cache_ = Image();
        Refresh();
    }

    void SetMode(HarmonyMode mode)
    {
        if(mode_ == mode)
            return;
        mode_ = mode;
        RebuildHandles();
        Refresh();
    }

    void SetPaletteCount(int count)
    {
        count = minmax(count, 2, 9);
        if(count_ == count)
            return;
        count_ = count;
        RebuildHandles();
        Refresh();
    }

    int GetPaletteCount() const { return count_; }
    void SelectHandle(int index)
    {
        if(index < 0 || index >= handles_.GetCount())
            return;
        selected_ = index;
        Refresh();
    }
    int GetGlobalGain() const { return value_; }
    void SetGlobalGain(int value)
    {
        value = ClampPercent_(value);
        if(value_ == value)
            return;
        value_ = value;
        cache_ = Image();
        Refresh();
    }

    const Vector<Point>& GetHandles() const { return handles_; }
    const Vector<int>& GetValues() const { return values_; }
    void SetState(const Vector<Point>& handles, const Vector<int>& values)
    {
        if(handles.IsEmpty() || handles.GetCount() != values.GetCount())
            return;
        handles_ = clone(handles);
        values_ = clone(values);
        count_ = minmax(handles_.GetCount(), 2, 9);
        for(int i = 0; i < handles_.GetCount(); i++) {
            handles_[i].x = NormalizeHue_(handles_[i].x);
            handles_[i].y = ClampPercent_(handles_[i].y);
            values_[i] = ClampPercent_(values_[i]);
        }
        selected_ = minmax(selected_, 0, handles_.GetCount() - 1);
        base_hue_ = handles_[0].x;
        int average_saturation = 0;
        for(const Point& handle : handles_)
            average_saturation += handle.y;
        manipulator_ = Point(base_hue_, average_saturation / max(1, handles_.GetCount()));
        Refresh();
    }

    Vector<SlotValue> GetColors() const
    {
        Vector<SlotValue> output;
        for(int i = 0; i < handles_.GetCount(); i++) {
            SlotValue& value = output.Add();
            value.color = HsvToColor_(handles_[i].x, handles_[i].y,
                                      values_[i] * value_ / 100.0);
            value.alpha = 255;
            value.label = i == 0 ? "Base" : Format("Harmony %d", i);
        }
        return output;
    }

    Event<bool> WhenChange;

    virtual void Paint(Draw& draw) override
    {
        Rect wheel = WheelRect();
        draw.DrawRect(GetSize(), SColorPaper());
        if(wheel.IsEmpty())
            return;
        EnsureCache(wheel.GetSize());
        draw.DrawImage(wheel.left, wheel.top, cache_);

        Point center = wheel.CenterPoint();
        for(int i = 0; i < handles_.GetCount(); i++) {
            Point p = HandlePoint(i, wheel);
            draw.DrawLine(center.x, center.y, p.x, p.y, DPI(2), White());
            draw.DrawLine(center.x, center.y + DPI(1), p.x, p.y + DPI(1), DPI(1), SColorShadow());
        }
        for(int i = 0; i < handles_.GetCount(); i++) {
            Point p = HandlePoint(i, wheel);
            Color color = HsvToColor_(handles_[i].x, handles_[i].y,
                                      values_[i] * value_ / 100.0);
            int radius = DPI(10);
            Rect marker = RectC(p.x - radius, p.y - radius, radius * 2, radius * 2);
            draw.DrawEllipse(marker.Inflated(DPI(2)), SColorPaper(), DPI(1), SColorShadow());
            draw.DrawEllipse(marker, color, DPI(2), White());
            if(i == selected_)
                draw.DrawEllipse(marker.Inflated(DPI(4)), Null, DPI(2), SColorHighlight());
        }
        Point manipulator = PointFor(manipulator_, wheel);
        Rect tool = RectC(manipulator.x - DPI(10), manipulator.y - DPI(10), DPI(20), DPI(20));
        draw.DrawEllipse(tool.Inflated(DPI(3)), SColorPaper(), DPI(1), SColorShadow());
        draw.DrawEllipse(tool, Blend(SColorPaper(), SColorText(), 65), DPI(2), White());
        draw.DrawEllipse(tool.Deflated(DPI(5)), Null, DPI(2), White());
    }

    virtual void LeftDown(Point point, dword) override
    {
        Rect wheel = WheelRect();
        manipulating_ = HitManipulator(point, wheel);
        if(!manipulating_)
            selected_ = HitHandle(point, wheel);
        if(manipulating_ || selected_ >= 0) {
            SetCapture();
            UpdateHandle(point, false);
        }
    }

    virtual void LeftDouble(Point point, dword) override
    {
        Rect wheel = WheelRect();
        if(!HitManipulator(point, wheel))
            return;
        if(mode_ != HARMONY_MONOCHROMATIC)
            for(Point& handle : handles_)
                handle.y = manipulator_.y;
        Refresh();
        if(WhenChange)
            WhenChange(true);
    }

    virtual void MouseMove(Point point, dword flags) override
    {
        if(HasCapture() && (flags & K_MOUSELEFT))
            UpdateHandle(point, false);
    }

    virtual void LeftUp(Point point, dword) override
    {
        if(!HasCapture())
            return;
        UpdateHandle(point, true);
        ReleaseCapture();
        manipulating_ = false;
    }

private:
    Rect WheelRect() const
    {
        Rect rect(Point(0, 0), GetSize());
        int side = max(0, min(rect.GetWidth(), rect.GetHeight()) - DPI(12));
        return RectC(rect.left + (rect.GetWidth() - side) / 2,
                     rect.top + (rect.GetHeight() - side) / 2, side, side);
    }

    Point HandlePoint(int index, const Rect& wheel) const
    {
        return PointFor(handles_[index], wheel);
    }

    Point PointFor(const Point& value, const Rect& wheel) const
    {
        double angle = value.x * M_PI / 180.0;
        double radius = value.y / 100.0 * wheel.GetWidth() / 2.0;
        Point center = wheel.CenterPoint();
        return Point(center.x + int(cos(angle) * radius + 0.5),
                     center.y - int(sin(angle) * radius + 0.5));
    }

    bool HitManipulator(Point point, const Rect& wheel) const
    {
        Point p = PointFor(manipulator_, wheel);
        int dx = point.x - p.x, dy = point.y - p.y;
        return dx * dx + dy * dy <= DPI(16) * DPI(16);
    }

    int HitHandle(Point point, const Rect& wheel) const
    {
        for(int i = handles_.GetCount() - 1; i >= 0; i--) {
            Point p = HandlePoint(i, wheel);
            int dx = point.x - p.x;
            int dy = point.y - p.y;
            if(dx * dx + dy * dy <= DPI(15) * DPI(15))
                return i;
        }
        return -1;
    }

    void UpdateHandle(Point point, bool final_commit)
    {
        if(!manipulating_ && (selected_ < 0 || selected_ >= handles_.GetCount()))
            return;
        Rect wheel = WheelRect();
        Point center = wheel.CenterPoint();
        double dx = point.x - center.x;
        double dy = center.y - point.y;
        int hue = NormalizeHue_(int(atan2(dy, dx) * 180.0 / M_PI + 0.5));
        int saturation = ClampPercent_(int(sqrt(dx * dx + dy * dy) /
                                             max(1.0, wheel.GetWidth() / 2.0) * 100.0 + 0.5));
        if(manipulating_) {
            int delta = hue - manipulator_.x;
            double radial_scale = saturation / double(max(1, manipulator_.y));
            for(Point& handle : handles_)
                handle.x = NormalizeHue_(handle.x + delta);
            if(mode_ != HARMONY_MONOCHROMATIC)
                for(Point& handle : handles_)
                    handle.y = ClampPercent_(int(handle.y * radial_scale + 0.5));
            manipulator_ = Point(hue, saturation);
            base_hue_ = handles_.IsEmpty() ? hue : handles_[0].x;
        }
        else if(mode_ == HARMONY_CUSTOM) {
            handles_[selected_] = Point(hue, saturation);
        }
        else {
            int delta = hue - handles_[selected_].x;
            for(Point& handle : handles_)
                handle.x = NormalizeHue_(handle.x + delta);
            base_hue_ = handles_[0].x;
            handles_[selected_].y = saturation;
        }
        Refresh();
        if(WhenChange)
            WhenChange(final_commit);
    }

    void RebuildHandles()
    {
        Vector<Point> old_handles = clone(handles_);
        Vector<int> old_values = clone(values_);
        handles_.Clear();
        values_.Clear();
        const int h = base_hue_;
        auto add = [&](int offset, int saturation = -1, int value = -1) {
            handles_.Add(Point(NormalizeHue_(h + offset), saturation < 0 ? saturation_ : saturation));
            values_.Add(value < 0 ? 100 : ClampPercent_(value));
        };
        if(mode_ == HARMONY_CUSTOM && !old_handles.IsEmpty()) {
            for(int i = 0; i < count_; i++) {
                if(i < old_handles.GetCount()) {
                    handles_.Add(old_handles[i]);
                    values_.Add(i < old_values.GetCount() ? old_values[i] : value_);
                }
                else
                    add(i * 360 / count_);
            }
            selected_ = minmax(selected_, 0, handles_.GetCount() - 1);
            return;
        }
        switch(mode_) {
        case HARMONY_ANALOGOUS:
            for(int i = 0; i < count_; i++) add(-60 + i * 120 / max(1, count_ - 1));
            break;
        case HARMONY_COMPLEMENTARY:
            for(int i = 0; i < count_; i++) add(i < (count_ + 1) / 2 ? -15 + i * 15 : 165 + (i - (count_ + 1) / 2) * 30);
            break;
        case HARMONY_SPLIT_COMPLEMENTARY: {
            const int offsets[] = { 0, 150, 210, 165, 195, -15, 15, 180, 30 };
            for(int i = 0; i < count_; i++) add(offsets[i]);
            break;
        }
        case HARMONY_TRIAD: {
            const int offsets[] = { 0, 120, 240, -15, 15, 105, 135, 225, 255 };
            for(int i = 0; i < count_; i++) add(offsets[i]);
            break;
        }
        case HARMONY_SQUARE:
            for(int i = 0; i < count_; i++) add((i % 4) * 90, i < 4 ? saturation_ : max(20, saturation_ - 25));
            break;
        case HARMONY_COMPOUND: {
            const int offsets[] = { -30, 0, 30, 150, 210, 180, 60, 300, 330 };
            for(int i = 0; i < count_; i++) add(offsets[i], i & 1 ? max(20, saturation_ - 20) : saturation_);
            break;
        }
        case HARMONY_SHADES:
            for(int i = 0; i < count_; i++) add(0, (i + 1) * 100 / (count_ + 1), 100 - i * 75 / max(1, count_ - 1));
            break;
        case HARMONY_MONOCHROMATIC:
            for(int i = 0; i < count_; i++) add(0, (i + 1) * 100 / (count_ + 1), 100);
            break;
        case HARMONY_CUSTOM:
        default:
            for(int i = 0; i < count_; i++) add(i * 360 / count_);
            break;
        }
        selected_ = minmax(selected_, 0, handles_.GetCount() - 1);
    }

    void EnsureCache(Size size)
    {
        if(!cache_.IsEmpty() && cache_.GetSize() == size && cache_value_ == value_)
            return;
        ImageBuffer buffer(size);
        Point center(size.cx / 2, size.cy / 2);
        double radius = max(1.0, min(size.cx, size.cy) / 2.0);
        for(int y = 0; y < size.cy; y++) {
            RGBA *row = buffer[y];
            for(int x = 0; x < size.cx; x++) {
                double dx = x - center.x;
                double dy = center.y - y;
                double distance = sqrt(dx * dx + dy * dy);
                if(distance > radius) {
                    row[x] = RGBAZero();
                    continue;
                }
                int hue = NormalizeHue_(int(atan2(dy, dx) * 180.0 / M_PI + 0.5));
                int saturation = ClampPercent_(int(distance / radius * 100.0 + 0.5));
                row[x] = RGBA(HsvToColor_(hue, saturation, value_));
                row[x].a = 255;
            }
        }
        cache_ = Image(buffer);
        cache_value_ = value_;
    }

    HarmonyMode mode_ = HARMONY_ANALOGOUS;
    Vector<Point> handles_;
    int selected_ = 0;
    int base_hue_ = 200;
    int saturation_ = 80;
    int value_ = 85;
    int count_ = 5;
    Vector<int> values_;
    Point manipulator_ = Point(200, 50);
    bool manipulating_ = false;
    int cache_value_ = -1;
    Image cache_;
};

const UiColorPicker::Style& UiColorPicker::StyleDefault()
{
    static Style style;
    ONCELOCK {
        style.metrics.face_enabled = true;
        style.metrics.frame_enabled = true;
        style.metrics.frame_width = DPI(1);
        style.metrics.radius = DPI(6);
        style.metrics.content_margin = Rect(DPI(2), DPI(2), DPI(2), DPI(2));
        style.metrics.focus_enabled = false;
        for(int i = 0; i < 4; i++) {
            style.palette.face[i] = UiFill::Solid(SColorPaper());
            style.palette.frame[i] = SColorShadow();
            style.palette.ink[i] = SColorText();
            style.palette.icon[i] = SColorText();
        }
    }
    return style;
}

UiColorPicker::UiColorPicker()
{
    slots_.SetCount(4);
    previous_slots_.SetCount(4);
    opening_slots_.SetCount(4);

    const Color defaults[4] = {
        Color(0, 120, 212),
        Color(255, 204, 0),
        Color(52, 199, 89),
        Color(255, 59, 48)
    };
    for(int i = 0; i < 4; i++) {
        slots_[i].color = defaults[i];
        slots_[i].alpha = 255;
        slots_[i].label = Format("C%d", i + 1);
        previous_slots_[i] = slots_[i];
        opening_slots_[i] = slots_[i];
    }

    BuildChildTree();
    ConfigureControls();
    WireEvents();
    LoadSharedSession();
    SetGeneratorMode(generator_mode_);

    opening_slots_ = clone(slots_);
    curve_source_color_ = GetColor();
    SyncThemeToChildren();
    SyncPageButtons();
    SyncChannelButtons();
    SyncSpectrumMode();
    RefreshPaletteGrid();
    RefreshGeneratorPalette();
    SyncAllFromActiveSlot();
}

UiColorPicker::~UiColorPicker()
{
    // The native window can already be tearing down here. State cleanup must
    // not query or release GUI capture.
    FinishEyedropperState(false);
    if(session_persistence_)
        SaveSharedSession(false);
}

void UiColorPicker::BuildChildTree()
{
    color_field_.Create();
    current_slot_button_.Create();
    recent_grid_.Create();
    palette_grid_.Create();
    stash_grid_.Create();
    generator_grid_.Create();
    generator_image_preview_.Create();
    generator_wheel_.Create();

    readout_hsv_.Create();
    readout_hex_.Create();
    readout_hsl_.Create();
    readout_rgb_float_.Create();
    readout_cmyk_.Create();
    readout_rgb_int_.Create();

    for(int i = 0; i < 4; i++) {
        primary_slot_button_[i].Create();
        previous_slot_button_[i].Create();
    }
    for(int i = 0; i < CHANNEL_COUNT; i++)
        channel_group_[i].Create();

    Add(main_root_.SizePos());
    main_root_.SetGap(DPI(2)).SetInset(0);
    main_root_.Add(navigation_bar_).Fixed(DPI(48));
    main_root_.Add(page_stack_).Expand(1);
    main_root_.Add(footer_bar_).Fixed(DPI(42));

    navigation_bar_.SetGap(DPI(5), DPI(5)).SetInset(DPI(2));
    for(int i = 0; i < PAGE_COUNT; i++)
        navigation_bar_.Add(page_button_[i]).Fixed(i == PAGE_GENERATOR ? DPI(88) : DPI(68));
    navigation_bar_.Add(navigation_spacer_).Expand(1);
    navigation_bar_.Add(slot_grid_host_).Fixed(DPI(220));

    for(int i = 0; i < 4; i++) {
        slot_grid_host_.Add(*primary_slot_button_[i]);
        slot_grid_host_.Add(*previous_slot_button_[i]);
    }

    page_stack_.AddPage(color_page_, "color");
    page_stack_.AddPage(palette_page_, "palettes");
    page_stack_.AddPage(curves_page_, "curves");
    page_stack_.AddPage(generator_page_, "generator");

    footer_bar_.SetGap(DPI(8), DPI(8)).SetInset(Rect(DPI(6), DPI(6), DPI(6), DPI(6)));
    footer_bar_.Add(footer_information_).Expand(1);
    footer_bar_.Add(footer_spacer_).Fixed(DPI(4));
    footer_bar_.Add(accept_button_).Fixed(DPI(110));
    footer_bar_.Add(cancel_button_).Fixed(DPI(110));

    color_page_.Add(spectrum_mode_drop_);
    color_page_.Add(eyedropper_button_);
    color_page_.Add(*color_field_);
    color_page_.Add(hue_axis_label_);
    color_page_.Add(gain_axis_label_);
    color_page_.Add(hue_axis_slider_);
    color_page_.Add(gain_axis_slider_);
    color_page_.Add(hue_axis_edit_);
    color_page_.Add(gain_axis_edit_);
    color_page_.Add(channel_mode_drop_);
    color_page_.Add(alpha_toggle_);
    color_page_.Add(alpha_toggle_label_);
    color_page_.Add(channel_stack_);
    for(int i = 0; i < CHANNEL_COUNT; i++)
        channel_stack_.AddPage(*channel_group_[i], AsString(i));
    color_page_.Add(*readout_hsv_);
    color_page_.Add(*readout_hex_);
    color_page_.Add(*readout_hsl_);
    color_page_.Add(*readout_rgb_float_);
    color_page_.Add(*readout_cmyk_);
    color_page_.Add(*readout_rgb_int_);

    palette_page_.Add(palette_drop_);
    palette_page_.Add(palette_badge_);
    palette_page_.Add(*palette_grid_);
    palette_page_.Add(stash_title_);
    palette_page_.Add(palette_hint_);
    palette_page_.Add(*stash_grid_);

    for(int i = 0; i < 4; i++) {
        curves_page_.Add(curve_button_[i]);
        curve_stack_.AddPage(curve_editor_[i], AsString(i));
    }
    curves_page_.Add(curve_capture_button_);
    curves_page_.Add(curve_reset_button_);
    curves_page_.Add(curve_stack_);
    curves_page_.Add(curve_hint_);

    generator_page_.Add(harmony_drop_);
    for(int i = 0; i < 3; i++)
        generator_page_.Add(generator_mode_button_[i]);
    generator_page_.Add(generator_refresh_button_);
    generator_page_.Add(generator_load_image_button_);
    generator_page_.Add(generator_clear_samples_button_);
    generator_page_.Add(generator_gain_label_);
    generator_page_.Add(generator_gain_slider_);
    generator_page_.Add(generator_gain_edit_);
    generator_page_.Add(generator_count_label_);
    generator_page_.Add(generator_count_slider_);
    generator_page_.Add(generator_count_edit_);
    generator_page_.Add(generator_use_button_);
    generator_page_.Add(generator_save_button_);
    generator_page_.Add(*generator_image_preview_);
    generator_page_.Add(*generator_wheel_);
    generator_page_.Add(*generator_grid_);
    generator_page_.Add(generator_hint_);
}

void UiColorPicker::ConfigureControls()
{
    const char *page_names[PAGE_COUNT] = { "Color", "Palettes", "Curves", "Generator" };
    for(int i = 0; i < PAGE_COUNT; i++) {
        page_button_[i].SetText(page_names[i]);
        page_button_[i].SetCheckable(true);
    }

    current_slot_button_->Configure(*this, 0, ColorSlotButton::CURRENT);
    for(int i = 0; i < 4; i++) {
        primary_slot_button_[i]->Configure(*this, i, ColorSlotButton::PRIMARY);
        previous_slot_button_[i]->Configure(*this, i, ColorSlotButton::PREVIOUS);
    }

    accept_button_.SetText("OK");
    cancel_button_.SetText("Cancel");
    footer_information_.SetText("Information / Detail");
    footer_information_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);

    spectrum_mode_drop_.Clear();
    spectrum_mode_drop_.Add("Hue Strip", (int)SPECTRUM_HUE_STRIP);
    spectrum_mode_drop_.Add("HSV Rectangle", (int)SPECTRUM_HSV_RECT);
    spectrum_mode_drop_.Add("RGB Spectrum", (int)SPECTRUM_RGB_SPECTRUM);
    spectrum_mode_drop_.SetDataSilently((int)spectrum_mode_);
    spectrum_mode_drop_.SetPlaceholderText("Spectrum type");

    eyedropper_button_.SetText("")
                       .SetIcon(ICON_DESIGN_FORMAT_PAINT_48())
                       .SetIconSize(DPI(15), DPI(15))
                       .SetIconRenderMode(UiIconRenderMode::MonoTint);
    eyedropper_button_.Tip("Sample a colour from the screen");
    eyedropper_button_.Enable(IsScreenEyedropperAvailable());

    hue_axis_label_.SetText("Hue");
    gain_axis_label_.SetText("Gain");
    hue_axis_slider_.SetRange(0, 359).SetStep(1);
    gain_axis_slider_.SetRange(0, 100).SetStep(1);
    hue_axis_edit_.SetPlaceholder("359");
    gain_axis_edit_.SetPlaceholder("100");
    hue_axis_edit_.SetTextAlign(UiAlign::RIGHT);
    gain_axis_edit_.SetTextAlign(UiAlign::RIGHT);

    const Size track_size(DPI(4096), DPI(5));
    const Size thumb_size(DPI(14), DPI(18));
    hue_axis_slider_.SetTrackSize(track_size).SetThumbSize(thumb_size);
    gain_axis_slider_.SetTrackSize(track_size).SetThumbSize(thumb_size);

    const char *channel_names[CHANNEL_COUNT] = {
        "RGB-F", "RGB-8", "HSB / HSV", "HLS", "TMI", "CMYK", "CIE Lab"
    };
    channel_mode_drop_.Clear();
    for(int i = 0; i < CHANNEL_COUNT; i++) {
        channel_button_[i].SetText(channel_names[i]);
        channel_button_[i].SetCheckable(true);
        channel_mode_drop_.Add(channel_names[i], i);
    }
    channel_mode_drop_.SetDataSilently((int)channel_mode_);

    channel_group_[CHANNEL_RGB_FLOAT]->SetRowCount(4);
    channel_group_[CHANNEL_RGB_FLOAT]->ConfigureRow(0, "R", 0.0, 1.0, 4, 10000);
    channel_group_[CHANNEL_RGB_FLOAT]->ConfigureRow(1, "G", 0.0, 1.0, 4, 10000);
    channel_group_[CHANNEL_RGB_FLOAT]->ConfigureRow(2, "B", 0.0, 1.0, 4, 10000);
    channel_group_[CHANNEL_RGB_FLOAT]->ConfigureRow(3, "A", 0.0, 1.0, 4, 10000, true);

    channel_group_[CHANNEL_RGB_INT]->SetRowCount(4);
    channel_group_[CHANNEL_RGB_INT]->ConfigureRow(0, "R", 0, 255, 0, 255);
    channel_group_[CHANNEL_RGB_INT]->ConfigureRow(1, "G", 0, 255, 0, 255);
    channel_group_[CHANNEL_RGB_INT]->ConfigureRow(2, "B", 0, 255, 0, 255);
    channel_group_[CHANNEL_RGB_INT]->ConfigureRow(3, "A", 0, 255, 0, 255, true);

    channel_group_[CHANNEL_HSV]->SetRowCount(4);
    channel_group_[CHANNEL_HSV]->ConfigureRow(0, "H", 0, 359, 0, 359);
    channel_group_[CHANNEL_HSV]->ConfigureRow(1, "S", 0, 100, 2, 10000);
    channel_group_[CHANNEL_HSV]->ConfigureRow(2, "V", 0, 100, 2, 10000);
    channel_group_[CHANNEL_HSV]->ConfigureRow(3, "A", 0, 100, 2, 10000, true);

    channel_group_[CHANNEL_HSL]->SetRowCount(4);
    channel_group_[CHANNEL_HSL]->ConfigureRow(0, "H", 0, 359, 0, 359);
    channel_group_[CHANNEL_HSL]->ConfigureRow(1, "S", 0, 100, 2, 10000);
    channel_group_[CHANNEL_HSL]->ConfigureRow(2, "L", 0, 100, 2, 10000);
    channel_group_[CHANNEL_HSL]->ConfigureRow(3, "A", 0, 100, 2, 10000, true);

    channel_group_[CHANNEL_TMI]->SetRowCount(4);
    channel_group_[CHANNEL_TMI]->ConfigureRow(0, "T", -100, 100, 2, 20000);
    channel_group_[CHANNEL_TMI]->ConfigureRow(1, "M", -100, 100, 2, 20000);
    channel_group_[CHANNEL_TMI]->ConfigureRow(2, "I", 0, 100, 2, 10000);
    channel_group_[CHANNEL_TMI]->ConfigureRow(3, "A", 0, 100, 2, 10000, true);

    channel_group_[CHANNEL_CMYK]->SetRowCount(5);
    channel_group_[CHANNEL_CMYK]->ConfigureRow(0, "C", 0, 100, 2, 10000);
    channel_group_[CHANNEL_CMYK]->ConfigureRow(1, "M", 0, 100, 2, 10000);
    channel_group_[CHANNEL_CMYK]->ConfigureRow(2, "Y", 0, 100, 2, 10000);
    channel_group_[CHANNEL_CMYK]->ConfigureRow(3, "K", 0, 100, 2, 10000);
    channel_group_[CHANNEL_CMYK]->ConfigureRow(4, "A", 0, 100, 2, 10000, true);

    channel_group_[CHANNEL_LAB]->SetRowCount(4);
    channel_group_[CHANNEL_LAB]->ConfigureRow(0, "L*", 0, 100, 2, 10000);
    channel_group_[CHANNEL_LAB]->ConfigureRow(1, "a*", -128, 127, 2, 25500);
    channel_group_[CHANNEL_LAB]->ConfigureRow(2, "b*", -128, 127, 2, 25500);
    channel_group_[CHANNEL_LAB]->ConfigureRow(3, "A", 0, 100, 2, 10000, true);

    alpha_toggle_.SetOn(alpha_enabled_);
    alpha_toggle_label_.SetText("Alpha");
    alpha_toggle_label_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);

    readout_hsv_->SetTitle("HSV-A");
    readout_hsv_->SetPlaceholder("hsv(200, 100%, 83%, 1.0)");
    readout_hex_->SetTitle("RGB HEX");
    readout_hex_->SetPlaceholder("#0078D4FF");
    readout_hsl_->SetTitle("HLS-A");
    readout_hsl_->SetPlaceholder("hsl(200, 100%, 42%, 1.0)");
    readout_rgb_float_->SetTitle("RGB Float");
    readout_rgb_float_->SetPlaceholder("0.0000, 0.4706, 0.8314, 1.0000");
    readout_cmyk_->SetTitle("CMYK-A");
    readout_cmyk_->SetPlaceholder("cmyk(100%, 43%, 0%, 17%, 1.0)");
    readout_rgb_int_->SetTitle("RGB 8BIT");
    readout_rgb_int_->SetPlaceholder("0, 120, 212, 255");

    recent_grid_->SetGrid(12, 1);
    palette_grid_->SetGrid(12, 8);
    stash_grid_->SetGrid(12, 2);
    stash_grid_->EnableDropTarget(true);
    stash_title_.SetText("User Stash");
    palette_hint_.SetText("Drag palette colours into the stash or directly onto C1-C4.");
    palette_badge_.SetAlign(UiAlign::RIGHT, UiAlign::CENTER);
    palette_use_button_.SetText("Use Selected");
    palette_save_button_.SetText("Save to Stash");
    stash_use_button_.SetText("Use Stash");
    stash_save_active_button_.SetText("Save Current");
    PopulatePaletteSelectors();

    const char *curve_names[4] = { "Master", "Red", "Green", "Blue" };
    for(int i = 0; i < 4; i++) {
        curve_button_[i].SetText(curve_names[i]);
        curve_button_[i].SetCheckable(true);
        curve_editor_[i].SetCurve(ShadowLinear());
    }
    curve_capture_button_.SetText("Capture Current");
    curve_reset_button_.SetText("Reset");
    curve_hint_.SetText("Curves apply to the captured source colour; Master is evaluated before each channel curve.");

    harmony_drop_.Clear();
    harmony_drop_.Add("Custom / Slots", (int)HARMONY_CUSTOM);
    harmony_drop_.Add("Analogous", (int)HARMONY_ANALOGOUS);
    harmony_drop_.Add("Complementary", (int)HARMONY_COMPLEMENTARY);
    harmony_drop_.Add("Split Complementary", (int)HARMONY_SPLIT_COMPLEMENTARY);
    harmony_drop_.Add("Triad", (int)HARMONY_TRIAD);
    harmony_drop_.Add("Square", (int)HARMONY_SQUARE);
    harmony_drop_.Add("Compound", (int)HARMONY_COMPOUND);
    harmony_drop_.Add("Shades", (int)HARMONY_SHADES);
    harmony_drop_.Add("Monochromatic", (int)HARMONY_MONOCHROMATIC);
    harmony_drop_.SetDataSilently((int)harmony_mode_);
    const char *generator_modes[3] = { "Primary Colour", "Image", "Colour Wheel" };
    for(int i = 0; i < 3; i++) {
        generator_mode_button_[i].SetText(generator_modes[i]);
        generator_mode_button_[i].SetCheckable(true);
    }
    generator_refresh_button_.SetText("Regenerate");
    generator_load_image_button_.SetText("Load Image");
    generator_clear_samples_button_.SetText("Clear Points");
    generator_gain_label_.SetText("Gain");
    generator_gain_slider_.SetRange(0, 100).SetStep(1).SetValue(85);
    generator_gain_edit_.SetTextUtf8("85");
    generator_gain_edit_.SetTextAlign(UiAlign::RIGHT);
    generator_count_label_.SetText("Colours");
    generator_count_slider_.SetRange(2, 9).SetStep(1).SetValue(3);
    generator_count_edit_.SetTextUtf8("3");
    generator_count_edit_.SetTextAlign(UiAlign::RIGHT);
    generator_use_button_.SetText("Use Selected");
    generator_save_button_.SetText("Save Palette");
    generator_grid_->SetGrid(6, 2);
    generator_hint_.SetText("Generate professional harmonies from the active colour, or load an image and extract dominant colours.");
    generator_base_color_ = GetColor();
    generator_wheel_->SetBase(generator_base_color_);
    generator_wheel_->SetMode(harmony_mode_);
    generator_wheel_->SetPaletteCount(3);
    for(int i = 0; i < 3; i++)
        generator_mode_button_[i].SetChecked(i == generator_mode_);
    generator_image_preview_->Show(false);
    generator_mode_button_[0].Show(false);
    generator_load_image_button_.Show(false);
    generator_clear_samples_button_.Show(false);
    generator_wheel_->Show(false);
    generator_gain_label_.Show(false);
    generator_gain_slider_.Show(false);
    generator_gain_edit_.Show(false);
    generator_count_label_.Show(false);
    generator_count_slider_.Show(false);
    generator_count_edit_.Show(false);

    curve_stack_.SetActivePage(0);
    channel_stack_.SetActivePage((int)channel_mode_);
    page_stack_.SetActivePage((int)page_mode_);
    UpdateAlphaAvailability();
}

void UiColorPicker::WireEvents()
{
    for(int i = 0; i < PAGE_COUNT; i++) {
        const int index = i;
        page_button_[i].WhenAction = [=] { SetPageMode((PageMode)index); };
    }

    for(int i = 0; i < 4; i++) {
        const int index = i;
        primary_slot_button_[i]->WhenAction = [=] { HandlePrimarySlot(index); };
        previous_slot_button_[i]->WhenAction = [=] { HandlePreviousSlot(index); };
    }
    current_slot_button_->WhenAction = [=] { SetPageMode(PAGE_COLOR); };

    accept_button_.WhenAction = [=] { HandleAccept(); };
    cancel_button_.WhenAction = [=] { HandleCancel(); };

    spectrum_mode_drop_.WhenSelectData = [=](const Value& data) {
        if(!IsNull(data))
            SetSpectrumMode((SpectrumMode)(int)data);
    };
    eyedropper_button_.WhenAction = [=] {
        PostCallback([=] { BeginScreenEyedropper(); });
    };

    color_field_->WhenPick = [=](Point point, bool final_commit) {
        Rect rect(Point(0, 0), color_field_->GetSize());
        if(rect.IsEmpty())
            return;
        int x = minmax(point.x, 0, max(0, rect.GetWidth() - 1));
        int y = minmax(point.y, 0, max(0, rect.GetHeight() - 1));
        int h = remembered_hue_, s = 0, v = 0;
        Color current = GetColor();
        int color_hue = 0;
        ColorToHsv_(current, color_hue, s, v);
        if(s > 0 && v > 0)
            h = remembered_hue_ = color_hue;
        switch(spectrum_mode_) {
        case SPECTRUM_HUE_STRIP:
            h = ClampHue_(int(x / double(max(1, rect.GetWidth() - 1)) * 359.0 + 0.5));
            remembered_hue_ = h;
            s = 100 - int(y / double(max(1, rect.GetHeight() - 1)) * 100.0 + 0.5);
            current = HsvToColor_(h, s, gain_axis_slider_.GetValue());
            break;
        case SPECTRUM_RGB_SPECTRUM:
            current = Color(ClampByte_(int(x / double(max(1, rect.GetWidth() - 1)) * 255.0 + 0.5)),
                            ClampByte_(255 - int(y / double(max(1, rect.GetHeight() - 1)) * 255.0 + 0.5)),
                            current.GetB());
            break;
        case SPECTRUM_HSV_RECT:
        default:
            s = int(x / double(max(1, rect.GetWidth() - 1)) * 100.0 + 0.5);
            v = 100 - int(y / double(max(1, rect.GetHeight() - 1)) * 100.0 + 0.5);
            current = HsvToColor_(h, s, v);
            break;
        }
        CommitColor(current, final_commit);
    };

    hue_axis_slider_.WhenChanging = [=] {
        if(syncing_controls_) return;
        int h = (int)hue_axis_slider_.GetValue();
        remembered_hue_ = h;
        int s = 0, v = 0, old_h = 0;
        ColorToHsv_(GetColor(), old_h, s, v);
        CommitColor(HsvToColor_(h, s, v), false);
    };
    hue_axis_slider_.WhenAction = [=] {
        if(syncing_controls_) return;
        int h = (int)hue_axis_slider_.GetValue();
        remembered_hue_ = h;
        int s = 0, v = 0, old_h = 0;
        ColorToHsv_(GetColor(), old_h, s, v);
        CommitColor(HsvToColor_(h, s, v), true);
    };
    gain_axis_slider_.WhenChanging = [=] {
        if(syncing_controls_) return;
        int h = remembered_hue_, s = 0, v = 0, ignored_hue = 0;
        ColorToHsv_(GetColor(), ignored_hue, s, v);
        CommitColor(HsvToColor_(h, s, gain_axis_slider_.GetValue()), false);
    };
    gain_axis_slider_.WhenAction = [=] {
        if(syncing_controls_) return;
        int h = remembered_hue_, s = 0, v = 0, ignored_hue = 0;
        ColorToHsv_(GetColor(), ignored_hue, s, v);
        CommitColor(HsvToColor_(h, s, gain_axis_slider_.GetValue()), true);
    };

    auto commit_axis = [=](UiLineEdit& edit, bool hue) {
        String text = edit.GetTextUtf8();
        if(LooksLikeColorExpression_(text)) {
            if(!TryApplyColorText(text, true))
                SyncAllFromActiveSlot();
            return;
        }
        double value = 0.0;
        if(!ParseSingleNumber_(text, value)) {
            SyncAllFromActiveSlot();
            return;
        }
        int h = remembered_hue_, s = 0, v = 0, ignored_hue = 0;
        ColorToHsv_(GetColor(), ignored_hue, s, v);
        if(hue)
            remembered_hue_ = h = ClampHue_(int(value + 0.5));
        else
            v = ClampPercent_(int(value + 0.5));
        CommitColor(HsvToColor_(h, s, v), true);
    };
    hue_axis_edit_.WhenChange = [=] {
        String text = hue_axis_edit_.GetTextUtf8();
        if(LooksLikeColorExpression_(text))
            TryApplyColorText(text, false);
    };
    gain_axis_edit_.WhenChange = [=] {
        String text = gain_axis_edit_.GetTextUtf8();
        if(LooksLikeColorExpression_(text))
            TryApplyColorText(text, false);
    };
    hue_axis_edit_.WhenAction = [=] { commit_axis(hue_axis_edit_, true); };
    gain_axis_edit_.WhenAction = [=] { commit_axis(gain_axis_edit_, false); };

    hue_axis_slider_.WhenPaintTrack = [=](Draw& draw, const UiSlider::PaintContext& context, bool& handled) {
        DrawHueTrack_(draw, context.track);
        handled = true;
    };
    hue_axis_slider_.WhenPaintActiveTrack = [=](Draw&, const UiSlider::PaintContext&, bool& handled) { handled = true; };
    gain_axis_slider_.WhenPaintTrack = [=](Draw& draw, const UiSlider::PaintContext& context, bool& handled) {
        int color_hue = 0, s = 0, v = 0;
        ColorToHsv_(GetColor(), color_hue, s, v);
        DrawValueTrack_(draw, context.track, s > 0 && v > 0 ? color_hue : remembered_hue_, s);
        handled = true;
    };
    gain_axis_slider_.WhenPaintActiveTrack = [=](Draw&, const UiSlider::PaintContext&, bool& handled) { handled = true; };

    for(int i = 0; i < CHANNEL_COUNT; i++) {
        const int mode = i;
        channel_button_[i].WhenAction = [=] { SetChannelMode((ChannelMode)mode); };
        channel_group_[i]->WhenValue = [=](int row, double value, bool final_commit) {
            HandleChannelValue((ChannelMode)mode, row, value, final_commit);
        };
        channel_group_[i]->WhenColorText = [=](String text, bool final_commit) {
            TryApplyColorText(text, final_commit);
        };
    }
    channel_mode_drop_.WhenSelectData = [=](const Value& data) {
        if(!IsNull(data))
            SetChannelMode((ChannelMode)(int)data);
    };

    alpha_toggle_.WhenAction = [=] { SetAlphaEnabled(alpha_toggle_.IsOn()); };

    ReadoutRow *readouts[] = {
        ~readout_hsv_, ~readout_hex_, ~readout_hsl_,
        ~readout_rgb_float_, ~readout_cmyk_, ~readout_rgb_int_
    };
    for(int i = 0; i < 6; i++) {
        ReadoutRow *row = readouts[i];
        const int readout = i;
        row->WhenLiveText = [=](String text) {
            if(LooksLikeColorExpression_(text))
                TryApplyReadoutText(readout, text, false);
        };
        row->WhenCommitText = [=](String text) {
            if(!TryApplyReadoutText(readout, text, true))
                SyncReadouts();
        };
    }

    palette_category_drop_.WhenSelectData = [=](const Value& data) {
        if(IsNull(data))
            return;
        palette_category_ = minmax((int)data, 0, 2);
        PopulatePaletteSelectors();
        RefreshPaletteGrid();
    };
    palette_drop_.WhenSelectData = [=](const Value& data) {
        if(!IsNull(data))
            SetPaletteIndex((int)data);
    };
    palette_grid_->WhenPick = [=](int index, SlotValue value) { HandlePalettePick(index, value); };
    palette_grid_->WhenDoublePick = [=](int index, SlotValue value) {
        HandlePalettePick(index, value);
        UseSelectedPaletteColor();
    };
    recent_grid_->WhenDoublePick = [=](int, SlotValue value) {
        CommitSlotValue(active_slot_, value.color, value.alpha, true);
    };
    stash_grid_->WhenPick = [=](int index, SlotValue value) { HandleStashPick(index, value); };
    stash_grid_->WhenDoublePick = [=](int index, SlotValue value) {
        HandleStashPick(index, value);
        UseSelectedStashColor();
    };
    stash_grid_->WhenDropValue = [=](SlotValue value) { HandlePaletteDropToStash(value); };
    palette_use_button_.WhenAction = [=] { UseSelectedPaletteColor(); };
    palette_save_button_.WhenAction = [=] { SaveSelectedPaletteToStash(); };
    stash_use_button_.WhenAction = [=] { UseSelectedStashColor(); };
    stash_save_active_button_.WhenAction = [=] { AddUserSwatch(GetColor(), GetAlpha()); };

    for(int i = 0; i < 4; i++) {
        const int index = i;
        curve_button_[i].WhenAction = [=] { SelectCurveChannel(index); };
        curve_editor_[i].WhenChanging = [=] { ApplyCurves(false); };
        curve_editor_[i].WhenAction = [=] { ApplyCurves(true); };
    }
    curve_capture_button_.WhenAction = [=] { CaptureCurveSource(); };
    curve_reset_button_.WhenAction = [=] { ResetCurves(); };

    harmony_drop_.WhenSelectData = [=](const Value& data) {
        if(!IsNull(data))
            SetHarmonyMode((HarmonyMode)(int)data);
    };
    for(int i = 0; i < 3; i++) {
        const int mode = i;
        generator_mode_button_[i].WhenAction = [=] { SetGeneratorMode(mode); };
    }
    generator_wheel_->WhenChange = [=](bool) {
        RefreshGeneratorPalette();
        int gain = generator_wheel_->GetGlobalGain();
        generator_gain_slider_.SetValue(gain);
        generator_gain_edit_.SetTextUtf8(AsString(gain));
        SaveSharedSession(false);
    };
    auto update_generator_gain = [=](int value) {
        value = ClampPercent_(value);
        generator_wheel_->SetGlobalGain(value);
        generator_gain_slider_.SetValue(value);
        generator_gain_edit_.SetTextUtf8(AsString(value));
        RefreshGeneratorPalette();
        SaveSharedSession(false);
    };
    generator_gain_slider_.WhenChanging = [=] { update_generator_gain(generator_gain_slider_.GetValue()); };
    generator_gain_slider_.WhenAction = [=] { update_generator_gain(generator_gain_slider_.GetValue()); };
    generator_gain_edit_.WhenAction = [=] {
        double value = 0;
        if(ParseSingleNumber_(generator_gain_edit_.GetTextUtf8(), value))
            update_generator_gain(int(value + 0.5));
    };
    auto update_generator_count = [=](int count) {
        count = minmax(count, 2, 9);
        generator_wheel_->SetPaletteCount(count);
        if(generator_mode_ == 1)
            generator_image_preview_->SetSampleCount(count);
        generator_count_slider_.SetValue(count);
        generator_count_edit_.SetTextUtf8(AsString(count));
        RefreshGeneratorPalette();
        RefreshLayout();
        SaveSharedSession(false);
    };
    generator_count_slider_.WhenChanging = [=] { update_generator_count(generator_count_slider_.GetValue()); };
    generator_count_slider_.WhenAction = [=] { update_generator_count(generator_count_slider_.GetValue()); };
    generator_count_edit_.WhenAction = [=] {
        double value = 0;
        if(ParseSingleNumber_(generator_count_edit_.GetTextUtf8(), value))
            update_generator_count(int(value + 0.5));
    };
    generator_image_preview_->WhenSamplesChanged = [=] {
        if(generator_mode_ != 1)
            return;
        generated_swatches_.Clear();
        Vector<SlotValue> base_values;
        Vector<Color> samples = generator_image_preview_->GetSamples();
        int gain = generator_wheel_->GetGlobalGain();
        for(int i = 0; i < samples.GetCount(); i++) {
            int h = 0, s = 0, v = 0;
            ColorToHsv_(samples[i], h, s, v);
            base_values.Add(MakeSlot_(HsvToColor_(h, s, v * gain / 100.0), 255,
                                      Format("Image sample %d", i + 1)));
        }
        Vector<SlotValue> values = clone(base_values);
        generated_base_count_ = base_values.GetCount();
        const int white_mix[] = { 205, 145, 85 };
        const int black_mix[] = { 40, 80, 125, 175 };
        for(int mix : white_mix)
            for(const SlotValue& base : base_values)
                values.Add(MakeSlot_(Blend(base.color, White(), mix), 255, "Tint"));
        for(int mix : black_mix)
            for(const SlotValue& base : base_values)
                values.Add(MakeSlot_(Blend(base.color, Black(), mix), 255, "Shade"));
        for(const SlotValue& value : values) {
            SlotData& item = generated_swatches_.Add();
            item.color = value.color;
            item.alpha = value.alpha;
            item.label = value.label;
        }
        generator_grid_->SetFlow(max(1, base_values.GetCount()), true);
        generator_grid_->SetItems(values);
    };
    generator_clear_samples_button_.WhenAction = [=] {
        generator_image_preview_->ClearSamples();
        generated_swatches_.Clear();
        generator_grid_->SetItems(Vector<SlotValue>());
    };
    generator_refresh_button_.WhenAction = [=] { RefreshGeneratorPalette(); };
    generator_load_image_button_.WhenAction = [=] { LoadGeneratorImage(); };
    generator_grid_->WhenPick = [=](int index, SlotValue value) { HandleGeneratorPick(index, value); };
    generator_grid_->WhenDoublePick = [=](int index, SlotValue value) {
        HandleGeneratorPick(index, value);
        UseGeneratedColor();
    };
    generator_use_button_.WhenAction = [=] { UseGeneratedColor(); };
    generator_save_button_.WhenAction = [=] { SaveGeneratedToStash(); };
}

UiColorPicker& UiColorPicker::SetCustomStyle(const Style& style)
{
    style_ = style;
    has_custom_style_ = true;
    OnStyleChanged();
    return *this;
}

UiColorPicker& UiColorPicker::ClearCustomStyle()
{
    if(!has_custom_style_)
        return *this;
    has_custom_style_ = false;
    theme_revision_ = 0;
    OnStyleChanged();
    return *this;
}

void UiColorPicker::OnStyleChanged()
{
    InvalidateStyleCache();
    SyncThemeToChildren();
    RefreshLayout();
    Refresh();
}

UiColorPicker::Style& UiColorPicker::StyleEdit()
{
    if(!has_custom_style_) {
        style_ = GetEffectiveStyle();
        has_custom_style_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

void UiColorPicker::InvalidateStyleCache()
{
    theme_revision_ = 0;
    children_theme_revision_ = 0;
    children_style_dirty_ = true;
}

void UiColorPicker::SyncThemeStyle()
{
    if(has_custom_style_)
        return;

    uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ != 0 && theme_revision_ == revision)
        return;

    themed_style_ = StyleDefault();
    const bool dark = UiThemeDetail::ResolveEffectiveMode(UiTheme::GetContext().mode) == UiThemeMode::Dark;
    Color face = dark ? Color(16, 18, 22) : Color(250, 252, 255);
    Color frame = dark ? Color(48, 52, 60) : Color(202, 210, 222);
    Color ink = dark ? Color(228, 232, 238) : Color(27, 33, 42);
    for(int i = 0; i < 4; i++) {
        themed_style_.palette.face[i] = UiFill::Solid(face);
        themed_style_.palette.frame[i] = frame;
        themed_style_.palette.ink[i] = ink;
        themed_style_.palette.icon[i] = ink;
    }
    theme_revision_ = revision;
    children_style_dirty_ = true;
}

const UiColorPicker::Style& UiColorPicker::GetEffectiveStyle() const
{
    if(has_custom_style_)
        return style_;
    const_cast<UiColorPicker *>(this)->SyncThemeStyle();
    return themed_style_;
}

UiColorPicker& UiColorPicker::SetPageMode(PageMode mode)
{
    if(mode < PAGE_COLOR || mode >= PAGE_COUNT)
        mode = PAGE_COLOR;
    if(page_mode_ == mode) {
        SyncPageButtons();
        return *this;
    }

    page_mode_ = mode;
    page_stack_.SetActivePage((int)mode);
    if(mode == PAGE_CURVES)
        curve_source_color_ = GetColor();
    else if(mode == PAGE_GENERATOR)
        RefreshGeneratorPalette();
    SyncPageButtons();
    SaveSharedSession(false);
    RefreshLayout();
    Refresh();
    if(WhenPageChanged)
        WhenPageChanged(page_mode_);
    return *this;
}

UiColorPicker& UiColorPicker::SetChannelMode(ChannelMode mode)
{
    if(mode < CHANNEL_RGB_FLOAT || mode >= CHANNEL_COUNT)
        mode = CHANNEL_RGB_FLOAT;
    if(channel_mode_ == mode) {
        SyncChannelButtons();
        return *this;
    }

    channel_mode_ = mode;
    channel_stack_.SetActivePage((int)mode);
    RefreshChannelModeValues(mode);
    SyncChannelButtons();
    SaveSharedSession(false);
    RefreshLayout();
    if(WhenChannelModeChanged)
        WhenChannelModeChanged(channel_mode_);
    return *this;
}

UiColorPicker& UiColorPicker::SetSlotCount(int count)
{
    count = minmax(count, 1, 4);
    if(slot_count_ == count)
        return *this;
    slot_count_ = count;
    if(active_slot_ >= slot_count_)
        active_slot_ = slot_count_ - 1;
    SyncSlotButtons();
    SaveSharedSession(true);
    RefreshLayout();
    return *this;
}

UiColorPicker& UiColorPicker::SetActiveSlot(int index)
{
    if(index < 0 || index >= slot_count_ || index == active_slot_)
        return *this;
    FinishLiveGesture();
    active_slot_ = index;
    curve_source_color_ = GetColor();
    SyncAllFromActiveSlot();
    SaveSharedSession(false);
    if(WhenSlotChanged)
        WhenSlotChanged(active_slot_);
    return *this;
}

UiColorPicker& UiColorPicker::SetSlotColor(int index, Color color, bool fire)
{
    if(index < 0 || index >= slots_.GetCount() || IsNull(color))
        return *this;
    if(slots_[index].color == color)
        return *this;

    previous_slots_[index] = slots_[index];
    slots_[index].color = color;
    if(index == active_slot_)
        SyncAllFromActiveSlot();
    else
        SyncSlotButtons();

    if(fire) {
        PushRecentColor(color, slots_[index].alpha);
        SaveSharedSession(true);
        if(WhenAction)
            WhenAction();
    }
    return *this;
}

Color UiColorPicker::GetSlotColor(int index) const
{
    return index >= 0 && index < slots_.GetCount() ? slots_[index].color : Black();
}

UiColorPicker& UiColorPicker::SetSlotAlpha(int index, int alpha, bool fire)
{
    if(index < 0 || index >= slots_.GetCount())
        return *this;
    alpha = ClampByte_(alpha);
    if(slots_[index].alpha == alpha)
        return *this;

    previous_slots_[index] = slots_[index];
    slots_[index].alpha = alpha;
    if(index == active_slot_)
        SyncAllFromActiveSlot();
    else
        SyncSlotButtons();

    if(fire) {
        SaveSharedSession(true);
        if(WhenAction)
            WhenAction();
    }
    return *this;
}

int UiColorPicker::GetSlotAlpha(int index) const
{
    if(index < 0 || index >= slots_.GetCount())
        return 255;
    return alpha_enabled_ ? slots_[index].alpha : 255;
}

UiColorPicker& UiColorPicker::SetSlot(int index, Color color, int alpha, bool fire)
{
    if(index < 0 || index >= slots_.GetCount() || IsNull(color))
        return *this;
    alpha = ClampByte_(alpha);
    if(slots_[index].color == color && slots_[index].alpha == alpha)
        return *this;

    previous_slots_[index] = slots_[index];
    slots_[index].color = color;
    slots_[index].alpha = alpha;
    if(index == active_slot_)
        SyncAllFromActiveSlot();
    else
        SyncSlotButtons();

    if(fire) {
        PushRecentColor(color, alpha);
        SaveSharedSession(true);
        if(WhenAction)
            WhenAction();
    }
    return *this;
}

UiColorPicker::SlotValue UiColorPicker::GetSlot(int index) const
{
    if(index < 0 || index >= slots_.GetCount())
        return SlotValue();
    return slots_[index].Export(alpha_enabled_);
}

Vector<UiColorPicker::SlotValue> UiColorPicker::GetSlots() const
{
    Vector<SlotValue> output;
    for(int i = 0; i < slot_count_; i++)
        output.Add(GetSlot(i));
    return output;
}

UiColorPicker& UiColorPicker::SetColor(Color color, bool fire)
{
    return SetSlotColor(active_slot_, color, fire);
}

UiColorPicker& UiColorPicker::SetAlpha(int alpha, bool fire)
{
    return SetSlotAlpha(active_slot_, alpha, fire);
}

UiColorPicker& UiColorPicker::SetSlotLabel(int index, const String& label)
{
    if(index < 0 || index >= slots_.GetCount())
        return *this;
    if(slots_[index].label == label)
        return *this;
    slots_[index].label = label;
    SyncSlotButtons();
    SaveSharedSession(true);
    return *this;
}

String UiColorPicker::GetSlotLabel(int index) const
{
    return index >= 0 && index < slots_.GetCount() ? slots_[index].label : String();
}

UiColorPicker& UiColorPicker::SetAlphaEnabled(bool on)
{
    if(alpha_enabled_ == on) {
        UpdateAlphaAvailability();
        return *this;
    }
    alpha_enabled_ = on;
    alpha_toggle_.SetOn(on);
    UpdateAlphaAvailability();
    SyncAllFromActiveSlot();
    SaveSharedSession(false);
    return *this;
}

UiColorPicker& UiColorPicker::SetSpectrumMode(SpectrumMode mode)
{
    if(mode < SPECTRUM_HSV_RECT || mode > SPECTRUM_RGB_SPECTRUM)
        mode = SPECTRUM_HUE_STRIP;
    if(spectrum_mode_ == mode) {
        SyncSpectrumMode();
        return *this;
    }
    spectrum_mode_ = mode;
    SyncSpectrumMode();
    SaveSharedSession(false);
    return *this;
}

UiColorPicker& UiColorPicker::SetHarmonyMode(HarmonyMode mode)
{
    if(mode < HARMONY_CUSTOM || mode > HARMONY_IMAGE_EXTRACT)
        mode = HARMONY_ANALOGOUS;
    harmony_mode_ = mode;
    harmony_drop_.SetDataSilently((int)mode);
    int default_count = 5;
    if(mode == HARMONY_COMPLEMENTARY)
        default_count = 2;
    else if(mode == HARMONY_SPLIT_COMPLEMENTARY || mode == HARMONY_TRIAD)
        default_count = 3;
    else if(mode == HARMONY_SQUARE)
        default_count = 4;
    generator_wheel_->SetPaletteCount(default_count);
    generator_wheel_->SetMode(mode);
    generator_count_slider_.SetValue(default_count);
    generator_count_edit_.SetTextUtf8(AsString(default_count));
    RefreshGeneratorPalette();
    SaveSharedSession(false);
    return *this;
}

UiColorPicker& UiColorPicker::SetGeneratorImage(const Image& image)
{
    generator_image_ = image;
    generator_image_preview_->SetImage(generator_image_);
    generator_image_preview_->SetSampleCount(generator_wheel_->GetPaletteCount());
    if(!image.IsEmpty())
        SetGeneratorMode(1);
    harmony_drop_.SetDataSilently((int)harmony_mode_);
    RefreshGeneratorPalette();
    return *this;
}

UiColorPicker& UiColorPicker::ExtractGeneratorPalette(int count)
{
    generated_swatches_.Clear();
    Vector<SlotValue> extracted = ExtractImagePalette_(generator_image_, count);
    for(const SlotValue& value : extracted) {
        SlotData& item = generated_swatches_.Add();
        item.color = value.color;
        item.alpha = value.alpha;
        item.label = value.label;
    }
    Vector<SlotValue> values;
    for(const SlotData& item : generated_swatches_)
        values.Add(item.Export(true));
    generator_grid_->SetGrid(6, max(1, (values.GetCount() + 5) / 6));
    generator_grid_->SetItems(values);
    selected_generated_index_ = -1;
    generator_grid_->SetSelectedIndex(-1);
    return *this;
}

UiColorPicker& UiColorPicker::AddUserSwatch(Color color)
{
    return AddUserSwatch(color, GetAlpha());
}

UiColorPicker& UiColorPicker::AddUserSwatch(Color color, int alpha)
{
    if(IsNull(color))
        return *this;
    alpha = ClampByte_(alpha);

    for(int i = 0; i < user_swatches_.GetCount(); i++) {
        if(user_swatches_[i].color == color && user_swatches_[i].alpha == alpha) {
            SlotData existing = user_swatches_[i];
            user_swatches_.Remove(i);
            user_swatches_.Insert(0, existing);
            selected_stash_index_ = 0;
            selected_stash_value_ = existing.Export(true);
            Vector<SlotValue> values;
            for(const SlotData& item : user_swatches_)
                values.Add(item.Export(true));
            stash_grid_->SetItems(values);
            stash_grid_->SetSelectedIndex(0);
            SaveSharedSession(false);
            return *this;
        }
    }

    SlotData item;
    item.color = color;
    item.alpha = alpha;
    item.label = Format("Stash %d", user_swatches_.GetCount() + 1);
    user_swatches_.Insert(0, item);
    while(user_swatches_.GetCount() > 108)
        user_swatches_.Remove(user_swatches_.GetCount() - 1);

    Vector<SlotValue> values;
    for(const SlotData& swatch : user_swatches_)
        values.Add(swatch.Export(true));
    stash_grid_->SetItems(values);
    selected_stash_index_ = 0;
    selected_stash_value_ = item.Export(true);
    stash_grid_->SetSelectedIndex(0);
    SaveSharedSession(false);
    return *this;
}

UiColorPicker& UiColorPicker::ClearUserSwatches()
{
    user_swatches_.Clear();
    selected_stash_index_ = -1;
    selected_stash_value_ = SlotValue();
    stash_grid_->SetItems(Vector<SlotValue>());
    stash_grid_->SetSelectedIndex(-1);
    SaveSharedSession(false);
    return *this;
}

UiColorPicker& UiColorPicker::ClearRecentSwatches()
{
    recent_swatches_.Clear();
    recent_grid_->SetItems(Vector<SlotValue>());
    SaveSharedSession(false);
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

UiColorPicker& UiColorPicker::EnableSessionPersistence(bool on)
{
    if(session_persistence_ == on)
        return *this;
    session_persistence_ = on;
    if(on)
        LoadSharedSession();
    return *this;
}

void UiColorPicker::ClearSharedSession()
{
    SharedColorPickerSession_& session = SharedSession_();
    session.initialized = false;
    session.slots.Clear();
    session.previous.Clear();
    session.recent.Clear();
    session.stash.Clear();
    session.active_slot = 0;
    session.alpha_enabled = true;
    session.page_mode = PAGE_COLOR;
    session.spectrum_mode = SPECTRUM_HUE_STRIP;
    session.channel_mode = CHANNEL_RGB_FLOAT;
    session.harmony_mode = HARMONY_TRIAD;
    session.palette_category = 0;
    session.palette_index = 0;
    session.selected_curve_channel = 0;
    session.generator_mode = 2;
    session.generator_count = 3;
    session.generator_gain = 100;
    session.generator_handles.Clear();
    session.generator_values.Clear();
    session.generator_image = Image();
    for(int i = 0; i < 4; i++)
        session.curves[i] = ShadowLinear();
}

bool UiColorPicker::IsScreenEyedropperAvailable() const
{
#ifdef PLATFORM_WIN32
    return true;
#else
    return false;
#endif
}

UiColorPicker& UiColorPicker::BeginScreenEyedropper()
{
    if(IsScreenEyedropperAvailable())
        StartEyedropper();
    return *this;
}

String UiColorPicker::FormatActiveHex() const
{
    return FormatHex_(GetColor());
}

String UiColorPicker::FormatActiveHex8() const
{
    return FormatHex8_(GetColor(), GetAlpha());
}

String UiColorPicker::FormatSlotHex8(int index) const
{
    return index >= 0 && index < slots_.GetCount()
         ? FormatHex8_(slots_[index].color, alpha_enabled_ ? slots_[index].alpha : 255)
         : String();
}

String UiColorPicker::FormatActiveRgb8() const
{
    Color color = GetColor();
    return Format("%d, %d, %d", color.GetR(), color.GetG(), color.GetB());
}

String UiColorPicker::FormatActiveRgbUnit() const
{
    Color color = GetColor();
    return Format("%.4f, %.4f, %.4f",
                  color.GetR() / 255.0,
                  color.GetG() / 255.0,
                  color.GetB() / 255.0);
}

String UiColorPicker::FormatActiveHsv() const
{
    int h = 0, s = 0, v = 0;
    ColorToHsv_(GetColor(), h, s, v);
    return Format("%d, %d, %d", h, s, v);
}

String UiColorPicker::FormatActiveAlpha() const
{
    return AsString(GetAlpha());
}

bool UiColorPicker::ParseColorText(const String& text, Color& color, int& alpha)
{
    ParsedColor_ parsed;
    if(!ParseColorText_(text, parsed))
        return false;
    color = parsed.color;
    alpha = parsed.has_alpha ? parsed.alpha : 255;
    return true;
}

void UiColorPicker::LoadSharedSession()
{
    if(!session_persistence_)
        return;

    SharedColorPickerSession_& session = SharedSession_();
    if(!session.initialized) {
        SaveSharedSession(true);
        return;
    }

    if(session.slots.GetCount() >= 1) {
        slot_count_ = min(4, session.slots.GetCount());
        for(int i = 0; i < slot_count_; i++) {
            slots_[i].color = session.slots[i].color;
            slots_[i].alpha = ClampByte_(session.slots[i].alpha);
            slots_[i].label = session.slots[i].label.IsEmpty() ? Format("C%d", i + 1) : session.slots[i].label;
        }
    }
    if(session.previous.GetCount() >= slot_count_) {
        for(int i = 0; i < slot_count_; i++) {
            previous_slots_[i].color = session.previous[i].color;
            previous_slots_[i].alpha = ClampByte_(session.previous[i].alpha);
            previous_slots_[i].label = slots_[i].label;
        }
    }
    else
        previous_slots_ = clone(slots_);

    recent_swatches_.Clear();
    for(const SlotValue& value : session.recent) {
        SlotData& item = recent_swatches_.Add();
        item.color = value.color;
        item.alpha = ClampByte_(value.alpha);
        item.label = value.label;
    }

    user_swatches_.Clear();
    for(const SlotValue& value : session.stash) {
        SlotData& item = user_swatches_.Add();
        item.color = value.color;
        item.alpha = ClampByte_(value.alpha);
        item.label = value.label;
    }

    active_slot_ = minmax(session.active_slot, 0, slot_count_ - 1);
    alpha_enabled_ = session.alpha_enabled;
    page_mode_ = (PageMode)minmax(session.page_mode, (int)PAGE_COLOR, (int)PAGE_GENERATOR);
    spectrum_mode_ = (SpectrumMode)minmax(session.spectrum_mode, (int)SPECTRUM_HSV_RECT, (int)SPECTRUM_RGB_SPECTRUM);
    channel_mode_ = (ChannelMode)minmax(session.channel_mode, (int)CHANNEL_RGB_FLOAT, (int)CHANNEL_COUNT - 1);
    harmony_mode_ = (HarmonyMode)minmax(session.harmony_mode, (int)HARMONY_CUSTOM, (int)HARMONY_IMAGE_EXTRACT);
    palette_category_ = minmax(session.palette_category, 0, 2);
    palette_index_ = minmax(session.palette_index, 0, PaletteRegistry_().GetCount() - 1);
    selected_curve_channel_ = minmax(session.selected_curve_channel, 0, 3);
    generator_mode_ = minmax(session.generator_mode, 0, 2);
    generator_image_ = session.generator_image;
    generator_image_preview_->SetImage(generator_image_);
    generator_wheel_->SetBase(GetColor());
    generator_wheel_->SetMode(harmony_mode_);
    generator_wheel_->SetPaletteCount(minmax(session.generator_count, 2, 9));
    generator_wheel_->SetGlobalGain(session.generator_gain);
    generator_wheel_->SetState(session.generator_handles, session.generator_values);
    generator_count_slider_.SetValue(generator_wheel_->GetPaletteCount());
    generator_count_edit_.SetTextUtf8(AsString(generator_wheel_->GetPaletteCount()));
    generator_gain_slider_.SetValue(generator_wheel_->GetGlobalGain());
    generator_gain_edit_.SetTextUtf8(AsString(generator_wheel_->GetGlobalGain()));
    for(int i = 0; i < 4; i++)
        curve_editor_[i].SetCurve(session.curves[i]);
    curve_stack_.SetActivePage(selected_curve_channel_);
    alpha_toggle_.SetOn(alpha_enabled_);
    UpdateAlphaAvailability();

    page_stack_.SetActivePage((int)page_mode_);
    channel_stack_.SetActivePage((int)channel_mode_);
    harmony_drop_.SetDataSilently((int)harmony_mode_);
    spectrum_mode_drop_.SetDataSilently((int)spectrum_mode_);

    PopulatePaletteSelectors();

    Vector<SlotValue> recent_values;
    for(const SlotData& item : recent_swatches_)
        recent_values.Add(item.Export(true));
    recent_grid_->SetItems(recent_values);

    Vector<SlotValue> stash_values;
    for(const SlotData& item : user_swatches_)
        stash_values.Add(item.Export(true));
    stash_grid_->SetItems(stash_values);
    SetGeneratorMode(generator_mode_);
}

void UiColorPicker::SaveSharedSession(bool include_slots)
{
    if(!session_persistence_)
        return;

    SharedColorPickerSession_& session = SharedSession_();
    session.initialized = true;
    if(include_slots) {
        session.slots.Clear();
        session.previous.Clear();
        for(int i = 0; i < slot_count_; i++) {
            session.slots.Add(slots_[i].Export(true));
            session.previous.Add(previous_slots_[i].Export(true));
        }
    }

    session.recent.Clear();
    for(const SlotData& item : recent_swatches_)
        session.recent.Add(item.Export(true));
    session.stash.Clear();
    for(const SlotData& item : user_swatches_)
        session.stash.Add(item.Export(true));

    session.active_slot = active_slot_;
    session.alpha_enabled = alpha_enabled_;
    session.page_mode = (int)page_mode_;
    session.spectrum_mode = (int)spectrum_mode_;
    session.channel_mode = (int)channel_mode_;
    session.harmony_mode = (int)harmony_mode_;
    session.palette_category = palette_category_;
    session.palette_index = palette_index_;
    session.selected_curve_channel = selected_curve_channel_;
    session.generator_mode = generator_mode_;
    session.generator_count = generator_wheel_->GetPaletteCount();
    session.generator_gain = generator_wheel_->GetGlobalGain();
    session.generator_handles = clone(generator_wheel_->GetHandles());
    session.generator_values = clone(generator_wheel_->GetValues());
    session.generator_image = generator_image_;
    for(int i = 0; i < 4; i++)
        session.curves[i] = curve_editor_[i].GetCurve();
}

void UiColorPicker::HandleAccept()
{
    FinishLiveGesture();
    accepted_ = true;
    opening_slots_ = clone(slots_);
    SaveSharedSession(true);
    if(WhenAccept)
        WhenAccept();
}

void UiColorPicker::HandleCancel()
{
    if(eyedropper_active_)
        StopEyedropper(false);
    live_gesture_ = false;
    live_slot_ = -1;
    slots_ = clone(opening_slots_);
    SyncAllFromActiveSlot();
    accepted_ = false;
    SaveSharedSession(false);
    if(WhenCancel)
        WhenCancel();
}

void UiColorPicker::SyncAllFromActiveSlot()
{
    if(active_slot_ < 0 || active_slot_ >= slots_.GetCount())
        return;

    bool old_sync = syncing_controls_;
    syncing_controls_ = true;

    Color color = slots_[active_slot_].color;
    int hue = 0, saturation = 0, value = 0;
    ColorToHsv_(color, hue, saturation, value);
    if(saturation > 0 && value > 0)
        remembered_hue_ = hue;
    else
        hue = remembered_hue_;

    hue_axis_slider_.SetValue(hue);
    gain_axis_slider_.SetValue(value);
    if(!(hue_axis_edit_.HasFocus() && hue_axis_edit_.IsDirty())) {
        hue_axis_edit_.SetTextUtf8(AsString(hue));
        hue_axis_edit_.ClearDirty();
    }
    if(!(gain_axis_edit_.HasFocus() && gain_axis_edit_.IsDirty())) {
        gain_axis_edit_.SetTextUtf8(AsString(value));
        gain_axis_edit_.ClearDirty();
    }

    color_field_->SetState(spectrum_mode_, color, hue, value);
    SyncChannelGroups();
    SyncReadouts();
    SyncSlotButtons();

    SlotValue active = slots_[active_slot_].Export(alpha_enabled_);
    recent_grid_->SetActive(active);
    palette_grid_->SetActive(active);
    stash_grid_->SetActive(active);
    generator_grid_->SetActive(active);

    footer_information_.SetText(Format("C%d  %s  Alpha %d",
                                       active_slot_ + 1,
                                       FormatHex_(color),
                                       GetAlpha()));

    syncing_controls_ = old_sync;
}

void UiColorPicker::SyncChannelGroups()
{
    RefreshChannelModeValues(channel_mode_);
}

void UiColorPicker::SyncReadouts()
{
    Color color = GetColor();
    int alpha = GetAlpha();
    int h = 0, s = 0, v = 0;
    int hh = 0, hs = 0, l = 0;
    int c = 0, m = 0, y = 0, k = 0;
    ColorToHsv_(color, h, s, v);
    if(s == 0 || v == 0)
        h = remembered_hue_;
    ColorToHsl_(color, hh, hs, l);
    ColorToCmyk_(color, c, m, y, k);

    readout_hsv_->SetValue(Format("%d, %d, %d, %.4f", h, s, v, alpha / 255.0));
    readout_hex_->SetValue(FormatHex8_(color, alpha));
    readout_hsl_->SetValue(Format("%d, %d, %d, %.4f", hh, hs, l, alpha / 255.0));
    readout_rgb_float_->SetValue(Format("%.4f, %.4f, %.4f, %.4f",
                                        color.GetR() / 255.0,
                                        color.GetG() / 255.0,
                                        color.GetB() / 255.0,
                                        alpha / 255.0));
    readout_cmyk_->SetValue(Format("%d, %d, %d, %d, %.4f",
                                   c, m, y, k, alpha / 255.0));
    readout_rgb_int_->SetValue(Format("%d, %d, %d, %d",
                                      color.GetR(), color.GetG(), color.GetB(), alpha));
}

void UiColorPicker::SyncSlotButtons()
{
    SlotValue active = slots_[active_slot_].Export(alpha_enabled_);
    current_slot_button_->SetValue(active, true, alpha_enabled_);

    for(int i = 0; i < 4; i++) {
        bool visible = i < slot_count_;
        primary_slot_button_[i]->Show(visible);
        previous_slot_button_[i]->Show(visible);
        if(!visible)
            continue;
        primary_slot_button_[i]->SetValue(slots_[i].Export(alpha_enabled_), i == active_slot_, alpha_enabled_);
        previous_slot_button_[i]->SetValue(previous_slots_[i].Export(alpha_enabled_), false, alpha_enabled_);
    }
}

void UiColorPicker::SyncSpectrumMode()
{
    spectrum_mode_drop_.SetDataSilently((int)spectrum_mode_);
    int h = 0, s = 0, v = 0;
    ColorToHsv_(GetColor(), h, s, v);
    if(s > 0 && v > 0)
        remembered_hue_ = h;
    else
        h = remembered_hue_;
    color_field_->SetState(spectrum_mode_, GetColor(), h, v);
    Refresh();
}

void UiColorPicker::SyncPageButtons()
{
    page_stack_.SetActivePage((int)page_mode_);
    children_style_dirty_ = true;
    for(int i = 0; i < PAGE_COUNT; i++)
        page_button_[i].SetChecked(i == (int)page_mode_);
    SyncThemeToChildren();
}

void UiColorPicker::SyncChannelButtons()
{
    channel_stack_.SetActivePage((int)channel_mode_);
    channel_mode_drop_.SetDataSilently((int)channel_mode_);
    children_style_dirty_ = true;
    for(int i = 0; i < CHANNEL_COUNT; i++)
        channel_button_[i].SetChecked(i == (int)channel_mode_);
    SyncThemeToChildren();
}

void UiColorPicker::CommitColor(Color color, bool final_commit)
{
    if(IsNull(color))
        return;
    CommitSlotValue(active_slot_, color, slots_[active_slot_].alpha, final_commit);
}

void UiColorPicker::CommitAlpha(int alpha, bool final_commit)
{
    CommitSlotValue(active_slot_, slots_[active_slot_].color, ClampByte_(alpha), final_commit);
}

void UiColorPicker::CommitSlotValue(int slot, Color color, int alpha, bool final_commit)
{
    if(slot < 0 || slot >= slots_.GetCount() || IsNull(color))
        return;
    alpha = ClampByte_(alpha);

    bool changed = slots_[slot].color != color || slots_[slot].alpha != alpha;
    if(!changed && !live_gesture_)
        return;

    if(!live_gesture_ || live_slot_ != slot) {
        if(live_gesture_)
            FinishLiveGesture();
        live_origin_ = slots_[slot];
        live_slot_ = slot;
        live_gesture_ = true;
    }

    slots_[slot].color = color;
    slots_[slot].alpha = alpha;
    if(slot == active_slot_)
        SyncAllFromActiveSlot();
    else
        SyncSlotButtons();

    if(final_commit) {
        previous_slots_[slot] = live_origin_;
        live_gesture_ = false;
        live_slot_ = -1;
        PushRecentColor(color, alpha);
        SaveSharedSession(true);
        if(WhenAction)
            WhenAction();
        return;
    }

    dword now = msecs();
    if(live_callback_ms_ == 0 || now - live_callback_ms_ >= 16) {
        live_callback_ms_ = now;
        if(WhenChanging)
            WhenChanging();
    }
}

void UiColorPicker::FinishLiveGesture()
{
    if(!live_gesture_ || live_slot_ < 0 || live_slot_ >= slots_.GetCount())
        return;

    int slot = live_slot_;
    previous_slots_[slot] = live_origin_;
    live_gesture_ = false;
    live_slot_ = -1;
    PushRecentColor(slots_[slot].color, slots_[slot].alpha);
    SaveSharedSession(true);
    SyncSlotButtons();
    if(WhenAction)
        WhenAction();
}

void UiColorPicker::PushRecentColor(Color color, int alpha)
{
    alpha = ClampByte_(alpha);
    for(int i = 0; i < recent_swatches_.GetCount(); i++) {
        if(recent_swatches_[i].color == color && recent_swatches_[i].alpha == alpha) {
            recent_swatches_.Remove(i);
            break;
        }
    }

    SlotData item;
    item.color = color;
    item.alpha = alpha;
    item.label = "Recent";
    recent_swatches_.Insert(0, item);
    while(recent_swatches_.GetCount() > 12)
        recent_swatches_.Remove(recent_swatches_.GetCount() - 1);

    Vector<SlotValue> values;
    for(const SlotData& swatch : recent_swatches_)
        values.Add(swatch.Export(true));
    recent_grid_->SetItems(values);
}

void UiColorPicker::HandlePrimarySlot(int index)
{
    SetActiveSlot(index);
}

void UiColorPicker::HandlePreviousSlot(int index)
{
    if(index < 0 || index >= slot_count_)
        return;
    SlotData old = slots_[index];
    slots_[index] = previous_slots_[index];
    previous_slots_[index] = old;
    active_slot_ = index;
    SyncAllFromActiveSlot();
    PushRecentColor(slots_[index].color, slots_[index].alpha);
    SaveSharedSession(true);
    if(WhenSlotChanged)
        WhenSlotChanged(index);
    if(WhenAction)
        WhenAction();
}

void UiColorPicker::HandleColorDrop(int slot, const SlotValue& value)
{
    if(slot < 0 || slot >= slot_count_ || IsNull(value.color))
        return;
    active_slot_ = slot;
    CommitSlotValue(slot, value.color, value.alpha, true);
    if(WhenSlotChanged)
        WhenSlotChanged(slot);
}

bool UiColorPicker::TryApplyColorText(const String& text, bool final_commit)
{
    ParsedColor_ parsed;
    if(!ParseColorText_(text, parsed))
        return false;
    int alpha = parsed.has_alpha ? parsed.alpha : slots_[active_slot_].alpha;
    CommitSlotValue(active_slot_, parsed.color, alpha, final_commit);
    return true;
}

bool UiColorPicker::TryApplyReadoutText(int readout, const String& text, bool final_commit)
{
    String trimmed = TrimBoth(text);
    String lower = ToLower(trimmed);
    if(trimmed.StartsWith("#") || trimmed.StartsWith("0x") || trimmed.StartsWith("0X") ||
       lower.StartsWith("rgb") || lower.StartsWith("hsv") || lower.StartsWith("hsl") ||
       lower.StartsWith("hls") || lower.StartsWith("cmyk") || lower.StartsWith("tmi"))
        return TryApplyColorText(text, final_commit);

    Vector<ParsedNumber_> number = ExtractNumbers_(text);
    Color color = GetColor();
    int alpha = slots_[active_slot_].alpha;
    if(readout == 1)
        return TryApplyColorText(text, final_commit);

    if(readout == 0 || readout == 2) {
        if(number.GetCount() < 3)
            return false;
        color = readout == 0
              ? HsvToColor_(number[0].value, number[1].value, number[2].value)
              : HslToColor_(number[0].value, number[1].value, number[2].value);
        if(number.GetCount() >= 4)
            alpha = ClampByte_(int(minmax(number[3].value, 0.0, 1.0) * 255.0 + 0.5));
    }
    else if(readout == 3) {
        if(number.GetCount() < 3)
            return false;
        color = Color(ClampByte_(int(minmax(number[0].value, 0.0, 1.0) * 255.0 + 0.5)),
                      ClampByte_(int(minmax(number[1].value, 0.0, 1.0) * 255.0 + 0.5)),
                      ClampByte_(int(minmax(number[2].value, 0.0, 1.0) * 255.0 + 0.5)));
        if(number.GetCount() >= 4)
            alpha = ClampByte_(int(minmax(number[3].value, 0.0, 1.0) * 255.0 + 0.5));
    }
    else if(readout == 4) {
        if(number.GetCount() < 4)
            return false;
        color = CmykToColor_(number[0].value, number[1].value,
                             number[2].value, number[3].value);
        if(number.GetCount() >= 5)
            alpha = ClampByte_(int(minmax(number[4].value, 0.0, 1.0) * 255.0 + 0.5));
    }
    else if(readout == 5) {
        if(number.GetCount() < 3)
            return false;
        color = Color(ClampByte_(int(number[0].value + 0.5)),
                      ClampByte_(int(number[1].value + 0.5)),
                      ClampByte_(int(number[2].value + 0.5)));
        if(number.GetCount() >= 4)
            alpha = ClampByte_(int(number[3].value + 0.5));
    }
    else
        return false;

    CommitSlotValue(active_slot_, color, alpha, final_commit);
    return true;
}

void UiColorPicker::HandleChannelValue(ChannelMode mode, int row, double value, bool final_commit)
{
    if(syncing_controls_)
        return;

    Color color = GetColor();
    int alpha = slots_[active_slot_].alpha;

    switch(mode) {
    case CHANNEL_RGB_FLOAT: {
        double values[4] = {
            color.GetR() / 255.0, color.GetG() / 255.0,
            color.GetB() / 255.0, alpha / 255.0
        };
        if(row >= 0 && row < 4)
            values[row] = value;
        color = Color(ClampByte_(int(values[0] * 255.0 + 0.5)),
                      ClampByte_(int(values[1] * 255.0 + 0.5)),
                      ClampByte_(int(values[2] * 255.0 + 0.5)));
        alpha = ClampByte_(int(values[3] * 255.0 + 0.5));
        break;
    }

    case CHANNEL_RGB_INT: {
        int values[4] = { color.GetR(), color.GetG(), color.GetB(), alpha };
        if(row >= 0 && row < 4)
            values[row] = ClampByte_(int(value + 0.5));
        color = Color(values[0], values[1], values[2]);
        alpha = values[3];
        break;
    }

    case CHANNEL_HSV: {
        int h = 0, s = 0, v = 0;
        ColorToHsv_(color, h, s, v);
        double values[4] = { (double)h, (double)s, (double)v, alpha / 2.55 };
        if(row >= 0 && row < 4)
            values[row] = value;
        color = HsvToColor_(values[0], values[1], values[2]);
        alpha = ClampByte_(int(values[3] * 2.55 + 0.5));
        break;
    }

    case CHANNEL_HSL: {
        int h = 0, s = 0, l = 0;
        ColorToHsl_(color, h, s, l);
        double values[4] = { (double)h, (double)s, (double)l, alpha / 2.55 };
        if(row >= 0 && row < 4)
            values[row] = value;
        color = HslToColor_(values[0], values[1], values[2]);
        alpha = ClampByte_(int(values[3] * 2.55 + 0.5));
        break;
    }

    case CHANNEL_TMI: {
        double t = 0.0, m = 0.0, intensity = 0.0;
        ColorToTmi_(color, t, m, intensity);
        double values[4] = { t, m, intensity, alpha / 2.55 };
        if(row >= 0 && row < 4)
            values[row] = value;
        color = TmiToColor_(values[0], values[1], values[2]);
        alpha = ClampByte_(int(values[3] * 2.55 + 0.5));
        break;
    }

    case CHANNEL_CMYK: {
        int c = 0, m = 0, y = 0, k = 0;
        ColorToCmyk_(color, c, m, y, k);
        double values[5] = { (double)c, (double)m, (double)y, (double)k, alpha / 2.55 };
        if(row >= 0 && row < 5)
            values[row] = value;
        color = CmykToColor_(values[0], values[1], values[2], values[3]);
        alpha = ClampByte_(int(values[4] * 2.55 + 0.5));
        break;
    }

    case CHANNEL_LAB: {
        double l = 0.0, a = 0.0, b = 0.0;
        ColorToLab_(color, l, a, b);
        double values[4] = { l, a, b, alpha / 2.55 };
        if(row >= 0 && row < 4)
            values[row] = value;
        color = LabToColor_(values[0], values[1], values[2]);
        alpha = ClampByte_(int(values[3] * 2.55 + 0.5));
        break;
    }

    default:
        break;
    }

    if(!alpha_enabled_)
        alpha = slots_[active_slot_].alpha;
    CommitSlotValue(active_slot_, color, alpha, final_commit);
}

void UiColorPicker::RefreshChannelModeValues(ChannelMode mode)
{
    if(mode < CHANNEL_RGB_FLOAT || mode >= CHANNEL_COUNT)
        return;

    Color color = GetColor();
    int alpha = slots_[active_slot_].alpha;
    ChannelGroup& group = *channel_group_[mode];

    switch(mode) {
    case CHANNEL_RGB_FLOAT:
        group.SetValue(0, color.GetR() / 255.0);
        group.SetValue(1, color.GetG() / 255.0);
        group.SetValue(2, color.GetB() / 255.0);
        group.SetValue(3, alpha / 255.0);
        break;
    case CHANNEL_RGB_INT:
        group.SetValue(0, color.GetR());
        group.SetValue(1, color.GetG());
        group.SetValue(2, color.GetB());
        group.SetValue(3, alpha);
        break;
    case CHANNEL_HSV: {
        int h = 0, s = 0, v = 0;
        ColorToHsv_(color, h, s, v);
        group.SetValue(0, h);
        group.SetValue(1, s);
        group.SetValue(2, v);
        group.SetValue(3, alpha / 2.55);
        break;
    }
    case CHANNEL_HSL: {
        int h = 0, s = 0, l = 0;
        ColorToHsl_(color, h, s, l);
        group.SetValue(0, h);
        group.SetValue(1, s);
        group.SetValue(2, l);
        group.SetValue(3, alpha / 2.55);
        break;
    }
    case CHANNEL_TMI: {
        double t = 0.0, m = 0.0, i = 0.0;
        ColorToTmi_(color, t, m, i);
        group.SetValue(0, t);
        group.SetValue(1, m);
        group.SetValue(2, i);
        group.SetValue(3, alpha / 2.55);
        break;
    }
    case CHANNEL_CMYK: {
        int c = 0, m = 0, y = 0, k = 0;
        ColorToCmyk_(color, c, m, y, k);
        group.SetValue(0, c);
        group.SetValue(1, m);
        group.SetValue(2, y);
        group.SetValue(3, k);
        group.SetValue(4, alpha / 2.55);
        break;
    }
    case CHANNEL_LAB: {
        double l = 0.0, a = 0.0, b = 0.0;
        ColorToLab_(color, l, a, b);
        group.SetValue(0, l);
        group.SetValue(1, a);
        group.SetValue(2, b);
        group.SetValue(3, alpha / 2.55);
        break;
    }
    default:
        break;
    }
}

void UiColorPicker::UpdateAlphaAvailability()
{
    alpha_toggle_.SetOn(alpha_enabled_);
    for(int i = 0; i < CHANNEL_COUNT; i++)
        channel_group_[i]->EnableAlpha(alpha_enabled_);
    readout_hsv_->Enable();
    readout_hex_->Enable();
    readout_hsl_->Enable();
    readout_rgb_float_->Enable();
    readout_cmyk_->Enable();
    readout_rgb_int_->Enable();
    alpha_toggle_label_.SetText("Alpha");
}

void UiColorPicker::PopulatePaletteSelectors()
{
    const Vector<PaletteDefinition_>& palettes = PaletteRegistry_();
    palette_drop_.Clear();
    for(int i = 0; i < palettes.GetCount(); i++)
        palette_drop_.Add(palettes[i].name, i);
    palette_index_ = minmax(palette_index_, 0, max(0, palettes.GetCount() - 1));
    palette_drop_.SetDataSilently(palette_index_);
}

void UiColorPicker::SetPaletteIndex(int index)
{
    const Vector<PaletteDefinition_>& palettes = PaletteRegistry_();
    if(index < 0 || index >= palettes.GetCount())
        return;
    palette_index_ = index;
    palette_category_ = FindPaletteCategory_(palettes[index].category);
    palette_drop_.SetDataSilently(palette_index_);
    RefreshPaletteGrid();
    SaveSharedSession(false);
}

void UiColorPicker::RefreshPaletteGrid()
{
    const Vector<PaletteDefinition_>& palettes = PaletteRegistry_();
    if(palette_index_ < 0 || palette_index_ >= palettes.GetCount())
        palette_index_ = 0;

    const PaletteDefinition_& palette = palettes[palette_index_];
    palette_grid_->SetGrid(palette.columns, palette.rows);
    palette_grid_->SetItems(palette.colors);
    palette_grid_->SetSelectedIndex(-1);
    palette_badge_.SetText(palette.badge);
    palette_badge_.Tip(Format("%s — %dx%d (%d colours)",
                              palette.name, palette.columns, palette.rows,
                              palette.colors.GetCount()));
    selected_palette_index_ = -1;
    selected_palette_value_ = SlotValue();
}

void UiColorPicker::HandlePalettePick(int index, const SlotValue& value)
{
    if(index < 0 || IsNull(value.color))
        return;
    selected_palette_index_ = index;
    selected_palette_value_ = value;
    palette_grid_->SetSelectedIndex(index);
}

void UiColorPicker::HandleStashPick(int index, const SlotValue& value)
{
    if(index < 0 || IsNull(value.color))
        return;
    selected_stash_index_ = index;
    selected_stash_value_ = value;
    stash_grid_->SetSelectedIndex(index);
}

void UiColorPicker::HandlePaletteDropToStash(const SlotValue& value)
{
    AddUserSwatch(value.color, value.alpha);
}

void UiColorPicker::SaveSelectedPaletteToStash()
{
    if(selected_palette_index_ >= 0 && !IsNull(selected_palette_value_.color))
        AddUserSwatch(selected_palette_value_.color, selected_palette_value_.alpha);
}

void UiColorPicker::UseSelectedPaletteColor()
{
    if(selected_palette_index_ >= 0 && !IsNull(selected_palette_value_.color))
        CommitSlotValue(active_slot_, selected_palette_value_.color,
                        selected_palette_value_.alpha, true);
}

void UiColorPicker::UseSelectedStashColor()
{
    if(selected_stash_index_ >= 0 && !IsNull(selected_stash_value_.color))
        CommitSlotValue(active_slot_, selected_stash_value_.color,
                        selected_stash_value_.alpha, true);
}

void UiColorPicker::SelectCurveChannel(int index)
{
    index = minmax(index, 0, 3);
    selected_curve_channel_ = index;
    curve_stack_.SetActivePage(index);
    children_style_dirty_ = true;
    for(int i = 0; i < 4; i++)
        curve_button_[i].SetChecked(i == index);
    SyncThemeToChildren();
}

void UiColorPicker::CaptureCurveSource()
{
    FinishLiveGesture();
    curve_source_color_ = GetColor();
    footer_information_.SetText(Format("Curve source captured: %s", FormatHex_(curve_source_color_)));
}

void UiColorPicker::ResetCurves()
{
    for(int i = 0; i < 4; i++)
        curve_editor_[i].SetCurve(ShadowLinear());
    curve_source_color_ = GetColor();
    SaveSharedSession(false);
    ApplyCurves(true);
}

void UiColorPicker::ApplyCurves(bool final_commit)
{
    auto evaluate = [&](double input, int channel) {
        double master = UiShadowCurveEval(curve_editor_[0].GetCurve(), ClampUnit_(input));
        return ClampUnit_(UiShadowCurveEval(curve_editor_[channel].GetCurve(), master));
    };

    Color result(ClampByte_(int(evaluate(curve_source_color_.GetR() / 255.0, 1) * 255.0 + 0.5)),
                 ClampByte_(int(evaluate(curve_source_color_.GetG() / 255.0, 2) * 255.0 + 0.5)),
                 ClampByte_(int(evaluate(curve_source_color_.GetB() / 255.0, 3) * 255.0 + 0.5)));
    CommitColor(result, final_commit);
    SaveSharedSession(false);
}

void UiColorPicker::SetGeneratorMode(int mode)
{
    generator_mode_ = minmax(mode, 1, 2);
    for(int i = 0; i < 3; i++)
        generator_mode_button_[i].SetChecked(i == generator_mode_);
    generator_mode_button_[1].SetCustomStyle(UiTheme::ResolveButton(generator_mode_ == 1 ? UiRole::Accent : UiRole::Subtle));
    generator_mode_button_[2].SetCustomStyle(UiTheme::ResolveButton(generator_mode_ == 2 ? UiRole::Accent : UiRole::Subtle));
    if(generator_mode_ == 0)
        generator_base_color_ = GetColor();
    generator_image_preview_->Show(generator_mode_ == 1);
    generator_load_image_button_.Show(generator_mode_ == 1);
    generator_clear_samples_button_.Show(generator_mode_ == 1);
    generator_wheel_->Show(generator_mode_ == 2);
    generator_gain_label_.Show(true);
    generator_gain_slider_.Show(true);
    generator_gain_edit_.Show(true);
    generator_count_label_.Show(true);
    generator_count_slider_.Show(true);
    generator_count_edit_.Show(true);
    RefreshGeneratorPalette();
    RefreshLayout();
}

void UiColorPicker::RefreshGeneratorPalette()
{
    Vector<SlotValue> base_values;
    if(generator_mode_ == 2)
        base_values = generator_wheel_->GetColors();
    else if(generator_mode_ == 1 && !generator_image_.IsEmpty()) {
        Vector<Color> samples = generator_image_preview_->GetSamples();
        for(int i = 0; i < samples.GetCount(); i++)
            base_values.Add(MakeSlot_(samples[i], 255, Format("Image sample %d", i + 1)));
    }
    else {
        Vector<SlotValue> slot_values = GetSlots();
        base_values = BuildHarmonyPalette_(generator_mode_ == 0 ? GetColor() : generator_base_color_,
                                           harmony_mode_, slot_values);
    }

    if(generator_mode_ == 1) {
        int gain = generator_wheel_->GetGlobalGain();
        for(SlotValue& value : base_values) {
            int h = 0, s = 0, v = 0;
            ColorToHsv_(value.color, h, s, v);
            value.color = HsvToColor_(h, s, v * gain / 100.0);
        }
    }

    Vector<SlotValue> values;
    for(const SlotValue& value : base_values)
        values.Add(value);
    generated_base_count_ = base_values.GetCount();
    const int white_mix[] = { 205, 145, 85 };
    const int black_mix[] = { 40, 80, 125, 175 };
    for(int mix : white_mix)
        for(const SlotValue& base : base_values)
            values.Add(MakeSlot_(Blend(base.color, White(), mix), base.alpha, "Tint"));
    for(int mix : black_mix)
        for(const SlotValue& base : base_values)
            values.Add(MakeSlot_(Blend(base.color, Black(), mix), base.alpha, "Shade"));

    generated_swatches_.Clear();
    for(const SlotValue& value : values) {
        SlotData& item = generated_swatches_.Add();
        item.color = value.color;
        item.alpha = value.alpha;
        item.label = value.label;
    }

    int columns = max(1, base_values.GetCount());
    int rows = max(1, (values.GetCount() + columns - 1) / columns);
    generator_grid_->SetGrid(columns, rows);
    generator_grid_->SetFlow(columns, true);
    generator_grid_->SetItems(values);
    generator_grid_->SetSelectedIndex(-1);
    selected_generated_index_ = -1;
    selected_generated_value_ = SlotValue();

    if(generator_mode_ == 1 && generator_image_.IsEmpty())
        generator_hint_.SetText("Load an image to extract its dominant palette.");
    else
        generator_hint_.SetText(Format("%d base colours with tint and shade rows.",
                                       base_values.GetCount()));
    RefreshLayout();
}

void UiColorPicker::HandleGeneratorPick(int index, const SlotValue& value)
{
    if(index < 0 || IsNull(value.color))
        return;
    selected_generated_index_ = index;
    selected_generated_value_ = value;
    generator_grid_->SetSelectedIndex(index);
    if(generator_mode_ == 2) {
        generator_wheel_->SelectHandle(index % max(1, generator_wheel_->GetPaletteCount()));
        int gain = generator_wheel_->GetGlobalGain();
        generator_gain_slider_.SetValue(gain);
        generator_gain_edit_.SetTextUtf8(AsString(gain));
    }
}

void UiColorPicker::LoadGeneratorImage()
{
    FileSel selector;
    selector.Type("Image files", "*.png *.jpg *.jpeg *.bmp");
    if(!selector.ExecuteOpen())
        return;

    Image image = StreamRaster::LoadFileAny(~selector);
    if(image.IsEmpty()) {
        Exclamation("Unable to load the selected image.");
        return;
    }
    SetGeneratorImage(image);
}

void UiColorPicker::SaveGeneratedToStash()
{
    int count = min(generated_base_count_, generated_swatches_.GetCount());
    for(int i = 0; i < count; i++)
        AddUserSwatch(generated_swatches_[i].color, generated_swatches_[i].alpha);
}

void UiColorPicker::UseGeneratedColor()
{
    if(selected_generated_index_ >= 0 && !IsNull(selected_generated_value_.color))
        CommitSlotValue(active_slot_, selected_generated_value_.color,
                        selected_generated_value_.alpha, true);
}

void UiColorPicker::StartEyedropper()
{
    if(eyedropper_active_ || !IsScreenEyedropperAvailable())
        return;
    FinishLiveGesture();
    live_origin_ = slots_[active_slot_];
    live_slot_ = active_slot_;
    eyedropper_active_ = true;
    eyedropper_dragging_ = false;
    SetFocus();
    SetCapture();
    footer_information_.SetText("Eyedropper: drag anywhere, release to accept, Escape to cancel");
    Refresh();
}

void UiColorPicker::FinishEyedropperState(bool commit)
{
    if(finishing_eyedropper_ || (!eyedropper_active_ && !stopping_eyedropper_))
        return;

    finishing_eyedropper_ = true;
    eyedropper_active_ = false;
    eyedropper_dragging_ = false;

    if(commit) {
        if(live_gesture_)
            FinishLiveGesture();
    }
    else {
        if(live_slot_ >= 0 && live_slot_ < slots_.GetCount())
            slots_[live_slot_] = live_origin_;
        live_gesture_ = false;
        live_slot_ = -1;
        SyncAllFromActiveSlot();
    }

    finishing_eyedropper_ = false;
    footer_information_.SetText("Information / Detail");
    Refresh();
}

void UiColorPicker::StopEyedropper(bool commit)
{
    if(stopping_eyedropper_)
        return;

    stopping_eyedropper_ = true;
    FinishEyedropperState(commit);

    if(HasCapture())
        ReleaseCapture();

    stopping_eyedropper_ = false;
}

void UiColorPicker::SampleEyedropper(bool final_commit)
{
    Color sampled;
    if(!ReadScreenColor_(sampled))
        return;
    if(!live_gesture_) {
        live_origin_ = slots_[active_slot_];
        live_slot_ = active_slot_;
        live_gesture_ = true;
    }
    CommitSlotValue(active_slot_, sampled, slots_[active_slot_].alpha, false);
    if(final_commit)
        StopEyedropper(true);
}

void UiColorPicker::SyncThemeToChildren()
{
    SyncThemeStyle();
    uint64 revision = UiTheme::GetRevision();
    if(!children_style_dirty_ && children_theme_revision_ == revision)
        return;

    for(int i = 0; i < PAGE_COUNT; i++)
        page_button_[i].SetCustomStyle(UiTheme::ResolveButton(i == (int)page_mode_ ? UiRole::Accent : UiRole::Subtle));
    for(int i = 0; i < CHANNEL_COUNT; i++)
        channel_button_[i].SetCustomStyle(UiTheme::ResolveButton(i == (int)channel_mode_ ? UiRole::Accent : UiRole::Subtle));
    for(int i = 0; i < 4; i++)
        curve_button_[i].SetCustomStyle(UiTheme::ResolveButton(i == selected_curve_channel_ ? UiRole::Accent : UiRole::Subtle));

    accept_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
    cancel_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Alert));
    palette_use_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
    generator_use_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
    palette_save_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    stash_use_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    stash_save_active_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    curve_capture_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    curve_reset_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    generator_refresh_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    generator_load_image_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    generator_clear_samples_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    generator_save_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    eyedropper_button_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));

    UiToolButton::Style slot_style = UiTheme::ResolveToolButton(UiRole::Subtle);
    slot_style.metrics.content_margin = Rect(0, 0, 0, 0);
    slot_style.metrics.radius = DPI(4);
    current_slot_button_->SetCustomStyle(slot_style);
    for(int i = 0; i < 4; i++) {
        primary_slot_button_[i]->SetCustomStyle(slot_style);
        previous_slot_button_[i]->SetCustomStyle(slot_style);
    }

    children_theme_revision_ = revision;
    children_style_dirty_ = false;
}

void UiColorPicker::Paint(Draw& draw)
{
    const Style& style = GetEffectiveStyle();
    Rect outer(Point(0, 0), GetSize());
    StyledState state = IsEnabled() && IsShowEnabled() ? ST_NORMAL : ST_DISABLED;
    UiPaintStyledBackground(draw, outer, style.palette, style.metrics, style.skin, state, false);
    UiPaintStyledForeground(draw, outer, style.palette, style.metrics, style.skin, state, false);
}

void UiColorPicker::Layout()
{
    const Style& style = GetEffectiveStyle();
    uint64 revision = UiTheme::GetRevision();
    if(children_style_dirty_ || children_theme_revision_ != revision)
        SyncThemeToChildren();

    Rect body = UiStyledInnerRect(GetSize(), style.metrics, style.skin);
    main_root_.SetRect(body);
    main_root_.Layout();
    navigation_bar_.Layout();
    footer_bar_.Layout();
    page_stack_.Layout();

    // Compact current + 4 primary + 4 previous layout from the Designer mockup.
    Rect slots(Point(0, 0), slot_grid_host_.GetSize());
    int gap = max(DPI(2), style.slot_gap);
    int cell_width = max(DPI(18), (slots.GetWidth() - gap * 3) / 4);
    int primary_height = min(DPI(30), max(DPI(26), slots.GetHeight() - DPI(12) - gap));
    int previous_height = max(DPI(9), min(DPI(11), slots.GetHeight() - primary_height - gap));
    for(int i = 0; i < 4; i++) {
        int x = slots.left + i * (cell_width + gap);
        if(i >= slot_count_) {
            primary_slot_button_[i]->SetRect(0, 0, 0, 0);
            previous_slot_button_[i]->SetRect(0, 0, 0, 0);
            continue;
        }
        primary_slot_button_[i]->SetRect(x, slots.top, cell_width, primary_height);
        previous_slot_button_[i]->SetRect(x, slots.top + primary_height + gap, cell_width, previous_height);
    }

    Rect page(Point(0, 0), page_stack_.GetSize());
    if(page.IsEmpty())
        return;

    const int inset = DPI(2);
    const int gap_px = max(DPI(6), style.page_gap);
    Rect content = page.Deflated(inset);

    if(page_mode_ == PAGE_COLOR) {
        int right_width = min(DPI(350), max(DPI(330), content.GetWidth() * 46 / 100));
        int left_width = max(DPI(220), content.GetWidth() - right_width - gap_px);
        if(left_width + right_width + gap_px > content.GetWidth())
            right_width = max(DPI(260), content.GetWidth() - left_width - gap_px);

        Rect left(content.left, content.top, content.left + left_width, content.bottom);
        Rect right(left.right + gap_px, content.top, content.right, content.bottom);

        int top_height = max(DPI(28), style.button_height);
        spectrum_mode_drop_.SetRect(left.left, left.top, left.GetWidth(), top_height);

        bool compact_height = content.GetHeight() < DPI(350);
        int axis_row = compact_height ? DPI(24) : DPI(28);
        int axis_gap = DPI(4);
        int field_top = left.top + top_height + DPI(6);
        color_field_->SetRect(left.left, field_top, left.GetWidth(), max(0, left.bottom - field_top));

        int label_width = DPI(24);
        int edit_width = DPI(64);
        int slider_left = right.left + label_width + DPI(3);
        int slider_width = max(0, right.GetWidth() - label_width - edit_width - DPI(7));

        int selector_height = DPI(25);
        int x = right.left;
        int mode_width = max(DPI(130), right.GetWidth() - DPI(110));
        channel_mode_drop_.SetRect(x, right.top, mode_width, selector_height);
        x += mode_width + DPI(3);
        eyedropper_button_.SetRect(x, right.top, DPI(25), selector_height);
        x += DPI(28);
        alpha_toggle_label_.SetRect(x, right.top, DPI(30), selector_height);
        x += DPI(30);
        alpha_toggle_.SetRect(x, right.top, max(DPI(38), right.right - x), selector_height);

        int readout_gap = DPI(4);
        int readout_height = compact_height ? DPI(36) : DPI(44);
        int readout_total = readout_height * 3 + readout_gap * 2;
        int axes_height = axis_row * 2 + axis_gap;
        int channel_top = right.top + selector_height + DPI(3);
        int channel_rows = channel_group_[channel_mode_]->GetRowCount();
        int desired_channel_height = channel_rows * (compact_height ? DPI(20) : DPI(25));
        int available_channel_height = right.bottom - channel_top - axes_height - readout_total - DPI(6);
        int channel_height = min(desired_channel_height,
                                 max(channel_rows * DPI(16), available_channel_height));
        channel_stack_.SetRect(right.left, channel_top, right.GetWidth(), channel_height);
        channel_stack_.Layout();

        int axes_top = channel_top + channel_height + DPI(2);
        int readout_top = axes_top + axes_height + DPI(3);
        int y = axes_top;
        hue_axis_label_.SetRect(right.left, y, label_width, axis_row);
        hue_axis_slider_.SetRect(slider_left, y, slider_width, axis_row);
        hue_axis_edit_.SetRect(right.right - edit_width, y + DPI(2), edit_width, axis_row - DPI(4));
        y += axis_row + axis_gap;
        gain_axis_label_.SetRect(right.left, y, label_width, axis_row);
        gain_axis_slider_.SetRect(slider_left, y, slider_width, axis_row);
        gain_axis_edit_.SetRect(right.right - edit_width, y + DPI(2), edit_width, axis_row - DPI(4));

        int readout_width = max(1, (right.GetWidth() - readout_gap) / 2);
        ReadoutRow *readout[6] = {
            ~readout_hsv_, ~readout_hex_, ~readout_hsl_,
            ~readout_rgb_float_, ~readout_cmyk_, ~readout_rgb_int_
        };
        for(int i = 0; i < 6; i++) {
            int row = i / 2;
            int column = i % 2;
            readout[i]->SetRect(right.left + column * (readout_width + readout_gap),
                                readout_top + row * (readout_height + readout_gap),
                                column == 0 ? readout_width : right.GetWidth() - readout_width - readout_gap,
                                readout_height);
        }
    }
    else if(page_mode_ == PAGE_PALETTES) {
        int selector_top = content.top;
        int selector_height = DPI(28);
        int badge_width = DPI(90);
        palette_drop_.SetRect(content.left, selector_top,
                              max(DPI(140), content.GetWidth() - badge_width - DPI(5)),
                              selector_height);
        palette_badge_.SetRect(content.right - badge_width, selector_top, badge_width, selector_height);

        int stash_height = min(DPI(110), max(DPI(76), content.GetHeight() / 3));
        int stash_labels_height = DPI(36);
        int grid_top = selector_top + selector_height + DPI(6);
        int grid_bottom = content.bottom - stash_height - stash_labels_height - DPI(6);
        palette_grid_->SetRect(content.left, grid_top, content.GetWidth(), max(DPI(80), grid_bottom - grid_top));

        int stash_label_top = max(grid_top + DPI(80), grid_bottom + DPI(5));
        stash_title_.SetRect(content.left, stash_label_top, DPI(120), DPI(18));
        palette_hint_.SetRect(content.left + DPI(125), stash_label_top,
                              max(0, content.GetWidth() - DPI(125)), DPI(18));
        int stash_top = stash_label_top + DPI(20);
        stash_grid_->SetRect(content.left, stash_top, content.GetWidth(), stash_height);

    }
    else if(page_mode_ == PAGE_CURVES) {
        int top_height = DPI(30);
        int gap = DPI(5);
        int button_width = max(DPI(76), (content.GetWidth() - DPI(230) - gap * 5) / 4);
        int x = content.left;
        for(int i = 0; i < 4; i++) {
            curve_button_[i].SetRect(x, content.top, button_width, top_height);
            x += button_width + gap;
        }
        curve_capture_button_.SetRect(x, content.top, DPI(120), top_height);
        x += DPI(125);
        curve_reset_button_.SetRect(x, content.top, max(DPI(80), content.right - x), top_height);

        int hint_height = DPI(24);
        curve_stack_.SetRect(content.left, content.top + top_height + DPI(6),
                             content.GetWidth(),
                             max(DPI(120), content.GetHeight() - top_height - hint_height - DPI(12)));
        curve_stack_.Layout();
        curve_hint_.SetRect(content.left, content.bottom - hint_height,
                            content.GetWidth(), hint_height);
    }
    else {
        int top_height = DPI(28);
        int gap = DPI(5);
        int x = content.left;
        generator_mode_button_[0].SetRect(0, 0, 0, 0);
        generator_mode_button_[1].SetRect(x, content.top, DPI(72), top_height);
        x += DPI(77);
        generator_mode_button_[2].SetRect(x, content.top, DPI(105), top_height);
        x += DPI(112);
        generator_gain_label_.SetRect(x, content.top, DPI(34), top_height);
        x += DPI(34);
        generator_gain_slider_.SetRect(x, content.top, DPI(92), top_height);
        x += DPI(95);
        generator_gain_edit_.SetRect(x, content.top + DPI(2), DPI(38), top_height - DPI(4));
        x += DPI(44);
        generator_count_label_.SetRect(x, content.top, DPI(46), top_height);
        x += DPI(46);
        generator_count_slider_.SetRect(x, content.top, DPI(72), top_height);
        x += DPI(75);
        generator_count_edit_.SetRect(x, content.top + DPI(2), DPI(34), top_height - DPI(4));

        int controls_top = content.top + top_height + gap;
        int harmony_width = min(DPI(220), max(DPI(160), content.GetWidth() / 3));
        harmony_drop_.SetRect(content.left, controls_top, harmony_width, top_height);
        generator_refresh_button_.SetRect(0, 0, 0, 0);
        generator_save_button_.SetRect(content.left + harmony_width + gap, controls_top, DPI(96), top_height);
        generator_load_image_button_.SetRect(content.left + harmony_width + DPI(106), controls_top, DPI(92), top_height);
        generator_clear_samples_button_.SetRect(content.left + harmony_width + DPI(203), controls_top, DPI(92), top_height);
        generator_use_button_.SetRect(0, 0, 0, 0);

        int hint_height = DPI(24);
        int body_top = controls_top + top_height + DPI(6);
        int body_bottom = content.bottom - hint_height - DPI(4);
        int matrix_width = generator_wheel_->GetPaletteCount() * DPI(34);
        int preview_width = max(DPI(220), content.GetWidth() - gap - matrix_width);
        preview_width = min(preview_width, max(DPI(220), content.GetWidth() * 65 / 100));
        generator_image_preview_->SetRect(content.left, body_top, preview_width,
                                           max(DPI(120), body_bottom - body_top));
        generator_wheel_->SetRect(content.left, body_top, preview_width,
                                  max(DPI(100), body_bottom - body_top));
        generator_grid_->SetRect(content.left + preview_width + gap, body_top,
                                 max(DPI(120), content.GetWidth() - preview_width - gap),
                                 max(DPI(120), body_bottom - body_top));
        generator_hint_.SetRect(content.left, content.bottom - hint_height,
                                content.GetWidth(), hint_height);
    }
}

Size UiColorPicker::GetMinSize() const
{
    return Size(DPI(620), DPI(380));
}

void UiColorPicker::SetData(const Value& value)
{
    if(value.Is<Color>()) {
        SetColor((Color)value, true);
        return;
    }

    if(value.Is<ValueArray>()) {
        ValueArray array = value;
        SetSlotCount(min(4, array.GetCount()));
        for(int i = 0; i < array.GetCount() && i < 4; i++)
            if(array[i].Is<Color>())
                SetSlotColor(i, (Color)array[i], false);
        SyncAllFromActiveSlot();
        SaveSharedSession(true);
    }
}

Value UiColorPicker::GetData() const
{
    if(slot_count_ == 1)
        return slots_[0].color;

    ValueArray array;
    for(int i = 0; i < slot_count_; i++)
        array.Add(slots_[i].color);
    return array;
}

void UiColorPicker::LeftDown(Point, dword)
{
    if(!eyedropper_active_)
        return;
    eyedropper_dragging_ = true;
    SampleEyedropper(false);
}

void UiColorPicker::LeftUp(Point, dword)
{
    if(!eyedropper_active_ || !eyedropper_dragging_)
        return;
    SampleEyedropper(true);
}

void UiColorPicker::MouseMove(Point, dword)
{
    if(eyedropper_active_)
        SampleEyedropper(false);
}

bool UiColorPicker::Key(dword key, int count)
{
    if(eyedropper_active_ && key == K_ESCAPE) {
        StopEyedropper(false);
        return true;
    }
    return Ctrl::Key(key, count);
}

Image UiColorPicker::CursorImage(Point point, dword flags)
{
    if(eyedropper_active_)
        return Image::Cross();
    return Ctrl::CursorImage(point, flags);
}

void UiColorPicker::CancelMode()
{
    if(eyedropper_active_)
        FinishEyedropperState(false);
    Ctrl::CancelMode();
}

} // namespace Upp
