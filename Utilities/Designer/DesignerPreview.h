#pragma once

#include "DesignerAdapter.h"
#include "DesignerBuiltins.h"

// Ui Designer preview surface.
// Copyright (c) 2026 C Edwards (dodobar). MIT licensed, matching the Ui package.
//
// DesignerPreview is the central visual editing canvas. It renders a virtual
// window, builds real Ui controls through adapters, tracks selection/drop hover,
// and maps pointer gestures back into model-level node moves.

namespace Upp {

// Virtual-window preview and interaction layer.
// The preview is not the model: it rebuilds from DesignerModel, records transient
// rectangles for hit testing, and emits events for selection, resize, and moves.
class DesignerPreview : public Ctrl {
public:
		Event<DesignerNodeId> WhenSelect;
		Event<> WhenChanged;
		Event<DesignerNodeId, DesignerNodeId, int> WhenMoveNode;
		void Set(DesignerModel* model, DesignerRegistry* registry);
		void SetThemeMode(UiThemeMode mode);
		void SyncRealPreview();
		void InvalidateRealPreview();
		void SetPlacementType(const String& type_id);
		DesignerNodeId TrackPlacement(Point p);
		int GetDropIndex() const;
		void Layout() override;
		void Paint(Draw& w) override;
		void LeftDown(Point p, dword) override;
		void MouseMove(Point p, dword) override;
		void ClearDropState();
		void CancelMode() override;
		Image CursorImage(Point p, dword) override;
		void LeftUp(Point p, dword) override;

private:
		Rect GetVirtualWindowRect() const;
		Rect GetResizeHandle(const Rect& root) const;
		void DrawResizeHandle(Draw& w, const Rect& root);
		void DrawDropIndicator(Draw& w, const Rect& root);
		Rect GetInsertMarkerRect(const DesignerNode& parent, const Rect& root) const;
		int GetSplitterPaneIndex(const DesignerNode& parent, Point p) const;
		Rect GetSplitterPaneRect(const DesignerNode& parent, int pane) const;
		String GetSplitterPaneName(const DesignerNode& parent, int pane) const;
		void DrawRoundedOutline(Draw& w, const Rect& r, Color c, int radius, int width);
		void DrawDashed(Draw& w, const Rect& r, Color c);
		void PaintNode(Draw& w, const DesignerNode& n, const Rect& r, int depth);
		void DrawLayoutDebug(Draw& w, const DesignerNode& n, Rect content);
		void PaintChildren(Draw& w, const DesignerNode& parent, Rect area, int depth);
		Size GetNodePreviewSize(const DesignerNode& n) const;
		void PaintBoxChildren(Draw& w, const DesignerNode& parent, Rect area, int depth);
		void PaintFlowGridChildren(Draw& w, const DesignerNode& parent, Rect area, int depth);
		void PaintGridChildren(Draw& w, const DesignerNode& parent, Rect area, int depth);
		DesignerNodeId Hit(Point p) const;
		int GetNodeDepth(DesignerNodeId id) const;
		DesignerNodeId ResolveDropTarget(DesignerNodeId hit) const;
		int FindChildIndex(const DesignerNode& parent, DesignerNodeId child) const;
		void ResetDropState();
		void UpdateDropSlot(Point p);
		bool UseHorizontalInsert(const DesignerNode& parent) const;
		Ctrl* BuildRealNode(DesignerNodeId id);
		void AddRealChild(DesignerAdapter& parent, Ctrl& child, const DesignerNode& parent_node,
		                  const DesignerNode& child_node, int index);
		void FinalizeRealNode(DesignerAdapter& adapter, const DesignerNode& node);
		void ApplyRealLayoutProperties(Ctrl& ctrl);
		void RebuildRealPreview();
		void LayoutRealPreview();
		void RefreshRealPreviewTree(Ctrl& ctrl);
		void UpdateRealRects(Ctrl& ctrl, Point offset);
		void ApplyRealOverlay();
		DesignerModel* model_ = nullptr;
		DesignerRegistry* registry_ = nullptr;
		Array<Ctrl> real_controls_;
		VectorMap<DesignerNodeId, DesignerAdapter*> real_adapters_;
		bool real_dirty_ = true;
		bool rebuilding_real_ = false;
		UiThemeMode theme_mode_ = UiThemeMode::Light;
		String placement_type_;
		bool resizing_ = false;
		DesignerNodeId drag_candidate_ = Designer_NULL;
		DesignerNodeId drop_target_ = Designer_NULL;
		int drop_index_ = -1;
		Point drag_start_;
		bool dragging_node_ = false;
};

}
