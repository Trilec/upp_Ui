#pragma once

#include "DesignerAdapter.h"

// Ui Designer inspector.
// Copyright (c) 2026 C Edwards (dodobar). MIT licensed, matching the Ui package.
//
// The inspector turns adapter property descriptors into themed composite rows.
// Each selected type gets a stable page inside UiStack so hidden controls from
// previous selections do not keep stale state or overlap the active editor.

namespace Upp {

// Descriptor-driven property editor for the selected DesignerNode.
// It commits changes through events only; the owning window turns those events
// into commands and refreshes preview/code/hierarchy from the model.
class DesignerInspector : public ParentCtrl {
public:
	typedef DesignerInspector CLASSNAME;

	DesignerInspector();

	void Set(DesignerModel *model, const DesignerRegistry *registry);
	void SetBindingGroup(const String& group);
	void SetNode(DesignerNodeId id);
	DesignerNodeId GetNode() const { return node_id_; }

	Size GetContentSize() const;
	int MeasureHeightForWidth(int width) const;

	virtual Size GetMinSize() const override;
	virtual void Layout() override;

	Event<DesignerNodeId, String, Value> WhenProperty;
	Event<DesignerNodeId, String> WhenName;
	Event<String> WhenNotes;

private:
	struct Row : Moveable<Row> {
		String property_id;
		DesignerEditorKind editor = DesignerEditorKind::Text;
		Ctrl *ctrl = nullptr;
	};

	struct Page {
		String key;
		UiBoxLayout layout;
		Vector<Row> rows;
		Vector<One<Ctrl>> owned;

		Page() : layout(UiDirection::V) {}
	};

	Value NodeProperty(const DesignerNode& n, const String& key, const Value& def) const;
	Value DefaultValue(const DesignerNode& n, const DesignerType& t, const DesignerApiBinding& b) const;
	Value PropertyValue(const DesignerNode& n, const DesignerType& t, const DesignerApiBinding& b) const;
	String RuntimeTypeName(const String& type_id) const;
	String PageKey(const DesignerNode& n, const Vector<DesignerApiBinding>& bindings) const;
	void Describe(Vector<DesignerApiBinding>& bindings, const DesignerNode& n) const;
	bool ShouldShowBinding(const DesignerApiBinding& b) const;

	Page& EnsurePage(const DesignerNode& n, const DesignerType& t, const Vector<DesignerApiBinding>& bindings,
	                 const String& type_text, const String& key);
	Page& AddPage(const String& key);
	int FindPage(const String& key) const;
	void AddOwned(Page& page, One<Ctrl> ctrl);
	void AddTypeRow(Page& page, const String& type_text);
	void AddNameRow(Page& page, const DesignerNode& n);
	void AddMessageRow(Page& page, const String& text);
	void AddBindingRow(Page& page, const DesignerNode& n, const DesignerType& t, const DesignerApiBinding& b);
	String BuildNoteText(const Vector<DesignerApiBinding>& bindings) const;
	void RefreshPage(Page& page, const DesignerNode& n, const DesignerType& t,
	                 const Vector<DesignerApiBinding>& bindings, const String& type_text);
	void SetRowValue(const DesignerNode& n, const DesignerType& t, const DesignerApiBinding& b, Row& row);
	void CommitChoice(const String& property_id, const Value& value, const char *source);
	void CommitChoice(const String& property_id, UiCompositeDropdown *row, const char *source);

	UiStack stack_;
	Array<Page> pages_;
	DesignerModel *model_ = nullptr;
	const DesignerRegistry *registry_ = nullptr;
	DesignerNodeId node_id_ = Designer_NULL;
	String binding_group_;
	bool syncing_ = false;
};

}
