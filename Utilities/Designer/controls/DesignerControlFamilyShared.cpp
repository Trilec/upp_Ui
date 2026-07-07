#include "DesignerControlFamilyShared.h"

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
