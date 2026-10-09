#include <Core/Core.h>
#include <Ui/Ui.h>
#include <cmath>
#include <limits>

using namespace Upp;

struct TestCtx {
    int checks = 0;
    int fails = 0;

    void Expect(bool cond, const String& msg)
    {
        checks++;
        if(!cond) {
            fails++;
            Cout() << "[FAIL] " << msg << "\n";
        }
    }

    void Section(const String& name)
    {
        Cout() << "\n=== " << name << " ===\n";
    }
};

static bool Near(double a, double b, double eps = 0.0001)
{
    return fabs(a - b) <= eps;
}

static void TestDataAndNormalization(TestCtx& t)
{
    t.Section("Data and normalization");

    UiRangeSegments r;
    t.Expect(r.GetSegmentCount() == 4, "default control starts with four equal segments");
    t.Expect(Near(r.GetMin(), 0.0) && Near(r.GetMax(), 100.0), "default scalar domain is 0..100");
    t.Expect(r.GetBoundaryCount() == 3, "four segments expose three internal boundaries");
    t.Expect(r.GetSelectedSegment() == -1 && r.GetActiveBoundary() == -1,
             "default control starts visually neutral with no selected segment or boundary");
    t.Expect(Near(r.GetBoundaryValue(0), 25.0) && Near(r.GetBoundaryValue(1), 50.0) && Near(r.GetBoundaryValue(2), 75.0),
             "default boundaries are equal quarters");

    Vector<UiRangeSegment> weighted;
    weighted.Add(UiRangeSegment(1, "LOD 0"));
    weighted.Add(UiRangeSegment(2, "LOD 1"));
    weighted.Add(UiRangeSegment(1, "LOD 2"));
    r.SetSegments(weighted);
    t.Expect(Near(r.GetSegmentSpan(0), 25.0) && Near(r.GetSegmentSpan(1), 50.0) && Near(r.GetSegmentSpan(2), 25.0),
             "SetSegments treats authored spans as proportional weights across the fixed domain");
    t.Expect(r.GetSegment(1).label == "LOD 1", "segment labels survive normalization");

    r.SetRange(-2.0, 2.0);
    t.Expect(Near(r.GetSegmentSpan(0), 1.0) && Near(r.GetSegmentSpan(1), 2.0) && Near(r.GetSegmentSpan(2), 1.0),
             "changing the scalar domain preserves segment ratios");
    t.Expect(Near(r.GetBoundaryValue(0), -1.0) && Near(r.GetBoundaryValue(1), 1.0),
             "boundaries are reported in domain units, not percentages");

    r.SetRange(0, 100).SetSegmentCount(5);
    double sum = 0.0;
    for(int i = 0; i < r.GetSegmentCount(); i++)
        sum += r.GetSegmentSpan(i);
    t.Expect(Near(sum, 100.0), "equal segment count fills the complete fixed range");
    t.Expect(r.GetBoundaryCount() == 4, "N segments always expose N-1 boundaries");
}

