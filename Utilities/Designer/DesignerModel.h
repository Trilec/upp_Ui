#pragma once

#include <Ui/Ui.h>

// Ui Designer model layer.
// Copyright (c) 2026 C Edwards (dodobar). MIT licensed, matching the Ui package.
//
// This header defines the persistent document graph for the visual designer:
// node identity, parent/child order, property values, selection, and validation.
// UI widgets should edit this model through commands rather than keeping state
// in the preview or inspector controls.

namespace Upp {

using DesignerNodeId = int;

static constexpr DesignerNodeId Designer_NULL = 0;
static constexpr DesignerNodeId Designer_ROOT = 1;

// One editable object in the designer document.
// Layout containers, controls, splitter panes, and the virtual window all use
// the same node shape so drag/drop, undo, serialization, and codegen share one
// source of truth.
struct DesignerNode : Moveable<DesignerNode> {
	DesignerNodeId id = Designer_NULL;
	DesignerNodeId parent = Designer_NULL;
	String type_id;
	String name;
	Vector<DesignerNodeId> children;
	ValueMap properties;
	Rect last_rect;
	bool expanded = true;
};

// Snapshot used by undo/redo when a whole subtree is deleted or restored.
// It deliberately mirrors DesignerNode so command objects can preserve history
// without retaining live pointers into the model array.
struct DesignerNodeState : Moveable<DesignerNodeState> {
	DesignerNodeId id = Designer_NULL;
	DesignerNodeId parent = Designer_NULL;
	String type_id;
	String name;
	Vector<DesignerNodeId> children;
	ValueMap properties;
	Rect last_rect;
	bool expanded = true;
};

// Owns the designer document and enforces tree invariants.
// Use AddNode/MoveNode/RemoveNode for structural edits and Validate() after
// complex command groups; preview rectangles are cached here only as view hints.
class DesignerModel {
public:
	DesignerModel();

	DesignerNodeId AddNode(const String& type_id, DesignerNodeId parent, int insert_index = -1);
	bool MoveNode(DesignerNodeId id, DesignerNodeId new_parent, int insert_index = -1);
	bool RemoveNode(DesignerNodeId id);
	bool CaptureSubtree(DesignerNodeId id, Vector<DesignerNodeState>& out) const;
	bool RestoreSubtree(const Vector<DesignerNodeState>& states, DesignerNodeId parent, int insert_index = -1);
	bool SetProperty(DesignerNodeId id, const String& property_id, const Value& value);
	bool RemoveProperty(DesignerNodeId id, const String& property_id);

	const DesignerNode* Find(DesignerNodeId id) const;
	DesignerNode* Find(DesignerNodeId id);
	int FindIndex(DesignerNodeId id) const;

	const Vector<DesignerNode>& GetNodes() const { return nodes_; }
	const Vector<DesignerNodeId>& GetSelection() const { return selection_; }
	void SetSelection(const Vector<DesignerNodeId>& ids);
	void SelectOne(DesignerNodeId id);
	bool Validate(String& error) const;
	bool Validate() const { String error; return Validate(error); }

	Size GetVirtualSize() const { return virtual_size_; }
	void SetVirtualSize(Size sz);

	Event<> WhenChanged;
	Event<> WhenSelectionChanged;

private:
	bool IsDescendant(DesignerNodeId parent, DesignerNodeId possible_child) const;
	bool ValidateVisit(DesignerNodeId id, Vector<DesignerNodeId>& visiting,
	                   Vector<DesignerNodeId>& visited, String& error) const;
	void RemoveFromParent(DesignerNodeId id);
	void RemoveRecursive(DesignerNodeId id);

	Vector<DesignerNode> nodes_;
	Vector<DesignerNodeId> selection_;
	Size virtual_size_ = Size(760, 460);
	DesignerNodeId next_id_ = Designer_ROOT + 1;
};

}
