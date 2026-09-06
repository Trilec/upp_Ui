// H2 scope-local spatial implementation.
// The pre-H2 UiNodeGraphSpatial.cpp remains in the repository as the recovery
// source while this file is Windows-validated. Ui.upp compiles exactly one of
// them; do not compile both translation units.
#include <Ui/UiGraph/UiNodeGraph.h>

#include <cmath>

namespace Upp {
namespace {

constexpr double kNodeGraphSpatialCell = 256.0;
constexpr int kNodeGraphMaxCellsPerObject = 256;
constexpr double kNodeGraphOverviewEdgeTilePx = 64.0;
constexpr int kNodeGraphOverviewMinEdgeCandidates = 512;

void RemoveId(Vector<UiGraphId>& ids, UiGraphId id)
{
    int q = FindIndex(ids, id);
    if(q >= 0)
        ids.Remove(q);
}

Pointf PortAnchorWorld(const UiGraphNode& node, UiGraphPortSide side)
{
    Pointf pos = node.position;
    Sizef size = node.size;
    int shape_wire = (int)node.shape;
    if(shape_wire == 2 || shape_wire == 3) {
        double extent = max(size.cx, size.cy);
        size = Sizef(extent, extent);
    }
    switch(side) {
    case UiGraphPortSide::Left:   return Pointf(pos.x, pos.y + size.cy * 0.5);
    case UiGraphPortSide::Right:  return Pointf(pos.x + size.cx, pos.y + size.cy * 0.5);
    case UiGraphPortSide::Top:    return Pointf(pos.x + size.cx * 0.5, pos.y);
    case UiGraphPortSide::Bottom: return Pointf(pos.x + size.cx * 0.5, pos.y + size.cy);
    case UiGraphPortSide::Auto:
    default:                      return pos + Pointf(size.cx * 0.5, size.cy * 0.5);
    }
}

int OverviewDirectionBucket(Pointf delta)
{
    constexpr double pi = 3.14159265358979323846;
    double angle = std::atan2(delta.y, delta.x);
    double normalized = (angle + pi) / (2.0 * pi);
    return minmax((int)std::floor(normalized * 8.0), 0, 7);
}

} // namespace

void UiNodeGraph::WorldRect::Include(Pointf p)
{
    if(!valid) {
        left = right = p.x;
        top = bottom = p.y;
        valid = true;
        return;
    }
    left = min(left, p.x);
    top = min(top, p.y);
    right = max(right, p.x);
    bottom = max(bottom, p.y);
}

void UiNodeGraph::WorldRect::Include(const WorldRect& r)
{
    if(!r.valid)
        return;
    Include(Pointf(r.left, r.top));
    Include(Pointf(r.right, r.bottom));
}

UiNodeGraph::WorldRect UiNodeGraph::WorldRect::Inflated(double amount) const
{
    WorldRect out = *this;
    if(out.valid) {
        amount = max(0.0, amount);
        out.left -= amount;
        out.top -= amount;
        out.right += amount;
        out.bottom += amount;
    }
    return out;
}

bool UiNodeGraph::WorldRect::Intersects(const WorldRect& r) const
{
    return valid && r.valid &&
           right >= r.left && r.right >= left &&
           bottom >= r.top && r.bottom >= top;
}

int64 UiNodeGraph::SpatialCellKey(int x, int y)
{
    return (int64)(((uint64)(uint32)x << 32) | (uint32)y);
}

void UiNodeGraph::SpatialCellRange(const WorldRect& bounds, int& x0, int& y0, int& x1, int& y1)
{
    if(!bounds.valid) {
        x0 = y0 = 0;
        x1 = y1 = -1;
        return;
    }
    x0 = (int)std::floor(bounds.left / kNodeGraphSpatialCell);
    y0 = (int)std::floor(bounds.top / kNodeGraphSpatialCell);
    x1 = (int)std::floor(bounds.right / kNodeGraphSpatialCell);
    y1 = (int)std::floor(bounds.bottom / kNodeGraphSpatialCell);
}

UiNodeGraph::WorldRect UiNodeGraph::GetNodeWorldBounds(const UiGraphNode& node) const
{
    Pointf pos = node.position;
    Sizef size = node.size;
    int shape_wire = (int)node.shape;
    if(shape_wire == 2 || shape_wire == 3) {
        double side = max(size.cx, size.cy);
        size = Sizef(side, side);
    }
    WorldRect out;
    out.Include(pos);
    out.Include(pos + Pointf(max(1.0, size.cx), max(1.0, size.cy)));
    return out;
}

UiNodeGraph::WorldRect UiNodeGraph::GetEdgeWorldBounds(const UiGraphEdge& edge) const
{
    WorldRect out;
    const UiGraphNode* source = model_ ? model_->FindNode(edge.source.node) : nullptr;
    const UiGraphNode* target = model_ ? model_->FindNode(edge.target.node) : nullptr;
    if(source)
        out.Include(GetNodeWorldBounds(*source));
    if(target)
        out.Include(GetNodeWorldBounds(*target));
    for(const Pointf& p : edge.waypoints)
        out.Include(p);

    if(!source || !target)
        return out.Inflated(32.0);

    const UiGraphPort* source_port = model_ ? model_->FindPort(edge.source) : nullptr;
    const UiGraphPort* target_port = model_ ? model_->FindPort(edge.target) : nullptr;
    UiGraphPortSide source_side = source_port ? ResolvePortSide(*source_port) : UiGraphPortSide::Right;
    UiGraphPortSide target_side = target_port ? ResolvePortSide(*target_port) : UiGraphPortSide::Left;
    Pointf a = PortAnchorWorld(*source, source_side);
    Pointf b = PortAnchorWorld(*target, target_side);
    out.Include(a);
    out.Include(b);

    const UiGraphEdgeStyle& style = FindEdgeStyleClass(edge.style_class);
    UiGraphRouteStyle route = edge.route == UiGraphRouteStyle::Inherit ? style.route : edge.route;
    double dx = b.x - a.x;
    double dy = b.y - a.y;
    double distance = std::sqrt(dx * dx + dy * dy);
    double min_zoom = max(0.01, GetEffectiveStyle().min_zoom);
    double extension_world = extension_bounds_.edge_paint_margin_px / min_zoom;
    double paint_margin = max(24.0, style.interaction_width + style.arrow_size + 8.0);
    paint_margin = max(paint_margin, extension_world);

    if(route == UiGraphRouteStyle::Custom || WhenResolveEdgeStyle)
        return out.Inflated(max(max(96.0, distance * 0.50),
                                extension_bounds_.custom_route_world_margin));
    if(route == UiGraphRouteStyle::Bezier) {
        double handle = max(24.0, distance * minmax(style.bezier_tension, 0.05, 1.25));
        out.Include(a + SideVector(source_side) * handle);
        out.Include(b + SideVector(target_side) * handle);
        return out.Inflated(paint_margin);
    }
    if(route == UiGraphRouteStyle::Orthogonal) {
        double lead = max(0.0, style.orthogonal_lead);
        out.Include(a + SideVector(source_side) * lead);
        out.Include(b + SideVector(target_side) * lead);
        return out.Inflated(paint_margin);
    }
    return out.Inflated(paint_margin);
}

UiNodeGraph::WorldRect UiNodeGraph::GetBackdropWorldBounds(const UiGraphBackdrop& backdrop) const
{
    WorldRect out;
    out.Include(backdrop.position);
    out.Include(backdrop.position + Pointf(backdrop.size.cx, backdrop.size.cy));
    return out;
}

UiNodeGraph::WorldRect UiNodeGraph::GetViewportWorldBounds(double screen_margin) const
{
    Size size = GetSize();
    WorldRect out;
    out.Include(ScreenToWorld(Point(0, 0)));
    out.Include(ScreenToWorld(Point(size.cx, size.cy)));
    double margin = max(0.0, screen_margin) / max(zoom_, 0.01);
    return out.Inflated(margin);
}

void UiNodeGraph::InvalidateSpatialIndex()
{
    spatial_dirty_ = true;
}

void UiNodeGraph::EnsureSpatialIndex()
{
    if(spatial_dirty_)
        RebuildSpatialIndex();
}

void UiNodeGraph::RebuildSpatialIndex()
{
    spatial_cells_.Clear();
    node_world_bounds_.Clear();
    edge_world_bounds_.Clear();
    backdrop_world_bounds_.Clear();
    spatial_global_nodes_.Clear();
    spatial_global_edges_.Clear();
    spatial_global_backdrops_.Clear();

    if(model_) {
        UiGraphScopeRef scope = GetScope();
        Vector<UiGraphNodeRef> nodes = model_->GetScopeNodes(scope);
        for(UiGraphNodeRef ref : nodes) {
            const UiGraphNode* node = model_->FindNode(ref);
            if(!node || !node->visible)
                continue;
            WorldRect bounds = GetNodeWorldBounds(*node);
            node_world_bounds_.Add(node->ref.id, bounds);
            AddNodeToSpatialCells(node->ref, bounds);
        }

        Vector<UiGraphEdgeRef> edges = model_->GetScopeEdges(scope);
        for(UiGraphEdgeRef ref : edges) {
            const UiGraphEdge* edge = model_->FindEdge(ref);
            if(!edge || !edge->visible)
                continue;
            WorldRect bounds = GetEdgeWorldBounds(*edge);
            edge_world_bounds_.Add(edge->ref.id, bounds);
            UiGraphRouteStyle route = edge->route;
            if(route == UiGraphRouteStyle::Inherit)
                route = FindEdgeStyleClass(edge->style_class).route;
            // Dynamic style resolution remains cell-indexed under ExtensionBounds.
        // Only a statically custom route (or a genuinely >256-cell envelope)
        // needs the global bucket.
        bool force_global = route == UiGraphRouteStyle::Custom;
            AddEdgeToSpatialCells(edge->ref, bounds, force_global);
        }

        Vector<UiGraphBackdropRef> backdrops = model_->GetScopeBackdrops(scope);
        for(UiGraphBackdropRef ref : backdrops) {
            const UiGraphBackdrop* backdrop = model_->FindBackdrop(ref);
            if(!backdrop || !backdrop->visible)
                continue;
            WorldRect bounds = GetBackdropWorldBounds(*backdrop);
            backdrop_world_bounds_.Add(backdrop->ref.id, bounds);
            AddBackdropToSpatialCells(backdrop->ref, bounds);
        }
    }

    spatial_dirty_ = false;
    spatial_build_serial_++;
}

void UiNodeGraph::AddNodeToSpatialCells(UiGraphNodeRef ref, const WorldRect& bounds)
{
    if(!ref.IsValid() || !bounds.valid)
        return;
    int x0, y0, x1, y1;
    SpatialCellRange(bounds, x0, y0, x1, y1);
    int64 cells = x1 >= x0 && y1 >= y0
                ? (int64)(x1 - x0 + 1) * (int64)(y1 - y0 + 1) : 0;
    if(cells <= 0)
        return;
    if(cells > kNodeGraphMaxCellsPerObject) {
        spatial_global_nodes_.FindAdd(ref.id);
        return;
    }
    for(int y = y0; y <= y1; y++)
        for(int x = x0; x <= x1; x++) {
            int64 key = SpatialCellKey(x, y);
            int i = spatial_cells_.Find(key);
            if(i < 0) {
                SpatialCell cell;
                spatial_cells_.Add(key, pick(cell));
                i = spatial_cells_.GetCount() - 1;
            }
            if(FindIndex(spatial_cells_[i].nodes, ref.id) < 0)
                spatial_cells_[i].nodes.Add(ref.id);
        }
}

void UiNodeGraph::AddEdgeToSpatialCells(UiGraphEdgeRef ref, const WorldRect& bounds, bool force_global)
{
    if(!ref.IsValid() || !bounds.valid)
        return;
    int x0, y0, x1, y1;
    SpatialCellRange(bounds, x0, y0, x1, y1);
    int64 cells = x1 >= x0 && y1 >= y0
                ? (int64)(x1 - x0 + 1) * (int64)(y1 - y0 + 1) : 0;
    if(force_global || cells > kNodeGraphMaxCellsPerObject) {
        spatial_global_edges_.FindAdd(ref.id);
        return;
    }
    if(cells <= 0)
        return;
    for(int y = y0; y <= y1; y++)
        for(int x = x0; x <= x1; x++) {
            int64 key = SpatialCellKey(x, y);
            int i = spatial_cells_.Find(key);
            if(i < 0) {
                SpatialCell cell;
                spatial_cells_.Add(key, pick(cell));
                i = spatial_cells_.GetCount() - 1;
            }
            if(FindIndex(spatial_cells_[i].edges, ref.id) < 0)
                spatial_cells_[i].edges.Add(ref.id);
        }
}

void UiNodeGraph::AddBackdropToSpatialCells(UiGraphBackdropRef ref, const WorldRect& bounds)
{
    if(!ref.IsValid() || !bounds.valid)
        return;
    int x0, y0, x1, y1;
    SpatialCellRange(bounds, x0, y0, x1, y1);
    int64 cells = x1 >= x0 && y1 >= y0
                ? (int64)(x1 - x0 + 1) * (int64)(y1 - y0 + 1) : 0;
    if(cells <= 0)
        return;
    if(cells > kNodeGraphMaxCellsPerObject) {
        spatial_global_backdrops_.FindAdd(ref.id);
        return;
    }
    for(int y = y0; y <= y1; y++)
        for(int x = x0; x <= x1; x++) {
            int64 key = SpatialCellKey(x, y);
            int i = spatial_cells_.Find(key);
            if(i < 0) {
                SpatialCell cell;
                spatial_cells_.Add(key, pick(cell));
                i = spatial_cells_.GetCount() - 1;
            }
            if(FindIndex(spatial_cells_[i].backdrops, ref.id) < 0)
                spatial_cells_[i].backdrops.Add(ref.id);
        }
}

void UiNodeGraph::QueryBackdropSpatial(const WorldRect& area, Index<UiGraphId>& backdrops) const
{
    backdrops.Clear();
    if(!area.valid)
        return;

    auto collect = [&](const SpatialCell& cell) {
        for(UiGraphId id : cell.backdrops) {
            int q = backdrop_world_bounds_.Find(id);
            if(q >= 0 && backdrop_world_bounds_[q].Intersects(area))
                backdrops.FindAdd(id);
        }
    };

    int x0, y0, x1, y1;
    SpatialCellRange(area, x0, y0, x1, y1);
    int64 query_cells = x1 >= x0 && y1 >= y0
                      ? (int64)(x1 - x0 + 1) * (int64)(y1 - y0 + 1) : 0;
    int64 sparse_threshold = max<int64>(1024, (int64)spatial_cells_.GetCount() * 4);
    if(query_cells > sparse_threshold) {
        for(int i = 0; i < spatial_cells_.GetCount(); i++)
            if(!spatial_cells_[i].backdrops.IsEmpty())
                collect(spatial_cells_[i]);
    }
    else {
        for(int y = y0; y <= y1; y++)
            for(int x = x0; x <= x1; x++) {
                int i = spatial_cells_.Find(SpatialCellKey(x, y));
                if(i >= 0)
                    collect(spatial_cells_[i]);
            }
    }

    for(int i = 0; i < spatial_global_backdrops_.GetCount(); i++) {
        UiGraphId id = spatial_global_backdrops_[i];
        int q = backdrop_world_bounds_.Find(id);
        if(q >= 0 && backdrop_world_bounds_[q].Intersects(area))
            backdrops.FindAdd(id);
    }
}

void UiNodeGraph::RemoveNodeFromSpatialCells(UiGraphNodeRef ref, const WorldRect& bounds)
{
    int global = spatial_global_nodes_.Find(ref.id);
    if(global >= 0) {
        spatial_global_nodes_.Remove(global);
        return;
    }
    int x0, y0, x1, y1;
    SpatialCellRange(bounds, x0, y0, x1, y1);
    for(int y = y0; y <= y1; y++)
        for(int x = x0; x <= x1; x++) {
            int i = spatial_cells_.Find(SpatialCellKey(x, y));
            if(i < 0)
                continue;
            // Keep empty slots reusable. Removing from ordered VectorMap would
            // shift unrelated cells during an ordinary local move.
            RemoveId(spatial_cells_[i].nodes, ref.id);
        }
}

void UiNodeGraph::RemoveEdgeFromSpatialCells(UiGraphEdgeRef ref, const WorldRect& bounds)
{
    int global = spatial_global_edges_.Find(ref.id);
    if(global >= 0) {
        spatial_global_edges_.Remove(global);
        return;
    }
    int x0, y0, x1, y1;
    SpatialCellRange(bounds, x0, y0, x1, y1);
    for(int y = y0; y <= y1; y++)
        for(int x = x0; x <= x1; x++) {
            int i = spatial_cells_.Find(SpatialCellKey(x, y));
            if(i < 0)
                continue;
            // Full index rebuild compacts empty slots; local movement does not.
            RemoveId(spatial_cells_[i].edges, ref.id);
        }
}

void UiNodeGraph::RemoveSpatialNode(UiGraphNodeRef ref)
{
    int i = node_world_bounds_.Find(ref.id);
    if(i >= 0) {
        WorldRect old = node_world_bounds_[i];
        RemoveNodeFromSpatialCells(ref, old);
        node_world_bounds_.Remove(i);
    }
    spatial_update_serial_++;
}

void UiNodeGraph::UpdateSpatialNode(UiGraphNodeRef ref)
{
    int i = node_world_bounds_.Find(ref.id);
    if(i >= 0)
        RemoveNodeFromSpatialCells(ref, node_world_bounds_[i]);

    const UiGraphNode* node = model_ ? model_->FindNode(ref) : nullptr;
    if(node && node->visible && model_->GetNodeScope(ref) == GetScope()) {
        WorldRect bounds = GetNodeWorldBounds(*node);
        if(i >= 0) {
            node_world_bounds_[i] = bounds;
            spatial_bounds_inplace_update_count_++;
        }
        else
            node_world_bounds_.Add(ref.id, bounds);
        AddNodeToSpatialCells(ref, bounds);
    }
    else if(i >= 0)
        node_world_bounds_.Remove(i);
    spatial_update_serial_++;
}

void UiNodeGraph::RemoveSpatialEdge(UiGraphEdgeRef ref)
{
    int i = edge_world_bounds_.Find(ref.id);
    if(i >= 0) {
        WorldRect old = edge_world_bounds_[i];
        RemoveEdgeFromSpatialCells(ref, old);
        edge_world_bounds_.Remove(i);
    }
    spatial_update_serial_++;
}

void UiNodeGraph::UpdateSpatialEdge(UiGraphEdgeRef ref)
{
    int i = edge_world_bounds_.Find(ref.id);
    if(i >= 0)
        RemoveEdgeFromSpatialCells(ref, edge_world_bounds_[i]);

    const UiGraphEdge* edge = model_ ? model_->FindEdge(ref) : nullptr;
    if(edge && edge->visible && model_->GetNodeScope(edge->source.node) == GetScope()) {
        WorldRect bounds = GetEdgeWorldBounds(*edge);
        if(i >= 0) {
            edge_world_bounds_[i] = bounds;
            spatial_bounds_inplace_update_count_++;
        }
        else
            edge_world_bounds_.Add(edge->ref.id, bounds);
        UiGraphRouteStyle route = edge->route;
        if(route == UiGraphRouteStyle::Inherit)
            route = FindEdgeStyleClass(edge->style_class).route;
        // Dynamic style resolution remains cell-indexed under ExtensionBounds.
        // Only a statically custom route (or a genuinely >256-cell envelope)
        // needs the global bucket.
        bool force_global = route == UiGraphRouteStyle::Custom;
        AddEdgeToSpatialCells(ref, bounds, force_global);
    }
    else if(i >= 0)
        edge_world_bounds_.Remove(i);
    spatial_update_serial_++;
}

void UiNodeGraph::QuerySpatial(const WorldRect& area, Index<UiGraphId>& nodes,
                               Index<UiGraphId>& edges, int flags) const
{
    nodes.Clear();
    edges.Clear();
    last_spatial_cell_probe_count_ = 0;
    last_spatial_occupied_cell_visit_count_ = 0;
    last_spatial_global_node_visit_count_ = 0;
    last_spatial_global_edge_visit_count_ = 0;
    last_spatial_raw_edge_candidate_count_ = 0;
    if(!area.valid)
        return;

    const bool want_nodes = (flags & SPATIAL_QUERY_NODES) != 0;
    const bool want_edges = (flags & SPATIAL_QUERY_EDGES) != 0;
    const bool reduce_overview = (flags & SPATIAL_QUERY_REDUCE_OVERVIEW) != 0;
    Index<UiGraphId> raw_edges;

    auto collect = [&](const SpatialCell& cell) {
        if(want_nodes)
            for(UiGraphId id : cell.nodes) {
                int q = node_world_bounds_.Find(id);
                if(q >= 0 && node_world_bounds_[q].Intersects(area))
                    nodes.FindAdd(id);
            }
        if(want_edges)
            for(UiGraphId id : cell.edges) {
                int q = edge_world_bounds_.Find(id);
                if(q >= 0 && edge_world_bounds_[q].Intersects(area))
                    raw_edges.FindAdd(id);
            }
    };

    int x0, y0, x1, y1;
    SpatialCellRange(area, x0, y0, x1, y1);
    int64 query_cells = x1 >= x0 && y1 >= y0
                      ? (int64)(x1 - x0 + 1) * (int64)(y1 - y0 + 1) : 0;
    int64 sparse_threshold = max<int64>(1024, (int64)spatial_cells_.GetCount() * 4);

    if(query_cells > sparse_threshold) {
        for(int i = 0; i < spatial_cells_.GetCount(); i++) {
            const SpatialCell& cell = spatial_cells_[i];
            if(cell.nodes.IsEmpty() && cell.edges.IsEmpty())
                continue;
            last_spatial_occupied_cell_visit_count_++;
            collect(cell);
        }
    }
    else {
        for(int y = y0; y <= y1; y++)
            for(int x = x0; x <= x1; x++) {
                last_spatial_cell_probe_count_++;
                int i = spatial_cells_.Find(SpatialCellKey(x, y));
                if(i >= 0)
                    collect(spatial_cells_[i]);
            }
    }

    if(want_nodes)
        for(int i = 0; i < spatial_global_nodes_.GetCount(); i++) {
            last_spatial_global_node_visit_count_++;
            UiGraphId id = spatial_global_nodes_[i];
            int q = node_world_bounds_.Find(id);
            if(q >= 0 && node_world_bounds_[q].Intersects(area))
                nodes.FindAdd(id);
        }

    if(want_edges)
        for(int i = 0; i < spatial_global_edges_.GetCount(); i++) {
            last_spatial_global_edge_visit_count_++;
            UiGraphId id = spatial_global_edges_[i];
            int q = edge_world_bounds_.Find(id);
            if(q >= 0 && edge_world_bounds_[q].Intersects(area))
                raw_edges.FindAdd(id);
        }

    last_spatial_raw_edge_candidate_count_ = raw_edges.GetCount();
    bool overview = want_edges && reduce_overview && model_
                 && zoom_ < lod_policy_.minimal_edge_zoom
                 && raw_edges.GetCount() > kNodeGraphOverviewMinEdgeCandidates
                 && !WhenResolveEdgeStyle;
    if(!overview) {
        for(int i = 0; i < raw_edges.GetCount(); i++)
            edges.FindAdd(raw_edges[i]);
        return;
    }

    const double world_tile = kNodeGraphOverviewEdgeTilePx / max(zoom_, 0.01);
    VectorMap<String, UiGraphId> overview_bins;
    for(int i = 0; i < raw_edges.GetCount(); i++) {
        UiGraphId id = raw_edges[i];
        const UiGraphEdge* edge = model_->FindEdge(UiGraphEdgeRef{id});
        if(!edge)
            continue;

        bool preserve = selected_edges_.Find(id) >= 0 || hot_edge_ == edge->ref
                     || selected_nodes_.Find(edge->source.node.id) >= 0
                     || selected_nodes_.Find(edge->target.node.id) >= 0;
        if(preserve) {
            edges.FindAdd(id);
            continue;
        }

        const UiGraphNode* source = model_->FindNode(edge->source.node);
        const UiGraphNode* target = model_->FindNode(edge->target.node);
        if(!source || !target) {
            edges.FindAdd(id);
            continue;
        }

        const UiGraphPort* source_port = model_->FindPort(edge->source);
        const UiGraphPort* target_port = model_->FindPort(edge->target);
        UiGraphPortSide source_side = source_port ? ResolvePortSide(*source_port) : UiGraphPortSide::Right;
        UiGraphPortSide target_side = target_port ? ResolvePortSide(*target_port) : UiGraphPortSide::Left;
        Pointf a = PortAnchorWorld(*source, source_side);
        Pointf b = PortAnchorWorld(*target, target_side);
        Pointf midpoint((a.x + b.x) * 0.5, (a.y + b.y) * 0.5);
        int bx = (int)std::floor(midpoint.x / world_tile);
        int by = (int)std::floor(midpoint.y / world_tile);
        int direction = OverviewDirectionBucket(b - a);

        String key;
        key << bx << ':' << by << ':' << direction << ':'
            << edge->style_class << ':' << (int)edge->enabled << ':' << (int)edge->directed;
        int q = overview_bins.Find(key);
        if(q < 0)
            overview_bins.Add(key, id);
        else if(id < overview_bins[q])
            overview_bins[q] = id;
    }

    for(int i = 0; i < overview_bins.GetCount(); i++)
        edges.FindAdd(overview_bins[i]);
}

UiGraphNodeRef UiNodeGraph::HitTestNodeSpatial(Point p) const
{
    UiNodeGraph* self = const_cast<UiNodeGraph*>(this);
    self->PrepareGeometry();
    self->EnsureSpatialIndex();
    last_node_hit_candidate_count_ = 0;
    if(!model_)
        return UiGraphNodeRef();

    const int r = max(max(1, DPI(2)), extension_bounds_.node_hit_margin_px);
    WorldRect area;
    area.Include(ScreenToWorld(Point(p.x - r, p.y - r)));
    area.Include(ScreenToWorld(Point(p.x + r, p.y + r)));
    Index<UiGraphId> nodes;
    Index<UiGraphId> edges;
    QuerySpatial(area, nodes, edges, SPATIAL_QUERY_NODES);
    last_node_hit_candidate_count_ = nodes.GetCount();

    Vector<int> geometry;
    geometry.Reserve(nodes.GetCount());
    for(int i = 0; i < nodes.GetCount(); i++) {
        int q = node_geometry_.Find(nodes[i]);
        if(q >= 0)
            geometry.Add(q);
    }
    Sort(geometry, [&](int a, int b) {
        const NodeGeometry& ga = node_geometry_[a];
        const NodeGeometry& gb = node_geometry_[b];
        return ga.z_order != gb.z_order ? ga.z_order < gb.z_order : ga.ref.id < gb.ref.id;
    });
    for(int i = geometry.GetCount() - 1; i >= 0; --i) {
        const NodeGeometry& g = node_geometry_[geometry[i]];
        const UiGraphNode* node = model_->FindNode(g.ref);
        if(node && PointInNodeGeometry(*node, g, p))
            return g.ref;
    }
    return UiGraphNodeRef();
}

UiGraphPortRef UiNodeGraph::HitTestPortSpatial(Point p) const
{
    UiNodeGraph* self = const_cast<UiNodeGraph*>(this);
    self->PrepareGeometry();
    self->EnsureSpatialIndex();
    last_port_hit_candidate_count_ = 0;
    if(!model_ || zoom_ < lod_policy_.port_zoom)
        return UiGraphPortRef();

    const int r = max(DPI(2), MaxPortHitRadiusForBroadPhase() + DPI(2));
    WorldRect area;
    area.Include(ScreenToWorld(Point(p.x - r, p.y - r)));
    area.Include(ScreenToWorld(Point(p.x + r, p.y + r)));
    Index<UiGraphId> nodes;
    Index<UiGraphId> edges;
    QuerySpatial(area, nodes, edges, SPATIAL_QUERY_NODES);
    last_port_hit_candidate_count_ = nodes.GetCount();

    Vector<int> geometry;
    geometry.Reserve(nodes.GetCount());
    for(int i = 0; i < nodes.GetCount(); i++) {
        int q = node_geometry_.Find(nodes[i]);
        if(q >= 0)
            geometry.Add(q);
    }
    Sort(geometry, [&](int a, int b) {
        const NodeGeometry& ga = node_geometry_[a];
        const NodeGeometry& gb = node_geometry_[b];
        return ga.z_order != gb.z_order ? ga.z_order < gb.z_order : ga.ref.id < gb.ref.id;
    });
    for(int n = geometry.GetCount() - 1; n >= 0; --n) {
        const NodeGeometry& g = node_geometry_[geometry[n]];
        const UiGraphNode* node = model_->FindNode(g.ref);
        if(!node)
            continue;
        for(int i = g.port_hits.GetCount() - 1; i >= 0; --i)
            if(g.port_hits[i].Contains(p))
                return UiGraphPortRef{node->ref, g.port_hits.GetKey(i)};
    }
    return UiGraphPortRef();
}

UiGraphEdgeRef UiNodeGraph::HitTestEdgeSpatial(Point p) const
{
    UiNodeGraph* self = const_cast<UiNodeGraph*>(this);
    self->PrepareGeometry();
    self->EnsureSpatialIndex();
    last_edge_hit_candidate_count_ = 0;
    if(!model_ || zoom_ < lod_policy_.edge_simplify_zoom)
        return UiGraphEdgeRef();

    const int r = max(DPI(2), (int)std::ceil(MaxEdgeHitWidthForBroadPhase()) + DPI(2));
    WorldRect area;
    area.Include(ScreenToWorld(Point(p.x - r, p.y - r)));
    area.Include(ScreenToWorld(Point(p.x + r, p.y + r)));
    Index<UiGraphId> nodes;
    Index<UiGraphId> edges;
    QuerySpatial(area, nodes, edges, SPATIAL_QUERY_EDGES);
    last_edge_hit_candidate_count_ = edges.GetCount();

    Vector<int> geometry;
    geometry.Reserve(edges.GetCount());
    for(int i = 0; i < edges.GetCount(); i++) {
        int q = edge_geometry_.Find(edges[i]);
        if(q >= 0)
            geometry.Add(q);
    }
    Sort(geometry);
    for(int i = geometry.GetCount() - 1; i >= 0; --i) {
        const EdgeGeometry& g = edge_geometry_[geometry[i]];
        if(!g.bounds.Contains(p))
            continue;
        const UiGraphEdge* edge = model_->FindEdge(g.ref);
        if(!edge || !edge->selectable)
            continue;
        UiGraphEdgeStyle style = ResolveEdgeStyle(*edge, GetEdgeVisualState(*edge));
        double hit_width = EffectiveEdgeHitWidth(style);
        for(int n = 1; n < g.points.GetCount(); n++)
            if(DistanceToSegment(Pointf(p.x, p.y),
                                 g.points[n - 1], g.points[n]) <= hit_width)
                return g.ref;
    }
    return UiGraphEdgeRef();
}

} // namespace Upp