static void TestBoundaryEditing(TestCtx& t)
{
    t.Section("Boundary editing");

    UiRangeSegments r;
    r.SetRange(0, 100).SetSegmentCount(4);
    r.SetBoundaryValue(1, 60);
    t.Expect(Near(r.GetSegmentSpan(0), 25.0), "moving a boundary leaves earlier unrelated segments unchanged");
    t.Expect(Near(r.GetSegmentSpan(1), 35.0) && Near(r.GetSegmentSpan(2), 15.0),
             "moving a boundary redistributes only its two neighbouring spans");
    t.Expect(Near(r.GetSegmentSpan(3), 25.0), "moving a boundary leaves later unrelated segments unchanged");
    t.Expect(Near(r.GetBoundaryValue(2), 75.0), "later boundary remains fixed while adjacent spans redistribute");

    r.SetStep(5).SetBoundaryValue(1, 63);
    t.Expect(Near(r.GetBoundaryValue(1), 65.0), "boundary editing snaps to the scalar step");

    r.SetMinimumSegmentSpan(10).SetBoundaryValue(0, 2);
    t.Expect(Near(r.GetBoundaryValue(0), 10.0), "minimum segment span clamps the left side of a boundary");
    r.SetBoundaryValue(0, 62);
    t.Expect(Near(r.GetBoundaryValue(0), 55.0), "minimum segment span clamps against the neighbouring segment on the right");

    Vector<double> values = r.GetBoundaryValues();
    t.Expect(values.GetCount() == r.GetBoundaryCount(), "boundary vector exposes every internal threshold");
    t.Expect(Near(values[0], r.GetBoundaryValue(0)), "boundary vector uses the same authoritative scalar values");

    UiRangeSegments thresholds;
    thresholds.SetRange(0, 100).SetStep(1).SetMinimumSegmentSpan(5);
    Vector<double> authored;
    authored.Add(15);
    authored.Add(45);
    authored.Add(82);
    thresholds.SetBoundaryValues(authored);
    t.Expect(thresholds.GetSegmentCount() == 4, "SetBoundaryValues derives N+1 contiguous segments from N thresholds");
    t.Expect(Near(thresholds.GetBoundaryValue(0), 15.0) && Near(thresholds.GetBoundaryValue(1), 45.0) && Near(thresholds.GetBoundaryValue(2), 82.0),
             "SetBoundaryValues preserves valid authored scalar thresholds");
    t.Expect(Near(thresholds.GetSegmentSpan(0), 15.0) && Near(thresholds.GetSegmentSpan(3), 18.0),
             "threshold-array input resolves segment spans against the fixed domain");
}

static void TestStructureMutation(TestCtx& t)
{
    t.Section("Split and remove");

    UiRangeSegments r;
    Vector<UiRangeSegment> pair;
    pair.Add(UiRangeSegment(50, "Near"));
    pair.Add(UiRangeSegment(50, "Far"));
    r.SetRange(0, 100).SetSegments(pair);
    r.SplitSegment(0, 0.25, "Micro");
    t.Expect(r.GetSegmentCount() == 3, "SplitSegment adds one contiguous range");
    t.Expect(Near(r.GetSegmentSpan(0), 12.5) && Near(r.GetSegmentSpan(1), 37.5),
             "SplitSegment divides only the chosen span using the requested ratio");
    t.Expect(r.GetSegment(1).label == "Micro", "split segment accepts a new label");

    double before = 0.0;
    for(int i = 0; i < r.GetSegmentCount(); i++)
        before += r.GetSegmentSpan(i);
    r.RemoveSegment(1);
    double after = 0.0;
    for(int i = 0; i < r.GetSegmentCount(); i++)
        after += r.GetSegmentSpan(i);
    t.Expect(r.GetSegmentCount() == 2, "RemoveSegment removes one authored range");
    t.Expect(Near(before, after) && Near(after, 100.0), "removing a segment merges its span and preserves the fixed total");

    r.ClearSegments();
    t.Expect(r.GetSegmentCount() == 0 && r.GetBoundaryCount() == 0, "ClearSegments removes data and boundaries together");
}

static void TestDataBinding(TestCtx& t)
{
    t.Section("Value binding");

    UiRangeSegments r;
    Vector<UiRangeSegment> authored;
    authored.Add(UiRangeSegment(30, "Normal", Color(1, 2, 3), String("normal")));
    authored.Add(UiRangeSegment(70, "Reduced", Null, String("reduced")));
    r.SetSegments(authored);

    Value data = r.GetData();
    t.Expect(data.Is<ValueArray>(), "GetData exports a serializable ValueArray");
    ValueArray array = data;
    t.Expect(array.GetCount() == 2 && array[0].Is<ValueMap>(), "each exported segment is a ValueMap record");

    UiRangeSegments copy;
    copy.SetData(data);
    t.Expect(copy.GetSegmentCount() == 2, "SetData restores the exported segment collection");
    t.Expect(copy.GetSegment(0).label == "Normal" && copy.GetSegment(1).label == "Reduced", "SetData restores labels");
    t.Expect(copy.GetSegment(0).color == Color(1, 2, 3), "SetData restores explicit segment colours");
    t.Expect(AsString(copy.GetSegment(1).data) == "reduced", "SetData restores application payload values");
    t.Expect(Near(copy.GetSegmentSpan(0), 30.0) && Near(copy.GetSegmentSpan(1), 70.0), "SetData restores proportional spans");
}

