#include <Ui/UiColorPicker/UiColorPickerPaletteLab.h>

#include <cmath>
#include <cstdlib>
#include <cfloat>

namespace Upp {
namespace UiColorPickerPaletteLab {

namespace {

int ClampByte(int value) { return minmax(value, 0, 255); }
int ClampPercent(int value) { return minmax(value, 0, 100); }
double ClampUnit(double value) { return minmax(value, 0.0, 1.0); }

struct ParsedNumber : Moveable<ParsedNumber> {
    double value = 0.0;
    bool percent = false;
};

struct LabValue : Moveable<LabValue> {
    double l = 0.0;
    double a = 0.0;
    double b = 0.0;
};

LabValue ToLab(Color color)
{
    LabValue out;
    ColorToLab(color, out.l, out.a, out.b);
    return out;
}

double LabDistanceSquared(const LabValue& a, const LabValue& b)
{
    double dl = a.l - b.l;
    double da = a.a - b.a;
    double db = a.b - b.b;
    return dl * dl + da * da + db * db;
}

double LabDistance(const LabValue& a, const LabValue& b)
{
    return sqrt(LabDistanceSquared(a, b));
}

bool IsHexDigit(int character)
{
    return (character >= '0' && character <= '9')
        || (character >= 'a' && character <= 'f')
        || (character >= 'A' && character <= 'F');
}

int HexValue(int character)
{
    if(character >= '0' && character <= '9')
        return character - '0';
    if(character >= 'a' && character <= 'f')
        return character - 'a' + 10;
    return character - 'A' + 10;
}

Vector<ParsedNumber> ExtractNumbers(const String& text)
{
    Vector<ParsedNumber> values;
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
        ParsedNumber& parsed = values.Add();
        parsed.value = value;
        parsed.percent = *end == '%';
        if(parsed.percent)
            end++;
        cursor = end;
    }
    return values;
}

double PercentOrUnit(const ParsedNumber& value)
{
    if(value.percent)
        return value.value;
    if(fabs(value.value) <= 1.0)
        return value.value * 100.0;
    return value.value;
}

int ParseAlpha(const ParsedNumber& value)
{
    if(value.percent)
        return ClampByte(int(value.value * 2.55 + 0.5));
    if(value.value >= 0.0 && value.value <= 1.0)
        return ClampByte(int(value.value * 255.0 + 0.5));
    return ClampByte(int(value.value + 0.5));
}

bool ParseHex(String text, Color& color, int& alpha, bool& has_alpha)
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
        if(!IsHexDigit((byte)text[i]))
            return false;

