#include "DesignerCodeGen.h"

namespace Upp {

static Value CodeGenNodeProperty(const DesignerNode& n, const String& key, const Value& def)
{
	int q = n.properties.Find(key);
	return q >= 0 ? n.properties.GetValue(q) : def;
}

static String CppString(const String& s)
{
	String out = "\"";
	for(int i = 0; i < s.GetCount(); i++) {
		byte c = s[i];
		if(c == '\\')
			out << "\\\\";
		else if(c == '"')
			out << "\\\"";
		else if(c == '\n')
			out << "\\n";
		else if(c == '\r')
			out << "\\r";
		else if(c == '\t')
			out << "\\t";
		else
			out.Cat(c);
	}
	out << "\"";
	return out;
}

static String VarName(DesignerNodeId id)
{
	return "node" + AsString(id);
}

static String DirectionExpr(const DesignerNode& n, const String& def)
{
	return CodeGenNodeProperty(n, "direction", def) == "H" ? "UiDirection::H" : "UiDirection::V";
}

static String GridMode(const DesignerNode& n)
{
	String mode = CodeGenNodeProperty(n, "mode", "Flow");
	return mode == "Grid" ? "Grid" : "Flow";
}

static String ColorExpr(Color c)
{
	if(IsNull(c))
		return "Null";
	return Format("Color(%d, %d, %d)", c.GetR(), c.GetG(), c.GetB());
}

static String SizingCall(const DesignerNode& n)
{
	String sizing = CodeGenNodeProperty(n, "sizing", "Fit");
	if(sizing == "Fixed") {
		int h = max(10, (int)CodeGenNodeProperty(n, "height", 32));
		return Format(".Fixed(DPI(%d))", h);
	}
	if(sizing == "Expand")
		return ".Expand(1)";
	return ".Fit()";
}

static void EmitDeclaration(String& out, const DesignerNode& n)
{
	String var = VarName(n.id);
	if(n.type_id == "BoxLayout")
		out << "\tUiBoxLayout " << var << ";\n";
	else if(n.type_id == "GridLayout")
		out << "\tUiGridLayout " << var << ";\n";
	else if(n.type_id == "UiLabel")
		out << "\tUiLabel " << var << ";\n";
	else if(n.type_id == "UiTitleCard")
		out << "\tUiTitleCard " << var << ";\n";
	else if(n.type_id == "UiButton")
		out << "\tUiButton " << var << ";\n";
	else if(n.type_id == "UiLineEdit")
		out << "\tUiLineEdit " << var << ";\n";
	else if(n.type_id == "UiSlider")
		out << "\tUiSlider " << var << ";\n";
	else if(n.type_id == "UiToggle")
		out << "\tUiToggle " << var << ";\n";
	else if(n.type_id == "UiDropdown")
		out << "\tUiDropdown " << var << ";\n";
	else
		out << "\tUiPanel " << var << ";\n";
}

static void EmitSetup(String& out, const DesignerNode& n, bool emit_designer_appearance)
{
	String var = VarName(n.id);
	if(emit_designer_appearance && n.type_id != "Window") {
		Color face = CodeGenNodeProperty(n, "face", Null);
		Color frame = CodeGenNodeProperty(n, "frame", Null);
		int radius = max(0, (int)CodeGenNodeProperty(n, "radius", 0));
		out << "\t\t// Designer appearance for " << n.name << ": face=" << ColorExpr(face)
		    << ", frame=" << ColorExpr(frame) << ", radius=" << radius << "\n";
	}
	if(n.type_id == "BoxLayout") {
		out << "\t\t" << var << ".SetDirection(" << DirectionExpr(n, "V") << ")"
		    << ".SetGap(DPI(" << (int)CodeGenNodeProperty(n, "gap", 8) << "))"
		    << ".SetInset(DPI(" << (int)CodeGenNodeProperty(n, "inset", 8) << "))";
		if((bool)CodeGenNodeProperty(n, "wrap", false))
			out << ".SetWrap(true)";
		if((bool)CodeGenNodeProperty(n, "debug", false))
			out << ".SetDebug(true)";
		out << ";\n";
	}
	else if(n.type_id == "GridLayout") {
		String mode = GridMode(n);
		out << "\t\t" << var << ".SetMode(UiGridLayout::" << mode << ")"
		    << ".SetDirection(" << DirectionExpr(n, "H") << ")"
		    << ".SetGap(DPI(" << (int)CodeGenNodeProperty(n, "gap", 8) << "))"
		    << ".SetInset(DPI(" << (int)CodeGenNodeProperty(n, "inset", 8) << "))";
		if(mode == "Flow")
			out << ".SetWrap(" << ((bool)CodeGenNodeProperty(n, "wrap", true) ? "true" : "false") << ")";
		if(mode == "Flow" && (bool)CodeGenNodeProperty(n, "align_cells", true)) {
			int cell_w = max(10, (int)CodeGenNodeProperty(n, "cell_width", 120));
			int cell_h = max(10, (int)CodeGenNodeProperty(n, "cell_height", 32));
			out << ".SetUnifiedItemSize(Size(DPI(" << cell_w << "), DPI(" << cell_h << ")))";
		}
		if((bool)CodeGenNodeProperty(n, "debug", false))
			out << ".SetDebug(true)";
		out << ";\n";
	}
	else if(n.type_id == "UiLabel")
		out << "\t\t" << var << ".SetText(" << CppString(CodeGenNodeProperty(n, "text", n.name)) << ");\n";
	else if(n.type_id == "UiTitleCard")
		out << "\t\t" << var << ".SetTitle(" << CppString(CodeGenNodeProperty(n, "text", n.name)) << ");\n";
	else if(n.type_id == "UiButton")
		out << "\t\t" << var << ".SetText(" << CppString(CodeGenNodeProperty(n, "text", n.name)) << ");\n";
	else if(n.type_id == "UiLineEdit") {
		out << "\t\t" << var << ".SetTextUtf8(" << CppString(CodeGenNodeProperty(n, "text", n.name)) << ");\n";
		String placeholder = CodeGenNodeProperty(n, "placeholder", "");
		if(!placeholder.IsEmpty())
			out << "\t\t" << var << ".SetPlaceholder(" << CppString(placeholder) << ");\n";
	}
	else if(n.type_id == "UiSlider")
		out << "\t\t" << var << ".SetRange(0, 100).SetValue(50);\n";
	else if(n.type_id == "UiToggle")
		out << "\t\t" << var << ".SetOn(" << ((bool)CodeGenNodeProperty(n, "on", true) ? "true" : "false") << ");\n";
	else if(n.type_id == "UiDropdown") {
		out << "\t\t" << var << ".UseInternalModel().Add(\"First\", \"First\").Add(\"Second\", \"Second\").Add(\"Third\", \"Third\");\n";
		out << "\t\t" << var << ".SetData(" << CppString(CodeGenNodeProperty(n, "selected", "First")) << ");\n";
	}
	else {
		out << "\t\t" << var << ".SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle));\n";
	}
}

static void EmitAddChild(String& out, const DesignerNode& parent, const DesignerNode& child, int index)
{
	String p = VarName(parent.id);
	String c = VarName(child.id);
	if(parent.id == Designer_ROOT)
		out << "\t\tAdd(" << c << ".SizePos());\n";
	else if(parent.type_id == "BoxLayout")
		out << "\t\t" << p << ".Add(" << c << ")" << SizingCall(child) << ";\n";
	else if(parent.type_id == "GridLayout") {
		if(GridMode(parent) == "Grid") {
			int columns = max(1, (int)CodeGenNodeProperty(parent, "columns", 2));
			int row = index / columns;
			int col = index % columns;
			out << "\t\t" << p << ".AddGrid(" << c << ", " << row << ", " << col << ");\n";
		}
		else {
			String sizing = CodeGenNodeProperty(child, "sizing", "Fit");
			if(sizing == "Fixed") {
				int w = max(10, (int)CodeGenNodeProperty(child, "width", 120));
				int h = max(10, (int)CodeGenNodeProperty(child, "height", 32));
				out << "\t\t" << p << ".Add(" << c << ", -1, false, Size(DPI(" << w << "), DPI(" << h << ")));\n";
			}
			else
				out << "\t\t" << p << ".Add(" << c << ");\n";
		}
	}
}

static void EmitAdds(String& out, const DesignerModel& model, const DesignerNode& parent)
{
	for(int i = 0; i < parent.children.GetCount(); i++) {
		DesignerNodeId child_id = parent.children[i];
		const DesignerNode* child = model.Find(child_id);
		if(!child)
			continue;
		EmitAddChild(out, parent, *child, i);
		EmitAdds(out, model, *child);
	}
}

String GenerateDesignerCode(const DesignerModel& model, const DesignerRegistry& registry,
                              const String& class_name, bool emit_designer_appearance)
{
	(void)registry;
	String out;
	out << "#include <CtrlLib/CtrlLib.h>\n"
	    << "#include <Ui/Ui.h>\n\n"
	    << "using namespace Upp;\n\n"
	    << "class " << class_name << " : public TopWindow {\n"
	    << "public:\n"
	    << "\ttypedef " << class_name << " CLASSNAME;\n\n"
	    << "\t" << class_name << "()\n"
	    << "\t{\n"
	    << "\t\tTitle(\"Generated Designer Layout\");\n"
	    << "\t\tSizeable().Zoomable();\n";
	Size sz = model.GetVirtualSize();
	out << "\t\tSetRect(0, 0, DPI(" << sz.cx << "), DPI(" << sz.cy << "));\n"
	    << "\t\tBuild();\n"
	    << "\t}\n\n"
	    << "private:\n"
	    << "\tvoid Build()\n"
	    << "\t{\n";
	for(const DesignerNode& n : model.GetNodes()) {
		if(n.id == Designer_ROOT)
			continue;
		EmitSetup(out, n, emit_designer_appearance);
	}
	const DesignerNode* root = model.Find(Designer_ROOT);
	if(root)
		EmitAdds(out, model, *root);
	out << "\t}\n\n";
	for(const DesignerNode& n : model.GetNodes()) {
		if(n.id == Designer_ROOT)
			continue;
		EmitDeclaration(out, n);
	}
	out << "};\n\n"
	    << "GUI_APP_MAIN\n"
	    << "{\n"
	    << "\t" << class_name << "().Run();\n"
	    << "}\n";
	return out;
}

}
