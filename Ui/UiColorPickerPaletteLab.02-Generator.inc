
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
