#include "UiColorPickerPaletteLab.h"

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
