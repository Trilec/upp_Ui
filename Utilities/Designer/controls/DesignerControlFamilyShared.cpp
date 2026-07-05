#include "DesignerControlFamilyShared.h"

namespace Upp {

Color DesignerLayoutFace()  { return Color(255, 224, 178); }
Color DesignerLayoutFrame() { return Color(217, 119, 6); }
Color DesignerDebugRed()    { return Color(220, 38, 38); }
Color DesignerPanelFace()   { return Color(187, 232, 203); }
Color DesignerPanelFrame()  { return Color(34, 150, 91); }
Color DesignerControlFace() { return Color(203, 224, 255); }
Color DesignerControlFrame(){ return Color(54, 116, 210); }

Image MakeDesignerTypeIcon(const String& id)
{
	if(id == "BoxLayout")
		return ICON_DESIGN_BOX_LAYOUT_48();
	if(id == "GridLayout")
		return ICON_DESIGN_GRID_4X4_48();
	if(id == "UiSplitter")
		return ICON_DESIGN_BORDER_HORIZONTAL_48();
	if(id == "UiQuadSplitter")
		return ICON_DESIGN_BORDER_INNER_48();
	if(id == "Spacer")
		return ICON_DESIGN_FIT_WIDTH_48();
	if(id == "UiPanel")
		return ICON_DESIGN_PANEL_48();
	if(id == "UiGroupPanel")
		return ICON_DESIGN_BORDER_OUTER_48();
	if(id == "UiAccordion" || id == "AccordionSectionSlot")
		return ICON_DESIGN_EXPANSION_PANELS_48();
	if(id == "UiScrollPanel")
		return ICON_DESIGN_EXPANSION_PANELS_48();
	if(id == "UiLabel")
		return ICON_DESIGN_LABEL_48();
	if(id == "UiTitleCard")
		return ICON_DESIGN_ID_CARD_48();
	if(id == "UiButton" || id == "UiToolButton" || id == "UiSplitButton")
		return ICON_DESIGN_BUTTON_48();
	if(id == "UiLineEdit")
		return ICON_DESIGN_EDIT_TEXT_48();
	if(id == "UiIntEdit")
		return ICON_DESIGN_EDIT_INT_48();
	if(id == "UiFloatEdit")
		return ICON_DESIGN_EDIT_FLOAT_48();
	if(id == "UiSlider")
		return ICON_DESIGN_SLIDERS_48();
	if(id == "UiToggle")
		return ICON_DESIGN_TOGGLE_COMPOSITE_48();
	if(id == "UiCompositeColor")
		return ICON_DESIGN_FORMAT_PAINT_48();
	if(id == "UiDropdown")
		return ICON_DESIGN_LIST_ALT_48();
	if(id == "UiCheckBox")
		return ICON_DESIGN_CHECK_SMALL_48();
	if(id == "UiBreadcrumbs")
		return ICON_DESIGN_BREADCRUMBS_48();
	if(id == "UiTab")
		return ICON_DESIGN_TAB_48();
	if(id == "UiStack")
		return ICON_DESIGN_STACK_48();
	if(id == "UiTable")
		return ICON_DESIGN_TABLE_48();
	if(id == "UiTree")
		return ICON_DESIGN_TREE_48();
	ImageBuffer ib(16, 16);
	RGBA clear;
	clear.r = clear.g = clear.b = clear.a = 0;
	RGBA blue;
	blue.r = 0; blue.g = 102; blue.b = 204; blue.a = 255;
	RGBA green;
	green.r = 27; green.g = 145; green.b = 72; green.a = 255;
	RGBA gray;
	gray.r = 102; gray.g = 112; gray.b = 128; gray.a = 255;
	Fill(~ib, clear, ib.GetLength());
	auto dot = [&](int x, int y, RGBA c) {
		if(x >= 0 && x < 16 && y >= 0 && y < 16)
			ib[y][x] = c;
	};
	auto rect = [&](int l, int t, int r, int b, RGBA c) {
		for(int y = t; y < b; y++)
			for(int x = l; x < r; x++)
				dot(x, y, c);
	};
	auto frame = [&](int l, int t, int r, int b, RGBA c) {
		rect(l, t, r, t + 2, c);
		rect(l, b - 2, r, b, c);
		rect(l, t, l + 2, b, c);
		rect(r - 2, t, r, b, c);
	};
	auto circle = [&](int cx, int cy, int rr, RGBA c) {
		for(int y = cy - rr; y <= cy + rr; y++)
			for(int x = cx - rr; x <= cx + rr; x++)
				if((x - cx) * (x - cx) + (y - cy) * (y - cy) <= rr * rr)
					dot(x, y, c);
	};

	if(id == "BoxLayout") {
		frame(2, 2, 14, 14, blue);
		rect(5, 5, 11, 7, blue);
		rect(5, 9, 11, 11, blue);
	}
	else if(id == "GridLayout") {
		frame(2, 2, 14, 14, blue);
		rect(7, 3, 9, 13, blue);
		rect(3, 7, 13, 9, blue);
	}
	else if(id == "UiSplitter") {
		frame(2, 3, 14, 13, blue);
		rect(7, 3, 9, 13, blue);
		rect(5, 7, 11, 9, blue);
	}
	else if(id == "UiButton" || id == "UiToolButton" || id == "UiSplitButton") {
		frame(2, 5, 14, 12, green);
		rect(5, 8, 11, 9, green);
		if(id == "UiSplitButton") {
			rect(11, 5, 12, 12, green);
			dot(13, 7, green);
			dot(14, 8, green);
			dot(13, 9, green);
		}
	}
	else if(id == "UiLineEdit") {
		frame(2, 4, 14, 12, green);
		rect(4, 10, 12, 11, green);
	}
	else if(id == "UiToggle") {
		frame(2, 5, 14, 12, green);
		circle(10, 8, 3, green);
	}
	else if(id == "UiDropdown") {
		frame(2, 4, 14, 12, green);
		rect(10, 7, 12, 9, green);
		dot(9, 6, green);
		dot(12, 6, green);
	}
	else if(id == "UiSlider") {
		rect(2, 8, 14, 10, green);
		circle(9, 9, 3, green);
	}
	else if(id == "UiPanel" || id == "UiScrollPanel" || id == "UiAccordion") {
		frame(2, 3, 14, 13, green);
		if(id == "UiScrollPanel")
			rect(11, 4, 13, 12, green);
	}
	else if(id == "UiTitleCard") {
		frame(2, 3, 14, 13, green);
		rect(4, 5, 12, 7, green);
		rect(4, 9, 10, 10, green);
	}
	else if(id == "UiLabel") {
		rect(3, 5, 13, 7, green);
		rect(3, 9, 10, 11, green);
	}
	else if(id == "UiIntEdit" || id == "UiFloatEdit") {
		frame(2, 4, 14, 12, green);
		rect(4, 7, 12, 9, green);
		if(id == "UiFloatEdit")
			dot(8, 10, green);
	}
	else
		circle(8, 8, 5, gray);
	return ib;
}

DesignerType MakeControlType(const String& id, const String& name, Size size)
{
	DesignerType t;
	t.id = id;
	t.display_name = name;
	t.default_base_name = name;
	t.toolbox_group = "Controls";
	t.runtime_cpp_type = id;
	t.icon = Null;
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
		bool placeholder = id == "Item";
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
		n.properties.Set("face_enabled", placeholder);
		n.properties.Set("frame_enabled", placeholder);
		n.properties.Set("font", "Sans");
		n.properties.Set("font_size", 11);
		n.properties.Set("align", "Left");
		n.properties.Set("align_h", "Left");
		n.properties.Set("align_v", "Center");
		n.properties.Set("icon", "None");
		n.properties.Set("icon_size", 18);
		if(id == "UiButton" || id == "UiToolButton" || id == "UiSplitButton")
			n.properties.Set("align", "Center");
		if(id == "UiButton" || id == "UiToolButton" || id == "UiSplitButton") {
			n.properties.Set("align_h", "Center");
			n.properties.Set("align_v", "Center");
		}
		if(id == "UiButton") {
			n.properties.Set("content_inset", 6);
			n.properties.Set("content_gap", 4);
			n.properties.Set("ink_enabled", false);
			n.properties.Set("icon_ink_enabled", false);
		}
		if(id == "UiToolButton") {
			n.properties.Set("content_inset", 4);
			n.properties.Set("content_gap", 4);
			n.properties.Set("ink_enabled", false);
			n.properties.Set("icon_ink_enabled", false);
		}
		if(id == "UiSplitButton") {
			n.properties.Set("text", "Save");
			n.properties.Set("icon_size", 16);
			n.properties.Set("content_inset", 6);
			n.properties.Set("content_gap", 4);
			n.properties.Set("split_width", 30);
			n.properties.Set("split_content_gap", 4);
			n.properties.Set("split_icon_size", 16);
			n.properties.Set("popup_min_width", 220);
			n.properties.Set("ink_enabled", false);
			n.properties.Set("icon_ink_enabled", false);
			n.properties.Set("choice_a", "Recent A");
			n.properties.Set("choice_b", "Recent B");
			n.properties.Set("choice_c", "Recent C");
		}
		if(id == "UiTitleCard") {
			n.properties.Set("content_inset", 8);
			n.properties.Set("media_gap", 10);
			n.properties.Set("media_reserve", 24);
			n.properties.Set("media_min", 16);
			n.properties.Set("media_auto_fit", false);
			n.properties.Set("media_side", "Left");
			n.properties.Set("media_align_h", "Center");
			n.properties.Set("media_align_v", "Center");
		}
		if(id == "UiToolButton") {
			n.properties.Set("text", "");
			n.properties.Set("icon", "Settings");
			n.properties.Set("icon_size", 20);
		}
		if(id == "UiLineEdit")
			n.properties.Set("placeholder", "Placeholder");
		if(id == "UiToggle")
			n.properties.Set("on", true);
		if(id == "UiSlider") {
			n.properties.Set("track_width", 120);
			n.properties.Set("track_height", 3);
			n.properties.Set("thumb_width", 20);
			n.properties.Set("thumb_height", 20);
			n.properties.Set("track_radius", 8);
			n.properties.Set("thumb_radius", 8);
			n.properties.Set("value", 50);
		}
		if(id == "UiDropdown")
			n.properties.Set("item_text", "First");
		if(id == "UiDropdown")
			n.properties.Set("selected_item", "First");
		if(id == "UiDropdown")
			n.properties.Set("indicator_side", "Right");
		if(id == "UiDropdown")
			n.properties.Set("indicator_closed_icon", "None");
		if(id == "UiDropdown")
			n.properties.Set("indicator_opened_icon", "None");
		if(id == "UiDropdown")
			n.properties.Set("indicator_size", 14);
		if(id == "UiCheckBox") {
			n.properties.Set("state", "Checked");
			n.properties.Set("tri_state", false);
			n.properties.Set("visual", "Classic");
		}
		if(id == "UiBreadcrumbs") {
			n.properties.Set("crumb_a", "Home");
			n.properties.Set("crumb_b", "Library");
			n.properties.Set("crumb_c", "Current");
			n.properties.Set("current", 2);
			n.properties.Set("trim", false);
			n.properties.Set("divider", "/");
			n.properties.Set("icon", "ICON_DESIGN_HOME_48");
			n.properties.Set("icon_size", 16);
		}
		if(id == "UiTab") {
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
			n.properties.Set("active", 0);
			n.properties.Set("tab_a", "TabA");
			n.properties.Set("tab_b", "TabB");
			n.properties.Set("tab_c", "TabC");
		}
		if(id == "UiStack")
			n.properties.Set("active", 0);
		if(id == "UiTable") {
			n.properties.Set("rows_count", 4);
			n.properties.Set("cols_count", 3);
			n.properties.Set("row_headers", true);
			n.properties.Set("column_headers", true);
			n.properties.Set("row_height", 28);
			n.properties.Set("header_height", 30);
			n.properties.Set("column_width", 120);
		}
		if(id == "UiTree") {
			n.properties.Set("root_visible", false);
			n.properties.Set("connectors", true);
			n.properties.Set("metadata", false);
		}
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
		n.properties.Set("show_value", id == "UiCompositeSlider");
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
		if(id == "UiCompositeColor") {
			n.properties.Set("label", "Color");
			n.properties.Set("value_text", "#0078D4");
			n.properties.Set("show_value", true);
			n.properties.Set("color_count", 4);
			n.properties.Set("color_1", Color(0, 120, 212));
			n.properties.Set("color_2", Color(226, 141, 0));
			n.properties.Set("color_3", Color(52, 199, 89));
			n.properties.Set("color_4", Color(0, 0, 0));
			n.properties.Set("color_label_1", "Accent");
			n.properties.Set("color_label_2", "Warning");
			n.properties.Set("color_label_3", "Success");
			n.properties.Set("color_label_4", "Ink");
			n.properties.Set("separator_2", false);
			n.properties.Set("separator_3", false);
			n.properties.Set("separator_4", false);
			n.properties.Set("layout_mode", "Inline");
			n.properties.Set("label_width", 112);
			n.properties.Set("value_width", 76);
			n.properties.Set("field_gap", 8);
			n.properties.Set("stack_gap", 4);
			n.properties.Set("h_sizing", "Fit");
			n.properties.Set("v_sizing", "Fit");
			n.properties.Set("fixed_width", 260);
			n.properties.Set("fixed_height", 32);
			n.properties.Set("width", 260);
			n.properties.Set("height", 32);
		}
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
	t.icon = id == "UiTab" ? ICON_DESIGN_TAB_48() : ICON_DESIGN_STACK_48();
	t.capabilities.is_container = true;
	t.capabilities.is_page_container = true;
	t.capabilities.can_have_children = true;
	t.capabilities.supports_children = true;
	t.capabilities.requires_default_child_slots = true;
	t.default_child_slots = DesignerDefaultChildSlotSet::PageContainerThreePages;
	t.child_emission = DesignerLayoutChildEmissionStrategy::PageContainerPage;
	if(id == "UiTab")
		SetDesignerAdapterFactory<DesignerTabAdapter>(t);
	else
		SetDesignerAdapterFactory<DesignerStackAdapter>(t);
	if(id == "UiTab") {
		SetDesignerThemeSchema(t,
			{"theme_override", "face_enabled", "face", "face_mode", "face_quad",
			 "frame_enabled", "frame", "frame_style", "frame_width", "radius",
			 "shadow_enabled", "shadow_distance", "shadow_offset_x", "shadow_offset_y",
			 "shadow_alpha", "shadow_color", "shadow_curve",
			 "visual", "placement", "expand_tabs", "close_buttons", "drag_handles",
			 "tab_font", "tab_font_size", "tab_icon_size", "tab_icon_side",
			 "content_gap", "affordance_gap"},
			{},
			{{"style helpers are emitted from theme/state, not as a separate instance-only schema", "The page container already owns the tab strip styling contract."}});
	}
	else {
		SetDesignerThemeSchema(t,
			{"theme_override", "face_enabled", "face", "face_mode", "face_quad",
			 "frame_enabled", "frame", "frame_style", "frame_width", "radius",
			 "shadow_enabled", "shadow_distance", "shadow_offset_x", "shadow_offset_y",
			 "shadow_alpha", "shadow_color", "shadow_curve"},
			{},
			{{"tab-specific fields are not relevant to UiStack", "UiStack only exports page activation and page chrome styling."}});
	}
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
		if(id == "UiTab") {
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
		}
	};
	return t;
}

