// Spatial hit-testing keeps the historical Square/Circle wire bytes valid for
// old graphs; use the migration names the same way UiNodeGraph.cpp does.
#define UIGRAPH_ENABLE_LEGACY_SHAPE_NAMES
#include <Ui/UiGraph/UiNodeGraph.h>
#undef UIGRAPH_ENABLE_LEGACY_SHAPE_NAMES

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
    if(node.shape == UiGraphNodeShape::Square || node.shape == UiGraphNodeShape::Circle) {
        double extent = max(size.cx, size.cy);
        size = Sizef(extent, extent);
    }

    switch(side) {
    case UiGraphPortSide::Left:
        return Pointf(pos.x, pos.y + size.cy * 0.5);
    case UiGraphPortSide::Right:
        return Pointf(pos.x + size.cx, pos.y + size.cy * 0.5);
    case UiGraphPortSide::Top:
        return Pointf(pos.x + size.cx * 0.5, pos.y);
    case UiGraphPortSide::Bottom:
        return Pointf(pos.x + size.cx * 0.5, pos.y + size.cy);
    case UiGraphPortSide::Auto:
    default:
        return pos + Pointf(size.cx * 0.5, size.cy * 0.5);
    }
}

int OverviewDirectionBucket(Pointf delta)
{
    constexpr double pi = 3.14159265358979323846;
    double angle = std::atan2(delta.y, delta.x);
    double normalized = (angle + pi) / (2.0 * pi);
    int bucket = (int)std::floor(normalized * 8.0);
    if(bucket < 0)
        bucket = 0;
    if(bucket > 7)
        bucket = 7;
    return bucket;
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

void UiNodeGraph::SpatialCellRange(const WorldRect& bounds,
                                   int& x0, int& y0, int& x1, int& y1)
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
    if(node.shape == UiGraphNodeShape::Square || node.shape == UiGraphNodeShape::Circle) {
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
    double paint_margin = max(24.0, style.interaction_width + style.arrow_size + 8.0);

    // A custom route or state-sensitive style callback can legally leave the
    // ordinary route envelope. Keep the established conservative bound for
    // those extension points; normal built-in routes use their actual envelope.
    if(route == UiGraphRouteStyle::Custom || WhenResolveEdgeStyle)
        return out.Inflated(max(96.0, distance * 0.50));

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
    spatial_global_nodes_.Clear();
    spatial_global_edges_.Clear();

    if(model_) {
        for(int i = 0; i < model_->GetNodeCount(); i++) {
            const UiGraphNode& node = model_->GetNode(i);
            if(!node.visible)
                continue;
            WorldRect bounds = GetNodeWorldBounds(node);
            node_world_bounds_.Add(node.ref.id, bounds);
            AddNodeToSpatialCells(node.ref, bounds);
        }
        for(int i = 0; i < model_->GetEdgeCount(); i++) {
            const UiGraphEdge& edge = model_->GetEdge(i);
            if(!edge.visible)
                continue;
            WorldRect bounds = GetEdgeWorldBounds(edge);
            edge_world_bounds_.Add(edge.ref.id, bounds);
            UiGraphRouteStyle route = edge.route;
            if(route == UiGraphRouteStyle::Inherit)
                route = FindEdgeStyleClass(edge.style_class).route;
            bool force_global = route == UiGraphRouteStyle::Custom || WhenResolveEdgeStyle;
            AddEdgeToSpatialCells(edge.ref, bounds, force_global);
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
                ? (int64)(x1 - x0 + 1) * (int64)(y1 - y0 + 1)
                : 0;
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

void UiNodeGraph::AddEdgeToSpatialCells(UiGraphEdgeRef ref,
                                        const WorldRect& bounds,
                                        bool force_global)
{
    if(!ref.IsValid() || !bounds.valid)
        return;
    int x0, y0, x1, y1;
    SpatialCellRange(bounds, x0, y0, x1, y1);
    int64 cells = x1 >= x0 && y1 >= y0
                ? (int64)(x1 - x0 + 1) * (int64)(y1 - y0 + 1)
                : 0;
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
            RemoveId(spatial_cells_[i].nodes, ref.id);
            if(spatial_cells_[i].nodes.IsEmpty() && spatial_cells_[i].edges.IsEmpty())
                spatial_cells_.Remove(i);
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
            RemoveId(spatial_cells_[i].edges, ref.id);
            if(spatial_cells_[i].nodes.IsEmpty() && spatial_cells_[i].edges.IsEmpty())
                spatial_cells_.Remove(i);
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
    if(i >= 0) {
        WorldRect old = node_world_bounds_[i];
        RemoveNodeFromSpatialCells(ref, old);
        node_world_bounds_.Remove(i);
    }
    const UiGraphNode* node = model_ ? model_->FindNode(ref) : nullptr;
    if(node && node->visible) {
        WorldRect bounds = GetNodeWorldBounds(*node);
        node_world_bounds_.Add(ref.id, bounds);
        AddNodeToSpatialCells(ref, bounds);
    }
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
    if(i >= 0) {
        WorldRect old = edge_world_bounds_[i];
        RemoveEdgeFromSpatialCells(ref, old);
        edge_world_bounds_.Remove(i);
    }
    const UiGraphEdge* edge = model_ ? model_->FindEdge(ref) : nullptr;
    if(edge && edge->visible) {
        WorldRect bounds = GetEdgeWorldBounds(*edge);
        edge_world_bounds_.Add(edge->ref.id, bounds);
        UiGraphRouteStyle route = edge->route;
        if(route == UiGraphRouteStyle::Inherit)
            route = FindEdgeStyleClass(edge->style_class).route;
        bool force_global = route == UiGraphRouteStyle::Custom || WhenResolveEdgeStyle;
        AddEdgeToSpatialCells(ref, bounds, force_global);
    }
    spatial_update_serial_++;
}

void UiNodeGraph::QuerySpatial(const WorldRect& area,
                               Index<UiGraphId>& nodes,
                               Index<UiGraphId>& edges) const
{
    nodes.Clear();
    edges.Clear();
    if(!area.valid)
        return;

    Index<UiGraphId> raw_edges;
    int x0, y0, x1, y1;
    SpatialCellRange(area, x0, y0, x1, y1);
    for(int y = y0; y <= y1; y++)
        for(int x = x0; x <= x1; x++) {
            int i = spatial_cells_.Find(SpatialCellKey(x, y));
            if(i < 0)
                continue;
            const SpatialCell& cell = spatial_cells_[i];
            for(UiGraphId id : cell.nodes) {
                int q = node_world_bounds_.Find(id);
                if(q >= 0 && node_world_bounds_[q].Intersects(area))
                    nodes.FindAdd(id);
            }
            for(UiGraphId id : cell.edges) {
                int q = edge_world_bounds_.Find(id);
                if(q >= 0 && edge_world_bounds_[q].Intersects(area))
                    raw_edges.FindAdd(id);
            }
        }

    for(int i = 0; i < spatial_global_nodes_.GetCount(); i++) {
        UiGraphId id = spatial_global_nodes_[i];
        int q = node_world_bounds_.Find(id);
        if(q >= 0 && node_world_bounds_[q].Intersects(area))
            nodes.FindAdd(id);
    }
    for(int i = 0; i < spatial_global_edges_.GetCount(); i++)
        raw_edges.FindAdd(spatial_global_edges_[i]);

    // Overview reduction is only part of a dirty prepared-geometry rebuild.
    // All ordinary spatial queries remain exact, so pointer hit tests, marquee
    // queries and dirty-paint lookup never receive a sampled topology set.
    // The semantic model is untouched; this only bounds the number of retained
    // EdgeGeometry records prepared for the minimum zoom overview.
    bool overview = geometry_dirty_ && model_ &&
                    zoom_ < lod_policy_.minimal_edge_zoom &&
                    raw_edges.GetCount() > kNodeGraphOverviewMinEdgeCandidates &&
                    !WhenResolveEdgeStyle;
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

        bool preserve = selected_edges_.Find(id) >= 0 ||
                        hot_edge_ == edge->ref ||
                        selected_nodes_.Find(edge->source.node.id) >= 0 ||
                        selected_nodes_.Find(edge->target.node.id) >= 0;
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
            << edge->style_class << ':' << (int)edge->enabled << ':'
            << (int)edge->directed;
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

    const int r = max(1, DPI(2));
    WorldRect area;
    area.Include(ScreenToWorld(Point(p.x - r, p.y - r)));
    area.Include(ScreenToWorld(Point(p.x + r, p.y + r)));
    Index<UiGraphId> nodes;
    Index<UiGraphId> edges;
    QuerySpatial(area, nodes, edges);
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

    // Broad enough for custom per-style hit radii while still touching only a
    // handful of hash cells even at low zoom. Exact port rects decide the hit.
    const int r = max(12, DPI(32));
    WorldRect area;
    area.Include(ScreenToWorld(Point(p.x - r, p.y - r)));
    area.Include(ScreenToWorld(Point(p.x + r, p.y + r)));
    Index<UiGraphId> nodes;
    Index<UiGraphId> edges;
    QuerySpatial(area, nodes, edges);
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

    const int r = max(10, DPI(24));
    WorldRect area;
    area.Include(ScreenToWorld(Point(p.x - r, p.y - r)));
    area.Include(ScreenToWorld(Point(p.x + r, p.y + r)));
    Index<UiGraphId> nodes;
    Index<UiGraphId> edges;
    QuerySpatial(area, nodes, edges);
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
        for(int n = 1; n < g.points.GetCount(); n++)
            if(DistanceToSegment(Pointf(p.x, p.y),
                                 Pointf(g.points[n - 1].x, g.points[n - 1].y),
                                 Pointf(g.points[n].x, g.points[n].y)) <= style.interaction_width)
                return g.ref;
    }
    return UiGraphEdgeRef();
}

} // namespace Upp
