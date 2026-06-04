#include "DesignerCodeGen.h"
#include "DesignerDefaults.h"

// DesignerCodeGen.cpp - converts the model tree into standalone U++ code.
// Generated output should be theme-first: emit layout/control API calls and
// only include explicit appearance when the caller requests designer metadata.

namespace Upp {

static Value CodeGenNodeProperty(const DesignerNode& n, const String& key, const Value& def)
{
	int q = n.properties.Find(key);
	return q >= 0 ? n.properties.GetValue(q) : def;
}

static bool CodeGenHasProperty(const DesignerNode& n, const String& key)
{
	return n.properties.Find(key) >= 0;
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
	if(i == 0)
		return CodeGenNodeProperty(n, "crumb_a", "Home");
	if(i == 1)
		return CodeGenNodeProperty(n, "crumb_b", "Library");
	if(i == 2)
		return CodeGenNodeProperty(n, "crumb_c", "Current");
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
	if(n.type_id == "UiAccordion")
		return "UiAccordion::Style";
	if(n.type_id == "UiLineEdit" || n.type_id == "UiIntEdit" || n.type_id == "UiFloatEdit")
		return "UiBaseEdit::Style";
	if(n.type_id == "UiDropdown")
		return "UiDropdown::Style";
	if(n.type_id == "UiBreadcrumbs")
		return "UiBreadcrumbs::Style";
	return String();
}

static String ResolveStyleExpr(const DesignerNode& n, const String& role_expr)
{
	if(n.type_id == "UiPanel" || n.type_id == "Item" || n.type_id == "Generic")
		return "UiTheme::ResolvePanel(" + role_expr + ")";
	if(n.type_id == "UiScrollPanel")
		return "UiScrollPanel::StyleDefault()";
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
	if(n.type_id == "UiAccordion")
		return "UiAccordion::StyleDefault()";
	if(n.type_id == "UiLineEdit" || n.type_id == "UiIntEdit" || n.type_id == "UiFloatEdit")
		return "UiTheme::ResolveEdit(" + role_expr + ")";
	if(n.type_id == "UiDropdown")
		return "UiTheme::ResolveDropdown(" + role_expr + ")";
	if(n.type_id == "UiBreadcrumbs")
		return "UiBreadcrumbs::StyleDefault()";
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


static void EmitSurfaceOverrideFields(String& out, const String& target, const DesignerNode& n, const String& prefix)
{
	String face_enabled_key = prefix.IsEmpty() ? String("face_enabled") : prefix + "_face_enabled";
	String face_key = prefix.IsEmpty() ? String("face") : prefix + "_face";
	String frame_enabled_key = prefix.IsEmpty() ? String("frame_enabled") : prefix + "_frame_enabled";
	String frame_key = prefix.IsEmpty() ? String("frame") : prefix + "_frame";
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
			out << "\t\t\tfor(int i = 0; i < 4; i++)\n"
			    << "\t\t\t\t" << target << ".palette.frame[i] = " << ColorExpr(frame) << ";\n"
			    << "\t\t\t" << target << ".metrics.frame_width = max(DPI(1), " << target << ".metrics.frame_width);\n";
		}
	}
	if(CodeGenHasProperty(n, radius_key))
		out << "\t\t\t" << target << ".metrics.radius = DPI(" << max(0, (int)CodeGenNodeProperty(n, radius_key, 0)) << ");\n";
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

static void EmitThemeStyle(String& out, const String& var, const DesignerNode& n, bool emit_designer_appearance)
{
	if(n.type_id == "UiAccordion") {
		if(emit_designer_appearance)
			EmitAccordionThemeStyle(out, var, n);
		return;
	}
	String role = CodeGenNodeProperty(n, "role", "Standard");
	String role_expr = RoleExpr(role);
	String style_type = StyleTypeExpr(n);
	String resolve_expr = ResolveStyleExpr(n, role_expr);
	if(style_type.IsEmpty() || resolve_expr.IsEmpty())
		return;
	bool override = emit_designer_appearance && (bool)CodeGenNodeProperty(n, "theme_override", false);
	bool custom_align = false;
	if(n.type_id == "UiCheckBox" || n.type_id == "UiToggle")
		custom_align = CodeGenNodeProperty(n, "align_h", "Left") != "Left" || CodeGenNodeProperty(n, "align_v", "Center") != "Center";
	if(!override) {
		if(n.type_id == "UiAccordion")
			return;
		if(!custom_align && role != "Standard" && n.type_id != "UiScrollPanel") {
			out << "\t\t" << var << ".SetCustomStyle(" << resolve_expr << ");\n";
			return;
		}
		else if(custom_align)
			override = true;
		else
			return;
	}

	out << "\t\t{\n"
	    << "\t\t\t" << style_type << " s = " << resolve_expr << ";\n";
	if(CodeGenHasProperty(n, "face_enabled")) {
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
			    << "\t\t\ts.metrics.frame_width = max(DPI(1), s.metrics.frame_width);\n";
		}
	}
	if(CodeGenHasProperty(n, "radius"))
		out << "\t\t\ts.metrics.radius = DPI(" << max(0, (int)CodeGenNodeProperty(n, "radius", 0)) << ");\n";
	if(CodeGenHasProperty(n, "shadow_enabled")) {
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
		EmitSurfaceOverrideFields(out, "s.indicator_palette", n, "indicator");
	}
	else if(n.type_id == "UiToggle") {
		EmitSurfaceOverrideFields(out, "s.track_palette", n, "track");
		EmitSurfaceOverrideFields(out, "s.thumb_palette", n, "thumb");
	}
	if(n.type_id == "UiLabel") {
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
		if(CodeGenHasProperty(n, "ink_enabled") && (bool)CodeGenNodeProperty(n, "ink_enabled", false)) {
			Color base_ink = UiTheme::ResolveDropdown(CodeGenRoleChoice(n)).palette.ink[ST_NORMAL];
			if(IsNull(base_ink))
				base_ink = SColorText();
			Color ink = CodeGenNodeProperty(n, "ink", base_ink);
			EmitPaletteColorOverrideFields(out, "s.palette", "ink", ink);
		}
	}
	else if(n.type_id == "UiLineEdit" || n.type_id == "UiIntEdit" || n.type_id == "UiFloatEdit") {
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
	if(n.type_id == "UiButton" || n.type_id == "UiSplitButton")
		EmitButtonInkOverrideFields(out, n, false);
	else if(n.type_id == "UiToolButton")
		EmitButtonInkOverrideFields(out, n, true);
	out << "\t\t\t" << var << ".SetCustomStyle(s);\n"
	    << "\t\t}\n";
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
	String style = AsString(CodeGenNodeProperty(n, "line_style", "Subtle"));
	if(style == "Custom")
		return max(1, (int)CodeGenNodeProperty(n, "line_thickness", 1));
	if(style == "Alert")
		return 4;
	if(style == "Accent" || style == "Standard")
		return 2;
	return 1;
}

static String SpacerLineStyleExpr(const DesignerNode& n)
{
	String style = AsString(CodeGenNodeProperty(n, "line_style", "Subtle"));
	if(style == "Standard")
		return "SPACER_LINE_STANDARD";
	if(style == "Accent")
		return "SPACER_LINE_ACCENT";
	if(style == "Alert")
		return "SPACER_LINE_ALERT";
	if(style == "Custom")
		return "SPACER_LINE_CUSTOM";
	return "SPACER_LINE_SUBTLE";
}

static String SpacerLineColorExpr(const DesignerNode& n)
{
	if(CodeGenHasProperty(n, "line_color_enabled") && (bool)CodeGenNodeProperty(n, "line_color_enabled", false))
		return ColorExpr((Color)CodeGenNodeProperty(n, "line_color", Color(148, 163, 184)));

	String style = AsString(CodeGenNodeProperty(n, "line_style", "Subtle"));
	if(style == "Custom") {
		return "SColorShadow()";
	}
	if(style == "Accent")
		return "UiTheme::ResolveButton(UiRole::Accent).palette.frame[ST_NORMAL]";
	if(style == "Alert")
		return "UiTheme::ResolveButton(UiRole::Alert).palette.frame[ST_NORMAL]";
	if(style == "Standard")
		return "UiTheme::ResolvePanel(UiPanelRole::Surface).palette.frame[ST_NORMAL]";
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

static String SpacerLineDashExpr(const DesignerNode& n)
{
	return CodeGenNodeProperty(n, "line_dash", "Solid") == "Dashed" ? "DASHED" : "SOLID";
}

static void EmitDeclaration(String& out, const VectorMap<DesignerNodeId, String>& names, const DesignerNode& n)
{
	String var = VarName(names, n.id);
	if(n.type_id == "Spacer")
		return;
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
	else if(n.type_id == "UiGroupPanel")
		out << "\tUiGroupPanel " << var << ";\n";
	else if(n.type_id == "UiButton")
		out << "\tUiButton " << var << ";\n";
	else if(n.type_id == "UiSplitButton")
		out << "\tUiSplitButton " << var << ";\n";
	else if(n.type_id == "UiToolButton")
		out << "\tUiToolButton " << var << ";\n";
	else if(n.type_id == "UiAccordion")
		out << "\tUiAccordion " << var << ";\n";
	else if(n.type_id == "UiLineEdit")
		out << "\tUiLineEdit " << var << ";\n";
	else if(n.type_id == "UiIntEdit")
		out << "\tUiIntEdit " << var << ";\n";
	else if(n.type_id == "UiFloatEdit")
		out << "\tUiFloatEdit " << var << ";\n";
	else if(n.type_id == "UiSlider")
		out << "\tUiSlider " << var << ";\n";	else if(n.type_id == "UiCompositeLabel")
		out << "\tUiCompositeLabel " << var << ";\n";
	else if(n.type_id == "UiCompositeEdit")
		out << "\tUiCompositeEdit " << var << ";\n";
	else if(n.type_id == "UiCompositeDropdown")
		out << "\tUiCompositeDropdown " << var << ";\n";
	else if(n.type_id == "UiCompositeToggle")
		out << "\tUiCompositeToggle " << var << ";\n";
	else if(n.type_id == "UiCompositeSlider")
		out << "\tUiCompositeSlider " << var << ";\n";
	else if(n.type_id == "UiSliderEdit")
		out << "\tUiSliderEdit " << var << ";\n";
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
	else if(n.type_id == "PaneSlot" || n.type_id == "PageSlot" || n.type_id == "AccordionSectionSlot")
		out << "\tParentCtrl " << var << ";\n";
	else if(n.type_id == "Spacer") {
		if((bool)CodeGenNodeProperty(n, "line_enabled", false))
			out << "\tUiPanel " << var << ";\n";
		else
			return;
	}
	else
		out << "\tUiPanel " << var << ";\n";
}

static String AxisSizing(const DesignerNode& n, const String& axis_key)
{
	return CodeGenNodeProperty(n, axis_key, "Fit");
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
	int min_w = DesignerClampMin((int)CodeGenNodeProperty(child, "min_width", DESIGNER_MIN_CLAMP));
	int min_h = DesignerClampMin((int)CodeGenNodeProperty(child, "min_height", DESIGNER_MIN_CLAMP));
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
	int fixed = horizontal ? DesignerClampMin((int)CodeGenFixedMetric(child, "height", DESIGNER_FIXED_FALLBACK_HEIGHT))
	                       : DesignerClampMin((int)CodeGenFixedMetric(child, "width", DESIGNER_FIXED_FALLBACK_WIDTH));
	fixed = max(fixed, cross_min);

	if(cross == "Fixed")
		out << Format(".MinMaxCross(DPI(%d), DPI(%d)).AlignSelf(UiBoxLayout::Align::Start)", fixed, fixed);
	else if(cross == "Fit")
		out << Format(".MinCross(DPI(%d)).AlignSelf(UiBoxLayout::Align::Start)", cross_min);
	else
		out << Format(".MinCross(DPI(%d)).AlignSelf(UiBoxLayout::Align::Stretch)", cross_min);
	return out;
}

static void EmitDirectChildLayout(String& out, const String& var, const DesignerNode& child)
{
	String hs = AxisSizing(child, "h_sizing");
	String vs = AxisSizing(child, "v_sizing");
	int min_w = DesignerClampMin((int)CodeGenNodeProperty(child, "min_width", DESIGNER_MIN_CLAMP));
	int min_h = DesignerClampMin((int)CodeGenNodeProperty(child, "min_height", DESIGNER_MIN_CLAMP));

	if(hs == "Expand")
		out << "\t\t" << var << ".HSizePosZ(0, 0);\n";
	else if(hs == "Fixed") {
		int w = max(DesignerClampMin((int)CodeGenFixedMetric(child, "width", DESIGNER_FIXED_FALLBACK_WIDTH)), min_w);
		out << "\t\t" << var << ".LeftPosZ(0, DPI(" << w << "));\n";
	}
	else
		out << "\t\t" << var << ".LeftPosZ(0, max(" << var << ".GetMinSize().cx, DPI(" << min_w << ")));\n";

	if(vs == "Expand")
		out << "\t\t" << var << ".VSizePosZ(0, 0);\n";
	else if(vs == "Fixed") {
		int h = max(DesignerClampMin((int)CodeGenFixedMetric(child, "height", DESIGNER_FIXED_FALLBACK_HEIGHT)), min_h);
		out << "\t\t" << var << ".TopPosZ(0, DPI(" << h << "));\n";
	}
	else
		out << "\t\t" << var << ".TopPosZ(0, max(" << var << ".GetMinSize().cy, DPI(" << min_h << ")));\n";
}

static String CompositeLayoutExpr(const String& mode)
{
	return mode == "Stacked" ? "UICOMPOSITE_STACKED" : "UICOMPOSITE_INLINE";
}

static String FieldAlignExpr(const String& side)
{
	if(side == "Left") return "UiAlign::LEFT";
	if(side == "Top") return "UiAlign::TOP";
	if(side == "Bottom") return "UiAlign::BOTTOM";
	return "UiAlign::RIGHT";
}

static void EmitCompositeSetup(String& out, const String& var, const DesignerNode& n)
{
	String label = CodeGenNodeProperty(n, "label", n.name);
	String value = CodeGenNodeProperty(n, "value_text", "Value");
	int label_w = DesignerClampMin((int)CodeGenNodeProperty(n, "label_width", 112));
	int field_gap = max(0, (int)CodeGenNodeProperty(n, "field_gap", 8));
	int stack_gap = max(0, (int)CodeGenNodeProperty(n, "stack_gap", 4));
	if(n.type_id == "UiCompositeLabel") {
		out << "\t\t" << var << ".SetLabel(" << CppString(label) << ").SetValueText(" << CppString(value) << ")"
		    << ".SetLabelWidth(DPI(" << label_w << ")).SetFieldGap(DPI(" << field_gap << "));\n";
	}
	else if(n.type_id == "UiCompositeEdit") {
		out << "\t\t" << var << ".SetLayoutMode(" << CompositeLayoutExpr(CodeGenNodeProperty(n, "layout_mode", "Inline")) << ")"
		    << ".SetLabel(" << CppString(label) << ").SetLabelWidth(DPI(" << label_w << "))"
		    << ".SetFieldGap(DPI(" << field_gap << ")).SetStackGap(DPI(" << stack_gap << "));\n"
		    << "\t\t" << var << ".SetData(" << CppString(value) << ");\n";
	}
	else if(n.type_id == "UiCompositeDropdown") {
		out << "\t\t" << var << ".SetLayoutMode(" << CompositeLayoutExpr(CodeGenNodeProperty(n, "layout_mode", "Inline")) << ")"
		    << ".SetLabel(" << CppString(label) << ").SetLabelWidth(DPI(" << label_w << "))"
		    << ".SetFieldGap(DPI(" << field_gap << ")).SetStackGap(DPI(" << stack_gap << "));\n"
		    << "\t\t" << var << ".Clear().Add(\"First\", \"First\").Add(\"Second\", \"Second\").Add(\"Third\", \"Third\");\n"
		    << "\t\t" << var << ".SelectByData(" << CppString(CodeGenNodeProperty(n, "selected", "First")) << ");\n";
	}
	else if(n.type_id == "UiCompositeToggle") {
		out << "\t\t" << var << ".SetLayoutMode(" << CompositeLayoutExpr(CodeGenNodeProperty(n, "layout_mode", "Inline")) << ")"
		    << ".SetLabel(" << CppString(label) << ").SetValueText(" << CppString(value) << ")"
		    << ".ShowValue(" << ((bool)CodeGenNodeProperty(n, "show_value", false) ? "true" : "false") << ")"
		    << ".SetLabelWidth(DPI(" << label_w << ")).SetValueWidth(DPI(" << max(0, (int)CodeGenNodeProperty(n, "value_width", 42)) << "))"
		    << ".SetFieldGap(DPI(" << field_gap << ")).SetStackGap(DPI(" << stack_gap << "));\n"
		    << "\t\t" << var << ".SetData(" << ((bool)CodeGenNodeProperty(n, "on", true) ? "true" : "false") << ");\n";
	}
	else if(n.type_id == "UiCompositeSlider") {
		int mn = (int)CodeGenNodeProperty(n, "min", 0);
		int mx = (int)CodeGenNodeProperty(n, "max", 100);
		int val = minmax((int)CodeGenNodeProperty(n, "value", 42), mn, mx);
		out << "\t\t" << var << ".SetLayoutMode(" << CompositeLayoutExpr(CodeGenNodeProperty(n, "layout_mode", "Inline")) << ")"
		    << ".SetLabel(" << CppString(label) << ").SetValueText(" << CppString(AsString(val)) << ")"
		    << ".ShowValue(" << ((bool)CodeGenNodeProperty(n, "show_value", true) ? "true" : "false") << ")"
		    << ".SetLabelWidth(DPI(" << label_w << ")).SetValueWidth(DPI(" << max(0, (int)CodeGenNodeProperty(n, "value_width", 48)) << "))"
		    << ".SetFieldGap(DPI(" << field_gap << ")).SetStackGap(DPI(" << stack_gap << "));\n"
		    << "\t\t" << var << ".Slider().SetRange(" << mn << ", " << mx << ");\n"
		    << "\t\t" << var << ".SetData(" << val << ");\n";
	}
	else if(n.type_id == "UiSliderEdit") {
		out << "\t\t" << var << ".SetRange(" << (double)CodeGenNodeProperty(n, "minf", 0.0) << ", "
		    << (double)CodeGenNodeProperty(n, "maxf", 100.0) << ")"
		    << ".SetStep(" << (double)CodeGenNodeProperty(n, "stepf", 1.0) << ")"
		    << ".SetValue(" << (double)CodeGenNodeProperty(n, "valuef", 42.0) << ")"
		    << ".SetFieldAlign(" << FieldAlignExpr(CodeGenNodeProperty(n, "field_align", "Right")) << ")"
		    << ".SetFieldWidth(DPI(" << max(0, (int)CodeGenNodeProperty(n, "field_width", 72)) << "))"
		    << ".SetGap(DPI(" << field_gap << "));\n";
	}
}static bool HasDesignerMinSizeOverride(const DesignerNode& n)
{
	return DesignerClampMin((int)CodeGenNodeProperty(n, "min_width", DESIGNER_MIN_CLAMP)) != DESIGNER_MIN_CLAMP ||
	       DesignerClampMin((int)CodeGenNodeProperty(n, "min_height", DESIGNER_MIN_CLAMP)) != DESIGNER_MIN_CLAMP;
}

static void EmitDesignerMinSize(String& out, const String& var, const DesignerNode& n)
{
	if(n.type_id == "Spacer" || n.type_id == "PaneSlot" || n.type_id == "UiPanel" ||
	   n.type_id == "PageSlot" || n.type_id == "AccordionSectionSlot")
		return;
	if(!HasDesignerMinSizeOverride(n))
		return;
	out << "\t\t" << var << ".SetMinSize(Size(DPI("
	    << DesignerClampMin((int)CodeGenNodeProperty(n, "min_width", DESIGNER_MIN_CLAMP))
	    << "), DPI("
	    << DesignerClampMin((int)CodeGenNodeProperty(n, "min_height", DESIGNER_MIN_CLAMP))
	    << ")));\n";
}

static void EmitSetup(String& out, const VectorMap<DesignerNodeId, String>& names,
                      const DesignerNode& n, bool emit_designer_appearance)
{
	String var = VarName(names, n.id);
	if(n.type_id == "PaneSlot" || n.type_id == "PageSlot" || n.type_id == "AccordionSectionSlot")
		return;
	EmitThemeStyle(out, var, n, emit_designer_appearance);
	EmitDesignerMinSize(out, var, n);
	if(n.type_id == "BoxLayout") {
		String wrap = CodeGenNodeProperty(n, "wrap", "None");
		out << "\t\t" << var << ".SetDirection(" << DirectionExpr(n, "V") << ")"
		    << ".SetGap(DPI(" << (int)CodeGenNodeProperty(n, "gap_x", (int)CodeGenNodeProperty(n, "gap", 8)) << "), "
		    << "DPI(" << (int)CodeGenNodeProperty(n, "gap_y", (int)CodeGenNodeProperty(n, "gap", 8)) << "))"
		    << ".SetInset(DPI(" << (int)CodeGenNodeProperty(n, "inset", 8) << "))"
		    << ".SetWrap(" << (wrap == "Snap" ? "UiBoxWrap::Snap" : wrap == "Flow" ? "UiBoxWrap::Flow" : "UiBoxWrap::None") << ")";
		if(wrap == "Snap") {
			out << ".SetWrapSnapCount(" << max(0, (int)CodeGenNodeProperty(n, "snap_count", 0)) << ")";
			int a = max(0, (int)CodeGenNodeProperty(n, "snap_size_a", 80));
			int b = max(0, (int)CodeGenNodeProperty(n, "snap_size_b", 0));
			if(a > 0 || b > 0) {
				out << ".SetWrapSnapSizes(Vector<int>()";
				if(a > 0)
					out << " << DPI(" << a << ")";
				if(b > 0)
					out << " << DPI(" << b << ")";
				out << ")";
			}
		}
		if((bool)CodeGenNodeProperty(n, "debug", false))
			out << ".SetDebugColor(" << ColorExpr(CodeGenDebugColor(n)) << ").SetDebug(true)";
		out << ";\n";
	}	else if(n.type_id == "UiCompositeLabel" || n.type_id == "UiCompositeEdit" ||
	        n.type_id == "UiCompositeDropdown" || n.type_id == "UiCompositeToggle" ||
	        n.type_id == "UiCompositeSlider" || n.type_id == "UiSliderEdit") {
		EmitCompositeSetup(out, var, n);
	}
	else if(n.type_id == "GridLayout") {
		out << "\t\t" << var << ".SetGridSize("
		    << max(1, (int)CodeGenNodeProperty(n, "columns", 2)) << ", "
		    << max(1, (int)CodeGenNodeProperty(n, "rows", 2)) << ")"
		    << ".SetMinCellSize(Size(DPI(" << DesignerClampMin((int)CodeGenNodeProperty(n, "cell_width", DESIGNER_GRID_CELL_WIDTH))
		    << "), DPI(" << DesignerClampMin((int)CodeGenNodeProperty(n, "cell_height", DESIGNER_GRID_CELL_HEIGHT)) << ")))"
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
			    << "\t\t\ts.thumb_cross = DPI(" << thumb_h << ");\n";
		}
		else {
			out << "\t\t\ts.thumb_main = DPI(" << thumb_h << ");\n"
			    << "\t\t\ts.thumb_cross = DPI(" << thumb_w << ");\n";
		}
		String grip_visual = CodeGenNodeProperty(n, "grip_visual", "");
		if(grip_visual.IsEmpty()) {
			if(CodeGenHasProperty(n, "show_grip") && !(bool)CodeGenNodeProperty(n, "show_grip", true))
				grip_visual = "None";
			else if(!IconExpr(CodeGenNodeProperty(n, "thumb_icon", "None")).IsEmpty())
				grip_visual = "Icon";
			else
				grip_visual = "Lines";
		}
		out << "\t\t\ts.grip_visual = " << (grip_visual == "None" ? "UISPLITTER_GRIP_NONE"
		                                    : grip_visual == "Dots" ? "UISPLITTER_GRIP_DOTS"
		                                    : grip_visual == "Icon" ? "UISPLITTER_GRIP_ICON"
		                                    : "UISPLITTER_GRIP_LINES") << ";\n"
		    << "\t\t\ts.grip_count = " << max(1, (int)CodeGenNodeProperty(n, "grip_count", 2)) << ";\n"
		    << "\t\t\ts.grip_size = DPI(" << max(1, (int)CodeGenNodeProperty(n, "grip_size", 2)) << ");\n"
		    << "\t\t\ts.grip_gap = DPI(" << max(0, (int)CodeGenNodeProperty(n, "grip_gap", 3)) << ");\n";
		if(CodeGenHasProperty(n, "grip_color_enabled") && (bool)CodeGenNodeProperty(n, "grip_color_enabled", false))
			out << "\t\t\ts.grip_color = " << ColorExpr(CodeGenNodeProperty(n, "grip_color", Null)) << ";\n";
		else
			out << "\t\t\ts.grip_color = Null;\n";
		String icon = IconExpr(CodeGenNodeProperty(n, "thumb_icon", "None"));
		if(!icon.IsEmpty()) {
			out << "\t\t\ts.thumb_icon = " << icon << ";\n"
			    << "\t\t\ts.grip_visual = UISPLITTER_GRIP_ICON;\n";
		}
		out << "\t\t\ts.thumb_icon_size = DPI(" << max(1, (int)CodeGenNodeProperty(n, "thumb_icon_size", 14)) << ");\n";
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
		out << "\t\t" << var << ".SetAlign(" << AlignHExpr(CodeGenNodeProperty(n, "align_h", CodeGenNodeProperty(n, "align", "Left")), "Left")
		    << ", " << AlignVExpr(CodeGenNodeProperty(n, "align_v", "Center"), "Center") << ");\n";
		out << "\t\t" << var << ".SetIconSide(" << AlignSideExpr(CodeGenNodeProperty(n, "icon_side", "Left"), "Left") << ");\n";
		out << "\t\t" << var << ".SetContentGap(DPI(" << max(0, (int)CodeGenNodeProperty(n, "content_gap", 6)) << "));\n";
		out << "\t\t" << var << ".SetIconScaleToContent(" << ((bool)CodeGenNodeProperty(n, "icon_scale", false) ? "true" : "false") << ");\n";
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
	else if(n.type_id == "UiGroupPanel") {
		out << "\t\t" << var << ".SetTitle(" << CppString(CodeGenNodeProperty(n, "text", n.name)) << ")"
		    << ".SetSubTitle(" << CppString(CodeGenNodeProperty(n, "subtitle", "")) << ")"
		    << ".SetSideTitle(" << CppString(CodeGenNodeProperty(n, "side_title", "")) << ")"
		    << ".SetHeaderMode(" << GroupHeaderModeExpr(CodeGenNodeProperty(n, "header_mode", "Inside")) << ")"
		    << ".SetHeaderPlacement(" << AlignSideExpr(CodeGenNodeProperty(n, "placement", "Top"), "Top") << ")"
		    << ".SetLine(" << ((bool)CodeGenNodeProperty(n, "line", false) ? "true" : "false") << ")"
		    << ".SetHeaderBand(" << ((bool)CodeGenNodeProperty(n, "header_band", false) ? "true" : "false") << ")"
		    << ".SetLineThickness(DPI(" << (int)CodeGenNodeProperty(n, "line_thickness", 1) << "))"
		    << ".SetInset(Rect(DPI(" << (int)CodeGenNodeProperty(n, "inset", 8) << "), DPI("
		    << (int)CodeGenNodeProperty(n, "inset", 8) << "), DPI("
		    << (int)CodeGenNodeProperty(n, "inset", 8) << "), DPI("
		    << (int)CodeGenNodeProperty(n, "inset", 8) << ")))"
		    << ".SetHeaderInset(Rect(DPI(" << (int)CodeGenNodeProperty(n, "header_inset", 6) << "), DPI("
		    << (int)CodeGenNodeProperty(n, "header_inset", 6) << "), DPI("
		    << (int)CodeGenNodeProperty(n, "header_inset", 6) << "), DPI("
		    << (int)CodeGenNodeProperty(n, "header_inset", 6) << ")));\n";
		String icon = IconExpr(CodeGenNodeProperty(n, "icon", "None"));
		if(!icon.IsEmpty())
			out << "\t\t" << var << ".SetIcon(" << icon << ").SetIconSize(DPI("
			    << (int)CodeGenNodeProperty(n, "icon_size", 16) << "));\n";
	}
	else if(n.type_id == "UiButton") {
		out << "\t\t" << var << ".SetText(" << CppString(CodeGenNodeProperty(n, "text", n.name)) << ")"
		    << ".SetContentInset(DPI(" << max(0, (int)CodeGenNodeProperty(n, "content_inset", 6)) << "))"
		    << ".SetContentGap(DPI(" << max(0, (int)CodeGenNodeProperty(n, "content_gap", 4)) << "));\n";
		out << "\t\t" << var << ".SetAlign(" << AlignHExpr(CodeGenNodeProperty(n, "align_h", CodeGenNodeProperty(n, "align", "Center")))
		    << ", " << AlignVExpr(CodeGenNodeProperty(n, "align_v", "Center")) << ");\n";
		out << "\t\t" << var << ".SetIconSide(" << AlignSideExpr(CodeGenNodeProperty(n, "icon_side", "Left"), "Left") << ");\n";
		String icon = IconExpr(CodeGenNodeProperty(n, "icon", "None"));
		if(!icon.IsEmpty())
			out << "\t\t" << var << ".SetIcon(" << icon << ").SetIconSize(DPI("
			    << (int)CodeGenNodeProperty(n, "icon_size", 16) << "), DPI("
			    << (int)CodeGenNodeProperty(n, "icon_size", 16) << "));\n";
	}
	else if(n.type_id == "UiSplitButton") {
		out << "\t\t" << var << ".SetText(" << CppString(CodeGenNodeProperty(n, "text", n.name)) << ")"
		    << ".SetContentInset(DPI(" << max(0, (int)CodeGenNodeProperty(n, "content_inset", 6)) << "))"
		    << ".SetContentGap(DPI(" << max(0, (int)CodeGenNodeProperty(n, "content_gap", 4)) << "))"
		    << ".SetSplitWidth(DPI(" << (int)CodeGenNodeProperty(n, "split_width", 30) << "))"
		    << ".SetSplitContentGap(DPI(" << max(0, (int)CodeGenNodeProperty(n, "split_content_gap", 4)) << "))"
		    << ".SetSplitIconSize(DPI(" << max(8, (int)CodeGenNodeProperty(n, "split_icon_size", 16)) << "))"
		    << ".SetPopupMinWidth(DPI(" << (int)CodeGenNodeProperty(n, "popup_min_width", 220) << "));\n";
		out << "\t\t" << var << ".SetAlign(" << AlignHExpr(CodeGenNodeProperty(n, "align_h", CodeGenNodeProperty(n, "align", "Center")))
		    << ", " << AlignVExpr(CodeGenNodeProperty(n, "align_v", "Center")) << ");\n";
		out << "\t\t" << var << ".SetIconSide(" << AlignSideExpr(CodeGenNodeProperty(n, "icon_side", "Left"), "Left") << ");\n";
		String icon = IconExpr(CodeGenNodeProperty(n, "icon", "None"));
		if(!icon.IsEmpty())
			out << "\t\t" << var << ".SetIcon(" << icon << ").SetIconSize(DPI("
			    << (int)CodeGenNodeProperty(n, "icon_size", 16) << "), DPI("
			    << (int)CodeGenNodeProperty(n, "icon_size", 16) << "));\n";
		out << "\t\t" << var << ".Add(" << CppString(CodeGenNodeProperty(n, "choice_a", "Recent A")) << ", \"a\")"
		    << ".Add(" << CppString(CodeGenNodeProperty(n, "choice_b", "Recent B")) << ", \"b\")"
		    << ".Add(" << CppString(CodeGenNodeProperty(n, "choice_c", "Recent C")) << ", \"c\");\n";
	}
	else if(n.type_id == "UiToolButton") {
		out << "\t\t" << var << ".SetText(" << CppString(CodeGenNodeProperty(n, "text", "")) << ")"
		    << ".SetContentInset(DPI(" << max(0, (int)CodeGenNodeProperty(n, "content_inset", 4)) << "))"
		    << ".SetContentGap(DPI(" << max(0, (int)CodeGenNodeProperty(n, "content_gap", 4)) << "));\n";
		out << "\t\t" << var << ".SetAlign(" << AlignHExpr(CodeGenNodeProperty(n, "align_h", CodeGenNodeProperty(n, "align", "Center")))
		    << ", " << AlignVExpr(CodeGenNodeProperty(n, "align_v", "Center")) << ");\n";
		out << "\t\t" << var << ".SetIconSide(" << AlignSideExpr(CodeGenNodeProperty(n, "icon_side", "Center"), "Center") << ");\n";
		String icon = IconExpr(CodeGenNodeProperty(n, "icon", "None"));
		if(!icon.IsEmpty())
			out << "\t\t" << var << ".SetIcon(" << icon << ").SetIconSize(DPI("
			    << (int)CodeGenNodeProperty(n, "icon_size", 20) << "), DPI("
			    << (int)CodeGenNodeProperty(n, "icon_size", 20) << "));\n";
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
		String item_text = CodeGenNodeProperty(n, "item_text", "First");
		out << "\t\t" << var << ".UseInternalModel().Clear().Add(" << CppString(item_text) << ", " << CppString(item_text) << ");\n";
		out << "\t\t" << var << ".Select(0);\n";
	}
	else if(n.type_id == "UiCheckBox") {
		out << "\t\t" << var << ".SetText(" << CppString(CodeGenNodeProperty(n, "text", n.name)) << ")"
		    << ".SetTriState(" << ((bool)CodeGenNodeProperty(n, "tri_state", false) ? "true" : "false") << ");\n";
		String visual = CodeGenNodeProperty(n, "visual", "Classic");
		if(visual == "Chip")
			out << "\t\t" << var << ".SetVisual(UICHECKVIS_CHIP);\n";
		else if(visual == "List")
			out << "\t\t" << var << ".SetVisual(UICHECKVIS_LIST);\n";
		else
			out << "\t\t" << var << ".SetVisual(UICHECKVIS_CLASSIC);\n";
		String state = CodeGenNodeProperty(n, "state", "Checked");
		out << "\t\t" << var << ".SetState(" << (state == "Indeterminate" ? "UICHECK_INDETERMINATE" :
		                                       state == "Unchecked" ? "UICHECK_UNCHECKED" : "UICHECK_CHECKED") << ");\n";
	}
	else if(n.type_id == "UiBreadcrumbs") {
		int count = CodeGenBreadcrumbCount(n);
		for(int i = 0; i < count; i++)
			out << "\t\t" << var << ".AddCrumb(" << CppString(CodeGenBreadcrumbCrumbText(n, i)) << ", "
			    << CppString(AsString(i)) << ");\n";
		out << "\t\t" << var << ".SetCurrentIndex("
		    << clamp((int)CodeGenNodeProperty(n, "current", min(2, count - 1)), 0, count - 1) << ");\n";
		out << "\t\t" << var << ".SetTrimOnSelect(" << ((bool)CodeGenNodeProperty(n, "trim", false) ? "true" : "false")
		    << ").SetDivider(" << CppString(CodeGenNodeProperty(n, "divider", "/")) << ");\n";
		String divider_icon = IconExpr(CodeGenNodeProperty(n, "divider_icon", "None"));
		if(!divider_icon.IsEmpty())
			out << "\t\t" << var << ".SetDividerIcon(" << divider_icon << ", Size(DPI("
			    << (int)CodeGenNodeProperty(n, "divider_icon_size", 14) << "), DPI("
			    << (int)CodeGenNodeProperty(n, "divider_icon_size", 14) << ")));\n";
		String icon = IconExpr(CodeGenNodeProperty(n, "icon", "None"));
		if(!icon.IsEmpty())
			out << "\t\t" << var << ".SetPathIcon(" << icon << ", UiAlign::LEFT, Size(DPI("
			    << (int)CodeGenNodeProperty(n, "icon_size", 16) << "), DPI("
			    << (int)CodeGenNodeProperty(n, "icon_size", 16) << ")));\n";
	}
	else if(n.type_id == "UiAccordion") {
		out << "\t\t" << var << ".SetSingleOpen(" << ((bool)CodeGenNodeProperty(n, "single_open", false) ? "true" : "false") << ")"
		    << ".SetEnforceOne(" << ((bool)CodeGenNodeProperty(n, "enforce_one", false) ? "true" : "false") << ")"
		    << ".ShowChevron(" << ((bool)CodeGenNodeProperty(n, "show_chevron", true) ? "true" : "false") << ")"
		    << ".SetChevronSide(" << AlignSideExpr(CodeGenNodeProperty(n, "chevron_side", "Right"), "Right") << ")"
		    << ".SetAnimation(" << ((bool)CodeGenNodeProperty(n, "animation", true) ? "true" : "false") << ", "
		    << (int)CodeGenNodeProperty(n, "open_ms", 120) << ", "
		    << (int)CodeGenNodeProperty(n, "close_ms", 0) << ")"
		    << ".ShowDragHandle(" << ((bool)CodeGenNodeProperty(n, "show_drag_handle", false) ? "true" : "false") << ")"
		    << ".EnableDragReorder(" << ((bool)CodeGenNodeProperty(n, "drag_reorder", false) ? "true" : "false") << ");\n";
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
	else if(n.type_id == "UiPanel") {
		out << "\t\t" << var << ".SetSizeMin(DPI("
		    << DesignerClampMin((int)CodeGenNodeProperty(n, "min_width", DESIGNER_MIN_CLAMP))
		    << "), DPI("
		    << DesignerClampMin((int)CodeGenNodeProperty(n, "min_height", DESIGNER_MIN_CLAMP))
		    << "));\n";
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
	if(child.type_id == "Spacer") {
		String kind = CodeGenNodeProperty(child, "spacer_kind", "Expander");
		int size = max(0, (int)CodeGenNodeProperty(child, "space", 24));
		int max_size = max(size, (int)CodeGenNodeProperty(child, "max_space", 1000000));
		int weight = max(1, (int)CodeGenNodeProperty(child, "weight", 1));
		if(parent.type_id == "BoxLayout") {
			if(kind == "Break")
				out << "\t\t" << p << ".AddBreak(" << weight << ");\n";
			else {
				out << "\t\t{\n";
				if(kind == "Fixed")
					out << "\t\t\tauto spacer = " << p << ".AddSpacer(1).Fixed(DPI(" << size << "));\n";
				else if(kind == "Bounded")
					out << "\t\t\tauto spacer = " << p << ".AddSpacer(" << weight << ").MinMain(DPI(" << size << ")).MaxMain(DPI(" << max_size << "));\n";
				else
					out << "\t\t\tauto spacer = " << p << ".AddSpacer(" << weight << ");\n";
				if((bool)CodeGenNodeProperty(child, "line_enabled", false)) {
					out << "\t\t\tspacer.LineEnabled(true)"
					    << ".LineStyle(" << SpacerLineStyleExpr(child) << ")"
					    << ".LineAlign(" << SpacerLineAlignExpr(child) << ")"
					    << ".LineThickness(DPI(" << SpacerLineThickness(child) << "))"
					    << ".LineDash(" << SpacerLineDashExpr(child) << ")"
					    << ".LineInset(DPI(" << max(0, (int)CodeGenNodeProperty(child, "line_inset", 0)) << "))";
					if(CodeGenHasProperty(child, "line_color_enabled") &&
					   (bool)CodeGenNodeProperty(child, "line_color_enabled", false))
						out << ".LineColorEnabled(true).LineColor(" << SpacerLineColorExpr(child) << ")";
					out << ";\n";
				}
				out << "\t\t}\n";
			}
			return;
		}
		if(parent.type_id == "GridLayout") {
			int columns = max(1, (int)CodeGenNodeProperty(parent, "columns", 2));
			int rows = max(1, (int)CodeGenNodeProperty(parent, "rows", 2));
			int row = clamp((int)CodeGenNodeProperty(child, "grid_row", index / columns), 0, rows - 1);
			int col = clamp((int)CodeGenNodeProperty(child, "grid_col", index % columns), 0, columns - 1);
			if(kind == "Break")
				return;
			out << "\t\t{\n";
			if(kind == "Fixed")
				out << "\t\t\tint item = " << p << ".AddGap(DPI(" << size << "));\n";
			else if(kind == "Bounded")
				out << "\t\t\tint item = " << p << ".AddSpacer(DPI(" << size << "), DPI(" << max_size << "));\n";
			else
				out << "\t\t\tint item = " << p << ".AddExpand(" << weight << ");\n";
			if((bool)CodeGenNodeProperty(child, "line_enabled", false)) {
				out << "\t\t\t" << p << ".SetItemSeparatorLine(item, true, " << SpacerLineStyleExpr(child)
				    << ", " << SpacerLineAlignExpr(child) << ", DPI(" << SpacerLineThickness(child)
				    << "), " << SpacerLineDashExpr(child) << ", DPI(" << max(0, (int)CodeGenNodeProperty(child, "line_inset", 0)) << ")";
				if(CodeGenHasProperty(child, "line_color_enabled") &&
				   (bool)CodeGenNodeProperty(child, "line_color_enabled", false))
					out << ", " << SpacerLineColorExpr(child);
				else
					out << ", Null";
				out << ");\n";
			}
			out << "\t\t}\n";
			return;
		}
		return;
	}
	if(parent.id == Designer_ROOT)
		out << "\t\tAdd(" << c << ".SizePos());\n";
	else if(parent.type_id == "BoxLayout")
		out << "\t\t" << p << ".Add(" << c << ")" << BoxSizingCall(parent, child) << ";\n";
	else if(parent.type_id == "GridLayout") {
		int columns = max(1, (int)CodeGenNodeProperty(parent, "columns", 2));
		int rows = max(1, (int)CodeGenNodeProperty(parent, "rows", 2));
		int row = clamp((int)CodeGenNodeProperty(child, "grid_row", index / columns), 0, rows - 1);
		int col = clamp((int)CodeGenNodeProperty(child, "grid_col", index % columns), 0, columns - 1);
		String hs = AxisSizing(child, "h_sizing");
		String vs = AxisSizing(child, "v_sizing");
		if(hs == "Fixed" || vs == "Fixed") {
			int w = DesignerClampMin((int)CodeGenFixedMetric(child, "width", DESIGNER_FIXED_FALLBACK_WIDTH));
			int h = DesignerClampMin((int)CodeGenFixedMetric(child, "height", DESIGNER_FIXED_FALLBACK_HEIGHT));
			out << "\t\t{\n";
			out << "\t\t\tint item = " << p << ".Add(" << c << ", " << row << ", " << col
			    << ", " << (hs == "Expand" ? "true" : "false")
			    << ", " << (vs == "Expand" ? "true" : "false")
			    << ", Size(DPI(" << w << "), DPI(" << h << ")));\n";
			out << "\t\t\t" << p << ".SetItemAlign(item, " << GridItemAlignHExpr(child) << ", " << GridItemAlignVExpr(child) << ");\n";
			out << "\t\t}\n";
		}
		else {
			out << "\t\t{\n";
			out << "\t\t\tint item = " << p << ".Add(" << c << ", " << row << ", " << col
			    << ", " << (hs == "Expand" ? "true" : "false")
			    << ", " << (vs == "Expand" ? "true" : "false") << ");\n";
			out << "\t\t\t" << p << ".SetItemAlign(item, " << GridItemAlignHExpr(child) << ", " << GridItemAlignVExpr(child) << ");\n";
			out << "\t\t}\n";
		}
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
	else if(parent.type_id == "UiAccordion") {
        String title = child.type_id == "AccordionSectionSlot" ? AsString(CodeGenNodeProperty(child, "section_title", child.name)) : child.name;
        String subtitle = child.type_id == "AccordionSectionSlot" ? AsString(CodeGenNodeProperty(child, "section_subtitle", "")) : String();

		bool open = child.type_id == "AccordionSectionSlot" ? (bool)CodeGenNodeProperty(child, "open", true) : true;
		out << "\t\t{\n";
		out << "\t\t\tint section = " << p << ".AddSection(" << CppString(title) << ", "
		    << CppString(subtitle) << ", String(), " << (open ? "true" : "false") << ");\n";
		if(child.type_id == "AccordionSectionSlot") {
			String lock = CodeGenNodeProperty(child, "lock", "None");
			if(lock != "None")
				out << "\t\t\t" << p << ".SetLockMode(section, UiAccordion::Lock::" << lock << ");\n";
			int body_height = (int)CodeGenNodeProperty(child, "body_height", -1);
			if(body_height > 0)
				out << "\t\t\t" << p << ".SetSectionBodyHeight(section, DPI(" << body_height << "));\n";
		}
		out << "\t\t\t" << p << ".GetSectionContent(section).Add(" << c << ".SizePos());\n";
		out << "\t\t}\n";
	}
	else if(parent.type_id == "UiScrollPanel")
		out << "\t\t" << p << ".Content().Add(" << c << ".SizePos());\n";
	else if(parent.type_id == "UiGroupPanel")
		out << "\t\t" << p << ".SetContent(" << c << ");\n";
	else if(parent.type_id == "UiPanel")
		out << "\t\t" << p << ".Add(" << c << ".SizePos());\n";
	else if(parent.type_id == "PaneSlot" || parent.type_id == "PageSlot" || parent.type_id == "AccordionSectionSlot")
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
