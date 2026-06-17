#include "DesignerModel.h"

// DesignerModel.cpp - source-of-truth document tree for the designer.
// The model owns hierarchy, properties, selection, and validation; preview and
// inspector state is derived from it and should not become authoritative.

namespace Upp {

static int FindId(const Vector<DesignerNodeId>& ids, DesignerNodeId id)
{
	for(int i = 0; i < ids.GetCount(); i++)
		if(ids[i] == id)
			return i;
	return -1;
}

static int NormalizeInsertIndex(int requested, int child_count)
{
	if(requested < 0)
		return child_count;
	return clamp(requested, 0, child_count);
}

DesignerModel::DesignerModel()
{
	DesignerNode& root = nodes_.Add();
	root.id = Designer_ROOT;
	root.type_id = "Window";
	root.name = "Window";
	selection_.Add(Designer_ROOT);
}

int DesignerModel::FindIndex(DesignerNodeId id) const
{
	for(int i = 0; i < nodes_.GetCount(); i++)
		if(nodes_[i].id == id)
			return i;
	return -1;
}

DesignerNode* DesignerModel::Find(DesignerNodeId id)
{
	int q = FindIndex(id);
	return q >= 0 ? &nodes_[q] : nullptr;
}

const DesignerNode* DesignerModel::Find(DesignerNodeId id) const
{
	int q = FindIndex(id);
	return q >= 0 ? &nodes_[q] : nullptr;
}

DesignerNodeId DesignerModel::AddNode(const String& type_id, DesignerNodeId parent, int insert_index)
{
	int parent_index = FindIndex(parent);
	if(parent_index < 0)
		parent = Designer_ROOT;
	parent_index = FindIndex(parent);
	if(parent_index < 0)
		return Designer_NULL;
	int child_count = nodes_[parent_index].children.GetCount();

	DesignerNode& n = nodes_.Add();
	n.id = next_id_++;
	n.parent = parent;
	n.type_id = type_id;
	n.name = Format("%s%d", type_id, n.id);

	parent_index = FindIndex(parent);
	insert_index = NormalizeInsertIndex(insert_index, child_count);
	if(insert_index < 0 || insert_index >= nodes_[parent_index].children.GetCount())
		nodes_[parent_index].children.Add(n.id);
	else
		nodes_[parent_index].children.Insert(insert_index, n.id);

	WhenChanged();
	return n.id;
}

void DesignerModel::RemoveFromParent(DesignerNodeId id)
{
	DesignerNode* n = Find(id);
	DesignerNode* p = n ? Find(n->parent) : nullptr;
	if(!p)
		return;
	int q = FindId(p->children, id);
	if(q >= 0)
		p->children.Remove(q);
}

bool DesignerModel::IsDescendant(DesignerNodeId parent, DesignerNodeId possible_child) const
{
	const DesignerNode* p = Find(possible_child);
	while(p && p->parent) {
		if(p->parent == parent)
			return true;
		p = Find(p->parent);
	}
	return false;
}

bool DesignerModel::MoveNode(DesignerNodeId id, DesignerNodeId new_parent, int insert_index)
{
	if(id == Designer_ROOT || id == Designer_NULL || id == new_parent)
		return false;
	DesignerNode* n = Find(id);
	DesignerNode* p = Find(new_parent);
	if(!n || !p || IsDescendant(id, new_parent))
		return false;

	RemoveFromParent(id);
	n->parent = new_parent;
	insert_index = NormalizeInsertIndex(insert_index, p->children.GetCount());
	if(insert_index < 0 || insert_index >= p->children.GetCount())
		p->children.Add(id);
	else
		p->children.Insert(insert_index, id);
	WhenChanged();
	return true;
}

void DesignerModel::RemoveRecursive(DesignerNodeId id)
{
	DesignerNode* n = Find(id);
	if(!n || id == Designer_ROOT)
		return;
	Vector<DesignerNodeId> children = clone(n->children);
	for(DesignerNodeId child : children)
		RemoveRecursive(child);
	int q = FindIndex(id);
	if(q >= 0)
		nodes_.Remove(q);
}

bool DesignerModel::RemoveNode(DesignerNodeId id)
{
	if(id == Designer_ROOT || !Find(id))
		return false;
	RemoveFromParent(id);
	RemoveRecursive(id);
	SelectOne(Designer_ROOT);
	WhenChanged();
	return true;
}

static void CopyNodeState(DesignerNodeState& s, const DesignerNode& n)
{
	s.id = n.id;
	s.parent = n.parent;
	s.type_id = n.type_id;
	s.name = n.name;
	s.children = clone(n.children);
	s.properties = n.properties;
	s.last_rect = n.last_rect;
	s.expanded = n.expanded;
}

static void CopyNode(DesignerNode& n, const DesignerNode& s)
{
	n.id = s.id;
	n.parent = s.parent;
	n.type_id = s.type_id;
	n.name = s.name;
	n.children = clone(s.children);
	n.properties = s.properties;
	n.last_rect = s.last_rect;
	n.expanded = s.expanded;
}

static Vector<DesignerNode> CloneNodes(const Vector<DesignerNode>& nodes)
{
	Vector<DesignerNode> out;
	for(const DesignerNode& s : nodes) {
		DesignerNode& n = out.Add();
		CopyNode(n, s);
	}
	return out;
}

bool DesignerModel::CaptureSubtree(DesignerNodeId id, Vector<DesignerNodeState>& out) const
{
	const DesignerNode* n = Find(id);
	if(!n || id == Designer_ROOT)
		return false;
	DesignerNodeState& s = out.Add();
	CopyNodeState(s, *n);
	for(DesignerNodeId child : n->children)
		if(!CaptureSubtree(child, out))
			return false;
	return true;
}

bool DesignerModel::RestoreSubtree(const Vector<DesignerNodeState>& states, DesignerNodeId parent, int insert_index)
{
	if(states.IsEmpty() || !Find(parent))
		return false;
	for(const DesignerNodeState& s : states)
		if(Find(s.id))
			return false;

	Vector<DesignerNodeId> top_children = clone(states[0].children);
	for(const DesignerNodeState& s : states) {
		DesignerNode& n = nodes_.Add();
		n.id = s.id;
		n.parent = s.parent;
		n.type_id = s.type_id;
		n.name = s.name;
		n.children = clone(s.children);
		n.properties = s.properties;
		n.last_rect = s.last_rect;
		n.expanded = s.expanded;
		next_id_ = max(next_id_, n.id + 1);
	}

	DesignerNode* restored = Find(states[0].id);
	DesignerNode* p = Find(parent);
	if(!restored || !p)
		return false;
	restored->parent = parent;
	restored->children = pick(top_children);
	insert_index = NormalizeInsertIndex(insert_index, p->children.GetCount());
	if(insert_index < 0 || insert_index >= p->children.GetCount())
		p->children.Add(restored->id);
	else
		p->children.Insert(insert_index, restored->id);
	SelectOne(restored->id);
	WhenChanged();
	return true;
}

bool DesignerModel::ReplaceDocument(const Vector<DesignerNodeState>& states, Size virtual_size,
                                    const Vector<DesignerNodeId>& selection, String& error)
{
	Vector<DesignerNode> old_nodes = CloneNodes(nodes_);
	Vector<DesignerNodeId> old_selection = clone(selection_);
	Size old_virtual_size = virtual_size_;
	DesignerNodeId old_next_id = next_id_;

	nodes_.Clear();
	next_id_ = Designer_ROOT + 1;
	for(const DesignerNodeState& s : states) {
		DesignerNode& n = nodes_.Add();
		n.id = s.id;
		n.parent = s.parent;
		n.type_id = s.type_id;
		n.name = s.name;
		n.children = clone(s.children);
		n.properties = s.properties;
		n.last_rect = s.last_rect;
		n.expanded = s.expanded;
		next_id_ = max(next_id_, n.id + 1);
	}
	virtual_size_ = Size(DesignerClampMin(virtual_size.cx, DESIGNER_WINDOW_MIN_WIDTH),
	                     DesignerClampMin(virtual_size.cy, DESIGNER_WINDOW_MIN_HEIGHT));
	selection_.Clear();
	for(DesignerNodeId id : selection)
		if(Find(id) && FindId(selection_, id) < 0)
			selection_.Add(id);
	if(selection_.IsEmpty())
		selection_.Add(Designer_ROOT);

	if(!Validate(error)) {
		nodes_ = pick(old_nodes);
		selection_ = pick(old_selection);
		virtual_size_ = old_virtual_size;
		next_id_ = old_next_id;
		return false;
	}
	WhenChanged();
	WhenSelectionChanged();
	return true;
}

bool DesignerModel::SetProperty(DesignerNodeId id, const String& property_id, const Value& value)
{
	DesignerNode* n = Find(id);
	if(!n)
		return false;
	n->properties.Set(property_id, value);
	WhenChanged();
	return true;
}

bool DesignerModel::RemoveProperty(DesignerNodeId id, const String& property_id)
{
	DesignerNode* n = Find(id);
	if(!n)
		return false;
	int q = n->properties.Find(property_id);
	if(q < 0)
		return false;
	n->properties.Remove(q);
	WhenChanged();
	return true;
}

void DesignerModel::SetSelection(const Vector<DesignerNodeId>& ids)
{
	selection_.Clear();
	for(DesignerNodeId id : ids)
		if(Find(id) && FindId(selection_, id) < 0)
			selection_.Add(id);
	if(selection_.IsEmpty())
		selection_.Add(Designer_ROOT);
	WhenSelectionChanged();
}

void DesignerModel::SelectOne(DesignerNodeId id)
{
	Vector<DesignerNodeId> ids;
	ids.Add(Find(id) ? id : Designer_ROOT);
	SetSelection(ids);
}

void DesignerModel::AddToSelection(DesignerNodeId id)
{
	id = Find(id) ? id : Designer_ROOT;
	if(FindId(selection_, id) >= 0)
		return;
	selection_.Add(id);
	WhenSelectionChanged();
}

void DesignerModel::RemoveFromSelection(DesignerNodeId id)
{
	int q = FindId(selection_, id);
	if(q < 0)
		return;
	selection_.Remove(q);
	if(selection_.IsEmpty())
		selection_.Add(Designer_ROOT);
	WhenSelectionChanged();
}

void DesignerModel::ToggleSelection(DesignerNodeId id)
{
	id = Find(id) ? id : Designer_ROOT;
	int q = FindId(selection_, id);
	if(q >= 0)
		selection_.Remove(q);
	else
		selection_.Add(id);
	if(selection_.IsEmpty())
		selection_.Add(Designer_ROOT);
	WhenSelectionChanged();
}

bool DesignerModel::IsSelected(DesignerNodeId id) const
{
	return FindId(selection_, id) >= 0;
}

bool DesignerModel::SetPropertyMany(const Vector<DesignerNodeId>& ids, const String& property_id, const Value& value)
{
	bool changed = false;
	for(DesignerNodeId id : ids) {
		DesignerNode* n = Find(id);
		if(!n)
			continue;
		int q = n->properties.Find(property_id);
		if(q >= 0 && n->properties.GetValue(q) == value)
			continue;
		n->properties.Set(property_id, value);
		changed = true;
	}
	if(changed)
		WhenChanged();
	return changed;
}

bool DesignerModel::ValidateVisit(DesignerNodeId id, Vector<DesignerNodeId>& visiting,
                                    Vector<DesignerNodeId>& visited, String& error) const
{
	if(FindId(visiting, id) >= 0) {
		error = Format("Cycle detected at node %d", id);
		return false;
	}
	if(FindId(visited, id) >= 0)
		return true;
	const DesignerNode* n = Find(id);
	if(!n) {
		error = Format("Missing node %d", id);
		return false;
	}
	visiting.Add(id);
	for(DesignerNodeId child_id : n->children) {
		const DesignerNode* child = Find(child_id);
		if(!child) {
			error = Format("Node %d references missing child %d", id, child_id);
			return false;
		}
		if(child->parent != id) {
			error = Format("Child %d parent mismatch", child_id);
			return false;
		}
		if(!ValidateVisit(child_id, visiting, visited, error))
			return false;
	}
	visiting.Remove(visiting.GetCount() - 1);
	visited.Add(id);
	return true;
}

bool DesignerModel::Validate(String& error) const
{
	error.Clear();
	if(!Find(Designer_ROOT)) {
		error = "Root node is missing";
		return false;
	}
	if(virtual_size_.cx < 40 || virtual_size_.cy < 40) {
		error = "Virtual window size is below minimum";
		return false;
	}
	for(int i = 0; i < nodes_.GetCount(); i++) {
		const DesignerNode& n = nodes_[i];
		if(n.id == Designer_NULL) {
			error = "Null node id is not allowed";
			return false;
		}
		for(int j = i + 1; j < nodes_.GetCount(); j++) {
			if(nodes_[j].id == n.id) {
				error = Format("Duplicate node id %d", n.id);
				return false;
			}
		}
		if(n.id == Designer_ROOT) {
			if(n.parent != Designer_NULL) {
				error = "Root node must not have a parent";
				return false;
			}
		}
		else if(!Find(n.parent)) {
			error = Format("Node %d has missing parent %d", n.id, n.parent);
			return false;
		}
	}
	Vector<DesignerNodeId> visiting;
	Vector<DesignerNodeId> visited;
	if(!ValidateVisit(Designer_ROOT, visiting, visited, error))
		return false;
	if(visited.GetCount() != nodes_.GetCount()) {
		error = "Model contains orphaned nodes";
		return false;
	}
	for(DesignerNodeId id : selection_) {
		if(!Find(id)) {
			error = Format("Selection references missing node %d", id);
			return false;
		}
	}
	return true;
}

void DesignerModel::SetVirtualSize(Size sz)
{
	virtual_size_ = Size(DesignerClampMin(sz.cx, DESIGNER_WINDOW_MIN_WIDTH),
	                     DesignerClampMin(sz.cy, DESIGNER_WINDOW_MIN_HEIGHT));
	WhenChanged();
}

}
