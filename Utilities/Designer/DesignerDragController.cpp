#include "DesignerDragController.h"

namespace Upp {

DesignerDropTarget DesignerMakeIntoTarget(DesignerNodeId parent, int insert_index)
{
	DesignerDropTarget target;
	target.parent = parent;
	target.insert_index = insert_index;
	target.zone = DesignerDropZone::Into;
	return target;
}

DesignerDropTarget DesignerMakeSiblingTarget(DesignerNodeId sibling, DesignerDropZone zone)
{
	DesignerDropTarget target;
	target.parent = sibling;
	target.zone = zone;
	return target;
}

void DesignerDragController::BeginToolDrag(const String& type_id)
{
	kind_ = type_id.IsEmpty() ? DesignerDragKind::None : DesignerDragKind::Tool;
	tool_type_ = type_id;
	node_id_ = Designer_NULL;
	target_ = DesignerDropTarget();
}

void DesignerDragController::BeginNodeDrag(DesignerNodeId id)
{
	kind_ = id == Designer_NULL || id == Designer_ROOT ? DesignerDragKind::None : DesignerDragKind::Node;
	tool_type_.Clear();
	node_id_ = id;
	target_ = DesignerDropTarget();
}

void DesignerDragController::Cancel()
{
	kind_ = DesignerDragKind::None;
	tool_type_.Clear();
	node_id_ = Designer_NULL;
	target_ = DesignerDropTarget();
}

void DesignerDragController::UpdateTarget(const DesignerModel& model, const DesignerRegistry& registry,
                                          const DesignerDropTarget& target)
{
	target_ = Validate(model, registry, target);
}

DesignerDropTarget DesignerDragController::Validate(const DesignerModel& model, const DesignerRegistry& registry,
                                                    const DesignerDropTarget& target) const
{
	DesignerDropTarget out = target;
	out.valid = false;
	out.message.Clear();

	if(kind_ == DesignerDragKind::None) {
		out.message = "No active drag";
		return out;
	}

	DesignerNodeId parent_id = target.parent;
	int insert_index = target.insert_index;
	if(target.zone == DesignerDropZone::Before || target.zone == DesignerDropZone::After) {
		const DesignerNode* sibling = model.Find(target.parent);
		const DesignerNode* parent = sibling ? model.Find(sibling->parent) : nullptr;
		if(!sibling || !parent) {
			out.message = "Missing sibling target";
			return out;
		}
		parent_id = parent->id;
		insert_index = 0;
		for(int i = 0; i < parent->children.GetCount(); i++) {
			if(parent->children[i] == sibling->id) {
				insert_index = i + (target.zone == DesignerDropZone::After ? 1 : 0);
				break;
			}
		}
	}

	const DesignerNode* parent = model.Find(parent_id);
	if(!parent) {
		out.message = "Missing parent target";
		return out;
	}

	const DesignerType* parent_type = registry.Find(parent->type_id);
	if(!parent_type || !parent_type->can_have_children) {
		out.message = "Target is not a container";
		return out;
	}

	DesignerNode child;
	if(kind_ == DesignerDragKind::Tool) {
		const DesignerType* child_type = registry.Find(tool_type_);
		if(!child_type) {
			out.message = "Unknown toolbox type";
			return out;
		}
		child.type_id = tool_type_;
	}
	else {
		const DesignerNode* existing = model.Find(node_id_);
		if(!existing) {
			out.message = "Missing dragged node";
			return out;
		}
		if(existing->id == parent_id) {
			out.message = "Cannot drop a node into itself";
			return out;
		}
		for(const DesignerNode* p = parent; p; p = model.Find(p->parent)) {
			if(p->id == existing->id) {
				out.message = "Cannot drop a node into its descendant";
				return out;
			}
		}
		child.type_id = existing->type_id;
	}

	if(!registry.CanDrop(*parent, child)) {
		out.message = "Container rejects this type";
		return out;
	}

	out.parent = parent_id;
	out.insert_index = insert_index;
	out.zone = DesignerDropZone::Into;
	out.valid = true;
	out.message = "Drop allowed";
	return out;
}

bool DesignerDragController::Drop(DesignerModel& model, DesignerCommandStack& commands)
{
	if(!target_.valid)
		return false;

	bool ok = false;
	if(kind_ == DesignerDragKind::Tool)
		ok = commands.AddNode(model, tool_type_, target_.parent, target_.insert_index) != Designer_NULL;
	else
		ok = commands.Execute(MakeDesignerMoveNodeCommand(node_id_, target_.parent, target_.insert_index), model);
	Cancel();
	return ok;
}

}
