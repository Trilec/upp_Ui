#include "DesignerCodeGen.h"

// DesignerCodeGen.cpp - converts the model tree into standalone U++ code.
// Generated output should be theme-first: emit layout/control API calls and
// only include explicit appearance when the caller requests designer metadata.

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

static String CodeIdentifier(String text)
{
	text = TrimBoth(text);
	String out;
	bool last_us = false;
	for(int i = 0; i < text.GetCount(); i++) {
		int c = text[i];
		bool ok = IsAlNum(c) || c == '_';
		if(ok) {
			if(out.IsEmpty() && IsDigit(c))
				out << '_';
			out.Cat(c);
			last_us = false;
		}
		else if(!last_us && !out.IsEmpty()) {
			out << '_';
			last_us = true;
		}
	}
	while(out.GetCount() && out[out.GetCount() - 1] == '_')
		out.Trim(out.GetCount() - 1);
	if(out.IsEmpty())
		out = "node";
	static const char *reserved[] = {
		"class", "private", "public", "protected", "template", "typename", "operator",
		"int", "double", "float", "bool", "char", "void", "auto", "return", "new", "delete"
	};
	for(const char *r : reserved)
		if(out == r)
			return "_" + out;
	return out;
}

static bool NameUsed(const VectorMap<DesignerNodeId, String>& names, const String& name)
{
	for(int i = 0; i < names.GetCount(); i++)
		if(names[i] == name)
			return true;
	return false;
}

static VectorMap<DesignerNodeId, String> BuildCodeNames(const DesignerModel& model)
{
	VectorMap<DesignerNodeId, String> names;
	for(const DesignerNode& n : model.GetNodes()) {
		if(n.id == Designer_ROOT)
			continue;
		String root = CodeIdentifier(n.name);
		String name = root;
		int suffix = 2;
		while(NameUsed(names, name))
			name = Format("%s_%02d", root, suffix++);
		names.Add(n.id, name);
	}
	return names;
}

static String VarName(const VectorMap<DesignerNodeId, String>& names, DesignerNodeId id)
{
	int q = names.Find(id);
	return q >= 0 ? names[q] : VarName(id);
}

static String DirectionExpr(const DesignerNode& n, const String& def)
{
	return CodeGenNodeProperty(n, "direction", def) == "H" ? "UiDirection::H" : "UiDirection::V";
}

static String AlignSideExpr(const String& side, const String& def = "Left")
{
	String s = side.IsEmpty() ? def : side;
	if(s == "Right")
		return "UiAlign::RIGHT";
	if(s == "Top")
		return "UiAlign::TOP";
	if(s == "Bottom")
		return "UiAlign::BOTTOM";
	return "UiAlign::LEFT";
}

static String TabVisualExpr(const String& visual)
{
	if(visual == "Classic")
		return "UITAB_CLASSIC";
	if(visual == "Document")
		return "UITAB_DOCUMENT";
	if(visual == "Segmented")
		return "UITAB_SEGMENTED";
	if(visual == "Rail")
		return "UITAB_RAIL";
	return "UITAB_UNDERLINE";
}

static String FontExpr(const String& family, int size)
{
	int z = max(7, size);
	if(family == "Mono")
		return Format("MonospaceZ(%d)", z);
	if(family == "Serif")
		return Format("SerifZ(%d)", z);
	if(family == "Segoe UI" || family == "Arial" || family == "Verdana" ||
	   family == "Tahoma" || family == "Consolas")
		return Format("Font().FaceName(%s).Height(%d)", CppString(family), z);
	return Format("SansSerifZ(%d)", z);
}

static String ColorExpr(Color c)
{
	if(IsNull(c))
		return "Null";
	return Format("Color(%d, %d, %d)", c.GetR(), c.GetG(), c.GetB());
}

static Color CodeGenDebugColor(const DesignerNode& n)
{
	if(!(bool)CodeGenNodeProperty(n, "debug_auto_color", false)) {
		Value v = CodeGenNodeProperty(n, "debug_color", Color(220, 38, 38));
		return IsNull(v) ? Color(220, 38, 38) : (Color)v;
	}
	static const Color palette[] = {
		Color(220, 38, 38), Color(217, 119, 6), Color(37, 99, 235),
		Color(22, 163, 74), Color(147, 51, 234), Color(8, 145, 178),
		Color(219, 39, 119)
	};
	return palette[abs((int)n.id) % (int)(sizeof(palette) / sizeof(palette[0]))];
}