static void TestGeometry(TestCtx& t)
{
    t.Section("Geometry and orientation");

    UiRangeSegments r;
    r.SetRange(0, 100).SetSegmentCount(4);
    UiRangeSegments::Geometry h = r.GetGeometry(Size(420, 90));
    t.Expect(!h.track.IsEmpty() && !h.content.IsEmpty(), "horizontal geometry resolves track and content rectangles");
    t.Expect(h.segments.GetCount() == 4 && h.boundaries.GetCount() == 3, "geometry exposes segment and boundary projections");
    t.Expect(h.segments[0].rect.left == h.content.left && h.segments[3].rect.right == h.content.right,
             "horizontal segments cover the complete content extent");
    t.Expect(h.segments[0].rect.right == h.segments[1].rect.left,
             "adjacent horizontal segments remain contiguous without hidden gaps");

    r.SetDirection(UiDirection::V);
    UiRangeSegments::Geometry v = r.GetGeometry(Size(120, 420));
    t.Expect(v.track.GetHeight() > v.track.GetWidth(), "vertical mode rotates the major axis");
    t.Expect(v.segments[0].rect.top == v.content.top && v.segments[3].rect.bottom == v.content.bottom,
             "vertical segments cover the complete content extent");
    t.Expect(v.boundaries[0].y < v.boundaries[1].y && v.boundaries[1].y < v.boundaries[2].y,
             "vertical boundaries preserve scalar ordering from top to bottom");

    r.SetDirection(UiDirection::H).SetReverse(true);
    UiRangeSegments::Geometry reversed = r.GetGeometry(Size(420, 90));
    t.Expect(r.IsReversed(), "reverse presentation is explicit control state");
    t.Expect(reversed.segments[0].rect.right == reversed.content.right &&
             reversed.segments[3].rect.left == reversed.content.left,
             "reverse presentation flips visual segment order without changing semantic indexes");
    t.Expect(reversed.boundaries[0].x > reversed.boundaries[1].x && reversed.boundaries[1].x > reversed.boundaries[2].x,
             "reverse presentation flips boundary projection while scalar values remain ordered");
    t.Expect(Near(r.GetBoundaryValue(0), 25.0) && Near(r.GetBoundaryValue(2), 75.0),
             "reverse presentation never mutates scalar thresholds");
}

static void TestPaletteThemeAndOverrides(TestCtx& t)
{
    t.Section("Palette, theme and overrides");

    UiThemeContext saved = UiTheme::GetContext();
    UiThemeContext light = saved;
    light.preset = UiThemePreset::Minimal;
    light.mode = UiThemeMode::Light;
    UiTheme::Set(light);

    UiRangeSegments r;
    r.SetSegmentCount(4);
    t.Expect(r.GetRole() == UiRole::Standard, "default semantic role is Standard");

    Vector<Color> anchors;
    anchors.Add(Color(255, 0, 0));
    anchors.Add(Color(0, 255, 0));
    anchors.Add(Color(0, 0, 255));
    r.SetPalette(anchors).SetPaletteMode(UiRangeSegments::PaletteMode::Gradient);
    UiRangeSegments::Geometry gradient = r.GetGeometry(Size(400, 90));
    t.Expect(gradient.segments[0].color == anchors[0], "gradient palette begins at the first authored anchor");
    t.Expect(gradient.segments[3].color == anchors[2], "gradient palette ends at the last authored anchor");
    t.Expect(gradient.segments[1].color != gradient.segments[0].color && gradient.segments[1].color != gradient.segments[3].color,
             "intermediate segment colours are deterministic interpolated samples");

    UiRangeSegment explicit_segment = r.GetSegment(1);
    explicit_segment.color = Color(7, 8, 9);
    r.SetSegment(1, explicit_segment);
    t.Expect(r.GetGeometry(Size(400, 90)).segments[1].color == Color(7, 8, 9),
             "explicit per-segment colour overrides automatic palette resolution");

    UiRangeSegments themed;
    themed.SetRole(UiRole::Alert).SetSegmentCount(3);
    UiSlider::Style alert = UiTheme::ResolveSlider(UiRole::Alert);
    Color alert_primary = alert.track_palette.ink[ST_NORMAL];
    t.Expect(themed.GetGeometry(Size(400, 90)).segments[0].color == alert_primary,
             "semantic role drives the inherited first series colour");

    UiRangeSegments::Style local = themed.GetStyle();
    local.series[0] = Color(12, 34, 56);
    themed.SetCustomStyle(local);
    UiThemeContext dark = light;
    dark.mode = UiThemeMode::Dark;
    UiTheme::Set(dark);
    t.Expect(themed.GetGeometry(Size(400, 90)).segments[0].color == Color(12, 34, 56),
             "custom style remains explicit across later theme revisions");
    themed.ClearCustomStyle();
    t.Expect(themed.GetGeometry(Size(400, 90)).segments[0].color != Color(12, 34, 56),
             "clearing custom style restores live role/theme inheritance");

    UiTheme::Set(saved);
}

