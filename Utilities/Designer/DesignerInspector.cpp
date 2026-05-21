#include "DesignerInspector.h"

// DesignerInspector.cpp - descriptor-driven property editor.
// Adapter bindings become themed composite rows on UiStack pages; edits are
// emitted as events so the app can apply them through commands.

namespace Upp {

DesignerInspector::DesignerInspector()
{
	Add(stack_.SizePos());
}

void DesignerInspector::Set(DesignerModel *model, const DesignerRegistry *registry)
{
	model_ = model;
	registry_ = registry;
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
	if(type_id == "Item")
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
	if(b.property_id == "gap" || b.property_id == "inset")
		return 8;
	if(b.property_id == "rows" || b.property_id == "columns")
		return 2;
	if(b.property_id == "cell_width")
		return 120;
	if(b.property_id == "cell_height")
		return 32;
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
	if(b.property_id == "width")
		return t.default_size.cx;
	if(b.property_id == "height")
		return t.default_size.cy;
	if(b.property_id == "radius")
		return 0;
	if(b.property_id == "face_enabled" || b.property_id == "frame_enabled" || b.property_id == "title_line")
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
		return (n.type_id == "BoxLayout" || n.type_id == "GridLayout") ? Color(207, 242, 226) : Color(214, 231, 255);
	if(b.property_id == "frame")
		return (n.type_id == "BoxLayout" || n.type_id == "GridLayout") ? Color(44, 156, 105) : Color(54, 116, 210);
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
	if(adapter)
		adapter->DescribeApi(bindings, n);
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
		type_text << Format(" (%dx%d Actual)", n->last_rect.GetWidth(), n->last_rect.GetHeight());
	else if(n->id == Designer_ROOT)
		type_text << Format(" (%dx%d Virtual)", model_->GetVirtualSize().cx, model_->GetVirtualSize().cy);

	String key = PageKey(*n, bindings);
	syncing_ = true;
	stack_.ClearPages();
	pages_.Clear();
	Page& page = AddPage(key);
	AddTypeRow(page, type_text);
	AddNameRow(page, *n);
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
	AddTypeRow(page, type_text);
	AddNameRow(page, n);
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
		for(int i = 0; i < b.choices.GetCount(); i++)
			row->Add(AsString(b.choices[i]), b.choices.GetKey(i));
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
	else if(UiCompositeColor *c = dynamic_cast<UiCompositeColor *>(row.ctrl))
		c->SetColor(0, IsNull(value) ? Color(214, 231, 255) : (Color)value);
	else if(UiCompositeEdit *c = dynamic_cast<UiCompositeEdit *>(row.ctrl))
		c->SetData(value);
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
