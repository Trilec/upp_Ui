#include "DesignerInspector.h"
#include "DesignerDefaults.h"
#include <memory>

// DesignerInspector.cpp - descriptor-driven property editor.
// Adapter bindings become themed composite rows on UiStack pages; edits are
// emitted as events so the app can apply them through commands.

namespace Upp {

static void DesignerMultiSelectLog(const String& text)
{
	return;
}

static void DesignerChoiceCommitLog(const String& text)
{
#ifdef _DEBUG
	RLOG(text);
#endif
}

static bool DesignerIsSafeMultiSelectProperty(const String& type_id, bool same_type, const String& id)
{
	if(id == "role" ||
	   id == "h_sizing" || id == "v_sizing" ||
	   id == "fixed_width" || id == "fixed_height" ||
	   id == "min_width" || id == "max_width" ||
	   id == "min_height" || id == "max_height" ||
	   id == "cell_align_h" || id == "cell_align_v" ||
	   id == "theme_override")
		return true;

	if(!same_type)
		return false;

	if(type_id == "UiButton" || type_id == "UiToolButton" || type_id == "UiSplitButton")
		return id == "text" || id == "icon" || id == "icon_size" || id == "icon_side" ||
		       id == "content_inset" || id == "content_gap" || id == "align_h" || id == "align_v";

	if(type_id == "UiLabel")
		return id == "text" || id == "icon" || id == "icon_size" || id == "icon_side" ||
		       id == "content_gap" || id == "inset" || id == "align_h" || id == "align_v" ||
		       id == "font" || id == "font_size";

	return false;
}

static bool DesignerIsSafeSizingProperty(const String& id)
{
	return id == "h_sizing" || id == "v_sizing" ||
	       id == "fixed_width" || id == "fixed_height" ||
	       id == "min_width" || id == "min_height" ||
	       id == "max_width" || id == "max_height" ||
	       id == "cell_align_h" || id == "cell_align_v";
}

static bool DesignerBindingEditableInMultiSelect(const DesignerApiBinding& binding)
{
	return DesignerIsSafeSizingProperty(binding.property_id) || binding.enabled;
}

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

static DesignerApiBinding DesignerCloneBinding(const DesignerApiBinding& src)
{
	DesignerApiBinding out;
	out.property_id = src.property_id;
	out.label = src.label;
	out.editor = src.editor;
	out.help = src.help;
	out.api_call = src.api_call;
	out.codegen_hint = src.codegen_hint;
	out.group = src.group;
	out.default_value = src.default_value;
	out.min_value = src.min_value;
	out.max_value = src.max_value;
	out.visible = src.visible;
	out.enabled = src.enabled;
	out.disabled_reason = src.disabled_reason;
	for(int i = 0; i < src.choices.GetCount(); i++)
		out.choices.Add(src.choices.GetKey(i), src.choices[i]);
	return out;
}

static const DesignerApiBinding* FindCommonBindingById(const Vector<DesignerApiBinding>& bindings, const String& id)
{
	for(const DesignerApiBinding& binding : bindings)
		if(binding.property_id == id)
			return &binding;
	return nullptr;
}

struct DesignerInspectorSurfaceDefault {
	Color face = SColorFace();
	Color frame = SColorShadow();
	int radius = 0;
	bool face_enabled = false;
	bool frame_enabled = false;
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
	out.face_enabled = s.metrics.face_enabled;
	out.frame_enabled = s.metrics.frame_enabled;
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
	if(n.type_id == "Window" || n.type_id == "UiPanel" ||
	   n.type_id == "Item" || n.type_id == "Generic")
		return DesignerInspectorSurfaceFromStyle(UiTheme::ResolvePanel(role));
	if(n.type_id == "UiScrollPanel")
		return DesignerInspectorSurfaceFromStyle(UiTheme::ResolveScrollPanel(role));
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
	inspector_generation_++;
	descriptor_cache_.Clear();
}

void DesignerInspector::SetSelection(const Vector<DesignerNodeId>& ids)
{
	selection_ = clone(ids);
	node_id_ = selection_.IsEmpty() ? Designer_NULL : selection_[0];
	inspector_generation_++;
	if(selection_.GetCount() == 1) {
		SetNode(selection_[0]);
		return;
	}
	if(!model_ || !registry_)
		return;

	Vector<const DesignerNode *> nodes;
	Vector<const DesignerType *> types;
	for(DesignerNodeId id : selection_) {
		const DesignerNode *n = model_->Find(id);
		const DesignerType *t = n ? registry_->Find(n->type_id) : nullptr;
		if(n && t) {
			nodes.Add(n);
			types.Add(t);
		}
	}
	if(nodes.GetCount() <= 1) {
		if(!nodes.IsEmpty())
			SetNode(nodes[0]->id);
		return;
	}

	Vector<DesignerApiBinding> bindings;
	DescribeCommon(bindings, nodes);
	bool same_type = true;
	for(int i = 1; i < nodes.GetCount(); i++)
		if(nodes[i]->type_id != nodes[0]->type_id)
			same_type = false;
	String type_text;
	if(same_type)
		type_text = RuntimeTypeName(nodes[0]->type_id);

	String key = "multi";
	if(same_type)
		key << ":" << nodes[0]->type_id;
	for(const DesignerApiBinding& b : bindings)
		key << "|" << b.property_id;

	syncing_ = true;
	stack_.ClearPages();
	pages_.Clear();
	Page& page = AddPage(key);
	AddMultiSelectionHeader(page, nodes.GetCount());
	if(binding_group_.IsEmpty() && same_type)
		AddTypeRow(page, type_text);
	else if(!binding_group_.IsEmpty() && bindings.IsEmpty())
		AddMessageRow(page, "No common overrides available.");
	for(const DesignerApiBinding& b : bindings)
		AddBindingRow(page, nodes, types, b);
	WhenNotes(BuildNoteText(bindings));
	RefreshMultiPage(page, nodes, types, bindings);
	stack_.SetActiveKey(key);
	if(Ctrl *active = stack_.GetActiveCtrl())
		active->Show();
	page.layout.Layout();
	stack_.Layout();
	RefreshLayout();
	Refresh();
	syncing_ = false;
}

bool DesignerInspector::HasRow(const String& property_id) const
{
	for(const Page& page : pages_)
		for(const Row& row : page.rows)
			if(row.property_id == property_id)
				return true;
	return false;
}

bool DesignerInspector::IsRowEnabled(const String& property_id) const
{
	for(const Page& page : pages_)
		for(const Row& row : page.rows)
			if(row.property_id == property_id)
				return row.ctrl && row.ctrl->IsEnabled();
	return false;
}

Value DesignerInspector::GetRowValue(const String& property_id) const
{
	for(const Page& page : pages_) {
		for(const Row& row : page.rows) {
			if(row.property_id != property_id || !row.ctrl)
				continue;
			if(const UiCompositeDropdown* c = dynamic_cast<const UiCompositeDropdown*>(row.ctrl))
				return c->GetData();
			if(const UiCompositeToggle* c = dynamic_cast<const UiCompositeToggle*>(row.ctrl))
				return c->GetData();
			if(const UiCompositeSlider* c = dynamic_cast<const UiCompositeSlider*>(row.ctrl))
				return c->GetData();
			if(const UiCompositeColor* c = dynamic_cast<const UiCompositeColor*>(row.ctrl))
				return c->GetColor(0);
			if(const UiCompositeEdit* c = dynamic_cast<const UiCompositeEdit*>(row.ctrl))
				return c->GetData();
		}
	}
	return Value();
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

Size DesignerInspector::NodeContextSize(const DesignerNode& n, const DesignerType& t) const
{
	if(n.last_rect.GetWidth() > 0 && n.last_rect.GetHeight() > 0)
		return n.last_rect.GetSize();
	if(n.id == Designer_ROOT)
		return model_ ? model_->GetVirtualSize() : t.default_size;

	Size fallback = t.default_size;
	String h_sizing = AsString(NodeProperty(n, "h_sizing", "Fit"));
	String v_sizing = AsString(NodeProperty(n, "v_sizing", "Fit"));
	if(h_sizing == "Fixed")
		fallback.cx = max(1, (int)NodeProperty(n, "fixed_width", NodeProperty(n, "width", fallback.cx)));
	else
		fallback.cx = max((int)NodeProperty(n, "min_width", DESIGNER_MIN_CLAMP), fallback.cx);
	if(v_sizing == "Fixed")
		fallback.cy = max(1, (int)NodeProperty(n, "fixed_height", NodeProperty(n, "height", fallback.cy)));
	else
		fallback.cy = max((int)NodeProperty(n, "min_height", DESIGNER_MIN_CLAMP), fallback.cy);
	return fallback;
}

String DesignerInspector::NodeContextText(const DesignerNode& n, const DesignerType& t) const
{
	Size sz = NodeContextSize(n, t);
	String type_text = RuntimeTypeName(n.type_id);
	return Format("%s [%d x %d]  %s", ~type_text, sz.cx, sz.cy, ~n.name);
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
	if(b.property_id == "title_color_enabled" || b.property_id == "subtitle_color_enabled")
		return false;
	if(b.property_id == "face_mode")
		return "Solid";
	if(b.property_id == "face_enabled" || b.property_id == "frame_enabled") {
		DesignerInspectorSurfaceDefault surface = DesignerInspectorThemeSurfaceDefault(n);
		if(!surface.found)
			return false;
		return b.property_id == "face_enabled" ? surface.face_enabled : surface.frame_enabled;
	}
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
	if(b.property_id == "title_color")
		return UiTheme::ResolveTitleCard(DesignerInspectorRoleChoice(NodeProperty(n, "role", "Standard"))).title_color;
	if(b.property_id == "subtitle_color")
		return UiTheme::ResolveTitleCard(DesignerInspectorRoleChoice(NodeProperty(n, "role", "Standard"))).subtitle_color;
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
	if(CanCacheDescriptorShape(n)) {
		String key = DescriptorCacheKey(n);
		int q = descriptor_cache_.Find(key);
		if(q >= 0) {
			for(const DesignerApiBinding& binding : descriptor_cache_[q])
				AppendBinding(bindings, binding);
			return;
		}
	}
	DesignerAdapter *adapter = nullptr;
	One<Ctrl> ctrl;
	ctrl.Attach(CreateDesignerAdapterCtrl(n, &adapter));
	if(adapter) {
		Vector<DesignerApiBinding> all;
		adapter->DescribeApi(all, n);
		for(int i = 0; i < all.GetCount(); i++)
			if(ShouldShowBinding(all[i]))
				AppendBinding(bindings, all[i]);
		if(CanCacheDescriptorShape(n)) {
			String key = DescriptorCacheKey(n);
			Vector<DesignerApiBinding> cached;
			for(const DesignerApiBinding& binding : bindings)
				AppendBinding(cached, binding);
			int q = descriptor_cache_.Find(key);
			if(q >= 0)
				descriptor_cache_[q] = pick(cached);
			else
				descriptor_cache_.Add(key, pick(cached));
		}
	}
}

void DesignerInspector::AppendBinding(Vector<DesignerApiBinding>& bindings, const DesignerApiBinding& binding) const
{
	bindings.Add(DesignerCloneBinding(binding));
}

void DesignerInspector::DescribeSelection(Vector<Vector<DesignerApiBinding>>& all_bindings,
                                          const Vector<const DesignerNode *>& nodes) const
{
	all_bindings.SetCount(nodes.GetCount());
	for(int i = 0; i < nodes.GetCount(); i++)
		Describe(all_bindings[i], *nodes[i]);
}

bool DesignerInspector::ShouldShowBinding(const DesignerApiBinding& b) const
{
	return binding_group_.IsEmpty() ? b.group.IsEmpty() : b.group == binding_group_;
}

bool DesignerInspector::CanCacheDescriptorShape(const DesignerNode& n) const
{
	return false;
}

String DesignerInspector::DescriptorCacheKey(const DesignerNode& n) const
{
	String key = n.type_id + "|" + binding_group_;
	if(n.type_id == "Spacer")
		key << "|break=" << ((bool)NodeProperty(n, "layout_break", false) ? "1" : "0");
	return key;
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
	selection_.Clear();
	selection_.Add(id);
	node_id_ = id;
	inspector_generation_++;
	if(!model_ || !registry_)
		return;
	const DesignerNode *n = model_->Find(id);
	const DesignerType *t = n ? registry_->Find(n->type_id) : nullptr;
	if(!n || !t)
		return;

	Vector<DesignerApiBinding> bindings;
	Describe(bindings, *n);
	String key = PageKey(*n, bindings);
	syncing_ = true;
	stack_.ClearPages();
	pages_.Clear();
	Page& page = AddPage(key);
	AddContextRow(page, *n, *t);
	if(!binding_group_.IsEmpty() && bindings.IsEmpty())
		AddMessageRow(page, "No overrides available");
	for(const DesignerApiBinding& b : bindings)
		AddBindingRow(page, *n, *t, b);
	WhenNotes(BuildNoteText(bindings));
	RefreshPage(page, *n, *t, bindings, NodeContextText(*n, *t));
	stack_.SetActiveKey(key);
	if(Ctrl *active = stack_.GetActiveCtrl())
		active->Show();
	page.layout.Layout();
	stack_.Layout();
	RefreshLayout();
	Refresh();
	syncing_ = false;
}

Value DesignerInspector::SelectionProperty(const Vector<const DesignerNode *>& nodes, const DesignerApiBinding& b, bool& mixed) const
{
	mixed = false;
	if(nodes.IsEmpty() || !registry_)
		return Value();
	const DesignerType *t0 = registry_->Find(nodes[0]->type_id);
	if(!t0)
		return Value();
	Value first = PropertyValue(*nodes[0], *t0, b);
	for(int i = 1; i < nodes.GetCount(); i++) {
		const DesignerType *ti = registry_->Find(nodes[i]->type_id);
		if(!ti)
			continue;
		Value v = PropertyValue(*nodes[i], *ti, b);
		if(v != first) {
			mixed = true;
			break;
		}
	}
	return first;
}

void DesignerInspector::DescribeCommon(Vector<DesignerApiBinding>& bindings, const Vector<const DesignerNode *>& nodes) const
{
	if(nodes.IsEmpty())
		return;
	bool same_type = true;
	String type_id = nodes[0]->type_id;
	for(int i = 1; i < nodes.GetCount(); i++)
		if(nodes[i]->type_id != type_id)
			same_type = false;
	Vector<Vector<DesignerApiBinding>> all_bindings;
	DescribeSelection(all_bindings, nodes);
	if(all_bindings.IsEmpty())
		return;
	const Vector<DesignerApiBinding>& first = all_bindings[0];
	for(const DesignerApiBinding& b : first) {
		if(!b.visible || b.property_id == "name" || b.editor == DesignerEditorKind::ReadOnly)
			continue;
		if(!DesignerIsSafeMultiSelectProperty(type_id, same_type, b.property_id))
			continue;
		if(!DesignerBindingEditableInMultiSelect(b))
			continue;
		bool common = true;
		for(int i = 1; i < all_bindings.GetCount() && common; i++) {
			const DesignerApiBinding *match = FindCommonBindingById(all_bindings[i], b.property_id);
			if(!match || !match->visible || match->editor != b.editor) {
				common = false;
				continue;
			}
			if(!DesignerBindingEditableInMultiSelect(*match)) {
				common = false;
				continue;
			}
			if(b.editor == DesignerEditorKind::Choice) {
				if(match->choices.GetCount() != b.choices.GetCount()) {
					common = false;
					continue;
				}
				for(int q = 0; q < b.choices.GetCount(); q++) {
					if(match->choices.GetKey(q) != b.choices.GetKey(q)) {
						common = false;
						break;
					}
				}
			}
			else if(b.editor == DesignerEditorKind::Int || b.editor == DesignerEditorKind::Slider) {
				if(match->min_value != b.min_value || match->max_value != b.max_value)
					common = false;
			}
		}
		if(common)
			AppendBinding(bindings, b);
	}
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
	AddContextRow(page, n, t);
	if(!binding_group_.IsEmpty() && bindings.IsEmpty())
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

void DesignerInspector::AddContextRow(Page& page, const DesignerNode& n, const DesignerType& t)
{
	One<Ctrl> ctrl;
	UiLabel *row = new UiLabel;
	ctrl.Attach(row);
	UiLabel::Style s = UiTheme::ResolveLabel(UiRole::Subtle);
	s.font = SansSerifZ(9).Bold();
	row->SetCustomStyle(s);
	row->SetText(NodeContextText(n, t));
	row->NoWantFocus();
	Row& r = page.rows.Add();
	r.property_id = "$context";
	r.editor = DesignerEditorKind::ReadOnly;
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

void DesignerInspector::AddMultiSelectionHeader(Page& page, int count)
{
	AddMessageRow(page, Format("Multiple selection (%d)", count));
}

void DesignerInspector::AddBindingRow(Page& page, const DesignerNode& n, const DesignerType& t,
                                        const DesignerApiBinding& b)
{
	if(!b.visible || b.property_id == "name")
		return;
	int label_w = DPI(88);
	int gap = DPI(8);
	Value value = PropertyValue(n, t, b);
	String property_id = b.property_id;
	DesignerNodeId row_node = n.id;
	int generation = inspector_generation_;

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
		row->Enable(b.enabled);
		row->WhenSelectData = [=](const Value& data) {
			if(!self)
				return;
			DesignerConsoleTrace("ROW_CHOICE",
				Format("node=%d property=%s value=%s editor=choice generation=%d inspector_generation=%d syncing=%d",
				       (int)row_node, property_id, StdFormat(data),
				       generation, inspector_generation_, syncing_ ? 1 : 0));
			PostInspectorIntent({row_node, property_id, data, false, true, "choice", generation, inspector_generation_, syncing_});
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
		row->Enable(b.enabled);
		row->WhenAction = [=] {
			if(!self)
				return;
			DesignerConsoleTrace("ROW_BOOL",
				Format("node=%d property=%s value=%s generation=%d inspector_generation=%d syncing=%d",
				       (int)row_node, property_id, StdFormat(self->GetData()),
				       generation, inspector_generation_, syncing_ ? 1 : 0));
			PostInspectorIntent({row_node, property_id, (bool)self->GetData(), false, true, "bool", generation, inspector_generation_, syncing_});
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
		row->Enable(b.enabled);
		row->WhenChanging = [=] {
			if(!self)
				return;
			int v = max(min_value, min(max_value, (int)self->GetData()));
			self->SetValueText(AsString(v));
			DesignerConsoleTrace("ROW_SLIDER_PRE",
				Format("node=%d property=%s value=%d generation=%d inspector_generation=%d syncing=%d",
				       (int)row_node, property_id, v, generation, inspector_generation_, syncing_ ? 1 : 0));
			PostInspectorIntent({row_node, property_id, v, true, false, "slider-preview", generation, inspector_generation_, syncing_});
		};
		row->WhenAction = [=] {
			if(!self)
				return;
			int v = max(min_value, min(max_value, (int)self->GetData()));
			self->SetValueText(AsString(v));
			DesignerConsoleTrace("ROW_SLIDER_COM",
				Format("node=%d property=%s value=%d generation=%d inspector_generation=%d syncing=%d",
				       (int)row_node, property_id, v, generation, inspector_generation_, syncing_ ? 1 : 0));
			PostInspectorIntent({row_node, property_id, v, false, true, "slider", generation, inspector_generation_, syncing_});
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
		row->Enable(b.enabled);
		row->WhenAction = [=] {
			if(!self)
				return;
			DesignerConsoleTrace("ROW_COLOR",
				Format("node=%d property=%s value=%s generation=%d inspector_generation=%d syncing=%d",
				       (int)row_node, property_id, StdFormat(self->GetColor(0)),
				       generation, inspector_generation_, syncing_ ? 1 : 0));
			PostInspectorIntent({row_node, property_id, self->GetColor(0), false, true, "color", generation, inspector_generation_, syncing_});
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
		row->Enable(b.enabled);
		row->WhenAction = [=] {
			if(!self)
				return;
			ValueArray out;
			for(int i = 0; i < 4; i++)
				out.Add(self->GetColor(i));
			DesignerConsoleTrace("ROW_QUAD",
				Format("node=%d property=%s value=%s generation=%d inspector_generation=%d syncing=%d",
				       (int)row_node, property_id, StdFormat(out),
				       generation, inspector_generation_, syncing_ ? 1 : 0));
			PostInspectorIntent({row_node, property_id, out, false, true, "quadcolor", generation, inspector_generation_, syncing_});
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
	row->Enable(b.enabled);
	row->WhenAction = [=] {
		if(!self)
			return;
		DesignerConsoleTrace("ROW_EDIT",
			Format("node=%d property=%s value=%s editor=edit-action generation=%d inspector_generation=%d syncing=%d",
			       (int)row_node, property_id, StdFormat(self->GetData()),
			       generation, inspector_generation_, syncing_ ? 1 : 0));
		PostInspectorIntent({row_node, property_id, self->GetData(), false, true, "edit-action", generation, inspector_generation_, syncing_});
	};
	row->WhenChange = [=] {
		if(!self)
			return;
		DesignerConsoleTrace("ROW_EDIT",
			Format("node=%d property=%s value=%s editor=edit-change generation=%d inspector_generation=%d syncing=%d",
			       (int)row_node, property_id, StdFormat(self->GetData()),
			       generation, inspector_generation_, syncing_ ? 1 : 0));
		PostInspectorIntent({row_node, property_id, self->GetData(), false, true, "edit-change", generation, inspector_generation_, syncing_});
	};
	Row& r = page.rows.Add();
	r.property_id = property_id;
	r.editor = b.editor;
	r.ctrl = row;
	AddOwned(page, pick(ctrl));
}

void DesignerInspector::AddBindingRow(Page& page, const Vector<const DesignerNode *>& nodes,
                                      const Vector<const DesignerType *>& types, const DesignerApiBinding& b)
{
	if(!b.visible || b.property_id == "name")
		return;
	bool same_type = true;
	String type_id = nodes.IsEmpty() ? String() : nodes[0]->type_id;
	for(int i = 1; i < nodes.GetCount(); i++)
		if(nodes[i]->type_id != type_id)
			same_type = false;
	if(!DesignerIsSafeMultiSelectProperty(type_id, same_type, b.property_id)) {
		return;
	}
	int label_w = DPI(88);
	int gap = DPI(8);
	bool mixed = false;
	Value value = SelectionProperty(nodes, b, mixed);
	String property_id = b.property_id;
	int generation = inspector_generation_;

	if(b.editor == DesignerEditorKind::Choice) {
		One<Ctrl> ctrl;
		UiCompositeDropdown *row = new UiCompositeDropdown;
		ctrl.Attach(row);
		Ptr<UiCompositeDropdown> self = row;
		row->SetLabel(b.label).SetLabelWidth(label_w).SetFieldGap(gap);
		if(mixed)
			row->Add("Mixed", Value());
		for(int i = 0; i < b.choices.GetCount(); i++) {
			row->Add(AsString(b.choices[i]), b.choices.GetKey(i));
			if(property_id == "icon" || property_id.EndsWith("_icon")) {
				Image icon = UiIconFromName(b.choices.GetKey(i));
				if(!IsNull(icon))
					row->Dropdown().SetItemIcon(mixed ? i + 1 : i, icon, UiIconRenderMode::MonoTint);
			}
		}
		row->SetData(mixed ? Value() : value);
		row->Enable(DesignerBindingEditableInMultiSelect(b));
		row->WhenSelectData = [=](const Value& data) {
			if(!self || IsNull(data))
				return;
			if(CanDeliverManyRowCommit(generation, property_id, "multi-choice"))
				PostInspectorManyCommit(generation, property_id, data);
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
		row->SetData(mixed ? false : (bool)value);
		row->Enable(DesignerBindingEditableInMultiSelect(b));
		row->WhenAction = [=] {
			if(self && CanDeliverManyRowCommit(generation, property_id, "multi-bool"))
				PostInspectorManyCommit(generation, property_id, (bool)self->GetData());
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
		int ivalue = IsNumber(value) ? (int)value : min_value;
		One<Ctrl> ctrl;
		UiCompositeSlider *row = new UiCompositeSlider;
		ctrl.Attach(row);
		Ptr<UiCompositeSlider> self = row;
		row->SetLabel(b.label).SetLabelWidth(label_w).SetFieldGap(gap).SetValueWidth(DPI(44));
		row->Slider().SetRange(min_value, max_value);
		row->SetData(ivalue);
		row->SetValueText(mixed ? "Mixed" : AsString(ivalue));
		row->Enable(DesignerBindingEditableInMultiSelect(b));
		row->WhenChanging = [=] {
			if(!self)
				return;
			if(!CanDeliverManyRowCommit(generation, property_id, "multi-slider-preview"))
				return;
			int v = max(min_value, min(max_value, (int)self->GetData()));
			self->SetValueText(AsString(v));
			PostInspectorManyPreview(generation, property_id, v);
		};
		row->WhenAction = [=] {
			if(!self)
				return;
			if(!CanDeliverManyRowCommit(generation, property_id, "multi-slider"))
				return;
			int v = max(min_value, min(max_value, (int)self->GetData()));
			self->SetValueText(AsString(v));
			PostInspectorManyCommit(generation, property_id, v);
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
		row->Enable(DesignerBindingEditableInMultiSelect(b));
		row->WhenAction = [=] {
			if(self && CanDeliverManyRowCommit(generation, property_id, "multi-color"))
				PostInspectorManyCommit(generation, property_id, self->GetColor(0));
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
	bool row_mixed = mixed;
	row->SetLabel(b.label).SetLabelWidth(label_w).SetFieldGap(gap).SetEditRole(UiRole::Accent);
	if(mixed) {
		row->SetData(Value());
		row->Edit().SetPlaceholder("Mixed");
	}
	else {
		row->Edit().SetPlaceholder("");
		row->SetData(value);
	}
	row->Enable(DesignerBindingEditableInMultiSelect(b));
	row->WhenAction = [=] {
		if(self && !(row_mixed && IsNull(self->GetData())) &&
		   CanDeliverManyRowCommit(generation, property_id, "multi-edit"))
			PostInspectorManyCommit(generation, property_id, self->GetData());
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

bool DesignerInspector::IsSingleNodeInspectorStateControlled(const DesignerApiBinding& binding) const
{
	if(!binding.visible)
		return false;
	if(binding.property_id == "name")
		return false;
	if(binding.editor == DesignerEditorKind::ReadOnly)
		return false;
	return true;
}

void DesignerInspector::PostInspectorIntent(const DesignerInspectorEditIntent& intent)
{
	DesignerNodeId related_id = Designer_NULL;
	if(model_) {
		if(const DesignerNode* n = model_->Find(intent.node_id))
			related_id = n->parent;
	}
	DesignerBeginTrace(TRACE_TRANSACTION, intent.node_id, related_id, intent.property_id, "inspector-row");
	DesignerConsoleTrace("INTENT_POST",
		Format("node=%d property=%s value=%s preview=%d final=%d editor=%s row_gen=%d inspector_gen=%d syncing=%d",
		       (int)intent.node_id, intent.property_id, StdFormat(intent.value),
		       intent.preview ? 1 : 0, intent.final_commit ? 1 : 0, intent.editor_kind,
		       intent.row_generation, intent.inspector_generation, intent.syncing ? 1 : 0));
	DesignerConsoleTrace("INSP_INTENT",
		Format("node=%d property=%s preview=%d final_commit=%d editor=%s row_generation=%d inspector_generation=%d syncing=%d value=%s",
		       (int)intent.node_id, intent.property_id, intent.preview ? 1 : 0, intent.final_commit ? 1 : 0,
		       intent.editor_kind, intent.row_generation, intent.inspector_generation, intent.syncing ? 1 : 0,
		       StdFormat(intent.value)));
#ifdef _DEBUG
	String type_id;
	if(model_) {
		if(const DesignerNode* n = model_->Find(intent.node_id))
			type_id = n->type_id;
	}
	RLOG(Format("RAW inspector intent: node=%d type=%s property=%s value=%s editor=%s row_generation=%d inspector_generation=%d syncing=%d preview=%d",
	            (int)intent.node_id, type_id, intent.property_id, StdFormat(intent.value), intent.editor_kind,
	            intent.row_generation, intent.inspector_generation, intent.syncing ? 1 : 0, intent.preview ? 1 : 0));
	RLOG(Format("RAW inspector intent flags: node=%d property=%s final_commit=%d",
	            (int)intent.node_id, intent.property_id, intent.final_commit ? 1 : 0));
#endif
	WhenInspectorIntent(intent);
}

bool DesignerInspector::CanDeliverRowCommit(int generation, DesignerNodeId row_node, const String& property_id, const char *editor_kind) const
{
#ifdef _DEBUG
	RLOG(Format("Inspector CanDeliverRowCommit node=%d property=%s row_generation=%d inspector_generation=%d syncing=%d editor=%s",
	            (int)row_node, property_id, generation, inspector_generation_, syncing_ ? 1 : 0, editor_kind));
#endif
	if(syncing_) {
		RLOG(Format("Inspector row commit blocked: syncing editor=%s property=%s",
		            editor_kind, property_id));
		return false;
	}

	if(node_id_ != row_node) {
		RLOG(Format("Inspector row commit blocked: node mismatch editor=%s property=%s row_node=%d current_node=%d",
		            editor_kind, property_id, (int)row_node, (int)node_id_));
		return false;
	}

	if(generation != inspector_generation_) {
		RLOG(Format("Inspector row commit generation changed but same node: editor=%s property=%s old=%d current=%d node=%d -- allowing",
		            editor_kind, property_id, generation, inspector_generation_, (int)row_node));
	}

	return true;
}

bool DesignerInspector::CanDeliverManyRowCommit(int generation, const String& property_id, const char *editor_kind) const
{
	if(syncing_) {
		RLOG(Format("Inspector row commit blocked: syncing editor=%s property=%s",
		            editor_kind, property_id));
		return false;
	}

	if(generation != inspector_generation_) {
		RLOG(Format("Inspector row commit blocked: generation mismatch editor=%s property=%s old=%d current=%d",
		            editor_kind, property_id, generation, inspector_generation_));
		return false;
	}

	return true;
}

void DesignerInspector::PostInspectorCommit(int generation, DesignerNodeId row_node, const String& property_id, const Value& value)
{
#ifdef _DEBUG
	RLOG(Format("Inspector commit post queued: node=%d property=%s raw=%s row_generation=%d inspector_generation=%d syncing=%d",
	            (int)row_node, property_id, StdFormat(value), generation, inspector_generation_, syncing_ ? 1 : 0));
#endif
	// Inspector user edits must be posted rather than committed synchronously.
	// Row callbacks may be triggered while popup/row controls are still unwinding.
	// Posting plus generation checks prevents stale row callbacks from rebuilding
	// the inspector stack during the originating control callback.
	Ptr<DesignerInspector> self = this;
	PostCallback([=] {
		if(!self) {
#ifdef _DEBUG
			RLOG(Format("PostInspectorCommit node=%d property=%s value=%s dropped=1 reason=inspector destroyed",
			            (int)row_node, property_id, StdFormat(value)));
#endif
			return;
		}
		if(self->syncing_) {
#ifdef _DEBUG
			RLOG(Format("PostInspectorCommit node=%d property=%s value=%s dropped=1 reason=syncing",
			            (int)row_node, property_id, StdFormat(value)));
#endif
			return;
		}
		if(generation != self->inspector_generation_) {
#ifdef _DEBUG
			RLOG(Format("PostInspectorCommit node=%d property=%s value=%s dropped=1 reason=generation mismatch row_generation=%d inspector_generation=%d current_node=%d",
			            (int)row_node, property_id, StdFormat(value),
			            generation, self->inspector_generation_, (int)self->node_id_));
#endif
		}
		if(self->node_id_ != row_node) {
#ifdef _DEBUG
			RLOG(Format("PostInspectorCommit node=%d property=%s value=%s dropped=1 reason=node mismatch current_node=%d",
			            (int)row_node, property_id, StdFormat(value), (int)self->node_id_));
#endif
			return;
		}
#ifdef _DEBUG
		RLOG(Format("PostInspectorCommit node=%d property=%s value=%s dropped=0",
		            (int)row_node, property_id, StdFormat(value)));
#endif
		self->WhenProperty(row_node, property_id, value);
#ifdef _DEBUG
		RLOG("Inspector commit delivered: " << property_id);
#endif
	});
}

void DesignerInspector::PostInspectorPreview(int generation, DesignerNodeId row_node, const String& property_id, const Value& value)
{
	if(syncing_ || generation != inspector_generation_ || node_id_ != row_node)
		return;
	WhenPropertyPreview(row_node, property_id, value);
}

void DesignerInspector::PostInspectorManyCommit(int generation, const String& property_id, const Value& value)
{
	Ptr<DesignerInspector> self = this;
	PostCallback([=] {
		if(!self) {
#ifdef _DEBUG
			RLOG("Inspector commit dropped: inspector destroyed");
#endif
			return;
		}
		if(self->syncing_) {
#ifdef _DEBUG
			RLOG("Inspector commit dropped: syncing property=" << property_id);
#endif
			return;
		}
		if(generation != self->inspector_generation_) {
#ifdef _DEBUG
			RLOG(Format("Inspector commit dropped: generation old=%d current=%d property=%s",
			            generation, self->inspector_generation_, property_id));
#endif
			return;
		}
#ifdef _DEBUG
		RLOG("Inspector commit requested: " << property_id << "=" << StdFormat(value));
#endif
		self->WhenPropertyMany(clone(self->selection_), property_id, value);
#ifdef _DEBUG
		RLOG("Inspector commit delivered: " << property_id);
#endif
	});
}

void DesignerInspector::PostInspectorManyPreview(int generation, const String& property_id, const Value& value)
{
	if(syncing_ || generation != inspector_generation_)
		return;
	WhenPropertyManyPreview(selection_, property_id, value);
}

void DesignerInspector::RefreshPage(Page& page, const DesignerNode& n, const DesignerType& t,
                                      const Vector<DesignerApiBinding>& bindings, const String& type_text)
{
	for(Row& row : page.rows) {
		if(row.property_id == "$context") {
			if(UiLabel *c = dynamic_cast<UiLabel *>(row.ctrl))
				c->SetText(type_text);
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

void DesignerInspector::RefreshMultiPage(Page& page, const Vector<const DesignerNode *>& nodes,
                                         const Vector<const DesignerType *>& types,
                                         const Vector<DesignerApiBinding>& bindings)
{
	for(Row& row : page.rows) {
		if(row.property_id == "$message" || row.property_id == "$type" || row.property_id == "$context")
			continue;
		for(const DesignerApiBinding& b : bindings) {
			if(b.property_id == row.property_id) {
				SetRowValue(nodes, types, b, row);
				break;
			}
		}
	}
}

void DesignerInspector::SetRowValue(const DesignerNode& n, const DesignerType& t,
                                      const DesignerApiBinding& b, Row& row)
{
	Value value = PropertyValue(n, t, b);
	if(row.ctrl)
		row.ctrl->Enable(b.enabled);
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

void DesignerInspector::SetRowValue(const Vector<const DesignerNode *>& nodes, const Vector<const DesignerType *>&,
                                    const DesignerApiBinding& b, Row& row)
{
	bool mixed = false;
	Value value = SelectionProperty(nodes, b, mixed);
	if(row.ctrl)
		row.ctrl->Enable(DesignerBindingEditableInMultiSelect(b));
	if(UiCompositeDropdown *c = dynamic_cast<UiCompositeDropdown *>(row.ctrl))
		c->SetData(mixed ? Value() : value);
	else if(UiCompositeToggle *c = dynamic_cast<UiCompositeToggle *>(row.ctrl))
		c->SetData(mixed ? false : (bool)value);
	else if(UiCompositeSlider *c = dynamic_cast<UiCompositeSlider *>(row.ctrl)) {
		int v = IsNumber(value) ? (int)value : 0;
		c->SetData(v);
		c->SetValueText(mixed ? "Mixed" : AsString(v));
	}
	else if(UiCompositeColor *c = dynamic_cast<UiCompositeColor *>(row.ctrl))
		c->SetColor(0, IsNull(value) ? Color(214, 231, 255) : (Color)value);
	else if(UiCompositeEdit *c = dynamic_cast<UiCompositeEdit *>(row.ctrl))
	{
		if(mixed) {
			c->SetData(Value());
			c->Edit().SetPlaceholder("Mixed");
		}
		else {
			c->Edit().SetPlaceholder("");
			c->SetData(value);
		}
	}
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