DesignerType MakePanelControlType(const String& id, const String& name, Size size)
{
	DesignerType t = MakeControlType(id, name, size);
	t.toolbox_group = "Containers";
	t.is_container = true;
	t.can_have_children = true;
	t.icon = id == "UiScrollPanel" ? ICON_DESIGN_EXPANSION_PANELS_48() : ICON_DESIGN_PANEL_48();
	t.capabilities.is_container = true;
	t.capabilities.can_have_children = true;
	t.capabilities.supports_children = true;
	t.child_emission = id == "UiScrollPanel"
	    ? DesignerLayoutChildEmissionStrategy::ScrollContent
	    : DesignerLayoutChildEmissionStrategy::PanelContent;
	if(id == "UiScrollPanel")
		SetDesignerAdapterFactory<DesignerScrollPanelAdapter>(t);
	else
		SetDesignerAdapterFactory<DesignerPanelAdapter>(t);
	SetDesignerThemeSchema(t,
		{"theme_override", "face_enabled", "face", "face_mode", "face_quad",
		 "frame_enabled", "frame", "frame_style", "frame_width", "radius",
		 "shadow_enabled", "shadow_distance", "shadow_offset_x", "shadow_offset_y",
		 "shadow_alpha", "shadow_color", "shadow_curve"},
		{},
		{{"scroll mode is behavior, not theme", "UiScrollPanel::SetScrollMode is runtime behavior."}});
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
		if(id == "UiScrollPanel")
			n.properties.Set("scroll_mode", "Auto");
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
