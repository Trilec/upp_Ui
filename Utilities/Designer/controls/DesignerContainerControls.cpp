#include "DesignerControlFamilies.h"
#include "DesignerControlFamilyShared.h"

namespace Upp {

static DesignerType MakeAccordionSectionSlotType()
{
	DesignerType t;
	t.id = "AccordionSectionSlot";
	t.display_name = "Accordion Section";
	t.default_base_name = "accordionSection";
	t.runtime_cpp_type = "ParentCtrl";
	t.icon = ICON_DESIGN_EXPANSION_PANELS_48();
	t.is_container = true;
	t.can_have_children = true;
	t.capabilities.is_container = true;
	t.capabilities.is_slot_node = true;
	t.capabilities.can_have_children = true;
	t.capabilities.supports_children = true;
	t.capabilities.is_headless_node = true;
	t.capabilities.supports_appearance_overrides = false;
	t.capabilities.supports_theme_export = false;
	t.child_emission = DesignerLayoutChildEmissionStrategy::SlotPassthrough;
	t.default_size = Size(240, 120);
	t.min_size = Size(40, 30);
	SetDesignerThemeSchema(t,
		{"theme_override", "face_enabled", "face", "face_mode", "face_quad",
		 "frame_enabled", "frame", "frame_style", "frame_width", "radius",
		 "shadow_enabled", "shadow_distance", "shadow_offset_x", "shadow_offset_y",
		 "shadow_alpha", "shadow_color", "shadow_curve"},
		{},
		{{"section_title, section_subtitle, open, lock, and body_height are slot content/state", "Accordion section slot is structural, not a theme surface."}});
	t.init_defaults = [](DesignerNode& n) {
		n.properties.Set("section_title", "Section");
		n.properties.Set("section_subtitle", "");
		n.properties.Set("open", true);
		n.properties.Set("lock", "None");
		n.properties.Set("body_height", -1);
		n.properties.Set("h_sizing", "Expand");
		n.properties.Set("v_sizing", "Expand");
		n.properties.Set("fixed_width", 240);
		n.properties.Set("fixed_height", 120);
		n.properties.Set("width", 240);
		n.properties.Set("height", 120);
		n.properties.Set("face_enabled", false);
		n.properties.Set("frame_enabled", false);
		n.properties.Set("radius", 0);
	};
	return t;
}

static DesignerType MakePageSlotType()
{
	DesignerType t;
	t.id = "PageSlot";
	t.display_name = "Page Slot";
	t.default_base_name = "pageSlot";
	t.runtime_cpp_type = "ParentCtrl";
	t.icon = ICON_DESIGN_STACK_48();
	t.is_container = true;
	t.can_have_children = true;
	t.capabilities.is_container = true;
	t.capabilities.is_slot_node = true;
	t.capabilities.can_have_children = true;
	t.capabilities.supports_children = true;
	t.capabilities.is_headless_node = true;
	t.capabilities.supports_appearance_overrides = false;
	t.capabilities.supports_theme_export = false;
	t.child_emission = DesignerLayoutChildEmissionStrategy::SlotPassthrough;
	t.default_size = Size(220, 140);
	t.min_size = Size(40, 30);
	SetDesignerThemeSchema(t,
		{"theme_override", "face_enabled", "face", "face_mode", "face_quad",
		 "frame_enabled", "frame", "frame_style", "frame_width", "radius",
		 "shadow_enabled", "shadow_distance", "shadow_offset_x", "shadow_offset_y",
		 "shadow_alpha", "shadow_color", "shadow_curve"},
		{},
		{{"page_title, show_title, and icon are page content/state", "Page slot is structural, not a theme surface."}});
	t.init_defaults = [](DesignerNode& n) {
		n.properties.Set("page_title", "Page");
		n.properties.Set("show_title", true);
		n.properties.Set("icon", "None");
		n.properties.Set("icon_size", 16);
		n.properties.Set("h_sizing", "Expand");
		n.properties.Set("v_sizing", "Expand");
		n.properties.Set("fixed_width", 220);
		n.properties.Set("fixed_height", 140);
		n.properties.Set("width", 220);
		n.properties.Set("height", 140);
		n.properties.Set("face_enabled", false);
		n.properties.Set("frame_enabled", false);
		n.properties.Set("radius", 0);
	};
	return t;
}

void RegisterDesignerContainerControls(DesignerRegistry& registry)
{
	registry.Register(MakePageSlotType());
	registry.Register(MakeAccordionSectionSlotType());
	registry.Register(MakePanelControlType("UiPanel", "Panel", Size(240, 140)));
	registry.Register(MakeGroupPanelType());
	registry.Register(MakePanelControlType("UiScrollPanel", "Scroll Panel", Size(260, 160)));
	registry.Register(MakeAccordionType());
	registry.Register(MakePageContainerType("UiTab", "Tab", Size(300, 180)));
	registry.Register(MakePageContainerType("UiStack", "Stack", Size(300, 180)));
}

}