static void TestSelectionAndPaint(TestCtx& t)
{
    t.Section("Selection and paint smoke");

    UiRangeSegments r;
    r.SetSelectedSegment(2).SetActiveBoundary(1);
    t.Expect(r.GetSelectedSegment() == 2, "selected segment is independently addressable");
    t.Expect(r.GetActiveBoundary() == 1, "active boundary is independently addressable");
    r.SetSelectedSegment(99).SetActiveBoundary(99);
    t.Expect(r.GetSelectedSegment() == -1 && r.GetActiveBoundary() == -1,
             "invalid public selection indexes normalize to none");

    r.SetRect(0, 0, 420, 90);
    ImageDraw draw(420, 90);
    r.Paint(draw);
    t.Expect(true, "horizontal paint completes without assertions");

    r.SetDirection(UiDirection::V).SetRect(0, 0, 120, 420);
    ImageDraw draw_vertical(120, 420);
    r.Paint(draw_vertical);
    t.Expect(true, "vertical paint completes without assertions");
}

static void TestCardPresentation(TestCtx& t)
{
    t.Section("Card presentation and mirrored stacks");
    UiRangeSegments r;
    Vector<UiRangeSegment> rows;
    for(int i=0;i<4;i++) {
        UiRangeSegment row(1,Format("Act %d",i+1));
        row.subtitle="28 pages - 12 scenes";
        rows.Add(row);
    }
    r.SetSegments(rows).ShowSegmentValues().ShowBoundaryValues(false).ShowEndpointValues(false);
    auto style=r.GetStyle();
    style.thumb_size=Size(6,28); style.thumb_rotate_with_direction=true;
    style.thumb_shape=UiRangeSegments::ThumbShape::RoundedRectangle;
    style.thumb_metrics.radius=3; style.thumb_hover_growth=0;
    style.label_padding=10; style.label_gap=4;
    for(UiDirection dir : {UiDirection::H,UiDirection::V})
    for(bool reverse : {false,true}) for(UiAlign side : {UiAlign::LEFT,UiAlign::RIGHT}) {
        style.track_size.cy=dir==UiDirection::H ? 64 : 240; style.value_side=side;
        r.SetCustomStyle(style).SetDirection(dir).SetReverse(reverse);
        r.SetRect(0,0,dir==UiDirection::H ? 880 : 300,dir==UiDirection::H ? 110 : 480);
        auto g=r.GetGeometry(r.GetSize());
        t.Expect(g.boundary_thumbs.GetCount()==3,"geometry exposes actual boundary handle bounds");
        for(int i=0;i<3;i++) {
            t.Expect(g.boundary_thumbs[i].GetSize()==(dir==UiDirection::H ? Size(6,28) : Size(28,6)),
                     "pill handles rotate with vertical stacks without inflating their visual width");
            t.Expect(g.boundary_thumbs[i].CenterPoint()==g.boundaries[i],"handles stay centred on their shared boundary");
        }
        for(const auto& sg : g.segments) {
            t.Expect(sg.value_text=="25%","card value is its span percentage, not its boundary value");
            t.Expect(!sg.label_rect.IsEmpty() && !sg.subtitle_rect.IsEmpty(),"roomy cards expose both title and subtitle");
            t.Expect(sg.label_rect.bottom<=sg.subtitle_rect.top,"title and subtitle never overlap");
            t.Expect(sg.label_align==(side==UiAlign::LEFT ? UiAlign::RIGHT : UiAlign::LEFT),
                     "automatic text alignment mirrors opposite the percentage");
            t.Expect(side==UiAlign::LEFT ? sg.value_rect.right<=sg.label_rect.left :
                                         sg.value_rect.left>=sg.label_rect.right,
                     "percentage and text occupy disjoint columns in both orientations");
            t.Expect(sg.rect.Contains(sg.label_rect) && sg.rect.Contains(sg.subtitle_rect) && sg.rect.Contains(sg.value_rect),
                     "all text rectangles stay inside their own segment");
        }
        ImageDraw draw(r.GetSize()); draw.DrawRect(r.GetSize(),White()); r.Paint(draw);
        t.Expect(Image(draw).GetSize()==r.GetSize(),"mirrored horizontal and vertical cards paint natively");
        r.SetActiveBoundary(1);
        r.Key(dir==UiDirection::H ? (reverse ? K_LEFT : K_RIGHT) : (reverse ? K_UP : K_DOWN),1);
        t.Expect(r.GetSegmentValueText(1)=="26%" && r.GetSegmentValueText(2)=="24%",
                 "editing in either orientation updates the neighbouring percentages from the scalar model");
        t.Expect(r.GetSegment(1).subtitle==rows[1].subtitle && r.GetSegment(2).label==rows[2].label,
                 "boundary edits preserve segment titles and subtitles");
        r.SetBoundaryValue(1,50).SetActiveBoundary(-1);
    }
    r.SetDirection(UiDirection::H).SetReverse(false);
    style.track_size.cy=64; style.value_side=UiAlign::RIGHT;
    r.SetCustomStyle(style).SetRect(0,0,880,110);
    UiRangeSegment single=r.GetSegment(1); single.subtitle.Clear(); r.SetSegment(1,single);
    auto g=r.GetGeometry(r.GetSize());
    t.Expect(g.segments[1].subtitle_rect.IsEmpty(),"absent subtitle does not reserve a second line");
    t.Expect(abs(g.segments[1].label_rect.CenterPoint().y-g.segments[1].rect.CenterPoint().y)<=1,
             "title without subtitle centres vertically");
    r.SetRange(-20,80);
    t.Expect(r.GetSegmentValueText(0)=="25%","percentage is independent of the domain origin");
    r.SetRange(10000000000000000.0,10000000000000004.0);
    t.Expect(r.GetSegmentValueText(0)=="25%","percentage does not lose precision by adding the domain origin");
    r.SetRange(1000,1200).SetValueDisplay(UiRangeSegments::ValueDisplay::Domain);
    t.Expect(r.GetSegmentValueText(0)=="50","domain readouts display span length");
    t.Expect(r.GetSegmentValueText(-1).IsEmpty() && r.GetSegmentValueText(4).IsEmpty(),"invalid readout indexes are harmless");
    r.ShowSegmentValues(false); g=r.GetGeometry(r.GetSize());
    t.Expect(g.segments[0].value_rect.IsEmpty(),"segment readouts can be hidden independently");
    r.ShowSegmentValues().ShowLabels(false); g=r.GetGeometry(r.GetSize());
    t.Expect(g.segments[0].label_rect.IsEmpty() && !g.segments[0].value_rect.IsEmpty(),"value-only cards do not require labels");
    r.ShowLabels().SetRect(0,0,90,50); g=r.GetGeometry(r.GetSize());
    for(const auto& sg:g.segments)
        t.Expect(sg.label_rect.IsEmpty() || (sg.label_rect & sg.value_rect).IsEmpty(),"narrow cards clip instead of overlapping columns");
    r.SetRect(0,0,880,110);
    Point handle=r.GetGeometry(r.GetSize()).boundaries[0]; handle.x+=6;
    t.Expect(r.CursorImage(handle,0)==Image::SizeHorz(),"slim handles retain a usable minimum hit target");
}