static String IconExpr(const String& icon)
{
	if(icon == "None")
		return String();
	const Vector<UiIconCatalogEntry>& catalog = UiIconCatalog();
	for(int i = 0; i < catalog.GetCount(); i++)
		if(icon == catalog[i].name)
			return icon + "()";
	if(icon == "Home") return "ICON_DESIGN_HOME_48()";
	if(icon == "Settings") return "ICON_DESIGN_SETTINGS_48()";
	if(icon == "Menu") return "ICON_DESIGN_MENU_48()";
	if(icon == "Search") return "ICON_ACTION_SEARCH_48()";
	if(icon == "Add") return "ICON_CONTENT_OUTLINED_ADD_48()";
	if(icon == "Check") return "ICON_ACTION_CHECK_CIRCLE_48()";
	if(icon == "Folder") return "ICON_DESIGN_FOLDER_48()";
	if(icon == "Image") return "ICON_DESIGN_IMAGE_48()";
	return String();
}

static void EmitDeclaration(String& out, const VectorMap<DesignerNodeId, String>& names, const DesignerNode& n)
{
	String var = VarName(names, n.id);
	if(n.type_id == "BoxLayout")
		out << "\tUiBoxLayout " << var << ";\n";
	else if(n.type_id == "GridLayout")
		out << "\tUiGridLayout " << var << ";\n";
	else if(n.type_id == "UiSplitter")
		out << "\tUiSplitter " << var << ";\n";
	else if(n.type_id == "UiQuadSplitter")
		out << "\tUiQuadSplitter " << var << ";\n";
	else if(n.type_id == "UiLabel")
		out << "\tUiLabel " << var << ";\n";
	else if(n.type_id == "UiTitleCard")
		out << "\tUiTitleCard " << var << ";\n";
	else if(n.type_id == "UiButton")
		out << "\tUiButton " << var << ";\n";
	else if(n.type_id == "UiLineEdit")
		out << "\tUiLineEdit " << var << ";\n";
	else if(n.type_id == "UiIntEdit")
		out << "\tUiIntEdit " << var << ";\n";
	else if(n.type_id == "UiFloatEdit")
		out << "\tUiFloatEdit " << var << ";\n";
	else if(n.type_id == "UiSlider")
		out << "\tUiSlider " << var << ";\n";
	else if(n.type_id == "UiToggle")
		out << "\tUiToggle " << var << ";\n";
	else if(n.type_id == "UiDropdown")
		out << "\tUiDropdown " << var << ";\n";
	else if(n.type_id == "UiCheckBox")
		out << "\tUiCheckBox " << var << ";\n";
	else if(n.type_id == "UiBreadcrumbs")
		out << "\tUiBreadcrumbs " << var << ";\n";
	else if(n.type_id == "UiTab")
		out << "\tUiTab " << var << ";\n";
	else if(n.type_id == "UiStack")
		out << "\tUiStack " << var << ";\n";
	else if(n.type_id == "UiTable")
		out << "\tUiTable " << var << ";\n";
	else if(n.type_id == "UiTree")
		out << "\tUiTree " << var << ";\n";
	else if(n.type_id == "UiScrollPanel")
		out << "\tUiScrollPanel " << var << ";\n";
	else if(n.type_id == "PaneSlot" || n.type_id == "PageSlot")
		out << "\tParentCtrl " << var << ";\n";
	else if(n.type_id == "Spacer")
		return;
	else
		out << "\tUiPanel " << var << ";\n";
}

static String AxisSizing(const DesignerNode& n, const String& axis_key)
{
	return CodeGenNodeProperty(n, axis_key, "Fit");
}

static String BoxSizingCall(const DesignerNode& parent, const DesignerNode& child)
{
	bool horizontal = CodeGenNodeProperty(parent, "direction", "V") == "H";
	String sizing = AxisSizing(child, horizontal ? "h_sizing" : "v_sizing");
	if(sizing == "Fixed") {
		int v = horizontal ? max(10, (int)CodeGenNodeProperty(child, "width", 120))
		                   : max(10, (int)CodeGenNodeProperty(child, "height", 32));
		return Format(".Fixed(DPI(%d))", v);
	}
	if(sizing == "Expand")
		return ".Expand(1)";
	return ".Fit()";
}

