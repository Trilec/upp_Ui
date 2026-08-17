#include <Ui/UiGraph/UiNodeGraph.h>

#include <cmath>

namespace Upp {

namespace {

constexpr double kNodeGraphSpatialCell = 256.0;
constexpr int kNodeGraphMaxCellsPerObject = 256;

void RemoveId(Vector<UiGraphId>& ids, UiGraphId id)
{
    int q = FindIndex(ids, id);
    if(q >= 0)
        ids.Remove(q);
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

    Pointf a, b;
    bool have_a = false, have_b = false;
    if(source) {
        a = source->position + Pointf(source->size.cx * 0.5, source->size.cy * 0.5);
        have_a = true;
    }
    if(target) {
        b = target->position + Pointf(target->size.cx * 0.5, target->size.cy * 0.5);
        have_b = true;
    }
    double distance = have_a && have_b
                    ? std::sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y))
                    : 0.0;
    // Bezier handles can leave the endpoint bounding box. This deliberately
    // overestimates ordinary route extent; false-positive candidates are cheap,
    // while false-negative culling would be a rendering bug.
    return out.Inflated(max(96.0, distance * 0.50));
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
            if(i >= 0)
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
            if(i >= 0)
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
        edge_world_bounds_.Add(ref.id, bounds);
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
                    edges.FindAdd(id);
            }
        }

    for(int i = 0; i < spatial_global_nodes_.GetCount(); i++) {
        UiGraphId id = spatial_global_nodes_[i];
        int q = node_world_bounds_.Find(id);
        if(q >= 0 && node_world_bounds_[q].Intersects(area))
            nodes.FindAdd(id);
    }
    for(int i = 0; i < spatial_global_edges_.GetCount(); i++)
        edges.FindAdd(spatial_global_edges_[i]);
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
    if(!model_)
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
    if(!model_)
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