static void TestPresentationPersistence(TestCtx& t)
{
    t.Section("Presentation persistence compatibility");
    UiRangeSegment old(25,"Act 1",Color(2,3,4),String("payload"));
    Event<Stream&> legacy=[&](Stream& s){s % old.span % old.label % old.color % old.data;};
    t.Expect(StoreAsString(old)==StoreAsString(legacy),"empty subtitle preserves legacy record bytes");
    UiRangeSegment copy; copy.subtitle="stale";
    t.Expect(LoadFromString(copy,StoreAsString(legacy)) && copy.subtitle.IsEmpty(),"legacy record loads and resets reused subtitle state");
    old.subtitle="28 pages - 12 scenes";
    t.Expect(LoadFromString(copy,StoreAsString(old)) && copy.subtitle==old.subtitle && copy.data==old.data,
             "authored subtitle and application payload round-trip together");
    UiRangeSegments r; Vector<UiRangeSegment> rows; rows.Add(old); r.SetSegments(rows);
    UiRangeSegments bound; bound.SetData(r.GetData());
    t.Expect(bound.GetSegment(0).subtitle==old.subtitle,"Value binding retains subtitles");
    UiRangeSegments::Style style=UiRangeSegments::StyleDefault();
    Event<Stream&> legacy_style=[&](Stream& s){
        s % style.track_palette % style.track_metrics % style.track_skin
          % style.thumb_palette % style.thumb_metrics % style.thumb_skin % style.value_palette;
        for(int i=0;i<UiRangeSegments::MAX_SERIES_COLORS;i++) s % style.series[i];
        s % style.series_count % style.label_font % style.value_font % style.track_size % style.thumb_size
          % style.thumb_dot_diameter % style.divider_width % style.divider_color % style.selected_frame
          % style.selected_frame_width % style.label_padding;
    };
    String legacy_bytes=StoreAsString(legacy_style);
    t.Expect(StoreAsString(style)==legacy_bytes,"default presentation preserves legacy style bytes");
    style.value_side=UiAlign::LEFT; style.thumb_shape=UiRangeSegments::ThumbShape::RoundedRectangle;
    style.thumb_rotate_with_direction=true; style.thumb_hover_growth=0;
    style.label_color=Color(2,3,4); style.subtitle_color=Color(5,6,7); style.value_color=Color(8,9,10);
    style.label_gap=5; style.value_gap=11; style.right_font=StdFontZ(15).Bold();
    UiRangeSegments::Style restored;
    t.Expect(LoadFromString(restored,StoreAsString(style)) && restored.value_side==UiAlign::LEFT &&
             restored.thumb_shape==style.thumb_shape && restored.thumb_rotate_with_direction &&
             restored.label_color==style.label_color && restored.subtitle_color==style.subtitle_color &&
             restored.value_color==style.value_color && restored.right_font==style.right_font &&
             restored.label_gap==5 && restored.value_gap==11,"authored card style fields round-trip");
    t.Expect(LoadFromString(restored,legacy_bytes) && restored.value_side==UiAlign::RIGHT &&
             restored.thumb_shape==UiRangeSegments::ThumbShape::Ellipse,"legacy style loads and clears presentation extensions");
    String bad_style=StoreAsString(style); bad_style.Set(1,2);
    t.Expect(!LoadFromString(restored,bad_style),"unknown presentation versions fail without misreading following fields");
    String bad_row=StoreAsString(old); bad_row.Set(sizeof(double),2);
    t.Expect(!LoadFromString(copy,bad_row),"unknown segment versions fail explicitly");
    Vector<UiRangeSegment> records; records.Add(old); records.Add(UiRangeSegment(75,"Act 2"));
    Vector<UiRangeSegment> loaded;
    t.Expect(LoadFromString(loaded,StoreAsString(records)) && loaded.GetCount()==2 &&
             loaded[0].subtitle==old.subtitle && loaded[1].label=="Act 2" && loaded[1].subtitle.IsEmpty(),
             "mixed extended and legacy records preserve the next record boundary");
}

