#include "UiGraphWorkspace.h"

namespace Upp {
namespace GraphWorkspace {
String QuoteCpp(const String& s)
{
    String out = "\"";
    for(int i = 0; i < s.GetCount(); i++) {
        byte c = (byte)s[i];
        if(c == '\\' || c == '"') { out.Cat('\\'); out.Cat(c); }
        else if(c < 32 || c >= 127) out << Format("\\%03o", (int)c);
        else out.Cat(c);
    }
    return out + "\"";
}
namespace {
String ColorCpp(const Value& v)
{
    if(IsNull(v)) return "Null";
    int n = (int)v;
    return Format("Color(%d, %d, %d)", (n >> 16) & 255, (n >> 8) & 255, n & 255);
}
String LiteralCpp(const Value& v)
{
    if(IsNull(v) && !v.Is<ValueArray>() && !v.Is<ValueMap>() && !v.Is<String>()) return "Value()";
    if(v.Is<String>()) return "Value(String(" + QuoteCpp(v) + "))";
    if(v.Is<bool>()) return (bool)v ? "Value(true)" : "Value(false)";
    if(v.Is<int64>()) return "Value(int64(" + AsString((int64)v) + "))";
    if(v.Is<int>()) return "Value(" + AsString((int)v) + ")";
    if(v.Is<double>()) return "Value(double(" + Format("%.17g", (double)v) + "))";
    String code;
    if(v.Is<ValueArray>()) {
        code = "[] { ValueArray a; "; ValueArray a = v;
        for(const auto& x : a) code << "a.Add(" << LiteralCpp(x) << "); ";
        return code + "return Value(a); }()";
    }
    code = "[] { ValueMap m; "; ValueMap m = v;
    for(int i = 0; i < m.GetCount(); i++) code << "m.Add(" << QuoteCpp(m.GetKey(i)) << ", " << LiteralCpp(m.GetValue(i)) << "); ";
    return code + "return Value(m); }()";
}
String ImageCpp(const Value& v)
{
    if(IsNull(v)) return "Image()";
    ValueMap m = v;
    String h = m["rgba"], result;
    result << "[] { ImageBuffer b(" << (int)m["width"] << ", " << (int)m["height"] << "); b.SetKind(IMAGE_ALPHA);\n        const char* hex =\n";
    for(int i = 0; i < h.GetCount(); i += 120) result << "            " << QuoteCpp(h.Mid(i, min(120, h.GetCount() - i))) << "\n";
    result << "        ; auto nib = [](char c) { return c <= '9' ? c - '0' : c - 'a' + 10; };\n"
           << "        auto read = [&] { byte c = byte(nib(hex[0]) * 16 + nib(hex[1])); hex += 2; return c; };\n"
           << "        for(int y = 0; y < b.GetHeight(); y++) for(int x = 0; x < b.GetWidth(); x++) {\n"
           << "            RGBA& p = b[y][x]; p.r = read(); p.g = read(); p.b = read(); p.a = read();\n"
           << "        } return Image(b); }()";
    return result;
}
String Metric(const Value& v) { return (int)v < 0 ? "-1" : "DPI(" + AsString((int)v) + ")"; }
void LayoutCode(String& out, const Value& value, const String& name)
{
    ValueMap m = value;
    out << "UiGraphNodeTemplate Make" << name << "Layout()\n{\n    UiGraphNodeTemplate t;\n    String error;\n";
    auto set = [&](const char* key, const char* member, const char* type) {
        out << "    t." << member << " = (" << type << ")" << AsString(m[key]) << ";\n";
    };
    set("kind", "kind", "UiGraphNodeTemplateKind"); set("body_mode", "body_mode", "UiGraphNodeBodyMode"); set("text_align", "text_align", "UiAlign");
    out << "    t.SetHeaderHeight(" << Metric(m["header"]) << ").SetFooterHeight(" << Metric(m["footer"]) << ");\n";
    ValueArray c = m["columns"], widths = m["thresholds"];
    out << "    t.SetContentColumns(" << Metric(c[0]) << ", " << Metric(c[1]) << ");\n"
        << "    t.SetOverlayColumns(" << Metric(c[2]) << ", " << Metric(c[3]) << ");\n"
        << "    t.SetBodyPortLanes(" << ((bool)m["body_ports_left"] ? "true" : "false") << ", " << ((bool)m["body_ports_right"] ? "true" : "false") << ");\n"
        << "    t.ellipse_bands = " << ((bool)m["ellipse_bands"] ? "true" : "false") << ";\n"
        << "    t.ellipse_band_width_percent = " << AsString(m["ellipse_band_width_percent"]) << ";\n"
        << "    t.SetLodWidths(" << AsString(widths[0]) << ", " << AsString(widths[1]) << ", " << AsString(widths[2]) << ");\n"
        << "    t.lod_widths.enabled = " << ((bool)m["width_policy"] ? "true" : "false") << ";\n"
        << "    t.micro_hints = " << ((bool)m["micro_hints"] ? "true" : "false") << ";\n"
        << "    t.micro_hint_budget = " << AsString(m["micro_budget"]) << ";\n";
    ValueArray slots = m["slots"];
    for(const Value& slot : slots) {
        ValueMap a = slot;
        out << "    {\n        UiGraphNodeSlotRule r;\n";
        for(const char* key : { "id", "label", "data_key", "overview_data_key" }) out << "        r." << key << " = " << QuoteCpp(a[key]) << ";\n";
        const char* keys[] = { "feature", "component_kind", "region", "placement", "flow", "align_h", "align_v", "small", "overflow", "image_fit", "icon_mode" };
        const char* types[] = { "UiGraphNodeSlotFeature", "UiGraphNodeComponentKind", "UiGraphNodeSlotRegion", "UiGraphNodeSlotPlacement", "UiGraphNodeSlotFlow", "UiAlign", "UiAlign", "UiGraphNodeSmallMode", "UiGraphNodeOverflow", "UiGraphNodeImageFit", "UiIconRenderMode" };
        for(int i = 0; i < 11; i++) out << "        r." << keys[i] << " = (" << types[i] << ")" << AsString(a[keys[i]]) << ";\n";
        for(const char* key : { "lod_mask", "force_on", "force_off", "readable_min_px", "max_items" }) out << "        r." << key << " = " << AsString(a[key]) << ";\n";
        for(const char* key : { "extent", "gap_after", "font_height" }) out << "        r." << key << " = " << Metric(a[key]) << ";\n";
        out << "        r.preferred_size = Size(" << Metric(a["preferred_width"]) << ", " << Metric(a["preferred_height"]) << ");\n"
            << "        r.use_literal = " << ((bool)a["use_literal"] ? "true" : "false") << ";\n"
            << "        r.literal = " << LiteralCpp(a["literal"]) << ";\n"
            << "        r.asset = " << ImageCpp(a["asset"]) << ";\n"
            << "        r.ink = " << ColorCpp(a["ink"]) << ";\n";
        ValueMap s = a["style"];
        out << "        r.component_style.role = (UiGraphNodeComponentRole)" << AsString(s["role"]) << ";\n"
            << "        r.component_style.font_face = " << QuoteCpp(s["font_face"]) << ";\n";
        for(const char* key : { "bold", "italic", "underline" }) out << "        r.component_style." << key << " = " << AsString(s[key]) << ";\n";
        for(const char* key : { "padding", "radius", "frame_width" }) out << "        r.component_style." << key << " = " << Metric(s[key]) << ";\n";
        for(const char* key : { "ink", "face", "frame" }) {
            ValueArray colors = s[key];
            for(int i = 0; i < 4; i++) out << "        r.component_style." << key << "[" << i << "] = " << ColorCpp(colors[i]) << ";\n";
        }
        out << "        if(!t.AddComponent(r, error)) Panic(~error);\n    }\n";
    }
    out << "    return t;\n}\n\n";
}
void StyleCode(String& out, const Value& v, const String& name)
{
    ValueMap s = v;
    out << "UiGraphNodeStyle Make" << name << "Style(const UiGraphNodeStyle& theme)\n{\n    UiGraphNodeStyle s = theme;\n";
    for(const char* key : { "face", "frame", "ink", "header" }) {
        ValueArray colors = s[key];
        for(int i = 0; i < 4; i++) {
            if(IsNull(colors[i])) continue;
            String index = "[" + AsString(i) + "]", color = ColorCpp(colors[i]);
            if(String(key) == "face") out << "    s.palette.face" << index << " = UiFill::Solid(" << color << ");\n";
            else if(String(key) == "frame") out << "    s.palette.frame" << index << " = " << color << ";\n";
            else if(String(key) == "header") out << "    s.header_face" << index << " = " << color << ";\n";
            else out << "    s.title_ink" << index << " = s.subtitle_ink" << index << " = s.description_ink" << index << " = s.port_label_ink" << index << " = s.palette.icon" << index << " = " << color << ";\n";
        }
    }
    if((int)s["frame_width"] >= 0) out << "    s.metrics.frame_width = " << Metric(s["frame_width"]) << "; s.metrics.frame_enabled = " << ((int)s["frame_width"] > 0 ? "true" : "false") << ";\n";
    if((int)s["header_band"] >= 0) out << "    s.show_header_band = " << ((int)s["header_band"] ? "true" : "false") << ";\n";
    if((int)s["shadow"] >= 0) out << "    s.metrics.shadow.enabled = " << ((int)s["shadow"] ? "true" : "false") << ";\n";
    if((int)s["shadow_y"] >= 0) out << "    s.metrics.shadow.offset_y = " << Metric(s["shadow_y"]) << ";\n";
    if((int)s["shadow_alpha"] >= 0) out << "    s.metrics.shadow.alpha = " << AsString(s["shadow_alpha"]) << ";\n";
    if(!IsNull(s["shadow_color"])) out << "    s.metrics.shadow.color = " << ColorCpp(s["shadow_color"]) << ";\n";
    out << "    return s;\n}\n\n";
}
} // namespace

String GenerateCpp(const Document& d, String& error)
{
    try {
        ValueMap root = Encode(d), family = root["family"]; ValueArray variants = family["shapes"];
        String ns = "UiGraphGenerated_";
        for(char c : d.family.name) ns.Cat(IsAlNum(c) && (byte)c < 128 ? c : '_');
        String out = "// Generated by UiGraph Node Design Workspace. Requires Ui, not the authoring package.\n#include <Ui/Ui.h>\nusing namespace Upp;\nnamespace " + ns + " {\n\n// TEMPLATE / LAYOUT\n";
        LayoutCode(out, family["layout"], "Base");
        for(int i = 0; i < SHAPE_COUNT; i++) { ValueMap v = variants[i]; if(d.family.layout_override[i]) LayoutCode(out, v["layout"], shape_names[i]); }
        out << "// STYLE OVERRIDES (null fields inherit the supplied theme)\n";
        StyleCode(out, family["style"], "Base");
        for(int i = 0; i < SHAPE_COUNT; i++) { ValueMap v = variants[i]; if(d.family.style_override[i]) StyleCode(out, v["style"], shape_names[i]); }
        out << "const UiGraphNodeTemplate& Layout(UiGraphNodeShape shape)\n{\n    static const auto base = MakeBaseLayout();\n    switch(shape) {\n";
        for(int i = 0; i < SHAPE_COUNT; i++) if(d.family.layout_override[i]) out << "    case UiGraphNodeShape::" << shape_names[i] << ": { static const auto t = Make" << shape_names[i] << "Layout(); return t; }\n";
        out << "    default: return base;\n    }\n}\n\nUiGraphNodeStyle Style(const UiGraphNodeStyle& theme, UiGraphNodeShape shape)\n{\n    switch(shape) {\n";
        for(int i = 0; i < SHAPE_COUNT; i++) if(d.family.style_override[i]) out << "    case UiGraphNodeShape::" << shape_names[i] << ": return Make" << shape_names[i] << "Style(theme);\n";
        out << "    default: return MakeBaseStyle(theme);\n    }\n}\n\nString ClassName(UiGraphNodeShape shape) { return String(" << QuoteCpp(ns + ".") << ") + AsString((int)shape); }\n\n"
            << "// Call once for each graph view; re-register after changing the theme.\nbool Register(UiNodeGraph& graph, String& error)\n{\n    const UiGraphNodeShape shapes[] = {";
        for(int i = 0; i < SHAPE_COUNT; i++) out << (i ? ", " : "") << "UiGraphNodeShape::" << shape_names[i];
        out << "};\n    for(auto shape : shapes) {\n        if(!graph.SetNodeTemplateClass(ClassName(shape), Layout(shape), error)) return false;\n        graph.SetNodeStyleClass(ClassName(shape), Style(graph.GetStyle().node, shape));\n    }\n    return true;\n}\n\n"
            << "// Host supplies position, size, ports and live data. No copied preview values.\nvoid Configure(UiGraphNode& node, UiGraphNodeShape shape)\n{\n    node.shape = shape; node.style_class = ClassName(shape);\n    switch(shape) {\n";
        for(int i = 0; i < SHAPE_COUNT; i++) {
            const auto& s = d.family.Style(i);
            out << "    case UiGraphNodeShape::" << shape_names[i] << ": node.role = (UiGraphNodeRole)" << s.role << "; ";
            if(s.radius >= 0) out << "node.corner_radius = DPI(" << s.radius << "); ";
            out << "break;\n";
        }
        out << "    default: break;\n    }\n}\n} // namespace " << ns << "\n";
        error.Clear(); return out;
    }
    catch(const Exc& e) { error = e; return String(); }
}
} // namespace GraphWorkspace
} // namespace Upp
