#include <Ui/Ui.h>

using namespace Upp;
namespace {
struct AccentTests {
    int checks = 0, failed = 0;
    void Check(bool value, const char* message) {
        checks++;
        if(!value) { failed++; Cout() << "[FAIL] " << message << '\n'; }
    }
};
Color AccentColor() { return Color(40, 120, 220); }
StyledPalette AccentPalette() {
    StyledPalette palette;
    for(int i = 0; i < 4; i++) {
        palette.face[i] = UiFill::Solid(White());
        palette.frame[i] = Color(80, 80, 80);
    }
    return palette;
}
Image RenderAccent(StyledMetrics metrics, Size size = Size(72, 52), bool base = false) {
    ImageDraw draw(size);
    draw.DrawRect(size, Color(235, 230, 225));
    if(base) UiPaintFaceFrameDashBase(draw, Rect(size), AccentPalette(), metrics, ST_NORMAL);
    else UiPaintFaceFrameDash(draw, Rect(size), AccentPalette(), metrics, ST_NORMAL);
    return draw;
}
bool SamePixel(const Image& a, const Image& b, int x, int y) {
    const RGBA& p = a[y][x]; const RGBA& q = b[y][x];
    return p.r == q.r && p.g == q.g && p.b == q.b && p.a == q.a;
}
bool SameImage(const Image& a, const Image& b) {
    if(a.GetSize() != b.GetSize()) return false;
    for(int y = 0; y < a.GetHeight(); y++) for(int x = 0; x < a.GetWidth(); x++)
        if(!SamePixel(a, b, x, y)) return false;
    return true;
}
bool IsAccent(const Image& image, int x, int y) {
    const RGBA& p = image[y][x];
    return p.r == 40 && p.g == 120 && p.b == 220;
}
void SerializeLegacyMetrics(Stream& s, StyledMetrics& m) {
    s % m.text_font % m.use_text_font % m.content_margin % m.radius % m.frame_width
      % m.frame_enabled % m.face_enabled % m.dashed % m.dash_pattern
      % m.high_contrast % m.focus_enabled % m.focus_margin % m.focus_alpha % m.focus_color
      % m.shadow % m.highlight;
}
void CheckPixels(AccentTests& t) {
    StyledMetrics m; m.radius = 0; m.frame_width = 1;
    m.frame_accent.color = AccentColor(); m.frame_accent.thickness = 3;
    for(int radius : {0, 12, 20}) for(int edges = 0; edges <= StyledFrameAccent::All; edges++) {
        m.radius = radius;
        m.frame_accent.edges = edges;
        Image image = RenderAccent(m);
        t.Check(IsAccent(image,36,1) == bool(edges & StyledFrameAccent::Top), "top mask");
        t.Check(IsAccent(image,36,50) == bool(edges & StyledFrameAccent::Bottom), "bottom mask");
        t.Check(IsAccent(image,1,26) == bool(edges & StyledFrameAccent::Left), "left mask");
        t.Check(IsAccent(image,70,26) == bool(edges & StyledFrameAccent::Right), "right mask");
        t.Check(image[0][36].r == 80, "original frame remains visible");
        t.Check(image[26][36].r == 255, "accent leaves the face centre alone");
        t.Check(SameImage(image, RenderAccent(m, Size(72,52), true)), "base/facade parity");
    }
    for(int edges=0;edges<=StyledFrameAccent::All;edges++) {
        m.radius=20; m.frame_accent.edges=edges;
        Image image=RenderAccent(m,Size(190,175));
        t.Check(IsAccent(image,95,1)==bool(edges & StyledFrameAccent::Top), "large tiled top mask");
        t.Check(IsAccent(image,95,173)==bool(edges & StyledFrameAccent::Bottom), "large tiled bottom mask");
        t.Check(IsAccent(image,1,87)==bool(edges & StyledFrameAccent::Left), "large tiled left mask");
        t.Check(IsAccent(image,188,87)==bool(edges & StyledFrameAccent::Right), "large tiled right mask");
    }
    m.radius = 12; m.frame_accent.edges = StyledFrameAccent::Top;
    Image rounded = RenderAccent(m);
    t.Check(rounded[0][0].r == 235, "rounded accent stays inside silhouette");
    t.Check(IsAccent(rounded,36,1), "rounded top thickness begins inside normal frame");
    t.Check(!IsAccent(rounded,1,26), "top accent does not decorate left straight edge");
    m.frame_accent.alpha = 128;
    m.frame_accent.edges = StyledFrameAccent::Top | StyledFrameAccent::Left;
    Image adjacent = RenderAccent(m);
    m.frame_accent.edges = StyledFrameAccent::All;
    Image all = RenderAccent(m);
    bool seam = true;
    for(int y=0;y<16;y++) for(int x=0;x<16;x++) seam &= SamePixel(adjacent, all, x, y);
    t.Check(seam, "adjacent translucent edges join without a doubled corner seam");
    Image tiled_all=RenderAccent(m,Size(190,175));
    for(int pair : {StyledFrameAccent::Top | StyledFrameAccent::Left,
                    StyledFrameAccent::Top | StyledFrameAccent::Right,
                    StyledFrameAccent::Bottom | StyledFrameAccent::Left,
                    StyledFrameAccent::Bottom | StyledFrameAccent::Right}) {
        m.frame_accent.edges=pair;
        Image tiled_pair=RenderAccent(m,Size(190,175));
        int x0=pair & StyledFrameAccent::Left ? 0 : 170;
        int y0=pair & StyledFrameAccent::Top ? 0 : 155;
        bool equal=true;
        for(int y=y0;y<y0+20;y++) for(int x=x0;x<x0+20;x++)
            equal &= SamePixel(tiled_pair,tiled_all,x,y);
        t.Check(equal, "all translucent rounded corner pairs match the full tiled ring");
    }
    t.Check(all[1][36].r > 40 && all[1][36].r < 255, "true accent opacity composites over the face");
    m.frame_enabled = false; m.face_enabled = false;
    m.frame_accent.edges = StyledFrameAccent::All;
    m.frame_accent.alpha = 255; m.frame_accent.color = AccentColor();
    Image accent_only = RenderAccent(m);
    t.Check(IsAccent(accent_only,36,1), "accent works independently of face/frame enable flags");
    m.frame_enabled = true; m.face_enabled = true;
    m.frame_accent.color = Null;
    Image inherited = RenderAccent(m);
    t.Check(inherited[1][36].r == 80, "Null accent color follows state frame color");
    m.frame_accent.alpha = 0;
    Image off = RenderAccent(m); m.frame_accent.edges = 0;
    t.Check(SameImage(off, RenderAccent(m)), "zero opacity is visually off");
    m.frame_accent.edges = StyledFrameAccent::All; m.frame_accent.alpha = 255;
    m.frame_accent.thickness = 0; off = RenderAccent(m); m.frame_accent.edges = 0;
    t.Check(SameImage(off, RenderAccent(m)), "zero thickness is visually off");
}
void CheckFading(AccentTests& t) {
    StyledMetrics m; m.radius=12; m.frame_accent.edges=StyledFrameAccent::Top;
    m.frame_accent.color=AccentColor(); m.frame_accent.thickness=3; m.frame_accent.alpha=128;
    auto render=[&](int alpha) {
        ImageDraw draw(Size(72,52)); draw.DrawRect(Size(72,52),White());
        UiPaintFaceFrameDashAlpha(draw,RectC(0,0,72,52),AccentPalette(),m,ST_NORMAL,alpha);
        return Image(draw);
    };
    Image faded=render(128), full=render(255);
    t.Check(faded[1][36].b > faded[1][36].r && faded[1][36].r > full[1][36].r,
            "faded surfaces retain accent with multiplied opacity");
    ImageDraw expected(Size(72,52)); expected.DrawRect(Size(72,52),White());
    UiPaintFaceFrameDash(expected,RectC(0,0,72,52),AccentPalette(),m,ST_NORMAL);
    t.Check(SameImage(full,Image(expected)), "full-opacity wrapper matches facade");
    m.face_enabled=false; m.frame_enabled=false;
    faded=render(128);
    t.Check(faded[1][36].b > faded[1][36].r && faded[26][36].r==255,
            "accent-only surfaces retain decoration during fade");
    ImageDraw clear(Size(72,52)); clear.DrawRect(Size(72,52),White());
    t.Check(SameImage(render(0),Image(clear)), "zero overall opacity hides accent");
}
void CheckStreams(AccentTests& t) {
    StyledMetrics m; m.radius=17; m.frame_width=3;
    Event<Stream&> legacy_writer = [&](Stream& stream) { SerializeLegacyMetrics(stream,m); };
    String legacy = StoreAsString(legacy_writer);
    t.Check(StoreAsString(m)==legacy, "default metrics preserve legacy bytes");
    StyledMetrics restored; restored.frame_accent.edges=StyledFrameAccent::All;
    t.Check(LoadFromString(restored,legacy), "legacy metrics load without seeking");
    t.Check(restored.radius==17 && restored.frame_accent.IsDefault(), "legacy load clears reused accent state");
    m.frame_accent.edges=StyledFrameAccent::Top|StyledFrameAccent::Right;
    m.frame_accent.color=AccentColor(); m.frame_accent.thickness=5; m.frame_accent.alpha=137;
    String encoded=StoreAsString(m);
    StringStream raw; raw.SetStoring(); m.Serialize(raw);
    t.Check((byte)raw.GetResult()[0]==250 && (byte)raw.GetResult()[1]==1, "authored stream is versioned");
    t.Check(LoadFromString(restored,encoded), "authored metrics load");
    t.Check(restored.frame_accent.edges==9 && restored.frame_accent.thickness==5 &&
            restored.frame_accent.color==AccentColor() && restored.frame_accent.alpha==137,
            "all authored accent fields round-trip");
    String invalid=raw.GetResult(); invalid.Set(1,2);
    StringStream bad(invalid); bad.SetLoading(); restored.Serialize(bad);
    t.Check(bad.IsError(), "unknown extended version fails instead of misaligning fields");
    m.frame_accent.edges=255; m.frame_accent.thickness=-3; m.frame_accent.alpha=999;
    t.Check(LoadFromString(restored,StoreAsString(m)), "invalid authored metrics load for normalization");
    t.Check(restored.frame_accent.edges==15 && restored.frame_accent.thickness==0 &&
            restored.frame_accent.alpha==255, "accent import validates masks and numeric limits");
    UiGroupPanel::Style source=UiGroupPanel::StyleDefault(); source.metrics.frame_accent=m.frame_accent;
    source.header_gap=23; source.header_mode=UiGroupPanel::Center;
    UiGroupPanel::Style group;
    t.Check(LoadFromString(group,StoreAsString(source)) && group.header_gap==23 &&
            group.header_mode==UiGroupPanel::Center && group.metrics.frame_accent.edges==15,
            "containing style fields stay aligned after extended metrics");
}
void CheckBoundsAndCache(AccentTests& t) {
    UiRasterCache::Clear();
    StyledMetrics m; m.radius=20; m.frame_accent.edges=StyledFrameAccent::All;
    m.frame_accent.color=AccentColor(); m.frame_accent.thickness=3;
    Image first=RenderAccent(m,Size(950,620));
    auto before=UiRasterCache::GetStats();
    Image second=RenderAccent(m,Size(950,620));
    auto after=UiRasterCache::GetStats();
    t.Check(SameImage(first,second), "large surface repeats exactly across tile boundaries");
    t.Check(after.hits>before.hits && after.insertions==before.insertions,
            "unchanged accent reuses exact bounded tiles");
    m.frame_accent.color=Color(220,40,50);
    t.Check(!SameImage(first,RenderAccent(m,Size(950,620))), "color changes invalidate decoration cache");
    UiRasterCache::Clear();
    m.radius=1024; m.frame_accent.thickness=2;
    auto large_before=UiRasterCache::GetStats();
    Image large=RenderAccent(m,Size(2048,2048));
    auto large_stats=UiRasterCache::GetStats();
    t.Check(large_stats.insertions-large_before.insertions<130,
            "large-radius accents cull transparent interior tiles");
    t.Check(large[1024][1024].r==255, "large-radius culling preserves empty centre");
    for(int width : {1,2,5,9}) for(int height : {1,3,8}) for(int radius : {0,3,60,INT_MAX})
    for(int frame : {0,1,8}) for(int thick : {0,1,8,INT_MAX}) {
        m.radius=radius; m.frame_width=frame; m.frame_accent.thickness=thick;
        t.Check(RenderAccent(m,Size(width,height)).GetSize()==Size(width,height), "tiny/oversized geometry stays bounded");
    }
}
void CheckPreparedTags(AccentTests& t) {
    UiTagData data("READY",UiRole::Accent);
    UiTagStyle style=UiResolveTagStyle(UiRole::Accent);
    style.metrics.frame_accent.edges=StyledFrameAccent::Top;
    style.metrics.frame_accent.color=AccentColor(); style.metrics.frame_accent.thickness=3;
    auto draw=[&](const UiTagStyle& recipe) {
        auto prepared=UiPrepareTag(data,recipe,RectC(0,0,120,40));
        ImageDraw canvas(Size(120,40)); canvas.DrawRect(Size(120,40),White());
        UiPaintTag(canvas,prepared,ST_NORMAL); return Image(canvas);
    };
    Image top=draw(style);
    t.Check(IsAccent(top,60,1), "prepared tag decoration consumes Frame Accent before Paint");
    style.metrics.frame_accent.edges=StyledFrameAccent::Bottom;
    Image bottom=draw(style);
    t.Check(!SameImage(top,bottom) && IsAccent(bottom,60,38), "prepared tag cache includes accent sides");
    style.metrics.frame_accent.alpha=90;
    t.Check(!SameImage(bottom,draw(style)), "prepared tag cache includes accent opacity");
    style.metrics.frame_accent.color=Color(220,40,50);
    t.Check(!SameImage(bottom,draw(style)), "prepared tag cache includes accent color");
}
void CheckControls(AccentTests& t) {
    UiPanel panel; UiGroupPanel group; UiScrollPanel scroll; UiButton button;
    Size a=panel.GetMinSize(), b=group.GetMinSize(), c=scroll.GetMinSize(), d=button.GetMinSize();
    panel.SetFrameAccent(StyledFrameAccent::Top,4,AccentColor());
    group.SetFrameAccent(StyledFrameAccent::Bottom,4,AccentColor());
    scroll.SetFrameAccent(StyledFrameAccent::Left,4,AccentColor());
    button.SetFrameAccent(StyledFrameAccent::Right,4,AccentColor());
    t.Check(a==panel.GetMinSize() && b==group.GetMinSize() && c==scroll.GetMinSize() && d==button.GetMinSize(),
            "accent never adds layout insets to panels or ordinary controls");
    t.Check(panel.HasCustomStyle() && panel.GetStyle().metrics.frame_accent.edges==1,
            "convenience API creates an explicit style snapshot");
    panel.ClearFrameAccent(); t.Check(panel.GetStyle().metrics.frame_accent.IsDefault(), "clear accent resets its settings");
    panel.ClearCustomStyle(); t.Check(!panel.HasCustomStyle(), "clear custom style restores inheritance");
    panel.SetFrameAccent(255,-5,Null,999);
    t.Check(panel.GetStyle().metrics.frame_accent.edges==15 && panel.GetStyle().metrics.frame_accent.thickness==0 &&
            panel.GetStyle().metrics.frame_accent.alpha==255, "public setter normalizes authored values");
    for(UiAlign placement : {UiAlign::TOP, UiAlign::BOTTOM, UiAlign::LEFT, UiAlign::RIGHT}) {
        UiGroupPanel centred; UiButton header;
        header.SetText("Tools");
        auto style=UiGroupPanel::StyleDefault();
        style.metrics.radius=12; style.metrics.frame_enabled=false;
        style.metrics.frame_accent.edges=StyledFrameAccent::All;
        style.metrics.frame_accent.color=AccentColor(); style.metrics.frame_accent.thickness=INT_MAX;
        centred.SetCustomStyle(style).SetTitle("Header").SetHeaderMode(UiGroupPanel::Center)
               .SetHeaderPlacement(placement).SetHeaderContent(header);
        centred.SetRect(0,0,280,160); centred.Layout();
        ImageDraw canvas(Size(280,160)); canvas.DrawRect(Size(280,160),White()); centred.Paint(canvas);
        Image image=canvas; Rect occupied=centred.GetHeaderContentRect();
        bool clear=true; int count=0;
        for(int y=0;y<160;y++) for(int x=0;x<280;x++) if(IsAccent(image,x,y)) {
            count++; if(occupied.Contains(Point(x,y))) clear=false;
        }
        t.Check(clear && count>20, "GroupPanel accent preserves centred header gaps with normal frame off");
    }
    UiRangeSegments segments;
    auto segment_style=UiRangeSegments::StyleDefault();
    segment_style.track_metrics.frame_accent.edges=StyledFrameAccent::Top;
    segment_style.track_metrics.frame_accent.color=AccentColor();
    segment_style.track_metrics.frame_accent.thickness=3;
    segments.SetCustomStyle(segment_style); segments.SetRect(0,0,320,80);
    Vector<UiRangeSegment> rows;
    rows.Add(UiRangeSegment(1,"A",Red())); rows.Add(UiRangeSegment(1,"B",Green()));
    segments.SetSegments(rows);
    ImageDraw segment_draw(Size(320,80));
    segment_draw.DrawRect(Size(320,80),White()); segments.Paint(segment_draw);
    Image segment_image=segment_draw; int accent_pixels=0;
    for(int y=0;y<80;y++) for(int x=0;x<320;x++) accent_pixels+=IsAccent(segment_image,x,y);
    t.Check(accent_pixels>100, "segment content cannot cover the track accent");
    // A scrolled content origin must not move the viewport's decoration.
    UiButton child; scroll.SetRect(0,0,140,90); scroll.Content().Add(child); child.SetRect(0,0,500,500);
    scroll.Layout(); ImageDraw draw1(Size(140,90)); draw1.DrawRect(Size(140,90),White()); scroll.Paint(draw1);
    scroll.SetScrollPos(Point(70,80));
    ImageDraw draw2(Size(140,90)); draw2.DrawRect(Size(140,90),White()); scroll.Paint(draw2);
    t.Check(SameImage(draw1,draw2), "ScrollPanel accent stays on the fixed viewport while content scrolls");
}
}
int RunFrameAccentSuite() {
    AccentTests t;
    CheckPixels(t); CheckFading(t); CheckStreams(t); CheckBoundsAndCache(t); CheckControls(t); CheckPreparedTags(t);
    Cout()<<"UI_FRAME_ACCENT_SUMMARY checks="<<t.checks<<" failed="<<t.failed<<'\n';
    return t.failed ? 1 : 0;
}