    int r, g, b;
    alpha = 255;
    has_alpha = false;
    if(count == 3 || count == 4) {
        r = HexValue(text[0]) * 17;
        g = HexValue(text[1]) * 17;
        b = HexValue(text[2]) * 17;
        if(count == 4) {
            alpha = HexValue(text[3]) * 17;
            has_alpha = true;
        }
    }
    else {
        r = HexValue(text[0]) * 16 + HexValue(text[1]);
        g = HexValue(text[2]) * 16 + HexValue(text[3]);
        b = HexValue(text[4]) * 16 + HexValue(text[5]);
        if(count == 8) {
            alpha = HexValue(text[6]) * 16 + HexValue(text[7]);
            has_alpha = true;
        }
    }
    color = Color(r, g, b);
    return true;
}

UiColorPicker::SlotValue Slot(Color color, int alpha = 255, const String& label = String())
{
    UiColorPicker::SlotValue value;
    value.color = color;
    value.alpha = ClampByte(alpha);
    value.label = label;
    return value;
}

struct MediumProfile {
    int minimum_value = 8;
    int maximum_value = 98;
    double saturation_limit = 1.0;
    double saturation_character = 1.0;
    int hero_value = 76;
};

MediumProfile Profile(UiColorPicker::MediumMode medium)
{
    MediumProfile p;
    switch(medium) {
    case UiColorPicker::MEDIUM_UI:
        p.minimum_value = 16; p.maximum_value = 96; p.saturation_limit = 0.90;
        p.saturation_character = 0.92; p.hero_value = 74;
        break;
    case UiColorPicker::MEDIUM_WEB:
        p.minimum_value = 8; p.maximum_value = 98; p.saturation_limit = 1.0;
        p.saturation_character = 1.0; p.hero_value = 78;
        break;
    case UiColorPicker::MEDIUM_PRINT:
        p.minimum_value = 14; p.maximum_value = 92; p.saturation_limit = 0.82;
        p.saturation_character = 0.88; p.hero_value = 70;
        break;
    case UiColorPicker::MEDIUM_PAINTING:
        p.minimum_value = 6; p.maximum_value = 96; p.saturation_limit = 0.94;
        p.saturation_character = 0.84; p.hero_value = 72;
        break;
    case UiColorPicker::MEDIUM_IMAGE_VFX:
    default:
        p.minimum_value = 4; p.maximum_value = 100; p.saturation_limit = 0.97;
        p.saturation_character = 0.96; p.hero_value = 76;
        break;
    }
    return p;
}

Vector<double> DistributionWeights(UiColorPicker::DistributionMode mode, int count)
{
    Vector<double> weight;
    weight.SetCount(max(0, count), 1.0);
    if(count <= 0)
        return weight;

    switch(mode) {
    case UiColorPicker::DISTRIBUTION_DOMINANT:
        if(count == 1)
            weight[0] = 1.0;
        else if(count == 2) {
            weight[0] = 0.70; weight[1] = 0.30;
        }
        else {
            weight[0] = 0.60; weight[1] = 0.30;
            for(int i = 2; i < count; i++)
                weight[i] = 0.10 / (count - 2);
        }
        break;
    case UiColorPicker::DISTRIBUTION_ACCENT_POP:
        if(count == 1)
            weight[0] = 1.0;
        else if(count == 2) {
            weight[0] = 0.72; weight[1] = 0.28;
        }
        else {
            weight[0] = 0.62;
            weight[count - 1] = 0.23;
            for(int i = 1; i < count - 1; i++)
                weight[i] = 0.15 / (count - 2);
        }
        break;
    case UiColorPicker::DISTRIBUTION_TONAL_RAMP:
        weight[0] = 2.5;
        for(int i = 1; i < count; i++)
            weight[i] = 0.75;
        break;
    case UiColorPicker::DISTRIBUTION_FREE_FORM:
        for(int i = 0; i < count; i++)
            weight[i] = max(1, count - i);
        break;
    case UiColorPicker::DISTRIBUTION_BALANCED:
    default:
        break;
    }
    return weight;
}

int ToneOffset(int index, int allocation, UiColorPicker::DistributionMode mode)
{
    if(index == 0)
        return 0;
    static const int balanced[] = { 0, 18, -18, 31, -31, 10, -10, 40, -40, 24, -24, 5 };
    static const int dominant[] = { 0, 12, -12, 25, -25, 36, -36, 6, -6, 18, -18, 42 };
    static const int accent[] = { 0, 8, -8, 18, -18, 28, -28, 38, -38, 13, -13, 44 };
    static const int ramp[] = { 0, 15, 30, 45, -15, -30, -45, 8, 23, 38, -23, -38 };
    const int *table = balanced;
    if(mode == UiColorPicker::DISTRIBUTION_DOMINANT)
        table = dominant;
    else if(mode == UiColorPicker::DISTRIBUTION_ACCENT_POP)
        table = accent;
    else if(mode == UiColorPicker::DISTRIBUTION_TONAL_RAMP)
        table = ramp;
    int offset = table[min(index, 11)];
    if(allocation <= 2 && index == 1)
        offset = mode == UiColorPicker::DISTRIBUTION_TONAL_RAMP ? 24 : 18;
    return offset;
}

String FamilyRole(int index, int count)
{
    if(index == 0)
        return "Primary";
    if(count > 2 && index == count - 1)
        return "Accent";
    if(index == 1)
        return "Secondary";
    return Format("Family %d", index + 1);
}

void AddPaletteColor(StaticPaletteDefinition& palette, Color color, const String& label = String())
{
    palette.swatches.Add(Slot(color, 255, label));
}

StaticPaletteDefinition MakeHuePalette(const String& id, const String& name,
                                       int rows, int saturation, int value,
                                       int saturation_step, int value_step)
{
    StaticPaletteDefinition p;
    p.id = id; p.name = name; p.category = "Creative"; p.preferred_columns = 12;
    for(int row = 0; row < rows; row++)
        for(int column = 0; column < 12; column++)
            AddPaletteColor(p, HsvToColor(column * 30,
                                          max(0, saturation - row * saturation_step),
                                          max(1, value - row * value_step)));
    return p;
}

struct ImageBin : Moveable<ImageBin> {
    int key = 0;
    int count = 0;
    int64 sum_r = 0;
    int64 sum_g = 0;
    int64 sum_b = 0;
    int64 sum_x = 0;
    int64 sum_y = 0;
    Color color = Black();
    LabValue lab;
    double weight = 0.0;
};

Color ProxyPixel(const Image& image, int x, int y)
{
    Size size = image.GetSize();
    if(size.IsEmpty())
        return Black();
    x = minmax(x, 0, size.cx - 1);
    y = minmax(y, 0, size.cy - 1);
    const RGBA& pixel = image[y][x];
    return Color(pixel.r, pixel.g, pixel.b);
}

Color RobustSeedColor(const Image& proxy, Point point)
{
    Color centre = ProxyPixel(proxy, point.x, point.y);
    LabValue centre_lab = ToLab(centre);
    Vector<int> red, green, blue;
    for(int dy = -3; dy <= 3; dy++)
        for(int dx = -3; dx <= 3; dx++) {
            Color sample = ProxyPixel(proxy, point.x + dx, point.y + dy);
            if(LabDistance(centre_lab, ToLab(sample)) <= 10.0) {
                red.Add(sample.GetR());
                green.Add(sample.GetG());
                blue.Add(sample.GetB());
            }
        }
    if(red.IsEmpty())
        return centre;
    Sort(red); Sort(green); Sort(blue);
    int m = red.GetCount() / 2;
    return Color(red[m], green[m], blue[m]);
}

void FloodSeed(const Image& proxy, const ImageExclusionSeed& seed, int tolerance,
               Vector<byte>& output)
{
    if(!seed.enabled || !seed.placed || proxy.IsEmpty())
        return;
    Size size = proxy.GetSize();
    int64 length64 = int64(size.cx) * size.cy;
    if(length64 <= 0 || length64 > INT_MAX)
        return;
    int length = (int)length64;
    Vector<byte> visited;
    visited.SetCount(length, 0);
    Point start(minmax(int(seed.position.x * size.cx), 0, size.cx - 1),
                minmax(int(seed.position.y * size.cy), 0, size.cy - 1));
    Color seed_color = IsNull(seed.color) ? RobustSeedColor(proxy, start) : seed.color;
    LabValue seed_lab = ToLab(seed_color);
    double threshold = max(1, tolerance) * 1.65;
    double local_threshold = threshold * 0.70 + 4.0;

    Vector<int> queue;
    queue.Reserve(length / 8 + 1);
    int start_index = start.y * size.cx + start.x;
    queue.Add(start_index);
    visited[start_index] = 1;
    for(int head = 0; head < queue.GetCount(); head++) {
        int index = queue[head];
        int x = index % size.cx;
        int y = index / size.cx;
        LabValue current = ToLab(ProxyPixel(proxy, x, y));
        if(LabDistance(current, seed_lab) > threshold)
            continue;
        output[index] = 1;
        for(int dy = -1; dy <= 1; dy++)
            for(int dx = -1; dx <= 1; dx++) {
                if(dx == 0 && dy == 0)
                    continue;
                int nx = x + dx, ny = y + dy;
                if(nx < 0 || nx >= size.cx || ny < 0 || ny >= size.cy)
                    continue;
                int next = ny * size.cx + nx;
                if(visited[next])
                    continue;
                visited[next] = 1;
                LabValue neighbour = ToLab(ProxyPixel(proxy, nx, ny));
                if(LabDistance(neighbour, seed_lab) <= threshold &&
                   LabDistance(neighbour, current) <= local_threshold)
                    queue.Add(next);
            }
    }
}

Vector<byte> BuildExclusionMask(const Image& proxy, const ImageAnalysisSettings& settings,
                                double& ignored_fraction)
{
    Size size = proxy.GetSize();
    int64 length64 = int64(size.cx) * size.cy;
    Vector<byte> mask;
    if(length64 <= 0 || length64 > INT_MAX)
        return mask;
    mask.SetCount((int)length64, 0);
    FloodSeed(proxy, settings.exclusion[0], settings.tolerance, mask);
    FloodSeed(proxy, settings.exclusion[1], settings.tolerance, mask);
    int ignored = 0;
    for(byte value : mask)
        ignored += value != 0;
    ignored_fraction = mask.IsEmpty() ? 0.0 : ignored / double(mask.GetCount());
    return mask;
}

Vector<GeneratedSwatch> ManualPointPalette(const Image& proxy, int count,
                                           Vector<Pointf>& positions)
{
    Vector<GeneratedSwatch> out;
    if(proxy.IsEmpty())
        return out;
    Size size = proxy.GetSize();
    int columns = count <= 6 ? count : 6;
    int rows = (count + columns - 1) / columns;
    for(int i = 0; i < count; i++) {
        int column = i % columns;
        int row = i / columns;
        Pointf p((column + 0.5) / columns, (row + 0.5) / rows);
        positions.Add(p);
        int cx = minmax(int(p.x * size.cx), 0, size.cx - 1);
        int cy = minmax(int(p.y * size.cy), 0, size.cy - 1);
        int64 sr = 0, sg = 0, sb = 0;
        int samples = 0;
        int radius = max(2, min(size.cx, size.cy) / 80);
        for(int dy = -radius; dy <= radius; dy++)
            for(int dx = -radius; dx <= radius; dx++) {
                if(dx * dx + dy * dy > radius * radius)
                    continue;
                Color c = ProxyPixel(proxy, cx + dx, cy + dy);
                sr += c.GetR(); sg += c.GetG(); sb += c.GetB(); samples++;
            }
        Color c(samples ? int(sr / samples) : 0,
                samples ? int(sg / samples) : 0,
                samples ? int(sb / samples) : 0);
        GeneratedSwatch& swatch = out.Add();
        swatch.value = Slot(c, 255, Format("Manual %d", i + 1));
        swatch.source_index = i;
        swatch.hero = i == 0;
    }
    return out;
}

} // namespace

