#pragma once

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    DesignerHierarchy
    =================

    Purpose
    - Public header for the DesignerHierarchy component.

    Intent
    - Define the runtime API, style contract, and integration points used by the rest of the Ui package.

    Thread context
    - GUI thread only.

    Usage
    - Include this header where the component is used or extended. Keep implementation details in the matching .cpp when present.

    Changelog
    - 2026-06: normalized the top-level header documentation.
*/

#include "DesignerModel.h"

// Ui Designer hierarchy/toolbox trees.
// Copyright (c) 2026 C Edwards (dodobar). MIT licensed, matching the Ui package.
//
// These are thin UiTree specializations that translate mouse gestures into
// designer drag events. They do not own model edits; the main window and drag
// controller decide whether a drop is valid and which command to execute.

namespace Upp {

// Toolbox tree used to start creation drags.
// Rows represent registered DesignerType entries, and drag events carry the type
// id plus screen position so preview and hierarchy can share one drop pipeline.
class DesignerToolboxTree : public UiTree {
public:
	Event<String, Point> WhenToolDrag;
	Event<String, Point> WhenToolDrop;
	Event<> WhenToolCancel;
	Event<String> WhenToolHover;

	void LeftDown(Point p, dword flags) override;
	void MouseMove(Point p, dword flags) override;
	void LeftDrag(Point p, dword flags) override;
	Image CursorImage(Point p, dword flags) override;
	void LeftUp(Point p, dword flags) override;
	void MouseLeave() override;
	void CancelMode() override;

private:
	void EmitHover(Point p);
	bool BeginFromSelection();
	void ResetDragState();
	void CancelDragNoRelease();

	String drag_type_;
	Point drag_start_;
	bool dragging_ = false;
};

// Model hierarchy tree used to select and move existing nodes.
// It preserves normal tree selection/expand behavior while adding external drop
// tracking for moves from preview or toolbox.
class DesignerHierarchyTree : public UiTree {
public:
	Event<DesignerNodeId, Point> WhenNodeDrag;
	Event<DesignerNodeId, UiTreeNodeRef, Point> WhenNodeDrop;
	Event<> WhenNodeCancel;

	UiTree::DropInfo TrackExternalDrop(Point p);

	void LeftDown(Point p, dword flags) override;
	void MouseMove(Point p, dword flags) override;
	void LeftDrag(Point p, dword flags) override;
	Image CursorImage(Point p, dword flags) override;
	void LeftUp(Point p, dword flags) override;
	void CancelMode() override;

private:
	bool BeginFromSelection();
	void PollDrag();
	void ResetDragState();
	void CancelDragNoRelease();

	DesignerNodeId drag_id_ = Designer_NULL;
	Point drag_start_;
	bool dragging_ = false;
};

}