static void EmitSetup(String& out, const VectorMap<DesignerNodeId, String>& names,
                      const DesignerNode& n, bool emit_designer_appearance)
{
	String var = VarName(names, n.id);
	if(n.type_id == "PaneSlot" || n.type_id == "PageSlot" || n.type_id == "Spacer")
		return;
	if(emit_designer_appearance && n.type_id != "Window" && n.type_id != "PaneSlot" && !(bool)CodeGenNodeProperty(n, "pane_slot", false)) {
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
			out << ".SetDebugColor(" << ColorExpr(CodeGenDebugColor(n)) << ").SetDebug(true)";
		out << ";\n";
	}
	else if(n.type_id == "GridLayout") {
		out << "\t\t" << var << ".SetGridSize("
		    << max(1, (int)CodeGenNodeProperty(n, "columns", 2)) << ", "
		    << max(1, (int)CodeGenNodeProperty(n, "rows", 2)) << ")"
		    << ".SetMinCellSize(Size(DPI(" << max(6, (int)CodeGenNodeProperty(n, "cell_width", 120))
		    << "), DPI(" << max(6, (int)CodeGenNodeProperty(n, "cell_height", 32)) << ")))"
		    << ".SetGap(DPI(" << (int)CodeGenNodeProperty(n, "gap", 8) << "))"
		    << ".SetInset(DPI(" << (int)CodeGenNodeProperty(n, "inset", 8) << "))";
		if((bool)CodeGenNodeProperty(n, "debug", false))
			out << ".SetDebugColor(" << ColorExpr(CodeGenDebugColor(n)) << ").SetDebug(true)";
		out << ";\n";
	}
	else if(n.type_id == "UiSplitter") {
		String dir = CodeGenNodeProperty(n, "direction", "H");
		out << "\t\t" << var << "." << (dir == "V" ? "Vert" : "Horz") << "();\n";
		out << "\t\t" << var << ".SetMinPixels(0, DPI(" << (int)CodeGenNodeProperty(n, "min_a", 80) << "))"
		    << ".SetMinPixels(1, DPI(" << (int)CodeGenNodeProperty(n, "min_b", 80) << "))"
		    << ".SetSplitPercent(" << (int)CodeGenNodeProperty(n, "split_percent", 50) << ");\n";
		out << "\t\t{\n"
		    << "\t\t\tUiSplitter::Style s = UiTheme::ResolveSplitter();\n"
		    << "\t\t\ts.hit_width = DPI(" << (int)CodeGenNodeProperty(n, "hit_width", 14) << ");\n"
		    << "\t\t\ts.track_thickness = DPI(" << (int)CodeGenNodeProperty(n, "track_thickness", 2) << ");\n"
		    << "\t\t\tint track_inset = DPI(" << (int)CodeGenNodeProperty(n, "track_inset", 0) << ");\n"
		    << "\t\t\ts.track_inset = Rect(track_inset, track_inset, track_inset, track_inset);\n";
		int thumb_w = (int)CodeGenNodeProperty(n, "thumb_width", 14);
		int thumb_h = (int)CodeGenNodeProperty(n, "thumb_height", 64);
		if(dir == "V") {
			out << "\t\t\ts.thumb_main = DPI(" << thumb_w << ");\n"
			    << "\t\t\ts.thumb_cross = DPI(" << thumb_h << ");\n"
			    << "\t\t\ts.thumb_icon = ICON_NAVIGATION_OUTLINED_MORE_VERT_48();\n";
		}
		else {
			out << "\t\t\ts.thumb_main = DPI(" << thumb_h << ");\n"
			    << "\t\t\ts.thumb_cross = DPI(" << thumb_w << ");\n"
			    << "\t\t\ts.thumb_icon = ICON_NAVIGATION_OUTLINED_MORE_HORIZ_48();\n";
		}
		out << "\t\t\ts.thumb_metrics.radius = DPI(" << (int)CodeGenNodeProperty(n, "thumb_radius", 8) << ");\n"
		    << "\t\t\t" << var << ".SetCustomStyle(s);\n"
		    << "\t\t}\n";
	}
	else if(n.type_id == "UiQuadSplitter") {
		out << "\t\t" << var << ".SetSplitPercent("
		    << (int)CodeGenNodeProperty(n, "column_percent", 50) << ", "
		    << (int)CodeGenNodeProperty(n, "row_percent", 50) << ")"
		    << ".SetMinPixels(0, DPI(" << (int)CodeGenNodeProperty(n, "min_a", 60) << "))"
		    << ".SetMinPixels(1, DPI(" << (int)CodeGenNodeProperty(n, "min_b", 60) << "))"
		    << ".SetMinPixels(2, DPI(" << (int)CodeGenNodeProperty(n, "min_c", 60) << "))"
		    << ".SetMinPixels(3, DPI(" << (int)CodeGenNodeProperty(n, "min_d", 60) << "));\n";
	}
	else if(n.type_id == "UiLabel") {
		out << "\t\t" << var << ".SetText(" << CppString(CodeGenNodeProperty(n, "text", n.name)) << ");\n";
		String icon = IconExpr(CodeGenNodeProperty(n, "icon", "None"));
		if(!icon.IsEmpty())
			out << "\t\t" << var << ".SetIcon(" << icon << ", UiIconRenderMode::MonoTint)"
			    << ".SetIconSize(DPI(" << (int)CodeGenNodeProperty(n, "icon_size", 18) << "), DPI("
			    << (int)CodeGenNodeProperty(n, "icon_size", 18) << "));\n";
	}
	else if(n.type_id == "UiTitleCard")
	{
		out << "\t\t" << var << ".SetTitle(" << CppString(CodeGenNodeProperty(n, "text", n.name)) << ")"
		    << ".SetSubTitle(" << CppString(CodeGenNodeProperty(n, "subtitle", "")) << ");\n";
		String icon = IconExpr(CodeGenNodeProperty(n, "icon", "None"));
		if(!icon.IsEmpty())
			out << "\t\t" << var << ".SetMedia(" << icon << ", Size(DPI("
			    << (int)CodeGenNodeProperty(n, "icon_size", 24) << "), DPI("
			    << (int)CodeGenNodeProperty(n, "icon_size", 24) << ")));\n";
	}
	else if(n.type_id == "UiButton") {
		out << "\t\t" << var << ".SetText(" << CppString(CodeGenNodeProperty(n, "text", n.name)) << ");\n";
		String icon = IconExpr(CodeGenNodeProperty(n, "icon", "None"));
		if(!icon.IsEmpty())
			out << "\t\t" << var << ".SetIcon(" << icon << ").SetIconSize(DPI("
			    << (int)CodeGenNodeProperty(n, "icon_size", 16) << "), DPI("
			    << (int)CodeGenNodeProperty(n, "icon_size", 16) << "));\n";
	}
	else if(n.type_id == "UiLineEdit") {
		out << "\t\t" << var << ".SetTextUtf8(" << CppString(CodeGenNodeProperty(n, "text", n.name)) << ");\n";
		String placeholder = CodeGenNodeProperty(n, "placeholder", "");
		if(!placeholder.IsEmpty())
			out << "\t\t" << var << ".SetPlaceholder(" << CppString(placeholder) << ");\n";
	}
	else if(n.type_id == "UiIntEdit") {
		out << "\t\t" << var << ".MinMax(" << (int)CodeGenNodeProperty(n, "min", 0)
		    << ", " << (int)CodeGenNodeProperty(n, "max", 100) << ")"
		    << ".Step(" << (int)CodeGenNodeProperty(n, "step", 1) << ")"
		    << ".ShowSpin(" << ((bool)CodeGenNodeProperty(n, "spin", true) ? "true" : "false") << ");\n";
		out << "\t\t" << var << ".SetValue(" << (int)CodeGenNodeProperty(n, "value", 42) << ");\n";
	}
	else if(n.type_id == "UiFloatEdit") {
		out << "\t\t" << var << ".MinMax(" << (double)CodeGenNodeProperty(n, "minf", 0.0)
		    << ", " << (double)CodeGenNodeProperty(n, "maxf", 100.0) << ")"
		    << ".Step(" << (double)CodeGenNodeProperty(n, "stepf", 0.1) << ")"
		    << ".Precision(" << (int)CodeGenNodeProperty(n, "precision", 2) << ")"
		    << ".ShowSpin(" << ((bool)CodeGenNodeProperty(n, "spin", true) ? "true" : "false") << ");\n";
		out << "\t\t" << var << ".SetValue(" << (double)CodeGenNodeProperty(n, "valuef", 3.14) << ");\n";
	}
	else if(n.type_id == "UiSlider")
		out << "\t\t" << var << ".SetRange(0, 100).SetValue(50);\n";
	else if(n.type_id == "UiToggle")
		out << "\t\t" << var << ".SetOn(" << ((bool)CodeGenNodeProperty(n, "on", true) ? "true" : "false") << ");\n";
	else if(n.type_id == "UiDropdown") {
		out << "\t\t" << var << ".UseInternalModel().Add(\"First\", \"First\").Add(\"Second\", \"Second\").Add(\"Third\", \"Third\");\n";
		out << "\t\t" << var << ".SetData(" << CppString(CodeGenNodeProperty(n, "selected", "First")) << ");\n";
	}
	else if(n.type_id == "UiCheckBox") {
		out << "\t\t" << var << ".SetText(" << CppString(CodeGenNodeProperty(n, "text", n.name)) << ")"
		    << ".SetTriState(" << ((bool)CodeGenNodeProperty(n, "tri_state", false) ? "true" : "false") << ");\n";
		String state = CodeGenNodeProperty(n, "state", "Checked");
		out << "\t\t" << var << ".SetState(" << (state == "Indeterminate" ? "UICHECK_INDETERMINATE" :
		                                       state == "Unchecked" ? "UICHECK_UNCHECKED" : "UICHECK_CHECKED") << ");\n";
	}
	else if(n.type_id == "UiBreadcrumbs") {
		out << "\t\t" << var << ".AddCrumb(" << CppString(CodeGenNodeProperty(n, "crumb_a", "Home")) << ", \"a\")"
		    << ".AddCrumb(" << CppString(CodeGenNodeProperty(n, "crumb_b", "Library")) << ", \"b\")"
		    << ".AddCrumb(" << CppString(CodeGenNodeProperty(n, "crumb_c", "Current")) << ", \"c\")"
		    << ".SetCurrentIndex(" << (int)CodeGenNodeProperty(n, "current", 2) << ");\n";
		String icon = IconExpr(CodeGenNodeProperty(n, "icon", "None"));
		if(!icon.IsEmpty())
			out << "\t\t" << var << ".SetPathIcon(" << icon << ", UiAlign::LEFT, Size(DPI("
			    << (int)CodeGenNodeProperty(n, "icon_size", 16) << "), DPI("
			    << (int)CodeGenNodeProperty(n, "icon_size", 16) << ")));\n";
	}
	else if(n.type_id == "UiTab") {
		String visual = CodeGenNodeProperty(n, "visual", "Underline");
		out << "\t\t" << var << ".SetVisual(" << TabVisualExpr(visual) << ")"
		    << ".SetPlacement(" << AlignSideExpr(CodeGenNodeProperty(n, "placement", "Top"), "Top") << ")"
		    << ".SetExpandTabs(" << ((bool)CodeGenNodeProperty(n, "expand_tabs", false) ? "true" : "false") << ")"
		    << ".EnableCloseButtons(" << ((bool)CodeGenNodeProperty(n, "close_buttons", false) ? "true" : "false") << ")"
		    << ".EnableDragHandles(" << ((bool)CodeGenNodeProperty(n, "drag_handles", false) ? "true" : "false") << ");\n";
		out << "\t\t" << var << ".SetTabFont("
		    << FontExpr(CodeGenNodeProperty(n, "tab_font", "Sans"), (int)CodeGenNodeProperty(n, "tab_font_size", 11))
		    << ").SetTabIconSize(DPI(" << (int)CodeGenNodeProperty(n, "tab_icon_size", 16) << "))"
		    << ".SetTabIconSide(" << AlignSideExpr(CodeGenNodeProperty(n, "tab_icon_side", "Left")) << ");\n";
	}
	else if(n.type_id == "UiStack") {
		// Headless page container: pages and active index are emitted after children.
	}
	else if(n.type_id == "UiTable") {
		out << "\t\t" << var << ".UseInternalModel();\n"
		    << "\t\t" << var << ".GetInternalModel().SetSize(" << (int)CodeGenNodeProperty(n, "rows_count", 4)
		    << ", " << (int)CodeGenNodeProperty(n, "cols_count", 3) << ");\n";
	}
	else if(n.type_id == "UiTree") {
		out << "\t\t" << var << ".GetInternalModel().AddChild(" << var << ".GetInternalModel().Root(), UiModelItem(\"Workspace\", \"workspace\"));\n";
		out << "\t\t" << var << ".ShowConnectorLines(" << ((bool)CodeGenNodeProperty(n, "connectors", true) ? "true" : "false") << ");\n";
	}
	else if(n.type_id == "UiScrollPanel") {
		String mode = CodeGenNodeProperty(n, "scroll_mode", "Auto");
		String expr = mode == "Vertical" ? "UIPANELSCROLL_VERTICAL" :
		              mode == "Horizontal" ? "UIPANELSCROLL_HORIZONTAL" :
		              mode == "None" ? "UIPANELSCROLL_NONE" : "UIPANELSCROLL_AUTO";
		out << "\t\t" << var << ".SetScrollMode(" << expr << ");\n";
	}
	else {
		out << "\t\t" << var << ".SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle));\n";
	}
}

