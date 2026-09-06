#include <Ui/Ui.h>

using namespace Upp;

namespace {

struct TestCtx {
    int checks = 0;
    int fails = 0;

    void Expect(bool ok, const String& text)
    {
        checks++;
        Cout() << (ok ? "PASS: " : "FAIL: ") << text << '\n';
        if(!ok)
            fails++;
    }
};

StyledPalette Palette(Color face = Color(246, 248, 251),
                      Color frame = Color(148, 163, 184))
{
    StyledPalette palette;
    for(int i = 0; i < 4; i++) {
        palette.face[i] = UiFill::Solid(face);
        palette.frame[i] = frame;
        palette.ink[i] = Color(30, 41, 59);
        palette.icon[i] = Color(30, 41, 59);
    }
    return palette;
}

StyledMetrics Metrics(int radius)
{
    StyledMetrics metrics;
    metrics.face_enabled = true;
    metrics.frame_enabled = true;
    metrics.frame_width = 1;
    metrics.radius = radius;
    metrics.dashed = false;
    metrics.shadow.enabled = false;
    metrics.highlight.enabled = false;
    return metrics;
}

} // namespace

int RunStyledSurfaceCacheSuite()
{
    TestCtx t;
    ImageDraw draw(420, 180);
    StyledSkin skin;
    StyledPalette palette = Palette();

    UiRasterCache::Clear();
    UiRasterCacheStats start = UiRasterCache::GetStats();

    UiRasterCacheKey exact_a = UiRasterCacheKeyBuilder("double/exact").Add(0.1234561).Build();
    UiRasterCacheKey exact_b = UiRasterCacheKeyBuilder("double/exact").Add(0.1234562).Build();
    UiRasterCacheKey quant_a = UiRasterCacheKeyBuilder("double/quant").Add(0.1234561, 100).Build();
    UiRasterCacheKey quant_b = UiRasterCacheKeyBuilder("double/quant").Add(0.1234562, 100).Build();
    t.Expect(exact_a.encoded != exact_b.encoded,
             "double raster keys are exact by default instead of silently quantized");
    t.Expect(quant_a.encoded == quant_b.encoded,
             "quantized double keys require an explicit quantization argument");

    StyledMetrics rounded = Metrics(12);
    Rect rounded_rect = RectC(12, 12, 120, 48);
    UiPaintStyledBackground(draw, rounded_rect, palette, rounded, skin, ST_NORMAL, false);
    UiRasterCacheStats first = UiRasterCache::GetStats();

    t.Expect(first.entries == 1,
             "first rounded styled surface creates one shared raster entry");
    t.Expect(first.bytes > 0 && first.insertions == start.insertions + 1
             && first.trim_calls >= start.trim_calls + 1,
             "cache maintains byte totals incrementally and exposes insertion/trim churn");
    t.Expect(first.misses == start.misses + 1,
             "first rounded styled surface records one cache miss");

    UiPaintStyledBackground(draw, rounded_rect, palette, rounded, skin, ST_NORMAL, false);
    UiRasterCacheStats second = UiRasterCache::GetStats();
    t.Expect(second.entries == first.entries,
             "repeated rounded styled surface reuses the existing entry");
    t.Expect(second.hits == first.hits + 1,
             "repeated rounded styled surface records a cache hit");
    t.Expect(second.misses == first.misses,
             "repeated rounded styled surface does not rerasterize");

    StyledMetrics flat = Metrics(0);
    UiPaintStyledBackground(draw, RectC(150, 12, 120, 48),
                            palette, flat, skin, ST_NORMAL, false);
    UiRasterCacheStats after_flat = UiRasterCache::GetStats();
    t.Expect(after_flat.entries == second.entries
             && after_flat.hits == second.hits
             && after_flat.misses == second.misses,
             "flat styled surface remains on direct Draw without cache traffic");

    StyledMetrics dashed = Metrics(12);
    dashed.dashed = true;
    dashed.dash_pattern = "4,3";
    UiPaintStyledBackground(draw, RectC(288, 12, 120, 48),
                            palette, dashed, skin, ST_NORMAL, false);
    UiRasterCacheStats after_dashed = UiRasterCache::GetStats();
    t.Expect(after_dashed.entries == after_flat.entries
             && after_dashed.hits == after_flat.hits
             && after_dashed.misses == after_flat.misses,
             "dashed rounded surface retains the established fallback path");

    StyledPalette alternate = Palette(Color(232, 240, 250), Color(100, 116, 139));
    UiPaintStyledBackground(draw, RectC(12, 82, 120, 48),
                            alternate, rounded, skin, ST_NORMAL, false);
    UiRasterCacheStats changed = UiRasterCache::GetStats();
    t.Expect(changed.entries == after_dashed.entries + 1,
             "different rounded surface presentation gets a distinct cache entry");
    t.Expect(changed.misses == after_dashed.misses + 1,
             "different rounded surface presentation records a new miss");

    StyledMetrics shadowed = rounded;
    shadowed.shadow.enabled = true;
    shadowed.shadow.distance = 4;
    shadowed.shadow.offset_x = 1;
    shadowed.shadow.offset_y = 1;
    shadowed.shadow.alpha = 40;
    Rect shadow_rect = RectC(150, 82, 120, 48);
    UiPaintStyledBackground(draw, shadow_rect, palette, shadowed, skin, ST_NORMAL, false);
    UiRasterCacheStats shadow_first = UiRasterCache::GetStats();
    UiPaintStyledBackground(draw, shadow_rect, palette, shadowed, skin, ST_NORMAL, false);
    UiRasterCacheStats shadow_second = UiRasterCache::GetStats();

    t.Expect(shadow_first.entries >= changed.entries + 1,
             "shadowed rounded surface preserves cached shadow composition");
    t.Expect(shadow_second.hits >= shadow_first.hits + 2,
             "repeated shadowed surface reuses both body and shadow cache work");

    Cout() << "\nUI_STYLED_SURFACE_CACHE_STATS"
           << " entries=" << shadow_second.entries
           << " bytes=" << shadow_second.bytes
           << " hits_delta=" << (shadow_second.hits - start.hits)
           << " misses_delta=" << (shadow_second.misses - start.misses)
           << " evictions_delta=" << (shadow_second.evictions - start.evictions)
           << " skipped_delta=" << (shadow_second.skipped_too_large - start.skipped_too_large)
           << '\n';

    Cout() << "UI_STYLED_SURFACE_CACHE_SUMMARY checks=" << t.checks
           << " failed=" << t.fails << '\n';
    return t.fails ? 1 : 0;
}
