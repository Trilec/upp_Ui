#include "WorkspaceViews.h"

namespace Upp {
namespace GraphWorkspace {
void DrawWorkspaceFrame(Draw& w, Rect r, Color c, int n)
{
    if(r.IsEmpty() || n <= 0) return;
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
    else if(!r.data_key.IsEmpty()) s << " / " << r.data_key;
    const auto& cs = r.component_style;
    bool styled = !IsNull(r.ink) || r.font_height || !cs.font_face.IsEmpty()
               || cs.role != UiGraphNodeComponentRole::Inherit || cs.bold >= 0
               || cs.italic >= 0 || cs.underline >= 0 || cs.padding || cs.frame_width || cs.radius;
    for(int i = 0; i < 4; i++) styled |= !IsNull(cs.ink[i]) || !IsNull(cs.face[i]) || !IsNull(cs.frame[i]);
    if(styled) s << " / styled";
    return s;
}

String ComponentOutcome(const UiGraphNodeComponentPresentation* c)
{
    if(!c) return "Not prepared";
    using Rep = UiGraphNodeComponentRepresentation;
    const char* name = "Hidden";
    switch(c->representation) {
    case Rep::Text: name = "Text"; break;
    case Rep::Icon: name = "Icon"; break;
    case Rep::Bar: name = "Bar"; break;
    case Rep::Dot: name = "Dot"; break;
    case Rep::Image: name = "Image"; break;
    case Rep::Progress: name = "Progress"; break;
    case Rep::Fields: name = "Fields"; break;
    case Rep::Tags: name = "Tags"; break;
    case Rep::Actions: name = "Actions"; break;
    case Rep::Mosaic: name = "Mosaic"; break;
    default: break;
    }
    String out = name;
    using Reason = UiGraphNodeComponentReason;
    switch(c->reason) {
    case Reason::PolicyOff: out << ": LOD off"; break;
    case Reason::MissingData: out << ": missing data"; break;
    case Reason::InvalidData: out << ": invalid data"; break;
    case Reason::NoSpace: out << ": no room"; break;
    case Reason::TooSmall: out << ": below readable size"; break;
    case Reason::Budget: out << ": Micro budget"; break;
    case Reason::AssetNotReady: out << ": asset not ready"; break;
    default:
        if(c->representation == Rep::Text) out << " " << c->font.GetHeight() << "px";
        else if(c->representation == Rep::Bar || c->representation == Rep::Dot) out << ": simplified";
        break;
    }
    return out;
}

// These are authoring callout targets only. Actual node regions still come
// exclusively from the retained production presentation.
int RegionView::ShelfColumns() const
{
    return max(1, (GetSize().cx - DPI(12) + DPI(4)) / DPI(64));
}
int RegionView::ShelfRows() const
{
    int count = 0;
    for(int r = 0; r < 8; r++)
        if(overlay_ == (r >= 4 && r <= 6) && RegionRect(presentation_, r).IsEmpty()) count++;
    return (count + ShelfColumns() - 1) / ShelfColumns();
}
Rect RegionView::Board() const
{
    Size size = GetSize();
    int bottom = max(DPI(12), size.cy - DPI(18) - ShelfRows() * DPI(26));
    return Rect(DPI(14), DPI(12), max(DPI(14), size.cx - DPI(14)), bottom);
}
bool RegionView::Active(const Target& target) const
{
    for(const auto& c : presentation_.components)
        if((int)c.region == target.region && (target.id.IsEmpty() || c.id == target.id)
           && c.representation != UiGraphNodeComponentRepresentation::Hidden) return true;
    return false;
}
Point RegionView::Project(Pointf p) const
{
    Rect board = Board();
    if(surface_.IsEmpty() || board.IsEmpty()) return Point(0, 0);
    double scale = min((double)board.GetWidth() / surface_.GetWidth(),
                       (double)board.GetHeight() / surface_.GetHeight());
    double x = board.left + (board.GetWidth() - surface_.GetWidth() * scale) * 0.5;
    double y = board.top + (board.GetHeight() - surface_.GetHeight() * scale) * 0.5;
    return Point(fround(x + (p.x - surface_.left) * scale), fround(y + (p.y - surface_.top) * scale));
}
Rect RegionView::Project(Rect r) const
{
    return r.IsEmpty() || Board().IsEmpty() ? Rect() : Rect(Project(Pointf(r.left, r.top)), Project(Pointf(r.right, r.bottom)));
}
void RegionView::Set(const UiGraphNodeTemplate& spec, const UiGraphNodePresentation& p,
                     const Vector<Pointf>& path, Rect surface, const WorkspaceSelection& selection)
{
    spec_ = spec; presentation_ = p; path_ = clone(path); surface_ = surface; selected_ = selection;
    Layout(); Refresh();
}
void RegionView::Layout()
{
    targets_.Clear(); hover_ = -1;
    int empty_count = 0;
    for(int r = 0; r < 8; r++) {
        if(overlay_ != (r >= 4 && r <= 6)) continue;
        Target& t = targets_.Add(); t.region = r; t.rect = Project(RegionRect(presentation_, r));
        if(t.rect.IsEmpty()) {
            t.empty = true;
            int columns = ShelfColumns();
            int cell = max(1, (GetSize().cx - DPI(12) + DPI(4)) / columns);
            int y = GetSize().cy - DPI(14) - ShelfRows() * DPI(26);
            t.rect = RectC(DPI(6) + (empty_count % columns) * cell,
                           max(0, y) + (empty_count / columns) * DPI(26),
                           max(0, cell - DPI(4)), DPI(22)) & Rect(GetSize());
            empty_count++;
        }
    }
    for(const auto& c : presentation_.components) {
        bool overlay = (int)c.region >= 4 && (int)c.region <= 6;
        if(overlay != overlay_ || c.slot.IsEmpty()) continue;
        Rect r = Project(c.slot).Deflated(DPI(3));
        if(r.GetHeight() < DPI(12) || r.GetWidth() < DPI(12)) continue;
        Target& t = targets_.Add(); t.region = (int)c.region; t.id = c.id;
        // Leave the structural heading visible when the region has room.
        int height = min(DPI(20), r.GetHeight());
        t.rect = RectC(r.left, r.bottom - height, r.GetWidth(), height);
    }
}
int RegionView::Hit(Point p) const
{
    if(!Rect(GetSize()).Contains(p)) return -1;
    // A body-side port lane may share a Content column rectangle. It is still
    // graph-owned chrome, not permission to drop a component over its labels.
    for(const Rect& lane : presentation_.port_lanes)
        if(Project(lane).Contains(p)) return -1;
    for(int i = targets_.GetCount() - 1; i >= 0; i--)
        if(targets_[i].rect.Contains(p)) return i;
    return -1;
}
void RegionView::Paint(Draw& w)
{
    w.DrawRect(GetSize(), Color(250, 252, 254));
    DrawWorkspaceFrame(w, Rect(GetSize()), Color(215, 224, 233));
    for(int i = 0; i < path_.GetCount(); i++)
        w.DrawLine(Project(path_[i]), Project(path_[(i + 1) % path_.GetCount()]), 1, Color(143, 156, 170));
    DrawWorkspaceFrame(w, Project(presentation_.safe), Color(171, 182, 193));
    DrawWorkspaceFrame(w, Project(presentation_.body), Color(239, 125, 34));
    Font font = StdFont().Height(DPI(10));
    for(int i = 0; i < targets_.GetCount(); i++) {
        const auto& t = targets_[i]; if(t.rect.IsEmpty()) continue;
        bool active = Active(t);
        Color color = active ? RegionColor(t.region) : Color(174, 185, 195);
        bool selected = selected_.id.IsEmpty() ? t.id.IsEmpty() && selected_.region == t.region : t.id == selected_.id;
        if(!t.id.IsEmpty()) {
            w.DrawRect(t.rect, selected ? Color(209, 234, 250) : active ? Color(238, 248, 253) : Color(240, 243, 246));
            int n = spec_.FindComponent(t.id);
            String label = n >= 0 ? spec_.slots[n].label : t.id;
            if(label.IsEmpty()) label = t.id;
            w.Clip(t.rect); w.DrawText(t.rect.left + DPI(3), t.rect.top + DPI(2), label, font, active ? Color(40, 94, 130) : Color(126, 139, 152)); w.End();
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
        DrawWorkspaceFrame(w, t.rect, selected || hover_ == i ? Color(12, 127, 211) : color, selected || hover_ == i ? DPI(2) : 1);
    }
    // Semantic port reservations are non-drop graph chrome.
    for(const Rect& lane : presentation_.port_lanes) DrawWorkspaceFrame(w, Project(lane), Color(19, 160, 216));
    w.DrawText(DPI(6), max(0, GetSize().cy - DPI(11)), "Grey: inactive. +: unreserved region.", font.Height(DPI(9)), Color(102, 117, 135));
}
void RegionView::LeftDown(Point p, dword)
{
    int i = Hit(p);
    if(i < 0) return;
    const Target target = targets_[i]; // selection may rebuild the target array
    WhenSelect(target.id, target.region);
    SetFocus(); // clicking a painted surface must transfer keyboard ownership
}
void RegionView::LeftDrag(Point p, dword)
{
    int i = Hit(p); if(i >= 0 && !targets_[i].id.IsEmpty()) WhenDrag(targets_[i].id);
}
void RegionView::DragAndDrop(Point p, PasteClip& d)
{
    int i = Hit(p); hover_ = -1;
    if(i >= 0 && WhenDrop) {
        Target target = targets_[i]; // callbacks can refresh the target array
        bool accepted = WhenDrop(d, target.region, target.id);
        int current = Hit(p);
        if(accepted && !d.IsPaste() && current >= 0
           && targets_[current].region == target.region && targets_[current].id == target.id) hover_ = current;
    }
    Refresh();
}

StructureView::StructureView()
{
    AddFrame(horizontal_.Horz()); AddFrame(scroll_);
    horizontal_.WhenScroll = scroll_.WhenScroll = [this] { drop_row_ = -1; Refresh(); };
    BackPaint(); WantFocus();
}
void StructureView::Set(const UiGraphNodeTemplate& spec, const UiGraphNodePresentation& p, const WorkspaceSelection& selected)
{
    spec_ = spec; presentation_ = p; selected_ = selected;
    rows_.Clear(); drop_row_ = -1;
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
    horizontal_.SetTotal(CanvasWidth()); horizontal_.SetPage(max(0, GetSize().cx));
    scroll_.SetTotal(rows_.GetCount() * RowHeight()); scroll_.SetPage(max(0, GetSize().cy - DPI(30)));
}
int StructureView::Hit(Point p) const
{
    if(!Rect(GetSize()).Contains(p) || p.y < DPI(30)) return -1;
    int i = (p.y - DPI(30) + scroll_.Get()) / RowHeight();
    return i >= 0 && i < rows_.GetCount() ? i : -1;
}
void StructureView::Paint(Draw& w)
{
    w.DrawRect(GetSize(), White());
    const int offset = horizontal_.Get();
    const int columns = Columns(), placement = min(DPI(310), columns * 3 / 5);
    const int width = CanvasWidth();
    Font font = StdFont().Height(DPI(11));
    // Header and rows use the same virtual coordinates. At narrow widths every
    // LOD remains reachable through the native horizontal scrollbar.
    auto text = [&](Rect area, const String& value, Font f, Color ink) {
        area.Offset(-offset, 0);
        if(area.IsEmpty()) return;
        w.Clip(area);
        DrawTextEllipsis(w, area.left + DPI(5), area.top + max(0, (area.GetHeight() - f.GetCy()) / 2),
                         max(0, area.GetWidth() - DPI(10)), value, "...", f, ink);
        w.End();
    };
    w.DrawRect(0, 0, GetSize().cx, DPI(30), Color(247, 249, 252));
    text(RectC(0, 0, placement, DPI(30)), "STRUCTURE / COMPONENT", font.Bold(), Color(90, 110, 132));
    text(RectC(placement, 0, columns - placement, DPI(30)), "PLACEMENT / FEATURES", font.Bold(), Color(90, 110, 132));
    for(int l = 0; l < 4; l++) {
        Rect box = RectC(columns + l * DPI(62), DPI(4), DPI(58), DPI(21));
        w.DrawRect(box.Offseted(-offset, 0), l < 2 ? Color(12, 127, 211) : Color(125, 166, 204));
        text(box, l ? "LOD " + AsString(l) : String("Normal"), font, White());
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
        int indent = DPI(7) + r.depth * DPI(15);
        text(RectC(indent, y, max(0, placement - indent - DPI(4)), RowHeight()), r.name,
             r.id.IsEmpty() ? font.Bold() : font, Color(42, 62, 82));
        if(n >= 0) {
            const auto& c = spec_.slots[n];
            text(RectC(placement, y, columns - placement - DPI(4), RowHeight()),
                 ComponentOutcome(presentation_.FindComponent(c.id)) + " | " + PlacementSummary(c),
                 font.Height(DPI(10)), Color(93, 110, 127));
        }
        for(int l = 0; l < 4; l++) {
            String label; Color ink(105, 123, 139), face(238, 242, 246), border(212, 220, 228);
            if(n >= 0) {
                const auto& c = spec_.slots[n]; byte bit = byte(1u << l);
                label = c.force_off & bit ? "Off" : c.force_on & bit ? "On" : c.Allows((UiGraphPresentationLevel)l) ? "Auto +" : "Auto -";
                bool on = c.Allows((UiGraphPresentationLevel)l);
                ink = on ? Color(36, 124, 72) : Color(181, 43, 34);
                face = on ? Color(234, 248, 239) : Color(255, 240, 238);
                border = on ? Color(159, 210, 174) : Color(239, 184, 178);
            }
            else {
                int total = 0, on = 0;
                for(int k = 0; k < spec_.slot_count; k++) if(r.mask & (1 << (int)spec_.slots[k].region)) { total++; on += spec_.slots[k].Allows((UiGraphPresentationLevel)l); }
                label = total ? AsString(on) + "/" + AsString(total) : String("-");
            }
            Rect badge = RectC(columns + l * DPI(62) + DPI(3), y + DPI(3), DPI(52), RowHeight() - DPI(6));
            w.DrawRect(badge.Offseted(-offset, 0), face); DrawWorkspaceFrame(w, badge.Offseted(-offset, 0), border);
            text(badge, label, font.Height(DPI(10)), ink);
        }
        if(drop_row_ == i && !r.id.IsEmpty())
            w.DrawRect(-offset, y, width, DPI(2), Color(12, 127, 211)); // insertion before component
    }
    w.End();
}
void StructureView::LeftDown(Point p, dword)
{
    int i = Hit(p); if(i < 0 || rows_[i].region < 0) return;
    Row row = rows_[i];
    int x = p.x + horizontal_.Get();
    int lod = x >= Columns() ? (x - Columns()) / DPI(62) : -1;
    WhenSelect(row.id, row.region);
    if(!row.id.IsEmpty() && lod >= 0 && lod < 4) WhenLod(row.id, lod);
    SetFocus();
}
void StructureView::LeftDrag(Point p, dword)
{
    int i = Hit(p);
    if(i >= 0 && p.x + horizontal_.Get() < Columns() && !rows_[i].id.IsEmpty()) {
        String id = rows_[i].id; WhenDrag(id);
    }
}
void StructureView::MouseWheel(Point, int z, dword)
{
    scroll_.Set(scroll_.Get() - z / 120 * RowHeight() * 3); Refresh();
}
void StructureView::HorzMouseWheel(Point, int z, dword)
{
    horizontal_.Set(horizontal_.Get() - z / 120 * DPI(40)); Refresh();
}
void StructureView::DragAndDrop(Point p, PasteClip& d)
{
    int i = Hit(p); drop_row_ = -1;
    if(i >= 0 && rows_[i].region >= 0 && WhenDrop) {
        Row row = rows_[i];
        bool accepted = WhenDrop(d, row.region, row.id);
        int current = Hit(p);
        if(accepted && !d.IsPaste() && current >= 0
           && rows_[current].region == row.region && rows_[current].id == row.id) drop_row_ = current;
    }
    Refresh();
}
void PreviewGraph::Paint(Draw& w)
{
    UiNodeGraph::Paint(w);
    if(!selected.id.IsEmpty()) {
        const auto* c = snapshot.FindComponent(selected.id);
        if(c && !c->slot.IsEmpty()) DrawWorkspaceFrame(w, c->slot, Color(100, 182, 230));
    }
}
void PreviewGraph::LeftDown(Point p, dword flags)
{
    // Prefer exact painted footprints. Then permit a small, slot-clipped hit
    // tolerance for a thin bar/dot; do not make a whole Fill slot steal clicks.
    for(int pass = 0; pass < 2; pass++)
        for(int layer = 1; layer >= 0; layer--)
            for(int i = snapshot.components.GetCount() - 1; i >= 0; i--) {
                const auto& c = snapshot.components[i];
                bool overlay = (int)c.region >= 4 && (int)c.region <= 6;
                if((overlay ? 1 : 0) != layer
                   || c.representation == UiGraphNodeComponentRepresentation::Hidden
                   || c.footprint.IsEmpty()) continue;
                Rect hit = (pass ? c.footprint.Deflated(-DPI(3)) : c.footprint)
                         & c.slot & snapshot.safe;
                if(hit.Contains(p)) {
                    String id = c.id; int region = (int)c.region;
                    WhenComponentSelect(id, region); // may replace snapshot
                    SetFocus();
                    return;
                }
            }
    UiNodeGraph::LeftDown(p, flags);
}
} // namespace GraphWorkspace
} // namespace Upp