static void EmitAddSpacer(String& out, const String& parent_var, const DesignerNode& parent,
                          const DesignerNode& child)
{
	String kind = CodeGenNodeProperty(child, "spacer_kind", "Expander");
	int size = max(0, (int)CodeGenNodeProperty(child, "space", 24));
	int max_size = max(size, (int)CodeGenNodeProperty(child, "max_space", 1000000));
	int weight = max(1, (int)CodeGenNodeProperty(child, "weight", 1));
	if(parent.type_id == "BoxLayout") {
		if(kind == "Break")
			out << "\t\t" << parent_var << ".AddBreak(" << weight << ");\n";
		else if(kind == "Fixed")
			out << "\t\t" << parent_var << ".AddSpacer(1).Fixed(DPI(" << size << "));\n";
		else
			out << "\t\t" << parent_var << ".AddSpacer(" << weight << ");\n";
	}
	else if(parent.type_id == "GridLayout") {
		if(kind == "Break")
			out << "\t\t" << parent_var << ".AddBreak();\n";
		else if(kind == "Fixed")
			out << "\t\t" << parent_var << ".AddGap(DPI(" << size << "));\n";
		else if(kind == "Bounded")
			out << "\t\t" << parent_var << ".AddSpacer(DPI(" << size << "), DPI(" << max_size << "));\n";
		else
			out << "\t\t" << parent_var << ".AddExpand(" << weight << ");\n";
	}
}

