#include "Demo.h"
#include <plugin/png/png.h>
#include <cmath>

namespace {
String Cpp(const String& text) { return AsCString(text); }
String ColorCode(Color color)
{
    return IsNull(color) ? String("Null") : Format("Color(%d, %d, %d)", color.GetR(), color.GetG(), color.GetB());
}
String ValueCode(const Value& value)
{
    auto Native = [&] {
        Value copy = value;
        return "NativeValue(" + Cpp(Base64Encode(StoreAsString(copy))) + ")";
    };
    if(value.IsVoid()) return "Value()";
    if(value.Is<Color>()) { Color color = value; return IsNull(color) ? String("Color(Null)") : ColorCode(color); }
    if(IsNull(value)) return Native();
    if(value.Is<String>()) return Cpp(value);
    if(value.Is<bool>()) return (bool)value ? "true" : "false";
    if(value.Is<int>()) return AsString((int)value);
    if(value.Is<int64>()) return "(int64)" + AsString((int64)value);
    if(value.Is<double>() && std::isfinite((double)value)) {
        String number = Format("%.17g", (double)value);
        if(number.Find('.') < 0 && number.Find('e') < 0 && number.Find('E') < 0) number << ".0";
        return number;
    }
    if(value.Is<ValueArray>()) {
        String out = "Values({";
        bool first = true;
        for(const auto& child : (ValueArray)value) { if(!first) out << ", "; first = false; out << ValueCode(child); }
        return out + "})";
    }
    if(value.Is<ValueMap>()) {
        ValueMap map = value; String out = "Map({";
        for(int i = 0; i < map.GetCount(); i++) { if(i) out << ", "; out << "{" << ValueCode(map.GetKey(i)) << ", " << ValueCode(map.GetValue(i)) << "}"; }
        return out + "})";
    }
    // Native Value fidelity for application-defined values that have no simple
    // C++ literal. This helper is emitted only if the data actually requires it.
    return Native();
}
const char* ElementCode(UiScreenplayElement element)
{
    static const char* names[] = {"Scene", "Action", "Character", "Dialogue", "Parenthetical", "Transition", "Shot", "Centered", "Lyrics", "PageBreak"};
    return names[minmax((int)element, 0, 9)];
}
}