int NormalizeHue(int hue)
{
    hue %= 360;
    if(hue < 0)
        hue += 360;
    return hue;
}

void ColorToHsv(Color color, int& hue, int& saturation, int& value)
{
    double h = 0.0, s = 0.0, v = 0.0;
    RGBtoHSV(color.GetR() / 255.0, color.GetG() / 255.0, color.GetB() / 255.0, h, s, v);
    hue = minmax(int(h * 360.0 + 0.5), 0, 359);
    saturation = ClampPercent(int(s * 100.0 + 0.5));
    value = ClampPercent(int(v * 100.0 + 0.5));
}

Color HsvToColor(double hue, double saturation, double value)
{
    hue = NormalizeHue(int(floor(hue + 0.5)));
    saturation = minmax(saturation, 0.0, 100.0);
    value = minmax(value, 0.0, 100.0);
    double r = 0.0, g = 0.0, b = 0.0;
    HSVtoRGB(hue / 360.0, saturation / 100.0, value / 100.0, r, g, b);
    return Color(ClampByte(int(r * 255.0 + 0.5)),
                 ClampByte(int(g * 255.0 + 0.5)),
                 ClampByte(int(b * 255.0 + 0.5)));
}

void ColorToHsl(Color color, int& hue, int& saturation, int& lightness)
{
    double r = color.GetR() / 255.0, g = color.GetG() / 255.0, b = color.GetB() / 255.0;
    double maximum = max(r, max(g, b));
    double minimum = min(r, min(g, b));
    double delta = maximum - minimum;
    double h = 0.0;
    double l = (maximum + minimum) * 0.5;
    double s = 0.0;
    if(delta > 1e-12) {
        double divisor = 1.0 - fabs(2.0 * l - 1.0);
        s = divisor > 1e-12 ? delta / divisor : 0.0;
        if(maximum == r)
            h = 60.0 * fmod((g - b) / delta, 6.0);
        else if(maximum == g)
            h = 60.0 * ((b - r) / delta + 2.0);
        else
            h = 60.0 * ((r - g) / delta + 4.0);
        if(h < 0.0)
            h += 360.0;
    }
    hue = minmax(int(h + 0.5), 0, 359);
    saturation = ClampPercent(int(s * 100.0 + 0.5));
    lightness = ClampPercent(int(l * 100.0 + 0.5));
}

Color HslToColor(double hue, double saturation, double lightness)
{
    hue = NormalizeHue(int(floor(hue + 0.5))) / 360.0;
    saturation = minmax(saturation / 100.0, 0.0, 1.0);
    lightness = minmax(lightness / 100.0, 0.0, 1.0);
    auto hue_to_rgb = [](double p, double q, double t) {
        if(t < 0.0) t += 1.0;
        if(t > 1.0) t -= 1.0;
        if(t < 1.0 / 6.0) return p + (q - p) * 6.0 * t;
        if(t < 1.0 / 2.0) return q;
        if(t < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
        return p;
    };
    double r = lightness, g = lightness, b = lightness;
    if(saturation > 1e-12) {
        double q = lightness < 0.5 ? lightness * (1.0 + saturation)
                                   : lightness + saturation - lightness * saturation;
        double p = 2.0 * lightness - q;
        r = hue_to_rgb(p, q, hue + 1.0 / 3.0);
        g = hue_to_rgb(p, q, hue);
        b = hue_to_rgb(p, q, hue - 1.0 / 3.0);
    }
    return Color(ClampByte(int(r * 255.0 + 0.5)),
                 ClampByte(int(g * 255.0 + 0.5)),
                 ClampByte(int(b * 255.0 + 0.5)));
}

void ColorToCmyk(Color color, int& cyan, int& magenta, int& yellow, int& black)
{
    double c = 0.0, m = 0.0, y = 0.0, k = 0.0;
    RGBtoCMYK(color.GetR() / 255.0, color.GetG() / 255.0, color.GetB() / 255.0, c, m, y, k);
    cyan = ClampPercent(int(c * 100.0 + 0.5));
    magenta = ClampPercent(int(m * 100.0 + 0.5));
    yellow = ClampPercent(int(y * 100.0 + 0.5));
    black = ClampPercent(int(k * 100.0 + 0.5));
}

Color CmykToColor(double cyan, double magenta, double yellow, double black)
{
    double r = 0.0, g = 0.0, b = 0.0;
    CMYKtoRGB(minmax(cyan / 100.0, 0.0, 1.0), minmax(magenta / 100.0, 0.0, 1.0),
              minmax(yellow / 100.0, 0.0, 1.0), minmax(black / 100.0, 0.0, 1.0), r, g, b);
    return Color(ClampByte(int(r * 255.0 + 0.5)),
                 ClampByte(int(g * 255.0 + 0.5)),
                 ClampByte(int(b * 255.0 + 0.5)));
}

void ColorToLab(Color color, double& l, double& a, double& b)
{
    auto linear = [](double value) {
        return value <= 0.04045 ? value / 12.92 : pow((value + 0.055) / 1.055, 2.4);
    };
    double r = linear(color.GetR() / 255.0);
    double g = linear(color.GetG() / 255.0);
    double blue = linear(color.GetB() / 255.0);
    double x = (r * 0.4124564 + g * 0.3575761 + blue * 0.1804375) / 0.95047;
    double y = r * 0.2126729 + g * 0.7151522 + blue * 0.0721750;
    double z = (r * 0.0193339 + g * 0.1191920 + blue * 0.9503041) / 1.08883;
    auto pivot = [](double value) {
        return value > 0.008856 ? pow(value, 1.0 / 3.0) : 7.787 * value + 16.0 / 116.0;
    };
    double fx = pivot(x), fy = pivot(y), fz = pivot(z);
    l = 116.0 * fy - 16.0;
    a = 500.0 * (fx - fy);
    b = 200.0 * (fy - fz);
}

Color LabToColor(double l, double a, double b)
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
            if(in_gamut(r) && in_gamut(g) && in_gamut(blue)) low = scale;
            else high = scale;
        }
        linear_rgb(low, r, g, blue);
    }
    auto encode = [](double value) {
        value = value <= 0.0031308 ? 12.92 * value
                                  : 1.055 * pow(max(0.0, value), 1.0 / 2.4) - 0.055;
        return ClampByte(int(minmax(value, 0.0, 1.0) * 255.0 + 0.5));
    };
    return Color(encode(r), encode(g), encode(blue));
}

void ColorToTmi(Color color, double& temperature, double& magenta, double& intensity)
{
    double r = color.GetR() / 255.0, g = color.GetG() / 255.0, b = color.GetB() / 255.0;
    intensity = minmax((r + g + b) / 3.0 * 100.0, 0.0, 100.0);
    temperature = minmax((r - b) * 100.0, -100.0, 100.0);
    magenta = minmax((((r + b) * 0.5) - g) * 100.0, -100.0, 100.0);
}

