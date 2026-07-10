#include "DesignerControlFamilyShared.h"
#include "../DesignerCodeGen.h"

namespace Upp {

static String DefaultBaseNameFromId(const String& id)
{
	String base = id;
	if(base.StartsWith("Ui") && base.GetCount() > 2 && IsUpper(base[2]))
		base = base.Mid(2);
	if(!base.IsEmpty())
		base = String(ToLower((int)(byte)base[0]), 1) + base.Mid(1);
	return base;
}

Color DesignerLayoutFace()  { return Color(255, 224, 178); }
Color DesignerLayoutFrame() { return Color(217, 119, 6); }
Color DesignerDebugRed()    { return Color(220, 38, 38); }
Color DesignerPanelFace()   { return Color(187, 232, 203); }
Color DesignerPanelFrame()  { return Color(34, 150, 91); }
Color DesignerControlFace() { return Color(203, 224, 255); }
Color DesignerControlFrame(){ return Color(54, 116, 210); }

DesignerType MakeControlType(const String& id, const String& name, Size size)
{
	DesignerType t;
	t.id = id;
	t.display_name = name;
	t.default_base_name = DefaultBaseNameFromId(id);
	t.toolbox_group = "Controls";
	t.runtime_cpp_type = id;
	t.capabilities.is_visible_control = true;
	t.capabilities.supports_inspector = true;
	t.capabilities.supports_preview = true;
	t.capabilities.supports_codegen = true;
	t.capabilities.supports_appearance_overrides = true;
	t.capabilities.supports_theme_export = true;
	t.theme_default_source = "UiTheme role-aware resolved defaults";
	SetDesignerThemeSchema(t,
		{"theme_override", "face_enabled", "face", "face_mode", "face_quad",
		 "frame_enabled", "frame", "frame_style", "frame_width", "radius",
		 "shadow_enabled", "shadow_distance", "shadow_offset_x", "shadow_offset_y",
		 "shadow_alpha", "shadow_color", "shadow_curve"});
	t.default_size = size;
	t.min_size = DesignerMinSize();
	t.init_defaults = [=](DesignerNode& n) {
		n.properties.Set("text", name);
		n.properties.Set("role", "Standard");
		n.properties.Set("h_sizing", "Fit");
		n.properties.Set("v_sizing", "Fit");
		n.properties.Set("fixed_width", size.cx);
		n.properties.Set("fixed_height", size.cy);
		n.properties.Set("width", size.cx);
		n.properties.Set("height", size.cy);
		n.properties.Set("face", DesignerControlFace());
		n.properties.Set("frame", DesignerControlFrame());
		n.properties.Set("radius", 0);
		n.properties.Set("face_enabled", false);
		n.properties.Set("frame_enabled", false);
		n.properties.Set("font", "Sans");
		n.properties.Set("font_size", 11);
		n.properties.Set("align", "Left");
		n.properties.Set("align_h", "Left");
		n.properties.Set("align_v", "Center");
		n.properties.Set("icon", "None");
		n.properties.Set("icon_size", 18);
	};
	return t;
}

DesignerType MakeCompositeType(const String& id, const String& name, Size size)
{
	DesignerType t = MakeControlType(id, name, size);
	t.toolbox_group = "Composites";
	t.capabilities.supports_appearance_overrides = false;
	t.capabilities.supports_theme_export = false;
	t.default_size = size;
	t.min_size = DesignerMinSize();
	t.init_defaults = [=](DesignerNode& n) {
		n.properties.Set("label", name);
		n.properties.Set("value_text", "Value");
		n.properties.Set("selected", "First");
		n.properties.Set("on", true);
		n.properties.Set("min", 0);
		n.properties.Set("max", 100);
		n.properties.Set("value", 42);
		n.properties.Set("minf", 0.0);
		n.properties.Set("maxf", 100.0);
		n.properties.Set("valuef", 42.0);
		n.properties.Set("stepf", 1.0);
		n.properties.Set("show_value", false);
		n.properties.Set("layout_mode", "Inline");
		n.properties.Set("label_width", 112);
		n.properties.Set("value_width", 48);
		n.properties.Set("field_width", 72);
		n.properties.Set("field_align", "Right");
		n.properties.Set("field_gap", 8);
		n.properties.Set("stack_gap", 4);
		n.properties.Set("h_sizing", "Fit");
		n.properties.Set("v_sizing", "Fit");
		n.properties.Set("fixed_width", size.cx);
		n.properties.Set("fixed_height", size.cy);
		n.properties.Set("width", size.cx);
		n.properties.Set("height", size.cy);
		n.properties.Set("face_enabled", false);
		n.properties.Set("frame_enabled", false);
		n.properties.Set("radius", 0);
	};
	return t;
}

DesignerType MakeGenericType()
{
	DesignerType t = MakeControlType("Generic", "Generic", Size(140, 48));
	t.toolbox_group.Clear();
	t.display_name = "Generic";
	t.runtime_cpp_type = "UiPanel";
	t.icon = ICON_DESIGN_PANEL_48();
	SetDesignerAdapterFactory<DesignerPanelAdapter>(t);
	t.init_defaults = [](DesignerNode& n) {
		n.properties.Set("text", "Generic");
		n.properties.Set("original_type", "");
		n.properties.Set("h_sizing", "Fit");
		n.properties.Set("v_sizing", "Fit");
		n.properties.Set("fixed_width", 140);
		n.properties.Set("fixed_height", 48);
		n.properties.Set("width", 140);
		n.properties.Set("height", 48);
		n.properties.Set("face", DesignerControlFace());
		n.properties.Set("frame", DesignerControlFrame());
		n.properties.Set("radius", 4);
		n.properties.Set("face_enabled", true);
		n.properties.Set("frame_enabled", true);
	};
	return t;
}

DesignerType MakePageContainerType(const String& id, const String& name, Size size)
{
	DesignerType t = MakeControlType(id, name, size);
	t.toolbox_group = "Containers";
	t.is_container = true;
	t.can_have_children = true;
	t.capabilities.is_container = true;
	t.capabilities.is_page_container = true;
	t.capabilities.can_have_children = true;
	t.capabilities.supports_children = true;
	t.capabilities.requires_default_child_slots = true;
	t.default_child_slots = DesignerDefaultChildSlotSet::PageContainerThreePages;
	t.child_emission = DesignerLayoutChildEmissionStrategy::PageContainerPage;
	t.codegen.route = DesignerCodeGenRoute::StructuralCentral;
	t.codegen.emit_setup = [](DesignerCodeGenContext& ctx, const DesignerNode& n) {
		if(n.type_id != "UiTab")
			return;
		String visual = AsString(ctx.Property(n, "visual", "Underline"));
		String& out = ctx.Out();
		String var = ctx.Var(n);
		out << "\t\t" << var << ".SetVisual(" << (visual == "Segmented" ? "UITAB_SEGMENTED" : visual == "Rail" ? "UITAB_RAIL" : visual == "Document" ? "UITAB_DOCUMENT" : visual == "Underline" ? "UITAB_UNDERLINE" : "UITAB_CLASSIC") << ")"
		    << ".SetPlacement(" << ctx.AlignSideExpr(ctx.Property(n, "placement", "Top"), "Top") << ")"
		    << ".SetExpandTabs(" << ((bool)ctx.Property(n, "expand_tabs", false) ? "true" : "false") << ")"
		    << ".EnableCloseButtons(" << ((bool)ctx.Property(n, "close_buttons", false) ? "true" : "false") << ")"
		    << ".EnableDragHandles(" << ((bool)ctx.Property(n, "drag_handles", false) ? "true" : "false") << ");\n";
		String font_family = AsString(ctx.Property(n, "tab_font", "Sans"));
		int font_size = max(7, (int)ctx.Property(n, "tab_font_size", 11));
		String font_expr = font_family == "Mono" ? Format("MonospaceZ(%d)", font_size)
		                   : font_family == "Serif" ? Format("SerifZ(%d)", font_size)
		                   : (font_family == "Segoe UI" || font_family == "Arial" || font_family == "Verdana" ||
		                      font_family == "Tahoma" || font_family == "Consolas")
		                         ? Format("Font().FaceName(%s).Height(%d)", ctx.CppString(font_family), font_size)
		                         : Format("SansSerifZ(%d)", font_size);
		out << "\t\t" << var << ".SetTabFont(" << font_expr
		    << ").SetTabIconSize(DPI(" << (int)ctx.Property(n, "tab_icon_size", 16) << "))"
		    << ".SetTabIconSide(" << ctx.AlignSideExpr(ctx.Property(n, "tab_icon_side", "Left"), "Left") << ");\n";
		out << "\t\t" << var << ".SetContentGap(DPI(" << max(0, (int)ctx.Property(n, "content_gap", 6)) << "))"
		    << ".SetAffordanceGap(DPI(" << max(0, (int)ctx.Property(n, "affordance_gap", 4)) << "));\n";
	};
	t.codegen.emit_child = EmitDesignerLayoutChild;
	t.codegen.emit_post_build = [](DesignerCodeGenContext& ctx, const DesignerNode& n) {
		if(n.type_id == "UiTab")
			ctx.Out() << "\t\t" << ctx.Var(n) << ".SetActiveTab(" << (int)ctx.Property(n, "active", 0) << ");\n";
		else if(n.type_id == "UiStack")
			ctx.Out() << "\t\t" << ctx.Var(n) << ".SetActivePage(" << (int)ctx.Property(n, "active", 0) << ");\n";
	};
	t.init_defaults = [=](DesignerNode& n) {
		n.properties.Set("text", name);
		n.properties.Set("h_sizing", "Expand");
		n.properties.Set("v_sizing", "Expand");
		n.properties.Set("fixed_width", size.cx);
		n.properties.Set("fixed_height", size.cy);
		n.properties.Set("width", size.cx);
		n.properties.Set("height", size.cy);
		n.properties.Set("face_enabled", false);
		n.properties.Set("frame_enabled", false);
		n.properties.Set("radius", 0);
		n.properties.Set("active", 0);
	};
	return t;
}

DesignerType MakePanelControlType(const String& id, const String& name, Size size)
{
	DesignerType t = MakeControlType(id, name, size);
	t.toolbox_group = "Containers";
	t.is_container = true;
	t.can_have_children = true;
	t.capabilities.is_container = true;
	t.capabilities.can_have_children = true;
	t.capabilities.supports_children = true;
	t.codegen.emit_setup = [](DesignerCodeGenContext& ctx, const DesignerNode& n) {
		String& out = ctx.Out();
		if(n.type_id == "UiPanel")
			out << "\t\t" << ctx.Var(n) << ".SetSizeMin(DPI(" << (int)ctx.Property(n, "min_width", 0)
			    << "), DPI(" << (int)ctx.Property(n, "min_height", 0) << "));\n";
		else if(n.type_id == "UiScrollPanel") {
			String mode = AsString(ctx.Property(n, "scroll_mode", "Auto"));
			String expr = mode == "Vertical" ? "UIPANELSCROLL_VERTICAL" :
			              mode == "Horizontal" ? "UIPANELSCROLL_HORIZONTAL" :
			              mode == "None" ? "UIPANELSCROLL_NONE" : "UIPANELSCROLL_AUTO";
			out << "\t\t" << ctx.Var(n) << ".SetScrollMode(" << expr << ");\n";
		}
	};
	t.codegen.emit_child = EmitDesignerLayoutChild;
	t.init_defaults = [=](DesignerNode& n) {
		n.properties.Set("text", name);
		n.properties.Set("h_sizing", "Expand");
		n.properties.Set("v_sizing", "Expand");
		n.properties.Set("fixed_width", size.cx);
		n.properties.Set("fixed_height", size.cy);
		n.properties.Set("width", size.cx);
		n.properties.Set("height", size.cy);
		n.properties.Set("face", DesignerPanelFace());
		n.properties.Set("frame", DesignerPanelFrame());
		n.properties.Set("radius", 8);
		n.properties.Set("inset", 0);
		n.properties.Set("face_enabled", false);
		n.properties.Set("frame_enabled", false);
	};
	return t;
}

DesignerType MakeGroupPanelType()
{
	DesignerType t = MakeControlType("UiGroupPanel", "Group Panel", Size(260, 160));
	t.toolbox_group = "Containers";
	t.is_container = true;
	t.can_have_children = true;
	t.icon = ICON_DESIGN_BORDER_OUTER_48();
	t.capabilities.is_container = true;
	t.capabilities.can_have_children = true;
	t.capabilities.supports_children = true;
	t.child_emission = DesignerLayoutChildEmissionStrategy::GroupPanelContent;
	t.codegen.route = DesignerCodeGenRoute::StructuralCentral;
	t.codegen.emit_setup = [](DesignerCodeGenContext& ctx, const DesignerNode& n) {
		String& out = ctx.Out();
		String var = ctx.Var(n);
		String header_mode = AsString(ctx.Property(n, "header_mode", "Inside"));
		String header_expr = header_mode == "Outside" ? "UiGroupPanel::Outside" :
		                     header_mode == "Center" ? "UiGroupPanel::Center" : "UiGroupPanel::Inside";
		out << "\t\t" << var << ".SetTitle(" << ctx.CppString(ctx.Property(n, "text", n.name)) << ")"
		    << ".SetSubTitle(" << ctx.CppString(ctx.Property(n, "subtitle", "")) << ")"
		    << ".SetSideTitle(" << ctx.CppString(ctx.Property(n, "side_title", "")) << ")"
		    << ".SetHeaderMode(" << header_expr << ")"
		    << ".SetHeaderPlacement(" << ctx.AlignSideExpr(ctx.Property(n, "placement", "Top"), "Top") << ")"
		    << ".SetLine(" << ((bool)ctx.Property(n, "line", false) ? "true" : "false") << ")"
		    << ".SetHeaderBand(" << ((bool)ctx.Property(n, "header_band", false) ? "true" : "false") << ")"
		    << ".SetLineThickness(DPI(" << (int)ctx.Property(n, "line_thickness", 1) << "))"
		    << ".SetInset(Rect(DPI(" << (int)ctx.Property(n, "inset", 8) << "), DPI(" << (int)ctx.Property(n, "inset", 8) << "), DPI(" << (int)ctx.Property(n, "inset", 8) << "), DPI(" << (int)ctx.Property(n, "inset", 8) << ")))"
		    << ".SetHeaderInset(Rect(DPI(" << (int)ctx.Property(n, "header_inset", 6) << "), DPI(" << (int)ctx.Property(n, "header_inset", 6) << "), DPI(" << (int)ctx.Property(n, "header_inset", 6) << "), DPI(" << (int)ctx.Property(n, "header_inset", 6) << ")))";
		String icon = ctx.IconExpr(ctx.Property(n, "icon", "None"));
		if(!icon.IsEmpty())
			out << "\t\t" << var << ".SetIcon(" << icon << ").SetIconSize(DPI(" << (int)ctx.Property(n, "icon_size", 16) << "));\n";
	};
	t.codegen.emit_child = EmitDesignerLayoutChild;
	SetDesignerAdapterFactory<DesignerGroupPanelAdapter>(t);
	SetDesignerThemeSchema(t,
		{"theme_override", "face_enabled", "face", "face_mode", "face_quad",
		 "frame_enabled", "frame", "frame_style", "frame_width", "radius",
		 "shadow_enabled", "shadow_distance", "shadow_offset_x", "shadow_offset_y",
		 "shadow_alpha", "shadow_color", "shadow_curve",
		 "line", "header_band", "header_mode", "placement", "icon", "icon_size",
		 "inset", "header_inset", "line_thickness"},
		{},
		{{"title, subtitle, and side title are content properties, not theme export fields", "Header text is instance content."}});
	t.init_defaults = [](DesignerNode& n) {
		n.properties.Set("text", "Group");
		n.properties.Set("subtitle", "");
		n.properties.Set("side_title", "");
		n.properties.Set("header_mode", "Inside");
		n.properties.Set("line", false);
		n.properties.Set("header_band", false);
		n.properties.Set("role", "Standard");
		n.properties.Set("placement", "Top");
		n.properties.Set("icon", "None");
		n.properties.Set("icon_size", 16);
		n.properties.Set("inset", 8);
		n.properties.Set("header_inset", 6);
		n.properties.Set("line_thickness", 1);
		n.properties.Set("h_sizing", "Expand");
		n.properties.Set("v_sizing", "Expand");
		n.properties.Set("fixed_width", 260);
		n.properties.Set("fixed_height", 160);
		n.properties.Set("width", 260);
		n.properties.Set("height", 160);
		n.properties.Set("face", DesignerPanelFace());
		n.properties.Set("frame", DesignerPanelFrame());
		n.properties.Set("radius", 8);
		n.properties.Set("face_enabled", true);
		n.properties.Set("frame_enabled", true);
	};
	return t;
}

DesignerType MakeAccordionType()
{
	DesignerType t = MakeControlType("UiAccordion", "Accordion", Size(300, 220));
	t.toolbox_group = "Containers";
	t.is_container = true;
	t.can_have_children = true;
	t.icon = ICON_DESIGN_EXPANSION_PANELS_48();
	t.capabilities.is_container = true;
	t.capabilities.can_have_children = true;
	t.capabilities.supports_children = true;
	t.capabilities.requires_default_child_slots = true;
	t.default_child_slots = DesignerDefaultChildSlotSet::AccordionThreeSections;
	t.child_emission = DesignerLayoutChildEmissionStrategy::AccordionSection;
	t.codegen.route = DesignerCodeGenRoute::StructuralCentral;
	t.codegen.emit_setup = [](DesignerCodeGenContext& ctx, const DesignerNode& n) {
		ctx.Out() << "\t\t" << ctx.Var(n) << ".SetSingleOpen(" << ((bool)ctx.Property(n, "single_open", false) ? "true" : "false")
		          << ").SetEnforceOne(" << ((bool)ctx.Property(n, "enforce_one", false) ? "true" : "false")
		          << ").ShowChevron(" << ((bool)ctx.Property(n, "show_chevron", true) ? "true" : "false")
		          << ").SetChevronSide(" << ctx.AlignSideExpr(ctx.Property(n, "chevron_side", "Right"), "Right") << ")"
		          << ".SetAnimation(" << ((bool)ctx.Property(n, "animation", true) ? "true" : "false") << ", "
		          << (int)ctx.Property(n, "open_ms", 120) << ", "
		          << (int)ctx.Property(n, "close_ms", 0) << ")"
		          << ".ShowDragHandle(" << ((bool)ctx.Property(n, "show_drag_handle", false) ? "true" : "false")
		          << ").EnableDragReorder(" << ((bool)ctx.Property(n, "drag_reorder", false) ? "true" : "false") << ");\n";
	};
	t.codegen.emit_child = EmitDesignerLayoutChild;
	SetDesignerAdapterFactory<DesignerAccordionAdapter>(t);
	SetDesignerThemeSchema(t,
		{"theme_override", "face_enabled", "face", "face_mode", "face_quad",
		 "frame_enabled", "frame", "frame_style", "frame_width", "radius",
		 "shadow_enabled", "shadow_distance", "shadow_offset_x", "shadow_offset_y",
		 "shadow_alpha", "shadow_color", "shadow_curve",
		 "header_face_enabled", "header_face", "header_frame_enabled", "header_frame",
		 "header_radius", "header_title", "header_subtitle",
		 "body_face_enabled", "body_face", "body_frame_enabled", "body_frame",
		 "body_radius", "chevron_side", "show_chevron"},
		{},
		{{"section title, body height, and open state are runtime content/state", "Accordion sections are model content."}});
	t.default_size = Size(300, 220);
	t.min_size = Size(120, 80);
	t.init_defaults = [](DesignerNode& n) {
		n.properties.Set("text", "Accordion");
		n.properties.Set("role", "Standard");
		n.properties.Set("h_sizing", "Expand");
		n.properties.Set("v_sizing", "Expand");
		n.properties.Set("fixed_width", 300);
		n.properties.Set("fixed_height", 220);
		n.properties.Set("width", 300);
		n.properties.Set("height", 220);
		n.properties.Set("single_open", false);
		n.properties.Set("enforce_one", false);
		n.properties.Set("show_chevron", true);
		n.properties.Set("chevron_side", "Right");
		n.properties.Set("animation", true);
		n.properties.Set("open_ms", 120);
		n.properties.Set("close_ms", 0);
		n.properties.Set("item_spacing", 8);
		n.properties.Set("header_body_gap", 4);
		n.properties.Set("body_min_height", 88);
		n.properties.Set("show_drag_handle", false);
		n.properties.Set("drag_reorder", false);
		n.properties.Set("face_enabled", false);
		n.properties.Set("frame_enabled", false);
		n.properties.Set("radius", 0);
		n.properties.Set("header_face_enabled", false);
		n.properties.Set("header_frame_enabled", false);
		n.properties.Set("header_radius", 8);
		n.properties.Set("header_face", Color(248, 250, 252));
		n.properties.Set("header_frame", Color(203, 213, 225));
		n.properties.Set("header_title", Color(0, 120, 212));
		n.properties.Set("header_subtitle", Color(100, 116, 139));
		n.properties.Set("body_face_enabled", false);
		n.properties.Set("body_frame_enabled", false);
		n.properties.Set("body_radius", 0);
		n.properties.Set("body_face", Color(255, 255, 255));
		n.properties.Set("body_frame", Color(226, 232, 240));
	};
	return t;
}

}