String ScreenplayDemo::Generate() const
{
    ValueMap native = ParseJSON(script_.Model().ToJson());
    ValueArray anchors = native["anchors"];
    auto paragraphs = script_.GetParagraphs();
    bool extensions = !script_.Model().GetEmbeds().IsEmpty() || anchors.GetCount() ||
                      script_.Model().GetBlocks().GetCount() != paragraphs.GetCount();
    int from = 0;
    for(const auto& paragraph : paragraphs) {
        bool canonical = false;
        for(const auto& block : script_.Model().GetBlocks())
            if(block.id == paragraph.id && block.role == UiScreenplay::Role(paragraph.element) &&
               block.indent == 0 && block.range.from == from && block.range.to == from + paragraph.text.GetCount())
                canonical = true;
        extensions |= !canonical;
        from += paragraph.text.GetCount() + 1;
    }
    String code = "#include <Ui/Ui.h>\nusing namespace Upp;\n\n";
    code << "static void ConfigureScreenplay(UiScreenplay& script)\n{\n"
         << "    UiThemeContext theme = UiTheme::GetContext();\n"
         << "    theme.preset = UiThemePreset::Minimal;\n"
         << "    theme.mode = UiThemeMode::" << (UiTheme::GetContext().mode == UiThemeMode::Dark ? "Dark" : "Light") << ";\n"
         << "    UiTheme::Set(theme);\n";
    if(UiTheme::GetContext().mode == UiThemeMode::Dark) code << "    Ctrl::SwapDarkLight();\n";
    code << "\n";
    if(extensions) {
        code << "    // Native extensions retain their exact structure.\n"
             << "    script.Model().FromJson(" << Cpp(script_.Model().ToJson()) << ");\n";
    }
    else {
        for(const auto& resource : script_.Model().GetResources()) {
            code << "    {\n        UiDocResource image;\n"
                 << "        image.key = " << Cpp(resource.key) << ";\n"
                 << "        image.resource_type = " << Cpp(resource.resource_type) << ";\n"
                 << "        image.bytes = Base64Decode(" << Cpp(Base64Encode(resource.bytes)) << ");\n"
                 << "        image.content_hash = " << Cpp(resource.content_hash) << ";\n"
                 << "        image.mime = " << Cpp(resource.mime) << ";\n"
                 << "        image.original_name = " << Cpp(resource.original_name) << ";\n"
                 << "        image.width = " << resource.width << "; image.height = " << resource.height << ";\n"
                 << "        image.meta = " << ValueCode(resource.meta) << ";\n"
                 << "        script.AddResource(image, false);\n    }\n";
        }
        code << "    Vector<UiScreenplayParagraph> paragraphs;\n";
        for(const auto& paragraph : script_.GetParagraphs()) {
            code << "    {\n        auto& paragraph = paragraphs.Add();\n"
                 << "        paragraph.id = " << Cpp(paragraph.id) << ";\n"
                 << "        paragraph.element = UiScreenplayElement::" << ElementCode(paragraph.element) << ";\n"
                 << "        paragraph.text = ToUnicode(" << Cpp(ToUtf8(paragraph.text)) << ", CHARSET_UTF8);\n";
            if(paragraph.meta.GetCount()) code << "        paragraph.meta = " << ValueCode(paragraph.meta) << ";\n";
            code << "    }\n";
        }
        code << "    script.SetParagraphs(paragraphs);\n\n    UiScreenplayFormat format;\n";
        auto format = script_.GetFormat().ToValue();
        auto defaults = UiScreenplayFormat().ToValue();
        for(int i = 0; i < format.GetCount(); i++) {
            String key = AsString(format.GetKey(i));
            if(format.GetValue(i) != defaults[key]) code << "    format." << key << " = " << ValueCode(format.GetValue(i)) << ";\n";
        }
        code << "    script.SetFormat(format);\n\n";
        auto title = script_.GetTitlePage().ToValue();
        code << "    UiScreenplayTitlePage cover;\n";
        for(int i = 0; i < title.GetCount(); i++) code << "    cover." << AsString(title.GetKey(i)) << " = " << ValueCode(title.GetValue(i)) << ";\n";
        code << "    script.SetTitlePage(cover);\n";
        const auto& meta = script_.Model().GetMeta();
        for(int i = 0; i < meta.GetCount(); i++) {
            String key = AsString(meta.GetKey(i));
            if(key != "screenplay.format" && key != "screenplay.title_page") code << "    script.Model().SetMeta(" << Cpp(key) << ", " << ValueCode(meta.GetValue(i)) << ");\n";
        }
        for(const auto& run : script_.Model().GetStyles()) {
            const auto& style = run.style;
            code << "    {\n        UiDocTextStyle emphasis;\n        emphasis.flags = " << (int)style.flags << ";\n";
            if(!IsNull(style.ink)) code << "        emphasis.ink = " << ColorCode(style.ink) << ";\n";
            if(!style.font_face.IsEmpty()) code << "        emphasis.font_face = " << Cpp(style.font_face) << ";\n";
            for(auto field : {std::make_pair("font_height", style.font_height), std::make_pair("size_delta", style.size_delta),
                              std::make_pair("leading_delta", style.leading_delta), std::make_pair("tracking_delta", style.tracking_delta)})
                if(field.second) code << "        emphasis." << field.first << " = " << field.second << ";\n";
            code << "        script.Model().SetStyle(UiDocRange(" << run.from << ", " << run.to << "), emphasis);\n    }\n";
        }
        for(const auto& annotation : script_.Model().GetAnnotations()) {
            code << "    {\n        UiDocCoreTransaction reference;\n        auto& change = reference.changes.Add();\n"
                 << "        change.type = UiDocCoreChange::AddAnnotation;\n"
                 << "        change.annotation.id = " << Cpp(annotation.id) << ";\n"
                 << "        change.annotation.type = " << Cpp(annotation.type) << ";\n"
                 << "        change.annotation.range = UiDocRange(" << annotation.range.from << ", " << annotation.range.to << ");\n"
                 << "        change.annotation.payload = " << ValueCode(annotation.payload) << ";\n"
                 << "        change.annotation.meta = " << ValueCode(annotation.meta) << ";\n"
                 << "        change.annotation.expanded = " << (annotation.expanded ? "true" : "false") << ";\n"
                 << "        change.annotation.printable = " << (annotation.printable ? "true" : "false") << ";\n"
                 << "        change.annotation.resolved = " << (annotation.resolved ? "true" : "false") << ";\n"
                 << "        script.Model().Apply(reference);\n    }\n";
        }
    }
    code << "\n    auto style = UiTheme::ResolveDoc();\n    style.line_gap = style.paragraph_gap = 0;\n    style.metrics.radius = DPI(8);\n";
    const char* fields[] = {"paper", "ink", "selection", "caret", "annotation", "radius", "frame_width", "frame", "accent.thickness", "accent.alpha", "accent.color"};
    const char* targets[] = {"page_face", "palette.ink[ST_NORMAL]", "selection_fill", "caret_ink", "annotation_fill", "metrics.radius", "metrics.frame_width", "palette.frame[ST_NORMAL]", "metrics.frame_accent.thickness", "metrics.frame_accent.alpha", "metrics.frame_accent.color"};
    for(int i = 0; i < 11; i++) {
        const auto* row = style_model_.Find(fields[i]);
        if(row && row->override_active) {
            if(i == 1 || i == 7) {
                String target = i == 1 ? "palette.ink[i]" : "palette.frame[i]";
                code << "    for(int i = 0; i < 4; i++) style." << target << " = " << ValueCode(row->value) << ";\n";
            }
            else code << "    style." << targets[i] << " = " << ValueCode(row->value) << ";\n";
        }
    }
    const char* edges[] = {"top", "bottom", "left", "right"};
    const char* names[] = {"Top", "Bottom", "Left", "Right"};
    for(int i = 0; i < 4; i++) {
        const auto* row = style_model_.Find("accent." + String(edges[i]));
        if(row && row->override_active) code << "    style.metrics.frame_accent.edges " << ((bool)row->value ? "|= " : "&= ~") << "StyledFrameAccent::" << names[i] << ";\n";
    }
    code << "    script.SetCustomStyle(style);\n"
         << "    script.ReadOnly(" << (script_.IsReadOnly() ? "true" : "false") << ");\n"
         << "    script.ShowMetadataCards(" << (script_.IsMetadataCardsShown() ? "true" : "false") << ");\n"
         << "    script.ShowMetadata(" << (script_.IsMetadataShown() ? "true" : "false") << ");\n"
         << "    script.ShowLineNumbers(" << (script_.IsLineNumbersShown() ? "true" : "false") << ");\n\n"
         << "}\n\nGUI_APP_MAIN\n{\n    UiScreenplay script;\n    ConfigureScreenplay(script);\n"
         << "    TopWindow window;\n    window.Title(\"UiScreenplay\").Sizeable().Zoomable();\n"
         << "    window.SetRect(0, 0, DPI(960), DPI(850));\n"
         << "    window.Add(script.SizePos());\n    window.Run();\n}\n";
    if(code.Find("NativeValue(") >= 0) {
        int at = code.Find("static void ConfigureScreenplay");
        code.Insert(at, "static Value NativeValue(const String& encoded)\n{\n    Value value; LoadFromString(value, Base64Decode(encoded)); return value;\n}\n\n");
    }
    int main = code.Find("static void ConfigureScreenplay");
    String helpers;
    if(code.Find("Map({") >= 0) helpers << "static ValueMap Map(std::initializer_list<std::pair<Value, Value>> entries)\n{\n    ValueMap map; for(const auto& entry : entries) map.Add(entry.first, entry.second); return map;\n}\n\n";
    if(code.Find("Values({") >= 0) helpers << "static ValueArray Values(std::initializer_list<Value> entries)\n{\n    ValueArray array; for(const auto& entry : entries) array.Add(entry); return array;\n}\n\n";
    code.Insert(main, helpers);
    return code;
}