Color TmiToColor(double temperature, double magenta, double intensity)
{
    double base = minmax(intensity, 0.0, 100.0) / 100.0;
    double t = minmax(temperature, -100.0, 100.0) / 100.0;
    double m = minmax(magenta, -100.0, 100.0) / 100.0;
    double r = base + t * 0.35 + m * 0.24;
    double b = base - t * 0.35 + m * 0.24;
    double g = base - m * 0.48;
    double maximum = max(r, max(g, b));
    double minimum = min(r, min(g, b));
    if(maximum > 1.0) { r /= maximum; g /= maximum; b /= maximum; }
    if(minimum < 0.0) {
        r -= minimum; g -= minimum; b -= minimum;
        maximum = max(r, max(g, b));
        if(maximum > 1.0) { r /= maximum; g /= maximum; b /= maximum; }
    }
    return Color(ClampByte(int(ClampUnit(r) * 255.0 + 0.5)),
                 ClampByte(int(ClampUnit(g) * 255.0 + 0.5)),
                 ClampByte(int(ClampUnit(b) * 255.0 + 0.5)));
}

String FormatHex(Color color)
{
    return Format("#%02X%02X%02X", color.GetR(), color.GetG(), color.GetB());
}

String FormatHex8(Color color, int alpha)
{
    return Format("#%02X%02X%02X%02X", color.GetR(), color.GetG(), color.GetB(), ClampByte(alpha));
}

bool ParseColorText(const String& source, Color& color, int& alpha, bool& has_alpha)
{
    String text = TrimBoth(source);
    if(text.IsEmpty())
        return false;
    has_alpha = false;
    alpha = 255;
    if(ParseHex(text, color, alpha, has_alpha))
        return true;
    if(text.GetCount() == 6 || text.GetCount() == 8) {
        bool all_hex = true;
        for(int i = 0; i < text.GetCount(); i++)
            all_hex = all_hex && IsHexDigit((byte)text[i]);
        if(all_hex) {
            String candidate = "#" + text;
            if(ParseHex(candidate, color, alpha, has_alpha))
                return true;
        }
    }
    String lower = ToLower(text);
    Vector<ParsedNumber> number = ExtractNumbers(text);
    bool rgb = lower.StartsWith("rgb");
    bool hsv = lower.StartsWith("hsv") || lower.StartsWith("hsb");
    bool hsl = lower.StartsWith("hsl") || lower.StartsWith("hls");
    bool cmyk = lower.StartsWith("cmyk");
    bool tmi = lower.StartsWith("tmi");
    if(rgb || (!hsv && !hsl && !cmyk && !tmi && (number.GetCount() == 3 || number.GetCount() == 4))) {
        if(number.GetCount() < 3 || number.GetCount() > 4)
            return false;
        bool unit = !number[0].percent && !number[1].percent && !number[2].percent
                 && fabs(number[0].value) <= 1.0 && fabs(number[1].value) <= 1.0
                 && fabs(number[2].value) <= 1.0;
        auto component = [&](const ParsedNumber& n) {
            if(n.percent) return int(n.value * 2.55 + 0.5);
            return unit ? int(n.value * 255.0 + 0.5) : int(n.value + 0.5);
        };
        color = Color(ClampByte(component(number[0])), ClampByte(component(number[1])),
                      ClampByte(component(number[2])));
        if(number.GetCount() == 4) { alpha = ParseAlpha(number[3]); has_alpha = true; }
        return true;
    }
    if(hsv) {
        if(number.GetCount() < 3 || number.GetCount() > 4) return false;
        color = HsvToColor(number[0].value, PercentOrUnit(number[1]), PercentOrUnit(number[2]));
    }
    else if(hsl) {
        if(number.GetCount() < 3 || number.GetCount() > 4) return false;
        color = HslToColor(number[0].value, PercentOrUnit(number[1]), PercentOrUnit(number[2]));
    }
    else if(cmyk) {
        if(number.GetCount() < 4 || number.GetCount() > 5) return false;
        color = CmykToColor(PercentOrUnit(number[0]), PercentOrUnit(number[1]),
                            PercentOrUnit(number[2]), PercentOrUnit(number[3]));
        if(number.GetCount() == 5) { alpha = ParseAlpha(number[4]); has_alpha = true; }
        return true;
    }
    else if(tmi) {
        if(number.GetCount() < 3 || number.GetCount() > 4) return false;
        color = TmiToColor(number[0].value, number[1].value, PercentOrUnit(number[2]));
    }
    else
        return false;
    if(number.GetCount() == 4) { alpha = ParseAlpha(number[3]); has_alpha = true; }
    return true;
}

Vector<int> HarmonyOffsets(UiColorPicker::HarmonyMode harmony)
{
    Vector<int> out;
    switch(harmony) {
    case UiColorPicker::HARMONY_ANALOGOUS:
        out << -30 << 0 << 30;
        break;
    case UiColorPicker::HARMONY_COMPLEMENTARY:
        out << 0 << 180;
        break;
    case UiColorPicker::HARMONY_SPLIT_COMPLEMENTARY:
        out << 0 << 150 << 210;
        break;
    case UiColorPicker::HARMONY_TRIAD:
        out << 0 << 120 << 240;
        break;
    case UiColorPicker::HARMONY_SQUARE:
        out << 0 << 90 << 180 << 270;
        break;
    case UiColorPicker::HARMONY_COMPOUND:
        out << 0 << 30 << 180 << 210;
        break;
    case UiColorPicker::HARMONY_SHADES:
    case UiColorPicker::HARMONY_MONOCHROMATIC:
    case UiColorPicker::HARMONY_CUSTOM:
    case UiColorPicker::HARMONY_IMAGE_EXTRACT:
    default:
        out << 0;
        break;
    }
    return out;
}

void ResetGeneratorFamilies(GeneratorRecipe& recipe, bool preserve_family_state)
{
    Vector<PaletteFamily> old = clone(recipe.families);
    Vector<int> offsets = HarmonyOffsets(recipe.harmony);
    if(recipe.harmony == UiColorPicker::HARMONY_CUSTOM && preserve_family_state && !old.IsEmpty())
        return;
    recipe.families.Clear();
    for(int i = 0; i < offsets.GetCount(); i++) {
        PaletteFamily& family = recipe.families.Add();
        family.id = i;
        family.canonical_offset = offsets[i];
        family.custom_offset = offsets[i];
        family.authored_saturation = i < old.GetCount() ? old[i].authored_saturation : max(35, 84 - i * 8);
        family.gain = i < old.GetCount() ? old[i].gain : 0;
        family.locked = i < old.GetCount() ? old[i].locked : false;
        family.priority = offsets.GetCount() - i;
        family.role = FamilyRole(i, offsets.GetCount());
    }
}

double GlobalSaturationScale(const GeneratorRecipe& recipe)
{
    int global = minmax(recipe.global_saturation, 0, 150);
    if(global <= 100)
        return global / 100.0;
    int maximum = 0;
    for(const PaletteFamily& family : recipe.families)
        if(!family.locked)
            maximum = max(maximum, family.authored_saturation);
    if(maximum <= 0)
        return 1.0;
    double rim_scale = 100.0 / maximum;
    double t = (global - 100) / 50.0;
    return 1.0 + (rim_scale - 1.0) * t;
}

