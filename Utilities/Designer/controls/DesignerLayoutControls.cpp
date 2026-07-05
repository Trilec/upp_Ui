#include "DesignerControlFamilies.h"
#include "DesignerControlFamilyShared.h"

namespace Upp {

static DesignerType MakeBoxLayoutType()
{
	DesignerType t;
	t.id = "BoxLayout";
	t.display_name = "Box Layout";
	t.default_base_name = "boxLayout";
	t.toolbox_group = "Layouts";
	t.runtime_cpp_type = "UiBoxLayout";
	t.icon = ICON_DESIGN_BOX_LAYOUT_48();
	t.is_container = true;
	t.can_have_children = true;
	t.capabilities.is_layout = true;
	t.capabilities.is_visible_control = false;
	t.capabilities.is_container = true;
	t.capabilities.can_have_children = true;
	t.capabilities.supports_children = true;
	t.capabilities.supports_theme_export = false;
	t.child_emission = DesignerLayoutChildEmissionStrategy::BoxLayoutItem;
	SetDesignerAdapterFactory<DesignerBoxLayoutAdapter>(t);
	t.default_size = Size(260, 160);
	t.min_size = Size(80, 50);
	t.init_defaults = [](DesignerNode& n) {
		n.properties.Set("direction", "V");
		n.properties.Set("wrap", "None");
		n.properties.Set("gap_x", 8);
		n.properties.Set("gap_y", 8);
		n.properties.Set("snap_count", 0);
		n.properties.Set("snap_size_a", 80);
		n.properties.Set("snap_size_b", 0);
		n.properties.Set("gap", 8);
		n.properties.Set("inset", 8);
		n.properties.Set("debug", false);
		n.properties.Set("debug_color", DesignerDebugRed());
		n.properties.Set("debug_auto_color", true);
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
	t.default_base_name = "gridLayout";
	t.toolbox_group = "Layouts";
	t.runtime_cpp_type = "UiGridLayout";
	t.icon = ICON_DESIGN_GRID_4X4_48();
	t.is_container = true;
	t.can_have_children = true;
	t.capabilities.is_layout = true;
	t.capabilities.is_visible_control = false;
	t.capabilities.is_container = true;
	t.capabilities.can_have_children = true;
	t.capabilities.supports_children = true;
	t.capabilities.supports_theme_export = false;
	t.child_emission = DesignerLayoutChildEmissionStrategy::GridItem;
	SetDesignerAdapterFactory<DesignerGridLayoutAdapter>(t);
	t.default_size = Size(280, 180);
	t.min_size = Size(DESIGNER_GRID_MIN_WIDTH, DESIGNER_GRID_MIN_HEIGHT);
	t.init_defaults = [](DesignerNode& n) {
		n.properties.Set("cell_width", DESIGNER_GRID_CELL_WIDTH);
		n.properties.Set("cell_height", DESIGNER_GRID_CELL_HEIGHT);
		n.properties.Set("rows", 2);
		n.properties.Set("columns", 2);
		n.properties.Set("gap", 8);
		n.properties.Set("inset", 8);
		n.properties.Set("debug", false);
		n.properties.Set("debug_color", DesignerDebugRed());
		n.properties.Set("debug_auto_color", true);
		n.properties.Set("h_sizing", "Expand");
		n.properties.Set("v_sizing", "Expand");
		n.properties.Set("face", DesignerLayoutFace());
		n.properties.Set("frame", DesignerLayoutFrame());
		n.properties.Set("radius", 0);
	};
	return t;
}

static DesignerType MakeSpacerType()
{
	DesignerType t;
	t.id = "Spacer";
	t.display_name = "Spacer";
	t.default_base_name = "spacer";
	t.toolbox_group = "Layouts";
	t.runtime_cpp_type = "UiSpacer";
	t.icon = ICON_DESIGN_FIT_WIDTH_48();
	t.capabilities.is_layout = true;
	t.capabilities.is_visible_control = false;
	t.capabilities.supports_theme_export = false;
	SetDesignerAdapterFactory<DesignerPanelAdapter>(t);
	t.default_size = Size(32, 32);
	t.min_size = Size(1, 1);
	t.init_defaults = [](DesignerNode& n) {
		n.properties.Set("weight", 1);
		n.properties.Set("layout_break", false);
		n.properties.Set("line_enabled", false);
		n.properties.Set("line_orientation", "Auto");
		n.properties.Set("line_align", "Center");
		n.properties.Set("line_thickness", 1);
		n.properties.Set("line_dash", "Solid");
		n.properties.Set("line_inset", 0);
		n.properties.Set("line_color_enabled", false);
		n.properties.Set("line_color", Null);
		n.properties.Set("h_sizing", "Expand");
		n.properties.Set("v_sizing", "Expand");
		n.properties.Set("width", 24);
		n.properties.Set("height", 24);
		n.properties.Set("fixed_width", 24);
		n.properties.Set("fixed_height", 24);
		n.properties.Set("min_width", 10);
		n.properties.Set("min_height", 10);
		n.properties.Set("max_width", 0);
		n.properties.Set("max_height", 0);
		n.properties.Set("face_enabled", false);
		n.properties.Set("frame_enabled", false);
		n.properties.Set("radius", 0);
	};
	return t;
}

static DesignerType MakeSplitterType()
{
	DesignerType t;
	t.id = "UiSplitter";
	t.display_name = "Splitter";
	t.default_base_name = "splitter";
	t.toolbox_group = "Layouts";
	t.runtime_cpp_type = "UiSplitter";
	t.icon = ICON_DESIGN_BORDER_HORIZONTAL_48();
	t.is_container = true;
	t.can_have_children = true;
	t.capabilities.is_layout = true;
	t.capabilities.is_visible_control = false;
	t.capabilities.is_container = true;
	t.capabilities.is_pane_container = true;
	t.capabilities.can_have_children = true;
	t.capabilities.supports_children = true;
	t.capabilities.requires_default_child_slots = true;
	t.capabilities.supports_theme_export = false;
	t.default_child_slots = DesignerDefaultChildSlotSet::SplitterTwoPanes;
	t.child_emission = DesignerLayoutChildEmissionStrategy::SplitterPane;
	SetDesignerAdapterFactory<DesignerSplitterAdapter>(t);
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
		n.properties.Set("grip_visual", "Lines");
		n.properties.Set("grip_count", 2);
		n.properties.Set("grip_size", 2);
		n.properties.Set("grip_gap", 3);
		n.properties.Set("grip_color_enabled", false);
		n.properties.Set("grip_color", Color());
		n.properties.Set("thumb_width", 14);
		n.properties.Set("thumb_height", 64);
		n.properties.Set("thumb_radius", 8);
		n.properties.Set("thumb_icon", "None");
		n.properties.Set("thumb_icon_size", 14);
		n.properties.Set("debug", false);
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
	t.default_base_name = "quadSplitter";
	t.toolbox_group = "Layouts";
	t.runtime_cpp_type = "UiQuadSplitter";
	t.icon = ICON_DESIGN_BORDER_INNER_48();
	t.is_container = true;
	t.can_have_children = true;
	t.capabilities.is_layout = true;
	t.capabilities.is_visible_control = false;
	t.capabilities.is_container = true;
	t.capabilities.is_pane_container = true;
	t.capabilities.can_have_children = true;
	t.capabilities.supports_children = true;
	t.capabilities.requires_default_child_slots = true;
	t.capabilities.supports_theme_export = false;
	t.default_child_slots = DesignerDefaultChildSlotSet::QuadSplitterFourPanes;
	t.child_emission = DesignerLayoutChildEmissionStrategy::SplitterPane;
	SetDesignerAdapterFactory<DesignerQuadSplitterAdapter>(t);
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
		n.properties.Set("h_sizing", "Expand");
		n.properties.Set("v_sizing", "Expand");
		n.properties.Set("face", DesignerLayoutFace());
		n.properties.Set("frame", DesignerLayoutFrame());
		n.properties.Set("face_enabled", false);
		n.properties.Set("frame_enabled", false);
	};
	return t;
}

static DesignerType MakePaneSlotType()
{
	DesignerType t;
	t.id = "PaneSlot";
	t.display_name = "Pane Slot";
	t.default_base_name = "paneSlot";
	t.runtime_cpp_type = "ParentCtrl";
	t.icon = ICON_DESIGN_BOTTOM_PANEL_OPEN_48();
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
	SetDesignerAdapterFactory<DesignerPanelAdapter>(t);
	t.default_size = Size(180, 120);
	t.min_size = Size(40, 30);
	t.init_defaults = [](DesignerNode& n) {
		n.properties.Set("h_sizing", "Expand");
		n.properties.Set("v_sizing", "Expand");
		n.properties.Set("fixed_width", 180);
		n.properties.Set("fixed_height", 120);
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
	t.default_base_name = "window";
	t.runtime_cpp_type = "TopWindow";
	t.is_container = true;
	t.can_have_children = true;
	t.capabilities.is_layout = true;
	t.capabilities.is_visible_control = false;
	t.capabilities.is_container = true;
	t.capabilities.can_have_children = true;
	t.capabilities.supports_children = true;
	t.capabilities.supports_theme_export = false;
	t.child_emission = DesignerLayoutChildEmissionStrategy::DirectChild;
	SetDesignerAdapterFactory<DesignerPanelAdapter>(t);
	t.default_size = DesignerWindowSize();
	t.min_size = DesignerWindowMinSize();
	t.can_drop = [](const DesignerNode&, const DesignerNode& child) {
		return child.type_id != "Spacer";
	};
	return t;
}

void RegisterDesignerLayoutControls(DesignerRegistry& registry)
{
	registry.Register(MakeWindowType());
	registry.Register(MakeBoxLayoutType());
	registry.Register(MakeGridLayoutType());
	registry.Register(MakeSpacerType());
	registry.Register(MakeSplitterType());
	registry.Register(MakeQuadSplitterType());
	registry.Register(MakePaneSlotType());
}

}