static Value ViewSignature(const UiScreenplay& script)
{
    ValueArray values;
    const auto& style = script.GetStyle();
    values.Add(style.page_face); values.Add(style.selection_fill); values.Add(style.caret_ink); values.Add(style.annotation_fill);
    for(int i = 0; i < 4; i++) { values.Add(style.palette.ink[i]); values.Add(style.palette.frame[i]); }
    values.Add(style.metrics.radius); values.Add(style.metrics.frame_width);
    values.Add(style.metrics.frame_accent.edges); values.Add(style.metrics.frame_accent.thickness);
    values.Add(style.metrics.frame_accent.alpha); values.Add(style.metrics.frame_accent.color);
    values.Add(script.IsReadOnly()); values.Add(script.IsMetadataCardsShown());
    values.Add(script.IsMetadataShown()); values.Add(script.IsLineNumbersShown());
    return values;
}

void ScreenplayDemo::ExportGenerated(const String& directory)
{
    RealizeDirectory(directory);
    auto Write = [&](const char* name) {
        SaveFile(AppendFileName(directory, String(name) + ".cpp"), Generate());
        SaveFile(AppendFileName(directory, String(name) + ".uidoc"), script_.Model().ToJson());
        Value signature = ViewSignature(script_);
        SaveFile(AppendFileName(directory, String(name) + ".view"), StoreAsString(signature));
    };
    Write("default");
    theme_.WhenAction();
    style_model_.Find("paper")->override_active = true; style_model_.SetValue("paper", Color(28, 30, 35));
    style_model_.Find("ink")->override_active = true; style_model_.SetValue("ink", White());
    style_model_.Find("accent.top")->override_active = true; style_model_.SetValue("accent.top", true);
    ApplyStyle(); Write("dark-styled");
    auto title = script_.GetTitlePage(); title.title = "A \"quoted\" title — 夜"; title.qtf = "[= [* A cover with emphasis]&Written by&Example Writer]";
    ImageDraw artwork(Size(120, 40)); artwork.DrawRect(0, 0, 120, 40, Color(70, 100, 130));
    UiDocResource resource; resource.resource_type = "image"; resource.bytes = PNGEncoder().SaveString(artwork);
    resource.width = 120; resource.height = 40; resource.mime = "image/png";
    title.resource_key = script_.AddResource(resource);
    script_.SetTitlePage(title);
    script_.Model().SetMark(UiDocRange(0, 6), UiDocTextStyle::BOLD, true);
    ValueMap ai; ai.Add("provider", "host supplied"); ai.Add("base_revision", (int64)script_.Model().GetRevision());
    script_.Model().SetMeta("screenplay.ai", ai);
    ValueMap typed;
    typed.Add("empty_integer", (int)Null); typed.Add("empty_colour", Color(Null));
    typed.Add("review_date", Date(2026, 10, 11)); typed.Add("weight", 1.0);
    ValueArray numbers; numbers.Add(1); numbers.Add((int64)2); numbers.Add(3.5); typed.Add("numbers", numbers);
    script_.Model().SetMeta("screenplay.typed_payload", typed);
    Write("rich-metadata");
    script_.Model().SetAnchor("scene-opening", 0);
    Write("native-extensions");
    UiScreenplay output; script_.ConfigurePrintView(output);
    Size size = output.GetPageFormat().size;
    Size dots((int)floor(output.GetFormat().page_width * 600 + 0.5), (int)floor(output.GetFormat().page_height * 600 + 0.5));
    PdfDraw pdf(dots);
    for(int page = 0; page < output.GetPageCount(); page++) {
        ImageDraw image(size); output.PaintPage(image, page);
        PNGEncoder().SaveFile(AppendFileName(directory, Format("demo-page-%d.png", page + 1)), image);
        pdf.StartPage(); DrawingDraw drawing(size); output.PaintPage(drawing, page);
        pdf.DrawDrawing(Rect(Point(0,0), dots), drawing.GetResult()); pdf.EndPage();
    }
    SaveFile(AppendFileName(directory, "screenplay.pdf"), pdf.Finish());
}