Vector<int> ApportionFamilies(const GeneratorRecipe& recipe, int active_family_count)
{
    int family_count = recipe.families.GetCount();
    Vector<int> allocation;
    allocation.SetCount(family_count, 0);
    if(family_count == 0 || recipe.requested_count <= 0 || active_family_count <= 0)
        return allocation;
    active_family_count = min(active_family_count, family_count);
    int requested = minmax(recipe.requested_count, 1, 12);
    int active = min(active_family_count, requested);
    for(int i = 0; i < active; i++)
        allocation[i] = 1;
    int remaining = requested - active;
    if(remaining <= 0)
        return allocation;

    Vector<double> weights = DistributionWeights(recipe.distribution, active);
    double sum = 0.0;
    for(double w : weights)
        sum += max(0.0, w);
    if(sum <= 0.0)
        sum = active;
    Vector<double> remainder;
    remainder.SetCount(active, 0.0);
    int assigned = 0;
    for(int i = 0; i < active; i++) {
        double quota = remaining * max(0.0, weights[i]) / sum;
        int base = int(floor(quota));
        allocation[i] += base;
        remainder[i] = quota - base;
        assigned += base;
    }
    int left = remaining - assigned;
    Vector<int> order;
    for(int i = 0; i < active; i++)
        order.Add(i);
    for(int i = 0; i < order.GetCount(); i++) {
        int best = i;
        for(int j = i + 1; j < order.GetCount(); j++) {
            int a = order[best], b = order[j];
            if(remainder[b] > remainder[a] + 1e-12 ||
               (fabs(remainder[b] - remainder[a]) <= 1e-12 &&
                (recipe.families[b].priority > recipe.families[a].priority ||
                 (recipe.families[b].priority == recipe.families[a].priority && b < a))))
                best = j;
        }
        Swap(order[i], order[best]);
    }
    for(int i = 0; i < left; i++)
        allocation[order[i % order.GetCount()]]++;
    return allocation;
}

Vector<GeneratedSwatch> GeneratePalette(const GeneratorRecipe& source)
{
    GeneratorRecipe recipe = clone(source);
    recipe.requested_count = minmax(recipe.requested_count, 2, 12);
    recipe.global_gain = minmax(recipe.global_gain, -50, 50);
    recipe.global_saturation = minmax(recipe.global_saturation, 0, 150);
    if(recipe.families.IsEmpty())
        ResetGeneratorFamilies(recipe, false);
    int active_count = min(recipe.families.GetCount(), recipe.requested_count);
    Vector<int> allocation = ApportionFamilies(recipe, active_count);
    MediumProfile profile = Profile(recipe.medium);
    double global_scale = GlobalSaturationScale(recipe);
    Vector<GeneratedSwatch> out;

    for(int family_index = 0; family_index < active_count; family_index++) {
        const PaletteFamily& family = recipe.families[family_index];
        int count = allocation[family_index];
        if(count <= 0)
            continue;
        int offset = recipe.free_angles ? family.custom_offset : family.canonical_offset;
        int hue = NormalizeHue(recipe.base_hue + offset);
        double saturation = family.locked ? family.authored_saturation
                                          : family.authored_saturation * global_scale;
        saturation = minmax(saturation * profile.saturation_character,
                            0.0, profile.saturation_limit * 100.0);
        int global_gain = family.locked ? 0 : recipe.global_gain;
        for(int tone = 0; tone < count; tone++) {
            int value = profile.hero_value + ToneOffset(tone, count, recipe.distribution)
                      + global_gain + family.gain;
            value = minmax(value, profile.minimum_value, profile.maximum_value);
            GeneratedSwatch& swatch = out.Add();
            swatch.family_id = family.id;
            swatch.source_index = tone;
            swatch.hero = tone == 0;
            swatch.value = Slot(HsvToColor(hue, saturation, value), 255,
                                tone == 0 ? family.role : Format("%s tone %d", family.role, tone + 1));
        }
    }
    while(out.GetCount() > recipe.requested_count)
        out.Remove(out.GetCount() - 1);
    return out;
}

