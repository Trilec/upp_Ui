#pragma once

#include "DesignerModel.h"

namespace Upp {

class DesignerToolboxTree : public UiTree {
public:
	Event<String, Point> WhenToolDrag;
	Event<String, Point> WhenToolDrop;
	Event<> WhenToolCancel;

	void LeftDown(Point p, dword flags) override;
	void MouseMove(Point p, dword flags) override;
	void LeftDrag(Point p, dword flags) override;
	Image CursorImage(Point p, dword flags) override;
	void LeftUp(Point p, dword flags) override;
	void CancelMode() override;

private:
	bool BeginFromSelection();
	void ResetDragState();
	void CancelDragNoRelease();

	String drag_type_;
	Point drag_start_;
	bool dragging_ = false;
};

class DesignerHierarchyTree : public UiTree {
public:
	Event<DesignerNodeId, Point> WhenNodeDrag;
	Event<DesignerNodeId, UiTreeNodeRef, Point> WhenNodeDrop;
	Event<> WhenNodeCancel;

	UiTreeNodeRef TrackExternalDrop(Point p);

	void LeftDown(Point p, dword flags) override;
	void MouseMove(Point p, dword flags) override;
	void LeftDrag(Point p, dword flags) override;
	Image CursorImage(Point p, dword flags) override;
	void LeftUp(Point p, dword flags) override;
	void CancelMode() override;

private:
	bool BeginFromSelection();
	void ResetDragState();
	void CancelDragNoRelease();

	DesignerNodeId drag_id_ = Designer_NULL;
	Point drag_start_;
	bool dragging_ = false;
};

}
