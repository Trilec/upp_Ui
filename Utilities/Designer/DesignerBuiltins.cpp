#include "DesignerBuiltins.h"

namespace Upp {

static Image MakeDesignerTypeIcon(const String& id)
{
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
	else if(id == "UiTitleCard") {
		frame(2, 3, 14, 13, green);
		rect(4, 5, 12, 7, green);
		rect(4, 9, 10, 10, green);
	}
	else if(id == "UiLabel") {
		rect(3, 5, 13, 7, green);
		rect(3, 9, 10, 11, green);
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
		n.properties.Set("face", Color(207, 242, 226));
		n.properties.Set("frame", Color(44, 156, 105));
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
		n.properties.Set("mode", "Flow");
		n.properties.Set("direction", "H");
		n.properties.Set("wrap", true);
		n.properties.Set("align_cells", true);
		n.properties.Set("cell_width", 120);
		n.properties.Set("cell_height", 96);
		n.properties.Set("rows", 2);
		n.properties.Set("columns", 2);
		n.properties.Set("gap", 8);
		n.properties.Set("inset", 8);
		n.properties.Set("debug", false);
		n.properties.Set("sizing", "Expand");
		n.properties.Set("face", Color(207, 242, 226));
		n.properties.Set("frame", Color(44, 156, 105));
		n.properties.Set("radius", 0);
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
		n.properties.Set("text", name);
		n.properties.Set("sizing", "Fit");
		n.properties.Set("width", size.cx);
		n.properties.Set("height", size.cy);
		n.properties.Set("face", Color(214, 231, 255));
		n.properties.Set("frame", Color(54, 116, 210));
		n.properties.Set("radius", 0);
		n.properties.Set("face_enabled", true);
		n.properties.Set("frame_enabled", true);
		n.properties.Set("font", "Sans");
		n.properties.Set("font_size", 11);
		n.properties.Set("align", "Left");
		if(id == "UiButton")
			n.properties.Set("align", "Center");
		if(id == "UiLineEdit")
			n.properties.Set("placeholder", "Placeholder");
		if(id == "UiToggle")
			n.properties.Set("on", true);
		if(id == "UiDropdown")
			n.properties.Set("selected", "First");
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
	registry.Register(MakeControlType("UiLabel", "Label", Size(120, 24)));
	registry.Register(MakeControlType("UiTitleCard", "Title Card", Size(220, 72)));
	registry.Register(MakeControlType("UiButton", "Button", Size(120, 32)));
	registry.Register(MakeControlType("UiLineEdit", "Edit", Size(180, 32)));
	registry.Register(MakeControlType("UiSlider", "Slider", Size(180, 32)));
	registry.Register(MakeControlType("UiToggle", "Toggle", Size(54, 28)));
	registry.Register(MakeControlType("UiDropdown", "Dropdown", Size(180, 32)));
	registry.Register(MakeControlType("Item", "Item", Size(100, 32)));
}

}