const Vector<StaticPaletteDefinition>& StaticPaletteLibrary()
{
    static Vector<StaticPaletteDefinition> palette;
    ONCELOCK {
        palette.Add(MakeHuePalette("full-spectrum", "Full Spectrum", 6, 96, 100, 5, 12));
        palette.Add(MakeHuePalette("strong-vivid", "Strong Vivid", 5, 100, 100, 4, 13));

        StaticPaletteDefinition pastel;
        pastel.id = "pastel-soft"; pastel.name = "Pastel Soft"; pastel.category = "Creative";
        pastel.preferred_columns = 12;
        for(int row = 0; row < 4; row++)
            for(int column = 0; column < 12; column++)
                AddPaletteColor(pastel, HslToColor(column * 30, 48 - row * 6, 91 - row * 8));
        palette.Add(pick(pastel));

        StaticPaletteDefinition tint;
        tint.id = "light-tints"; tint.name = "Light Tints"; tint.category = "Creative";
        tint.preferred_columns = 12;
        for(int row = 0; row < 4; row++)
            for(int column = 0; column < 12; column++)
                AddPaletteColor(tint, HslToColor(column * 30, 62 - row * 7, 96 - row * 6));
        palette.Add(pick(tint));

        StaticPaletteDefinition earth;
        earth.id = "earth-muted"; earth.name = "Earth Muted"; earth.category = "Creative";
        earth.preferred_columns = 12;
        const int earth_hue[12] = { 10, 20, 31, 42, 55, 70, 86, 104, 126, 150, 178, 205 };
        for(int row = 0; row < 5; row++)
            for(int column = 0; column < 12; column++)
                AddPaletteColor(earth, HslToColor(earth_hue[column], 58 - row * 6, 28 + row * 12));
        palette.Add(pick(earth));

        StaticPaletteDefinition ui;
        ui.id = "bright-ui"; ui.name = "Bright UI Accents"; ui.category = "Creative";
        ui.preferred_columns = 5;
        const Color bases[5] = { Color(34,197,94), Color(245,158,11), Color(239,68,68),
                                 Color(59,130,246), Color(168,85,247) };
        const int mix[5] = { 185, 110, 0, -55, -115 };
        for(int row = 0; row < 5; row++)
            for(int column = 0; column < 5; column++)
                AddPaletteColor(ui, mix[row] >= 0 ? Blend(bases[column], White(), mix[row])
                                                  : Blend(bases[column], Black(), -mix[row]));
        palette.Add(pick(ui));

        StaticPaletteDefinition grey;
        grey.id = "grayscale"; grey.name = "Grayscale"; grey.category = "Creative";
        grey.preferred_columns = 12;
        for(int i = 0; i < 12; i++) {
            int level = 255 - i * 23;
            AddPaletteColor(grey, Color(level, level, level), Format("Gray %d", level));
        }
        palette.Add(pick(grey));

        StaticPaletteDefinition web;
        web.id = "web-safe"; web.name = "Web Safe"; web.category = "Creative";
        web.preferred_columns = 12;
        const int levels[6] = { 0, 51, 102, 153, 204, 255 };
        for(int r : levels)
            for(int g : levels)
                for(int b : levels)
                    AddPaletteColor(web, Color(r, g, b));
        palette.Add(pick(web));

        palette.Add(MakeHuePalette("wide-gamut", "HDR / Wide Gamut Reference", 5, 100, 100, 2, 9));

        StaticPaletteDefinition skin;
        skin.id = "skin-tones"; skin.name = "Skin Tones"; skin.category = "Reference";
        skin.source_reference = "Monk Skin Tone screen-reference sequence";
        skin.preferred_columns = 11;
        const Color skin_values[11] = {
            Color(246,237,228), Color(243,231,219), Color(247,234,208), Color(234,218,186),
            Color(215,189,150), Color(181,151,111), Color(160,126,86), Color(130,92,67),
            Color(96,65,52), Color(62,43,35), Color(41,30,25)
        };
        for(int i = 0; i < 11; i++) AddPaletteColor(skin, skin_values[i], Format("Tone %d", i + 1));
        palette.Add(pick(skin));

        StaticPaletteDefinition xrite;
        xrite.id = "xrite-colorchecker"; xrite.name = "X-Rite ColorChecker";
        xrite.category = "Calibration"; xrite.authoritative = false;
        xrite.source_reference = "ColorChecker Classic 24-patch sRGB screen reference; not measured chart truth";
        xrite.preferred_columns = 6;
        const Color xrite_values[24] = {
            Color(0x73,0x52,0x44), Color(0xC2,0x96,0x82), Color(0x62,0x7A,0x9D), Color(0x57,0x6C,0x43),
            Color(0x85,0x80,0xB1), Color(0x67,0xBD,0xAA), Color(0xD6,0x7E,0x2C), Color(0x50,0x5B,0xA6),
            Color(0xC1,0x5A,0x63), Color(0x5E,0x3C,0x73), Color(0x9D,0xBC,0x40), Color(0xE0,0xA3,0x2E),
            Color(0x38,0x3D,0x88), Color(0x46,0x94,0x49), Color(0xAF,0x36,0x3C), Color(0xF3,0xC3,0x00),
            Color(0x9C,0x5A,0xA5), Color(0x00,0xA1,0xC7), Color(0xF3,0xF3,0xF2), Color(0xC8,0xC8,0xC8),
            Color(0xA0,0xA0,0xA0), Color(0x7A,0x7A,0x7A), Color(0x55,0x55,0x55), Color(0x34,0x34,0x34)
        };
        for(int i = 0; i < 24; i++) AddPaletteColor(xrite, xrite_values[i], Format("Patch %02d", i + 1));
        palette.Add(pick(xrite));

        StaticPaletteDefinition big;
        big.id = "big-color-gray"; big.name = "B.I.G. Color & Gray Card";
        big.category = "Calibration"; big.source_reference = "Prior UiColorPicker 9-patch screen reference";
        big.preferred_columns = 3;
        const Color big_values[9] = {
            Color(245,245,245), Color(119,119,119), Color(35,35,35), Color(175,50,50),
            Color(50,150,50), Color(50,50,170), Color(0,180,180), Color(180,50,140), Color(220,190,30)
        };
        for(int i = 0; i < 9; i++) AddPaletteColor(big, big_values[i], Format("Patch %d", i + 1));
        palette.Add(pick(big));

        StaticPaletteDefinition video;
        video.id = "passport-video"; video.name = "ColorChecker Passport Video";
        video.category = "Calibration"; video.source_reference = "Prior UiColorPicker 24-patch screen reference";
        video.preferred_columns = 6;
        const Color video_values[24] = {
            Color(255,0,0), Color(0,255,0), Color(0,0,255), Color(0,255,255), Color(255,0,255), Color(255,255,0),
            Color(245,215,196), Color(232,184,154), Color(212,154,122), Color(184,122,92), Color(154,92,66), Color(122,66,46),
            Color(245,245,245), Color(208,208,208), Color(171,171,171), Color(134,134,134), Color(97,97,97), Color(60,60,60),
            Color(255,250,240), Color(240,230,210), Color(46,46,46), Color(31,31,31), Color(255,107,61), Color(61,107,255)
        };
        for(int i = 0; i < 24; i++) AddPaletteColor(video, video_values[i], Format("Video %02d", i + 1));
        palette.Add(pick(video));

        StaticPaletteDefinition spyder24;
        spyder24.id = "spyder-24"; spyder24.name = "SpyderCheckr 24"; spyder24.category = "Calibration";
        spyder24.source_reference = "Prior UiColorPicker 24-patch screen reference"; spyder24.preferred_columns = 6;
        const Color spyder_values[24] = {
            Color(157,107,83), Color(199,154,125), Color(110,139,90), Color(90,122,184),
            Color(184,90,110), Color(83,110,157), Color(142,76,54), Color(196,150,110),
            Color(78,62,48), Color(175,119,84), Color(122,139,175), Color(191,175,153),
            Color(240,240,240), Color(192,192,192), Color(144,144,144), Color(96,96,96),
            Color(48,48,48), Color(0,0,0), Color(255,0,0), Color(0,255,0),
            Color(0,0,255), Color(255,255,0), Color(255,0,255), Color(0,255,255)
        };
        for(int i = 0; i < 24; i++) AddPaletteColor(spyder24, spyder_values[i], Format("Patch %02d", i + 1));
        palette.Add(pick(spyder24));

        StaticPaletteDefinition spyder48;
        spyder48.id = "spyder-48"; spyder48.name = "SpyderCheckr 48"; spyder48.category = "Calibration";
        spyder48.source_reference = "Prior UiColorPicker extended 48-patch screen reference"; spyder48.preferred_columns = 8;
        for(const UiColorPicker::SlotValue& value : palette.Top().swatches)
            spyder48.swatches.Add(value);
        for(int i = 0; i < 24; i++)
            AddPaletteColor(spyder48, Blend(spyder_values[i], Black(), 45), Format("Dark patch %02d", i + 1));
        palette.Add(pick(spyder48));

        StaticPaletteDefinition monk;
        monk.id = "monk-scale"; monk.name = "Monk Skin Tone Scale"; monk.category = "Reference";
        monk.source_reference = "Ten-step Monk skin-tone display approximation used by prior picker";
        monk.preferred_columns = 10;
        const Color monk_values[10] = {
            Color(245,230,215), Color(237,210,192), Color(224,184,160), Color(208,154,122), Color(192,130,92),
            Color(168,106,66), Color(139,82,50), Color(110,62,38), Color(82,46,30), Color(62,34,24)
        };
        for(int i = 0; i < 10; i++) AddPaletteColor(monk, monk_values[i], Format("MSTS %d", i + 1));
        palette.Add(pick(monk));

        StaticPaletteDefinition industry;
        industry.id = "industry-reference"; industry.name = "Industrial Reference";
        industry.category = "Reference"; industry.source_reference = "Prior UiColorPicker screen-reference grid";
        industry.preferred_columns = 10;
        const Color industry_base[10] = {
            Color(186,12,47), Color(224,82,6), Color(239,179,0), Color(173,165,0), Color(0,137,85),
            Color(0,143,156), Color(0,102,161), Color(52,63,139), Color(112,48,160), Color(173,34,109)
        };
        const int industry_mix[10] = { 190,150,110,70,30,0,-28,-56,-84,-116 };
        for(int row = 0; row < 10; row++)
            for(int column = 0; column < 10; column++)
                AddPaletteColor(industry, industry_mix[row] >= 0
                    ? Blend(industry_base[column], White(), industry_mix[row])
                    : Blend(industry_base[column], Black(), -industry_mix[row]),
                    Format("REF %02d-%02d", row + 1, column + 1));
        palette.Add(pick(industry));
    }
    return palette;
}

