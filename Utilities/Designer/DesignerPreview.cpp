#include "DesignerPreview.h"

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

void DesignerPreview::Paint(Draw& w)
{
			bool dark = theme_mode_ == UiThemeMode::Dark;
			w.DrawRect(GetSize(), dark ? Color(18, 22, 28) : Color(246, 248, 251));
			if(!model_)
				return;
			if(real_dirty_)
				RebuildRealPreview();
			Rect root = GetVirtualWindowRect();
			Size vsz = model_->GetVirtualSize();
			DrawRoundedOutline(w, root, SColorHighlight(), DPI(8), DPI(2));
			for(Ctrl *child = GetFirstChild(); child; child = child->GetNext())
				child->SetRect(root.Deflated(DPI(12)));
			Layout();
			for(Ctrl *child = GetFirstChild(); child; child = child->GetNext())
				UpdateRealRects(*child, Point(0, 0));
			ApplyRealOverlay();
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
				model_->SetVirtualSize(Size(max(DPI(240), p.x - root.left),
				                            max(DPI(180), p.y - root.top)));
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
			w.DrawRect(r.left, r.top, r.GetWidth(), DPI(2), c);
			w.DrawRect(r.left, r.bottom - DPI(2), r.GetWidth(), DPI(2), c);
			w.DrawRect(r.left, r.top, DPI(2), r.GetHeight(), c);
			w.DrawRect(r.right - DPI(2), r.top, DPI(2), r.GetHeight(), c);
			Rect marker = GetInsertMarkerRect(*target, root);
			if(!marker.IsEmpty())
				w.DrawRect(marker, c);
			String verb = !placement_type_.IsEmpty() ? "Insert in " : "Move in ";
			String label = (drop_target_ == Designer_ROOT ? verb + "Window" : verb + target->name)
			             + Format(" at %d", max(0, drop_index_));
			Font tag_font = SansSerifZ(9);
			Size tsz = GetTextSize(label, tag_font);
			Rect tag = RectC(r.left + DPI(8), max(root.top + DPI(6), r.top - DPI(22)), tsz.cx + DPI(12), DPI(20));
			w.DrawRect(tag, c);
			w.DrawText(tag.left + DPI(6), tag.top + DPI(4), label, tag_font, Black());
		}

