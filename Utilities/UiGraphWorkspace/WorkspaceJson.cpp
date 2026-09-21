#include "UiGraphWorkspace.h"
#include <cmath>
#include <filesystem>

namespace Upp {
namespace GraphWorkspace {
namespace {
constexpr int MAX_JSON = 8 * 1024 * 1024;
int Dip(int px) { return fround(px * 1024.0 / DPI(1024)); }

struct Reader {
    ValueMap map;
    Index<String> read;
    explicit Reader(const Value& v) {
        if(!v.Is<ValueMap>()) throw Exc("Expected object");
        map = v;
        for(int i = 0; i < map.GetCount(); i++) {
            if(!map.GetKey(i).Is<String>() || map.FindNext(i) >= 0) throw Exc("Invalid or duplicate object key");
        }
    }
    Value Get(const char* name) {
        int i = map.Find(name);
        if(i < 0) throw Exc(String("Missing field: ") + name);
        read.FindAdd(name);
        return map.GetValue(i);
    }
    int Int(const char* name, int low, int high) {
        Value v = Get(name);
        if(!v.Is<int>() && !v.Is<int64>() && !v.Is<double>()) throw Exc(String("Expected integer: ") + name);
        double n = (double)v;
        if(!std::isfinite(n) || n < low || n > high || floor(n) != n) throw Exc(String("Out of range: ") + name);
        return (int)n;
    }
    double Number(const char* name, double low, double high) {
        Value v = Get(name);
        if(!v.Is<int>() && !v.Is<int64>() && !v.Is<double>()) throw Exc("Expected number");
        double n = (double)v;
        if(!std::isfinite(n) || n < low || n > high) throw Exc("Number out of range");
        return n;
    }
    bool Bool(const char* name) {
        Value v = Get(name);
        if(!v.Is<bool>()) throw Exc(String("Expected Boolean: ") + name);
        return (bool)v;
    }
    String Str(const char* name, int maxlen = 8192) {
        Value v = Get(name);
        if(!v.Is<String>() || String(v).GetCount() > maxlen) throw Exc(String("Invalid string: ") + name);
        return v;
    }
    void Done() {
        if(read.GetCount() != map.GetCount()) throw Exc("Unknown field; refusing lossy import");
    }
};

void CheckLiteral(const Value& v, int depth = 0)
{
    if(depth > 8) throw Exc("Literal nesting limit");
    if(IsNull(v) || v.Is<bool>()) return;
    if(v.Is<String>()) { if(String(v).GetCount() > 8192) throw Exc("Literal too long"); return; }
    if(v.Is<int>() || v.Is<int64>() || v.Is<double>()) {
        if(!std::isfinite((double)v)) throw Exc("Non-finite literal");
        return;
    }
    if(v.Is<ValueArray>()) {
        ValueArray a = v;
        if(a.GetCount() > 32) throw Exc("Literal item limit is 32");
        for(const Value& x : a) CheckLiteral(x, depth + 1);
        return;
    }
    if(v.Is<ValueMap>()) {
        Reader r(v);
        if(r.map.GetCount() > 32) throw Exc("Literal item limit is 32");
        for(int i = 0; i < r.map.GetCount(); i++) {
            if(String(r.map.GetKey(i)).GetCount() > 256) throw Exc("Literal key too long");
            CheckLiteral(r.map.GetValue(i), depth + 1);
        }
        return;
    }
    throw Exc("Literal type is not portable JSON");
}

Value ColorValue(Color c) { return IsNull(c) ? Value() : Value((c.GetR() << 16) | (c.GetG() << 8) | c.GetB()); }
Color ReadColor(const Value& v)
{
    if(IsNull(v)) return Null;
    if(!v.Is<int>() && !v.Is<int64>() && !v.Is<double>()) throw Exc("Expected RGB or null");
    double d = (double)v;
    if(!std::isfinite(d) || d < 0 || d > 0xffffff || floor(d) != d) throw Exc("Invalid RGB");
    int n = (int)d; return Color(n >> 16, (n >> 8) & 255, n & 255);
}
Value Colors(const Color* p) { ValueArray a; for(int i = 0; i < 4; i++) a.Add(ColorValue(p[i])); return a; }
void ReadColors(const Value& v, Color* p)
{
    if(!v.Is<ValueArray>()) throw Exc("Expected four state colours");
    ValueArray a = v;
    if(a.GetCount() != 4) throw Exc("Expected four state colours");
    for(int i = 0; i < 4; i++) p[i] = ReadColor(a[i]);
}

Value ImageValue(const Image& image)
{
    if(IsNull(image)) return Value();
    if(image.GetWidth() > 256 || image.GetHeight() > 256) throw Exc("Static asset exceeds 256x256; import a thumbnail");
    String bytes;
    const char hex[] = "0123456789abcdef";
    for(int y = 0; y < image.GetHeight(); y++) for(int x = 0; x < image.GetWidth(); x++) {
        RGBA p = image[y][x]; const byte channels[] = {p.r, p.g, p.b, p.a};
        for(byte c : channels) { bytes.Cat(hex[c >> 4]); bytes.Cat(hex[c & 15]); }
    }
    return ValueMap()("width", image.GetWidth())("height", image.GetHeight())("rgba", bytes);
}
Image ReadImage(const Value& v)
{
    if(IsNull(v)) return Image();
    Reader r(v); int w = r.Int("width", 1, 256), h = r.Int("height", 1, 256);
    String s = r.Str("rgba", 256 * 256 * 8); r.Done();
    if(s.GetCount() != w * h * 8) throw Exc("Invalid asset byte count");
    auto nibble = [](int c) -> int {
        if(c >= '0' && c <= '9') return c - '0';
        if(c >= 'a' && c <= 'f') return c - 'a' + 10;
        throw Exc("Invalid asset hex");
    };
    ImageBuffer b(w, h); b.SetKind(IMAGE_ALPHA);
    int n = 0;
    for(int y = 0; y < h; y++) for(int x = 0; x < w; x++) {
        byte c[4]; for(int i = 0; i < 4; i++) { c[i] = byte(nibble(s[n]) * 16 + nibble(s[n + 1])); n += 2; }
        if(c[0] > c[3] || c[1] > c[3] || c[2] > c[3]) throw Exc("Asset is not premultiplied RGBA");
        RGBA& p = b[y][x]; p.r = c[0]; p.g = c[1]; p.b = c[2]; p.a = c[3];
    }
    return Image(b);
}

Value EncodeLayout(const UiGraphNodeTemplate& t)
{
    ValueMap m;
    m.Add("kind", (int)t.kind); m.Add("body_mode", (int)t.body_mode); m.Add("text_align", (int)t.text_align);
    m.Add("header", t.header_height < 0 ? -1 : Dip(t.header_height)); m.Add("footer", Dip(t.footer_height));
    ValueArray columns; columns.Add(Dip(t.content_left_width)); columns.Add(Dip(t.content_right_width));
    columns.Add(Dip(t.overlay_left_width)); columns.Add(Dip(t.overlay_right_width)); m.Add("columns", columns);
    m.Add("body_ports_left", t.left_port_lane_body_only); m.Add("body_ports_right", t.right_port_lane_body_only);
    m.Add("ellipse_bands", t.ellipse_bands);
    m.Add("ellipse_band_width_percent", t.ellipse_band_width_percent);
    m.Add("width_policy", t.lod_widths.enabled);
    ValueArray thresholds; thresholds.Add(t.lod_widths.normal); thresholds.Add(t.lod_widths.lod1); thresholds.Add(t.lod_widths.lod2);
    m.Add("thresholds", thresholds); m.Add("micro_hints", t.micro_hints); m.Add("micro_budget", t.micro_hint_budget);
    ValueArray slots;
    for(int i = 0; i < t.slot_count; i++) {
        const auto& r = t.slots[i]; ValueMap a;
#define FIELD(x) a.Add(#x, r.x)
#define ENUM(x) a.Add(#x, (int)r.x)
#define METRIC(x) a.Add(#x, r.x < 0 ? -1 : Dip(r.x))
        FIELD(id); FIELD(label); FIELD(data_key); FIELD(overview_data_key); FIELD(use_literal);
        CheckLiteral(r.literal); FIELD(literal);
        ENUM(feature); ENUM(component_kind); ENUM(region); ENUM(placement); ENUM(flow);
        ENUM(align_h); ENUM(align_v); ENUM(small); ENUM(overflow); ENUM(image_fit); ENUM(icon_mode);
        ENUM(lod_mask); ENUM(force_on); ENUM(force_off);
        METRIC(extent); METRIC(gap_after); METRIC(font_height); FIELD(readable_min_px); FIELD(max_items);
        a.Add("preferred_width", Dip(r.preferred_size.cx)); a.Add("preferred_height", Dip(r.preferred_size.cy));
        a.Add("ink", ColorValue(r.ink)); a.Add("asset", ImageValue(r.asset));
#undef FIELD
#undef ENUM
#undef METRIC
        const auto& s = r.component_style; ValueMap style;
        style.Add("role", (int)s.role); style.Add("font_face", s.font_face);
        style.Add("bold", s.bold); style.Add("italic", s.italic); style.Add("underline", s.underline);
        style.Add("padding", Dip(s.padding)); style.Add("frame_width", Dip(s.frame_width)); style.Add("radius", Dip(s.radius));
        style.Add("ink", Colors(s.ink)); style.Add("face", Colors(s.face)); style.Add("frame", Colors(s.frame));
        a.Add("style", style); slots.Add(a);
    }
    m.Add("slots", slots); return m;
}

ValueArray ArrayOf(const Value& v, int count)
{
    if(!v.Is<ValueArray>()) throw Exc("Expected array");
    ValueArray a = v;
    if(a.GetCount() != count) throw Exc("Invalid array length");
    return a;
}
int ArrayInt(const Value& v, int low, int high)
{
    Reader r(ValueMap()("v", v)); return r.Int("v", low, high);
}
UiGraphNodeTemplate ReadLayout(const Value& v, int version)
{
    Reader m(v); UiGraphNodeTemplate t;
    t.kind = (UiGraphNodeTemplateKind)m.Int("kind", 0, 7);
    t.body_mode = (UiGraphNodeBodyMode)m.Int("body_mode", 0, 6);
    t.text_align = (UiAlign)m.Int("text_align", 0, 255);
    int header = m.Int("header", -1, 16384); t.header_height = header < 0 ? -1 : DPI(header);
    t.footer_height = DPI(m.Int("footer", 0, 16384));
    auto columns = ArrayOf(m.Get("columns"), 4);
    t.SetContentColumns(DPI(ArrayInt(columns[0], 0, 16384)), DPI(ArrayInt(columns[1], 0, 16384)));
    t.SetOverlayColumns(DPI(ArrayInt(columns[2], 0, 16384)), DPI(ArrayInt(columns[3], 0, 16384)));
    t.left_port_lane_body_only = m.Bool("body_ports_left"); t.right_port_lane_body_only = m.Bool("body_ports_right");
    // Version 1 predates bands. Preserve its geometry; do not apply new-family
    // defaults while importing an existing authoring document. Version 2 is strict.
    if(version >= 2) {
        t.ellipse_bands = m.Bool("ellipse_bands");
        t.ellipse_band_width_percent = m.Int("ellipse_band_width_percent", 20, 100);
    }
    t.lod_widths.enabled = m.Bool("width_policy");
    auto widths = ArrayOf(m.Get("thresholds"), 3);
    t.lod_widths.normal = ArrayInt(widths[0], 1, 16384); t.lod_widths.lod1 = ArrayInt(widths[1], 1, 16384); t.lod_widths.lod2 = ArrayInt(widths[2], 1, 16384);
    t.micro_hints = m.Bool("micro_hints"); t.micro_hint_budget = m.Int("micro_budget", 0, 16);
    Value list = m.Get("slots"); m.Done();
    if(!list.Is<ValueArray>()) throw Exc("Expected slots");
    ValueArray slots = list;
    if(slots.GetCount() > t.MAX_SLOTS) throw Exc("Slot capacity exceeded");
    for(const Value& value : slots) {
        Reader a(value); UiGraphNodeSlotRule r;
        r.id = a.Str("id", 128); r.label = a.Str("label", 256); r.data_key = a.Str("data_key", 256);
        r.overview_data_key = a.Str("overview_data_key", 256); r.use_literal = a.Bool("use_literal");
        r.literal = a.Get("literal"); CheckLiteral(r.literal);
#define ENUM(x, type, hi) r.x = (type)a.Int(#x, 0, hi)
        ENUM(feature, UiGraphNodeSlotFeature, 7); ENUM(component_kind, UiGraphNodeComponentKind, 7);
        ENUM(region, UiGraphNodeSlotRegion, 7); ENUM(placement, UiGraphNodeSlotPlacement, 5);
        ENUM(flow, UiGraphNodeSlotFlow, 1); ENUM(align_h, UiAlign, 255); ENUM(align_v, UiAlign, 255);
        ENUM(small, UiGraphNodeSmallMode, 3); ENUM(overflow, UiGraphNodeOverflow, 2);
        ENUM(image_fit, UiGraphNodeImageFit, 1); ENUM(icon_mode, UiIconRenderMode, 2);
        ENUM(lod_mask, byte, 15); ENUM(force_on, byte, 15); ENUM(force_off, byte, 15);
#undef ENUM
        r.extent = DPI(a.Int("extent", 0, 16384));
        int gap = a.Int("gap_after", -1, 16384); r.gap_after = gap < 0 ? -1 : DPI(gap);
        r.font_height = DPI(a.Int("font_height", 0, 512)); r.readable_min_px = a.Int("readable_min_px", 1, 128);
        r.max_items = a.Int("max_items", 1, 12);
        r.preferred_size = Size(DPI(a.Int("preferred_width", 0, 16384)), DPI(a.Int("preferred_height", 0, 16384)));
        r.ink = ReadColor(a.Get("ink")); r.asset = ReadImage(a.Get("asset"));
        Reader s(a.Get("style")); a.Done(); auto& cs = r.component_style;
        cs.role = (UiGraphNodeComponentRole)s.Int("role", 0, 4); cs.font_face = s.Str("font_face", 256);
        cs.bold = s.Int("bold", -1, 1); cs.italic = s.Int("italic", -1, 1); cs.underline = s.Int("underline", -1, 1);
        cs.padding = DPI(s.Int("padding", 0, 512)); cs.frame_width = DPI(s.Int("frame_width", 0, 32)); cs.radius = DPI(s.Int("radius", 0, 512));
        ReadColors(s.Get("ink"), cs.ink); ReadColors(s.Get("face"), cs.face); ReadColors(s.Get("frame"), cs.frame); s.Done();
        String error; if(!t.AddComponent(r, error)) throw Exc(error);
    }
    if(t.text_align != UiAlign::LEFT && t.text_align != UiAlign::CENTER && t.text_align != UiAlign::RIGHT) throw Exc("Invalid template alignment");
    String error; if(!t.Validate(error)) throw Exc(error);
    return t;
}

Value EncodeStyle(const Appearance& s)
{
    return ValueMap()("face", Colors(s.face))("frame", Colors(s.frame))("ink", Colors(s.ink))("header", Colors(s.header))
        ("role", s.role)("radius", s.radius)("frame_width", s.frame_width)("header_band", s.header_band)
        ("shadow", s.shadow)("shadow_y", s.shadow_y)("shadow_alpha", s.shadow_alpha)("shadow_color", ColorValue(s.shadow_color));
}
Appearance ReadStyle(const Value& v)
{
    Reader r(v); Appearance s;
    ReadColors(r.Get("face"), s.face); ReadColors(r.Get("frame"), s.frame); ReadColors(r.Get("ink"), s.ink); ReadColors(r.Get("header"), s.header);
    s.role = r.Int("role", 0, 3); s.radius = r.Int("radius", -1, 512); s.frame_width = r.Int("frame_width", -1, 32);
    s.header_band = r.Int("header_band", -1, 1); s.shadow = r.Int("shadow", -1, 1); s.shadow_y = r.Int("shadow_y", -1, 128);
    s.shadow_alpha = r.Int("shadow_alpha", -1, 255); s.shadow_color = ReadColor(r.Get("shadow_color")); r.Done(); return s;
}
} // namespace

Value Encode(const Document& d)
{
    String error; if(!Validate(d, error)) throw Exc(error);
    CheckLiteral(d.data);
    ValueArray variants;
    for(int i = 0; i < SHAPE_COUNT; i++) {
        ValueMap v;
        v.Add("layout", d.family.layout_override[i] ? EncodeLayout(d.family.shape_layout[i]) : Value());
        v.Add("style", d.family.style_override[i] ? EncodeStyle(d.family.shape_style[i]) : Value());
        variants.Add(v);
    }
    ValueMap family;
    family.Add("name", d.family.name); family.Add("layout", EncodeLayout(d.family.base_layout));
    family.Add("style", EncodeStyle(d.family.base_style)); family.Add("shapes", variants);
    ValueMap preview;
    preview.Add("shape", d.shape); preview.Add("edit_base", d.edit_base);
    preview.Add("title", d.title); preview.Add("subtitle", d.subtitle); preview.Add("description", d.description); preview.Add("data", d.data);
    preview.Add("inputs", d.inputs); preview.Add("outputs", d.outputs); preview.Add("connector", d.connector);
    preview.Add("width", d.size.cx); preview.Add("height", d.size.cy); preview.Add("zoom", d.zoom);
    preview.Add("pan_x", d.pan.x); preview.Add("pan_y", d.pan.y);
    return ValueMap()("schema", "uigraph.workspace")("version", 2)("units", "logical96")("family", family)("preview", preview);
}

bool Decode(const Value& v, Document& output, String& error)
{
    try {
        Reader root(v);
        if(root.Str("schema") != "uigraph.workspace") throw Exc("Unsupported workspace schema");
        int version = root.Int("version", 1, 2);
        if(root.Str("units") != "logical96") throw Exc("Unsupported workspace units");
        Document d; Reader f(root.Get("family"));
        d.family.name = f.Str("name", 128); d.family.base_layout = ReadLayout(f.Get("layout"), version); d.family.base_style = ReadStyle(f.Get("style"));
        auto variants = ArrayOf(f.Get("shapes"), SHAPE_COUNT); f.Done();
        for(int i = 0; i < SHAPE_COUNT; i++) {
            Reader s(variants[i]); Value layout = s.Get("layout"), style = s.Get("style"); s.Done();
            if(!IsNull(layout)) { d.family.shape_layout[i] = ReadLayout(layout, version); d.family.layout_override[i] = true; }
            if(!IsNull(style)) { d.family.shape_style[i] = ReadStyle(style); d.family.style_override[i] = true; }
        }
        Reader p(root.Get("preview")); root.Done();
        d.shape = p.Int("shape", 0, 7); d.edit_base = p.Bool("edit_base");
        d.title = p.Str("title"); d.subtitle = p.Str("subtitle"); d.description = p.Str("description");
        Value data = p.Get("data"); CheckLiteral(data); if(!data.Is<ValueMap>()) throw Exc("Preview data must be an object"); d.data = data;
        d.inputs = p.Int("inputs", 0, 32); d.outputs = p.Int("outputs", 0, 32); d.connector = p.Int("connector", 0, 2);
        d.size = Size(p.Int("width", 32, 2048), p.Int("height", 24, 2048));
        d.zoom = p.Number("zoom", 0.01, 8); d.pan = Pointf(p.Number("pan_x", -1000000, 1000000), p.Number("pan_y", -1000000, 1000000)); p.Done();
        if(!Validate(d, error)) return false;
        d.revision = output.revision + 1;
        output = d; error.Clear(); return true;
    }
    catch(const Exc& e) { error = e; return false; }
}

bool Load(const String& text, Document& output, String& error)
{
    if(text.GetCount() > MAX_JSON) { error = "Workspace exceeds 8 MiB"; return false; }
    int depth = 0; bool quote = false, escape = false;
    for(char c : text) {
        if(quote) { if(escape) escape = false; else if(c == '\\') escape = true; else if(c == '"') quote = false; }
        else if(c == '"') quote = true;
        else if(c == '{' || c == '[') { if(++depth > 32) { error = "JSON nesting limit"; return false; } }
        else if(c == '}' || c == ']') depth--;
    }
    try { return Decode(ParseJSON(text), output, error); }
    catch(const Exc& e) { error = e; return false; }
}

bool SaveAtomic(const String& path, const String& text, String& error)
{
    if(path.IsEmpty() || text.GetCount() > MAX_JSON) { error = "Invalid destination or file exceeds 8 MiB"; return false; }
    String temp = path + ".tmp-" + AsString(Random()) + "-" + AsString(Random());
    if(FileExists(temp) || !SaveFile(temp, text)) { error = "Could not write temporary file"; return false; }
    bool ok = false;
    try {
        auto from = std::filesystem::u8path(~temp), to = std::filesystem::u8path(~path);
#ifdef PLATFORM_WIN32
        ok = ::MoveFileExW(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
        std::error_code ec; std::filesystem::rename(from, to, ec); ok = !ec;
#endif
    }
    catch(const std::exception&) { ok = false; }
    if(!ok) { FileDelete(temp); error = "Atomic replacement failed; previous file preserved"; return false; }
    error.Clear(); return true;
}
} // namespace GraphWorkspace
} // namespace Upp