int FindStaticPalette(const String& id)
{
    const Vector<StaticPaletteDefinition>& library = StaticPaletteLibrary();
    for(int i = 0; i < library.GetCount(); i++)
        if(library[i].id == id)
            return i;
    return -1;
}

Image MakeAnalysisProxy(const Image& image, int maximum_dimension)
{
    if(image.IsEmpty())
        return Image();
    maximum_dimension = minmax(maximum_dimension, 16, 4096);
    Size source = image.GetSize();
    int maximum = max(source.cx, source.cy);
    Size target = source;
    if(maximum > maximum_dimension) {
        double scale = maximum_dimension / double(maximum);
        target.cx = max(1, int(source.cx * scale + 0.5));
        target.cy = max(1, int(source.cy * scale + 0.5));
    }
    int64 length = int64(target.cx) * target.cy;
    if(length <= 0 || length > int64(maximum_dimension) * maximum_dimension)
        return Image();
    ImageBuffer buffer(target);
    for(int y = 0; y < target.cy; y++) {
        int sy = min(source.cy - 1, int((y + 0.5) * source.cy / target.cy));
        RGBA *row = buffer[y];
        for(int x = 0; x < target.cx; x++) {
            int sx = min(source.cx - 1, int((x + 0.5) * source.cx / target.cx));
            row[x] = image[sy][sx];
        }
    }
    return Image(buffer);
}

ImageAnalysisResult AnalyzeImage(const Image& original, const ImageAnalysisSettings& source_settings)
{
    ImageAnalysisResult result;
    result.original_size = original.GetSize();
    if(original.IsEmpty()) {
        result.diagnostic = "No image is loaded.";
        return result;
    }
    ImageAnalysisSettings settings = clone(source_settings);
    settings.requested_count = minmax(settings.requested_count, 2, 12);
    settings.tolerance = minmax(settings.tolerance, 1, 100);
    result.proxy = MakeAnalysisProxy(original, 512);
    result.proxy_size = result.proxy.GetSize();
    if(result.proxy.IsEmpty()) {
        result.diagnostic = "The image could not be converted to a bounded analysis proxy.";
        return result;
    }
    result.exclusion_mask = BuildExclusionMask(result.proxy, settings, result.ignored_fraction);
    if(settings.analysis == UiColorPicker::IMAGE_MANUAL_POINTS) {
        result.swatches = ManualPointPalette(result.proxy, settings.requested_count,
                                             result.representative_positions);
        result.diagnostic = Format("Manual sample proxy %dx%d.", result.proxy_size.cx, result.proxy_size.cy);
        return result;
    }
    if(result.ignored_fraction >= 0.98) {
        result.diagnostic = "Background exclusion removed nearly the entire image; lower tolerance or disable A/B.";
        return result;
    }

    Size size = result.proxy.GetSize();
    const int bin_count = 32768;
    Vector<int> counts;
    Vector<int64> sum_r, sum_g, sum_b, sum_x, sum_y;
    counts.SetCount(bin_count, 0);
    sum_r.SetCount(bin_count, 0); sum_g.SetCount(bin_count, 0); sum_b.SetCount(bin_count, 0);
    sum_x.SetCount(bin_count, 0); sum_y.SetCount(bin_count, 0);
    int valid_pixels = 0;
    for(int y = 0; y < size.cy; y++) {
        const RGBA *row = result.proxy[y];
        for(int x = 0; x < size.cx; x++) {
            int index = y * size.cx + x;
            if(index < result.exclusion_mask.GetCount() && result.exclusion_mask[index])
                continue;
            if(row[x].a < 16)
                continue;
            int key = ((row[x].r >> 3) << 10) | ((row[x].g >> 3) << 5) | (row[x].b >> 3);
            counts[key]++;
            sum_r[key] += row[x].r; sum_g[key] += row[x].g; sum_b[key] += row[x].b;
            sum_x[key] += x; sum_y[key] += y;
            valid_pixels++;
        }
    }
    if(valid_pixels == 0) {
        result.diagnostic = "Background exclusion removed every analysable pixel.";
        return result;
    }

    LabValue edge_mean;
    int edge_samples = 0;
    for(int x = 0; x < size.cx; x++) {
        LabValue top = ToLab(ProxyPixel(result.proxy, x, 0));
        LabValue bottom = ToLab(ProxyPixel(result.proxy, x, size.cy - 1));
        edge_mean.l += top.l + bottom.l; edge_mean.a += top.a + bottom.a; edge_mean.b += top.b + bottom.b;
        edge_samples += 2;
    }
    for(int y = 1; y + 1 < size.cy; y++) {
        LabValue left = ToLab(ProxyPixel(result.proxy, 0, y));
        LabValue right = ToLab(ProxyPixel(result.proxy, size.cx - 1, y));
        edge_mean.l += left.l + right.l; edge_mean.a += left.a + right.a; edge_mean.b += left.b + right.b;
        edge_samples += 2;
    }
    if(edge_samples) {
        edge_mean.l /= edge_samples; edge_mean.a /= edge_samples; edge_mean.b /= edge_samples;
    }

    Vector<ImageBin> bins;
    bins.Reserve(4096);
    for(int key = 0; key < bin_count; key++) {
        if(counts[key] <= 0)
            continue;
        ImageBin& bin = bins.Add();
        bin.key = key; bin.count = counts[key];
        bin.color = Color(int(sum_r[key] / counts[key]), int(sum_g[key] / counts[key]), int(sum_b[key] / counts[key]));
        bin.sum_x = sum_x[key]; bin.sum_y = sum_y[key];
        bin.lab = ToLab(bin.color);
        double chroma = sqrt(bin.lab.a * bin.lab.a + bin.lab.b * bin.lab.b);
        switch(settings.coverage) {
        case UiColorPicker::COVERAGE_AREA_WEIGHTED:
            bin.weight = bin.count;
            break;
        case UiColorPicker::COVERAGE_BORDER_AWARE: {
            double edge_distance = LabDistance(bin.lab, edge_mean);
            bin.weight = pow((double)bin.count, 0.65) * (0.25 + min(1.0, edge_distance / 35.0));
            break;
        }
        case UiColorPicker::COVERAGE_DISTINCTIVE:
            bin.weight = pow((double)bin.count, 0.45) * (0.20 + min(2.0, chroma / 28.0));
            break;
        case UiColorPicker::COVERAGE_BALANCED:
        default:
            bin.weight = pow((double)bin.count, 0.65);
            break;
        }
        if(settings.analysis == UiColorPicker::IMAGE_INTERFACE) {
            double extreme = max(fabs(bin.lab.l - 50.0) / 50.0, chroma / 70.0);
            bin.weight *= 0.75 + extreme;
        }
        else if(settings.analysis == UiColorPicker::IMAGE_ACCENT_FINDER)
            bin.weight *= 0.35 + min(2.5, chroma / 22.0);
        else if(settings.analysis == UiColorPicker::IMAGE_NATURE_SCENE)
            bin.weight *= 0.75 + min(1.25, chroma / 45.0);
        else if(settings.analysis == UiColorPicker::IMAGE_PAINT_MATERIAL)
            bin.weight *= bin.count < valid_pixels / 300 ? 0.15 : 1.0;
        bin.weight = max(0.0001, bin.weight);
    }
    if(bins.IsEmpty()) {
        result.diagnostic = "The image produced no stable colour bins.";
        return result;
    }

    int cluster_count = min(settings.requested_count, bins.GetCount());
    Vector<LabValue> centroid;
    centroid.SetCount(cluster_count);
    Vector<int> seed_index;
    seed_index.SetCount(cluster_count, -1);
    int first = 0;
    for(int i = 1; i < bins.GetCount(); i++)
        if(bins[i].weight > bins[first].weight + 1e-12 ||
           (fabs(bins[i].weight - bins[first].weight) <= 1e-12 && bins[i].key < bins[first].key))
            first = i;
    seed_index[0] = first;
    centroid[0] = bins[first].lab;
    for(int c = 1; c < cluster_count; c++) {
        int best = -1;
        double best_score = -1.0;
        for(int i = 0; i < bins.GetCount(); i++) {
            bool already = false;
            for(int j = 0; j < c; j++)
                already = already || seed_index[j] == i;
            if(already)
                continue;
            double distance = DBL_MAX;
            for(int j = 0; j < c; j++)
                distance = min(distance, LabDistanceSquared(bins[i].lab, centroid[j]));
            double score = distance * sqrt(bins[i].weight);
            if(score > best_score + 1e-12 ||
               (fabs(score - best_score) <= 1e-12 && (best < 0 || bins[i].key < bins[best].key))) {
                best = i; best_score = score;
            }
        }
        if(best < 0) best = c % bins.GetCount();
        seed_index[c] = best;
        centroid[c] = bins[best].lab;
    }

    Vector<int> assignment;
    assignment.SetCount(bins.GetCount(), 0);
    Vector<double> cluster_weight;
    cluster_weight.SetCount(cluster_count, 0.0);
    for(int iteration = 0; iteration < 10; iteration++) {
        Vector<double> sl, sa, sb;
        sl.SetCount(cluster_count, 0.0); sa.SetCount(cluster_count, 0.0); sb.SetCount(cluster_count, 0.0);
        cluster_weight.SetCount(cluster_count, 0.0);
        for(int i = 0; i < bins.GetCount(); i++) {
            int best = 0;
            double best_distance = LabDistanceSquared(bins[i].lab, centroid[0]);
            for(int c = 1; c < cluster_count; c++) {
                double distance = LabDistanceSquared(bins[i].lab, centroid[c]);
                if(distance < best_distance - 1e-12) { best = c; best_distance = distance; }
            }
            assignment[i] = best;
            double weight = bins[i].weight;
            sl[best] += bins[i].lab.l * weight;
            sa[best] += bins[i].lab.a * weight;
            sb[best] += bins[i].lab.b * weight;
            cluster_weight[best] += weight;
        }
        for(int c = 0; c < cluster_count; c++)
            if(cluster_weight[c] > 0.0) {
                centroid[c].l = sl[c] / cluster_weight[c];
                centroid[c].a = sa[c] / cluster_weight[c];
                centroid[c].b = sb[c] / cluster_weight[c];
            }
    }

    Vector<int> representative;
    representative.SetCount(cluster_count, -1);
    for(int i = 0; i < bins.GetCount(); i++) {
        int c = assignment[i];
        if(representative[c] < 0 ||
           LabDistanceSquared(bins[i].lab, centroid[c]) < LabDistanceSquared(bins[representative[c]].lab, centroid[c]) - 1e-12 ||
           (fabs(LabDistanceSquared(bins[i].lab, centroid[c]) - LabDistanceSquared(bins[representative[c]].lab, centroid[c])) <= 1e-12 &&
            bins[i].key < bins[representative[c]].key))
            representative[c] = i;
    }
    Vector<int> order;
    for(int c = 0; c < cluster_count; c++)
        order.Add(c);
    for(int i = 0; i < order.GetCount(); i++) {
        int best = i;
        for(int j = i + 1; j < order.GetCount(); j++) {
            int a = order[best], b = order[j];
            if(cluster_weight[b] > cluster_weight[a] + 1e-12 ||
               (fabs(cluster_weight[b] - cluster_weight[a]) <= 1e-12 && b < a))
                best = j;
        }
        Swap(order[i], order[best]);
    }

    for(int rank = 0; rank < order.GetCount(); rank++) {
        int c = order[rank];
        int ri = representative[c] >= 0 ? representative[c] : seed_index[c];
        const ImageBin& bin = bins[ri];
        GeneratedSwatch& swatch = result.swatches.Add();
        swatch.value = Slot(bin.color, 255, Format("Extracted %d", rank + 1));
        swatch.source_index = ri;
        swatch.hero = false;
        Pointf position((bin.sum_x / double(max(1, bin.count))) / max(1, size.cx - 1),
                        (bin.sum_y / double(max(1, bin.count))) / max(1, size.cy - 1));
        result.representative_positions.Add(position);
    }
    result.diagnostic = Format("Analyzed %d pixels through a %dx%d proxy; %.1f%% excluded.",
                               valid_pixels, size.cx, size.cy, result.ignored_fraction * 100.0);
    return result;
}

