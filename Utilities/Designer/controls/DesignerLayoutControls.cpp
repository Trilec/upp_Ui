#include "DesignerControlFamilies.h"
#include "DesignerControlFamilyShared.h"
#include "../DesignerCodeGen.h"

namespace Upp {

static int DesignerLayoutSpacerLineThickness(const DesignerNode& n)
{
	return max(1, (int)DesignerNodeProperty(n, "line_thickness", 1));
}

static String DesignerLayoutSpacerLineColorExpr(const DesignerNode& n)
{
	if(DesignerNodeHasProperty(n, "line_color_enabled") && (bool)DesignerNodeProperty(n, "line_color_enabled", false)) {
		Color c = (Color)DesignerNodeProperty(n, "line_color", Color(148, 163, 184));
		return Format("Color(%d, %d, %d)", c.GetR(), c.GetG(), c.GetB());
	}
	return "UiTheme::ResolvePanel(UiPanelRole::Subtle).palette.frame[ST_NORMAL]";
}

static String DesignerLayoutSpacerLineAlignExpr(const DesignerNode& n)
{
	String align = AsString(DesignerNodeProperty(n, "line_align", "Center"));
	if(align == "Start")
		return "UiCrossAlign::Start";
	if(align == "End")
		return "UiCrossAlign::End";
	return "UiCrossAlign::Center";
}

static String DesignerLayoutSpacerLineOrientationExpr(const DesignerNode& n)
{
	String orientation = AsString(DesignerNodeProperty(n, "line_orientation", "Auto"));
	if(orientation == "Vertical")
		return "UiSpacerLineOrientation::Vertical";
	if(orientation == "Horizontal")
		return "UiSpacerLineOrientation::Horizontal";
	return "UiSpacerLineOrientation::Auto";
}

static String DesignerLayoutSpacerLineDashExpr(const DesignerNode& n)
{
	return DesignerNodeProperty(n, "line_dash", "Solid") == "Dashed" ? "DASHED" : "SOLID";
}

static String DesignerLayoutGridItemAlignHExpr(const DesignerNode& n)
{
	String align = AsString(DesignerNodeProperty(n, "cell_align_h", "Auto"));
	if(align == "Auto")
		align = AsString(DesignerNodeProperty(n, "align_h", DesignerNodeProperty(n, "align", "Left")));
	if(align == "Right")
		return "UiGridLayout::Align::End";
	if(align == "Center")
		return "UiGridLayout::Align::Center";
	return "UiGridLayout::Align::Start";
}

static String DesignerLayoutGridItemAlignVExpr(const DesignerNode& n)
{
	String align = AsString(DesignerNodeProperty(n, "cell_align_v", "Auto"));
	if(align == "Auto")
		align = AsString(DesignerNodeProperty(n, "align_v", DesignerNodeProperty(n, "align", "Top")));
	if(align == "Bottom")
		return "UiGridLayout::Align::End";
	if(align == "Center")
		return "UiGridLayout::Align::Center";
	return "UiGridLayout::Align::Start";
}

static String DesignerLayoutBoxCrossAlignExpr(const DesignerNode& n, bool horizontal_parent)
{
	String align = AsString(DesignerNodeProperty(n, horizontal_parent ? "cell_align_v" : "cell_align_h", "Auto"));
	if(align == "Center")
		return "UiBoxLayout::Align::Center";
	if(align == "Bottom" || align == "Right")
		return "UiBoxLayout::Align::End";
	return "UiBoxLayout::Align::Start";
}

static String DesignerDirectSizeModeExpr(const DesignerNode& n, const String& key)
{
	String sizing = AsString(DesignerNodeProperty(n, key, "Fit"));
	if(sizing == "Expand")
		return "UIDIRECT_EXPAND";
	if(sizing == "Fixed")
		return "UIDIRECT_FIXED";
	return "UIDIRECT_FIT";
}

static String DesignerDirectAlignHExpr(const DesignerNode& n)
{
	String align = AsString(DesignerNodeProperty(n, "cell_align_h", "Left"));
	if(align == "Center")
		return "UiAlign::CENTER";
	if(align == "Right")
		return "UiAlign::RIGHT";
	return "UiAlign::LEFT";
}

static String DesignerDirectAlignVExpr(const DesignerNode& n)
{
	String align = AsString(DesignerNodeProperty(n, "cell_align_v", "Top"));
	if(align == "Center")
		return "UiAlign::CENTER";
	if(align == "Bottom")
		return "UiAlign::BOTTOM";
	return "UiAlign::TOP";
}

static String DesignerDirectHostVar(DesignerCodeGenContext& ctx, const DesignerNode& child)
{
	return ctx.Var(child) + "_host";
}

void EmitDesignerLayoutChild(DesignerCodeGenContext& ctx, const DesignerNode& parent, const DesignerNode& child, int index)
{
	String& out = ctx.Out();
	String p = ctx.Var(parent);
	String c = ctx.Var(child);
	if(child.type_id == "Spacer" && parent.id == Designer_ROOT)
		return;
	if(child.type_id == "Spacer") {
		int weight = max(1, (int)ctx.Property(child, "weight", 1));
		int line_min = (bool)ctx.Property(child, "line_enabled", false) ? DesignerLayoutSpacerLineThickness(child) : 0;
		bool layout_break = (bool)ctx.Property(child, "layout_break", false);
		if(parent.type_id == "BoxLayout") {
			if(layout_break)
				out << "\t\t" << p << ".AddBreak(" << weight << ");\n";
			else {
				bool parent_horizontal = AsString(ctx.Property(parent, "direction", "V")) == "H";
				String main_sizing = parent_horizontal ? AsString(ctx.Property(child, "h_sizing", "Fit")) : AsString(ctx.Property(child, "v_sizing", "Fit"));
				String cross_sizing = parent_horizontal ? AsString(ctx.Property(child, "v_sizing", "Fit")) : AsString(ctx.Property(child, "h_sizing", "Fit"));
				int min_w = max(0, (int)ctx.Property(child, "min_width", 0));
				int min_h = max(0, (int)ctx.Property(child, "min_height", 0));
				int max_w = max(0, (int)ctx.Property(child, "max_width", 0));
				int max_h = max(0, (int)ctx.Property(child, "max_height", 0));
				int fixed_w = max(DesignerClampMin((int)ctx.Property(child, "fixed_width", DESIGNER_FIXED_FALLBACK_WIDTH)), min_w);
				int fixed_h = max(DesignerClampMin((int)ctx.Property(child, "fixed_height", DESIGNER_FIXED_FALLBACK_HEIGHT)), min_h);
				String orientation = AsString(ctx.Property(child, "line_orientation", "Auto"));
				if(orientation == "Vertical")
					min_w = max(min_w, line_min);
				else if(orientation == "Horizontal")
					min_h = max(min_h, line_min);
				int min_main = parent_horizontal ? min_w : min_h;
				int max_main = parent_horizontal ? max_w : max_h;
				int fixed_main = parent_horizontal ? fixed_w : fixed_h;
				int min_cross = parent_horizontal ? min_h : min_w;
				int max_cross = parent_horizontal ? max_h : max_w;
				int fixed_cross = parent_horizontal ? fixed_h : fixed_w;
				String cross_align = DesignerLayoutBoxCrossAlignExpr(child, parent_horizontal);
				out << "\t\t{\n";
				out << "\t\t\tauto spacer = " << p << ".AddSpacer(" << weight << ");\n";
				if(main_sizing == "Fixed")
					out << "\t\t\tspacer.Fixed(DPI(" << fixed_main << "));\n";
				else if(main_sizing == "Expand")
					out << "\t\t\tspacer.Expand(" << weight << ").MinMain(DPI(" << min_main << "));\n";
				else
					out << "\t\t\tspacer.Fit().MinMain(DPI(" << min_main << "));\n";
				if(max_main > 0)
					out << "\t\t\tspacer.MaxMain(DPI(" << max(max_main, min_main) << "));\n";
				if(cross_sizing == "Fixed")
					out << "\t\t\tspacer.MinMaxCross(DPI(" << fixed_cross << "), DPI(" << fixed_cross << ")).AlignSelf(" << cross_align << ");\n";
				else if(cross_sizing == "Expand") {
					if(max_cross > 0)
						out << "\t\t\tspacer.MinMaxCross(DPI(" << min_cross << "), DPI(" << max(max_cross, min_cross) << ")).AlignSelf(UiBoxLayout::Align::Stretch);\n";
					else
						out << "\t\t\tspacer.MinCross(DPI(" << min_cross << ")).AlignSelf(UiBoxLayout::Align::Stretch);\n";
				}
				else {
					if(max_cross > 0)
						out << "\t\t\tspacer.MinMaxCross(DPI(" << min_cross << "), DPI(" << max(max_cross, min_cross) << ")).AlignSelf(" << cross_align << ");\n";
					else
						out << "\t\t\tspacer.MinCross(DPI(" << min_cross << ")).AlignSelf(" << cross_align << ");\n";
				}
				if((bool)ctx.Property(child, "line_enabled", false)) {
					out << "\t\t\tspacer.LineEnabled(true)"
					    << ".LineOrientation(" << DesignerLayoutSpacerLineOrientationExpr(child) << ")"
					    << ".LineAlign(" << DesignerLayoutSpacerLineAlignExpr(child) << ")"
					    << ".LineThickness(DPI(" << DesignerLayoutSpacerLineThickness(child) << "))"
					    << ".LineDash(" << DesignerLayoutSpacerLineDashExpr(child) << ")"
					    << ".LineInset(DPI(" << max(0, (int)ctx.Property(child, "line_inset", 0)) << "))";
					if((bool)ctx.Property(child, "line_color_enabled", false))
						out << ".LineColorEnabled(true).LineColor(" << DesignerLayoutSpacerLineColorExpr(child) << ")";
					out << ";\n";
				}
				out << "\t\t}\n";
			}
			return;
		}
		if(parent.id == Designer_ROOT) {
			out << "\t\tAdd(" << c << ");\n";
			return;
		}
		if(parent.type_id == "BoxLayout") {
			out << "\t\t" << p << ".Add(" << c << ")";
			String direction = AsString(ctx.Property(parent, "direction", "V"));
			bool horizontal = direction == "H";
			String hs = AsString(ctx.Property(child, "h_sizing", "Fit"));
			String vs = AsString(ctx.Property(child, "v_sizing", "Fit"));
			int min_w = max(0, (int)ctx.Property(child, "min_width", 0));
			int min_h = max(0, (int)ctx.Property(child, "min_height", 0));
			if(horizontal) {
				if(hs == "Fixed")
					out << ".Fixed(DPI(" << max(DesignerClampMin((int)ctx.Property(child, "fixed_width", DESIGNER_FIXED_FALLBACK_WIDTH)), min_w) << "))";
				else if(hs == "Expand")
					out << ".Expand(1)";
				else
					out << ".Fit()";
				if(vs == "Fixed")
					out << ".MinMaxCross(DPI(" << max(DesignerClampMin((int)ctx.Property(child, "fixed_height", DESIGNER_FIXED_FALLBACK_HEIGHT)), min_h) << "), DPI(" << max(DesignerClampMin((int)ctx.Property(child, "fixed_height", DESIGNER_FIXED_FALLBACK_HEIGHT)), min_h) << "))";
				else if(vs == "Expand")
					out << ".MinCross(DPI(" << min_h << ")).AlignSelf(UiBoxLayout::Align::Stretch)";
				else
					out << ".MinCross(DPI(" << min_h << "))";
			}
			else {
				if(vs == "Fixed")
					out << ".Fixed(DPI(" << max(DesignerClampMin((int)ctx.Property(child, "fixed_height", DESIGNER_FIXED_FALLBACK_HEIGHT)), min_h) << "))";
				else if(vs == "Expand")
					out << ".Expand(1)";
				else
					out << ".Fit()";
				if(hs == "Fixed")
					out << ".MinMaxCross(DPI(" << max(DesignerClampMin((int)ctx.Property(child, "fixed_width", DESIGNER_FIXED_FALLBACK_WIDTH)), min_w) << "), DPI(" << max(DesignerClampMin((int)ctx.Property(child, "fixed_width", DESIGNER_FIXED_FALLBACK_WIDTH)), min_w) << "))";
				else if(hs == "Expand")
					out << ".MinCross(DPI(" << min_w << ")).AlignSelf(UiBoxLayout::Align::Stretch)";
				else
					out << ".MinCross(DPI(" << min_w << "))";
			}
			out << ";\n";
			return;
		}
		if(parent.type_id == "GridLayout") {
			int columns = max(1, (int)ctx.Property(parent, "columns", 2));
			int rows = max(1, (int)ctx.Property(parent, "rows", 2));
			int row = clamp((int)ctx.Property(child, "grid_row", index / columns), 0, rows - 1);
			int col = clamp((int)ctx.Property(child, "grid_col", index % columns), 0, columns - 1);
			String hs = AsString(ctx.Property(child, "h_sizing", "Fit"));
			String vs = AsString(ctx.Property(child, "v_sizing", "Fit"));
			if(hs == "Fixed" || vs == "Fixed") {
				int w = DesignerClampMin((int)ctx.Property(child, "fixed_width", DESIGNER_FIXED_FALLBACK_WIDTH));
				int h = DesignerClampMin((int)ctx.Property(child, "fixed_height", DESIGNER_FIXED_FALLBACK_HEIGHT));
				out << "\t\t{\n";
				out << "\t\t\tint item = " << p << ".Add(" << c << ", " << row << ", " << col
				    << ", " << (hs == "Expand" ? "true" : "false")
				    << ", " << (vs == "Expand" ? "true" : "false")
				    << ", Size(DPI(" << w << "), DPI(" << h << ")));\n";
				out << "\t\t\t" << p << ".SetItemAlign(item, " << DesignerLayoutGridItemAlignHExpr(child) << ", " << DesignerLayoutGridItemAlignVExpr(child) << ");\n";
				out << "\t\t}\n";
			}
			else {
				out << "\t\t{\n";
				out << "\t\t\tint item = " << p << ".Add(" << c << ", " << row << ", " << col
				    << ", " << (hs == "Expand" ? "true" : "false")
				    << ", " << (vs == "Expand" ? "true" : "false") << ");\n";
				out << "\t\t\t" << p << ".SetItemAlign(item, " << DesignerLayoutGridItemAlignHExpr(child) << ", " << DesignerLayoutGridItemAlignVExpr(child) << ");\n";
				out << "\t\t}\n";
			}
			return;
		}
		if(parent.type_id == "UiSplitter") {
			out << "\t\t" << p << ".Add(" << c << ");\n";
			if(index == 0) {
				out << "\t\t" << p << ".SetMinPixels(0, DPI(" << (int)ctx.Property(parent, "min_a", 80) << "));\n";
				out << "\t\t" << p << ".SetMinPixels(1, DPI(" << (int)ctx.Property(parent, "min_b", 80) << "));\n";
			}
			out << "\t\t" << p << ".SetSplitPercent(" << (int)ctx.Property(parent, "split_percent", 50) << ");\n";
			return;
		}
		if(parent.type_id == "UiQuadSplitter") {
			out << "\t\t" << p << ".Add(" << c << ");\n";
			if(index == 0) {
				out << "\t\t" << p << ".SetMinPixels(0, DPI(" << (int)ctx.Property(parent, "min_a", 60) << "));\n";
				out << "\t\t" << p << ".SetMinPixels(1, DPI(" << (int)ctx.Property(parent, "min_b", 60) << "));\n";
				out << "\t\t" << p << ".SetMinPixels(2, DPI(" << (int)ctx.Property(parent, "min_c", 60) << "));\n";
				out << "\t\t" << p << ".SetMinPixels(3, DPI(" << (int)ctx.Property(parent, "min_d", 60) << "));\n";
			}
			out << "\t\t" << p << ".SetSplitPercent(" << (int)ctx.Property(parent, "column_percent", 50) << ", "
			    << (int)ctx.Property(parent, "row_percent", 50) << ");\n";
			return;
		}
		if(parent.type_id == "UiPanel") {
			int inset = max(0, (int)ctx.Property(parent, "inset", 0));
			if(inset > 0)
				out << "\t\t" << p << ".Add(" << c << ".SizePosZ(DPI(" << inset << "), DPI(" << inset << "), DPI(" << inset << "), DPI(" << inset << ")));\n";
			else
				out << "\t\t" << p << ".Add(" << c << ".SizePos());\n";
			return;
		}
		if(parent.type_id == "UiScrollPanel") {
			int inset = max(0, (int)ctx.Property(parent, "inset", 0));
			if(inset > 0)
				out << "\t\t" << p << ".Content().Add(" << c << ".SizePosZ(DPI(" << inset << "), DPI(" << inset << "), DPI(" << inset << "), DPI(" << inset << ")));\n";
			else
				out << "\t\t" << p << ".Content().Add(" << c << ".SizePos());\n";
			return;
		}
		if(parent.type_id == "UiGroupPanel") {
			String host = DesignerDirectHostVar(ctx, child);
			int fixed_w = max(DesignerClampMin((int)ctx.Property(child, "fixed_width", DESIGNER_FIXED_FALLBACK_WIDTH)),
			                  max(0, (int)ctx.Property(child, "min_width", 0)));
			int fixed_h = max(DesignerClampMin((int)ctx.Property(child, "fixed_height", DESIGNER_FIXED_FALLBACK_HEIGHT)),
			                  max(0, (int)ctx.Property(child, "min_height", 0)));
			int min_w = max(0, (int)ctx.Property(child, "min_width", 0));
			int min_h = max(0, (int)ctx.Property(child, "min_height", 0));
			int max_w = max(0, (int)ctx.Property(child, "max_width", 0));
			int max_h = max(0, (int)ctx.Property(child, "max_height", 0));
			out << "\t\t" << host << ".SetContent(" << c << ");\n";
			out << "\t\t" << host << ".SetSizing(" << DesignerDirectSizeModeExpr(child, "h_sizing")
			    << ", " << DesignerDirectSizeModeExpr(child, "v_sizing") << ");\n";
			out << "\t\t" << host << ".SetFixedSize(Size(DPI(" << fixed_w << "), DPI(" << fixed_h << ")));\n";
			out << "\t\t" << host << ".SetMinimumSize(Size(DPI(" << min_w << "), DPI(" << min_h << ")));\n";
			out << "\t\t" << host << ".SetMaximumSize(Size(DPI(" << max_w << "), DPI(" << max_h << ")));\n";
			out << "\t\t" << host << ".SetAlign(" << DesignerDirectAlignHExpr(child) << ", "
			    << DesignerDirectAlignVExpr(child) << ");\n";
			out << "\t\t" << p << ".SetContent(" << host << ");\n";
			return;
		}
		if(parent.type_id == "UiTab") {
			String title = (bool)ctx.Property(child, "show_title", true) ? AsString(ctx.Property(child, "page_title", child.name)) : String();
			String icon = ctx.IconExpr(ctx.Property(child, "icon", "None"));
			out << "\t\t" << p << ".Add(" << c << ", " << ctx.CppString(title);
			if(!icon.IsEmpty())
				out << ", " << icon;
			out << ");\n";
			return;
		}
		if(parent.type_id == "UiStack") {
			out << "\t\t" << p << ".AddPage(" << c << ", " << ctx.CppString(ctx.Property(child, "page_title", child.name)) << ");\n";
			return;
		}
		if(parent.type_id == "UiAccordion") {
			String title = child.type_id == "AccordionSectionSlot" ? AsString(ctx.Property(child, "section_title", child.name)) : child.name;
			String subtitle = child.type_id == "AccordionSectionSlot" ? AsString(ctx.Property(child, "section_subtitle", "")) : String();
			bool open = child.type_id == "AccordionSectionSlot" ? (bool)ctx.Property(child, "open", true) : true;
			out << "\t\t{\n";
			out << "\t\t\tint section = " << p << ".AddSection(" << ctx.CppString(title) << ", " << ctx.CppString(subtitle)
			    << ", String(), " << (open ? "true" : "false") << ");\n";
			if(child.type_id == "AccordionSectionSlot") {
				String lock = AsString(ctx.Property(child, "lock", "None"));
				if(lock != "None")
					out << "\t\t\t" << p << ".SetLockMode(section, UiAccordion::Lock::" << lock << ");\n";
				int body_height = (int)ctx.Property(child, "body_height", -1);
				if(body_height > 0)
					out << "\t\t\t" << p << ".SetSectionBodyHeight(section, DPI(" << body_height << "));\n";
			}
			out << "\t\t\t" << p << ".GetSectionContent(section).Add(" << c << ".SizePos());\n";
			out << "\t\t}\n";
			return;
		}
		if(parent.type_id == "PaneSlot" || parent.type_id == "PageSlot" || parent.type_id == "AccordionSectionSlot") {
			out << "\t\t" << p << ".Add(" << c << ".SizePos());\n";
			return;
		}
	}
}

static void EmitDesignerLayoutSetup(DesignerCodeGenContext& ctx, const DesignerNode& n)
{
	String& out = ctx.Out();
	String var = ctx.Var(n);
	if(n.type_id == "UiSplitter") {
		String dir = AsString(ctx.Property(n, "direction", "H"));
		String role = AsString(ctx.Property(n, "role", "Standard"));
		String role_expr = role == "Subtle" ? "UiPanelRole::Subtle" :
		                  role == "Accent" ? "UiPanelRole::Accent" :
		                  role == "Alert" ? "UiPanelRole::Alert" : "UiPanelRole::Standard";
		out << "\t\t" << var << "." << (dir == "V" ? "Vert" : "Horz") << "();\n";
		out << "\t\t" << var << ".SetMinPixels(0, DPI(" << (int)ctx.Property(n, "min_a", 80) << "))"
		    << ".SetMinPixels(1, DPI(" << (int)ctx.Property(n, "min_b", 80) << "))"
		    << ".SetSplitPercent(" << (int)ctx.Property(n, "split_percent", 50) << ");\n";
		out << "\t\t{\n"
		    << "\t\t\tUiSplitter::Style s = UiTheme::ResolveSplitter(" << role_expr << ");\n"
		    << "\t\t\ts.hit_width = DPI(" << (int)ctx.Property(n, "hit_width", 14) << ");\n"
		    << "\t\t\ts.track_thickness = DPI(" << (int)ctx.Property(n, "track_thickness", 2) << ");\n"
		    << "\t\t\tint track_inset = DPI(" << (int)ctx.Property(n, "track_inset", 0) << ");\n"
		    << "\t\t\ts.track_inset = Rect(track_inset, track_inset, track_inset, track_inset);\n";
		int thumb_w = (int)ctx.Property(n, "thumb_width", 14);
		int thumb_h = (int)ctx.Property(n, "thumb_height", 64);
		if(dir == "V") {
			out << "\t\t\ts.thumb_main = DPI(" << thumb_w << ");\n"
			    << "\t\t\ts.thumb_cross = DPI(" << thumb_h << ");\n";
		}
		else {
			out << "\t\t\ts.thumb_main = DPI(" << thumb_h << ");\n"
			    << "\t\t\ts.thumb_cross = DPI(" << thumb_w << ");\n";
		}
		String grip_visual = AsString(ctx.Property(n, "grip_visual", ""));
		if(grip_visual.IsEmpty()) {
			if(!(bool)ctx.Property(n, "show_grip", true))
				grip_visual = "None";
			else if(!ctx.IconExpr(ctx.Property(n, "thumb_icon", "None")).IsEmpty())
				grip_visual = "Icon";
			else
				grip_visual = "Lines";
		}
		out << "\t\t\ts.grip_visual = " << (grip_visual == "None" ? "UISPLITTER_GRIP_NONE"
		                                    : grip_visual == "Dots" ? "UISPLITTER_GRIP_DOTS"
		                                    : grip_visual == "Icon" ? "UISPLITTER_GRIP_ICON"
		                                    : "UISPLITTER_GRIP_LINES") << ";\n"
		    << "\t\t\ts.grip_count = " << max(1, (int)ctx.Property(n, "grip_count", 2)) << ";\n"
		    << "\t\t\ts.grip_size = DPI(" << max(1, (int)ctx.Property(n, "grip_size", 2)) << ");\n"
		    << "\t\t\ts.grip_gap = DPI(" << max(0, (int)ctx.Property(n, "grip_gap", 3)) << ");\n";
		if((bool)ctx.Property(n, "grip_color_enabled", false))
			out << "\t\t\ts.grip_color = Color(" << ((Color)ctx.Property(n, "grip_color", Color())).GetR() << ", "
			    << ((Color)ctx.Property(n, "grip_color", Color())).GetG() << ", "
			    << ((Color)ctx.Property(n, "grip_color", Color())).GetB() << ");\n";
		else
			out << "\t\t\ts.grip_color = Null;\n";
		String icon = ctx.IconExpr(ctx.Property(n, "thumb_icon", "None"));
		if(!icon.IsEmpty()) {
			out << "\t\t\ts.thumb_icon = " << icon << ";\n"
			    << "\t\t\ts.grip_visual = UISPLITTER_GRIP_ICON;\n";
		}
		out << "\t\t\ts.thumb_icon_size = DPI(" << max(1, (int)ctx.Property(n, "thumb_icon_size", 14)) << ");\n";
		out << "\t\t\ts.thumb_metrics.radius = DPI(" << (int)ctx.Property(n, "thumb_radius", 8) << ");\n"
		    << "\t\t\t" << var << ".SetCustomStyle(s);\n"
		    << "\t\t}\n";
	}
	else if(n.type_id == "UiQuadSplitter") {
		out << "\t\t" << var << ".SetSplitPercent(" << (int)ctx.Property(n, "column_percent", 50) << ", "
		    << (int)ctx.Property(n, "row_percent", 50) << ")"
		    << ".SetMinPixels(0, DPI(" << (int)ctx.Property(n, "min_a", 60) << "))"
		    << ".SetMinPixels(1, DPI(" << (int)ctx.Property(n, "min_b", 60) << "))"
		    << ".SetMinPixels(2, DPI(" << (int)ctx.Property(n, "min_c", 60) << "))"
		    << ".SetMinPixels(3, DPI(" << (int)ctx.Property(n, "min_d", 60) << "));\n";
	}
}

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
	t.codegen.route = DesignerCodeGenRoute::LayoutCentral;
	t.codegen.emit_setup = [](DesignerCodeGenContext& ctx, const DesignerNode& n) {
		String& out = ctx.Out();
		String var = ctx.Var(n);
		String wrap = AsString(ctx.Property(n, "wrap", "None"));
		out << "\t\t" << var << ".SetDirection(" << (AsString(ctx.Property(n, "direction", "V")) == "H" ? "UiDirection::H" : "UiDirection::V") << ")"
		    << ".SetGap(DPI(" << (int)ctx.Property(n, "gap_x", (int)ctx.Property(n, "gap", 8)) << "), "
		    << "DPI(" << (int)ctx.Property(n, "gap_y", (int)ctx.Property(n, "gap", 8)) << "))"
		    << ".SetInset(DPI(" << (int)ctx.Property(n, "inset", 8) << "))"
		    << ".SetWrap(" << (wrap == "Snap" ? "UiBoxWrap::Snap" : wrap == "Flow" ? "UiBoxWrap::Flow" : "UiBoxWrap::None") << ")";
		if(wrap == "Snap") {
			out << ".SetWrapSnapCount(" << max(0, (int)ctx.Property(n, "snap_count", 0)) << ")";
			int a = max(0, (int)ctx.Property(n, "snap_size_a", 80));
			int b = max(0, (int)ctx.Property(n, "snap_size_b", 0));
			if(a > 0 || b > 0) {
				out << ".SetWrapSnapSizes(Vector<int>()";
				if(a > 0)
					out << " << DPI(" << a << ")";
				if(b > 0)
					out << " << DPI(" << b << ")";
				out << ")";
			}
		}
		if((bool)ctx.Property(n, "debug", false))
			out << ".SetDebugColor(" << ctx.ColorExpr(ctx.Property(n, "debug_color", DesignerDebugRed())) << ").SetDebug(true)";
		if(wrap != "None")
			out << ".SetWrapAutoResize(true)";
		out << ";\n";
	};
	t.codegen.emit_child = [](DesignerCodeGenContext& ctx, const DesignerNode& parent, const DesignerNode& child, int index) {
		String& out = ctx.Out();
		String p = ctx.Var(parent);
		String c = ctx.Var(child);
		if(child.type_id == "Spacer" && parent.id == Designer_ROOT)
			return;
		if(child.type_id == "Spacer") {
			int weight = max(1, (int)ctx.Property(child, "weight", 1));
		int line_min = (bool)ctx.Property(child, "line_enabled", false) ? DesignerLayoutSpacerLineThickness(child) : 0;
			bool layout_break = (bool)ctx.Property(child, "layout_break", false);
			if(layout_break)
				out << "\t\t" << p << ".AddBreak(" << weight << ");\n";
			else {
				bool parent_horizontal = AsString(ctx.Property(parent, "direction", "V")) == "H";
				String main_sizing = parent_horizontal ? AsString(ctx.Property(child, "h_sizing", "Fit")) : AsString(ctx.Property(child, "v_sizing", "Fit"));
				String cross_sizing = parent_horizontal ? AsString(ctx.Property(child, "v_sizing", "Fit")) : AsString(ctx.Property(child, "h_sizing", "Fit"));
				int min_w = max(0, (int)ctx.Property(child, "min_width", 0));
				int min_h = max(0, (int)ctx.Property(child, "min_height", 0));
				int max_w = max(0, (int)ctx.Property(child, "max_width", 0));
				int max_h = max(0, (int)ctx.Property(child, "max_height", 0));
				int fixed_w = max(DesignerClampMin((int)ctx.Property(child, "fixed_width", DESIGNER_FIXED_FALLBACK_WIDTH)), min_w);
				int fixed_h = max(DesignerClampMin((int)ctx.Property(child, "fixed_height", DESIGNER_FIXED_FALLBACK_HEIGHT)), min_h);
				String orientation = AsString(ctx.Property(child, "line_orientation", "Auto"));
				if(orientation == "Vertical")
					min_w = max(min_w, line_min);
				else if(orientation == "Horizontal")
					min_h = max(min_h, line_min);
				int min_main = parent_horizontal ? min_w : min_h;
				int max_main = parent_horizontal ? max_w : max_h;
				int fixed_main = parent_horizontal ? fixed_w : fixed_h;
				int min_cross = parent_horizontal ? min_h : min_w;
				int max_cross = parent_horizontal ? max_h : max_w;
				int fixed_cross = parent_horizontal ? fixed_h : fixed_w;
				String cross_align = DesignerLayoutBoxCrossAlignExpr(child, parent_horizontal);
				out << "\t\t{\n";
				out << "\t\t\tauto spacer = " << p << ".AddSpacer(" << weight << ");\n";
				if(main_sizing == "Fixed")
					out << "\t\t\tspacer.Fixed(DPI(" << fixed_main << "));\n";
				else if(main_sizing == "Expand")
					out << "\t\t\tspacer.Expand(" << weight << ").MinMain(DPI(" << min_main << "));\n";
				else
					out << "\t\t\tspacer.Fit().MinMain(DPI(" << min_main << "));\n";
				if(max_main > 0)
					out << "\t\t\tspacer.MaxMain(DPI(" << max(max_main, min_main) << "));\n";
				if(cross_sizing == "Fixed")
					out << "\t\t\tspacer.MinMaxCross(DPI(" << fixed_cross << "), DPI(" << fixed_cross << ")).AlignSelf(" << cross_align << ");\n";
				else if(cross_sizing == "Expand") {
					if(max_cross > 0)
						out << "\t\t\tspacer.MinMaxCross(DPI(" << min_cross << "), DPI(" << max(max_cross, min_cross) << ")).AlignSelf(UiBoxLayout::Align::Stretch);\n";
					else
						out << "\t\t\tspacer.MinCross(DPI(" << min_cross << ")).AlignSelf(UiBoxLayout::Align::Stretch);\n";
				}
				else {
					if(max_cross > 0)
						out << "\t\t\tspacer.MinMaxCross(DPI(" << min_cross << "), DPI(" << max(max_cross, min_cross) << ")).AlignSelf(" << cross_align << ");\n";
					else
						out << "\t\t\tspacer.MinCross(DPI(" << min_cross << ")).AlignSelf(" << cross_align << ");\n";
				}
				if((bool)ctx.Property(child, "line_enabled", false)) {
					out << "\t\t\tspacer.LineEnabled(true)"
					    << ".LineOrientation(" << DesignerLayoutSpacerLineOrientationExpr(child) << ")"
					    << ".LineAlign(" << DesignerLayoutSpacerLineAlignExpr(child) << ")"
					    << ".LineThickness(DPI(" << DesignerLayoutSpacerLineThickness(child) << "))"
					    << ".LineDash(" << DesignerLayoutSpacerLineDashExpr(child) << ")"
					    << ".LineInset(DPI(" << max(0, (int)ctx.Property(child, "line_inset", 0)) << "))";
					if((bool)ctx.Property(child, "line_color_enabled", false))
						out << ".LineColorEnabled(true).LineColor(" << DesignerLayoutSpacerLineColorExpr(child) << ")";
					out << ";\n";
				}
				out << "\t\t}\n";
			}
			return;
		}
		if(parent.id == Designer_ROOT) {
			out << "\t\tAdd(" << c << ");\n";
			out << "\t\t" << c << ".SizePos();\n";
			return;
		}
		if(parent.type_id == "BoxLayout") {
			out << "\t\t" << p << ".Add(" << c << ")";
			String direction = AsString(ctx.Property(parent, "direction", "V"));
			bool horizontal = direction == "H";
			String hs = AsString(ctx.Property(child, "h_sizing", "Fit"));
			String vs = AsString(ctx.Property(child, "v_sizing", "Fit"));
			int min_w = max(0, (int)ctx.Property(child, "min_width", 0));
			int min_h = max(0, (int)ctx.Property(child, "min_height", 0));
			if(horizontal) {
				if(hs == "Fixed")
					out << ".Fixed(DPI(" << max(DesignerClampMin((int)ctx.Property(child, "fixed_width", DESIGNER_FIXED_FALLBACK_WIDTH)), min_w) << "))";
				else if(hs == "Expand")
					out << ".Expand(1)";
				else
					out << ".Fit()";
				if(vs == "Fixed")
					out << ".MinMaxCross(DPI(" << max(DesignerClampMin((int)ctx.Property(child, "fixed_height", DESIGNER_FIXED_FALLBACK_HEIGHT)), min_h) << "), DPI(" << max(DesignerClampMin((int)ctx.Property(child, "fixed_height", DESIGNER_FIXED_FALLBACK_HEIGHT)), min_h) << "))";
				else if(vs == "Expand")
					out << ".MinCross(DPI(" << min_h << ")).AlignSelf(UiBoxLayout::Align::Stretch)";
				else
					out << ".MinCross(DPI(" << min_h << "))";
			}
			else {
				if(vs == "Fixed")
					out << ".Fixed(DPI(" << max(DesignerClampMin((int)ctx.Property(child, "fixed_height", DESIGNER_FIXED_FALLBACK_HEIGHT)), min_h) << "))";
				else if(vs == "Expand")
					out << ".Expand(1)";
				else
					out << ".Fit()";
				if(hs == "Fixed")
					out << ".MinMaxCross(DPI(" << max(DesignerClampMin((int)ctx.Property(child, "fixed_width", DESIGNER_FIXED_FALLBACK_WIDTH)), min_w) << "), DPI(" << max(DesignerClampMin((int)ctx.Property(child, "fixed_width", DESIGNER_FIXED_FALLBACK_WIDTH)), min_w) << "))";
				else if(hs == "Expand")
					out << ".MinCross(DPI(" << min_w << ")).AlignSelf(UiBoxLayout::Align::Stretch)";
				else
					out << ".MinCross(DPI(" << min_w << "))";
			}
			out << ";\n";
			return;
		}
		if(parent.type_id == "GridLayout") {
			int columns = max(1, (int)ctx.Property(parent, "columns", 2));
			int rows = max(1, (int)ctx.Property(parent, "rows", 2));
			int row = clamp((int)ctx.Property(child, "grid_row", index / columns), 0, rows - 1);
			int col = clamp((int)ctx.Property(child, "grid_col", index % columns), 0, columns - 1);
			String hs = AsString(ctx.Property(child, "h_sizing", "Fit"));
			String vs = AsString(ctx.Property(child, "v_sizing", "Fit"));
			if(hs == "Fixed" || vs == "Fixed") {
				int w = DesignerClampMin((int)ctx.Property(child, "fixed_width", DESIGNER_FIXED_FALLBACK_WIDTH));
				int h = DesignerClampMin((int)ctx.Property(child, "fixed_height", DESIGNER_FIXED_FALLBACK_HEIGHT));
				out << "\t\t{\n";
				out << "\t\t\tint item = " << p << ".Add(" << c << ", " << row << ", " << col
				    << ", " << (hs == "Expand" ? "true" : "false")
				    << ", " << (vs == "Expand" ? "true" : "false")
				    << ", Size(DPI(" << w << "), DPI(" << h << ")));\n";
				out << "\t\t\t" << p << ".SetItemAlign(item, " << DesignerLayoutGridItemAlignHExpr(child) << ", " << DesignerLayoutGridItemAlignVExpr(child) << ");\n";
				out << "\t\t}\n";
			}
			else {
				out << "\t\t{\n";
				out << "\t\t\tint item = " << p << ".Add(" << c << ", " << row << ", " << col
				    << ", " << (hs == "Expand" ? "true" : "false")
				    << ", " << (vs == "Expand" ? "true" : "false") << ");\n";
				out << "\t\t\t" << p << ".SetItemAlign(item, " << DesignerLayoutGridItemAlignHExpr(child) << ", " << DesignerLayoutGridItemAlignVExpr(child) << ");\n";
				out << "\t\t}\n";
			}
			return;
		}
		if(parent.type_id == "UiSplitter") {
			out << "\t\t" << p << ".Add(" << c << ");\n";
			if(index == 0) {
				out << "\t\t" << p << ".SetMinPixels(0, DPI(" << (int)ctx.Property(parent, "min_a", 80) << "));\n";
				out << "\t\t" << p << ".SetMinPixels(1, DPI(" << (int)ctx.Property(parent, "min_b", 80) << "));\n";
			}
			out << "\t\t" << p << ".SetSplitPercent(" << (int)ctx.Property(parent, "split_percent", 50) << ");\n";
			return;
		}
		if(parent.type_id == "UiQuadSplitter") {
			out << "\t\t" << p << ".Add(" << c << ");\n";
			if(index == 0) {
				out << "\t\t" << p << ".SetMinPixels(0, DPI(" << (int)ctx.Property(parent, "min_a", 60) << "));\n";
				out << "\t\t" << p << ".SetMinPixels(1, DPI(" << (int)ctx.Property(parent, "min_b", 60) << "));\n";
				out << "\t\t" << p << ".SetMinPixels(2, DPI(" << (int)ctx.Property(parent, "min_c", 60) << "));\n";
				out << "\t\t" << p << ".SetMinPixels(3, DPI(" << (int)ctx.Property(parent, "min_d", 60) << "));\n";
			}
			out << "\t\t" << p << ".SetSplitPercent(" << (int)ctx.Property(parent, "column_percent", 50) << ", "
			    << (int)ctx.Property(parent, "row_percent", 50) << ");\n";
			return;
		}
		if(parent.type_id == "UiPanel") {
			int inset = max(0, (int)ctx.Property(parent, "inset", 0));
			if(inset > 0)
				out << "\t\t" << p << ".Add(" << c << ".SizePosZ(DPI(" << inset << "), DPI(" << inset << "), DPI(" << inset << "), DPI(" << inset << ")));\n";
			else
				out << "\t\t" << p << ".Add(" << c << ".SizePos());\n";
			return;
		}
		if(parent.type_id == "UiScrollPanel") {
			int inset = max(0, (int)ctx.Property(parent, "inset", 0));
			if(inset > 0)
				out << "\t\t" << p << ".Content().Add(" << c << ".SizePosZ(DPI(" << inset << "), DPI(" << inset << "), DPI(" << inset << "), DPI(" << inset << ")));\n";
			else
				out << "\t\t" << p << ".Content().Add(" << c << ".SizePos());\n";
			return;
		}
		if(parent.type_id == "UiGroupPanel") {
			out << "\t\t" << p << ".SetContent(" << c << ");\n";
			return;
		}
		if(parent.type_id == "UiTab") {
			String title = (bool)ctx.Property(child, "show_title", true) ? AsString(ctx.Property(child, "page_title", child.name)) : String();
			String icon = ctx.IconExpr(ctx.Property(child, "icon", "None"));
			out << "\t\t" << p << ".Add(" << c << ", " << ctx.CppString(title);
			if(!icon.IsEmpty())
				out << ", " << icon;
			out << ");\n";
			return;
		}
		if(parent.type_id == "UiStack") {
			out << "\t\t" << p << ".AddPage(" << c << ", " << ctx.CppString(ctx.Property(child, "page_title", child.name)) << ");\n";
			return;
		}
		if(parent.type_id == "UiAccordion") {
			String title = child.type_id == "AccordionSectionSlot" ? AsString(ctx.Property(child, "section_title", child.name)) : child.name;
			String subtitle = child.type_id == "AccordionSectionSlot" ? AsString(ctx.Property(child, "section_subtitle", "")) : String();
			bool open = child.type_id == "AccordionSectionSlot" ? (bool)ctx.Property(child, "open", true) : true;
			out << "\t\t{\n";
			out << "\t\t\tint section = " << p << ".AddSection(" << ctx.CppString(title) << ", " << ctx.CppString(subtitle)
			    << ", String(), " << (open ? "true" : "false") << ");\n";
			if(child.type_id == "AccordionSectionSlot") {
				String lock = AsString(ctx.Property(child, "lock", "None"));
				if(lock != "None")
					out << "\t\t\t" << p << ".SetLockMode(section, UiAccordion::Lock::" << lock << ");\n";
				int body_height = (int)ctx.Property(child, "body_height", -1);
				if(body_height > 0)
					out << "\t\t\t" << p << ".SetSectionBodyHeight(section, DPI(" << body_height << "));\n";
			}
			out << "\t\t\t" << p << ".GetSectionContent(section).Add(" << c << ".SizePos());\n";
			out << "\t\t}\n";
			return;
		}
		if(parent.type_id == "PaneSlot" || parent.type_id == "PageSlot" || parent.type_id == "AccordionSectionSlot") {
			out << "\t\t" << p << ".Add(" << c << ".SizePos());\n";
			return;
		}
	};
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
	t.codegen.route = DesignerCodeGenRoute::LayoutCentral;
	t.codegen.emit_setup = [](DesignerCodeGenContext& ctx, const DesignerNode& n) {
		String& out = ctx.Out();
		String var = ctx.Var(n);
		out << "\t\t" << var << ".SetGridSize(" << max(1, (int)ctx.Property(n, "columns", 2)) << ", "
		    << max(1, (int)ctx.Property(n, "rows", 2)) << ")"
		    << ".SetMinCellSize(Size(DPI(" << DesignerClampMin((int)ctx.Property(n, "cell_width", DESIGNER_GRID_CELL_WIDTH))
		    << "), DPI(" << DesignerClampMin((int)ctx.Property(n, "cell_height", DESIGNER_GRID_CELL_HEIGHT)) << ")))"
		    << ".SetGap(DPI(" << (int)ctx.Property(n, "gap", 8) << "))"
		    << ".SetInset(DPI(" << (int)ctx.Property(n, "inset", 8) << "))";
		if((bool)ctx.Property(n, "debug", false))
			out << ".SetDebugColor(" << ctx.ColorExpr(ctx.Property(n, "debug_color", DesignerDebugRed())) << ").SetDebug(true)";
		out << ";\n";
	};
	t.codegen.emit_child = [](DesignerCodeGenContext& ctx, const DesignerNode& parent, const DesignerNode& child, int index) {
		EmitDesignerLayoutChild(ctx, parent, child, index);
	};
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
	t.codegen.route = DesignerCodeGenRoute::NoRuntimeOutput;
	t.codegen.emit_child = [](DesignerCodeGenContext& ctx, const DesignerNode& parent, const DesignerNode& child, int index) {
		EmitDesignerLayoutChild(ctx, parent, child, index);
	};
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
	t.codegen.route = DesignerCodeGenRoute::StructuralCentral;
	t.codegen.emit_setup = EmitDesignerLayoutSetup;
	t.codegen.emit_child = [](DesignerCodeGenContext& ctx, const DesignerNode& parent, const DesignerNode& child, int index) {
		EmitDesignerLayoutChild(ctx, parent, child, index);
	};
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
	t.codegen.route = DesignerCodeGenRoute::StructuralCentral;
	t.codegen.emit_setup = EmitDesignerLayoutSetup;
	t.codegen.emit_child = [](DesignerCodeGenContext& ctx, const DesignerNode& parent, const DesignerNode& child, int index) {
		EmitDesignerLayoutChild(ctx, parent, child, index);
	};
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
	t.codegen.route = DesignerCodeGenRoute::Headless;
	t.codegen.emit_child = [](DesignerCodeGenContext& ctx, const DesignerNode& parent, const DesignerNode& child, int index) {
		EmitDesignerLayoutChild(ctx, parent, child, index);
	};
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
	t.codegen.route = DesignerCodeGenRoute::NoRuntimeOutput;
	t.codegen.emit_child = [](DesignerCodeGenContext& ctx, const DesignerNode& parent, const DesignerNode& child, int index) {
		EmitDesignerLayoutChild(ctx, parent, child, index);
	};
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
