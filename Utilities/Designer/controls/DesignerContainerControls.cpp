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
	DesignerType panel = MakePanelControlType("UiPanel", "Panel", Size(240, 140));
	panel.icon = ICON_DESIGN_PANEL_48();
	SetDesignerAdapterFactory<DesignerPanelAdapter>(panel);
	{
		auto common_init = panel.init_defaults;
		panel.init_defaults = [=](DesignerNode& n) {
			if(common_init)
				common_init(n);
			n.properties.Set("face", DesignerPanelFace());
			n.properties.Set("frame", DesignerPanelFrame());
			n.properties.Set("radius", 8);
			n.properties.Set("face_enabled", false);
			n.properties.Set("frame_enabled", false);
		};
	}
	SetDesignerThemeSchema(panel,
		{"theme_override", "face_enabled", "face", "face_mode", "face_quad",
		 "frame_enabled", "frame", "frame_style", "frame_width", "radius",
		 "shadow_enabled", "shadow_distance", "shadow_offset_x", "shadow_offset_y",
		 "shadow_alpha", "shadow_color", "shadow_curve"},
		{},
		{{"panel content is model content, not theme export state", "UiPanel theme export stays on the surface chrome contract."}});
	registry.Register(panel);
	registry.Register(MakeGroupPanelType());
	DesignerType scroll = MakePanelControlType("UiScrollPanel", "Scroll Panel", Size(260, 160));
	scroll.icon = ICON_DESIGN_EXPANSION_PANELS_48();
	SetDesignerAdapterFactory<DesignerScrollPanelAdapter>(scroll);
	{
		auto common_init = scroll.init_defaults;
		scroll.init_defaults = [=](DesignerNode& n) {
			if(common_init)
				common_init(n);
			n.properties.Set("scroll_mode", "Auto");
		};
	}
	SetDesignerThemeSchema(scroll,
		{"theme_override", "face_enabled", "face", "face_mode", "face_quad",
		 "frame_enabled", "frame", "frame_style", "frame_width", "radius",
		 "shadow_enabled", "shadow_distance", "shadow_offset_x", "shadow_offset_y",
		 "shadow_alpha", "shadow_color", "shadow_curve"},
		{},
		{{"scroll mode is behavior, not theme", "UiScrollPanel::SetScrollMode is runtime behavior."}});
	registry.Register(scroll);
	registry.Register(MakeAccordionType());
	DesignerType tab = MakePageContainerType("UiTab", "Tab", Size(300, 180));
	tab.icon = ICON_DESIGN_TAB_48();
	SetDesignerAdapterFactory<DesignerTabAdapter>(tab);
	SetDesignerThemeSchema(tab,
		{"theme_override", "face_enabled", "face", "face_mode", "face_quad",
		 "frame_enabled", "frame", "frame_style", "frame_width", "radius",
		 "shadow_enabled", "shadow_distance", "shadow_offset_x", "shadow_offset_y",
		 "shadow_alpha", "shadow_color", "shadow_curve",
		 "visual", "placement", "expand_tabs", "close_buttons", "drag_handles",
		 "tab_font", "tab_font_size", "tab_icon_size", "tab_icon_side",
		 "content_gap", "affordance_gap"},
		{},
		{{"style helpers are emitted from theme/state, not as a separate instance-only schema", "The page container already owns the tab strip styling contract."}});
	{
		auto common_init = tab.init_defaults;
		tab.init_defaults = [=](DesignerNode& n) {
			if(common_init)
				common_init(n);
			n.properties.Set("visual", "Underline");
			n.properties.Set("placement", "Top");
			n.properties.Set("expand_tabs", false);
			n.properties.Set("close_buttons", false);
			n.properties.Set("drag_handles", false);
			n.properties.Set("tab_font", "Sans");
			n.properties.Set("tab_font_size", 11);
			n.properties.Set("tab_icon_size", 16);
			n.properties.Set("tab_icon_side", "Left");
			n.properties.Set("content_gap", 6);
			n.properties.Set("affordance_gap", 4);
		};
	}
	registry.Register(tab);
	DesignerType stack = MakePageContainerType("UiStack", "Stack", Size(300, 180));
	stack.icon = ICON_DESIGN_STACK_48();
	SetDesignerAdapterFactory<DesignerStackAdapter>(stack);
	SetDesignerThemeSchema(stack,
		{"theme_override", "face_enabled", "face", "face_mode", "face_quad",
		 "frame_enabled", "frame", "frame_style", "frame_width", "radius",
		 "shadow_enabled", "shadow_distance", "shadow_offset_x", "shadow_offset_y",
		 "shadow_alpha", "shadow_color", "shadow_curve"},
		{},
		{{"tab-specific fields are not relevant to UiStack", "UiStack only exports page activation and page chrome styling."}});
	registry.Register(stack);
}

}
