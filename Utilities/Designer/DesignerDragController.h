#ifndef _Utilities_DesignerDragController_h_
#define _Utilities_DesignerDragController_h_

/*

    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    DesignerDragController
    ======================

    Purpose
    - Public header for the DesignerDragController component.
     This is the one owner for drag intent. Toolbox, hierarchy, and preview can
     report hover targets, but validation and final model edits should pass through
     this controller so drag behavior stays consistent.

    Intent
    - Define the runtime API, style contract, and integration points used by the rest of the Ui package.

    Thread context
    - GUI thread only.

    Usage
    - Include this header where the component is used or extended. Keep implementation details in the matching .cpp when present.

    Changelog
    - 2026-06: normalized the top-level header documentation.
*/

#include "DesignerCommands.h"
#include "DesignerRegistry.h"

namespace Upp {

// Semantic drop zone used when dragging around tree-like targets.
// The preview mostly maps hit geometry into an Into target plus insert index,
// while the hierarchy can still express before/after sibling intent.
enum class DesignerDropZone {
	None,
	Into,
	Before,
	After
};

// Validated destination for a drag operation.
// parent/insert_index are intentionally model-level concepts: dropping changes
// the tree first, then preview and hierarchy are rebuilt from that model.
struct DesignerDropTarget : Moveable<DesignerDropTarget> {
	DesignerNodeId parent = Designer_NULL;
	int insert_index = -1;
	DesignerDropZone zone = DesignerDropZone::None;
	bool valid = false;
	String message;
};

// Active drag source kind.
// Tool drags create new nodes; node drags move existing model subtrees.
enum class DesignerDragKind {
	None,
	Tool,
	Node
};

// State machine for one drag interaction.
// BeginToolDrag/BeginNodeDrag start a gesture, UpdateTarget validates the current
// hover destination, and Drop emits the command when the mouse is released.
class DesignerDragController {
public:
	void BeginToolDrag(const String& type_id);
	void BeginNodeDrag(DesignerNodeId id);
	void BeginNodeDrag(const Vector<DesignerNodeId>& ids);
	void UpdateTarget(const DesignerModel& model, const DesignerRegistry& registry,
	                  const DesignerDropTarget& target);
	void Cancel();

	bool IsActive() const { return kind_ != DesignerDragKind::None; }
	DesignerDragKind GetKind() const { return kind_; }
	const DesignerDropTarget& GetTarget() const { return target_; }
	String GetToolType() const { return tool_type_; }
	DesignerNodeId GetNodeId() const { return node_id_; }
	const Vector<DesignerNodeId>& GetNodeIds() const { return node_ids_; }

	bool Drop(DesignerModel& model, DesignerCommandStack& commands);

private:
	DesignerDropTarget Validate(const DesignerModel& model, const DesignerRegistry& registry,
	                            const DesignerDropTarget& target) const;

	DesignerDragKind kind_ = DesignerDragKind::None;
	String tool_type_;
	DesignerNodeId node_id_ = Designer_NULL;
	Vector<DesignerNodeId> node_ids_;
	DesignerDropTarget target_;
};

DesignerDropTarget DesignerMakeIntoTarget(DesignerNodeId parent, int insert_index = -1);
DesignerDropTarget DesignerMakeSiblingTarget(DesignerNodeId sibling, DesignerDropZone zone);

}

#endif
