#pragma once

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    DesignerInspector
    =================

    Purpose
    - Public header for the DesignerInspector component.

    Intent
    - Define the runtime API, style contract, and integration points used by the rest of the Ui package.

    Thread context
    - GUI thread only.

    Usage
    - Include this header where the component is used or extended. Keep implementation details in the matching .cpp when present.

    Changelog
    - 2026-06: normalized the top-level header documentation.
*/

#include "DesignerAdapter.h"

// Ui Designer inspector.
// Copyright (c) 2026 C Edwards (dodobar). MIT licensed, matching the Ui package.
//
// The inspector turns adapter property descriptors into themed composite rows.
// Each selected type gets a stable page inside UiStack so hidden controls from
// previous selections do not keep stale state or overlap the active editor.

namespace Upp {

int DesignerTraceSeq();
void DesignerConsoleTrace(const String& tag, const String& msg);

struct DesignerInspectorEditIntent {
	DesignerNodeId node_id = Designer_NULL;
	String property_id;
	Value value;
	bool preview = false;
	bool final_commit = true;
	String editor_kind;
	int row_generation = 0;
	int inspector_generation = 0;
	bool syncing = false;
};

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
	void SetSelection(const Vector<DesignerNodeId>& ids);
	DesignerNodeId GetNode() const { return node_id_; }
	bool HasRow(const String& property_id) const;
	bool IsRowEnabled(const String& property_id) const;
	Value GetRowValue(const String& property_id) const;

	Size GetContentSize() const;
	int MeasureHeightForWidth(int width) const;

	virtual Size GetMinSize() const override;
	virtual void Layout() override;

	Event<DesignerNodeId, String, Value> WhenProperty;
	Event<DesignerNodeId, String, Value> WhenPropertyPreview;
	Event<const Vector<DesignerNodeId>&, String, Value> WhenPropertyMany;
	Event<const Vector<DesignerNodeId>&, String, Value> WhenPropertyManyPreview;
	Event<const DesignerInspectorEditIntent&> WhenInspectorIntent;
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
	Value SelectionProperty(const Vector<const DesignerNode *>& nodes, const DesignerApiBinding& b, bool& mixed) const;
	Value DefaultValue(const DesignerNode& n, const DesignerType& t, const DesignerApiBinding& b) const;
	Value PropertyValue(const DesignerNode& n, const DesignerType& t, const DesignerApiBinding& b) const;
	String RuntimeTypeName(const String& type_id) const;
	String NodeContextText(const DesignerNode& n, const DesignerType& t) const;
	Size NodeContextSize(const DesignerNode& n, const DesignerType& t) const;
	String PageKey(const DesignerNode& n, const Vector<DesignerApiBinding>& bindings) const;
	void Describe(Vector<DesignerApiBinding>& bindings, const DesignerNode& n) const;
	void AppendBinding(Vector<DesignerApiBinding>& bindings, const DesignerApiBinding& binding) const;
	void DescribeSelection(Vector<Vector<DesignerApiBinding>>& all_bindings, const Vector<const DesignerNode *>& nodes) const;
	void DescribeCommon(Vector<DesignerApiBinding>& bindings, const Vector<const DesignerNode *>& nodes) const;
	bool ShouldShowBinding(const DesignerApiBinding& b) const;
	// Descriptor caching is disabled for now.
	// The current descriptor set carries node-state-dependent enabled/visible
	// information for theme overrides, sizing, and control-specific parts.
	// Reusing cached bindings here is what caused stale Theme Overrides rows and
	// misleading multi-select editability. After V1 this can come back as a
	// true shape-only cache with node-state facts excluded from the cached copy.
	bool CanCacheDescriptorShape(const DesignerNode& n) const;
	String DescriptorCacheKey(const DesignerNode& n) const;

	Page& EnsurePage(const DesignerNode& n, const DesignerType& t, const Vector<DesignerApiBinding>& bindings,
	                 const String& type_text, const String& key);
	Page& AddPage(const String& key);
	int FindPage(const String& key) const;
	void AddOwned(Page& page, One<Ctrl> ctrl);
	void AddTypeRow(Page& page, const String& type_text);
	void AddContextRow(Page& page, const DesignerNode& n, const DesignerType& t);
	void AddMessageRow(Page& page, const String& text);
	void AddMultiSelectionHeader(Page& page, int count);
	void AddBindingRow(Page& page, const DesignerNode& n, const DesignerType& t, const DesignerApiBinding& b);
	void AddBindingRow(Page& page, const Vector<const DesignerNode *>& nodes, const Vector<const DesignerType *>& types,
	                   const DesignerApiBinding& b);
	String BuildNoteText(const Vector<DesignerApiBinding>& bindings) const;
	void RefreshPage(Page& page, const DesignerNode& n, const DesignerType& t,
	                 const Vector<DesignerApiBinding>& bindings, const String& type_text);
	void RefreshMultiPage(Page& page, const Vector<const DesignerNode *>& nodes,
	                      const Vector<const DesignerType *>& types, const Vector<DesignerApiBinding>& bindings);
	void SetRowValue(const DesignerNode& n, const DesignerType& t, const DesignerApiBinding& b, Row& row);
	void SetRowValue(const Vector<const DesignerNode *>& nodes, const Vector<const DesignerType *>& types,
	                 const DesignerApiBinding& b, Row& row);
	Value QuadFaceValue(const DesignerNode& n, Color face) const;
	bool IsSingleNodeInspectorStateControlled(const DesignerApiBinding& binding) const;
	void PostInspectorIntent(const DesignerInspectorEditIntent& intent);
	bool CanDeliverRowCommit(int generation, DesignerNodeId row_node, const String& property_id, const char *editor_kind) const;
	bool CanDeliverManyRowCommit(int generation, const String& property_id, const char *editor_kind) const;
	void PostInspectorCommit(int generation, DesignerNodeId row_node, const String& property_id, const Value& value);
	void PostInspectorPreview(int generation, DesignerNodeId row_node, const String& property_id, const Value& value);
	void PostInspectorManyCommit(int generation, const String& property_id, const Value& value);
	void PostInspectorManyPreview(int generation, const String& property_id, const Value& value);

	UiStack stack_;
	Array<Page> pages_;
	DesignerModel *model_ = nullptr;
	const DesignerRegistry *registry_ = nullptr;
	DesignerNodeId node_id_ = Designer_NULL;
	Vector<DesignerNodeId> selection_;
	String binding_group_;
	bool syncing_ = false;
	int inspector_generation_ = 0;
	mutable VectorMap<String, Vector<DesignerApiBinding>> descriptor_cache_;
};

}