Vector<GeneratedSwatch> ApplyImagePostProcessing(const Vector<GeneratedSwatch>& source,
                                                  UiColorPicker::MediumMode medium,
                                                  int global_gain,
                                                  int global_saturation,
                                                  int hero_index,
                                                  int hero_gain)
{
    Vector<GeneratedSwatch> output = clone(source);
    MediumProfile profile = Profile(medium);
    global_gain = minmax(global_gain, -50, 50);
    global_saturation = minmax(global_saturation, 0, 150);
    hero_gain = minmax(hero_gain, -50, 50);
    for(int i = 0; i < output.GetCount(); i++) {
        int h = 0, s = 0, v = 0;
        ColorToHsv(output[i].value.color, h, s, v);
        double sat = s * global_saturation / 100.0 * profile.saturation_character;
        sat = min(sat, profile.saturation_limit * 100.0);
        int value = v + global_gain + (i == hero_index ? hero_gain : 0);
        value = minmax(value, profile.minimum_value, profile.maximum_value);
        output[i].value.color = HsvToColor(h, sat, value);
        output[i].hero = i == hero_index;
    }
    return output;
}

bool AddUniqueTransactional(Vector<UiColorPicker::SlotValue>& destination,
                            const Vector<UiColorPicker::SlotValue>& source,
                            int capacity, bool allow_duplicates, int *rejected)
{
    capacity = max(0, capacity);
    Vector<UiColorPicker::SlotValue> append;
    for(const UiColorPicker::SlotValue& value : source) {
        if(IsNull(value.color))
            continue;
        bool duplicate = false;
        if(!allow_duplicates) {
            for(const UiColorPicker::SlotValue& existing : destination)
                duplicate = duplicate || (existing.color == value.color && existing.alpha == value.alpha);
            for(const UiColorPicker::SlotValue& existing : append)
                duplicate = duplicate || (existing.color == value.color && existing.alpha == value.alpha);
        }
        if(!duplicate)
            append.Add(value);
    }
    int overflow = max(0, destination.GetCount() + append.GetCount() - capacity);
    if(rejected)
        *rejected = overflow;
    if(overflow > 0)
        return false;
    for(const UiColorPicker::SlotValue& value : append)
        destination.Add(value);
    return true;
}

} // namespace UiColorPickerPaletteLab
} // namespace Upp