static void TestThumbRasterAndBounds(TestCtx& t)
{
    t.Section("Handle shape, exact raster reuse and bounded layout");
    UiRangeSegments r;
    r.SetSegmentCount(2).ShowLabels(false).ShowBoundaryValues(false).ShowEndpointValues(false).ShowDividers(false);
    r.SetRect(0,0,240,90);
    auto style=r.GetStyle();
    style.track_size.cy=64; style.series_count=1; style.series[0]=White();
    style.thumb_size=Size(20,40); style.thumb_dot_diameter=0;
    style.thumb_metrics.face_enabled=true; style.thumb_metrics.frame_enabled=false;
    style.thumb_metrics.radius=0; style.thumb_hover_growth=0;
    for(int st=0;st<4;st++) style.thumb_palette.face[st]=UiFill::Solid(Black());
    auto render=[&]() -> Image { ImageDraw draw(r.GetSize()); draw.DrawRect(r.GetSize(),White()); r.Paint(draw); return draw; };
    r.SetCustomStyle(style);
    UiRasterCache::Clear();
    Image ellipse=render();
    auto first=UiRasterCache::GetStats(); render(); auto second=UiRasterCache::GetStats();
    t.Expect(second.hits>first.hits && second.misses==first.misses,"identical handle and strip paints reuse exact cached rasters");
    style.thumb_shape=UiRangeSegments::ThumbShape::RoundedRectangle; r.SetCustomStyle(style);
    Image square=render(); auto changed=UiRasterCache::GetStats();
    Rect thumb=r.GetGeometry(r.GetSize()).boundary_thumbs[0];
    Point sample(thumb.left+2,thumb.top+2);
    t.Expect(ellipse[sample.y][sample.x].r>240 && square[sample.y][sample.x].r<15,
             "rounded-rectangle selection changes the actual silhouette rather than reusing an ellipse");
    t.Expect(changed.misses>second.misses,"handle shape participates in the raster cache key");
    style.thumb_metrics.radius=8; r.SetCustomStyle(style); Image rounded=render();
    t.Expect(rounded[sample.y][sample.x].r>square[sample.y][sample.x].r,
             "authored handle radius rounds the painted corners");
    style.thumb_size=Size(1,28); style.thumb_metrics.radius=0;
    r.SetCustomStyle(style); Image slim=render();
    Point centre=r.GetGeometry(r.GetSize()).boundary_thumbs[0].CenterPoint();
    t.Expect(slim[centre.y][centre.x].r<15,"a one-pixel frameless handle still paints its authored face");
    style.thumb_size=Size(INT_MAX,INT_MAX); style.track_size=Size(INT_MAX,INT_MAX);
    style.thumb_hover_growth=INT_MAX; style.label_padding=INT_MAX;
    r.SetCustomStyle(style).SetActiveBoundary(0).SetRect(0,0,17,19);
    auto g=r.GetGeometry(r.GetSize());
    t.Expect(g.track.IsEmpty() || Rect(r.GetSize()).Contains(g.track),"oversized authored geometry stays inside a tiny control");
    for(const auto& handle:g.boundary_thumbs)
        t.Expect(Rect(r.GetSize()).Contains(handle),"oversized hot handles are clipped to available bounds");
    Size minimum=r.GetMinSize();
    t.Expect(minimum.cx>0 && minimum.cy>0,"oversized minimum dimensions saturate instead of integer wrapping");
}

CONSOLE_APP_MAIN
{
    TestCtx t;
    TestDataAndNormalization(t);
    TestBoundaryEditing(t);
    TestStructureMutation(t);
    TestDataBinding(t);
    TestGeometry(t);
    TestPaletteThemeAndOverrides(t);
    TestSelectionAndPaint(t);
    TestCardPresentation(t);
    TestPresentationPersistence(t);
    TestThumbRasterAndBounds(t);

    Cout() << "\nUIRANGESEGMENTS_SUMMARY checks=" << t.checks
           << " failed=" << t.fails << "\n";
    SetExitCode(t.fails ? 1 : 0);
}
