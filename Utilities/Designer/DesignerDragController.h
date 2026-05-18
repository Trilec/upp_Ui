#pragma once

#include "DesignerCommands.h"
#include "DesignerRegistry.h"

namespace Upp {

enum class DesignerDropZone {
	None,
	Into,
	Before,
	After
};

struct DesignerDropTarget : Moveable<DesignerDropTarget> {
	DesignerNodeId parent = Designer_NULL;
	int insert_index = -1;
	DesignerDropZone zone = DesignerDropZone::None;
	bool valid = false;
	String message;
};

enum class DesignerDragKind {
	None,
	Tool,
	Node
};

class DesignerDragController {
public:
	void BeginToolDrag(const String& type_id);
	void BeginNodeDrag(DesignerNodeId id);
	void UpdateTarget(const DesignerModel& model, const DesignerRegistry& registry,
	                  const DesignerDropTarget& target);
	void Cancel();

	bool IsActive() const { return kind_ != DesignerDragKind::None; }
	DesignerDragKind GetKind() const { return kind_; }
	const DesignerDropTarget& GetTarget() const { return target_; }
	String GetToolType() const { return tool_type_; }
	DesignerNodeId GetNodeId() const { return node_id_; }

	bool Drop(DesignerModel& model, DesignerCommandStack& commands);

private:
	DesignerDropTarget Validate(const DesignerModel& model, const DesignerRegistry& registry,
	                            const DesignerDropTarget& target) const;

	DesignerDragKind kind_ = DesignerDragKind::None;
	String tool_type_;
	DesignerNodeId node_id_ = Designer_NULL;
	DesignerDropTarget target_;
};

DesignerDropTarget DesignerMakeIntoTarget(DesignerNodeId parent, int insert_index = -1);
DesignerDropTarget DesignerMakeSiblingTarget(DesignerNodeId sibling, DesignerDropZone zone);

}