int ScreenplayDemo::SelfTest()
{
    int checks = 0, failures = 0;
    auto Expect = [&](bool condition) { checks++; if(!condition) failures++; };
    for(int page = 0; page < 4; page++) { Page(page); Expect(pages_.GetActivePage() == page); }
    for(int element = 0; element < 9; element++) {
        inspector_model_.SetValue("element", element); ApplyInspector("element");
        Expect(script_.GetElement() == (UiScreenplayElement)element);
    }
    for(auto* button : {&theme_, &theme_}) { button->WhenAction(); Expect(!Generate().IsEmpty()); }
    auto format = script_.GetFormat(); format.scene_numbers = true;
    Expect(script_.SetFormat(format)); Expect(script_.GetFormat().scene_numbers);
    AddReference(false); Expect(!selected_annotation_.IsEmpty());
    data_model_.SetValue("reference_text", "Changed story objective"); ApplyData("reference_text");
    bool found = false;
    for(const auto& annotation : script_.Model().GetAnnotations())
        if(annotation.id == selected_annotation_) found = annotation.payload["text"] == "Changed story objective";
    Expect(found);
    String error; Expect(script_.Model().Validate(&error));
    Cout() << Format("UISCREENPLAY_DEMO_SUMMARY checks=%d failed=%d\n", checks, failures);
    return failures ? 1 : 0;
}