Rect DesignerPreview::GetInsertMarkerRect(const DesignerNode& parent, const Rect& root) const
{
			Rect pr = parent.id == Designer_ROOT ? root.Deflated(DPI(12)) : parent.last_rect.Deflated(DPI(10), DPI(24), DPI(10), DPI(10));
			if(pr.IsEmpty())
				return Rect(0, 0, 0, 0);
			int count = parent.children.GetCount();
			if(count <= 0)
				return RectC(pr.left, pr.top, pr.GetWidth(), DPI(4));
			int idx = clamp(drop_index_, 0, count);
			if(parent.type_id == "GridLayout" && DesignerPreviewNodeProperty(parent, "mode", "Flow") == "Grid") {
				int gap = (int)DesignerPreviewNodeProperty(parent, "gap", DPI(8));
				int inset = (int)DesignerPreviewNodeProperty(parent, "inset", 0);
				Rect grid = pr.Deflated(inset);
				if(grid.IsEmpty())
					return Rect(0, 0, 0, 0);
				int columns = max(1, (int)DesignerPreviewNodeProperty(parent, "columns", 2));
				int rows = max(1, (int)DesignerPreviewNodeProperty(parent, "rows", 2));
				rows = max(rows, (count + columns - 1) / columns);
				int cw = max(DPI(24), (grid.GetWidth() - gap * (columns - 1)) / columns);
				int ch = max(DPI(24), (grid.GetHeight() - gap * (rows - 1)) / rows);
				bool vertical = DesignerPreviewNodeProperty(parent, "direction", "H") == "V";
				int row = vertical ? idx % rows : idx / columns;
				int col = vertical ? idx / rows : idx % columns;
				if(idx >= count && count > 0) {
					row = vertical ? ((count - 1) % rows) + 1 : (count - 1) / columns;
					col = vertical ? (count - 1) / rows : ((count - 1) % columns) + 1;
				}
				if(vertical && row >= rows) {
					row = 0;
					col++;
				}
				else if(!vertical && col >= columns) {
					col = 0;
					row++;
				}
				row = min(row, rows - 1);
				col = min(col, columns - 1);
				int y = grid.top + row * (ch + gap);
				int x = grid.left + col * (cw + gap);
				if(vertical) {
					if(row == 0 && idx > 0 && idx < count)
						return RectC(x - max(DPI(2), gap / 2), grid.top, DPI(4), grid.GetHeight());
					return RectC(x, y - DPI(2), min(cw, max(0, grid.right - x)), DPI(4));
				}
				if(col == 0 && idx > 0 && idx < count)
					return RectC(grid.left, y - max(DPI(2), gap / 2), grid.GetWidth(), DPI(4));
				if(idx >= count && count > 0 && ((count - 1) % columns) == columns - 1)
					x = grid.left;
				return RectC(x - DPI(2), y, DPI(4), min(ch, max(0, grid.bottom - y)));
			}
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
			bool dark = theme_mode_ == UiThemeMode::Dark;
			Color default_face = t && t->is_container ? (dark ? Color(38, 82, 64) : Color(207, 242, 226))
			                                          : (dark ? Color(42, 68, 104) : Color(214, 231, 255));
			Color default_frame = t && t->is_container ? Color(44, 156, 105) : Color(54, 116, 210);
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
				Rect content = r.Deflated(DPI(10), DPI(24), DPI(10), DPI(10));
				PaintChildren(w, n, content, depth + 1);
				DrawLayoutDebug(w, n, content);
			}
		}

void DesignerPreview::DrawLayoutDebug(Draw& w, const DesignerNode& n, Rect content)
{
			if(!(bool)DesignerPreviewNodeProperty(n, "debug", false))
				return;
			Color c = Color(255, 128, 0);
			int inset = (int)DesignerPreviewNodeProperty(n, "inset", 0);
			Rect inner = content.Deflated(inset);
			w.DrawRect(inner.left, inner.top, inner.GetWidth(), DPI(1), c);
			w.DrawRect(inner.left, inner.bottom - DPI(1), inner.GetWidth(), DPI(1), c);
			w.DrawRect(inner.left, inner.top, DPI(1), inner.GetHeight(), c);
			w.DrawRect(inner.right - DPI(1), inner.top, DPI(1), inner.GetHeight(), c);
			if(n.type_id != "GridLayout")
				return;
			String mode = DesignerPreviewNodeProperty(n, "mode", "Flow");
			if(mode == "Flow") {
				String label = "Flow grid: wrap uses child sizes; rows/columns are ignored";
				w.DrawText(inner.left + DPI(4), inner.top + DPI(4), label, SansSerifZ(9).Bold(), c);
				return;
			}
			int gap = (int)DesignerPreviewNodeProperty(n, "gap", DPI(8));
			int columns = max(1, (int)DesignerPreviewNodeProperty(n, "columns", 2));
			int rows = max(1, (int)DesignerPreviewNodeProperty(n, "rows", 2));
			int count = n.children.GetCount();
			rows = max(rows, (count + columns - 1) / columns);
			int cw = max(DPI(24), (inner.GetWidth() - gap * (columns - 1)) / columns);
			int ch = max(DPI(24), (inner.GetHeight() - gap * (rows - 1)) / rows);
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
				if(DesignerPreviewNodeProperty(parent, "mode", "Flow") == "Flow")
					PaintFlowGridChildren(w, parent, area, depth);
				else
					PaintGridChildren(w, parent, area, depth);
			}
			else
				PaintBoxChildren(w, parent, area, depth);
		}