static void EmitAddChild(String& out, const VectorMap<DesignerNodeId, String>& names,
                         const DesignerNode& parent, const DesignerNode& child, int index)
{
	String p = VarName(names, parent.id);
	String c = VarName(names, child.id);
	if(child.type_id == "Spacer" && parent.id == Designer_ROOT)
		return;
	if(parent.id == Designer_ROOT)
		out << "\t\tAdd(" << c << ".SizePos());\n";
	else if(child.type_id == "Spacer" && (parent.type_id == "BoxLayout" || parent.type_id == "GridLayout"))
		EmitAddSpacer(out, p, parent, child);
	else if(parent.type_id == "BoxLayout")
		out << "\t\t" << p << ".Add(" << c << ")" << BoxSizingCall(parent, child) << ";\n";
	else if(parent.type_id == "GridLayout") {
		int columns = max(1, (int)CodeGenNodeProperty(parent, "columns", 2));
		int row = max(0, (int)CodeGenNodeProperty(child, "grid_row", index / columns));
		int col = max(0, (int)CodeGenNodeProperty(child, "grid_col", index % columns));
		String hs = AxisSizing(child, "h_sizing");
		String vs = AxisSizing(child, "v_sizing");
		if(hs == "Fixed" || vs == "Fixed") {
			int w = max(10, (int)CodeGenNodeProperty(child, "width", 120));
			int h = max(10, (int)CodeGenNodeProperty(child, "height", 32));
			out << "\t\t" << p << ".Add(" << c << ", " << row << ", " << col
			    << ", " << (hs == "Expand" ? "true" : "false")
			    << ", " << (vs == "Expand" ? "true" : "false")
			    << ", Size(DPI(" << w << "), DPI(" << h << ")));\n";
		}
		else
			out << "\t\t" << p << ".Add(" << c << ", " << row << ", " << col
			    << ", " << (hs == "Expand" ? "true" : "false")
			    << ", " << (vs == "Expand" ? "true" : "false") << ");\n";
	}
	else if(parent.type_id == "UiSplitter") {
		out << "\t\t" << p << ".Add(" << c << ");\n";
		if(index == 0) {
			out << "\t\t" << p << ".SetMinPixels(0, DPI(" << (int)CodeGenNodeProperty(parent, "min_a", 80) << "));\n";
			out << "\t\t" << p << ".SetMinPixels(1, DPI(" << (int)CodeGenNodeProperty(parent, "min_b", 80) << "));\n";
		}
		out << "\t\t" << p << ".SetSplitPercent(" << (int)CodeGenNodeProperty(parent, "split_percent", 50) << ");\n";
	}
	else if(parent.type_id == "UiQuadSplitter") {
		out << "\t\t" << p << ".Add(" << c << ");\n";
		if(index == 0) {
			out << "\t\t" << p << ".SetMinPixels(0, DPI(" << (int)CodeGenNodeProperty(parent, "min_a", 60) << "));\n";
			out << "\t\t" << p << ".SetMinPixels(1, DPI(" << (int)CodeGenNodeProperty(parent, "min_b", 60) << "));\n";
			out << "\t\t" << p << ".SetMinPixels(2, DPI(" << (int)CodeGenNodeProperty(parent, "min_c", 60) << "));\n";
			out << "\t\t" << p << ".SetMinPixels(3, DPI(" << (int)CodeGenNodeProperty(parent, "min_d", 60) << "));\n";
		}
		out << "\t\t" << p << ".SetSplitPercent("
		    << (int)CodeGenNodeProperty(parent, "column_percent", 50) << ", "
		    << (int)CodeGenNodeProperty(parent, "row_percent", 50) << ");\n";
	}
	else if(parent.type_id == "UiTab") {
		String title = (bool)CodeGenNodeProperty(child, "show_title", true)
		             ? AsString(CodeGenNodeProperty(child, "page_title", child.name))
		             : String();
		String icon = IconExpr(CodeGenNodeProperty(child, "icon", "None"));
		out << "\t\t" << p << ".Add(" << c << ", " << CppString(title);
		if(!icon.IsEmpty())
			out << ", " << icon;
		out << ");\n";
	}
	else if(parent.type_id == "UiStack")
		out << "\t\t" << p << ".AddPage(" << c << ", " << CppString(CodeGenNodeProperty(child, "page_title", child.name)) << ");\n";
	else if(parent.type_id == "UiScrollPanel")
		out << "\t\t" << p << ".Content().Add(" << c << ".SizePos());\n";
	else if(parent.type_id == "UiPanel")
		out << "\t\t" << p << ".Add(" << c << ".SizePos());\n";
	else if(parent.type_id == "PaneSlot" || parent.type_id == "PageSlot")
		out << "\t\t" << p << ".Add(" << c << ".SizePos());\n";
}

