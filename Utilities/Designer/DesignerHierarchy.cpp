#include "DesignerHierarchy.h"

// DesignerHierarchy.cpp - thin UiTree gesture wrappers for toolbox/hierarchy.
// These controls keep normal tree behavior but expose drag events to the shared
// designer drag controller instead of mutating the model directly.

namespace Upp {

void DesignerToolboxTree::LeftDown(Point p, dword flags)
{
	UiTree::LeftDown(p, flags);
	ResetDragState();
	if(BeginFromSelection()) {
		drag_start_ = p;
		SetCapture();
	}
}

void DesignerToolboxTree::MouseMove(Point p, dword flags)
{
	EmitHover(p);

	if(drag_type_.IsEmpty()) {
		UiTree::MouseMove(p, flags);
		return;
	}

	if(!(flags & K_MOUSELEFT)) {
		CancelDragNoRelease();
		return;
	}

	if(!dragging_ && Length(p - drag_start_) > DPI(4))
		dragging_ = true;

	if(dragging_)
		WhenToolDrag(drag_type_, GetMousePos());
}

void DesignerToolboxTree::LeftDrag(Point p, dword flags)
{
	if(drag_type_.IsEmpty()) {
		drag_start_ = p;
		BeginFromSelection();
		if(!drag_type_.IsEmpty() && !HasCapture())
			SetCapture();
	}
	MouseMove(p, flags | K_MOUSELEFT);
}

Image DesignerToolboxTree::CursorImage(Point p, dword flags)
{
	return dragging_ ? Image::SizeAll() : UiTree::CursorImage(p, flags);
}

void DesignerToolboxTree::MouseLeave()
{
	UiTree::MouseLeave();
	WhenToolHover(String());
}

void DesignerToolboxTree::LeftUp(Point p, dword flags)
{
	String type = drag_type_;
	bool was_dragging = dragging_;

	ResetDragState();

	if(was_dragging && !type.IsEmpty())
		WhenToolDrop(type, GetMousePos());
	else {
		UiTree::LeftUp(p, flags);
		WhenToolCancel();
	}

	if(HasCapture())
		ReleaseCapture();
}

void DesignerToolboxTree::EmitHover(Point p)
{
	UiTreeNodeRef ref = GetNodeAt(p);
	if(!GetModel().IsValid(ref)) {
		WhenToolHover(String());
		return;
	}

	Value v = GetModel().Get(ref).data;
	WhenToolHover(IsString(v) ? (String)v : String());
}

void DesignerToolboxTree::CancelMode()
{
	CancelDragNoRelease();
	UiTree::CancelMode();
}

bool DesignerToolboxTree::BeginFromSelection()
{
	Value v = GetData();
	if(!IsString(v))
		return false;
	drag_type_ = (String)v;
	return !drag_type_.IsEmpty();
}

void DesignerToolboxTree::ResetDragState()
{
	drag_type_.Clear();
	dragging_ = false;
}

void DesignerToolboxTree::CancelDragNoRelease()
{
	bool active = !drag_type_.IsEmpty() || dragging_;
	ResetDragState();
	if(active)
		WhenToolCancel();
}

UiTreeNodeRef DesignerHierarchyTree::TrackExternalDrop(Point p)
{
	UiTree::MouseMove(p, 0);
	Refresh();
	return GetHotNode();
}

void DesignerHierarchyTree::LeftDown(Point p, dword flags)
{
	UiTree::LeftDown(p, flags);
	BeginFromSelection();
	drag_start_ = p;
	dragging_ = false;
}

void DesignerHierarchyTree::MouseMove(Point p, dword flags)
{
	UiTree::MouseMove(p, flags);

	if(drag_id_ == Designer_NULL || drag_id_ == Designer_ROOT)
		return;

	if(!(flags & K_MOUSELEFT)) {
		CancelDragNoRelease();
		return;
	}

	if(!dragging_ && Length(p - drag_start_) > DPI(4)) {
		dragging_ = true;
		SetCapture();
	}

	if(dragging_)
		WhenNodeDrag(drag_id_, GetMousePos());
}

void DesignerHierarchyTree::LeftDrag(Point p, dword flags)
{
	if(drag_id_ == Designer_NULL) {
		drag_start_ = p;
		BeginFromSelection();
	}
	MouseMove(p, flags | K_MOUSELEFT);
}

Image DesignerHierarchyTree::CursorImage(Point p, dword flags)
{
	return dragging_ ? Image::SizeAll() : UiTree::CursorImage(p, flags);
}

void DesignerHierarchyTree::LeftUp(Point p, dword flags)
{
	DesignerNodeId id = drag_id_;
	bool was_dragging = dragging_;
	UiTreeNodeRef hot = GetHotNode();
	Point screen = GetMousePos();

	ResetDragState();

	if(was_dragging && id != Designer_NULL && id != Designer_ROOT)
		WhenNodeDrop(id, hot, screen);
	else {
		UiTree::LeftUp(p, flags);
		WhenNodeCancel();
	}

	if(HasCapture())
		ReleaseCapture();
}

void DesignerHierarchyTree::CancelMode()
{
	CancelDragNoRelease();
	UiTree::CancelMode();
}

bool DesignerHierarchyTree::BeginFromSelection()
{
	Value v = GetData();
	drag_id_ = IsNumber(v) ? (int)v : Designer_NULL;
	return drag_id_ != Designer_NULL && drag_id_ != Designer_ROOT;
}

void DesignerHierarchyTree::ResetDragState()
{
	drag_id_ = Designer_NULL;
	dragging_ = false;
}

void DesignerHierarchyTree::CancelDragNoRelease()
{
	bool active = drag_id_ != Designer_NULL || dragging_;
	ResetDragState();
	if(active)
		WhenNodeCancel();
}

}