Size DesignerPreview::GetNodePreviewSize(const DesignerNode& n) const
{
			const DesignerType* t = registry_ ? registry_->Find(n.type_id) : nullptr;
			Size def = t ? t->default_size : Size(DPI(120), DPI(32));
			Size minsz = t ? t->min_size : Size(DPI(24), DPI(20));
			String sizing = DesignerPreviewNodeProperty(n, "sizing", "Fit");
			if(sizing == "Fixed")
				def = Size((int)DesignerPreviewNodeProperty(n, "width", def.cx), (int)DesignerPreviewNodeProperty(n, "height", def.cy));
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
				String sizing = DesignerPreviewNodeProperty(*child, "sizing", "Fit");
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

void DesignerPreview::PaintFlowGridChildren(Draw& w, const DesignerNode& parent, Rect area, int depth)
{
			int gap = (int)DesignerPreviewNodeProperty(parent, "gap", DPI(8));
			int inset = (int)DesignerPreviewNodeProperty(parent, "inset", 0);
			area = area.Deflated(inset);
			bool horizontal = DesignerPreviewNodeProperty(parent, "direction", "H") == "H";
			bool wrap = (bool)DesignerPreviewNodeProperty(parent, "wrap", true);
			bool align_cells = (bool)DesignerPreviewNodeProperty(parent, "align_cells", true);
			Size cell_size((int)DesignerPreviewNodeProperty(parent, "cell_width", 120),
			               (int)DesignerPreviewNodeProperty(parent, "cell_height", 32));
			int count = parent.children.GetCount();
			if(horizontal) {
				int x = area.left;
				int y = area.top;
				int row_h = 0;
				for(int i = 0; i < count; i++) {
					DesignerNode* child = model_->Find(parent.children[i]);
					if(!child)
						continue;
					Size sz = GetNodePreviewSize(*child);
					if(align_cells)
						sz = Size(max(DPI(10), cell_size.cx), max(DPI(10), cell_size.cy));
					int cx = min(sz.cx, area.GetWidth());
					int cy = sz.cy;
					if(wrap && x > area.left && x + cx > area.right) {
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
			}
			else {
				int x = area.left;
				int y = area.top;
				int col_w = 0;
				for(int i = 0; i < count; i++) {
					DesignerNode* child = model_->Find(parent.children[i]);
					if(!child)
						continue;
					Size sz = GetNodePreviewSize(*child);
					if(align_cells)
						sz = Size(max(DPI(10), cell_size.cx), max(DPI(10), cell_size.cy));
					int cx = sz.cx;
					int cy = min(sz.cy, area.GetHeight());
					if(wrap && y > area.top && y + cy > area.bottom) {
						y = area.top;
						x += col_w + gap;
						col_w = 0;
					}
					if(x >= area.right)
						break;
					Rect cr = RectC(x, y, min(cx, max(0, area.right - x)), min(cy, max(0, area.bottom - y)));
					child->last_rect = cr;
					PaintNode(w, *child, cr, depth);
					y += cy + gap;
					col_w = max(col_w, cx);
				}
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
			int cw = max(DPI(24), (area.GetWidth() - gap * (columns - 1)) / columns);
			int ch = max(DPI(24), (area.GetHeight() - gap * (rows - 1)) / rows);
			for(int i = 0; i < count; i++) {
				DesignerNode* child = model_->Find(parent.children[i]);
				if(!child)
					continue;
				bool vertical = DesignerPreviewNodeProperty(parent, "direction", "H") == "V";
				int col = vertical ? i / rows : i % columns;
				int row = vertical ? i % rows : i / columns;
				Rect cell = RectC(area.left + col * (cw + gap), area.top + row * (ch + gap), cw, ch);
				Size sz = GetNodePreviewSize(*child);
				String sizing = DesignerPreviewNodeProperty(*child, "sizing", "Fit");
				Rect cr = sizing == "Expand" ? cell : RectC(cell.left, cell.top, min(cell.GetWidth(), sz.cx), min(cell.GetHeight(), sz.cy));
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
			if(parent && hit_node && hit_node->parent == parent->id) {
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
			return &ctrl;
		}

void DesignerPreview::AddRealChild(DesignerAdapter& parent, Ctrl& child,
                                     const DesignerNode& parent_node,
                                     const DesignerNode& child_node, int index)
{
			Ctrl& parent_ctrl = parent.GetCtrl();
			String sizing = DesignerPreviewNodeProperty(child_node, "sizing", "Fit");
			if(DesignerBoxLayoutAdapter *box = dynamic_cast<DesignerBoxLayoutAdapter *>(&parent)) {
				UiBoxLayout::ItemRef ref = box->Add(child);
				if(sizing == "Fixed") {
					bool horizontal = DesignerPreviewNodeProperty(parent_node, "direction", "V") == "H";
					int fixed = horizontal
					          ? (int)DesignerPreviewNodeProperty(child_node, "width", 120)
					          : (int)DesignerPreviewNodeProperty(child_node, "height", 32);
					ref.Fixed(DPI(max(10, fixed)));
				}
				else if(sizing == "Expand")
					ref.Expand(1);
				else
					ref.Fit();
			}
			else if(DesignerGridLayoutAdapter *grid = dynamic_cast<DesignerGridLayoutAdapter *>(&parent)) {
				String mode = DesignerPreviewNodeProperty(parent_node, "mode", "Flow");
				Size fixed(0, 0);
				if(sizing == "Fixed")
					fixed = Size(DPI(max(10, (int)DesignerPreviewNodeProperty(child_node, "width", 120))),
					             DPI(max(10, (int)DesignerPreviewNodeProperty(child_node, "height", 32))));
				if(mode == "Grid") {
					int columns = max(1, (int)DesignerPreviewNodeProperty(parent_node, "columns", 2));
					grid->AddGrid(child, index / columns, index % columns, sizing == "Expand", fixed);
				}
				else
					grid->Add(child, -1, sizing == "Expand", fixed);
			}
			else if(DesignerSplitterAdapter *splitter = dynamic_cast<DesignerSplitterAdapter *>(&parent)) {
				splitter->Add(child);
				int pane = max(0, index);
				if(sizing == "Fixed") {
					bool vertical = DesignerPreviewNodeProperty(parent_node, "direction", "H") == "V";
					int fixed = vertical
					          ? (int)DesignerPreviewNodeProperty(child_node, "height", 80)
					          : (int)DesignerPreviewNodeProperty(child_node, "width", 120);
					splitter->SetMinPixels(pane, DPI(max(10, fixed)));
				}
				splitter->SetSplitPercent((int)DesignerPreviewNodeProperty(parent_node, "split_percent", 50));
			}
			else if(DesignerQuadSplitterAdapter *quad = dynamic_cast<DesignerQuadSplitterAdapter *>(&parent)) {
				quad->Add(child);
				int pane = max(0, index);
				if(sizing == "Fixed") {
					int fixed = max((int)DesignerPreviewNodeProperty(child_node, "width", 120),
					                (int)DesignerPreviewNodeProperty(child_node, "height", 80));
					quad->SetMinPixels(pane, DPI(max(10, fixed)));
				}
				quad->SetSplitPercent((int)DesignerPreviewNodeProperty(parent_node, "column_percent", 50),
				                      (int)DesignerPreviewNodeProperty(parent_node, "row_percent", 50));
			}
			else if(DesignerScrollPanelAdapter *scroll = dynamic_cast<DesignerScrollPanelAdapter *>(&parent))
				scroll->Content().Add(child);
			else
				parent_ctrl.Add(child);
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
				state.radius = n ? (int)DesignerPreviewNodeProperty(*n, "radius", 0) : 0;
				adapter->SetOverlayState(state);
			}
		}

}
