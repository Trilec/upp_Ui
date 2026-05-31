#include "DesignerInspector.h"
#include "DesignerDefaults.h"

// DesignerInspector.cpp - descriptor-driven property editor.
// Adapter bindings become themed composite rows on UiStack pages; edits are
// emitted as events so the app can apply them through commands.

namespace Upp {

static UiRole DesignerInspectorRoleChoice(const Value& role)
{
	String s = AsString(role);
	if(s == "Subtle")
		return UiRole::Subtle;
	if(s == "Accent")
		return UiRole::Accent;
	if(s == "Alert")
		return UiRole::Alert;
	return UiRole::Standard;
}

struct DesignerInspectorSurfaceDefault {
	Color face = SColorFace();
	Color frame = SColorShadow();
	int radius = 0;
	bool shadow_enabled = false;
	int shadow_distance = 6;
	int shadow_offset_x = 0;
	int shadow_offset_y = 0;
	int shadow_alpha = 90;
	Color shadow_color = Black();
	String shadow_curve = "Soft";
	bool found = false;
};

template <class Style>
static DesignerInspectorSurfaceDefault DesignerInspectorSurfaceFromStyle(const Style& s)
{
	DesignerInspectorSurfaceDefault out;
	out.face = s.palette.face[ST_NORMAL].IsSolid() ? s.palette.face[ST_NORMAL].color : SColorFace();
	out.frame = s.palette.frame[ST_NORMAL];
	out.radius = s.metrics.radius;
	out.shadow_enabled = s.metrics.shadow.enabled;
	out.shadow_distance = s.metrics.shadow.distance;
	out.shadow_offset_x = s.metrics.shadow.offset_x;
	out.shadow_offset_y = s.metrics.shadow.offset_y;
	out.shadow_alpha = s.metrics.shadow.alpha;
	out.shadow_color = s.metrics.shadow.color;
	out.shadow_curve = s.metrics.shadow.mode == SHADOW_HARD ? "Hard" : "Soft";
	out.found = true;
	return out;
}

static DesignerInspectorSurfaceDefault DesignerInspectorThemeSurfaceDefault(const DesignerNode& n)
{
	int role_pos = n.properties.Find("role");
	UiRole role = DesignerInspectorRoleChoice(role_pos >= 0 ? n.properties.GetValue(role_pos) : Value("Standard"));
	if(n.type_id == "Window" || n.type_id == "UiPanel" || n.type_id == "UiScrollPanel" ||
	   n.type_id == "Item" || n.type_id == "Generic")
		return DesignerInspectorSurfaceFromStyle(UiTheme::ResolvePanel(role));
	if(n.type_id == "UiGroupPanel")
		return DesignerInspectorSurfaceFromStyle(UiTheme::ResolveGroupPanel(role));
	if(n.type_id == "UiLabel")
		return DesignerInspectorSurfaceFromStyle(UiTheme::ResolveLabel(role));
	if(n.type_id == "UiTitleCard")
		return DesignerInspectorSurfaceFromStyle(UiTheme::ResolveTitleCard(role));
	if(n.type_id == "UiButton" || n.type_id == "UiSplitButton")
		return DesignerInspectorSurfaceFromStyle(UiTheme::ResolveButton(role));
	if(n.type_id == "UiToolButton")
		return DesignerInspectorSurfaceFromStyle(UiTheme::ResolveToolButton(role));
	if(n.type_id == "UiAccordion")
		return DesignerInspectorSurfaceFromStyle(UiAccordion::StyleDefault());
	if(n.type_id == "UiLineEdit" || n.type_id == "UiIntEdit" || n.type_id == "UiFloatEdit")
		return DesignerInspectorSurfaceFromStyle(UiTheme::ResolveEdit(role));
	if(n.type_id == "UiDropdown")
		return DesignerInspectorSurfaceFromStyle(UiTheme::ResolveDropdown(role));
	if(n.type_id == "UiBreadcrumbs")
		return DesignerInspectorSurfaceFromStyle(UiBreadcrumbs::StyleDefault());
	return DesignerInspectorSurfaceDefault();
}

DesignerInspector::DesignerInspector()
{
	Add(stack_.SizePos());
}

void DesignerInspector::Set(DesignerModel *model, const DesignerRegistry *registry)
{
	model_ = model;
	registry_ = registry;
}

void DesignerInspector::SetBindingGroup(const String& group)
{
	binding_group_ = group;
}

Value DesignerInspector::NodeProperty(const DesignerNode& n, const String& key, const Value& def) const
{
	int q = n.properties.Find(key);
	return q >= 0 ? n.properties.GetValue(q) : def;
}

String DesignerInspector::RuntimeTypeName(const String& type_id) const
{
	if(type_id == "Window")
		return "TopWindow";
	if(type_id == "BoxLayout")
		return "UiBoxLayout";
	if(type_id == "GridLayout")
		return "UiGridLayout";
	if(type_id == "UiSplitter")
		return "UiSplitter";
	if(type_id == "UiQuadSplitter")
		return "UiQuadSplitter";
	if(type_id == "UiStack")
		return "UiStack";
	if(type_id == "UiAccordion")
		return "UiAccordion";
	if(type_id == "AccordionSectionSlot")
		return "ParentCtrl";
	if(type_id == "UiToolButton")
		return "UiToolButton";
	if(type_id == "PageSlot")
		return "ParentCtrl";
	if(type_id == "Item" || type_id == "Generic")
		return "UiPanel";
	return type_id;
}

Value DesignerInspector::DefaultValue(const DesignerNode& n, const DesignerType& t,
                                        const DesignerApiBinding& b) const
{
	if(!IsNull(b.default_value))
		return b.default_value;
	if(b.property_id == "direction")
		return n.type_id == "GridLayout" ? "H" : "V";
	if(b.property_id == "h_sizing" || b.property_id == "v_sizing")
		return (n.type_id == "BoxLayout" || n.type_id == "GridLayout" || n.type_id == "Window") ? "Expand" : "Fit";
	if(b.property_id == "wrap")
		return n.type_id == "GridLayout";
	if(b.property_id == "debug")
		return false;
	if(b.property_id == "debug_color")
		return Color(220, 38, 38);
	if(b.property_id == "debug_auto_color")
		return true;
	if(b.property_id == "gap" || b.property_id == "inset")
		return 8;
	if(b.property_id == "rows" || b.property_id == "columns")
		return 2;
	if(b.property_id == "cell_width")
		return DESIGNER_GRID_CELL_WIDTH;
	if(b.property_id == "cell_height")
		return DESIGNER_GRID_CELL_HEIGHT;
	if(b.property_id == "split_percent")
		return 50;
	if(b.property_id == "column_percent" || b.property_id == "row_percent")
		return 50;
	if(b.property_id == "min_a" || b.property_id == "min_b")
		return 80;
	if(b.property_id == "min_c" || b.property_id == "min_d")
		return 60;
	if(b.property_id == "hit_width")
		return 14;
	if(b.property_id == "track_thickness")
		return 2;
	if(b.property_id == "track_inset")
		return 0;
	if(b.property_id == "thumb_width")
		return 14;
	if(b.property_id == "thumb_height")
		return 64;
	if(b.property_id == "thumb_radius")
		return 8;
	if(b.property_id == "scroll_mode")
		return "Auto";
	if(b.property_id == "single_open" || b.property_id == "enforce_one" ||
	   b.property_id == "drag_reorder" || b.property_id == "show_drag_handle")
		return false;
	if(b.property_id == "show_chevron" || b.property_id == "animation" || b.property_id == "open")
		return true;
	if(b.property_id == "chevron_side")
		return "Right";
	if(b.property_id == "open_ms")
		return 120;
	if(b.property_id == "close_ms")
		return 0;
	if(b.property_id == "item_spacing")
		return 8;
	if(b.property_id == "header_body_gap")
		return 4;
	if(b.property_id == "body_min_height")
		return 88;
	if(b.property_id == "section_title")
		return "Section";
	if(b.property_id == "section_subtitle")
		return "";
	if(b.property_id == "lock")
		return "None";
	if(b.property_id == "body_height")
		return 0;

	if(b.property_id == "min" || b.property_id == "max" || b.property_id == "step")
		return b.property_id == "max" ? 100 : b.property_id == "step" ? 1 : 0;
	if(b.property_id == "value")
		return 42;
	if(b.property_id == "minf")
		return 0.0;
	if(b.property_id == "maxf")
		return 100.0;
	if(b.property_id == "stepf")
		return 0.1;
	if(b.property_id == "valuef")
		return 3.14;
	if(b.property_id == "precision")
		return 2;
	if(b.property_id == "spin")
		return true;
	if(b.property_id == "width" || b.property_id == "fixed_width")
		return t.default_size.cx;
	if(b.property_id == "height" || b.property_id == "fixed_height")
		return t.default_size.cy;
	if(b.property_id == "min_width" || b.property_id == "min_height")
		return DESIGNER_MIN_CLAMP;
	if(b.property_id == "radius") {
		DesignerInspectorSurfaceDefault surface = DesignerInspectorThemeSurfaceDefault(n);
		return surface.found ? surface.radius : 0;
	}
	if(b.property_id == "theme_override")
		return false;
	if(b.property_id == "face_mode")
		return "Solid";
	if(b.property_id == "face_enabled" || b.property_id == "frame_enabled")
		return false;
	if(b.property_id == "shadow_enabled") {
		DesignerInspectorSurfaceDefault surface = DesignerInspectorThemeSurfaceDefault(n);
		return surface.found ? surface.shadow_enabled : false;
	}
	if(b.property_id == "shadow_distance") {
		DesignerInspectorSurfaceDefault surface = DesignerInspectorThemeSurfaceDefault(n);
		return surface.found ? surface.shadow_distance : 6;
	}
	if(b.property_id == "shadow_offset_x") {
		DesignerInspectorSurfaceDefault surface = DesignerInspectorThemeSurfaceDefault(n);
		return surface.found ? surface.shadow_offset_x : 0;
	}
	if(b.property_id == "shadow_offset_y") {
		DesignerInspectorSurfaceDefault surface = DesignerInspectorThemeSurfaceDefault(n);
		return surface.found ? surface.shadow_offset_y : 0;
	}
	if(b.property_id == "shadow_alpha") {
		DesignerInspectorSurfaceDefault surface = DesignerInspectorThemeSurfaceDefault(n);
		return surface.found ? surface.shadow_alpha : 90;
	}
	if(b.property_id == "shadow_color") {
		DesignerInspectorSurfaceDefault surface = DesignerInspectorThemeSurfaceDefault(n);
		return surface.found ? surface.shadow_color : Black();
	}
	if(b.property_id == "shadow_curve") {
		DesignerInspectorSurfaceDefault surface = DesignerInspectorThemeSurfaceDefault(n);
		return surface.found ? surface.shadow_curve : "Soft";
	}
	if(b.property_id == "title_line")
		return true;
	if(b.property_id == "card_line")
		return false;
	if(b.property_id == "align")
		return "Left";
	if(b.property_id == "font" || b.property_id == "title_font" || b.property_id == "subtitle_font")
		return "Sans";
	if(b.property_id == "font_size")
		return 11;
	if(b.property_id == "title_size")
		return 12;
	if(b.property_id == "subtitle_size")
		return 10;
	if(b.property_id == "subtitle")
		return "";
	if(b.property_id == "face")
	{
		DesignerInspectorSurfaceDefault surface = DesignerInspectorThemeSurfaceDefault(n);
		return surface.found ? surface.face : Color(203, 224, 255);
	}
	if(b.property_id == "face_quad") {
		DesignerInspectorSurfaceDefault surface = DesignerInspectorThemeSurfaceDefault(n);
		Color face = surface.found ? surface.face : Color(203, 224, 255);
		return QuadFaceValue(n, face);
	}
	if(b.property_id == "frame")
	{
		DesignerInspectorSurfaceDefault surface = DesignerInspectorThemeSurfaceDefault(n);
		return surface.found ? surface.frame : Color(54, 116, 210);
	}
	if(b.property_id == "text")
		return n.name;
	if(b.property_id == "value")
		return 50;
	return Value();
}

Value DesignerInspector::PropertyValue(const DesignerNode& n, const DesignerType& t,
                                         const DesignerApiBinding& b) const
{
	return NodeProperty(n, b.property_id, DefaultValue(n, t, b));
}

void DesignerInspector::Describe(Vector<DesignerApiBinding>& bindings, const DesignerNode& n) const
{
	if(!registry_ || n.id == Designer_ROOT)
		return;
	DesignerAdapter *adapter = nullptr;
	One<Ctrl> ctrl;
	ctrl.Attach(CreateDesignerAdapterCtrl(n, &adapter));
	if(adapter) {
		Vector<DesignerApiBinding> all;
		adapter->DescribeApi(all, n);
		for(int i = 0; i < all.GetCount(); i++)
			if(ShouldShowBinding(all[i]))
				bindings.Add(pick(all[i]));
	}
}

bool DesignerInspector::ShouldShowBinding(const DesignerApiBinding& b) const
{
	return binding_group_.IsEmpty() ? b.group.IsEmpty() : b.group == binding_group_;
}

String DesignerInspector::PageKey(const DesignerNode& n, const Vector<DesignerApiBinding>& bindings) const
{
	String key = n.type_id;
	for(const DesignerApiBinding& b : bindings)
		if(b.visible && b.enabled && b.property_id != "name")
			key << '|' << b.property_id;
	return key;
}

void DesignerInspector::SetNode(DesignerNodeId id)
{
	node_id_ = id;
	if(!model_ || !registry_)
		return;
	const DesignerNode *n = model_->Find(id);
	const DesignerType *t = n ? registry_->Find(n->type_id) : nullptr;
	if(!n || !t)
		return;

	Vector<DesignerApiBinding> bindings;
	Describe(bindings, *n);
		String type_text = RuntimeTypeName(n->type_id);
	if(n->last_rect.GetWidth() > 0 && n->last_rect.GetHeight() > 0)
		type_text << " (" << AsString(n->last_rect.GetWidth()) << "x"
		          << AsString(n->last_rect.GetHeight()) << ") WH";
	else if(n->id == Designer_ROOT)
		type_text << " (" << AsString(model_->GetVirtualSize().cx) << "x"
		          << AsString(model_->GetVirtualSize().cy) << ") WH";
	else
		type_text << " (size pending) WH";

	String key = PageKey(*n, bindings);
	syncing_ = true;
	stack_.ClearPages();
	pages_.Clear();
	Page& page = AddPage(key);
	if(binding_group_.IsEmpty()) {
		AddTypeRow(page, type_text);
		AddNameRow(page, *n);
	}
	else if(bindings.IsEmpty())
		AddMessageRow(page, "No overrides available");
	for(const DesignerApiBinding& b : bindings)
		AddBindingRow(page, *n, *t, b);
	WhenNotes(BuildNoteText(bindings));
	RefreshPage(page, *n, *t, bindings, type_text);
	stack_.SetActiveKey(key);
	if(Ctrl *active = stack_.GetActiveCtrl())
		active->Show();
	page.layout.Layout();
	stack_.Layout();
	RefreshLayout();
	Refresh();
	syncing_ = false;
}

int DesignerInspector::FindPage(const String& key) const
{
	for(int i = 0; i < pages_.GetCount(); i++)
		if(pages_[i].key == key)
			return i;
	return -1;
}

DesignerInspector::Page& DesignerInspector::AddPage(const String& key)
{
	Page& page = pages_.Add();
	page.key = key;
	page.layout.SetGap(DPI(8)).SetInset(DPI(8));
	stack_.AddPage(page.layout, key);
	return page;
}

DesignerInspector::Page& DesignerInspector::EnsurePage(const DesignerNode& n, const DesignerType& t,
                                                           const Vector<DesignerApiBinding>& bindings,
                                                           const String& type_text, const String& key)
{
	int q = FindPage(key);
	if(q >= 0)
		return pages_[q];
	Page& page = AddPage(key);
	if(binding_group_.IsEmpty()) {
		AddTypeRow(page, type_text);
		AddNameRow(page, n);
	}
	else if(bindings.IsEmpty())
		AddMessageRow(page, "No overrides available");
	for(const DesignerApiBinding& b : bindings)
		AddBindingRow(page, n, t, b);
	return page;
}

void DesignerInspector::AddOwned(Page& page, One<Ctrl> ctrl)
{
	Ctrl& c = *ctrl;
	page.owned.Add(pick(ctrl));
	page.layout.Add(c).Fit();
}

void DesignerInspector::AddTypeRow(Page& page, const String& type_text)
{
	One<Ctrl> ctrl;
	UiCompositeLabel *row = new UiCompositeLabel;
	ctrl.Attach(row);
	row->SetLabel("Type").SetLabelWidth(DPI(88)).SetFieldGap(DPI(8)).SetValueRole(UiRole::Standard);
	row->SetValueText(type_text);
	Row& r = page.rows.Add();
	r.property_id = "$type";
	r.editor = DesignerEditorKind::ReadOnly;
	r.ctrl = row;
	AddOwned(page, pick(ctrl));
}

void DesignerInspector::AddNameRow(Page& page, const DesignerNode& n)
{
	One<Ctrl> ctrl;
	UiCompositeEdit *row = new UiCompositeEdit;
	ctrl.Attach(row);
	DesignerNodeId row_node = n.id;
	Ptr<UiCompositeEdit> self = row;
	row->SetLabel("Name").SetLabelWidth(DPI(88)).SetFieldGap(DPI(8)).SetEditRole(UiRole::Accent);
	row->SetData(n.name);
	row->WhenAction = [=] {
		if(!syncing_ && self && node_id_ == row_node)
			WhenName(row_node, AsString(self->GetData()));
	};
	row->WhenChange = [=] {
		if(!syncing_ && self && node_id_ == row_node)
			WhenName(row_node, AsString(self->GetData()));
	};
	Row& r = page.rows.Add();
	r.property_id = "name";
	r.editor = DesignerEditorKind::Text;
	r.ctrl = row;
	AddOwned(page, pick(ctrl));
}

void DesignerInspector::AddMessageRow(Page& page, const String& text)
{
	One<Ctrl> ctrl;
	UiCompositeLabel *row = new UiCompositeLabel;
	ctrl.Attach(row);
	row->SetLabel("").SetLabelWidth(DPI(0)).SetFieldGap(0).SetValueRole(UiRole::Subtle);
	row->SetValueText(text);
	Row& r = page.rows.Add();
	r.property_id = "$message";
	r.editor = DesignerEditorKind::ReadOnly;
	r.ctrl = row;
	AddOwned(page, pick(ctrl));
}

void DesignerInspector::AddBindingRow(Page& page, const DesignerNode& n, const DesignerType& t,
                                        const DesignerApiBinding& b)
{
	if(!b.visible || !b.enabled || b.property_id == "name")
		return;
	int label_w = DPI(88);
	int gap = DPI(8);
	Value value = PropertyValue(n, t, b);
	String property_id = b.property_id;
	DesignerNodeId row_node = n.id;

	if(b.editor == DesignerEditorKind::Choice) {
		One<Ctrl> ctrl;
		UiCompositeDropdown *row = new UiCompositeDropdown;
		ctrl.Attach(row);
		Ptr<UiCompositeDropdown> self = row;
		row->SetLabel(b.label).SetLabelWidth(label_w).SetFieldGap(gap);
		for(int i = 0; i < b.choices.GetCount(); i++) {
			row->Add(AsString(b.choices[i]), b.choices.GetKey(i));
			if(property_id == "icon" || property_id.EndsWith("_icon")) {
				Image icon = UiIconFromName(b.choices.GetKey(i));
				if(!IsNull(icon))
					row->Dropdown().SetItemIcon(i, icon, UiIconRenderMode::MonoTint);
			}
		}
		row->SetData(value);
		row->WhenSelectData = [=](const Value& data) {
			if(self && node_id_ == row_node)
				CommitChoice(property_id, data, "select");
		};
		row->WhenClose = [=] {
			PostCallback([=] {
				if(self && node_id_ == row_node)
					CommitChoice(property_id, self->GetData(), "close");
			});
		};
		Row& r = page.rows.Add();
		r.property_id = property_id;
		r.editor = b.editor;
		r.ctrl = row;
		AddOwned(page, pick(ctrl));
		return;
	}

	if(b.editor == DesignerEditorKind::Bool) {
		One<Ctrl> ctrl;
		UiCompositeToggle *row = new UiCompositeToggle;
		ctrl.Attach(row);
		Ptr<UiCompositeToggle> self = row;
		row->SetLabel(b.label).SetLabelWidth(label_w).SetFieldGap(gap).ShowValue(false);
		row->SetData((bool)value);
		row->WhenAction = [=] {
			if(!syncing_ && self && node_id_ == row_node)
				WhenProperty(row_node, property_id, (bool)self->GetData());
		};
		Row& r = page.rows.Add();
		r.property_id = property_id;
		r.editor = b.editor;
		r.ctrl = row;
		AddOwned(page, pick(ctrl));
		return;
	}

	if(b.editor == DesignerEditorKind::Int || b.editor == DesignerEditorKind::Slider) {
		int min_value = IsNumber(b.min_value) ? (int)b.min_value : 0;
		int max_value = IsNumber(b.max_value) ? (int)b.max_value : 1000;
		int ivalue = IsNumber(value) ? (int)value : 0;
		One<Ctrl> ctrl;
		UiCompositeSlider *row = new UiCompositeSlider;
		ctrl.Attach(row);
		Ptr<UiCompositeSlider> self = row;
		row->SetLabel(b.label).SetLabelWidth(label_w).SetFieldGap(gap).SetValueWidth(DPI(36));
		row->Slider().SetRange(min_value, max_value);
		row->SetData(ivalue);
		row->SetValueText(AsString(ivalue));
		row->WhenChanging = [=] {
			if(syncing_ || !self || node_id_ != row_node)
				return;
			int v = max(min_value, min(max_value, (int)self->GetData()));
			self->SetValueText(AsString(v));
			WhenProperty(row_node, property_id, v);
		};
		row->WhenAction = [=] {
			if(syncing_ || !self || node_id_ != row_node)
				return;
			int v = max(min_value, min(max_value, (int)self->GetData()));
			self->SetValueText(AsString(v));
			WhenProperty(row_node, property_id, v);
		};
		Row& r = page.rows.Add();
		r.property_id = property_id;
		r.editor = b.editor;
		r.ctrl = row;
		AddOwned(page, pick(ctrl));
		return;
	}

	if(b.editor == DesignerEditorKind::Color) {
		One<Ctrl> ctrl;
		UiCompositeColor *row = new UiCompositeColor;
		ctrl.Attach(row);
		Ptr<UiCompositeColor> self = row;
		row->SetLabel(b.label).SetLabelWidth(label_w).SetFieldGap(gap).SetColorCount(1).ShowValue(true);
		row->SetColor(0, IsNull(value) ? Color(214, 231, 255) : (Color)value);
		row->WhenAction = [=] {
			if(!syncing_ && self && node_id_ == row_node)
				WhenProperty(row_node, property_id, self->GetColor(0));
		};
		Row& r = page.rows.Add();
		r.property_id = property_id;
		r.editor = b.editor;
		r.ctrl = row;
		AddOwned(page, pick(ctrl));
		return;
	}

	if(b.editor == DesignerEditorKind::QuadColor) {
		ValueArray colors = value.Is<ValueArray>() ? ValueArray(value) : ValueArray();
		while(colors.GetCount() < 4)
			colors.Add(Color(214, 231, 255));
		One<Ctrl> ctrl;
		UiCompositeColor *row = new UiCompositeColor;
		ctrl.Attach(row);
		Ptr<UiCompositeColor> self = row;
		row->SetLabel(b.label).SetLabelWidth(label_w).SetFieldGap(gap).SetColorCount(4).ShowValue(false);
		row->SetColorLabel(0, "Top left").SetColorLabel(1, "Top right")
		   .SetColorLabel(2, "Bottom left").SetColorLabel(3, "Bottom right");
		for(int i = 0; i < 4; i++)
			row->SetColor(i, IsNull(colors[i]) ? Color(214, 231, 255) : (Color)colors[i]);
		row->WhenAction = [=] {
			if(!syncing_ && self && node_id_ == row_node) {
				ValueArray out;
				for(int i = 0; i < 4; i++)
					out.Add(self->GetColor(i));
				WhenProperty(row_node, property_id, out);
			}
		};
		Row& r = page.rows.Add();
		r.property_id = property_id;
		r.editor = b.editor;
		r.ctrl = row;
		AddOwned(page, pick(ctrl));
		return;
	}

	One<Ctrl> ctrl;
	UiCompositeEdit *row = new UiCompositeEdit;
	ctrl.Attach(row);
	Ptr<UiCompositeEdit> self = row;
	row->SetLabel(b.label).SetLabelWidth(label_w).SetFieldGap(gap).SetEditRole(UiRole::Accent);
	row->SetData(value);
	row->WhenAction = [=] {
		if(!syncing_ && self && node_id_ == row_node)
			WhenProperty(row_node, property_id, self->GetData());
	};
	row->WhenChange = [=] {
		if(!syncing_ && self && node_id_ == row_node)
			WhenProperty(row_node, property_id, self->GetData());
	};
	Row& r = page.rows.Add();
	r.property_id = property_id;
	r.editor = b.editor;
	r.ctrl = row;
	AddOwned(page, pick(ctrl));
}

String DesignerInspector::BuildNoteText(const Vector<DesignerApiBinding>& bindings) const
{
	String note;
	for(const DesignerApiBinding& b : bindings) {
		if(b.property_id == "width" || b.property_id == "height")
			continue;
		if(b.visible && !b.enabled && !b.disabled_reason.IsEmpty()) {
			if(!note.IsEmpty())
				note << "\n";
			note << b.label << ": " << b.disabled_reason;
		}
	}
	return note;
}

void DesignerInspector::RefreshPage(Page& page, const DesignerNode& n, const DesignerType& t,
                                      const Vector<DesignerApiBinding>& bindings, const String& type_text)
{
	for(Row& row : page.rows) {
		if(row.property_id == "$type") {
			if(UiCompositeLabel *c = dynamic_cast<UiCompositeLabel *>(row.ctrl))
				c->SetValueText(type_text);
			continue;
		}
		if(row.property_id == "name") {
			if(UiCompositeEdit *c = dynamic_cast<UiCompositeEdit *>(row.ctrl))
				c->SetData(n.name);
			continue;
		}
		for(const DesignerApiBinding& b : bindings) {
			if(b.property_id == row.property_id) {
				SetRowValue(n, t, b, row);
				break;
			}
		}
	}
}

void DesignerInspector::SetRowValue(const DesignerNode& n, const DesignerType& t,
                                      const DesignerApiBinding& b, Row& row)
{
	Value value = PropertyValue(n, t, b);
	if(UiCompositeDropdown *c = dynamic_cast<UiCompositeDropdown *>(row.ctrl))
		c->SetData(value);
	else if(UiCompositeToggle *c = dynamic_cast<UiCompositeToggle *>(row.ctrl))
		c->SetData((bool)value);
	else if(UiCompositeSlider *c = dynamic_cast<UiCompositeSlider *>(row.ctrl)) {
		int v = IsNumber(value) ? (int)value : 0;
		c->SetData(v);
		c->SetValueText(AsString(v));
	}
	else if(UiCompositeColor *c = dynamic_cast<UiCompositeColor *>(row.ctrl)) {
		if(row.editor == DesignerEditorKind::QuadColor) {
			ValueArray colors = value.Is<ValueArray>() ? ValueArray(value) : ValueArray();
			while(colors.GetCount() < 4)
				colors.Add(Color(214, 231, 255));
			c->SetColorCount(4);
			for(int i = 0; i < 4; i++)
				c->SetColor(i, IsNull(colors[i]) ? Color(214, 231, 255) : (Color)colors[i]);
		}
		else
			c->SetColor(0, IsNull(value) ? Color(214, 231, 255) : (Color)value);
	}
	else if(UiCompositeEdit *c = dynamic_cast<UiCompositeEdit *>(row.ctrl))
		c->SetData(value);
}

Value DesignerInspector::QuadFaceValue(const DesignerNode& n, Color face) const
{
	Value quad = NodeProperty(n, "face_quad", Value());
	if(quad.Is<ValueArray>()) {
		ValueArray a = quad;
		while(a.GetCount() < 4)
			a.Add(face);
		return a;
	}
	ValueArray a;
	a.Add(NodeProperty(n, "face_tl", face));
	a.Add(NodeProperty(n, "face_tr", Blend(face, White(), 18)));
	a.Add(NodeProperty(n, "face_bl", Blend(face, Black(), 10)));
	a.Add(NodeProperty(n, "face_br", Blend(face, White(), 8)));
	return a;
}

void DesignerInspector::CommitChoice(const String& property_id, const Value& value, const char *source)
{
	if(syncing_ || IsNull(value) || !model_)
		return;
	const DesignerNode *n = model_->Find(node_id_);
	if(!n || n->id == Designer_ROOT)
		return;
	Value current = NodeProperty(*n, property_id, Value());
	if(!IsNull(current) && current == value)
		return;
	WhenProperty(node_id_, property_id, value);
}

void DesignerInspector::CommitChoice(const String& property_id, UiCompositeDropdown *row, const char *source)
{
	if(!row)
		return;
	CommitChoice(property_id, row->GetData(), source);
}

void DesignerInspector::Layout()
{
	Show();
	stack_.Show();
	stack_.SetRect(GetSize());
	stack_.Layout();
	if(Ctrl *active = stack_.GetActiveCtrl())
		active->Show();
}

Size DesignerInspector::GetMinSize() const
{
	return stack_.GetMinSize();
}

Size DesignerInspector::GetContentSize() const
{
	return stack_.GetContentSize();
}

int DesignerInspector::MeasureHeightForWidth(int width) const
{
	if(Ctrl *c = const_cast<UiStack&>(stack_).GetActiveCtrl()) {
		if(UiBoxLayout *box = dynamic_cast<UiBoxLayout *>(c))
			return box->MeasureHeightForWidth(width);
		return c->GetMinSize().cy;
	}
	return GetMinSize().cy;
}

}
