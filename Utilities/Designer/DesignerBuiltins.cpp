#include "DesignerBuiltins.h"

// DesignerBuiltins.cpp - stock toolbox/control catalog for the designer.
// Add new built-in controls here by registering a DesignerType, default model
// properties, and an adapter implementation in DesignerAdapter.

namespace Upp {

static Color DesignerLayoutFace()  { return Color(255, 224, 178); }
static Color DesignerLayoutFrame() { return Color(217, 119, 6); }
static Color DesignerPanelFace()   { return Color(187, 232, 203); }
static Color DesignerPanelFrame()  { return Color(34, 150, 91); }
static Color DesignerControlFace() { return Color(203, 224, 255); }
static Color DesignerControlFrame(){ return Color(54, 116, 210); }

static Image MakeDesignerTypeIcon(const String& id)
{
	if(id == "BoxLayout")
		return ICON_DESIGN_BOX_LAYOUT_48();
	if(id == "GridLayout")
		return ICON_DESIGN_GRID_LAYOUT_48();
	if(id == "UiSplitter")
		return ICON_DESIGN_BORDER_HORIZONTAL_48();
	if(id == "UiQuadSplitter")
		return ICON_DESIGN_BORDER_INNER_48();
	if(id == "UiPanel")
		return ICON_DESIGN_PANEL_48();
	if(id == "UiScrollPanel")
		return ICON_DESIGN_SCROLL_PANEL_48();
	if(id == "UiLabel")
		return ICON_DESIGN_LABEL_48();
	if(id == "UiTitleCard")
		return ICON_DESIGN_ID_CARD_48();
	if(id == "UiButton")
		return ICON_DESIGN_BUTTON_48();
	if(id == "UiLineEdit")
		return ICON_DESIGN_EDIT_TEXT_48();
	if(id == "UiIntEdit")
		return ICON_DESIGN_EDIT_INT_48();
	if(id == "UiFloatEdit")
		return ICON_DESIGN_EDIT_FLOAT_48();
	if(id == "UiSlider")
		return ICON_DESIGN_TUNE_48();
	if(id == "UiToggle")
		return ICON_DESIGN_TOGGLE_COMPOSITE_48();
	if(id == "UiDropdown")
		return ICON_DESIGN_LIST_ALT_48();
	if(id == "UiCheckBox")
		return ICON_DESIGN_CHECK_SMALL_48();
	if(id == "UiBreadcrumbs")
		return ICON_DESIGN_BREADCRUMBS_48();
	if(id == "UiTab")
		return ICON_DESIGN_TAB_48();
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
	else if(id == "UiButton") {
		frame(2, 5, 14, 12, green);
		rect(5, 8, 11, 9, green);
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
	else if(id == "UiPanel" || id == "UiScrollPanel") {
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

static DesignerType MakeBoxLayoutType()
{
	DesignerType t;
	t.id = "BoxLayout";
	t.display_name = "Box Layout";
	t.toolbox_group = "Layouts";
	t.icon = MakeDesignerTypeIcon(t.id);
	t.is_container = true;
	t.can_have_children = true;
	t.default_size = Size(260, 160);
	t.min_size = Size(80, 50);
	t.init_defaults = [](DesignerNode& n) {
		n.properties.Set("direction", "V");
		n.properties.Set("wrap", false);
		n.properties.Set("gap", 8);
		n.properties.Set("inset", 8);
		n.properties.Set("debug", false);
		n.properties.Set("sizing", "Expand");
		n.properties.Set("h_sizing", "Expand");
		n.properties.Set("v_sizing", "Expand");
		n.properties.Set("face", DesignerLayoutFace());
		n.properties.Set("frame", DesignerLayoutFrame());
		n.properties.Set("radius", 0);
	};
	return t;
}

static DesignerType MakeGridLayoutType()
{
	DesignerType t;
	t.id = "GridLayout";
	t.display_name = "Grid Layout";
	t.toolbox_group = "Layouts";
	t.icon = MakeDesignerTypeIcon(t.id);
	t.is_container = true;
	t.can_have_children = true;
	t.default_size = Size(280, 180);
	t.min_size = Size(100, 60);
	t.init_defaults = [](DesignerNode& n) {
		n.properties.Set("cell_width", 120);
		n.properties.Set("cell_height", 96);
		n.properties.Set("rows", 2);
		n.properties.Set("columns", 2);
		n.properties.Set("gap", 8);
		n.properties.Set("inset", 8);
		n.properties.Set("debug", false);
		n.properties.Set("sizing", "Expand");
		n.properties.Set("h_sizing", "Expand");
		n.properties.Set("v_sizing", "Expand");
		n.properties.Set("face", DesignerLayoutFace());
		n.properties.Set("frame", DesignerLayoutFrame());
		n.properties.Set("radius", 0);
	};
	return t;
}

static DesignerType MakeSplitterType()
{
	DesignerType t;
	t.id = "UiSplitter";
	t.display_name = "Splitter";
	t.toolbox_group = "Layouts";
	t.icon = MakeDesignerTypeIcon(t.id);
	t.is_container = true;
	t.can_have_children = true;
	t.default_size = Size(320, 180);
	t.min_size = Size(100, 60);
	t.init_defaults = [](DesignerNode& n) {
		n.properties.Set("direction", "H");
		n.properties.Set("split_percent", 50);
		n.properties.Set("min_a", 80);
		n.properties.Set("min_b", 80);
		n.properties.Set("hit_width", 14);
		n.properties.Set("track_thickness", 2);
		n.properties.Set("track_inset", 0);
		n.properties.Set("thumb_width", 14);
		n.properties.Set("thumb_height", 64);
		n.properties.Set("thumb_radius", 8);
		n.properties.Set("debug", false);
		n.properties.Set("sizing", "Expand");
		n.properties.Set("h_sizing", "Expand");
		n.properties.Set("v_sizing", "Expand");
		n.properties.Set("face", DesignerLayoutFace());
		n.properties.Set("frame", DesignerLayoutFrame());
		n.properties.Set("face_enabled", false);
		n.properties.Set("frame_enabled", false);
	};
	return t;
}

static DesignerType MakeQuadSplitterType()
{
	DesignerType t;
	t.id = "UiQuadSplitter";
	t.display_name = "Quad Splitter";
	t.toolbox_group = "Layouts";
	t.icon = MakeDesignerTypeIcon(t.id);
	t.is_container = true;
	t.can_have_children = true;
	t.default_size = Size(360, 220);
	t.min_size = Size(160, 120);
	t.init_defaults = [](DesignerNode& n) {
		n.properties.Set("column_percent", 50);
		n.properties.Set("row_percent", 50);
		n.properties.Set("min_a", 60);
		n.properties.Set("min_b", 60);
		n.properties.Set("min_c", 60);
		n.properties.Set("min_d", 60);
		n.properties.Set("debug", false);
		n.properties.Set("sizing", "Expand");
		n.properties.Set("h_sizing", "Expand");
		n.properties.Set("v_sizing", "Expand");
		n.properties.Set("face", DesignerLayoutFace());
		n.properties.Set("frame", DesignerLayoutFrame());
		n.properties.Set("face_enabled", false);
		n.properties.Set("frame_enabled", false);
	};
	return t;
}

static DesignerType MakeControlType(const String& id, const String& name, Size size)
{
	DesignerType t;
	t.id = id;
	t.display_name = name;
	t.toolbox_group = "Controls";
	t.icon = MakeDesignerTypeIcon(id);
	t.default_size = size;
	t.min_size = Size(24, 20);
	t.init_defaults = [=](DesignerNode& n) {
		bool placeholder = id == "Item";
		n.properties.Set("text", name);
		n.properties.Set("sizing", "Fit");
		n.properties.Set("h_sizing", "Fit");
		n.properties.Set("v_sizing", "Fit");
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
		n.properties.Set("icon", "None");
		n.properties.Set("icon_size", 18);
		if(id == "UiButton")
			n.properties.Set("align", "Center");
		if(id == "UiLineEdit")
			n.properties.Set("placeholder", "Placeholder");
		if(id == "UiToggle")
			n.properties.Set("on", true);
		if(id == "UiDropdown")
			n.properties.Set("selected", "First");
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
			n.properties.Set("active", 0);
			n.properties.Set("tab_a", "TabA");
			n.properties.Set("tab_b", "TabB");
			n.properties.Set("tab_c", "TabC");
		}
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

static DesignerType MakePanelControlType(const String& id, const String& name, Size size)
{
	DesignerType t = MakeControlType(id, name, size);
	t.toolbox_group = "Containers";
	t.is_container = true;
	t.can_have_children = true;
	t.init_defaults = [=](DesignerNode& n) {
		n.properties.Set("text", name);
		n.properties.Set("sizing", "Expand");
		n.properties.Set("h_sizing", "Expand");
		n.properties.Set("v_sizing", "Expand");
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

static DesignerType MakePaneSlotType()
{
	DesignerType t;
	t.id = "PaneSlot";
	t.display_name = "Pane Slot";
	t.icon = ICON_DESIGN_BOTTOM_PANEL_OPEN_48();
	t.is_container = true;
	t.can_have_children = true;
	t.default_size = Size(180, 120);
	t.min_size = Size(40, 30);
	t.init_defaults = [](DesignerNode& n) {
		n.properties.Set("sizing", "Expand");
		n.properties.Set("h_sizing", "Expand");
		n.properties.Set("v_sizing", "Expand");
		n.properties.Set("width", 180);
		n.properties.Set("height", 120);
		n.properties.Set("face_enabled", false);
		n.properties.Set("frame_enabled", false);
		n.properties.Set("radius", 0);
	};
	return t;
}

static DesignerType MakeWindowType()
{
	DesignerType t;
	t.id = "Window";
	t.display_name = "Window";
	t.is_container = true;
	t.can_have_children = true;
	t.default_size = Size(760, 460);
	t.min_size = Size(240, 180);
	return t;
}

void RegisterDesignerBuiltins(DesignerRegistry& registry)
{
	registry.Register(MakeWindowType());
	registry.Register(MakeBoxLayoutType());
	registry.Register(MakeGridLayoutType());
	registry.Register(MakeSplitterType());
	registry.Register(MakeQuadSplitterType());
	registry.Register(MakePaneSlotType());
	registry.Register(MakePanelControlType("UiPanel", "Panel", Size(240, 140)));
	registry.Register(MakePanelControlType("UiScrollPanel", "Scroll Panel", Size(260, 160)));
	registry.Register(MakeControlType("UiLabel", "Label", Size(120, 24)));
	registry.Register(MakeControlType("UiTitleCard", "Title Card", Size(220, 72)));
	registry.Register(MakeControlType("UiButton", "Button", Size(120, 32)));
	registry.Register(MakeControlType("UiLineEdit", "Edit", Size(180, 32)));
	registry.Register(MakeControlType("UiIntEdit", "Integer Edit", Size(140, 32)));
	registry.Register(MakeControlType("UiFloatEdit", "Float Edit", Size(140, 32)));
	registry.Register(MakeControlType("UiSlider", "Slider", Size(180, 32)));
	registry.Register(MakeControlType("UiToggle", "Toggle", Size(54, 28)));
	registry.Register(MakeControlType("UiDropdown", "Dropdown", Size(180, 32)));
	registry.Register(MakeControlType("UiCheckBox", "Checkbox", Size(150, 28)));
	registry.Register(MakeControlType("UiBreadcrumbs", "Breadcrumbs", Size(260, 32)));
	registry.Register(MakeControlType("UiTab", "Tab", Size(300, 180)));
	registry.Register(MakeControlType("UiTable", "Table", Size(320, 180)));
	registry.Register(MakeControlType("UiTree", "Tree", Size(260, 180)));
	registry.Register(MakeControlType("Item", "Item", Size(100, 32)));
}

}
