#include "UiDesignerCodeGen.h"

namespace Upp {

static String SanitizeIdentifier(String value)
{
    value = TrimBoth(value);
    if(value.IsEmpty())
        value = "control";
    String out;
    for(int i = 0; i < value.GetCount(); i++) {
        const int c = value[i];
        if(IsAlNum(c) || c == '_')
            out.Cat(c);
        else if(out.IsEmpty() || out[out.GetCount() - 1] != '_')
            out.Cat('_');
    }
    while(out.GetCount() && out[out.GetCount() - 1] == '_')
        out.Trim(out.GetCount() - 1);
    if(out.IsEmpty())
        out = "control";
    if(IsDigit(out[0]))
        out = "_" + out;
    static const char *reserved[] = {
        "class", "private", "public", "protected", "template", "typename",
        "operator", "int", "double", "float", "bool", "char", "void",
        "auto", "return", "new", "delete", "namespace"
    };
    for(const char *word : reserved)
        if(out == word)
            return "_" + out;
    return out;
}

static String CppString(const String& text)
{
    String out = "\"";
    for(int i = 0; i < text.GetCount(); i++) {
        const byte c = text[i];
        if(c == '\\') out << "\\\\";
        else if(c == '"') out << "\\\"";
        else if(c == '\n') out << "\\n";
        else if(c == '\r') out << "\\r";
        else if(c == '\t') out << "\\t";
        else out.Cat(c);
    }
    out << "\"";
    return out;
}

static String NamespaceOpen(const String& ns)
{
    return ns.IsEmpty() ? String() : "namespace " + ns + " {\n\n";
}

static String NamespaceClose(const String& ns)
{
    return ns.IsEmpty() ? String() : "\n} // namespace " + ns + "\n";
}

static String CrossAlignExpr(const String& value)
{
    if(value == "Stretch" || value == "Fill") return "UiCrossAlign::Stretch";
    if(value == "Start") return "UiCrossAlign::Start";
    if(value == "End") return "UiCrossAlign::End";
    if(value == "Center") return "UiCrossAlign::Center";
    return "UiCrossAlign::Auto";
}

static String LineOrientationExpr(const String& value)
{
    if(value == "Vertical") return "UiSpacerLineOrientation::Vertical";
    if(value == "Horizontal") return "UiSpacerLineOrientation::Horizontal";
    return "UiSpacerLineOrientation::Auto";
}

static String LineDashExpr(const String& value)
{
    if(value == "Dash") return "DASHED";
    if(value == "Dot") return "DOTTED";
    return "SOLID";
}

const UiDesignerGeneratedFile* UiDesignerGeneratedProject::FindFile(
    const String& path) const
{
    for(const UiDesignerGeneratedFile& file : files)
        if(file.relative_path == path)
            return &file;
    return nullptr;
}

bool UiDesignerValidateCppIdentifier(const String& value, String& error)
{
    if(value.IsEmpty()) {
        error = "C++ identifier is empty";
        return false;
    }
    if(!IsAlpha(value[0]) && value[0] != '_') {
        error = "C++ identifier must start with a letter or underscore: " + value;
        return false;
    }
    for(int i = 1; i < value.GetCount(); i++)
        if(!IsAlNum(value[i]) && value[i] != '_') {
            error = "C++ identifier contains an invalid character: " + value;
            return false;
        }
    if(SanitizeIdentifier(value) != value) {
        error = "C++ identifier is reserved or not canonical: " + value;
        return false;
    }
    error.Clear();
    return true;
}

bool UiDesignerValidateGenerationOptions(
    const UiDesignerCodeGenerationOptions& options, String& error)
{
    if(!UiDesignerValidateCppIdentifier(options.package_name, error) ||
       !UiDesignerValidateCppIdentifier(options.class_name, error))
        return false;
    if(!options.namespace_name.IsEmpty()) {
        Vector<String> parts = Split(options.namespace_name, "::");
        for(const String& part : parts)
            if(!UiDesignerValidateCppIdentifier(part, error))
                return false;
    }
    error.Clear();
    return true;
}

String UiDesignerCodeGenerator::MemberName(const UiDesignerNode& node) const
{
    String base = SanitizeIdentifier(node.name.IsEmpty()
                                     ? ToLower(node.type)
                                     : node.name);
    return base + "_n" + AsString(node.id);
}

String UiDesignerCodeGenerator::EmitColor(Color c) const
{
    return Format("Color(%d, %d, %d)", c.GetR(), c.GetG(), c.GetB());
}

String UiDesignerCodeGenerator::EmitValue(const Value& value) const
{
    if(IsNull(value))
        return "Value()";
    if(value.Is<String>())
        return CppString(value);
    if(value.Is<bool>())
        return (bool)value ? "true" : "false";
    if(value.Is<int>() || value.Is<int64>())
        return AsString(value);
    if(value.Is<double>())
        return Format("%.12g", (double)value);
    if(value.Is<Color>())
        return EmitColor((Color)value);
    return "ParseJSON(" + CppString(AsJSON(value, false)) + ")";
}

String UiDesignerCodeGenerator::QualifiedClass(
    const UiDesignerCodeGenerationOptions& options,
    const String& suffix) const
{
    return options.namespace_name.IsEmpty()
        ? options.class_name + suffix
        : options.namespace_name + "::" + options.class_name + suffix;
}

void UiDesignerCodeGenerator::EmitSetup(
    String& out, const UiDesignerNode& node,
    const UiDesignerControlSpec& spec) const
{
    if(spec.IsSemanticItem())
        return;
    const String member = MemberName(node);

    if(spec.runtime_kind == UiDesignerRuntimeKind::UiBoxLayout) {
        const String direction = node.GetProperty("direction", "V");
        out << "\t" << member << ".SetDirection(UiDirection::"
            << (direction == "H" ? "H" : "V") << ");\n";
    }
    if(spec.runtime_kind == UiDesignerRuntimeKind::UiGridLayout)
        out << "\t" << member << ".SetGridSize("
            << (int)node.GetProperty("columns", 2) << ", "
            << (int)node.GetProperty("rows", 2) << ");\n";

    if(const UiDesignerPropertySpec* text = spec.FindProperty("text")) {
        const Value value = node.GetProperty("text", text->default_value);
        if(!IsNull(value)) {
            if(spec.stock_upp)
                out << "\t" << member << ".SetLabel(" << EmitValue(value) << ");\n";
            else
                out << "\t" << member << ".SetText(" << EmitValue(value) << ");\n";
        }
    }
    if(const UiDesignerPropertySpec* title = spec.FindProperty("title"))
        out << "\t" << member << ".SetTitle("
            << EmitValue(node.GetProperty("title", title->default_value))
            << ");\n";
    if(spec.FindProperty("checked"))
        out << "\t" << member << ".SetData("
            << EmitValue(node.GetProperty("checked", false)) << ");\n";
    if(spec.FindProperty("value")) {
        const Value value = node.GetProperty("value", 50);
        if(spec.runtime_kind == UiDesignerRuntimeKind::UiSlider)
            out << "\t" << member << ".SetRange("
                << EmitValue(node.GetProperty("minimum", 0)) << ", "
                << EmitValue(node.GetProperty("maximum", 100))
                << ").SetValue(" << EmitValue(value) << ");\n";
        else if(spec.runtime_kind == UiDesignerRuntimeKind::UiProgressBar)
            out << "\t" << member << ".Set((int)"
                << EmitValue(value) << ", (int)"
                << EmitValue(node.GetProperty("maximum", 100)) << ");\n";
        else
            out << "\t" << member << ".SetData(" << EmitValue(value) << ");\n";
    }
    if(spec.FindProperty("color"))
        out << "\t" << member << ".SetData("
            << EmitValue(node.GetProperty("color", Color(58, 132, 255)))
            << ");\n";
    if(spec.FindProperty("visible"))
        out << "\t" << member << ".Show("
            << EmitValue(node.GetProperty("visible", true)) << ");\n";
    if(spec.FindProperty("enabled"))
        out << "\t" << member << ".Enable("
            << EmitValue(node.GetProperty("enabled", true)) << ");\n";

    const int x = node.GetProperty("x", 20);
    const int y = node.GetProperty("y", 20);
    const int cx = node.GetProperty("width", spec.default_size.cx);
    const int cy = node.GetProperty("height", spec.default_size.cy);
    out << "\t" << member << ".SetRect(DPI(" << x << "), DPI(" << y
        << "), DPI(" << cx << "), DPI(" << cy << "));\n";
}

void UiDesignerCodeGenerator::EmitSpacer(
    String& out, const UiDesignerNode& spacer,
    const UiDesignerNode& parent) const
{
    const String p = MemberName(parent);
    const bool is_break = spacer.GetProperty("layout_break", false);
    String chain;
    if(parent.type == "UiBoxLayout") {
        chain = p + (is_break ? ".AddBreak()" : ".AddSpacer()");
        if(!is_break) {
            const String direction = parent.GetProperty("direction", "V");
            const String main_mode = spacer.GetProperty(
                direction == "H" ? "h_sizing" : "v_sizing", "Auto");
            const int fixed = spacer.GetProperty(
                direction == "H" ? "fixed_width" : "fixed_height", 0);
            const int weight = max(1, (int)(double)spacer.GetProperty("weight", 1.0));
            if(main_mode == "Fixed" && fixed > 0)
                chain << ".Fixed(DPI(" << fixed << "))";
            else
                chain << ".Expand(" << weight << ")";
        }
        const int minw = spacer.GetProperty("min_width", 0);
        const int maxw = spacer.GetProperty("max_width", 0);
        const int minh = spacer.GetProperty("min_height", 0);
        const int maxh = spacer.GetProperty("max_height", 0);
        if(minw || maxw)
            chain << ".MinMaxWidth(DPI(" << minw << "), "
                  << (maxw ? "DPI(" + AsString(maxw) + ")" : "INT_MAX") << ")";
        if(minh || maxh)
            chain << ".MinMaxHeight(DPI(" << minh << "), "
                  << (maxh ? "DPI(" + AsString(maxh) + ")" : "INT_MAX") << ")";
    }
    else if(parent.type == "UiGridLayout") {
        const int row = spacer.GetProperty("grid_row", 0);
        const int column = spacer.GetProperty("grid_column", 0);
        chain = p + ".AddBlank(" + AsString(row) + ", " + AsString(column) + ")";
        const String hs = spacer.GetProperty("h_sizing", "Auto");
        const String vs = spacer.GetProperty("v_sizing", "Auto");
        if(hs == "Fill") chain << ".ExpandX()";
        if(vs == "Fill") chain << ".ExpandY()";
        const int fixedw = spacer.GetProperty("fixed_width", 0);
        const int fixedh = spacer.GetProperty("fixed_height", 0);
        const int minw = spacer.GetProperty("min_width", 0);
        const int minh = spacer.GetProperty("min_height", 0);
        const int maxw = spacer.GetProperty("max_width", 0);
        const int maxh = spacer.GetProperty("max_height", 0);
        if(hs == "Fixed" && fixedw) chain << ".FixedWidth(DPI(" << fixedw << "))";
        if(vs == "Fixed" && fixedh) chain << ".FixedHeight(DPI(" << fixedh << "))";
        if(minw) chain << ".MinWidth(DPI(" << minw << "))";
        if(minh) chain << ".MinHeight(DPI(" << minh << "))";
        if(maxw) chain << ".MaxWidth(DPI(" << maxw << "))";
        if(maxh) chain << ".MaxHeight(DPI(" << maxh << "))";
    }
    else {
        out << "\t// Unsupported Spacer parent: " << parent.type << "\n";
        return;
    }

    if(spacer.GetProperty("line_enabled", false)) {
        chain << ".LineEnabled()"
              << ".LineOrientation("
              << LineOrientationExpr(spacer.GetProperty("line_orientation", "Horizontal"))
              << ")"
              << ".LineAlign("
              << CrossAlignExpr(spacer.GetProperty("line_align", "Center")) << ")"
              << ".LineThickness(DPI("
              << (int)spacer.GetProperty("line_thickness", 1) << "))"
              << ".LineDash("
              << LineDashExpr(spacer.GetProperty("line_dash", "Solid")) << ")"
              << ".LineInset(DPI("
              << (int)spacer.GetProperty("line_inset", 0) << "))";
        if(spacer.GetProperty("line_color_enabled", false))
            chain << ".LineColorEnabled().LineColor("
                  << EmitColor(spacer.GetProperty("line_color", Color(128, 128, 128)))
                  << ")";
    }
    out << "\t" << chain << ";\n";
}

void UiDesignerCodeGenerator::EmitChildren(
    String& out, const UiDesignerDocument& document,
    const UiDesignerNode& node) const
{
    const String parent = node.id == document.GetRootId()
                              ? String() : MemberName(node);
    const UiDesignerControlSpec* parent_spec =
        node.id == document.GetRootId() ? nullptr : catalog_.Find(node.type);

    for(UiDesignerNodeId child_id : node.children) {
        const UiDesignerNode* child = document.Find(child_id);
        if(!child)
            continue;
        const UiDesignerControlSpec* child_spec = catalog_.Find(child->type);
        if(!child_spec)
            continue;
        if(child_spec->IsSemanticItem()) {
            EmitSpacer(out, *child, node);
            continue;
        }

        const String member = MemberName(*child);
        const String title = child->GetProperty(
            "title", child->GetProperty("text", child->name));
        const String adapter = parent_spec ? parent_spec->child_adapter_id : "root";

        if(adapter == "root" || parent.IsEmpty())
            out << "\tAdd(" << member << ");\n";
        else if(adapter == "box")
            out << "\t" << parent << ".Add(" << member << ").Fit();\n";
        else if(adapter == "grid")
            out << "\t" << parent << ".Add(" << member << ", "
                << (int)child->GetProperty("grid_row", 0) << ", "
                << (int)child->GetProperty("grid_column", 0)
                << ", true);\n";
        else if(adapter == "tab" || adapter == "upp_tab")
            out << "\t" << parent << ".Add(" << member << ", "
                << EmitValue(title) << ");\n";
        else if(adapter == "stack")
            out << "\t" << parent << ".Add(" << member << ", "
                << EmitValue(child->name) << ");\n";
        else if(adapter == "accordion") {
            const String section = "section_" + AsString(child->id);
            out << "\tconst int " << section << " = " << parent
                << ".AddSection(" << EmitValue(title) << ", true);\n"
                << "\t" << parent << ".GetSectionContent(" << section
                << ").Add(" << member << ".SizePos());\n";
        }
        else if(adapter == "upp_splitter")
            out << "\t" << parent << " << " << member << ";\n";
        else
            out << "\t" << parent << ".Add(" << member << ");\n";
        EmitChildren(out, document, *child);
    }
}

static String EventLambdaPrefix(const String& event_id)
{
    if(event_id == "WhenSelect")
        return "[=](int, const Value&)";
    return "[=]";
}

static String HandlerIdentifier(const String& name)
{
    return SanitizeIdentifier(name.IsEmpty() ? "OnGeneratedAction" : name);
}

void UiDesignerCodeGenerator::EmitBinding(
    String& out, const UiDesignerDocument& document,
    const UiDesignerNode& node,
    const UiDesignerActionBinding& binding) const
{
    if(!binding.enabled)
        return;
    const String member = MemberName(node);
    String body;
    const UiDesignerNode* target = binding.target
        ? document.Find(binding.target) : nullptr;
    const UiDesignerControlSpec* target_spec = target
        ? catalog_.Find(target->type) : nullptr;
    const String target_member = target ? MemberName(*target) : String();

    switch(binding.action) {
    case UiDesignerActionType::CloseWindow:
    case UiDesignerActionType::ExitApplication:
        body = "Close();";
        break;
    case UiDesignerActionType::AcceptDialog:
        body = "AcceptBreak(IDOK);";
        break;
    case UiDesignerActionType::CancelDialog:
        body = "RejectBreak(IDCANCEL);";
        break;
    case UiDesignerActionType::SetProperty:
        if(!target)
            body = "/* Missing SetProperty target */";
        else if(binding.target_property == "visible")
            body = target_member + ".Show(" + EmitValue(binding.value) + ");";
        else if(binding.target_property == "enabled")
            body = target_member + ".Enable(" + EmitValue(binding.value) + ");";
        else if(binding.target_property == "text" && target_spec && target_spec->stock_upp)
            body = target_member + ".SetLabel(" + EmitValue(binding.value) + ");";
        else if(binding.target_property == "text")
            body = target_member + ".SetText(" + EmitValue(binding.value) + ");";
        else
            body = target_member + ".SetData(" + EmitValue(binding.value) + ");";
        break;
    case UiDesignerActionType::ToggleProperty:
        if(!target)
            body = "/* Missing ToggleProperty target */";
        else if(binding.target_property == "visible")
            body = target_member + ".Show(!" + target_member + ".IsShown());";
        else if(binding.target_property == "enabled")
            body = target_member + ".Enable(!" + target_member + ".IsEnabled());";
        else
            body = target_member + ".SetData(!(bool)" + target_member + ".GetData());";
        break;
    case UiDesignerActionType::AdjustValue:
        body = target
            ? target_member + ".SetData((double)" + target_member +
              ".GetData() + " + Format("%.12g", binding.delta) + ");"
            : "/* Missing AdjustValue target */";
        break;
    case UiDesignerActionType::ActivatePage:
        body = target
            ? target_member + ".SetActivePage((int)" + EmitValue(binding.value) + ");"
            : "/* Missing ActivatePage target */";
        break;
    case UiDesignerActionType::CallNamedHandler:
        body = HandlerIdentifier(binding.handler_name) + "();";
        break;
    }

    out << "\t" << member << "." << binding.event_id << " = "
        << EventLambdaPrefix(binding.event_id) << " { " << body << " };\n";
}

Vector<String> UiDesignerCodeGenerator::CollectHandlers(
    const UiDesignerDocument& document) const
{
    Index<String> handlers;
    for(const UiDesignerNode& node : document.GetNodes())
        for(const UiDesignerActionBinding& binding : node.actions)
            if(binding.action == UiDesignerActionType::CallNamedHandler &&
               !binding.handler_name.IsEmpty())
                handlers.FindAdd(HandlerIdentifier(binding.handler_name));
    Vector<String> result;
    for(int i = 0; i < handlers.GetCount(); i++)
        result.Add(handlers[i]);
    Sort(result);
    return result;
}

String UiDesignerCodeGenerator::GenerateHeader(
    const UiDesignerDocument& document, const String& class_name) const
{
    UiDesignerCodeGenerationOptions options;
    options.package_name = class_name;
    options.class_name = class_name;
    UiDesignerGeneratedProject project = Generate(document, options);
    return project.generated_header;
}

String UiDesignerCodeGenerator::GenerateSource(
    const UiDesignerDocument& document, const String& class_name) const
{
    UiDesignerCodeGenerationOptions options;
    options.package_name = class_name;
    options.class_name = class_name;
    UiDesignerGeneratedProject project = Generate(document, options);
    return project.generated_source;
}

String UiDesignerCodeGenerator::GeneratePackage(const String& package_name) const
{
    return "description \"Generated UiDesigner application\";\n\n"
           "uses\n\tCtrlLib,\n\tUi;\n\n"
           "file\n\t" + package_name + ".generated.h,\n\t" +
           package_name + ".generated.cpp,\n\t" + package_name + ".h,\n\t" +
           package_name + ".cpp,\n\tmain.cpp;\n";
}

UiDesignerGeneratedProject UiDesignerCodeGenerator::Generate(
    const UiDesignerDocument& document, const String& class_name) const
{
    UiDesignerCodeGenerationOptions options;
    options.package_name = class_name;
    options.class_name = class_name;
    return Generate(document, options);
}

UiDesignerGeneratedProject UiDesignerCodeGenerator::Generate(
    const UiDesignerDocument& document,
    const UiDesignerCodeGenerationOptions& options) const
{
    UiDesignerGeneratedProject result;
    String error;
    if(!UiDesignerValidateGenerationOptions(options, error)) {
        result.diagnostics.Add(error);
        return result;
    }
    if(!catalog_.ValidateDocument(document, error)) {
        result.diagnostics.Add(error);
        return result;
    }

    const String base = options.class_name + "Generated";
    const String guard = "_Generated_" +
        SanitizeIdentifier(options.namespace_name + "_" + options.class_name) + "_h_";
    const Vector<String> handlers = CollectHandlers(document);

    String gh;
    gh << "#ifndef " << guard << "\n#define " << guard << "\n\n"
       << "#include <CtrlLib/CtrlLib.h>\n#include <Ui/Ui.h>\n"
       << "#include <Ui/UiColorPicker.h>\n\n"
       << NamespaceOpen(options.namespace_name)
       << "class " << base << " : public TopWindow {\n"
       << "public:\n\ttypedef " << base << " CLASSNAME;\n"
       << "\tvoid BuildGeneratedUi();\n\n"
       << "protected:\n\tvirtual void BindActions() {}\n";
    for(const String& handler : handlers)
        gh << "\tvirtual void " << handler << "() {}\n";
    gh << "\n\tvoid BuildControls();\n\tvoid BuildLayout();\n"
       << "\tvoid BindGeneratedActions();\n\n";
    for(const UiDesignerNode& node : document.GetNodes()) {
        if(node.id == document.GetRootId())
            continue;
        const UiDesignerControlSpec* spec = catalog_.Find(node.type);
        if(!spec || spec->IsSemanticItem())
            continue;
        gh << "\t" << spec->runtime_cpp_type << " " << MemberName(node) << ";\n";
    }
    gh << "};\n" << NamespaceClose(options.namespace_name)
       << "\n#endif\n";

    String gs;
    gs << "#include \"" << options.class_name << ".generated.h\"\n\n"
       << NamespaceOpen(options.namespace_name)
       << "void " << base << "::BuildGeneratedUi()\n{\n"
       << "\tTitle(" << CppString(options.class_name) << ").Sizeable().Zoomable();\n"
       << "\tSetRect(0, 0, DPI(" << document.GetVirtualSize().cx
       << "), DPI(" << document.GetVirtualSize().cy << "));\n"
       << "\tBuildControls();\n\tBuildLayout();\n\tBindGeneratedActions();\n}\n\n"
       << "void " << base << "::BuildControls()\n{\n";
    for(const UiDesignerNode& node : document.GetNodes()) {
        if(node.id == document.GetRootId())
            continue;
        const UiDesignerControlSpec* spec = catalog_.Find(node.type);
        if(spec)
            EmitSetup(gs, node, *spec);
    }
    gs << "}\n\nvoid " << base << "::BuildLayout()\n{\n";
    if(const UiDesignerNode* root = document.Find(document.GetRootId()))
        EmitChildren(gs, document, *root);
    gs << "}\n\nvoid " << base << "::BindGeneratedActions()\n{\n";
    for(const UiDesignerNode& node : document.GetNodes())
        for(const UiDesignerActionBinding& binding : node.actions)
            EmitBinding(gs, document, node, binding);
    gs << "}\n" << NamespaceClose(options.namespace_name);

    const String user_guard = "_" + SanitizeIdentifier(options.class_name) + "_h_";
    String uh;
    uh << "#ifndef " << user_guard << "\n#define " << user_guard << "\n\n"
       << "#include \"" << options.class_name << ".generated.h\"\n\n"
       << NamespaceOpen(options.namespace_name)
       << "class " << options.class_name << " : public " << base << " {\n"
       << "public:\n\ttypedef " << options.class_name << " CLASSNAME;\n"
       << "\t" << options.class_name << "();\n\n"
       << "protected:\n\tvoid BindActions() override;\n";
    for(const String& handler : handlers)
        uh << "\tvoid " << handler << "() override;\n";
    uh << "};\n" << NamespaceClose(options.namespace_name)
       << "\n#endif\n";

    String us;
    us << "#include \"" << options.class_name << ".h\"\n\n"
       << NamespaceOpen(options.namespace_name)
       << options.class_name << "::" << options.class_name << "()\n{\n"
       << "\tBuildGeneratedUi();\n\tBindActions();\n}\n\n"
       << "void " << options.class_name << "::BindActions()\n{\n"
       << "\t// Add application-owned event wiring here. This file is preserved.\n}\n";
    for(const String& handler : handlers)
        us << "\nvoid " << options.class_name << "::" << handler << "()\n{\n"
           << "\t// User-owned named handler.\n}\n";
    us << NamespaceClose(options.namespace_name);

    String main_cpp;
    main_cpp << "#include \"" << options.class_name << ".h\"\n"
             << "using namespace Upp;\n"
             << "GUI_APP_MAIN { "
             << (options.namespace_name.IsEmpty()
                    ? options.class_name
                    : options.namespace_name + "::" + options.class_name)
             << "().Run(); }\n";

    result.generated_header = gh;
    result.generated_source = gs;
    result.user_header = uh;
    result.user_source = us;
    result.main_source = main_cpp;
    result.header = gh;
    result.source = gs;
    result.package = GeneratePackage(options.package_name);
    result.json = UiDesignerSerialize(document, true);

    auto AddFile = [&](const String& path, const String& content,
                       bool generated) {
        UiDesignerGeneratedFile& file = result.files.Add();
        file.relative_path = path;
        file.content = content;
        file.generator_owned = generated;
    };
    AddFile(options.class_name + ".generated.h", gh, true);
    AddFile(options.class_name + ".generated.cpp", gs, true);
    AddFile(options.class_name + ".h", uh, false);
    AddFile(options.class_name + ".cpp", us, false);
    AddFile("main.cpp", main_cpp, false);
    AddFile(options.package_name + ".upp", result.package, true);
    if(options.include_source_design)
        AddFile("design.json", result.json, true);
    return result;
}

static bool RemoveTree(const String& path)
{
    if(!DirectoryExists(path))
        return true;
    return DeleteFolderDeep(path);
}

bool UiDesignerWriteGeneratedProject(
    const String& folder, const UiDesignerGeneratedProject& project,
    const UiDesignerExportWriteOptions& options,
    Vector<String>& written_files, String& error)
{
    written_files.Clear();
    if(!project.IsValid()) {
        error = project.diagnostics.IsEmpty()
            ? "Generated project is invalid"
            : Join(project.diagnostics, "\n");
        return false;
    }
    if(folder.IsEmpty()) {
        error = "Export folder is empty";
        return false;
    }

    for(const UiDesignerGeneratedFile& file : project.files) {
        const String destination = AppendFileName(folder, file.relative_path);
        if(FileExists(destination)) {
            if(!file.generator_owned && options.preserve_user_files)
                continue;
            if(options.overwrite == UiDesignerOverwritePolicy::RefuseExisting) {
                error = "Export would replace existing file: " + destination;
                return false;
            }
        }
    }

    const String stage = AppendFileName(
        folder, ".uidesigner-stage-" + AsString(Uuid::Create()));
    if(!RealizeDirectory(stage)) {
        error = "Unable to create export staging directory";
        return false;
    }

    bool ok = true;
    for(const UiDesignerGeneratedFile& file : project.files) {
        const String path = AppendFileName(stage, file.relative_path);
        RealizeDirectory(GetFileFolder(path));
        if(!SaveFile(path, file.content)) {
            error = "Unable to stage " + file.relative_path;
            ok = false;
            break;
        }
    }

    if(ok) {
        RealizeDirectory(folder);
        for(const UiDesignerGeneratedFile& file : project.files) {
            const String destination = AppendFileName(folder, file.relative_path);
            if(FileExists(destination) && !file.generator_owned &&
               options.preserve_user_files)
                continue;
            const String staged = AppendFileName(stage, file.relative_path);
            const String temporary = destination + ".uidesigner-tmp-" +
                                     AsString(Uuid::Create());
            RealizeDirectory(GetFileFolder(destination));
            if(!FileCopy(staged, temporary)) {
                error = "Unable to prepare " + destination;
                ok = false;
                break;
            }
            if(FileExists(destination) && !DeleteFile(destination)) {
                DeleteFile(temporary);
                error = "Unable to replace " + destination;
                ok = false;
                break;
            }
            if(!FileMove(temporary, destination)) {
                DeleteFile(temporary);
                error = "Unable to publish " + destination;
                ok = false;
                break;
            }
            written_files.Add(destination);
        }
    }

    RemoveTree(stage);
    if(!ok)
        return false;
    error.Clear();
    return true;
}

bool UiDesignerWriteGeneratedProject(
    const String& folder, const String& package_name,
    const UiDesignerGeneratedProject& project, String& error)
{
    UiDesignerExportWriteOptions options;
    options.overwrite = UiDesignerOverwritePolicy::ReplaceGenerated;
    Vector<String> written;
    return UiDesignerWriteGeneratedProject(folder, project, options,
                                           written, error);
}

}
