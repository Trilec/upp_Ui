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
