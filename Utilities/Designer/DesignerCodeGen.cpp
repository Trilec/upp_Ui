#include "DesignerCodeGen.h"
#include "DesignerDefaults.h"

// DesignerCodeGen.cpp - converts the model tree into standalone U++ code.
// Generated output should be theme-first: emit layout/control API calls and
// only include explicit appearance when the caller requests designer metadata.

namespace Upp {

static String ExportPlaceholder(const String& value, const char *placeholder)
{
	String text = TrimBoth(value);
	return text.IsEmpty() ? String("[") + placeholder + "]" : text;
}

static bool IsExactDesign(DesignerAppearanceMode mode)
{
	return mode == DesignerAppearanceMode::ExactDesign;
}

static Value CodeGenNodeProperty(const DesignerNode& n, const String& key, const Value& def)
{
	int q = n.properties.Find(key);
	return q >= 0 ? n.properties.GetValue(q) : def;
}

static bool CodeGenHasProperty(const DesignerNode& n, const String& key)
{
	return n.properties.Find(key) >= 0;
}

static String VarName(const VectorMap<DesignerNodeId, String>& names, DesignerNodeId id);
static String CppString(const String& s);
static String ColorExpr(Color c);
static String IconExpr(const String& icon);
static String AlignHExpr(const String& align, const String& def);
static String AlignVExpr(const String& align, const String& def);
static String AlignSideExpr(const String& side, const String& def);
static void ReportCodeGenContractError(String& out, const DesignerNode& n, const String& reason);

String DesignerCodeGenContext::Var(const DesignerNode& node) const
{
	return VarName(names_, node.id);
}

Value DesignerCodeGenContext::Property(const DesignerNode& node, const String& property, const Value& fallback) const
{
	return CodeGenNodeProperty(node, property, fallback);
}

bool DesignerCodeGenContext::HasProperty(const DesignerNode& node, const String& property) const
{
	return CodeGenHasProperty(node, property);
}

String DesignerCodeGenContext::CppString(const Value& value) const
{
	return ::Upp::CppString(AsString(value));
}

String DesignerCodeGenContext::ColorExpr(const Value& value) const
{
	return ::Upp::ColorExpr((Color)value);
}

String DesignerCodeGenContext::IconExpr(const Value& value) const
{
	return ::Upp::IconExpr(AsString(value));
}

String DesignerCodeGenContext::AlignHExpr(const Value& value, const String& fallback) const
{
	return ::Upp::AlignHExpr(AsString(value), fallback);
}

String DesignerCodeGenContext::AlignVExpr(const Value& value, const String& fallback) const
{
	return ::Upp::AlignVExpr(AsString(value), fallback);
}

String DesignerCodeGenContext::AlignSideExpr(const Value& value, const String& fallback) const
{
	return ::Upp::AlignSideExpr(AsString(value), fallback);
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

static String StyleHelperName(const VectorMap<DesignerNodeId, String>& names, const DesignerNode& n)
{
	return "Make" + CodeIdentifier(VarName(names, n.id)) + "Style";
}

static bool HasThemeOverride(const DesignerNode& n, DesignerAppearanceMode appearance_mode)
{
	return IsExactDesign(appearance_mode) && (bool)CodeGenNodeProperty(n, "theme_override", false);
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

static String AlignHExpr(const String& align, const String& def = "Center")
{
	String s = align.IsEmpty() ? def : align;
	if(s == "Left")
		return "UiAlign::LEFT";
	if(s == "Right")
		return "UiAlign::RIGHT";
	return "UiAlign::CENTER";
}

static String AlignVExpr(const String& align, const String& def = "Center")
{
	String s = align.IsEmpty() ? def : align;
	if(s == "Top")
		return "UiAlign::TOP";
	if(s == "Bottom")
		return "UiAlign::BOTTOM";
	return "UiAlign::CENTER";
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

static String LineStyleExpr(const String& style);

static String GroupHeaderModeExpr(const String& mode)
{
	if(mode == "Outside")
		return "UiGroupPanel::Outside";
	if(mode == "Center")
		return "UiGroupPanel::Center";
	return "UiGroupPanel::Inside";
}

static String RoleExpr(const String& role)
{
	if(role == "Subtle")
		return "UiRole::Subtle";
	if(role == "Accent")
		return "UiRole::Accent";
	if(role == "Alert")
		return "UiRole::Alert";
	return "UiRole::Standard";
}

static String CheckVisualExpr(const String& visual)
{
	if(visual == "Chip")
		return "UICHECKVIS_CHIP";
	if(visual == "List")
		return "UICHECKVIS_LIST";
	return "UICHECKVIS_CLASSIC";
}

static UiRole CodeGenRoleChoice(const DesignerNode& n)
{
	String role = AsString(CodeGenNodeProperty(n, "role", "Standard"));
	if(role == "Subtle")
		return UiRole::Subtle;
	if(role == "Accent")
		return UiRole::Accent;
	if(role == "Alert")
		return UiRole::Alert;
	return UiRole::Standard;
}

static String ColorExpr(Color c);

static int CodeGenBreadcrumbCount(const DesignerNode& n)
{
	return max(1, min(24, (int)CodeGenNodeProperty(n, "crumb_count", 3)));
}

static String CodeGenBreadcrumbCrumbKey(int i)
{
	return Format("crumb_%d", i + 1);
}

static String CodeGenBreadcrumbCrumbText(const DesignerNode& n, int i)
{
	String key = CodeGenBreadcrumbCrumbKey(i);
	if(CodeGenHasProperty(n, key))
		return CodeGenNodeProperty(n, key, Format("Crumb %d", i + 1));
	return Format("Crumb %d", i + 1);
}

static Color CodeGenQuadFaceColor(const DesignerNode& n, int i, Color fallback)
{
	Value quad = CodeGenNodeProperty(n, "face_quad", Value());
	if(quad.Is<ValueArray>()) {
		ValueArray a = quad;
		if(i >= 0 && i < a.GetCount() && !IsNull(a[i]))
			return a[i];
	}
	static const char *legacy[] = { "face_tl", "face_tr", "face_bl", "face_br" };
	if(i >= 0 && i < 4)
		return CodeGenNodeProperty(n, legacy[i], fallback);
	return fallback;
}

static String StyleTypeExpr(const DesignerNode& n)
{
	if(n.type_id == "UiPanel" || n.type_id == "Item" || n.type_id == "Generic")
		return "UiPanel::Style";
	if(n.type_id == "UiScrollPanel")
		return "UiScrollPanel::Style";
	if(n.type_id == "UiGroupPanel")
		return "UiGroupPanel::Style";
	if(n.type_id == "UiLabel")
		return "UiLabel::Style";
	if(n.type_id == "UiTitleCard")
		return "UiTitleCard::Style";
	if(n.type_id == "UiButton" || n.type_id == "UiSplitButton")
		return "UiButton::Style";
	if(n.type_id == "UiToolButton")
		return "UiToolButton::Style";
	if(n.type_id == "UiCheckBox")
		return "UiCheckBox::Style";
	if(n.type_id == "UiToggle")
		return "UiToggle::Style";
	if(n.type_id == "UiProgressBar")
		return "UiProgressBar::Style";
	if(n.type_id == "UiSlider")
		return "UiSlider::Style";
	if(n.type_id == "UiAccordion")
		return "UiAccordion::Style";
	if(n.type_id == "UiLineEdit" || n.type_id == "UiIntEdit" || n.type_id == "UiFloatEdit" ||
	   n.type_id == "UiMaskEdit" || n.type_id == "UiPasswordEdit")
		return "UiBaseEdit::Style";
	if(n.type_id == "UiDoc")
		return "UiDoc::Style";
	if(n.type_id == "UiDropdown")
		return "UiDropdown::Style";
	if(n.type_id == "UiBreadcrumbs")
		return "UiBreadcrumbs::Style";
	if(n.type_id == "UiTab")
		return "UiTab::Style";
	return String();
}

static String ResolveStyleExpr(const DesignerNode& n, const String& role_expr)
{
	if(n.type_id == "UiPanel" || n.type_id == "Item" || n.type_id == "Generic")
		return "UiTheme::ResolvePanel(" + role_expr + ")";
	if(n.type_id == "UiScrollPanel")
		return "UiTheme::ResolveScrollPanel(" + role_expr + ")";
	if(n.type_id == "UiGroupPanel")
		return "UiTheme::ResolveGroupPanel(" + role_expr + ")";
	if(n.type_id == "UiLabel")
		return "UiTheme::ResolveLabel(" + role_expr + ")";
	if(n.type_id == "UiTitleCard")
		return "UiTheme::ResolveTitleCard(" + role_expr + ")";
	if(n.type_id == "UiButton" || n.type_id == "UiSplitButton")
		return "UiTheme::ResolveButton(" + role_expr + ")";
	if(n.type_id == "UiToolButton")
		return "UiTheme::ResolveToolButton(" + role_expr + ")";
	if(n.type_id == "UiCheckBox")
		return "UiTheme::ResolveCheckBox(" + role_expr + ", " +
		       CheckVisualExpr(AsString(CodeGenNodeProperty(n, "visual", "Classic"))) + ")";
	if(n.type_id == "UiToggle")
		return "UiTheme::ResolveToggle(" + role_expr + ")";
	if(n.type_id == "UiProgressBar")
		return "UiTheme::ResolveProgressBar(" + role_expr + ")";
	if(n.type_id == "UiSlider")
		return "UiTheme::ResolveSlider(" + RoleExpr(AsString(CodeGenNodeProperty(n, "role", "Standard"))) + ")";
	if(n.type_id == "UiAccordion")
		return "UiAccordion::StyleDefault()";
	if(n.type_id == "UiLineEdit" || n.type_id == "UiIntEdit" || n.type_id == "UiFloatEdit" ||
	   n.type_id == "UiMaskEdit" || n.type_id == "UiPasswordEdit")
		return "UiTheme::ResolveEdit(" + role_expr + ")";
	if(n.type_id == "UiDoc")
		return "UiDoc::StyleDefault()";
	if(n.type_id == "UiDropdown")
		return "UiTheme::ResolveDropdown(" + role_expr + ")";
	if(n.type_id == "UiBreadcrumbs")
		return "UiBreadcrumbs::StyleDefault()";
	if(n.type_id == "UiTab")
		return "UiTheme::ResolveTab(" + role_expr + ", " + TabVisualExpr(AsString(CodeGenNodeProperty(n, "visual", "Document"))) + ")";
	return String();
}

static String ShadowCurveExpr(const String& curve)
{
	if(curve == "Linear")
		return "ShadowLinear()";
	if(curve == "Tight")
		return "ShadowTight()";
	if(curve == "Hard")
		return "ShadowHardCurve()";
	return "ShadowSoft()";
}

static void EmitButtonInkOverrideFields(String& out, const DesignerNode& n, bool tool_button)
{
	if(CodeGenHasProperty(n, "ink_enabled") && (bool)CodeGenNodeProperty(n, "ink_enabled", false)) {
		Color base_ink = tool_button
			? UiTheme::ResolveToolButton(CodeGenRoleChoice(n)).palette.ink[ST_NORMAL]
			: UiTheme::ResolveButton(CodeGenRoleChoice(n)).palette.ink[ST_NORMAL];
		Color ink = CodeGenNodeProperty(n, "ink", base_ink);
		out << "\t\t\ts.palette.ink[ST_NORMAL] = " << ColorExpr(ink) << ";\n"
		    << "\t\t\ts.palette.ink[ST_HOT] = " << ColorExpr(ink) << ";\n"
		    << "\t\t\ts.palette.ink[ST_PRESSED] = " << ColorExpr(ink) << ";\n"
		    << "\t\t\ts.palette.ink[ST_DISABLED] = DisabledColor(" << ColorExpr(ink) << ");\n";
	}

	if(CodeGenHasProperty(n, "icon_ink_enabled") && (bool)CodeGenNodeProperty(n, "icon_ink_enabled", false)) {
		StyledPalette pal = tool_button
			? UiTheme::ResolveToolButton(CodeGenRoleChoice(n)).palette
			: UiTheme::ResolveButton(CodeGenRoleChoice(n)).palette;
		Color base_icon = UiResolveIconColor(pal, ST_NORMAL);
		if(IsNull(base_icon))
			base_icon = pal.ink[ST_NORMAL];
		Color icon = CodeGenNodeProperty(n, "icon_ink", base_icon);
		out << "\t\t\ts.palette.icon[ST_NORMAL] = " << ColorExpr(icon) << ";\n"
		    << "\t\t\ts.palette.icon[ST_HOT] = " << ColorExpr(icon) << ";\n"
		    << "\t\t\ts.palette.icon[ST_PRESSED] = " << ColorExpr(icon) << ";\n"
		    << "\t\t\ts.palette.icon[ST_DISABLED] = DisabledColor(" << ColorExpr(icon) << ");\n";
	}
}

static void EmitPaletteColorOverrideFields(String& out, const String& target, const String& field, Color color)
{
	out << "\t\t\t" << target << "." << field << "[ST_NORMAL] = " << ColorExpr(color) << ";\n"
	    << "\t\t\t" << target << "." << field << "[ST_HOT] = " << ColorExpr(color) << ";\n"
	    << "\t\t\t" << target << "." << field << "[ST_PRESSED] = " << ColorExpr(color) << ";\n"
	    << "\t\t\t" << target << "." << field << "[ST_DISABLED] = DisabledColor(" << ColorExpr(color) << ");\n";
}

static void EmitFrameStyleOverrideFields(String& out, const String& target, const String& key, const DesignerNode& n)
{
	if(!CodeGenHasProperty(n, key))
		return;
	String style = AsString(CodeGenNodeProperty(n, key, "Solid"));
	if(style == "Dashed" || style == "DASHED")
		out << "\t\t\t" << target << ".dashed = true;\n"
		    << "\t\t\t" << target << ".dash_pattern = \"6,4\";\n";
	else if(style == "Dotted" || style == "DOTTED")
		out << "\t\t\t" << target << ".dashed = true;\n"
		    << "\t\t\t" << target << ".dash_pattern = \"1,3\";\n";
	else
		out << "\t\t\t" << target << ".dashed = false;\n"
		    << "\t\t\t" << target << ".dash_pattern.Clear();\n";
}


static void EmitSurfaceOverrideFields(String& out, const String& target, const DesignerNode& n, const String& prefix)
{
	String face_enabled_key = prefix.IsEmpty() ? String("face_enabled") : prefix + "_face_enabled";
	String face_key = prefix.IsEmpty() ? String("face") : prefix + "_face";
	String frame_enabled_key = prefix.IsEmpty() ? String("frame_enabled") : prefix + "_frame_enabled";
	String frame_key = prefix.IsEmpty() ? String("frame") : prefix + "_frame";
	String frame_width_key = prefix.IsEmpty() ? String("frame_width") : prefix + "_frame_width";
	String frame_style_key = prefix.IsEmpty() ? String("frame_style") : prefix + "_frame_style";
	String radius_key = prefix.IsEmpty() ? String("radius") : prefix + "_radius";
	String var_prefix = prefix.IsEmpty() ? String("surface") : prefix;
	if(CodeGenHasProperty(n, face_enabled_key)) {
		bool face_enabled = (bool)CodeGenNodeProperty(n, face_enabled_key, false);
		out << "\t\t\t" << target << ".metrics.face_enabled = " << (face_enabled ? "true" : "false") << ";\n";
		if(face_enabled) {
			Color face = CodeGenNodeProperty(n, face_key, SColorFace());
			out << "\t\t\tColor " << var_prefix << "_face = " << ColorExpr(face) << ";\n"
			    << "\t\t\t" << target << ".palette.face[ST_NORMAL] = UiFill::Solid(" << var_prefix << "_face);\n"
			    << "\t\t\t" << target << ".palette.face[ST_HOT] = UiFill::Solid(Blend(" << var_prefix << "_face, White(), 24));\n"
			    << "\t\t\t" << target << ".palette.face[ST_PRESSED] = UiFill::Solid(Blend(" << var_prefix << "_face, Black(), 16));\n"
			    << "\t\t\t" << target << ".palette.face[ST_DISABLED] = UiFill::Solid(Blend(" << var_prefix << "_face, SColorFace(), 90));\n";
		}
	}
	if(CodeGenHasProperty(n, frame_enabled_key)) {
		bool frame_enabled = (bool)CodeGenNodeProperty(n, frame_enabled_key, false);
		out << "\t\t\t" << target << ".metrics.frame_enabled = " << (frame_enabled ? "true" : "false") << ";\n";
		if(frame_enabled) {
			Color frame = CodeGenNodeProperty(n, frame_key, SColorShadow());
			int frame_width = max(0, (int)CodeGenNodeProperty(n, frame_width_key, 1));
			out << "\t\t\tfor(int i = 0; i < 4; i++)\n"
			    << "\t\t\t\t" << target << ".palette.frame[i] = " << ColorExpr(frame) << ";\n"
			    << "\t\t\t" << target << ".metrics.frame_width = DPI(" << frame_width << ");\n";
		}
	}
	EmitFrameStyleOverrideFields(out, target + ".metrics", frame_style_key, n);
	if(CodeGenHasProperty(n, radius_key))
		out << "\t\t\t" << target << ".metrics.radius = DPI(" << max(0, (int)CodeGenNodeProperty(n, radius_key, 0)) << ");\n";
}

static void EmitProgressSurfaceOverrideFields(String& out, const String& palette_target,
                                              const String& metrics_target, const String& prefix,
                                              const DesignerNode& n)
{
	String face_enabled_key = prefix + "_face_enabled";
	String face_key = prefix + "_face";
	String frame_enabled_key = prefix + "_frame_enabled";
	String frame_key = prefix + "_frame";
	String radius_key = prefix + "_radius";
	if(CodeGenHasProperty(n, face_enabled_key)) {
		bool face_enabled = (bool)CodeGenNodeProperty(n, face_enabled_key, false);
		out << "\t\t\t" << metrics_target << ".face_enabled = " << (face_enabled ? "true" : "false") << ";\n";
		if(face_enabled) {
			Color face = CodeGenNodeProperty(n, face_key, SColorFace());
			out << "\t\t\tColor " << prefix << "_face = " << ColorExpr(face) << ";\n"
			    << "\t\t\t" << palette_target << ".face[ST_NORMAL] = UiFill::Solid(" << prefix << "_face);\n"
			    << "\t\t\t" << palette_target << ".face[ST_HOT] = UiFill::Solid(Blend(" << prefix << "_face, White(), 24));\n"
			    << "\t\t\t" << palette_target << ".face[ST_PRESSED] = UiFill::Solid(Blend(" << prefix << "_face, Black(), 16));\n"
			    << "\t\t\t" << palette_target << ".face[ST_DISABLED] = UiFill::Solid(Blend(" << prefix << "_face, SColorFace(), 90));\n";
		}
	}
	if(CodeGenHasProperty(n, frame_enabled_key)) {
		bool frame_enabled = (bool)CodeGenNodeProperty(n, frame_enabled_key, false);
		out << "\t\t\t" << metrics_target << ".frame_enabled = " << (frame_enabled ? "true" : "false") << ";\n";
		if(frame_enabled) {
			Color frame = CodeGenNodeProperty(n, frame_key, SColorShadow());
			out << "\t\t\tfor(int i = 0; i < 4; i++)\n"
			    << "\t\t\t\t" << palette_target << ".frame[i] = " << ColorExpr(frame) << ";\n";
		}
	}
	if(CodeGenHasProperty(n, radius_key))
		out << "\t\t\t" << metrics_target << ".radius = DPI(" << max(0, (int)CodeGenNodeProperty(n, radius_key, 0)) << ");\n";
}

static void EmitAccordionThemeStyle(String& out, const String& var, const DesignerNode& n)
{
	if(!(bool)CodeGenNodeProperty(n, "theme_override", false))
		return;
	String role = CodeGenNodeProperty(n, "role", "Standard");
	String role_expr = RoleExpr(role);
	out << "\t\t{\n"
	    << "\t\t\tUiAccordion::Style s = UiAccordion::StyleDefault();\n"
	    << "\t\t\tUiPanel::Style panel = UiTheme::ResolvePanel(" << role_expr << ");\n"
	    << "\t\t\ts.palette = panel.palette;\n"
	    << "\t\t\ts.metrics = panel.metrics;\n"
	    << "\t\t\ts.metrics.radius = max(DPI(8), panel.metrics.radius);\n"
	    << "\t\t\ts.transparent = true;\n"
	    << "\t\t\ts.metrics.face_enabled = false;\n"
	    << "\t\t\ts.metrics.frame_enabled = false;\n"
	    << "\t\t\ts.metrics.frame_width = 0;\n"
	    << "\t\t\ts.metrics.shadow.enabled = false;\n"
	    << "\t\t\ts.body_style = UiTheme::ResolvePanel(" << role_expr << ");\n"
	    << "\t\t\ts.body_style.transparent = true;\n"
	    << "\t\t\ts.body_style.metrics.face_enabled = false;\n"
	    << "\t\t\ts.body_style.metrics.frame_enabled = false;\n"
	    << "\t\t\ts.body_style.metrics.frame_width = 0;\n"
	    << "\t\t\ts.body_style.metrics.radius = 0;\n"
	    << "\t\t\ts.body_style.metrics.focus_enabled = false;\n"
	    << "\t\t\ts.body_style.metrics.content_margin = Rect(0, 0, 0, 0);\n"
	    << "\t\t\ts.body_style.metrics.shadow.enabled = false;\n"
	    << "\t\t\ts.header_style = UiTheme::ResolveTitleCard(" << role_expr << ");\n"
	    << "\t\t\ts.header_style.metrics.content_margin = Rect(DPI(10), DPI(6), DPI(10), DPI(6));\n"
	    << "\t\t\ts.header_style.hover_enabled = false;\n"
	    << "\t\t\ts.header_style.metrics.focus_enabled = false;\n"
	    << "\t\t\ts.header_style.title_line = false;\n"
	    << "\t\t\ts.header_style.card_line = true;\n"
	    << "\t\t\ts.header_style.media_tint_mono = true;\n";
	EmitSurfaceOverrideFields(out, "s", n, "");
	EmitSurfaceOverrideFields(out, "s.header_style", n, "header");
	EmitSurfaceOverrideFields(out, "s.body_style", n, "body");
	if(CodeGenHasProperty(n, "header_title")) {
		out << "\t\t\tColor header_title = " << ColorExpr(CodeGenNodeProperty(n, "header_title", Color(0, 120, 212))) << ";\n"
		    << "\t\t\ts.header_style.title_color = header_title;\n"
		    << "\t\t\tfor(int i = 0; i < 4; i++) {\n"
		    << "\t\t\t\ts.header_style.palette.ink[i] = header_title;\n"
		    << "\t\t\t\ts.header_style.palette.icon[i] = header_title;\n"
		    << "\t\t\t}\n";
	}
	if(CodeGenHasProperty(n, "header_subtitle"))
		out << "\t\t\ts.header_style.subtitle_color = " << ColorExpr(CodeGenNodeProperty(n, "header_subtitle", Color(100, 116, 139))) << ";\n";
	out << "\t\t\t" << var << ".SetCustomStyle(s);\n"
	    << "\t\t}\n";
}

static void EmitThemeStyle(String& out, const String& var, const DesignerNode& n, DesignerAppearanceMode appearance_mode)
{
	if(n.type_id == "UiAccordion") {
		if(IsExactDesign(appearance_mode))
			EmitAccordionThemeStyle(out, var, n);
		return;
	}
	String role = CodeGenNodeProperty(n, "role", "Standard");
	String role_expr = RoleExpr(role);
	String style_type = StyleTypeExpr(n);
	String resolve_expr = ResolveStyleExpr(n, role_expr);
	if(style_type.IsEmpty() || resolve_expr.IsEmpty())
		return;
	bool override = IsExactDesign(appearance_mode) && (bool)CodeGenNodeProperty(n, "theme_override", false);
	bool force_style = false;
	if(n.type_id == "UiTitleCard") {
		force_style = CodeGenNodeProperty(n, "card_line_side", "Bottom") != "Bottom"
		           || CodeGenNodeProperty(n, "card_line_length", "Large") != "Large"
		           || CodeGenNodeProperty(n, "card_line_style", "Solid") != "Solid"
		           || (int)CodeGenNodeProperty(n, "card_line_thickness", 1) != 1
		           || (int)CodeGenNodeProperty(n, "card_line_gap", 0) != 0
		           || (bool)CodeGenNodeProperty(n, "card_line_color_enabled", false);
	}
	else if(n.type_id == "UiTab") {
		force_style = (int)CodeGenNodeProperty(n, "content_gap", 6) != 6
		           || (int)CodeGenNodeProperty(n, "affordance_gap", 4) != 4;
	}
	bool custom_align = false;
	if(n.type_id == "UiCheckBox" || n.type_id == "UiToggle")
		custom_align = CodeGenNodeProperty(n, "align_h", "Left") != "Left" || CodeGenNodeProperty(n, "align_v", "Center") != "Center";
	if(force_style)
		override = true;
	if(!override) {
		if(n.type_id == "UiAccordion")
			return;
		if(!custom_align && role != "Standard" && !force_style) {
			out << "\t\t" << var << ".SetCustomStyle(" << resolve_expr << ");\n";
			return;
		}
		else
			return;
	}

	if(IsExactDesign(appearance_mode) && (bool)CodeGenNodeProperty(n, "theme_override", false)) {
		out << "\t\t// Designer appearance override for " << (n.name.IsEmpty() ? n.type_id : n.name) << ".\n"
		    << "\t\t// Role base: " << AsString(CodeGenNodeProperty(n, "role", "Standard")) << ".\n"
		    << "\t\t// Remove this block to return to theme defaults.\n";
	}

	out << "\t\t{\n"
	    << "\t\t\t" << style_type << " s = " << resolve_expr << ";\n";
	if(CodeGenHasProperty(n, "face_enabled")) {
		if(IsExactDesign(appearance_mode) && (bool)CodeGenNodeProperty(n, "theme_override", false))
			out << "\t\t\t// Surface override.\n";
		bool face_enabled = (bool)CodeGenNodeProperty(n, "face_enabled", false);
		out << "\t\t\ts.metrics.face_enabled = " << (face_enabled ? "true" : "false") << ";\n";
		if(face_enabled) {
			Color face = CodeGenNodeProperty(n, "face", SColorFace());
			if(CodeGenNodeProperty(n, "face_mode", "Solid") == "Quad") {
				out << "\t\t\tUiFill face_fill = UiFill::ImageFill(MakeQuadGradientTile(48, "
				    << ColorExpr(CodeGenQuadFaceColor(n, 0, face)) << ", "
				    << ColorExpr(CodeGenQuadFaceColor(n, 1, face)) << ", "
				    << ColorExpr(CodeGenQuadFaceColor(n, 2, face)) << ", "
				    << ColorExpr(CodeGenQuadFaceColor(n, 3, face)) << ", 0));\n"
				    << "\t\t\tfor(int i = 0; i < 4; i++)\n"
				    << "\t\t\t\ts.palette.face[i] = face_fill;\n";
			}
			else {
				out << "\t\t\tColor face = " << ColorExpr(face) << ";\n"
				    << "\t\t\ts.palette.face[ST_NORMAL] = UiFill::Solid(face);\n"
				    << "\t\t\ts.palette.face[ST_HOT] = UiFill::Solid(Blend(face, White(), 24));\n"
				    << "\t\t\ts.palette.face[ST_PRESSED] = UiFill::Solid(Blend(face, Black(), 16));\n"
				    << "\t\t\ts.palette.face[ST_DISABLED] = UiFill::Solid(Blend(face, SColorFace(), 90));\n";
			}
		}
	}
	if(CodeGenHasProperty(n, "frame_enabled")) {
		bool frame_enabled = (bool)CodeGenNodeProperty(n, "frame_enabled", false);
		out << "\t\t\ts.metrics.frame_enabled = " << (frame_enabled ? "true" : "false") << ";\n";
		if(frame_enabled) {
			Color frame = CodeGenNodeProperty(n, "frame", SColorShadow());
			out << "\t\t\tfor(int i = 0; i < 4; i++)\n"
			    << "\t\t\t\ts.palette.frame[i] = " << ColorExpr(frame) << ";\n"
			    << "\t\t\ts.metrics.frame_width = DPI(" << max(0, (int)CodeGenNodeProperty(n, "frame_width", 1)) << ");\n";
		}
	}
	if(CodeGenHasProperty(n, "radius"))
		out << "\t\t\ts.metrics.radius = DPI(" << max(0, (int)CodeGenNodeProperty(n, "radius", 0)) << ");\n";
	EmitFrameStyleOverrideFields(out, "s.metrics", "frame_style", n);
	if(n.type_id == "UiDropdown") {
		bool theme_override = (bool)CodeGenNodeProperty(n, "theme_override", false);
		bool draws_surface = (bool)CodeGenNodeProperty(n, "face_enabled", false)
		                  || (bool)CodeGenNodeProperty(n, "frame_enabled", false)
		                  || (bool)CodeGenNodeProperty(n, "shadow_enabled", false);
		if(theme_override && draws_surface)
			out << "\t\t\ts.transparent = false;\n";
	}
	if(n.type_id == "UiTitleCard") {
		if(CodeGenHasProperty(n, "title_color_enabled") && (bool)CodeGenNodeProperty(n, "title_color_enabled", false)) {
			if(IsExactDesign(appearance_mode))
				out << "\t\t\t// Text/icon override.\n";
			out << "\t\t\ts.title_color = "
			    << ColorExpr(CodeGenNodeProperty(n, "title_color", UiTheme::ResolveTitleCard(CodeGenRoleChoice(n)).title_color))
			    << ";\n";
		}
		if(CodeGenHasProperty(n, "subtitle_color_enabled") && (bool)CodeGenNodeProperty(n, "subtitle_color_enabled", false)) {
			if(IsExactDesign(appearance_mode) && !(CodeGenHasProperty(n, "title_color_enabled") && (bool)CodeGenNodeProperty(n, "title_color_enabled", false)))
				out << "\t\t\t// Text/icon override.\n";
			out << "\t\t\ts.subtitle_color = "
			    << ColorExpr(CodeGenNodeProperty(n, "subtitle_color", UiTheme::ResolveTitleCard(CodeGenRoleChoice(n)).subtitle_color))
			    << ";\n";
		}
		if(CodeGenHasProperty(n, "card_line_side")) {
			if(IsExactDesign(appearance_mode) && (bool)CodeGenNodeProperty(n, "theme_override", false))
				out << "\t\t\t// Layout-specific override.\n";
			out << "\t\t\ts.card_line_side = " << AlignSideExpr(CodeGenNodeProperty(n, "card_line_side", "Bottom"), "Bottom") << ";\n";
		}
		if(CodeGenHasProperty(n, "card_line_length"))
			out << "\t\t\ts.card_line_length = " << (CodeGenNodeProperty(n, "card_line_length", "Large") == "Small" ? "SMALL" :
			                                              CodeGenNodeProperty(n, "card_line_length", "Large") == "Medium" ? "MEDIUM" : "LARGE") << ";\n";
		if(CodeGenHasProperty(n, "card_line_style"))
			out << "\t\t\ts.card_line_style = " << LineStyleExpr(AsString(CodeGenNodeProperty(n, "card_line_style", "Solid"))) << ";\n";
		if(CodeGenHasProperty(n, "card_line_thickness"))
			out << "\t\t\ts.card_line_thickness = DPI(" << max(1, (int)CodeGenNodeProperty(n, "card_line_thickness", 1)) << ");\n";
		if(CodeGenHasProperty(n, "card_line_gap"))
			out << "\t\t\ts.card_line_gap = DPI(" << max(0, (int)CodeGenNodeProperty(n, "card_line_gap", 0)) << ");\n";
		if(CodeGenHasProperty(n, "card_line_color_enabled")) {
			bool enabled = (bool)CodeGenNodeProperty(n, "card_line_color_enabled", false);
			out << "\t\t\ts.card_line_color_enabled = " << (enabled ? "true" : "false") << ";\n";
			if(enabled)
				out << "\t\t\ts.card_line_color = "
				    << ColorExpr(CodeGenNodeProperty(n, "card_line_color", UiTheme::ResolveTitleCard(CodeGenRoleChoice(n)).card_line_color))
				    << ";\n";
		}
	}
	if(CodeGenHasProperty(n, "shadow_enabled")) {
		if(IsExactDesign(appearance_mode) && (bool)CodeGenNodeProperty(n, "theme_override", false))
			out << "\t\t\t// Shadow override.\n";
		bool shadow = (bool)CodeGenNodeProperty(n, "shadow_enabled", false);
		out << "\t\t\ts.metrics.shadow.enabled = " << (shadow ? "true" : "false") << ";\n";
		if(shadow) {
			out << "\t\t\ts.metrics.shadow.distance = DPI(" << max(0, (int)CodeGenNodeProperty(n, "shadow_distance", 6)) << ");\n"
			    << "\t\t\ts.metrics.shadow.offset_x = DPI(" << (int)CodeGenNodeProperty(n, "shadow_offset_x", 0) << ");\n"
			    << "\t\t\ts.metrics.shadow.offset_y = DPI(" << (int)CodeGenNodeProperty(n, "shadow_offset_y", 0) << ");\n"
			    << "\t\t\ts.metrics.shadow.alpha = " << minmax((int)CodeGenNodeProperty(n, "shadow_alpha", 90), 0, 255) << ";\n"
			    << "\t\t\ts.metrics.shadow.color = " << ColorExpr(CodeGenNodeProperty(n, "shadow_color", Black())) << ";\n"
			    << "\t\t\ts.metrics.shadow.mode = SHADOW_CURVE;\n"
			    << "\t\t\ts.metrics.shadow.curve = " << ShadowCurveExpr(CodeGenNodeProperty(n, "shadow_curve", "Soft")) << ";\n";
		}
	}
	if(custom_align) {
		if(n.type_id == "UiCheckBox" || n.type_id == "UiToggle") {
			out << "\t\t\ts.align_h = " << AlignHExpr(CodeGenNodeProperty(n, "align_h", "Left"), "Left") << ";\n"
			    << "\t\t\ts.align_v = " << AlignVExpr(CodeGenNodeProperty(n, "align_v", "Center"), "Center") << ";\n";
		}
	}
	if(n.type_id == "UiCheckBox") {
		if(IsExactDesign(appearance_mode) && (bool)CodeGenNodeProperty(n, "theme_override", false))
			out << "\t\t\t// Indicator override.\n";
		EmitSurfaceOverrideFields(out, "s.indicator_palette", n, "indicator");
	}
	else if(n.type_id == "UiToggle") {
		if(IsExactDesign(appearance_mode) && (bool)CodeGenNodeProperty(n, "theme_override", false))
			out << "\t\t\t// Indicator override.\n";
		EmitSurfaceOverrideFields(out, "s.track_palette", n, "track");
		EmitSurfaceOverrideFields(out, "s.thumb_palette", n, "thumb");
	}
	if(n.type_id == "UiProgressBar") {
		if(IsExactDesign(appearance_mode) && (bool)CodeGenNodeProperty(n, "theme_override", false))
			out << "\t\t\t// Progress part override.\n";
		EmitProgressSurfaceOverrideFields(out, "s.track_palette", "s.track_metrics", "track", n);
		EmitProgressSurfaceOverrideFields(out, "s.fill_palette", "s.fill_metrics", "progress", n);
		if(CodeGenHasProperty(n, "filled_text_enabled") && (bool)CodeGenNodeProperty(n, "filled_text_enabled", false))
			out << "\t\t\ts.filled_text = " << ColorExpr(CodeGenNodeProperty(n, "filled_text", UiTheme::ResolveProgressBar(CodeGenRoleChoice(n)).filled_text)) << ";\n";
		if(CodeGenHasProperty(n, "empty_text_enabled") && (bool)CodeGenNodeProperty(n, "empty_text_enabled", false))
			out << "\t\t\ts.empty_text = " << ColorExpr(CodeGenNodeProperty(n, "empty_text", UiTheme::ResolveProgressBar(CodeGenRoleChoice(n)).empty_text)) << ";\n";
	}
	if(n.type_id == "UiSlider") {
		if(IsExactDesign(appearance_mode) && (bool)CodeGenNodeProperty(n, "theme_override", false))
			out << "\t\t\t// Layout-specific override.\n";
		out << "\t\t\ts.track_size = Size(DPI(" << max(20, (int)CodeGenNodeProperty(n, "track_width", 120))
		    << "), DPI(" << max(1, (int)CodeGenNodeProperty(n, "track_height", 3)) << "));\n"
		    << "\t\t\ts.thumb_size = Size(DPI(" << max(6, (int)CodeGenNodeProperty(n, "thumb_width", 20))
		    << "), DPI(" << max(6, (int)CodeGenNodeProperty(n, "thumb_height", 20)) << "));\n"
		    << "\t\t\ts.track_metrics.radius = DPI(" << max(0, (int)CodeGenNodeProperty(n, "track_radius", CodeGenNodeProperty(n, "radius", 8))) << ");\n"
		    << "\t\t\ts.thumb_metrics.radius = DPI(" << max(0, (int)CodeGenNodeProperty(n, "thumb_radius", 8)) << ");\n";
	}
	else if(n.type_id == "UiTab") {
		out << "\t\t\ts.content_gap = DPI(" << max(0, (int)CodeGenNodeProperty(n, "content_gap", 6)) << ");\n"
		    << "\t\t\ts.affordance_gap = DPI(" << max(0, (int)CodeGenNodeProperty(n, "affordance_gap", 4)) << ");\n";
	}
	if(n.type_id == "UiLabel") {
		if(IsExactDesign(appearance_mode) && (bool)CodeGenNodeProperty(n, "theme_override", false))
			out << "\t\t\t// Text/icon override.\n";
		if(CodeGenHasProperty(n, "ink_enabled") && (bool)CodeGenNodeProperty(n, "ink_enabled", false)) {
			Color base_ink = UiTheme::ResolveLabel(CodeGenRoleChoice(n)).palette.ink[ST_NORMAL];
			if(IsNull(base_ink))
				base_ink = SColorText();
			Color ink = CodeGenNodeProperty(n, "ink", base_ink);
			EmitPaletteColorOverrideFields(out, "s.palette", "ink", ink);
		}
		if(CodeGenHasProperty(n, "icon_ink_enabled") && (bool)CodeGenNodeProperty(n, "icon_ink_enabled", false)) {
			StyledPalette pal = UiTheme::ResolveLabel(CodeGenRoleChoice(n)).palette;
			Color base_icon = UiResolveIconColor(pal, ST_NORMAL);
			if(IsNull(base_icon))
				base_icon = pal.ink[ST_NORMAL];
			if(IsNull(base_icon))
				base_icon = SColorText();
			Color icon = CodeGenNodeProperty(n, "icon_ink", base_icon);
			EmitPaletteColorOverrideFields(out, "s.palette", "icon", icon);
		}
	}
	else if(n.type_id == "UiCheckBox") {
		if(IsExactDesign(appearance_mode) && (bool)CodeGenNodeProperty(n, "theme_override", false))
			out << "\t\t\t// Text/icon override.\n";
		String visual = AsString(CodeGenNodeProperty(n, "visual", "Classic"));
		UiCheckVisual vis = visual == "Chip" ? UICHECKVIS_CHIP :
		                    visual == "List" ? UICHECKVIS_LIST : UICHECKVIS_CLASSIC;
		UiCheckBox::Style base = UiTheme::ResolveCheckBox(CodeGenRoleChoice(n), vis);
		if(CodeGenHasProperty(n, "ink_enabled") && (bool)CodeGenNodeProperty(n, "ink_enabled", false)) {
			Color base_ink = IsNull(base.palette.ink[ST_NORMAL]) ? SColorText() : base.palette.ink[ST_NORMAL];
			Color ink = CodeGenNodeProperty(n, "ink", base_ink);
			EmitPaletteColorOverrideFields(out, "s.palette", "ink", ink);
		}
		if(CodeGenHasProperty(n, "indicator_ink_enabled") && (bool)CodeGenNodeProperty(n, "indicator_ink_enabled", false)) {
			Color base_indicator = UiResolveIconColor(base.indicator_palette, ST_NORMAL);
			if(IsNull(base_indicator))
				base_indicator = base.indicator_palette.ink[ST_NORMAL];
			if(IsNull(base_indicator))
				base_indicator = SColorText();
			Color indicator = CodeGenNodeProperty(n, "indicator_ink", base_indicator);
			EmitPaletteColorOverrideFields(out, "s.indicator_palette", "ink", indicator);
		}
	}
	else if(n.type_id == "UiDropdown") {
		if(IsExactDesign(appearance_mode) && (bool)CodeGenNodeProperty(n, "theme_override", false))
			out << "\t\t\t// Text/icon override.\n";
		if(CodeGenHasProperty(n, "ink_enabled") && (bool)CodeGenNodeProperty(n, "ink_enabled", false)) {
			Color base_ink = UiTheme::ResolveDropdown(CodeGenRoleChoice(n)).palette.ink[ST_NORMAL];
			if(IsNull(base_ink))
				base_ink = SColorText();
			Color ink = CodeGenNodeProperty(n, "ink", base_ink);
			EmitPaletteColorOverrideFields(out, "s.palette", "ink", ink);
		}
	}
	else if(n.type_id == "UiLineEdit" || n.type_id == "UiIntEdit" || n.type_id == "UiFloatEdit" ||
	        n.type_id == "UiMaskEdit" || n.type_id == "UiPasswordEdit") {
		if(IsExactDesign(appearance_mode) && (bool)CodeGenNodeProperty(n, "theme_override", false))
			out << "\t\t\t// Text/icon override.\n";
		UiBaseEdit::Style base = UiTheme::ResolveEdit(CodeGenRoleChoice(n));
		if(CodeGenHasProperty(n, "ink_enabled") && (bool)CodeGenNodeProperty(n, "ink_enabled", false)) {
			Color base_ink = IsNull(base.palette.ink[ST_NORMAL]) ? SColorText() : base.palette.ink[ST_NORMAL];
			Color ink = CodeGenNodeProperty(n, "ink", base_ink);
			EmitPaletteColorOverrideFields(out, "s.palette", "ink", ink);
		}
		if(CodeGenHasProperty(n, "placeholder_ink_enabled") && (bool)CodeGenNodeProperty(n, "placeholder_ink_enabled", false)) {
			Color placeholder = CodeGenNodeProperty(n, "placeholder_ink", base.placeholder_ink);
			out << "\t\t\ts.placeholder_ink = " << ColorExpr(placeholder) << ";\n";
		}
	}
	else if(n.type_id == "UiDoc") {
		if(IsExactDesign(appearance_mode) && (bool)CodeGenNodeProperty(n, "theme_override", false))
			out << "\t\t\t// Text/icon override.\n";
		UiDoc::Style base = UiDoc::StyleDefault();
		if(CodeGenHasProperty(n, "ink_enabled") && (bool)CodeGenNodeProperty(n, "ink_enabled", false)) {
			Color base_ink = IsNull(base.palette.ink[ST_NORMAL]) ? SColorText() : base.palette.ink[ST_NORMAL];
			Color ink = CodeGenNodeProperty(n, "ink", base_ink);
			EmitPaletteColorOverrideFields(out, "s.palette", "ink", ink);
		}
	}
	if(n.type_id == "UiButton" || n.type_id == "UiSplitButton")
		EmitButtonInkOverrideFields(out, n, false);
	else if(n.type_id == "UiToolButton")
		EmitButtonInkOverrideFields(out, n, true);
	out << "\t\t\t" << var << ".SetCustomStyle(s);\n"
	    << "\t\t}\n";
}

static String BuildThemeHelperBody(const String& var, const DesignerNode& n)
{
	String temp;
	EmitThemeStyle(temp, var, n, DesignerAppearanceMode::ExactDesign);
	temp.Replace("\t\t\t" + var + ".SetCustomStyle(s);\n", "\treturn s;\n");
	temp.Replace("\t\t{\n", "");
	temp.Replace("\t\t}\n", "");
	temp.Replace("\t\t\t", "\t");
	temp.Replace("\t\t", "\t");
	return temp;
}

static void EmitThemeHelper(String& out, const VectorMap<DesignerNodeId, String>& names, const DesignerNode& n)
{
	if(!HasThemeOverride(n, DesignerAppearanceMode::ExactDesign))
		return;
	String helper = StyleHelperName(names, n);
	String style_type = StyleTypeExpr(n);
	if(style_type.IsEmpty())
		return;
	out << style_type << " " << helper << "()\n"
	    << "{\n"
	    << "\t// Source node: " << (n.name.IsEmpty() ? String("<unnamed>") : n.name) << " / " << n.type_id << "\n"
	    << "\t// Move this method to GeneratedDesignerTheme.cpp if splitting the generated app.\n";
	String role = AsString(CodeGenNodeProperty(n, "role", "Standard"));
	out << "\t// Base role: " << role << ". Designer appearance overrides applied below.\n";
	out << BuildThemeHelperBody("__target__", n);
	out << "}\n\n";
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

static int SpacerLineThickness(const DesignerNode& n)
{
	return max(1, (int)CodeGenNodeProperty(n, "line_thickness", 1));
}

static String SpacerLineColorExpr(const DesignerNode& n)
{
	if(CodeGenHasProperty(n, "line_color_enabled") && (bool)CodeGenNodeProperty(n, "line_color_enabled", false))
		return ColorExpr((Color)CodeGenNodeProperty(n, "line_color", Color(148, 163, 184)));
	return "UiTheme::ResolvePanel(UiPanelRole::Subtle).palette.frame[ST_NORMAL]";
}

static String SpacerLineAlignExpr(const DesignerNode& n)
{
	String align = AsString(CodeGenNodeProperty(n, "line_align", "Center"));
	if(align == "Start")
		return "UiCrossAlign::Start";
	if(align == "End")
		return "UiCrossAlign::End";
	return "UiCrossAlign::Center";
}

static String SpacerLineOrientationExpr(const DesignerNode& n)
{
	String orientation = AsString(CodeGenNodeProperty(n, "line_orientation", "Auto"));
	if(orientation == "Vertical")
		return "UiSpacerLineOrientation::Vertical";
	if(orientation == "Horizontal")
		return "UiSpacerLineOrientation::Horizontal";
	return "UiSpacerLineOrientation::Auto";
}

static String SpacerLineDashExpr(const DesignerNode& n)
{
	return CodeGenNodeProperty(n, "line_dash", "Solid") == "Dashed" ? "DASHED" : "SOLID";
}

static String LineStyleExpr(const String& style)
{
	if(style == "Dashed" || style == "DASHED")
		return "DASHED";
	if(style == "Dotted" || style == "DOTTED")
		return "DOTTED";
	return "SOLID";
}

static const DesignerControlSpec* CodeGenSpec(const DesignerRegistry& registry, const DesignerNode& n)
{
	return registry.FindSpec(n.type_id);
}

static bool CodeGenIsHeadlessNode(const DesignerRegistry& registry, const DesignerNode& n)
{
	const DesignerControlSpec* spec = CodeGenSpec(registry, n);
	return spec && spec->IsHeadlessNode();
}

static void EmitDeclaration(DesignerCodeGenContext& ctx, const DesignerRegistry& registry,
                            const VectorMap<DesignerNodeId, String>& names, const DesignerNode& n)
{
	const DesignerControlSpec* spec = CodeGenSpec(registry, n);
	if(spec && spec->codegen.emit_declaration) {
		spec->codegen.emit_declaration(ctx, n);
		return;
	}
	String& out = ctx.Out();
	String var = VarName(names, n.id);
	if(!spec) {
		ReportCodeGenContractError(out, n, "missing control spec");
		return;
	}
	if(spec->codegen.route == DesignerCodeGenRoute::NoRuntimeOutput)
		return;
	if(spec->runtime_cpp_type.IsEmpty()) {
		ReportCodeGenContractError(out, n, "missing runtime type for declaration");
		return;
	}
	out << "\t" << spec->runtime_cpp_type << " " << var << ";\n";
	const DesignerNode* parent = ctx.Model().Find(n.parent);
	if(parent && parent->type_id == "UiGroupPanel")
		out << "\tUiDirectContentHost " << var << "_host;\n";
}

static String AxisSizing(const DesignerNode& n, const String& axis_key)
{
	return CodeGenNodeProperty(n, axis_key, "Fit");
}

static int SpacerAxisMin(const DesignerNode& n, bool width_axis)
{
	return max(0, (int)CodeGenNodeProperty(n, width_axis ? "min_width" : "min_height", 0));
}

static int SpacerAxisMax(const DesignerNode& n, bool width_axis)
{
	return max(0, (int)CodeGenNodeProperty(n, width_axis ? "max_width" : "max_height", 0));
}

static int CodeGenFixedMetric(const DesignerNode& n, const String& axis_key, int fallback)
{
	String explicit_key = axis_key == "width" ? "fixed_width" : "fixed_height";
	int q = n.properties.Find(explicit_key);
	if(q >= 0)
		return (int)n.properties.GetValue(q);
	q = n.properties.Find(axis_key);
	return q >= 0 ? (int)n.properties.GetValue(q) : fallback;
}

static int CodeGenMinMetric(const DesignerNode& n, const char* key)
{
	return max(0, (int)CodeGenNodeProperty(n, key, 0));
}
static String GridItemAlignHExpr(const DesignerNode& n)
{
	String align = CodeGenNodeProperty(n, "cell_align_h", "Auto");
	if(align == "Auto")
		align = CodeGenNodeProperty(n, "align_h", CodeGenNodeProperty(n, "align", "Left"));
	if(align == "Right")
		return "UiGridLayout::Align::End";
	if(align == "Center")
		return "UiGridLayout::Align::Center";
	return "UiGridLayout::Align::Start";
}

static String GridItemAlignVExpr(const DesignerNode& n)
{
	String align = CodeGenNodeProperty(n, "cell_align_v", "Auto");
	if(align == "Auto")
		align = CodeGenNodeProperty(n, "align_v", "Top");
	if(align == "Bottom")
		return "UiGridLayout::Align::End";
	if(align == "Center")
		return "UiGridLayout::Align::Center";
	return "UiGridLayout::Align::Start";
}

static String BoxCrossAlignExpr(const DesignerNode& n, bool horizontal_parent)
{
	String align = CodeGenNodeProperty(n, horizontal_parent ? "cell_align_v" : "cell_align_h", "Auto");
	if(align == "Center")
		return "UiBoxLayout::Align::Center";
	if(align == "Bottom" || align == "Right")
		return "UiBoxLayout::Align::End";
	return "UiBoxLayout::Align::Start";
}

static String BoxSizingCall(const DesignerNode& parent, const DesignerNode& child)
{
	bool horizontal = CodeGenNodeProperty(parent, "direction", "V") == "H";
	String sizing = AxisSizing(child, horizontal ? "h_sizing" : "v_sizing");
	if(sizing == "Fixed") {
		int v = horizontal ? DesignerClampMin((int)CodeGenFixedMetric(child, "width", DESIGNER_FIXED_FALLBACK_WIDTH))
		                   : DesignerClampMin((int)CodeGenFixedMetric(child, "height", DESIGNER_FIXED_FALLBACK_HEIGHT));
		return Format(".Fixed(DPI(%d))", v);
	}
	if(sizing == "Expand")
		return ".Expand(1)";
	return ".Fit()";
}

static String BoxMinCall(const DesignerNode& parent, const DesignerNode& child)
{
	int min_w = CodeGenMinMetric(child, "min_width");
	int min_h = CodeGenMinMetric(child, "min_height");
	bool horizontal = CodeGenNodeProperty(parent, "direction", "V") == "H";
	String hs = AxisSizing(child, "h_sizing");
	String vs = AxisSizing(child, "v_sizing");
	String out;
	if(horizontal)
		out << Format(".MinMain(DPI(%d))", min_w);
	else
		out << Format(".MinMain(DPI(%d))", min_h);

	String cross = horizontal ? vs : hs;
	int cross_min = horizontal ? min_h : min_w;
	String cross_align = BoxCrossAlignExpr(child, horizontal);
	int fixed = horizontal ? DesignerClampMin((int)CodeGenFixedMetric(child, "height", DESIGNER_FIXED_FALLBACK_HEIGHT))
	                       : DesignerClampMin((int)CodeGenFixedMetric(child, "width", DESIGNER_FIXED_FALLBACK_WIDTH));
	fixed = max(fixed, cross_min);

	if(cross == "Fixed")
		out << Format(".MinMaxCross(DPI(%d), DPI(%d)).AlignSelf(", fixed, fixed) << cross_align << ")";
	else if(cross == "Fit")
		out << Format(".MinCross(DPI(%d)).AlignSelf(", cross_min) << cross_align << ")";
	else
		out << Format(".MinCross(DPI(%d)).AlignSelf(UiBoxLayout::Align::Stretch)", cross_min);
	return out;
}

static void EmitDirectChildLayout(String& out, const String& var, const DesignerNode& child)
{
	String hs = AxisSizing(child, "h_sizing");
	String vs = AxisSizing(child, "v_sizing");
	int min_w = CodeGenMinMetric(child, "min_width");
	int min_h = CodeGenMinMetric(child, "min_height");
	int max_w = max(0, (int)CodeGenNodeProperty(child, "max_width", 0));
	int max_h = max(0, (int)CodeGenNodeProperty(child, "max_height", 0));
	String width_expr;

	if(hs == "Expand")
	{
		out << "\t\t" << var << ".HSizePosZ(0, 0);\n";
		width_expr = "GetSize().cx";
	}
	else if(hs == "Fixed") {
		int w = max(DesignerClampMin((int)CodeGenFixedMetric(child, "width", DESIGNER_FIXED_FALLBACK_WIDTH)), min_w);
		if(max_w > 0)
			w = min(w, max_w);
		out << "\t\t" << var << ".LeftPosZ(0, DPI(" << w << "));\n";
		width_expr = "DPI(" + AsString(w) + ")";
	}
	else {
		String w_expr = "max(" + var + ".GetMinSize().cx, DPI(" + AsString(min_w) + "))";
		if(max_w > 0)
			w_expr = "min(" + w_expr + ", DPI(" + AsString(max_w) + "))";
		out << "\t\t" << var << ".LeftPosZ(0, " << w_expr << ");\n";
		width_expr = w_expr;
	}

	if(vs == "Expand")
		out << "\t\t" << var << ".VSizePosZ(0, 0);\n";
	else if(vs == "Fixed") {
		int h = max(DesignerClampMin((int)CodeGenFixedMetric(child, "height", DESIGNER_FIXED_FALLBACK_HEIGHT)), min_h);
		if(max_h > 0)
			h = min(h, max_h);
		out << "\t\t" << var << ".TopPosZ(0, DPI(" << h << "));\n";
	}
	else {
		bool wrapped_horizontal_box = child.type_id == "BoxLayout" &&
		                              CodeGenNodeProperty(child, "direction", "V") == "H" &&
		                              CodeGenNodeProperty(child, "wrap", "None") != "None";
		String h_expr = "max(" + var + ".GetMinSize().cy, DPI(" + AsString(min_h) + "))";
		if(max_h > 0)
			h_expr = "min(" + h_expr + ", DPI(" + AsString(max_h) + "))";
		if(wrapped_horizontal_box)
			out << "\t\t" << var << ".TopPosZ(0, min(max(" << var << ".MeasureHeightForWidth(" << width_expr << "), DPI(" << min_h << ")), "
			    << (max_h > 0 ? "DPI(" + AsString(max_h) + ")" : h_expr) << ");\n";
		else
			out << "\t\t" << var << ".TopPosZ(0, " << h_expr << ");\n";
	}
}

static bool HasDesignerMinSizeOverride(const DesignerNode& n)
{
	return CodeGenMinMetric(n, "min_width") > 0 ||
	       CodeGenMinMetric(n, "min_height") > 0;
}

static void EmitDesignerMinSize(String& out, const DesignerRegistry& registry, const String& var, const DesignerNode& n)
{
	if(n.type_id == "Spacer" || n.type_id == "UiPanel" || CodeGenIsHeadlessNode(registry, n))
		return;
	if(!HasDesignerMinSizeOverride(n))
		return;
	out << "\t\t" << var << ".SetMinSize(Size(DPI("
	    << CodeGenMinMetric(n, "min_width")
	    << "), DPI("
	    << CodeGenMinMetric(n, "min_height")
	    << ")));\n";
}

static void ReportCodeGenContractError(String& out, const DesignerNode& n, const String& reason)
{
	String message = Format("DESIGNER CODEGEN CONTRACT ERROR: %s %s", n.type_id, reason);
	RLOG(message);
	out << "\t\t// " << message << "\n";
}

static void EmitSetup(DesignerCodeGenContext& ctx, const DesignerRegistry& registry, const VectorMap<DesignerNodeId, String>& names,
                      const DesignerNode& n, DesignerAppearanceMode appearance_mode)
{
	const DesignerControlSpec* spec = CodeGenSpec(registry, n);
	String& out = ctx.Out();
	String var = VarName(names, n.id);
	if(!spec) {
		ReportCodeGenContractError(out, n, "missing control spec");
		return;
	}
	if(spec->codegen.route == DesignerCodeGenRoute::NoRuntimeOutput || spec->codegen.route == DesignerCodeGenRoute::Headless)
		return;
	if(!HasThemeOverride(n, appearance_mode))
		EmitThemeStyle(out, var, n, appearance_mode);
	EmitDesignerMinSize(out, registry, var, n);
	if(spec->codegen.emit_setup) {
		spec->codegen.emit_setup(ctx, n);
	}
	else if(spec->codegen.route != DesignerCodeGenRoute::NoRuntimeOutput &&
	        spec->codegen.route != DesignerCodeGenRoute::Headless)
		ReportCodeGenContractError(out, n, "missing setup hook");
	String tooltip = CodeGenNodeProperty(n, "tooltip", String());
	if(!tooltip.IsEmpty() && n.type_id != "Spacer")
		out << "\t\t" << var << ".Tip(" << CppString(tooltip) << ");\n";
}

static void EmitAddChild(DesignerCodeGenContext& ctx, const DesignerNode& parent, const DesignerNode& child, int index)
{
	String& out = ctx.Out();
	const DesignerControlSpec* parent_spec = CodeGenSpec(ctx.Registry(), parent);
	if(parent.id == Designer_ROOT)
		out << "\t\tAdd(" << ctx.Var(child) << ");\n";
	else if(parent_spec && parent_spec->codegen.emit_child)
		parent_spec->codegen.emit_child(ctx, parent, child, index);
	else
		ReportCodeGenContractError(out, parent, "missing child emitter");
}

static void EmitAdds(DesignerCodeGenContext& ctx, const DesignerModel& model, const DesignerNode& parent)
{
	for(int i = 0; i < parent.children.GetCount(); i++) {
		DesignerNodeId child_id = parent.children[i];
		const DesignerNode* child = model.Find(child_id);
		if(!child)
			continue;
		EmitAddChild(ctx, parent, *child, i);
		EmitAdds(ctx, model, *child);
	}
}

static void EmitPostAddSetup(DesignerCodeGenContext& ctx, const DesignerModel& model)
{
	for(const DesignerNode& n : model.GetNodes()) {
		const DesignerControlSpec* spec = CodeGenSpec(ctx.Registry(), n);
		if(spec && spec->codegen.emit_post_build)
			spec->codegen.emit_post_build(ctx, n);
	}
}

static void EmitAppearanceApply(String& out, const VectorMap<DesignerNodeId, String>& names,
                                const DesignerModel& model, DesignerAppearanceMode appearance_mode)
{
	for(const DesignerNode& n : model.GetNodes()) {
		if(n.id == Designer_ROOT || !HasThemeOverride(n, appearance_mode))
			continue;
		out << "\t\t" << VarName(names, n.id) << ".SetCustomStyle(" << StyleHelperName(names, n) << "());\n";
	}
}

String GenerateDesignerCode(const DesignerModel& model, const DesignerRegistry& registry,
                            const DesignerCodeGenOptions& options)
{
	VectorMap<DesignerNodeId, String> names = BuildCodeNames(model);
	String out;
	DesignerCodeGenContext ctx(out, registry, model, names, options.appearance_mode);
	if(options.emit_export_header) {
		out << "// Generated by U++ Ui Designer.\n"
		    << "// Designer version: " << ExportPlaceholder(options.designer_version, "DESIGNER_VERSION") << "\n"
		    << "// Appearance mode: " << (options.appearance_mode == DesignerAppearanceMode::ExactDesign ? "ExactDesign" : "ThemeFirst") << "\n"
		    << "// Source design: " << ExportPlaceholder(options.source_design_filename, "SOURCE_DESIGN_JSON") << "\n"
		    << "// Package: " << ExportPlaceholder(options.package_name, "PACKAGE_NAME") << "\n"
		    << "// UMK path: " << ExportPlaceholder(options.umk_path, "PATH_TO_UPP_OR_UMK") << "\n"
		    << "// Exported package: " << ExportPlaceholder(options.exported_package_path, "PATH_TO_EXPORTED_PACKAGE") << "\n"
		    << "// Build method: " << ExportPlaceholder(options.build_method, "BUILD_METHOD") << "\n"
		    << "// Output executable: " << ExportPlaceholder(options.output_exe_path, "PATH_TO_OUTPUT_EXE") << "\n"
		    << "// Regenerate from design.json when the Designer model changes.\n"
		    << "// This file is " << (options.appearance_mode == DesignerAppearanceMode::ExactDesign ? "exact-design" : "theme-first") << " generated output.\n"
		    << "// Explicit Designer appearance overrides are marked in code.\n\n";
	}
	out << "#include <CtrlLib/CtrlLib.h>\n"
	    << "#include <Ui/Ui.h>\n\n"
	    << "using namespace Upp;\n\n"
	    << "// -----------------------------------------------------------------------------\n"
	    << "// Theme candidates\n"
	    << "// -----------------------------------------------------------------------------\n";
	if(IsExactDesign(options.appearance_mode)) {
		out << "// These style helpers were generated from Designer appearance overrides.\n"
		    << "// They can be copied into a shared UiTheme preset later.\n"
		    << "// Instance-specific text, layout, data, and event wiring remain outside this section.\n\n";
		for(const DesignerNode& n : model.GetNodes())
			if(n.id != Designer_ROOT)
				EmitThemeHelper(out, names, n);
	}
	else {
		out << "// Designer appearance export omitted in theme-first mode.\n"
		    << "// Instance-specific text, layout, data, and event wiring remain outside this section.\n\n";
	}

	out << "class " << options.class_name << " : public TopWindow {\n"
	    << "public:\n"
	    << "\ttypedef " << options.class_name << " CLASSNAME;\n\n"
	    << "\t" << options.class_name << "()\n"
	    << "\t{\n"
	    << "\t\tInitWindow();\n"
	    << "\t\tInitThemeContext();\n"
	    << "\t\tBuildControls();\n"
	    << "\t\tApplyAppearanceOverrides();\n"
	    << "\t\tBuildLayout();\n"
	    << "\t\tPostBuild();\n"
	    << "\t}\n\n"
	    << "private:\n"
	    << "\tvoid InitWindow()\n"
	    << "\t{\n"
	    << "\t\tTitle(\"Generated Designer Layout\");\n"
	    << "\t\tSizeable().Zoomable();\n";
	Size sz = model.GetVirtualSize();
	out << "\t\tSetRect(0, 0, DPI(" << sz.cx << "), DPI(" << sz.cy << "));\n"
	    << "\t}\n\n"
	    << "\tvoid InitThemeContext()\n"
	    << "\t{\n"
	    << "\t\t// Move this method to GeneratedDesignerTheme.cpp if splitting the generated app.\n"
	    << "\t\t// Theme context / preset setup.\n"
	    << "\t}\n\n"
	    << "\tvoid BuildControls()\n"
	    << "\t{\n"
	    << "\t\t// Control text, icons, values, ranges, roles, and basic behaviour.\n";
	for(const DesignerNode& n : model.GetNodes()) {
		if(n.id == Designer_ROOT)
			continue;
		EmitSetup(ctx, registry, names, n, options.appearance_mode);
	}
	out << "\t}\n\n"
	    << "\tvoid ApplyAppearanceOverrides()\n"
	    << "\t{\n";
	if(IsExactDesign(options.appearance_mode))
		EmitAppearanceApply(out, names, model, options.appearance_mode);
	else
		out << "\t\t// Designer appearance export omitted; theme-first output only.\n";
	out << "\t}\n\n"
	    << "\tvoid BuildLayout()\n"
	    << "\t{\n"
	    << "\t\t// Parent-child layout tree only.\n";
	const DesignerNode* root = model.Find(Designer_ROOT);
	if(root)
		EmitAdds(ctx, model, *root);
	out << "\t}\n\n"
	    << "\tvoid PostBuild()\n"
	    << "\t{\n"
	    << "\t\t// Active tabs/pages and late setup.\n";
	EmitPostAddSetup(ctx, model);
	out << "\t}\n\n";
	for(const DesignerNode& n : model.GetNodes()) {
		if(n.id == Designer_ROOT)
			continue;
		EmitDeclaration(ctx, registry, names, n);
	}
	out << "};\n\n"
	    << "GUI_APP_MAIN\n"
	    << "{\n"
	    << "\t" << options.class_name << "().Run();\n"
	    << "}\n";
	return out;
}

String GenerateDesignerCode(const DesignerModel& model, const DesignerRegistry& registry,
                            const String& class_name, DesignerAppearanceMode appearance_mode)
{
	DesignerCodeGenOptions options;
	options.class_name = class_name;
	options.appearance_mode = appearance_mode;
	return GenerateDesignerCode(model, registry, options);
}

String GenerateDesignerCode(const DesignerModel& model, const DesignerRegistry& registry,
                            const String& class_name, bool emit_designer_appearance)
{
	return GenerateDesignerCode(model, registry, class_name,
	                            emit_designer_appearance ? DesignerAppearanceMode::ExactDesign
	                                                      : DesignerAppearanceMode::ThemeFirst);
}

}
