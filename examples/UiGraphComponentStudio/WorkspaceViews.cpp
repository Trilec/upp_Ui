#include "WorkspaceViews.h"

namespace Upp {
namespace GraphWorkspace {
void Frame(Draw& w, Rect r, Color c, int n)
{
    if(r.IsEmpty()) return;
    n = min(n, min(r.GetWidth(), r.GetHeight()));
    w.DrawRect(r.left, r.top, r.GetWidth(), n, c);
    w.DrawRect(r.left, r.bottom - n, r.GetWidth(), n, c);
    w.DrawRect(r.left, r.top, n, r.GetHeight(), c);
    w.DrawRect(r.right - n, r.top, n, r.GetHeight(), c);
}
Color RegionColor(int r)
{
    return r == 0 ? Color(196, 53, 45) : r == 7 ? Color(49, 184, 86)
         : r >= 4 ? Color(106, 99, 215) : Color(19, 160, 216);
}
Rect RegionRect(const UiGraphNodePresentation& p, int r)
{
    switch(r) {
    case 0: return p.header;
    case 1: return p.content_left; case 2: return p.content_main; case 3: return p.content_right;
    case 4: return p.overlay_left; case 5: return p.overlay_main; case 6: return p.overlay_right;
    case 7: return p.footer;
    default: return Rect();
    }
}
String PlacementSummary(const UiGraphNodeSlotRule& r)
{
    const char* place[] = { "Fill", "Top", "Bottom", "Left", "Right", "Center" };
    String s = place[(int)r.placement];
    s << " / " << (r.align_h == UiAlign::LEFT ? "left" : r.align_h == UiAlign::RIGHT ? "right" : "center");
    s << " / " << (r.flow == UiGraphNodeSlotFlow::Stable ? "stable" : "reflow");
    if(r.overflow == UiGraphNodeOverflow::Wrap) s << " / wrap";
    if(r.use_literal) s << " / static";
    if(!IsNull(r.ink) || r.font_height || !r.component_style.font_face.IsEmpty()) s << " / styled";
    return s;
}

Point RegionView::Project(Pointf p) const
{
    if(surface_.IsEmpty()) return Point(0, 0);
    Size size = GetSize();
    double scale = min((double)max(1, size.cx - DPI(28)) / surface_.GetWidth(),
                       (double)max(1, size.cy - DPI(60)) / surface_.GetHeight());
    double x = (size.cx - surface_.GetWidth() * scale) * 0.5;
    double y = DPI(12) + (max(1, size.cy - DPI(60)) - surface_.GetHeight() * scale) * 0.5;
    return Point(fround(x + (p.x - surface_.left) * scale), fround(y + (p.y - surface_.top) * scale));
}
Rect RegionView::Project(Rect r) const
{
    return r.IsEmpty() ? Rect() : Rect(Project(Pointf(r.left, r.top)), Project(Pointf(r.right, r.bottom)));
}
void RegionView::Set(const UiGraphNodeTemplate& spec, const UiGraphNodePresentation& p,
                     const Vector<Pointf>& path, Rect surface, const Selection& selection)
{
    spec_ = spec; presentation_ = p; path_ = clone(path); surface_ = surface; selected_ = selection;
    Layout(); Refresh();
}
void RegionView::Layout()
{
    targets_.Clear();
    int empty_count = 0;
    for(int r = 0; r < 8; r++) {
        if(overlay_ != (r >= 4 && r <= 6)) continue;
        Target& t = targets_.Add(); t.region = r; t.rect = Project(RegionRect(presentation_, r));
        if(t.rect.IsEmpty()) {
            t.empty = true;
            t.rect = RectC(DPI(6) + empty_count * DPI(64), max(0, GetSize().cy - DPI(36)), DPI(60), DPI(26));
            empty_count++;
        }
    }
    for(const auto& c : presentation_.components) {
        bool overlay = (int)c.region >= 4 && (int)c.region <= 6;
        if(overlay != overlay_ || c.slot.IsEmpty()) continue;
        Target& t = targets_.Add(); t.region = (int)c.region; t.id = c.id;
        Rect r = Project(c.slot);
        t.rect = RectC(r.left + DPI(3), r.top + DPI(3), max(0, r.GetWidth() - DPI(6)), min(DPI(20), max(0, r.GetHeight() - DPI(6))));
    }
}
int RegionView::Hit(Point p) const
{
    for(int i = targets_.GetCount() - 1; i >= 0; i--) if(targets_[i].rect.Contains(p)) return i;
    return -1;
}
void RegionView::Paint(Draw& w)
{
    w.DrawRect(GetSize(), Color(250, 252, 254));
    Frame(w, Rect(GetSize()), Color(215, 224, 233));
    for(int i = 0; i < path_.GetCount(); i++)
        w.DrawLine(Project(path_[i]), Project(path_[(i + 1) % path_.GetCount()]), 1, Color(143, 156, 170));
    Frame(w, Project(presentation_.safe), Color(171, 182, 193));
    Frame(w, Project(presentation_.body), Color(239, 125, 34));
    Font font = StdFont().Height(DPI(10));
    for(int i = 0; i < targets_.GetCount(); i++) {
        const auto& t = targets_[i]; if(t.rect.IsEmpty()) continue;
        bool active = false;
        for(const auto& c : presentation_.components)
            if((int)c.region == t.region && c.representation != UiGraphNodeComponentRepresentation::Hidden) active = true;
        Color color = active ? RegionColor(t.region) : Color(174, 185, 195);
        bool selected = selected_.id.IsEmpty() ? t.id.IsEmpty() && selected_.region == t.region : t.id == selected_.id;
        if(!t.id.IsEmpty()) {
            w.DrawRect(t.rect, selected ? Color(209, 234, 250) : Color(238, 248, 253));
            int n = spec_.FindComponent(t.id);
            String label = n >= 0 ? spec_.slots[n].label : t.id;
            if(label.IsEmpty()) label = t.id;
            w.Clip(t.rect); w.DrawText(t.rect.left + DPI(3), t.rect.top + DPI(2), label, font, Color(40, 94, 130)); w.End();
        }
        else {
            if(overlay_ && !t.empty) {
                w.Clip(t.rect);
                for(int x = t.rect.left - t.rect.GetHeight(); x < t.rect.right; x += DPI(12))
                    w.DrawLine(x, t.rect.bottom, x + t.rect.GetHeight(), t.rect.top, 1, Color(226, 224, 245));
                w.End();
            }
            String label = t.empty ? String("+ ") + region_names[t.region] : String(region_names[t.region]);
            w.Clip(t.rect); w.DrawText(t.rect.left + DPI(3), t.rect.top + DPI(2), label, font, color); w.End();
        }
        Frame(w, t.rect, selected || hover_ == i ? Color(12, 127, 211) : color, selected || hover_ == i ? DPI(2) : 1);
    }
    // Semantic port reservations are non-drop graph chrome.
    for(const Rect& lane : presentation_.port_lanes) Frame(w, Project(lane), Color(19, 160, 216));
    w.DrawText(DPI(6), max(0, GetSize().cy - DPI(11)), "Grey: inactive. + target: unreserved (confirmation required).", font.Height(DPI(9)), Color(102, 117, 135));
}
void RegionView::LeftDown(Point p, dword)
{
    int i = Hit(p); if(i >= 0) WhenSelect(targets_[i].id, targets_[i].region);
}
void RegionView::LeftDrag(Point p, dword)
{
    int i = Hit(p); if(i >= 0 && !targets_[i].id.IsEmpty()) WhenDrag(targets_[i].id);
}
void RegionView::DragAndDrop(Point p, PasteClip& d)
{
    int i = Hit(p); hover_ = -1;
    if(i >= 0 && WhenDrop && WhenDrop(d, targets_[i].region, targets_[i].id)) hover_ = i;
    Refresh();
}

StructureView::StructureView()
{
    AddFrame(scroll_); scroll_.WhenScroll = [this] { Refresh(); };
    BackPaint();
}
void StructureView::Set(const UiGraphNodeTemplate& spec, const UiGraphNodePresentation& p, const Selection& selected)
{
    spec_ = spec; presentation_ = p; selected_ = selected;
    rows_.Clear();
    auto add = [&](const char* name, int depth, int region, int mask) {
        auto& r = rows_.Add(); r.name = name; r.depth = depth; r.region = region; r.mask = mask;
        if(region < 0) return;
        for(int i = 0; i < spec_.slot_count; i++) if((int)spec_.slots[i].region == region) {
            const auto& c = spec_.slots[i]; auto& row = rows_.Add();
            row.id = c.id; row.name = c.label.IsEmpty() ? c.id : c.label;
            row.depth = depth + 1; row.region = region;
        }
    };
    add("Node / Safe Area", 0, -1, 255); add("Header", 1, 0, 1);
    add("Body", 1, -1, 126); add("Content", 2, -1, 14);
    add("Left", 3, 1, 2); add("Main", 3, 2, 4); add("Right", 3, 3, 8);
    add("Overlay (same Body extent)", 2, -1, 112);
    add("Left", 3, 4, 16); add("Main", 3, 5, 32); add("Right", 3, 6, 64);
    add("Footer", 1, 7, 128); add("Port lanes (graph-owned / not component targets)", 1, -1, 0);
    Layout(); Refresh();
}
void StructureView::Layout()
{
    scroll_.SetTotal(rows_.GetCount() * RowHeight()); scroll_.SetPage(max(0, GetSize().cy - DPI(30)));
}
int StructureView::Hit(Point p) const
{
    if(p.y < DPI(30)) return -1;
    int i = (p.y - DPI(30) + scroll_.Get()) / RowHeight();
    return i >= 0 && i < rows_.GetCount() ? i : -1;
}
void StructureView::Paint(Draw& w)
{
    w.DrawRect(GetSize(), White());
    int columns = Columns(), placement = min(DPI(310), columns * 3 / 5);
    Font font = StdFont().Height(DPI(11));
    w.DrawRect(0, 0, GetSize().cx, DPI(30), Color(247, 249, 252));
    w.DrawText(DPI(8), DPI(7), "STRUCTURE / COMPONENT", font.Bold(), Color(90, 110, 132));
    w.DrawText(placement, DPI(7), "PLACEMENT / FEATURES", font.Bold(), Color(90, 110, 132));
    for(int l = 0; l < 4; l++) {
        Rect box = RectC(columns + l * DPI(62), DPI(4), DPI(58), DPI(21));
        w.DrawRect(box, l < 2 ? Color(12, 127, 211) : Color(125, 166, 204));
        w.DrawText(box.left + DPI(6), box.top + DPI(3), l ? "LOD " + AsString(l) : String("Normal"), font, White());
    }
    w.Clip(0, DPI(30), GetSize().cx, max(0, GetSize().cy - DPI(30)));
    for(int i = 0; i < rows_.GetCount(); i++) {
        const Row& r = rows_[i]; int y = DPI(30) + i * RowHeight() - scroll_.Get();
        if(y + RowHeight() < DPI(30) || y >= GetSize().cy) continue;
        bool selected = r.id.IsEmpty() ? selected_.id.IsEmpty() && r.region >= 0 && selected_.region == r.region : selected_.id == r.id;
        w.DrawRect(0, y, GetSize().cx, RowHeight(), selected ? Color(232, 246, 255) : r.id.IsEmpty() ? Color(250, 252, 254) : White());
        if(selected || drop_row_ == i) w.DrawRect(0, y, DPI(3), RowHeight(), Color(12, 127, 211));
        w.DrawRect(0, y + RowHeight() - 1, GetSize().cx, 1, Color(232, 237, 242));
        int n = r.id.IsEmpty() ? -1 : spec_.FindComponent(r.id);
        w.Clip(0, y, placement - DPI(5), RowHeight());
        w.DrawText(DPI(7) + r.depth * DPI(15), y + DPI(5), r.name, r.id.IsEmpty() ? font.Bold() : font, Color(42, 62, 82));
        w.End();
        if(n >= 0) {
            const auto& c = spec_.slots[n];
            w.Clip(placement, y, max(0, columns - placement - DPI(5)), RowHeight());
            w.DrawText(placement, y + DPI(6), PlacementSummary(c), font.Height(DPI(10)), Color(93, 110, 127)); w.End();
        }
        for(int l = 0; l < 4; l++) {
            String text; Color ink(105, 123, 139);
            if(n >= 0) {
                const auto& c = spec_.slots[n]; byte bit = byte(1u << l);
                text = c.force_off & bit ? "Off" : c.force_on & bit ? "On" : c.Allows((UiGraphPresentationLevel)l) ? "Auto +" : "Auto -";
                ink = c.Allows((UiGraphPresentationLevel)l) ? Color(36, 124, 72) : Color(181, 43, 34);
            }
            else {
                int total = 0, on = 0;
                for(int k = 0; k < spec_.slot_count; k++) if(r.mask & (1 << (int)spec_.slots[k].region)) { total++; on += spec_.slots[k].Allows((UiGraphPresentationLevel)l); }
                text = total ? AsString(on) + "/" + AsString(total) : String("-");
            }
            w.DrawText(columns + l * DPI(62) + DPI(8), y + DPI(5), text, font, ink);
        }
    }
    w.End();
}
void StructureView::LeftDown(Point p, dword)
{
    int i = Hit(p); if(i < 0 || rows_[i].region < 0) return;
    Row row = rows_[i]; WhenSelect(row.id, row.region);
    if(!row.id.IsEmpty() && p.x >= Columns()) {
        int l = (p.x - Columns()) / DPI(62); if(l >= 0 && l < 4) WhenLod(row.id, l);
    }
}
void StructureView::LeftDrag(Point p, dword)
{
    int i = Hit(p); if(i >= 0 && !rows_[i].id.IsEmpty()) WhenDrag(rows_[i].id);
}
void StructureView::MouseWheel(Point, int z, dword)
{
    scroll_.Set(scroll_.Get() - z / 120 * RowHeight() * 3); Refresh();
}
void StructureView::DragAndDrop(Point p, PasteClip& d)
{
    int i = Hit(p); drop_row_ = -1;
    if(i >= 0 && rows_[i].region >= 0 && WhenDrop && WhenDrop(d, rows_[i].region, rows_[i].id)) drop_row_ = i;
    Refresh();
}
void PreviewGraph::Paint(Draw& w)
{
    UiNodeGraph::Paint(w);
    if(!selected.id.IsEmpty()) {
        const auto* c = snapshot.FindComponent(selected.id);
        if(c && !c->slot.IsEmpty()) Frame(w, c->slot, Color(100, 182, 230));
    }
}
void PreviewGraph::LeftDown(Point p, dword flags)
{
    for(int layer = 1; layer >= 0; layer--)
        for(int i = snapshot.components.GetCount() - 1; i >= 0; i--) {
            const auto& c = snapshot.components[i];
            bool overlay = (int)c.region >= 4 && (int)c.region <= 6;
            if((overlay ? 1 : 0) == layer && c.representation != UiGraphNodeComponentRepresentation::Hidden && c.footprint.Contains(p)) {
                WhenComponentSelect(c.id, (int)c.region); return;
            }
        }
    UiNodeGraph::LeftDown(p, flags);
}
} // namespace GraphWorkspace
} // namespace Upp