static void EmitAdds(String& out, const VectorMap<DesignerNodeId, String>& names,
                     const DesignerModel& model, const DesignerNode& parent)
{
	for(int i = 0; i < parent.children.GetCount(); i++) {
		DesignerNodeId child_id = parent.children[i];
		const DesignerNode* child = model.Find(child_id);
		if(!child)
			continue;
		EmitAddChild(out, names, parent, *child, i);
		EmitAdds(out, names, model, *child);
	}
}

static void EmitPostAddSetup(String& out, const VectorMap<DesignerNodeId, String>& names, const DesignerModel& model)
{
	for(const DesignerNode& n : model.GetNodes()) {
		String var = VarName(names, n.id);
		if(n.type_id == "UiTab")
			out << "\t\t" << var << ".SetActiveTab(" << (int)CodeGenNodeProperty(n, "active", 0) << ");\n";
		else if(n.type_id == "UiStack")
			out << "\t\t" << var << ".SetActivePage(" << (int)CodeGenNodeProperty(n, "active", 0) << ");\n";
	}
}

String GenerateDesignerCode(const DesignerModel& model, const DesignerRegistry& registry,
                              const String& class_name, bool emit_designer_appearance)
{
	(void)registry;
	VectorMap<DesignerNodeId, String> names = BuildCodeNames(model);
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
		EmitSetup(out, names, n, emit_designer_appearance);
	}
	const DesignerNode* root = model.Find(Designer_ROOT);
	if(root)
		EmitAdds(out, names, model, *root);
	EmitPostAddSetup(out, names, model);
	out << "\t}\n\n";
	for(const DesignerNode& n : model.GetNodes()) {
		if(n.id == Designer_ROOT)
			continue;
		EmitDeclaration(out, names, n);
	}
	out << "};\n\n"
	    << "GUI_APP_MAIN\n"
	    << "{\n"
	    << "\t" << class_name << "().Run();\n"
	    << "}\n";
	return out;
}

}
