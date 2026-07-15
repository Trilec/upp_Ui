#include "UiDesignerCodeGen.h"

namespace Upp {

static String SanitizeIdentifier(String value)
{
    if(value.IsEmpty())
        value = "control";
    for(int i = 0; i < value.GetCount(); i++)
        if(!IsAlNum(value[i]) && value[i] != '_')
            value.Set(i, '_');
    if(IsDigit(value[0]))
        value = "_" + value;
    return value;
}

String UiDesignerCodeGenerator::MemberName(const UiDesignerNode& node) const
{
    String base = SanitizeIdentifier(node.name.IsEmpty()
                                     ? ToLower(node.type)
                                     : node.name);
    return base + "_n" + AsString(node.id);
}

String UiDesignerCodeGenerator::EmitValue(const Value& value) const
{
    if(IsNull(value))
        return "Value()";
    if(value.Is<String>())
        return AsCString((String)value);
    if(value.Is<bool>())
        return (bool)value ? "true" : "false";
    if(value.Is<int>() || value.Is<int64>())
        return AsString(value);
    if(value.Is<double>())
        return Format("%.12g", (double)value);
    if(value.Is<Color>()) {
        Color c = value;
        return Format("Color(%d, %d, %d)", c.GetR(), c.GetG(), c.GetB());
    }
    return "ParseJSON(\"" + AsCString(AsJSON(value)) + "\")";
}

void UiDesignerCodeGenerator::EmitSetup(
    String& out, const UiDesignerNode& node,
    const UiDesignerControlSpec& spec) const
{
    const String member = MemberName(node);
    const ValueMap& p = node.properties;

    if(const UiDesignerPropertySpec* text = spec.FindProperty("text")) {
        const Value value = node.GetProperty("text", text->default_value);
        if(!IsNull(value)) {
            if(spec.runtime_kind == UiDesignerRuntimeKind::UppLabel ||
               spec.runtime_kind == UiDesignerRuntimeKind::UppButton ||
               spec.runtime_kind == UiDesignerRuntimeKind::UppOption)
                out << "\t" << member << ".SetLabel(" << EmitValue(value) << ");\n";
            else
                out << "\t" << member << ".SetText(" << EmitValue(value) << ");\n";
        }
    }
    if(const UiDesignerPropertySpec* title = spec.FindProperty("title")) {
        const Value value = node.GetProperty("title", title->default_value);
        out << "\t" << member << ".SetTitle(" << EmitValue(value) << ");\n";
    }
    if(spec.FindProperty("checked"))
        out << "\t" << member << ".SetData("
            << EmitValue(node.GetProperty("checked", false)) << ");\n";
    if(spec.FindProperty("value")) {
        Value value = node.GetProperty("value", 50);
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

void UiDesignerCodeGenerator::EmitChildren(
    String& out, const UiDesignerDocument& document,
    const UiDesignerNode& node) const
{
    const String parent = node.id == document.GetRootId()
                              ? String()
                              : MemberName(node);
    const UiDesignerControlSpec* parent_spec =
        node.id == document.GetRootId() ? nullptr : catalog_.Find(node.type);

    for(UiDesignerNodeId child_id : node.children) {
        const UiDesignerNode* child = document.Find(child_id);
        if(!child)
            continue;
        const String member = MemberName(*child);
        const String title = child->GetProperty(
            "title", child->GetProperty("text", child->name));

        if(parent.IsEmpty())
            out << "\tAdd(" << member << ");\n";
        else if(parent_spec &&
                parent_spec->runtime_kind == UiDesignerRuntimeKind::UiTab)
            out << "\t" << parent << ".Add(" << member << ", "
                << EmitValue(title) << ");\n";
        else if(parent_spec &&
                parent_spec->runtime_kind == UiDesignerRuntimeKind::UiStack)
            out << "\t" << parent << ".Add(" << member << ", "
                << EmitValue(child->name) << ");\n";
        else if(parent_spec &&
                parent_spec->runtime_kind == UiDesignerRuntimeKind::UiAccordion) {
            const String section = "section_" + AsString(child->id);
            out << "\tconst int " << section << " = " << parent
                << ".AddSection(" << EmitValue(title) << ", true);\n";
            out << "\t" << parent << ".GetSectionContent(" << section
                << ").Add(" << member << ".SizePos());\n";
        }
        else
            out << "\t" << parent << ".Add(" << member << ");\n";
        EmitChildren(out, document, *child);
    }
}

String UiDesignerCodeGenerator::GenerateHeader(
    const UiDesignerDocument& document, const String& class_name) const
{
    String out;
    out << "#ifndef _Generated_" << class_name << "_h_\n"
        << "#define _Generated_" << class_name << "_h_\n\n"
        << "#include <CtrlLib/CtrlLib.h>\n"
        << "#include <Ui/Ui.h>\n\n"
        << "namespace Upp {\n\n"
        << "class " << class_name << " : public TopWindow {\n"
        << "public:\n"
        << "\ttypedef " << class_name << " CLASSNAME;\n"
        << "\t" << class_name << "();\n\n"
        << "private:\n"
        << "\tvoid BuildControls();\n"
        << "\tvoid BuildLayout();\n\n";

    for(const UiDesignerNode& node : document.GetNodes()) {
        if(node.id == document.GetRootId())
            continue;
        const UiDesignerControlSpec* spec = catalog_.Find(node.type);
        if(!spec)
            continue;
        out << "\t" << spec->runtime_cpp_type << " "
            << MemberName(node) << ";\n";
    }

    out << "};\n\n}\n\n#endif\n";
    return out;
}

String UiDesignerCodeGenerator::GenerateSource(
    const UiDesignerDocument& document, const String& class_name) const
{
    String out;
    out << "#include \"" << class_name << ".h\"\n\n"
        << "namespace Upp {\n\n"
        << class_name << "::" << class_name << "()\n"
        << "{\n"
        << "\tTitle(\"" << class_name << "\").Sizeable().Zoomable();\n"
        << "\tSetRect(0, 0, DPI(" << document.GetVirtualSize().cx
        << "), DPI(" << document.GetVirtualSize().cy << "));\n"
        << "\tBuildControls();\n"
        << "\tBuildLayout();\n"
        << "}\n\n"
        << "void " << class_name << "::BuildControls()\n"
        << "{\n";

    for(const UiDesignerNode& node : document.GetNodes()) {
        if(node.id == document.GetRootId())
            continue;
        const UiDesignerControlSpec* spec = catalog_.Find(node.type);
        if(spec)
            EmitSetup(out, node, *spec);
    }

    out << "}\n\n"
        << "void " << class_name << "::BuildLayout()\n"
        << "{\n";

    const UiDesignerNode* root = document.Find(document.GetRootId());
    if(root)
        EmitChildren(out, document, *root);

    out << "}\n\n}\n";
    return out;
}

String UiDesignerCodeGenerator::GeneratePackage(
    const String& package_name) const
{
    return "description \"Generated UiDesigner application\";\n\n"
           "uses\n\tCtrlLib,\n\tUi;\n\n"
           "file\n\t" + package_name + ".h,\n\t" +
           package_name + ".cpp,\n\tmain.cpp;\n";
}

UiDesignerGeneratedProject UiDesignerCodeGenerator::Generate(
    const UiDesignerDocument& document, const String& class_name) const
{
    UiDesignerGeneratedProject result;
    result.header = GenerateHeader(document, class_name);
    result.source = GenerateSource(document, class_name);
    result.package = GeneratePackage(class_name);
    result.json = UiDesignerSerialize(document, true);
    return result;
}

bool UiDesignerWriteGeneratedProject(
    const String& folder, const String& package_name,
    const UiDesignerGeneratedProject& project, String& error)
{
    RealizeDirectory(folder);
    if(!SaveFile(AppendFileName(folder, package_name + ".h"), project.header) ||
       !SaveFile(AppendFileName(folder, package_name + ".cpp"), project.source) ||
       !SaveFile(AppendFileName(folder, package_name + ".upp"), project.package) ||
       !SaveFile(AppendFileName(folder, "design.json"), project.json)) {
        error = "Unable to write generated project to " + folder;
        return false;
    }

    const String main_cpp =
        "#include \"" + package_name + ".h\"\n"
        "using namespace Upp;\n"
        "GUI_APP_MAIN { " + package_name + "().Run(); }\n";
    if(!SaveFile(AppendFileName(folder, "main.cpp"), main_cpp)) {
        error = "Unable to write generated main.cpp";
        return false;
    }
    error.Clear();
    return true;
}

}
