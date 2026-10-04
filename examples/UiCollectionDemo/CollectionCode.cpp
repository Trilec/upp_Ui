#include "UiCollectionDemo.h"

namespace Upp {
namespace {

String CppText(const String& value)
{
    String text = "\"";
    for(byte c : value) {
        if(c == '\\') text << "\\\\";
        else if(c == '"') text << "\\\"";
        else if(c == '\n') text << "\\n";
        else if(c == '\r') text << "\\r";
        else if(c == '\t') text << "\\t";
        else if(c < 32) text << Format("\\%03o", (int)c);
        else text.Cat(c);
    }
    return text << '"';
}

String CppColor(Color value)
{
    return IsNull(value) ? String("Null") : Format("Color(%d, %d, %d)", value.GetR(), value.GetG(), value.GetB());
}

void ApplySurface(StyledPalette& palette, StyledMetrics& metrics, const PropertyEditorModel& model)
{
    auto active = [&](const char* id) { const auto* item = model.Find(id); return item && item->override_active; };
    auto value = [&](const char* id) { return model.Find(id)->value; };
    for(int state : { ST_NORMAL, ST_DISABLED }) {
        String suffix = state == ST_NORMAL ? "normal" : "disabled";
        String face = "surface.face." + suffix, frame = "surface.frame." + suffix;
        if(active(face)) palette.face[state] = UiFill::Solid(Color(value(face)));
        if(active(frame)) palette.frame[state] = Color(value(frame));
    }
    if(active("surface.face")) metrics.face_enabled = value("surface.face");
    if(active("surface.frame")) metrics.frame_enabled = value("surface.frame");
    if(active("surface.frame_width")) metrics.frame_width = DPI((int)value("surface.frame_width"));
    if(active("surface.radius")) metrics.radius = DPI((int)value("surface.radius"));
    if(active("surface.margin_x")) metrics.content_margin.left = metrics.content_margin.right = DPI((int)value("surface.margin_x"));
    if(active("surface.margin_y")) metrics.content_margin.top = metrics.content_margin.bottom = DPI((int)value("surface.margin_y"));
    if(active("surface.focus")) metrics.focus_enabled = value("surface.focus");
    if(active("surface.shadow")) metrics.shadow.enabled = value("surface.shadow");
    if(active("surface.highlight")) metrics.highlight.enabled = value("surface.highlight");
}

} // namespace

UiList::Style UiCollectionDemo::ListStyle() const
{
    UiList::Style style = UiTheme::ResolveList();
    ApplySurface(style.palette, style.metrics, override_model);
    style.row_height = DPI((int)Config("list.height"));
    style.item_spacing = DPI((int)Config("list.spacing"));
    style.show_checks = Config("list.checks");
    style.show_icons = Config("list.icons");
    style.show_metadata_marker = Config("list.metadata");
    style.show_row_separator = Config("list.separators");
    style.striped_rows = Config("list.striped");
    style.right_text_as_badge = Config("list.badges");
    style.show_drag_handle = Config("list.drag_handle");
    style.drag_side = AsString(Config("list.drag_side")) == "Left" ? UiAlign::LEFT : UiAlign::RIGHT;
    if(Active("list.radius")) style.row_radius = DPI((int)Override("list.radius"));
    if(Active("list.pad_x")) style.h_padding = DPI((int)Override("list.pad_x"));
    if(Active("list.pad_y")) style.v_padding = DPI((int)Override("list.pad_y"));
    if(Active("list.drag_size")) style.drag_size = DPI((int)Override("list.drag_size"));
    if(Active("list.drag_gap")) style.drag_gap = DPI((int)Override("list.drag_gap"));
    if(Active("list.hot_face")) style.hot_face = Color(Override("list.hot_face"));
    if(Active("list.selected_face")) style.selected_face = Color(Override("list.selected_face"));
    if(Active("list.even")) style.row_even_face = Color(Override("list.even"));
    if(Active("list.odd")) style.row_odd_face = Color(Override("list.odd"));
    if(Active("list.badge_face")) style.badge_face = Color(Override("list.badge_face"));
    if(Active("list.badge_ink")) style.badge_ink = Color(Override("list.badge_ink"));
    if(Active("list.font")) style.font.FaceName(AsString(Override("list.font")));
    if(Active("list.font_height")) style.font.Height(DPI((int)Override("list.font_height")));
    if(Active("list.icon_size")) style.icon_size = DPI((int)Override("list.icon_size"));
    if(Active("list.ink")) style.ink = Color(Override("list.ink"));
    if(Active("list.icon")) style.palette.icon[ST_NORMAL] = Color(Override("list.icon"));
    return style;
}

UiGallery::Style UiCollectionDemo::GalleryStyle() const
{
    UiGallery defaults;
    UiGallery::Style style = defaults.GetStyle();
    ApplySurface(style.palette, style.metrics, override_model);
    if(Active("gallery.selection")) style.selection_frame = Color(Override("gallery.selection"));
    if(Active("gallery.selection_width")) style.selection_frame_width = DPI((int)Override("gallery.selection_width"));
    if(Active("gallery.marquee")) style.marquee_frame = Color(Override("gallery.marquee"));
    if(Active("gallery.marquee_fill")) style.marquee_fill = Color(Override("gallery.marquee_fill"));
    return style;
}

UiItemRenderStyle UiCollectionDemo::RenderStyle() const
{
    UiItemRenderImage defaults;
    UiItemRenderStyle style = defaults.GetStyle();
    static const char* states[] = { "normal", "hot", "selected", "disabled" };
    for(int i = 0; i < 4; i++) {
        String ink = "renderer.ink." + String(states[i]), icon = "renderer.icon." + String(states[i]);
        if(Active(ink)) style.palette.ink[i] = Color(Override(ink));
        if(Active(icon)) style.palette.icon[i] = Color(Override(icon));
    }
    for(Font* font : { &style.title_font, &style.subtitle_font, &style.description_font, &style.right_font }) {
        if(Active("renderer.font")) font->FaceName(AsString(Override("renderer.font")));
        if(Active("renderer.font_height")) font->Height(DPI((int)Override("renderer.font_height")));
        if(Active("renderer.font_bold")) font->Bold((bool)Override("renderer.font_bold"));
    }
    if(Active("renderer.icon_size")) style.icon_size = DPI((int)Override("renderer.icon_size"));
    if(Active("renderer.description")) style.show_description = Override("renderer.description");
    if(Active("renderer.right_text")) style.show_right_text = Override("renderer.right_text");
    return style;
}

String UiCollectionDemo::GenerateCode() const
{
    String view = AsString(Config("view"));
    bool use_list=view!="Gallery", use_gallery=view!="List";
    String out = "#include <Ui/Ui.h>\n\nusing namespace Upp;\n\n"
                 "// Configuration exported by List + Gallery Designer.\n"
                 "// Supply your UiListModel and assets; they must outlive the views.\n"
                 "// The standalone window below uses a small text-only sample dataset.\n"
                 "// Data-page edits, sample size and demo images are host data, not style.\n"
                 "\nvoid ConfigureCollection(UiListModel& model";
    if(use_list) out << ", UiList& list";
    if(use_gallery) out << ", UiGallery& gallery";
    out << ")\n{\n";
    auto boolean = [&](const char* target, const String& id) { out << "    " << target << " = " << ((bool)Config(id) ? "true" : "false") << ";\n"; };
    auto number = [&](const char* target, const String& id) { out << "    " << target << " = DPI(" << (int)Config(id) << ");\n"; };
    auto override_number = [&](const String& target, const String& id) {
        if(Active(id)) out << "    " << target << " = DPI(" << (int)Override(id) << ");\n";
    };
    auto override_bool = [&](const String& target, const String& id) {
        if(Active(id)) out << "    " << target << " = " << ((bool)Override(id) ? "true" : "false") << ";\n";
    };
    auto override_color = [&](const String& target, const String& id) {
        if(Active(id)) out << "    " << target << " = " << CppColor(Color(Override(id))) << ";\n";
    };
    auto surface = [&](const char* name) {
        for(int state : { ST_NORMAL, ST_DISABLED }) {
            String suffix = state == ST_NORMAL ? "normal" : "disabled";
            String state_code = state == ST_NORMAL ? "ST_NORMAL" : "ST_DISABLED";
            String id = "surface.face." + suffix;
            if(Active(id)) out << "    " << name << ".palette.face[" << state_code << "] = UiFill::Solid(" << CppColor(Color(Override(id))) << ");\n";
            override_color(String(name) + ".palette.frame[" + state_code + "]", "surface.frame." + suffix);
        }
        String metrics = String(name) + ".metrics.";
        override_bool(metrics + "face_enabled", "surface.face");
        override_bool(metrics + "frame_enabled", "surface.frame");
        override_number(metrics + "frame_width", "surface.frame_width");
        override_number(metrics + "radius", "surface.radius");
        override_number(metrics + "content_margin.left", "surface.margin_x");
        override_number(metrics + "content_margin.right", "surface.margin_x");
        override_number(metrics + "content_margin.top", "surface.margin_y");
        override_number(metrics + "content_margin.bottom", "surface.margin_y");
        override_bool(metrics + "focus_enabled", "surface.focus");
        override_bool(metrics + "shadow.enabled", "surface.shadow");
        override_bool(metrics + "highlight.enabled", "surface.highlight");
    };
    if(use_list) {
    out << "    list.SetModel(model);\n"
        << "    list.Enable(" << ((bool)Config("enabled") ? "true" : "false") << ");\n"
        << "    list.SetSelectionMode(" << ((bool)Config("multi") ? "UILISTSEL_MULTI" : "UILISTSEL_SINGLE") << ");\n"
        << "    list.EnableRenameOnDblClick(" << ((bool)Config("list.rename") ? "true" : "false") << ");\n"
        << "    list.EnableDragReorder(" << ((bool)Config("list.reorder") ? "true" : "false") << ");\n"
        << "    UiList::Style ls = UiTheme::ResolveList();\n";
    number("ls.row_height", "list.height"); number("ls.item_spacing", "list.spacing");
    boolean("ls.show_checks", "list.checks"); boolean("ls.show_icons", "list.icons");
    boolean("ls.show_metadata_marker", "list.metadata"); boolean("ls.show_row_separator", "list.separators");
    boolean("ls.striped_rows", "list.striped"); boolean("ls.right_text_as_badge", "list.badges");
    boolean("ls.show_drag_handle", "list.drag_handle");
    out << "    ls.drag_side = UiAlign::" << (AsString(Config("list.drag_side")) == "Left" ? "LEFT" : "RIGHT") << ";\n";
    surface("ls");
    override_number("ls.row_radius", "list.radius"); override_number("ls.h_padding", "list.pad_x");
    override_number("ls.v_padding", "list.pad_y"); override_number("ls.drag_size", "list.drag_size");
    override_number("ls.drag_gap", "list.drag_gap"); override_number("ls.icon_size", "list.icon_size");
    override_color("ls.hot_face", "list.hot_face"); override_color("ls.selected_face", "list.selected_face");
    override_color("ls.row_even_face", "list.even"); override_color("ls.row_odd_face", "list.odd");
    override_color("ls.badge_face", "list.badge_face"); override_color("ls.badge_ink", "list.badge_ink");
    override_color("ls.ink", "list.ink"); override_color("ls.palette.icon[ST_NORMAL]", "list.icon");
    if(Active("list.font")) out << "    ls.font.FaceName(" << CppText(AsString(Override("list.font"))) << ");\n";
    if(Active("list.font_height")) out << "    ls.font.Height(DPI(" << (int)Override("list.font_height") << "));\n";
    out << "    list.SetCustomStyle(ls);\n";
    String renderer = AsString(Config("renderer")) == "Image" ? "UiItemRenderImage" : "UiItemRenderBasic";
    out << "    " << renderer << " list_render;\n    list.SetItemRender(list_render);\n";
    }
    if(use_gallery) {
    out << "    gallery.SetModel(model);\n"
        << "    gallery.Enable(" << ((bool)Config("enabled") ? "true" : "false") << ");\n"
        << "    gallery.SetSelectionMode(" << ((bool)Config("multi") ? "UIGALLERYSEL_MULTI" : "UIGALLERYSEL_SINGLE") << ");\n";
    String renderer = AsString(Config("renderer")) == "Image" ? "UiItemRenderImage" : "UiItemRenderBasic";
    out << "    " << renderer << " gallery_render;\n";
    if(AnyOverride("renderer.")) {
        out << "    UiItemRenderStyle rs = gallery_render.GetStyle();\n";
        static const char* suffix[] = { "normal", "hot", "selected", "disabled" };
        static const char* state[] = { "ST_NORMAL", "ST_HOT", "ST_PRESSED", "ST_DISABLED" };
        for(int i = 0; i < 4; i++) {
            override_color("rs.palette.ink[" + String(state[i]) + "]", "renderer.ink." + String(suffix[i]));
            override_color("rs.palette.icon[" + String(state[i]) + "]", "renderer.icon." + String(suffix[i]));
        }
        for(const char* font : { "title_font", "subtitle_font", "description_font", "right_font" }) {
            if(Active("renderer.font")) out << "    rs." << font << ".FaceName(" << CppText(AsString(Override("renderer.font"))) << ");\n";
            if(Active("renderer.font_height")) out << "    rs." << font << ".Height(DPI(" << (int)Override("renderer.font_height") << "));\n";
            if(Active("renderer.font_bold")) out << "    rs." << font << ".Bold(" << ((bool)Override("renderer.font_bold") ? "true" : "false") << ");\n";
        }
        override_number("rs.icon_size", "renderer.icon_size");
        override_bool("rs.show_description", "renderer.description"); override_bool("rs.show_right_text", "renderer.right_text");
        out << "    gallery_render.SetCustomStyle(rs);\n";
    }
    out << "    gallery.SetItemRender(gallery_render);\n";
    if(AnyOverride("surface.") || AnyOverride("gallery.")) {
        out << "    UiGallery::Style gs = gallery.GetStyle();\n";
        surface("gs");
        override_color("gs.selection_frame", "gallery.selection");
        override_number("gs.selection_frame_width", "gallery.selection_width");
        override_color("gs.marquee_frame", "gallery.marquee"); override_color("gs.marquee_fill", "gallery.marquee_fill");
        out << "    gallery.SetCustomStyle(gs);\n";
    }
    out << "    gallery.SetItemSize(Size(DPI(" << (int)Config("gallery.width") << "), DPI(" << (int)Config("gallery.height") << ")))\n"
        << "           .SetGap(DPI(" << (int)Config("gallery.gap") << ")).SetInset(DPI(" << (int)Config("gallery.inset") << "))\n"
        << "           .SetOverscanRows(" << (int)Config("gallery.overscan") << ").SetZoomRange(0.5, 2.5)\n"
        << "           .SetZoom(" << Format("%.17g", (double)Config("gallery.zoom")) << ");\n";
    }
    out << "}\n\n";
    out << "class CollectionWindow : public TopWindow {\n"
           "    UiListModel model; // Declared before borrowed views.\n"
           "    UiPanel surface;\n"
           ;
    if(use_list) out << "    UiList list;\n";
    if(use_gallery) out << "    UiGallery gallery;\n";
    out << "public:\n"
           "    CollectionWindow() {\n"
           "        Title(\"Collection\"); Sizeable().Zoomable(); SetRect(0, 0, DPI(1000), DPI(600));\n"
           "        for(int i = 0; i < 24; i++) model.Add(Format(\"Item %d\", i + 1), i);\n";
    if(data_index >= 0 && data_index < model.GetCount())
        out << "        // A sample title from the inspected record; replace the sample model with your data.\n"
            << "        model.Get(0).text = " << CppText(model.Get(data_index).text) << "; model.Touch(0);\n";
    out << "        ConfigureCollection(model";
    if(use_list) out << ", list";
    if(use_gallery) out << ", gallery";
    out << ");\n        Add(surface.SizePos());\n";
    if(view != "Gallery") out << "        surface.Add(list);\n";
    if(view != "List") out << "        surface.Add(gallery);\n";
    out << "    }\n    void Layout() override {\n        Size size = GetSize();\n";
    if(view == "Compare")
        out << "        int left = max(0, (size.cx - DPI(8)) * 38 / 100);\n"
               "        list.SetRect(0, 0, left, size.cy);\n"
               "        gallery.SetRect(left + DPI(8), 0, max(0, size.cx - left - DPI(8)), size.cy);\n";
    else out << "        " << (view == "List" ? "list" : "gallery") << ".SetRect(0, 0, size.cx, size.cy);\n";
    out << "    }\n};\n\nGUI_APP_MAIN\n{\n    UiTheme::Set(UiThemeMode::"
        << (UiTheme::GetContext().mode == UiThemeMode::Dark ? "Dark" : "Light")
        << ");\n    CollectionWindow().Run();\n}\n";
    return out;
}

bool UiCollectionDemo::RunAcceptance(const String& directory)
{
    RealizeDirectory(directory);
    String report;
    int checks = 0, failed = 0;
    auto expect = [&](bool condition, const char* label) {
        checks++; if(!condition) failed++;
        report << (condition ? "PASS " : "FAIL ") << label << '\n';
    };
    SaveFile(AppendFileName(directory, "default.cpp"), GenerateCode());
    SelectView("Compare");
    Layout(); list.Layout(); gallery.Layout();
    expect(list.GetSize().cy <= GetSize().cy && gallery.GetSize().cy <= GetSize().cy,
           "preview bounds follow the visible window");
    expect(list.GetLiveItemRenderCount() < 60 && gallery.GetLiveItemRenderCount() < 120,
           "10,000 item comparison uses small renderer pools");
    expect(&list.Model() == &model && &gallery.Model() == &model, "both views share the production model");
    SaveFile(AppendFileName(directory, "comparison.cpp"), GenerateCode());
    int schema = inspector_model.GetStructureRevision();
    inspector_model.SetValue("list.height", 42); ApplyConfiguration("list.height");
    inspector_model.SetValue("gallery.gap", 13); ApplyConfiguration("gallery.gap");
    inspector_model.SetValue("gallery.zoom", 1.35); ApplyConfiguration("gallery.zoom");
    expect(list.GetStyle().row_height == DPI(42) && gallery.GetGap() == DPI(13)
           && fabs(gallery.GetZoom() - 1.35) < 0.0001, "authored geometry reaches both public APIs");
    expect(inspector_model.GetStructureRevision() == schema, "property edits preserve inspector schema");
    SaveFile(AppendFileName(directory, "configured.cpp"), GenerateCode());
    override_model.Find("surface.face.normal")->override_active = true;
    override_model.SetValue("surface.face.normal", Color(24, 40, 70));
    override_model.Find("renderer.font_height")->override_active = true;
    override_model.SetValue("renderer.font_height", 17);
    override_model.Find("list.font_height")->override_active = true;
    override_model.SetValue("list.font_height", 16);
    ApplyConfiguration();
    expect(list.GetStyle().palette.face[ST_NORMAL].color == Color(24, 40, 70)
           && gallery.GetStyle().palette.face[ST_NORMAL].color == Color(24, 40, 70), "surface override reaches both controls");
    InspectItem(3);
    data_model.SetValue("text", "Quoted \"item\" \\ path\nline"); ApplyData("text");
    expect(model.Get(3).text == "Quoted \"item\" \\ path\nline", "Data edits the active model directly");
    SaveFile(AppendFileName(directory, "overrides.cpp"), GenerateCode());
    SelectPage(2); SelectView("Gallery");
    expect(!list.IsShown() && gallery.IsShown() && stk_pages.GetActivePage() == 2, "view switch preserves Data page");
    SaveFile(AppendFileName(directory, "gallery.cpp"), GenerateCode());
    SelectView("List");
    expect(list.IsShown() && !gallery.IsShown(), "List-only preview hides Gallery");
    SaveFile(AppendFileName(directory, "list.cpp"), GenerateCode());
    UiTheme::Set(UiThemeMode::Dark); ApplyTheme(); ApplyConfiguration();
    expect(stk_pages.GetActivePage() == 2 && Active("surface.face.normal"), "theme switch preserves page and authored overrides");
    Reset(override_model, "surface.face.normal");
    expect(!Active("surface.face.normal") && gallery.GetStyle().palette.face[ST_NORMAL].color != Color(24, 40, 70), "reset restores current theme inheritance");
    SelectView("Compare");
    int64 start = usecs();
    for(int i = 0; i < 100; i++) list.MouseWheel(Point(20, 20), -120, 0);
    int64 list_us = usecs() - start;
    start = usecs();
    for(int i = 0; i < 100; i++) gallery.MouseWheel(Point(20, 20), -120, 0);
    int64 gallery_us = usecs() - start;
    report << "SCROLL_CPU iterations=100 list_us=" << list_us << " gallery_us=" << gallery_us
           << " list_pool=" << list.GetLiveItemRenderCount() << " gallery_pool=" << gallery.GetLiveItemRenderCount() << '\n';
    expect(list.GetLiveItemRenderCount() < 60 && gallery.GetLiveItemRenderCount() < 120,
           "repeated scrolling keeps both renderer pools bounded");
    report << "UI_COLLECTION_DEMO_ACCEPTANCE checks=" << checks << " failed=" << failed << '\n';
    SaveFile(AppendFileName(directory, "acceptance.log"), report);
    return failed == 0;
}

} // namespace Upp
