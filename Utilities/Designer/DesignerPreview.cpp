#include "DesignerPreview.h"
#include "DesignerDefaults.h"

// DesignerPreview.cpp - virtual-window preview and pointer interaction surface.
// It rebuilds real Ui controls through adapters, records hit rectangles for the
// model, and turns drag/release gestures into model-level move events.

namespace Upp {

static int DesignerPreviewFindNodeId(const Vector<DesignerNodeId>& ids, DesignerNodeId id)
{
	for(int i = 0; i < ids.GetCount(); i++)
		if(ids[i] == id)
			return i;
	return -1;
}

static Value DesignerPreviewNodeProperty(const DesignerNode& n, const String& key, const Value& def)
{
	int q = n.properties.Find(key);
	return q >= 0 ? n.properties.GetValue(q) : def;
}

static String DesignerPreviewAxisSizing(const DesignerNode& n, const String& axis_key)
{
	return DesignerPreviewNodeProperty(n, axis_key, "Fit");
}

static Color DesignerPreviewBackground(UiThemeMode mode)
{
	return mode == UiThemeMode::Dark ? Color(32, 32, 32) : Color(246, 248, 251);
}

static bool DesignerPreviewIsLayoutType(const DesignerType *t)
{
	return t && (t->toolbox_group == "Layouts" || t->id == "Window");
}

static bool DesignerPreviewIsPanelType(const DesignerType *t)
{
	return t && (t->toolbox_group == "Containers" || t->id == "PaneSlot" || t->id == "PageSlot");
}

static bool DesignerPreviewIsPageContainer(const DesignerNode& n)
{
	return n.type_id == "UiTab" || n.type_id == "UiStack";
}

static DesignerNodeId DesignerPreviewActivePageSlot(const DesignerNode& n)
{
	if(!DesignerPreviewIsPageContainer(n) || n.children.IsEmpty())
		return Designer_NULL;
	int active = clamp((int)DesignerPreviewNodeProperty(n, "active", 0), 0, n.children.GetCount() - 1);
	return n.children[active];
}

static Color DesignerPreviewCategoryFace(const DesignerType *t, UiThemeMode mode)
{
	bool dark = mode == UiThemeMode::Dark;
	if(DesignerPreviewIsLayoutType(t))
		return dark ? Color(88, 59, 31) : Color(255, 224, 178);
	if(DesignerPreviewIsPanelType(t))
		return dark ? Color(34, 78, 54) : Color(187, 232, 203);
	return dark ? Color(42, 68, 104) : Color(203, 224, 255);
}

static Color DesignerPreviewCategoryFrame(const DesignerType *t, UiThemeMode mode)
{
	bool dark = mode == UiThemeMode::Dark;
	if(DesignerPreviewIsLayoutType(t))
		return dark ? Color(245, 158, 66) : Color(217, 119, 6);
	if(DesignerPreviewIsPanelType(t))
		return dark ? Color(74, 222, 128) : Color(34, 150, 91);
	return dark ? Color(96, 165, 250) : Color(54, 116, 210);
}

static Color DesignerPreviewWindowOutline(Color base, UiThemeMode mode)
{
	Color paper = DesignerPreviewBackground(mode);
	return Blend(base, paper, 190);
}

static UiGridLayout::Align DesignerPreviewGridAlignH(const DesignerNode& n)
{
	String align = DesignerPreviewNodeProperty(n, "cell_align_h", "Auto");
	if(align == "Auto")
		align = DesignerPreviewNodeProperty(n, "align_h", DesignerPreviewNodeProperty(n, "align", "Left"));
	if(align == "Right")
		return UiGridLayout::Align::End;
	if(align == "Center")
		return UiGridLayout::Align::Center;
	return UiGridLayout::Align::Start;
}

static UiGridLayout::Align DesignerPreviewGridAlignV(const DesignerNode& n)
{
	String align = DesignerPreviewNodeProperty(n, "cell_align_v", "Auto");
	if(align == "Auto")
		align = DesignerPreviewNodeProperty(n, "align_v", "Top");
	if(align == "Bottom")
		return UiGridLayout::Align::End;
	if(align == "Center")
		return UiGridLayout::Align::Center;
	return UiGridLayout::Align::Start;
}

void DesignerPreview::Set(DesignerModel* model, DesignerRegistry* registry)
{
			model_ = model;
			registry_ = registry;
			real_dirty_ = true;
			SyncRealPreview();
			Refresh();
		}

void DesignerPreview::SetThemeMode(UiThemeMode mode)
{
			theme_mode_ = mode;
			real_dirty_ = true;
			SyncRealPreview();
			Refresh();
		}

void DesignerPreview::SyncRealPreview()
{
			real_dirty_ = true;
			RebuildRealPreview();
		}

void DesignerPreview::InvalidateRealPreview()
{
			real_dirty_ = true;
			RebuildRealPreview();
			LayoutRealPreview();
			for(Ctrl *child = GetFirstChild(); child; child = child->GetNext())
				RefreshRealPreviewTree(*child);
			Refresh();
		}

void DesignerPreview::SetPlacementType(const String& type_id)
{
			placement_type_ = type_id;
			if(type_id.IsEmpty())
				drop_target_ = Designer_NULL;
			Refresh();
		}

DesignerNodeId DesignerPreview::TrackPlacement(Point p)
{
			UpdateDropSlot(p);
			Refresh();
			return drop_target_;
		}

int DesignerPreview::GetDropIndex() const
{ return drop_index_; }

void DesignerPreview::Layout()
{
			LayoutRealPreview();
		}

void DesignerPreview::Paint(Draw& w)
{
			w.DrawRect(GetSize(), DesignerPreviewBackground(theme_mode_));
			if(!model_)
				return;
			LayoutRealPreview();
			Rect root = GetVirtualWindowRect();
			Size vsz = model_->GetVirtualSize();
			DrawRoundedOutline(w, root, DesignerPreviewWindowOutline(SColorHighlight(), theme_mode_), DPI(8), DPI(4));
			DrawDropIndicator(w, root);
			DrawResizeHandle(w, root);
			if(!placement_type_.IsEmpty())
				w.DrawText(root.left + DPI(10), root.bottom + DPI(8),
				           "Release over a highlighted layout or insert line to place " + placement_type_,
				           SansSerifZ(9), SColorHighlight());
		}

void DesignerPreview::LeftDown(Point p, dword)
{
			SetFocus();
			Rect root = GetVirtualWindowRect();
			if(GetResizeHandle(root).Contains(p)) {
				resizing_ = true;
				SetCapture();
				return;
			}
			DesignerNodeId id = Hit(p);
			drag_candidate_ = id;
			drag_start_ = p;
			dragging_node_ = false;
			WhenSelect(id ? id : Designer_ROOT);
		}

void DesignerPreview::MouseMove(Point p, dword)
{
			Rect root = GetVirtualWindowRect();
			if(resizing_) {
				model_->SetVirtualSize(Size(max(DPI(DESIGNER_WINDOW_MIN_WIDTH), p.x - root.left),
				                            max(DPI(DESIGNER_WINDOW_MIN_HEIGHT), p.y - root.top)));
				WhenChanged();
				Refresh();
				return;
			}
			if(!GetMouseLeft() && dragging_node_) {
				ClearDropState();
				Refresh();
				return;
			}
			if(!placement_type_.IsEmpty() && !GetMouseLeft()) {
				SetPlacementType(String());
				return;
			}
			if(!placement_type_.IsEmpty()) {
				UpdateDropSlot(p);
				Refresh();
				return;
			}
			if(drag_candidate_ && drag_candidate_ != Designer_ROOT) {
				if(!dragging_node_ && Length(p - drag_start_) > DPI(4)) {
					dragging_node_ = true;
					SetCapture();
				}
				if(dragging_node_)
					UpdateDropSlot(p);
				Refresh();
			}
		}

void DesignerPreview::ClearDropState()
{
			ResetDropState();
			if(HasCapture())
				ReleaseCapture();
		}

void DesignerPreview::CancelMode()
{
			ResetDropState();
			Ctrl::CancelMode();
		}

Image DesignerPreview::CursorImage(Point p, dword)
{
			return GetResizeHandle(GetVirtualWindowRect()).Contains(p) ? Image::SizeAll() : Image::Arrow();
		}

void DesignerPreview::LeftUp(Point p, dword)
{
			if(resizing_) {
				resizing_ = false;
				ReleaseCapture();
				WhenChanged();
				return;
			}
			if(dragging_node_) {
				UpdateDropSlot(p);
				DesignerNodeId drag = drag_candidate_;
				DesignerNodeId target = drop_target_;
				int index = drop_index_;
				ResetDropState();
				if(HasCapture())
					ReleaseCapture();
				if(drag && target)
					WhenMoveNode(drag, target, index);
				Refresh();
			}
		}

Rect DesignerPreview::GetVirtualWindowRect() const
{
			Rect root = Rect(GetSize()).Deflated(DPI(30));
			if(!model_)
				return root;
			Size vsz = model_->GetVirtualSize();
			return RectC(root.left, root.top, min(root.GetWidth(), vsz.cx), min(root.GetHeight(), vsz.cy));
		}

Rect DesignerPreview::GetResizeHandle(const Rect& root) const
{
			return RectC(root.right - DPI(8), root.bottom - DPI(8), DPI(16), DPI(16));
		}

void DesignerPreview::DrawResizeHandle(Draw& w, const Rect& root)
{
			Rect h = GetResizeHandle(root);
			w.DrawEllipse(h, SColorHighlight(), DPI(1), SColorHighlight());
		}

void DesignerPreview::DrawDropIndicator(Draw& w, const Rect& root)
{
			if((!dragging_node_ && placement_type_.IsEmpty()) || !drop_target_)
				return;
			const DesignerNode* target = model_->Find(drop_target_);
			if(!target)
				return;
			Rect r = drop_target_ == Designer_ROOT ? root : target->last_rect;
			if(r.IsEmpty())
				return;
			Color c = Color(255, 191, 0);
			auto DrawFrame = [&](Rect q, int thick) {
				if(q.IsEmpty())
					return;
				w.DrawRect(q.left, q.top, q.GetWidth(), thick, c);
				w.DrawRect(q.left, q.bottom - thick, q.GetWidth(), thick, c);
				w.DrawRect(q.left, q.top, thick, q.GetHeight(), c);
				w.DrawRect(q.right - thick, q.top, thick, q.GetHeight(), c);
			};
			DrawFrame(r, DPI(2));
			Rect marker = GetInsertMarkerRect(*target, root);
			if(target->type_id == "UiSplitter" || target->type_id == "UiQuadSplitter" || target->type_id == "GridLayout")
				DrawFrame(marker, DPI(3));
			else if(!marker.IsEmpty())
				w.DrawRect(marker, c);
			String verb = !placement_type_.IsEmpty() ? "Insert in " : "Move in ";
			String target_name = drop_target_ == Designer_ROOT ? "Window" : target->name;
			if(target->type_id == "UiSplitter" || target->type_id == "UiQuadSplitter")
				target_name << " / " << GetSplitterPaneName(*target, drop_index_);
			String label = verb + target_name + Format(" at %d", max(0, drop_index_));
			Font tag_font = SansSerifZ(9);
			Size tsz = GetTextSize(label, tag_font);
			Rect tag = RectC(r.left + DPI(8), max(root.top + DPI(6), r.top - DPI(22)), tsz.cx + DPI(12), DPI(20));
			w.DrawRect(tag, c);
			w.DrawText(tag.left + DPI(6), tag.top + DPI(4), label, tag_font, Black());
		}

Rect DesignerPreview::GetInsertMarkerRect(const DesignerNode& parent, const Rect& root) const
{
			if(parent.type_id == "UiSplitter" || parent.type_id == "UiQuadSplitter")
				return GetSplitterPaneRect(parent, drop_index_);
			Rect pr = GetContainerContentRect(parent, root);
			if(pr.IsEmpty())
				return Rect(0, 0, 0, 0);
			if(parent.type_id == "GridLayout") {
				int gap = (int)DesignerPreviewNodeProperty(parent, "gap", DPI(8));
				int inset = (int)DesignerPreviewNodeProperty(parent, "inset", 0);
				Rect grid = pr.Deflated(inset);
				if(grid.IsEmpty())
					return Rect(0, 0, 0, 0);
				int columns = max(1, (int)DesignerPreviewNodeProperty(parent, "columns", 2));
				int rows = max(1, (int)DesignerPreviewNodeProperty(parent, "rows", 2));
				int cw = max(DPI(DESIGNER_GRID_CELL_WIDTH), (grid.GetWidth() - gap * (columns - 1)) / columns);
				int ch = max(DPI(DESIGNER_GRID_CELL_HEIGHT), (grid.GetHeight() - gap * (rows - 1)) / rows);
				int idx = clamp(drop_index_, 0, columns * rows - 1);
				int row = idx / columns;
				int col = idx % columns;
				int y = grid.top + row * (ch + gap);
				int x = grid.left + col * (cw + gap);
				return RectC(x, y, cw, ch);
			}
			int count = parent.children.GetCount();
			if(count <= 0)
				return RectC(pr.left, pr.top, pr.GetWidth(), DPI(4));
			int idx = clamp(drop_index_, 0, count);
			bool horizontal = DesignerPreviewNodeProperty(parent, "direction", "V") == "H";
			if(horizontal) {
				int x;
				if(idx <= 0) {
					const DesignerNode* first = model_->Find(parent.children[0]);
					x = first ? first->last_rect.left - DPI(4) : pr.left;
				}
				else if(idx >= count) {
					const DesignerNode* last = model_->Find(parent.children[count - 1]);
					x = last ? last->last_rect.right + DPI(4) : pr.right;
				}
				else {
					const DesignerNode* prev = model_->Find(parent.children[idx - 1]);
					const DesignerNode* next = model_->Find(parent.children[idx]);
					x = prev && next ? (prev->last_rect.right + next->last_rect.left) / 2 : pr.left;
				}
				return RectC(x - DPI(2), pr.top, DPI(4), pr.GetHeight());
			}
			int y;
			if(idx <= 0) {
				const DesignerNode* first = model_->Find(parent.children[0]);
				y = first ? first->last_rect.top - DPI(4) : pr.top;
			}
			else if(idx >= count) {
				const DesignerNode* last = model_->Find(parent.children[count - 1]);
				y = last ? last->last_rect.bottom + DPI(4) : pr.bottom;
			}
			else {
				const DesignerNode* prev = model_->Find(parent.children[idx - 1]);
				const DesignerNode* next = model_->Find(parent.children[idx]);
				y = prev && next ? (prev->last_rect.bottom + next->last_rect.top) / 2 : pr.top;
			}
			return RectC(pr.left, y - DPI(2), pr.GetWidth(), DPI(4));
		}

Rect DesignerPreview::GetContainerContentRect(const DesignerNode& parent, const Rect& root) const
{
			if(parent.id == Designer_ROOT)
				return root;
			if(parent.type_id == "UiPanel" || parent.type_id == "UiScrollPanel")
				return parent.last_rect.Deflated(DPI(4));
			return parent.last_rect.Deflated(DPI(10), DPI(24), DPI(10), DPI(10));
		}

int DesignerPreview::GetSplitterPaneIndex(const DesignerNode& parent, Point p) const
{
			Rect r = parent.last_rect.Deflated(DPI(10), DPI(24), DPI(10), DPI(10));
			if(r.IsEmpty())
				r = parent.last_rect;
			if(parent.type_id == "UiQuadSplitter") {
				int split_x = r.left + r.GetWidth() * (int)DesignerPreviewNodeProperty(parent, "column_percent", 50) / 100;
				int split_y = r.top + r.GetHeight() * (int)DesignerPreviewNodeProperty(parent, "row_percent", 50) / 100;
				return (p.y >= split_y ? 2 : 0) + (p.x >= split_x ? 1 : 0);
			}
			bool vertical = DesignerPreviewNodeProperty(parent, "direction", "H") == "V";
			int split = vertical
			          ? r.top + r.GetHeight() * (int)DesignerPreviewNodeProperty(parent, "split_percent", 50) / 100
			          : r.left + r.GetWidth() * (int)DesignerPreviewNodeProperty(parent, "split_percent", 50) / 100;
			return (vertical ? p.y >= split : p.x >= split) ? 1 : 0;
		}

Rect DesignerPreview::GetSplitterPaneRect(const DesignerNode& parent, int pane) const
{
			Rect r = parent.last_rect.Deflated(DPI(10), DPI(24), DPI(10), DPI(10));
			if(r.IsEmpty())
				r = parent.last_rect;
			if(r.IsEmpty())
				return r;
			if(parent.type_id == "UiQuadSplitter") {
				int split_x = r.left + r.GetWidth() * (int)DesignerPreviewNodeProperty(parent, "column_percent", 50) / 100;
				int split_y = r.top + r.GetHeight() * (int)DesignerPreviewNodeProperty(parent, "row_percent", 50) / 100;
				switch(clamp(pane, 0, 3)) {
				case 0: return Rect(r.left, r.top, split_x, split_y).Deflated(DPI(2));
				case 1: return Rect(split_x, r.top, r.right, split_y).Deflated(DPI(2));
				case 2: return Rect(r.left, split_y, split_x, r.bottom).Deflated(DPI(2));
				default: return Rect(split_x, split_y, r.right, r.bottom).Deflated(DPI(2));
				}
			}
			bool vertical = DesignerPreviewNodeProperty(parent, "direction", "H") == "V";
			int split = vertical
			          ? r.top + r.GetHeight() * (int)DesignerPreviewNodeProperty(parent, "split_percent", 50) / 100
			          : r.left + r.GetWidth() * (int)DesignerPreviewNodeProperty(parent, "split_percent", 50) / 100;
			if(vertical)
				return (pane <= 0 ? Rect(r.left, r.top, r.right, split) : Rect(r.left, split, r.right, r.bottom)).Deflated(DPI(2));
			return (pane <= 0 ? Rect(r.left, r.top, split, r.bottom) : Rect(split, r.top, r.right, r.bottom)).Deflated(DPI(2));
		}

String DesignerPreview::GetSplitterPaneName(const DesignerNode& parent, int pane) const
{
			if(parent.type_id == "UiQuadSplitter") {
				static const char *name[] = { "Top left pane", "Top right pane", "Bottom left pane", "Bottom right pane" };
				return name[clamp(pane, 0, 3)];
			}
			bool vertical = DesignerPreviewNodeProperty(parent, "direction", "H") == "V";
			if(vertical)
				return pane <= 0 ? "Top pane" : "Bottom pane";
			return pane <= 0 ? "Left pane" : "Right pane";
		}

void DesignerPreview::DrawDashed(Draw& w, const Rect& r, Color c)
{
			if(r.IsEmpty())
				return;
			StyledPalette pal;
			pal.frame[ST_NORMAL] = c;
			StyledMetrics m;
			m.face_enabled = false;
			m.frame_enabled = true;
			m.frame_width = DPI(1);
			m.radius = DPI(8);
			m.dashed = true;
			m.dash_pattern = "4,4";
			UiPaintFaceFrameDash(w, r.Deflated(DPI(2)), pal, m, ST_NORMAL);
		}

void DesignerPreview::PaintNode(Draw& w, const DesignerNode& n, const Rect& r, int depth)
{
			const DesignerType* t = registry_ ? registry_->Find(n.type_id) : nullptr;
			bool selected = DesignerPreviewFindNodeId(model_->GetSelection(), n.id) >= 0;
			Color default_face = DesignerPreviewCategoryFace(t, theme_mode_);
			Color default_frame = DesignerPreviewCategoryFrame(t, theme_mode_);
			Color face = DesignerPreviewNodeProperty(n, "face", default_face);
			Color frame = DesignerPreviewNodeProperty(n, "frame", default_frame);
			bool face_enabled = (bool)DesignerPreviewNodeProperty(n, "face_enabled", true);
			bool frame_enabled = (bool)DesignerPreviewNodeProperty(n, "frame_enabled", true);
			int radius = min((int)DesignerPreviewNodeProperty(n, "radius", 0), min(r.GetWidth(), r.GetHeight()) / 2);
			if(radius > 0) {
				ImageBuffer ib(r.GetSize());
				Fill(~ib, RGBAZero(), ib.GetLength());
				BufferPainter p(ib, MODE_ANTIALIASED);
				p.Begin();
				p.RoundedRectangle(0.5, 0.5, r.GetWidth() - 1.0, r.GetHeight() - 1.0, radius);
				if(face_enabled)
					p.Fill(face);
				if(frame_enabled)
					p.Stroke(1.0, frame);
				p.End();
				w.DrawImage(r.left, r.top, ib);
			}
			else {
				if(face_enabled)
					w.DrawRect(r, face);
				if(frame_enabled) {
					w.DrawRect(r.left, r.top, r.GetWidth(), DPI(1), frame);
					w.DrawRect(r.left, r.bottom - DPI(1), r.GetWidth(), DPI(1), frame);
					w.DrawRect(r.left, r.top, DPI(1), r.GetHeight(), frame);
					w.DrawRect(r.right - DPI(1), r.top, DPI(1), r.GetHeight(), frame);
				}
			}
			if(selected)
				DrawDashed(w, r, SColorHighlight());
			String label = (t ? t->display_name : n.type_id) + ": " + n.name;
			w.DrawText(r.left + DPI(6), r.top + DPI(4), label, SansSerifZ(9).Bold(), SColorText());
			if(t && t->is_container) {
				Rect content = GetContainerContentRect(n, r);
				PaintChildren(w, n, content, depth + 1);
				DrawLayoutDebug(w, n, content);
			}
		}

void DesignerPreview::DrawLayoutDebug(Draw& w, const DesignerNode& n, Rect content)
{
			if(!(bool)DesignerPreviewNodeProperty(n, "debug", false))
				return;
			Color c = (bool)DesignerPreviewNodeProperty(n, "debug_auto_color", false)
			          ? Color(217, 119, 6)
			          : (Color)DesignerPreviewNodeProperty(n, "debug_color", Color(220, 38, 38));
			int inset = (int)DesignerPreviewNodeProperty(n, "inset", 0);
			Rect inner = content.Deflated(inset);
			w.DrawRect(inner.left, inner.top, inner.GetWidth(), DPI(1), c);
			w.DrawRect(inner.left, inner.bottom - DPI(1), inner.GetWidth(), DPI(1), c);
			w.DrawRect(inner.left, inner.top, DPI(1), inner.GetHeight(), c);
			w.DrawRect(inner.right - DPI(1), inner.top, DPI(1), inner.GetHeight(), c);
			if(n.type_id != "GridLayout")
				return;
			int gap = (int)DesignerPreviewNodeProperty(n, "gap", DPI(8));
			int columns = max(1, (int)DesignerPreviewNodeProperty(n, "columns", 2));
			int rows = max(1, (int)DesignerPreviewNodeProperty(n, "rows", 2));
			int count = n.children.GetCount();
			rows = max(rows, (count + columns - 1) / columns);
			int cw = max(DPI(DESIGNER_GRID_CELL_WIDTH), (inner.GetWidth() - gap * (columns - 1)) / columns);
			int ch = max(DPI(DESIGNER_GRID_CELL_HEIGHT), (inner.GetHeight() - gap * (rows - 1)) / rows);
			String label = Format("Grid: %d columns x %d rows, gap %d", columns, rows, gap);
			w.DrawText(inner.left + DPI(4), inner.top + DPI(4), label, SansSerifZ(9).Bold(), c);
			for(int row = 0; row < rows; row++) {
				for(int col = 0; col < columns; col++) {
					Rect cell = RectC(inner.left + col * (cw + gap), inner.top + row * (ch + gap), cw, ch);
					if(cell.left >= inner.right || cell.top >= inner.bottom)
						continue;
					cell.right = min(cell.right, inner.right);
					cell.bottom = min(cell.bottom, inner.bottom);
					w.DrawRect(cell.left, cell.top, cell.GetWidth(), DPI(1), c);
					w.DrawRect(cell.left, cell.bottom - DPI(1), cell.GetWidth(), DPI(1), c);
					w.DrawRect(cell.left, cell.top, DPI(1), cell.GetHeight(), c);
					w.DrawRect(cell.right - DPI(1), cell.top, DPI(1), cell.GetHeight(), c);
				}
			}
		}

void DesignerPreview::DrawRoundedOutline(Draw& w, const Rect& r, Color c, int radius, int width)
{
			if(r.IsEmpty())
				return;
			ImageBuffer ib(r.GetSize());
			Fill(~ib, RGBAZero(), ib.GetLength());
			BufferPainter p(ib, MODE_ANTIALIASED);
			p.Begin();
			p.RoundedRectangle(width * 0.5, width * 0.5,
			                   r.GetWidth() - width * 0.5, r.GetHeight() - width * 0.5,
			                   radius);
			p.Stroke(width, c);
			p.End();
			w.DrawImage(r.left, r.top, ib);
		}

void DesignerPreview::PaintChildren(Draw& w, const DesignerNode& parent, Rect area, int depth)
{
			int count = parent.children.GetCount();
			if(count <= 0)
				return;
			if(parent.type_id == "GridLayout") {
				PaintGridChildren(w, parent, area, depth);
			}
			else
				PaintBoxChildren(w, parent, area, depth);
		}

Size DesignerPreview::GetNodePreviewSize(const DesignerNode& n) const
{
			const DesignerType* t = registry_ ? registry_->Find(n.type_id) : nullptr;
			Size def = t ? t->default_size : Size(DPI(DESIGNER_DEFAULT_WIDTH), DPI(DESIGNER_DEFAULT_HEIGHT));
			Size minsz = t ? t->min_size : Size(DPI(DESIGNER_MIN_WIDTH), DPI(DESIGNER_MIN_HEIGHT));
			if(DesignerPreviewAxisSizing(n, "h_sizing") == "Fixed")
				def.cx = (int)DesignerPreviewNodeProperty(n, "width", def.cx);
			if(DesignerPreviewAxisSizing(n, "v_sizing") == "Fixed")
				def.cy = (int)DesignerPreviewNodeProperty(n, "height", def.cy);
			return Size(max(minsz.cx, def.cx), max(minsz.cy, def.cy));
		}

void DesignerPreview::PaintBoxChildren(Draw& w, const DesignerNode& parent, Rect area, int depth)
{
			int gap = (int)DesignerPreviewNodeProperty(parent, "gap", DPI(8));
			int inset = (int)DesignerPreviewNodeProperty(parent, "inset", 0);
			area = area.Deflated(inset);
			bool horizontal = DesignerPreviewNodeProperty(parent, "direction", "V") == "H";
			bool wrap = horizontal && (bool)DesignerPreviewNodeProperty(parent, "wrap", false);
			int count = parent.children.GetCount();
			if(wrap) {
				int x = area.left;
				int y = area.top;
				int row_h = 0;
				for(int i = 0; i < count; i++) {
					DesignerNode* child = model_->Find(parent.children[i]);
					if(!child)
						continue;
					Size sz = GetNodePreviewSize(*child);
					int cx = min(sz.cx, area.GetWidth());
					int cy = sz.cy;
					if(x > area.left && x + cx > area.right) {
						x = area.left;
						y += row_h + gap;
						row_h = 0;
					}
					if(y >= area.bottom)
						break;
					Rect cr = RectC(x, y, min(cx, max(0, area.right - x)), min(cy, max(0, area.bottom - y)));
					child->last_rect = cr;
					PaintNode(w, *child, cr, depth);
					x += cx + gap;
					row_h = max(row_h, cy);
				}
				return;
			}
			int fixed_main = 0;
			int expand_count = 0;
			Vector<int> mains;
			for(DesignerNodeId child_id : parent.children) {
				DesignerNode* child = model_->Find(child_id);
				if(!child) {
					mains.Add(0);
					continue;
				}
				Size sz = GetNodePreviewSize(*child);
				String sizing = DesignerPreviewAxisSizing(*child, horizontal ? "h_sizing" : "v_sizing");
				if(sizing == "Expand") {
					mains.Add(-1);
					expand_count++;
				}
				else {
					int main = horizontal ? sz.cx : sz.cy;
					mains.Add(main);
					fixed_main += main;
				}
			}
			int available = max(0, (horizontal ? area.GetWidth() : area.GetHeight()) - gap * max(0, count - 1) - fixed_main);
			int expand_main = expand_count ? max(DPI(36), available / expand_count) : 0;
			int pos = horizontal ? area.left : area.top;
			for(int i = 0; i < count; i++) {
				DesignerNode* child = model_->Find(parent.children[i]);
				if(!child)
					continue;
				Size sz = GetNodePreviewSize(*child);
				int main = mains[i] < 0 ? max(horizontal ? sz.cx : sz.cy, expand_main) : mains[i];
				Rect cr = horizontal
				        ? RectC(pos, area.top, min(main, max(0, area.right - pos)), area.GetHeight())
				        : RectC(area.left, pos, area.GetWidth(), min(main, max(0, area.bottom - pos)));
				child->last_rect = cr;
				PaintNode(w, *child, cr, depth);
				pos += main + gap;
				if(pos >= (horizontal ? area.right : area.bottom))
					break;
			}
		}

void DesignerPreview::PaintGridChildren(Draw& w, const DesignerNode& parent, Rect area, int depth)
{
			int gap = (int)DesignerPreviewNodeProperty(parent, "gap", DPI(8));
			int inset = (int)DesignerPreviewNodeProperty(parent, "inset", 0);
			area = area.Deflated(inset);
			int columns = max(1, (int)DesignerPreviewNodeProperty(parent, "columns", 2));
			int rows = max(1, (int)DesignerPreviewNodeProperty(parent, "rows", 2));
			int count = parent.children.GetCount();
			rows = max(rows, (count + columns - 1) / columns);
			int cw = max(DPI(DESIGNER_GRID_CELL_WIDTH), (area.GetWidth() - gap * (columns - 1)) / columns);
			int ch = max(DPI(DESIGNER_GRID_CELL_HEIGHT), (area.GetHeight() - gap * (rows - 1)) / rows);
			for(int i = 0; i < count; i++) {
				DesignerNode* child = model_->Find(parent.children[i]);
				if(!child)
					continue;
				int col = clamp((int)DesignerPreviewNodeProperty(*child, "grid_col", i % columns), 0, columns - 1);
				int row = clamp((int)DesignerPreviewNodeProperty(*child, "grid_row", i / columns), 0, rows - 1);
				Rect cell = RectC(area.left + col * (cw + gap), area.top + row * (ch + gap), cw, ch);
				Size sz = GetNodePreviewSize(*child);
				String hs = DesignerPreviewAxisSizing(*child, "h_sizing");
				String vs = DesignerPreviewAxisSizing(*child, "v_sizing");
				Size want(hs == "Expand" ? cell.GetWidth() : min(cell.GetWidth(), sz.cx),
				          vs == "Expand" ? cell.GetHeight() : min(cell.GetHeight(), sz.cy));
				int x = cell.left;
				int y = cell.top;
				if(hs != "Expand") {
					UiGridLayout::Align ax = DesignerPreviewGridAlignH(*child);
					if(ax == UiGridLayout::Align::Center)
						x = cell.left + (cell.GetWidth() - want.cx) / 2;
					else if(ax == UiGridLayout::Align::End)
						x = cell.right - want.cx;
				}
				if(vs != "Expand") {
					UiGridLayout::Align ay = DesignerPreviewGridAlignV(*child);
					if(ay == UiGridLayout::Align::Center)
						y = cell.top + (cell.GetHeight() - want.cy) / 2;
					else if(ay == UiGridLayout::Align::End)
						y = cell.bottom - want.cy;
				}
				Rect cr = RectC(x, y, want.cx, want.cy);
				child->last_rect = cr;
				PaintNode(w, *child, cr, depth);
			}
		}

DesignerNodeId DesignerPreview::Hit(Point p) const
{
			DesignerNodeId best = Designer_NULL;
			int best_area = INT_MAX;
			int best_depth = -1;
			for(const DesignerNode& n : model_->GetNodes()) {
				if(n.id == Designer_ROOT)
					continue;
				if(n.last_rect.Contains(p)) {
					int area = n.last_rect.GetWidth() * n.last_rect.GetHeight();
					int depth = GetNodeDepth(n.id);
					if(depth > best_depth || (depth == best_depth && area < best_area)) {
						best = n.id;
						best_area = area;
						best_depth = depth;
					}
				}
			}
			return best;
		}

int DesignerPreview::GetNodeDepth(DesignerNodeId id) const
{
			int depth = 0;
			const DesignerNode* n = model_ ? model_->Find(id) : nullptr;
			while(n && n->parent && n->parent != Designer_ROOT) {
				depth++;
				n = model_->Find(n->parent);
			}
			return depth;
		}

DesignerNodeId DesignerPreview::ResolveDropTarget(DesignerNodeId hit) const
{
			if(!model_ || !registry_)
				return Designer_NULL;
			DesignerNodeId id = hit ? hit : Designer_ROOT;
			while(id) {
				const DesignerNode* n = model_->Find(id);
				const DesignerType* t = n ? registry_->Find(n->type_id) : nullptr;
				if(n && DesignerPreviewIsPageContainer(*n)) {
					DesignerNodeId page = DesignerPreviewActivePageSlot(*n);
					if(page != Designer_NULL && page != drag_candidate_)
						return page;
				}
				if(n && t && t->can_have_children && id != drag_candidate_)
					return id;
				id = n ? n->parent : Designer_NULL;
			}
			return Designer_ROOT;
		}

int DesignerPreview::FindChildIndex(const DesignerNode& parent, DesignerNodeId child) const
{
			for(int i = 0; i < parent.children.GetCount(); i++)
				if(parent.children[i] == child)
					return i;
			return -1;
		}

void DesignerPreview::ResetDropState()
{
			drag_candidate_ = Designer_NULL;
			drop_target_ = Designer_NULL;
			drop_index_ = -1;
			dragging_node_ = false;
		}

void DesignerPreview::UpdateDropSlot(Point p)
{
			DesignerNodeId hit = Hit(p);
			drop_target_ = ResolveDropTarget(hit);
			const DesignerNode* parent = model_ ? model_->Find(drop_target_) : nullptr;
			const DesignerNode* hit_node = model_ ? model_->Find(hit) : nullptr;
			drop_index_ = parent ? parent->children.GetCount() : -1;
			if(parent && (parent->type_id == "UiSplitter" || parent->type_id == "UiQuadSplitter")) {
				int panes = parent->type_id == "UiQuadSplitter" ? 4 : 2;
				drop_index_ = clamp(GetSplitterPaneIndex(*parent, p), 0, panes - 1);
			}
			else if(parent && parent->type_id == "GridLayout") {
				Rect pr = GetContainerContentRect(*parent, GetVirtualWindowRect());
				int inset = (int)DesignerPreviewNodeProperty(*parent, "inset", 0);
				Rect grid = pr.Deflated(inset);
				int columns = max(1, (int)DesignerPreviewNodeProperty(*parent, "columns", 2));
				int rows = max(1, (int)DesignerPreviewNodeProperty(*parent, "rows", 2));
				int gap = (int)DesignerPreviewNodeProperty(*parent, "gap", DPI(8));
				int cw = max(DPI(DESIGNER_GRID_CELL_WIDTH), (grid.GetWidth() - gap * (columns - 1)) / columns);
				int ch = max(DPI(DESIGNER_GRID_CELL_HEIGHT), (grid.GetHeight() - gap * (rows - 1)) / rows);
				int col = clamp((p.x - grid.left) / max(1, cw + gap), 0, columns - 1);
				int row = clamp((p.y - grid.top) / max(1, ch + gap), 0, rows - 1);
				drop_index_ = row * columns + col;
			}
			else if(parent && hit_node && hit_node->parent == parent->id) {
				int q = FindChildIndex(*parent, hit_node->id);
				if(q >= 0)
					drop_index_ = q + (UseHorizontalInsert(*parent) ? p.x >= (hit_node->last_rect.left + hit_node->last_rect.right) / 2
					                                               : p.y >= (hit_node->last_rect.top + hit_node->last_rect.bottom) / 2 ? 1 : 0);
			}
			else if(parent && parent->children.GetCount()) {
				bool horizontal = UseHorizontalInsert(*parent);
				for(int i = 0; i < parent->children.GetCount(); i++) {
					const DesignerNode* child = model_->Find(parent->children[i]);
					if(!child)
						continue;
					int mid = horizontal ? (child->last_rect.left + child->last_rect.right) / 2
					                     : (child->last_rect.top + child->last_rect.bottom) / 2;
					if((horizontal ? p.x : p.y) < mid) {
						drop_index_ = i;
						break;
					}
				}
			}
			const DesignerNode* drag_node = model_ && drag_candidate_ ? model_->Find(drag_candidate_) : nullptr;
			if(parent && drag_node && parent->id == drag_node->parent) {
				int old = FindChildIndex(*parent, drag_candidate_);
				if(old >= 0 && old < drop_index_)
					drop_index_--;
			}
		}

bool DesignerPreview::UseHorizontalInsert(const DesignerNode& parent) const
{
			if(parent.type_id == "GridLayout")
				return DesignerPreviewNodeProperty(parent, "direction", "H") == "H";
			return DesignerPreviewNodeProperty(parent, "direction", "V") == "H";
		}

Ctrl* DesignerPreview::BuildRealNode(DesignerNodeId id)
{
			DesignerNode* n = model_ ? model_->Find(id) : nullptr;
			if(!n || id == Designer_ROOT)
				return nullptr;
			DesignerAdapter *adapter = nullptr;
			Ctrl *raw = CreateDesignerAdapterCtrl(*n, &adapter);
			if(!raw || !adapter)
				return nullptr;
			raw->IgnoreMouse().NoWantFocus();
			Ctrl& ctrl = real_controls_.Add(raw);
			real_adapters_.Add(id, adapter);
			for(int i = 0; i < n->children.GetCount(); i++) {
				DesignerNode* child_node = model_->Find(n->children[i]);
				Ctrl *child = BuildRealNode(n->children[i]);
				if(child && child_node)
					AddRealChild(*adapter, *child, *n, *child_node, i);
			}
			FinalizeRealNode(*adapter, *n);
			return &ctrl;
		}

void DesignerPreview::AddRealChild(DesignerAdapter& parent, Ctrl& child,
                                     const DesignerNode& parent_node,
                                     const DesignerNode& child_node, int index)
{
			Ctrl& parent_ctrl = parent.GetCtrl();
			String hs = DesignerPreviewAxisSizing(child_node, "h_sizing");
			String vs = DesignerPreviewAxisSizing(child_node, "v_sizing");
			if(DesignerBoxLayoutAdapter *box = dynamic_cast<DesignerBoxLayoutAdapter *>(&parent)) {
				UiBoxLayout::ItemRef ref = box->Add(child);
				bool horizontal = DesignerPreviewNodeProperty(parent_node, "direction", "V") == "H";
				String main_sizing = horizontal ? hs : vs;
				if(main_sizing == "Fixed") {
					int fixed = horizontal
					          ? (int)DesignerPreviewNodeProperty(child_node, "width", DESIGNER_FIXED_FALLBACK_WIDTH)
					          : (int)DesignerPreviewNodeProperty(child_node, "height", DESIGNER_FIXED_FALLBACK_HEIGHT);
					ref.Fixed(DPI(DesignerClampMin(fixed)));
				}
				else if(main_sizing == "Expand")
					ref.Expand(1);
				else
					ref.Fit();
			}
			else if(DesignerGridLayoutAdapter *grid = dynamic_cast<DesignerGridLayoutAdapter *>(&parent)) {
				Size fixed(0, 0);
				if(hs == "Fixed" || vs == "Fixed")
					fixed = Size(DPI(DesignerClampMin((int)DesignerPreviewNodeProperty(child_node, "width", DESIGNER_FIXED_FALLBACK_WIDTH))),
					             DPI(DesignerClampMin((int)DesignerPreviewNodeProperty(child_node, "height", DESIGNER_FIXED_FALLBACK_HEIGHT))));
				int columns = max(1, (int)DesignerPreviewNodeProperty(parent_node, "columns", 2));
				int row = (int)DesignerPreviewNodeProperty(child_node, "grid_row", index / columns);
				int col = (int)DesignerPreviewNodeProperty(child_node, "grid_col", index % columns);
				int item = grid->Add(child, row, col, hs == "Expand", vs == "Expand", fixed);
				grid->SetItemAlign(item, DesignerPreviewGridAlignH(child_node), DesignerPreviewGridAlignV(child_node));
			}
			else if(DesignerSplitterAdapter *splitter = dynamic_cast<DesignerSplitterAdapter *>(&parent)) {
				splitter->Add(child);
				int pane = max(0, index);
				splitter->SetMinPixels(0, DPI((int)DesignerPreviewNodeProperty(parent_node, "min_a", 80)));
				splitter->SetMinPixels(1, DPI((int)DesignerPreviewNodeProperty(parent_node, "min_b", 80)));
				bool vertical = DesignerPreviewNodeProperty(parent_node, "direction", "H") == "V";
				String pane_sizing = DesignerPreviewAxisSizing(child_node, vertical ? "v_sizing" : "h_sizing");
				if(pane_sizing == "Fixed") {
					int fixed = vertical
					          ? (int)DesignerPreviewNodeProperty(child_node, "height", DESIGNER_SPLITTER_FALLBACK_HEIGHT)
					          : (int)DesignerPreviewNodeProperty(child_node, "width", DESIGNER_FIXED_FALLBACK_WIDTH);
					splitter->SetMinPixels(pane, DPI(DesignerClampMin(fixed)));
				}
			}
			else if(DesignerQuadSplitterAdapter *quad = dynamic_cast<DesignerQuadSplitterAdapter *>(&parent)) {
				quad->Add(child);
				int pane = max(0, index);
				quad->SetMinPixels(0, DPI((int)DesignerPreviewNodeProperty(parent_node, "min_a", 60)));
				quad->SetMinPixels(1, DPI((int)DesignerPreviewNodeProperty(parent_node, "min_b", 60)));
				quad->SetMinPixels(2, DPI((int)DesignerPreviewNodeProperty(parent_node, "min_c", 60)));
				quad->SetMinPixels(3, DPI((int)DesignerPreviewNodeProperty(parent_node, "min_d", 60)));
				if(DesignerPreviewAxisSizing(child_node, "h_sizing") == "Fixed" ||
				   DesignerPreviewAxisSizing(child_node, "v_sizing") == "Fixed") {
					int fixed = max((int)DesignerPreviewNodeProperty(child_node, "width", DESIGNER_FIXED_FALLBACK_WIDTH),
					                (int)DesignerPreviewNodeProperty(child_node, "height", DESIGNER_SPLITTER_FALLBACK_HEIGHT));
					quad->SetMinPixels(pane, DPI(DesignerClampMin(fixed)));
				}
			}
			else if(DesignerTabAdapter *tab = dynamic_cast<DesignerTabAdapter *>(&parent)) {
				String title = DesignerPreviewNodeProperty(child_node, "page_title", child_node.name);
				if(!(bool)DesignerPreviewNodeProperty(child_node, "show_title", true))
					title.Clear();
				tab->Add(child, title, UiIconFromName(DesignerPreviewNodeProperty(child_node, "icon", "None")));
			}
			else if(DesignerStackAdapter *stack = dynamic_cast<DesignerStackAdapter *>(&parent)) {
				String title = DesignerPreviewNodeProperty(child_node, "page_title", child_node.name);
				stack->AddPage(child, title);
			}
			else if(DesignerScrollPanelAdapter *scroll = dynamic_cast<DesignerScrollPanelAdapter *>(&parent))
				scroll->Content().Add(child.SizePos());
			else if(DesignerGroupPanelAdapter *group = dynamic_cast<DesignerGroupPanelAdapter *>(&parent)) {
				if(!group->GetContent())
					group->SetContent(child);
				child.SizePos();
			}
			else
				parent_ctrl.Add(child.SizePos());
		}

void DesignerPreview::FinalizeRealNode(DesignerAdapter& adapter, const DesignerNode& node)
{
			if(DesignerSplitterAdapter *splitter = dynamic_cast<DesignerSplitterAdapter *>(&adapter)) {
				splitter->SetMinPixels(0, DPI((int)DesignerPreviewNodeProperty(node, "min_a", 80)));
				splitter->SetMinPixels(1, DPI((int)DesignerPreviewNodeProperty(node, "min_b", 80)));
				splitter->SetSplitPercent((int)DesignerPreviewNodeProperty(node, "split_percent", 50));
			}
			else if(DesignerQuadSplitterAdapter *quad = dynamic_cast<DesignerQuadSplitterAdapter *>(&adapter)) {
				quad->SetMinPixels(0, DPI((int)DesignerPreviewNodeProperty(node, "min_a", 60)));
				quad->SetMinPixels(1, DPI((int)DesignerPreviewNodeProperty(node, "min_b", 60)));
				quad->SetMinPixels(2, DPI((int)DesignerPreviewNodeProperty(node, "min_c", 60)));
				quad->SetMinPixels(3, DPI((int)DesignerPreviewNodeProperty(node, "min_d", 60)));
				quad->SetSplitPercent((int)DesignerPreviewNodeProperty(node, "column_percent", 50),
				                      (int)DesignerPreviewNodeProperty(node, "row_percent", 50));
			}
			else if(DesignerTabAdapter *tab = dynamic_cast<DesignerTabAdapter *>(&adapter)) {
				if(tab->GetCount() > 0)
					tab->SetActiveTab(clamp((int)DesignerPreviewNodeProperty(node, "active", 0), 0, tab->GetCount() - 1));
			}
			else if(DesignerStackAdapter *stack = dynamic_cast<DesignerStackAdapter *>(&adapter)) {
				if(stack->GetCount() > 0)
					stack->SetActivePage(clamp((int)DesignerPreviewNodeProperty(node, "active", 0), 0, stack->GetCount() - 1));
			}
		}

void DesignerPreview::ApplyRealLayoutProperties(Ctrl& ctrl)
{
			if(DesignerAdapter *adapter = AsDesignerAdapter(ctrl)) {
				if(const DesignerNode* n = model_ ? model_->Find(adapter->GetNodeId()) : nullptr)
					FinalizeRealNode(*adapter, *n);
			}
			ctrl.Layout();
			for(Ctrl *child = ctrl.GetFirstChild(); child; child = child->GetNext())
				ApplyRealLayoutProperties(*child);
		}

void DesignerPreview::RebuildRealPreview()
{
			if(rebuilding_real_)
				return;
			rebuilding_real_ = true;
			real_dirty_ = false;
			for(int i = 0; i < real_controls_.GetCount(); i++)
				real_controls_[i].Remove();
			real_adapters_.Clear();
			real_controls_.Clear();
			if(!model_ || !registry_) {
				rebuilding_real_ = false;
				return;
			}
			const DesignerNode* root = model_->Find(Designer_ROOT);
			if(!root) {
				rebuilding_real_ = false;
				return;
			}
			for(DesignerNodeId child_id : root->children) {
				Ctrl *child = BuildRealNode(child_id);
				if(child)
					Add(*child);
			}
			rebuilding_real_ = false;
		}

void DesignerPreview::LayoutRealPreview()
{
			if(!model_)
				return;
			if(real_dirty_)
				RebuildRealPreview();
			Rect root = GetVirtualWindowRect();
			for(Ctrl *child = GetFirstChild(); child; child = child->GetNext())
				child->SetRect(root);
			for(Ctrl *child = GetFirstChild(); child; child = child->GetNext())
				ApplyRealLayoutProperties(*child);
			for(Ctrl *child = GetFirstChild(); child; child = child->GetNext())
				UpdateRealRects(*child, Point(0, 0));
			ApplyRealOverlay();
		}

void DesignerPreview::RefreshRealPreviewTree(Ctrl& ctrl)
{
			ctrl.RefreshLayout();
			ctrl.Refresh();
			for(Ctrl *child = ctrl.GetFirstChild(); child; child = child->GetNext())
				RefreshRealPreviewTree(*child);
		}

void DesignerPreview::UpdateRealRects(Ctrl& ctrl, Point offset)
{
			if(DesignerAdapter *adapter = AsDesignerAdapter(ctrl)) {
				DesignerNode* n = model_ ? model_->Find(adapter->GetNodeId()) : nullptr;
				if(n)
					n->last_rect = ctrl.GetRect() + offset;
			}
			Point child_offset = offset + ctrl.GetRect().TopLeft();
			for(Ctrl *child = ctrl.GetFirstChild(); child; child = child->GetNext())
				UpdateRealRects(*child, child_offset);
		}

void DesignerPreview::ApplyRealOverlay()
{
			for(int i = 0; i < real_adapters_.GetCount(); i++) {
				DesignerAdapter *adapter = real_adapters_[i];
				if(!adapter)
					continue;
				DesignerOverlayState state;
				state.selected = DesignerPreviewFindNodeId(model_->GetSelection(), real_adapters_.GetKey(i)) >= 0;
				state.drop_target = real_adapters_.GetKey(i) == drop_target_;
				const DesignerNode* n = model_ ? model_->Find(real_adapters_.GetKey(i)) : nullptr;
				state.debug = n && (bool)DesignerPreviewNodeProperty(*n, "debug", false);
				if(n) {
					if((bool)DesignerPreviewNodeProperty(*n, "debug_auto_color", false)) {
						static const Color palette[] = {
							Color(220, 38, 38), Color(217, 119, 6), Color(37, 99, 235),
							Color(22, 163, 74), Color(147, 51, 234), Color(8, 145, 178),
							Color(219, 39, 119)
						};
						state.debug_color = palette[abs((int)n->id) % (int)(sizeof(palette) / sizeof(palette[0]))];
					}
					else
						state.debug_color = (Color)DesignerPreviewNodeProperty(*n, "debug_color", Color(220, 38, 38));
				}
				state.radius = n ? (int)DesignerPreviewNodeProperty(*n, "radius", 0) : 0;
				adapter->SetOverlayState(state);
			}
		}

}
