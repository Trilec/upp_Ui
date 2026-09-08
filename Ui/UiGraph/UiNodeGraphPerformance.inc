namespace Upp {
namespace {

constexpr double kUiGraphPi = 3.14159265358979323846;
constexpr int kLodMicroRasterCacheLimit = 32;
constexpr int kLodMicroPathCacheLimit = 64;

struct ViewNodeOrderRecord : Moveable<ViewNodeOrderRecord> {
    const UiGraphNode* node = nullptr;
    UiGraphNodeRef ref;
    int z_order = 0;
};

Vector<Pointf> LodNodeShapePath(const UiGraphNode& node, const Rect& rect)
{
    Vector<Pointf> v;
    double sx = node.size.cx > 0.0 ? rect.GetWidth() / node.size.cx : 1.0;
    double sy = node.size.cy > 0.0 ? rect.GetHeight() / node.size.cy : 1.0;
    double radius = max(0.0, node.corner_radius * max(0.01, min(sx, sy)));
    double l = rect.left, t = rect.top, r = rect.right, b = rect.bottom;
    double cx = (l + r) * 0.5, cy = (t + b) * 0.5;

    switch(node.shape) {
    case UiGraphNodeShape::Circle:
    case UiGraphNodeShape::Ellipse:
        return UiGeometry::EllipsePath(rect);
    case UiGraphNodeShape::Diamond:
        v << Pointf(cx, t) << Pointf(r, cy) << Pointf(cx, b) << Pointf(l, cy);
        return UiGeometry::RoundedPolygon(v, radius);
    case UiGraphNodeShape::Triangle:
        v << Pointf(cx, t) << Pointf(r, b) << Pointf(l, b);
        return UiGeometry::RoundedPolygon(v, radius);
    case UiGraphNodeShape::Hexagon: {
        double dx = rect.GetWidth() * 0.24;
        v << Pointf(l + dx, t) << Pointf(r - dx, t) << Pointf(r, cy)
          << Pointf(r - dx, b) << Pointf(l + dx, b) << Pointf(l, cy);
        return UiGeometry::RoundedPolygon(v, radius);
    }
    case UiGraphNodeShape::Capsule:
        return UiGeometry::CapsulePath(rect);
    case UiGraphNodeShape::Cloud: {
        double w = rect.GetWidth(), h = rect.GetHeight();
        v << Pointf(l + w * 0.08, t + h * 0.62)
          << Pointf(l + w * 0.05, t + h * 0.42)
          << Pointf(l + w * 0.20, t + h * 0.28)
          << Pointf(l + w * 0.36, t + h * 0.30)
          << Pointf(l + w * 0.45, t + h * 0.10)
          << Pointf(l + w * 0.66, t + h * 0.12)
          << Pointf(l + w * 0.75, t + h * 0.30)
          << Pointf(l + w * 0.92, t + h * 0.36)
          << Pointf(l + w * 0.95, t + h * 0.60)
          << Pointf(l + w * 0.82, t + h * 0.80)
          << Pointf(l + w * 0.18, t + h * 0.82);
        return UiGeometry::RoundedPolygon(v, max(radius, min(w, h) * 0.10));
    }
    case UiGraphNodeShape::Document: {
        double fold = min(rect.GetWidth(), rect.GetHeight()) * 0.22;
        v << Pointf(l, t) << Pointf(r - fold, t) << Pointf(r, t + fold)
          << Pointf(r, b) << Pointf(l, b);
        return UiGeometry::RoundedPolygon(v, radius);
    }
    case UiGraphNodeShape::Database:
        v << Pointf(l, t + rect.GetHeight() * 0.14)
          << Pointf(l + rect.GetWidth() * 0.18, t)
          << Pointf(r - rect.GetWidth() * 0.18, t)
          << Pointf(r, t + rect.GetHeight() * 0.14)
          << Pointf(r, b - rect.GetHeight() * 0.14)
          << Pointf(r - rect.GetWidth() * 0.18, b)
          << Pointf(l + rect.GetWidth() * 0.18, b)
          << Pointf(l, b - rect.GetHeight() * 0.14);
        return UiGeometry::RoundedPolygon(v, max(radius, rect.GetHeight() * 0.08));
    case UiGraphNodeShape::Rectangle:
    case UiGraphNodeShape::Custom:
        v << Pointf(l, t) << Pointf(r, t) << Pointf(r, b) << Pointf(l, b);
        return v;
    case UiGraphNodeShape::Square:
    case UiGraphNodeShape::RoundedRectangle:
    default:
        v << Pointf(l, t) << Pointf(r, t) << Pointf(r, b) << Pointf(l, b);
        return UiGeometry::RoundedPolygon(v, radius);
    }
}

void LodIntegerPath(const Vector<Pointf>& path, Vector<Point>& out)
{
    // Paint can visit thousands of micro nodes in one frame. SetCount(0)
    // preserves the caller-owned Vector capacity; Clear() releases it in U++.
    out.SetCount(0);
    out.Reserve(path.GetCount());
    for(const Pointf& p : path) {
        Point q(fround(p.x), fround(p.y));
        if(out.IsEmpty() || out.Top() != q)
            out.Add(q);
    }
    if(out.GetCount() > 1 && out.Top() == out[0])
        out.Drop();
}

// Painter's default miter limit is 4.0. Reserve the worst-case half-stroke
// miter extension plus AA coverage so cached sharp joins and dirty bounds agree.
int LodMicroStrokePad(int frame_width)
{
    if(frame_width <= 0)
        return 1;
    return max(1, frame_width * 2 + 2);
}

struct LodMicroRasterKey : Moveable<LodMicroRasterKey> {
    int shape = 0;
    Size surface_size;
    Color face;
    Color frame;
    int frame_width = 0;
    Vector<Pointf> local_path;

    bool Matches(const UiGraphNode& node,
                 const Vector<Pointf>& retained_path,
                 const Rect& surface,
                 Color paint_face, Color paint_frame, int paint_frame_width) const
    {
        if(shape != (int)(byte)node.shape
           || surface_size != surface.GetSize()
           || face != paint_face
           || frame != paint_frame
           || frame_width != paint_frame_width
           || local_path.GetCount() != retained_path.GetCount())
            return false;

        for(int i = 0; i < local_path.GetCount(); i++) {
            Pointf q(retained_path[i].x - surface.left,
                     retained_path[i].y - surface.top);
            if(local_path[i] != q)
                return false;
        }
        return true;
    }

    void Set(const UiGraphNode& node,
             const Vector<Pointf>& retained_path,
             const Rect& surface,
             Color paint_face, Color paint_frame, int paint_frame_width)
    {
        shape = (int)(byte)node.shape;
        surface_size = surface.GetSize();
        face = paint_face;
        frame = paint_frame;
        frame_width = paint_frame_width;
        local_path.Clear();
        local_path.Reserve(retained_path.GetCount());
        for(const Pointf& p : retained_path)
            local_path.Add(Pointf(p.x - surface.left,
                                 p.y - surface.top));
    }
};

struct LodMicroRasterEntry : Moveable<LodMicroRasterEntry> {
    LodMicroRasterKey key;
    Image image;
    int pad = 0;
};

Image LodMicroPolygonRaster(const UiGraphNode& node,
                            const Vector<Pointf>& retained_path,
                            const Rect& surface,
                            Color face, Color frame, int frame_width,
                            int pad)
{
    if(surface.IsEmpty() || retained_path.GetCount() < 3)
        return Image();

    Size raster_size(surface.GetWidth() + pad * 2,
                     surface.GetHeight() + pad * 2);
    if(raster_size.cx <= 0 || raster_size.cy <= 0)
        return Image();

    UiRasterCachePolicy policy = UiRasterPolicyAA("aa/nodegraph-micro-shape");
    policy.allow_scale_from_bucket = false;
    policy.max_axis = 128;
    policy.max_single_image_bytes = 256 * 1024;

    // UiRasterCache::Get() validates image bytes only after the factory runs and
    // does not enforce max_axis. Reject before ImageBuffer construction.
    Size admitted_size = UiQuantizeRasterSize(raster_size, policy);
    if(admitted_size.cx <= 0 || admitted_size.cy <= 0) {
        UiRasterCache::NoteSkippedTooLarge();
        return Image();
    }

    UiRasterCacheKeyBuilder kb("aa/nodegraph-micro-shape");
    kb.Add((int)(byte)node.shape)
      .Add(surface.GetSize())
      .Add(face)
      .Add(frame)
      .Add(frame_width)
      .Add(retained_path.GetCount())
      .Add(pad);
    // The retained local path is the final-pixel rendering authority. Include
    // every exact local coordinate so dynamic style resolution, corner-radius
    // changes and subpixel phase cannot alias two visibly different rasters.
    for(const Pointf& p : retained_path) {
        kb.Add(p.x - surface.left);
        kb.Add(p.y - surface.top);
    }

    return UiRasterCache::Get(kb.Build(), policy, [&] {
        ImageBuffer ib(raster_size);
        ib.SetKind(IMAGE_ALPHA);
        Fill(~ib, RGBAZero(), ib.GetLength());

        Vector<Pointf> local;
        local.Reserve(retained_path.GetCount());
        for(const Pointf& p : retained_path)
            local.Add(Pointf(p.x - surface.left + pad,
                             p.y - surface.top + pad));

        BufferPainter painter(ib, MODE_ANTIALIASED);
        FillPath(painter, local, face);
        if(frame_width > 0 && !IsNull(frame))
            StrokePath(painter, local, (double)frame_width, frame, true);
        painter.Finish();
        return Image(ib);
    });
}

} // namespace

void UiNodeGraph::PaintGraphGeometry(Draw& w)
{
    last_paint_path_ = PaintPath::None;
    last_paint_fallback_reason_ = PaintFallbackReason::None;
    last_painted_port_count_ = 0;
    auto rich = [&](PaintFallbackReason reason) {
        last_paint_path_ = PaintPath::Rich;
        last_paint_fallback_reason_ = reason;
        PaintGraphGeometryRenderBase(w);
    };
    last_micro_raster_count_ = 0;
    last_micro_cached_draw_count_ = 0;
    last_micro_direct_fallback_count_ = 0;
    Size size = GetSize();
    if(size.cx <= 0 || size.cy <= 0 || !model_) {
        rich(PaintFallbackReason::EmptyScene);
        return;
    }
    Rect viewport(Point(0, 0), size);
    Rect paint = w.GetPaintRect() & viewport;
    if(paint.IsEmpty())
        return;

    // This fast path is deliberately narrow: normal viewing of projected-micro
    // nodes below edge-label/detail scale. Rich/reference nodes, active gestures,
    // custom body/foreground/overlay hooks and non-direct arrow styles retain the
    // accepted renderer unchanged.
    // Camera-only pan has the same presentation as idle. Semantic gestures
    // retain the established rich overlay path; record why before doing work.
    PaintFallbackReason reason = PaintFallbackReason::None;
    if(interaction_ != InteractionMode::None && interaction_ != InteractionMode::Pan)
        reason = PaintFallbackReason::SemanticGesture;
    else if(!marquee_preview_nodes_.IsEmpty())
        reason = PaintFallbackReason::MarqueePreview;
    else if(zoom_ >= lod_policy_.edge_label_zoom)
        reason = PaintFallbackReason::DetailScale;
    else if(WhenPaintNodeBackground || WhenPaintNodeForeground || WhenPaintNodeOverlay)
        reason = PaintFallbackReason::CustomPaint;
    if(reason != PaintFallbackReason::None) {
        rich(reason);
        return;
    }

    WorldRect paint_world;
    paint_world.Include(ScreenToWorld(paint.TopLeft()));
    paint_world.Include(ScreenToWorld(paint.BottomRight()));
    paint_world = paint_world.Inflated(GetPaintQueryMargin() / max(zoom_, 0.01));
    Index<UiGraphId> node_ids;
    Index<UiGraphId> edge_ids;
    QuerySpatial(paint_world, node_ids, edge_ids,
                 SPATIAL_QUERY_NODES | SPATIAL_QUERY_EDGES | SPATIAL_QUERY_REDUCE_OVERVIEW);

    Vector<int> paint_nodes;
    paint_nodes.Reserve(node_ids.GetCount());
    for(int i = 0; i < node_ids.GetCount(); i++) {
        int q = node_geometry_.Find(node_ids[i]);
        if(q < 0 || (node_geometry_[q].paint_bounds & paint).IsEmpty())
            continue;
        if(!node_geometry_[q].micro) {
            rich(PaintFallbackReason::NonMicroNode);
            return;
        }
        const UiGraphNode* node = model_->FindNode(node_geometry_[q].ref);
        if(!node || node->shape == UiGraphNodeShape::Custom) {
            rich(PaintFallbackReason::CustomShape);
            return;
        }
        paint_nodes.Add(q);
    }
    if(paint_nodes.IsEmpty()) {
        rich(PaintFallbackReason::NoMicroNodes);
        return;
    }
    Sort(paint_nodes, [&](int a, int b) {
        const NodeGeometry& ga = node_geometry_[a];
        const NodeGeometry& gb = node_geometry_[b];
        return ga.z_order != gb.z_order ? ga.z_order < gb.z_order : ga.ref.id < gb.ref.id;
    });

    Vector<int> paint_edges;
    Vector<UiGraphEdgeStyle> paint_edge_styles;
    Vector<UiGraphVisualState> paint_edge_states;
    paint_edges.Reserve(edge_ids.GetCount());
    paint_edge_styles.Reserve(edge_ids.GetCount());
    paint_edge_states.Reserve(edge_ids.GetCount());
    if(zoom_ >= lod_policy_.edge_hide_zoom)
        for(int i = 0; i < edge_ids.GetCount(); i++) {
            int q = edge_geometry_.Find(edge_ids[i]);
            if(q < 0 || (edge_geometry_[q].bounds & paint).IsEmpty())
                continue;
            const UiGraphEdge* edge = model_->FindEdge(edge_geometry_[q].ref);
            if(!edge)
                continue;
            UiGraphVisualState state = GetEdgeVisualState(*edge);
            UiGraphEdgeStyle style = ResolveEdgeStyle(*edge, state);
            if(UiNodeGraphDetail::ResolveEdgePaintBackend(*edge, style,
                    edge_geometry_[q].simplified, zoom_, lod_policy_)
               == UiNodeGraphDetail::EdgePaintBackend::Painter) {
                rich(PaintFallbackReason::PainterEdge);
                return;
            }
            paint_edges.Add(q);
            paint_edge_styles.Add(pick(style));
            paint_edge_states.Add(state);
        }

    last_paint_path_ = PaintPath::Micro;
    last_paint_node_visit_count_ = paint_nodes.GetCount();
    last_paint_edge_visit_count_ = paint_edges.GetCount();
    last_painted_node_count_ = 0;
    last_painted_edge_count_ = 0;
    last_simplified_edge_count_ = 0;
    last_edge_paint_usecs_ = 0;
    last_node_paint_usecs_ = 0;
    last_node_surface_paint_usecs_ = 0;
    last_node_details_paint_usecs_ = 0;
    last_node_content_paint_usecs_ = 0;

    int64 edge_started = usecs();
    for(int i = 0; i < paint_edges.GetCount(); i++) {
        const EdgeGeometry& g = edge_geometry_[paint_edges[i]];
        const UiGraphEdge* edge = model_->FindEdge(g.ref);
        if(!edge)
            continue;
        const UiGraphVisualState state = paint_edge_states[i];
        const UiGraphEdgeStyle& style = paint_edge_styles[i];
        const bool painted = PaintEdgeDirect(w, *edge, g.points, g.simplified, style, state,
                                             zoom_, lod_policy_);
        ASSERT(painted); // All Painter-required edges were rejected before paint.
        if(painted) {
            last_painted_edge_count_++;
            if(g.simplified)
                last_simplified_edge_count_++;
        }
    }
    last_edge_paint_usecs_ = max<int64>(0, usecs() - edge_started);

    // Micro paint is intentionally appearance-light, but it must also stay
    // allocation-light. Stock roles/style classes are immutable for this paint
    // pass, so resolve and scale them once. Dynamic WhenResolveNodeStyle remains
    // per-node because callers may depend on node/state, but raster reuse is
    // still safe because the key uses the resolved paint state plus the actual
    // retained local silhouette.
    UiGraphNodeStyle role_styles[4];
    StyledMetrics role_metrics[4];
    bool role_ready[4] = { false, false, false, false };

    Array<StyledMetrics> class_metrics;
    Vector<byte> class_ready;
    if(!WhenResolveNodeStyle && !node_styles_.IsEmpty()) {
        class_metrics.SetCount(node_styles_.GetCount());
        class_ready.SetCount(node_styles_.GetCount(), 0);
    }

    Vector<Point> integer_path;
    integer_path.Reserve(16);
    Vector<LodMicroRasterEntry> micro_rasters;
    micro_rasters.Reserve(16);
    UiGraphNodeStyle dynamic_style;
    StyledMetrics dynamic_metrics;

    int64 node_started = usecs();
    for(int q : paint_nodes) {
        const NodeGeometry& g = node_geometry_[q];
        const UiGraphNode* node = model_->FindNode(g.ref);
        if(!node)
            continue;

        UiGraphVisualState state = GetNodeVisualState(*node);
        const UiGraphNodeStyle* style = nullptr;
        const StyledMetrics* metrics = nullptr;

        if(WhenResolveNodeStyle) {
            dynamic_style = ResolveNodeStyle(*node, state);
            dynamic_metrics = ScaleNodeMetrics(dynamic_style.metrics, zoom_);
            style = &dynamic_style;
            metrics = &dynamic_metrics;
        }
        else {
            int class_index = node->style_class.IsEmpty() ? -1 : node_styles_.Find(node->style_class);
            if(class_index >= 0) {
                style = &node_styles_[class_index];
                if(!class_ready[class_index]) {
                    class_metrics[class_index] = ScaleNodeMetrics(style->metrics, zoom_);
                    class_ready[class_index] = 1;
                }
                metrics = &class_metrics[class_index];
            }
            else {
                int role = minmax((int)node->role, 0, 3);
                if(!role_ready[role]) {
                    role_styles[role] = StyleForRole(GetEffectiveStyle().node, node->role);
                    role_metrics[role] = ScaleNodeMetrics(role_styles[role].metrics, zoom_);
                    role_ready[role] = true;
                }
                style = &role_styles[role];
                metrics = &role_metrics[role];
            }
        }

        int si = VisualStateIndex(state);
        Color face = ResolveFace(style->palette.face[si], White());
        Color frame = state == UiGraphVisualState::Selected ? SelectedFrameColor(*style)
                                                            : style->palette.frame[si];
        int frame_width = IsNull(frame) ? 0 : max(1, metrics->frame_width);
        if(state == UiGraphVisualState::Selected)
            frame_width = max(frame_width,
                              max(DPI(2), DPI(fround(lod_policy_.selection_outline_width))));

        if(node->shape == UiGraphNodeShape::Circle || node->shape == UiGraphNodeShape::Ellipse)
            w.DrawEllipse(g.surface, face, frame_width, frame);
        else {
            int cached_index = -1;
            for(int i = 0; i < micro_rasters.GetCount(); i++)
                if(micro_rasters[i].key.Matches(*node, g.hit_path, g.surface,
                                                face, frame, frame_width)) {
                    cached_index = i;
                    break;
                }

            if(cached_index < 0
               && micro_rasters.GetCount() < kLodMicroRasterCacheLimit) {
                const int pad = LodMicroStrokePad(frame_width);
                Image image = LodMicroPolygonRaster(*node, g.hit_path, g.surface,
                                                    face, frame, frame_width, pad);
                if(!IsNull(image)) {
                    LodMicroRasterEntry& entry = micro_rasters.Add();
                    entry.key.Set(*node, g.hit_path, g.surface,
                                  face, frame, frame_width);
                    entry.pad = pad;
                    entry.image = image;
                    cached_index = micro_rasters.GetCount() - 1;
                }
            }

            if(cached_index >= 0) {
                const LodMicroRasterEntry& entry = micro_rasters[cached_index];
                UiDrawCachedRaster(w, g.surface.Inflated(entry.pad), entry.image);
                last_micro_cached_draw_count_++;
            }
            else {
                // High-uniqueness scenes and cache-policy/size refusals keep
                // the direct retained-silhouette path.
                LodIntegerPath(g.hit_path, integer_path);
                if(integer_path.GetCount() >= 3)
                    w.DrawPolygon(integer_path, face, frame_width, frame);
                else
                    w.DrawRect(g.surface, face);
                last_micro_direct_fallback_count_++;
            }
        }
        last_painted_node_count_++;
    }
    last_micro_raster_count_ = micro_rasters.GetCount();
    last_node_surface_paint_usecs_ = max<int64>(0, usecs() - node_started);
    last_node_paint_usecs_ = last_node_surface_paint_usecs_;
}

UiNodeGraph& UiNodeGraph::BeginViewUpdate()
{
    if(view_update_depth_ == 0) {
        view_update_dirty_ = false;
        view_update_viewport_changed_ = false;
        view_update_selection_changed_ = false;
        view_update_scope_changed_ = false;
        view_update_scope_ = UiGraphScopeRef();
        view_update_geometry_serial_start_ = geometry_build_serial_;
        view_update_spatial_serial_start_ = spatial_build_serial_;
        last_view_update_geometry_build_count_ = 0;
        last_view_update_spatial_build_count_ = 0;
    }
    view_update_depth_++;
    return *this;
}

UiNodeGraph& UiNodeGraph::EndViewUpdate()
{
    ASSERT(view_update_depth_ > 0);
    if(view_update_depth_ <= 0)
        return *this;
    view_update_depth_--;
    if(view_update_depth_ == 0)
        FlushViewUpdate();
    return *this;
}

bool UiNodeGraph::DeferViewUpdate(bool viewport_changed, bool selection_changed)
{
    InvalidateGeometry();
    if(view_update_depth_ <= 0)
        return false;

    view_update_dirty_ = true;
    view_update_viewport_changed_ |= viewport_changed;
    view_update_selection_changed_ |= selection_changed;
    return true;
}

void UiNodeGraph::QueueScopeChanged(UiGraphScopeRef scope)
{
    if(view_update_depth_ <= 0) {
        WhenScopeChanged(scope);
        return;
    }
    view_update_scope_changed_ = true;
    view_update_scope_ = scope;
}

void UiNodeGraph::PrepareViewGeometry()
{
    if(batch_update_depth_ > 0 || view_update_depth_ > 0)
        return;
    if(!geometry_dirty_ && model_ && model_revision_ == model_->GetRevision() &&
       geometry_zoom_ == zoom_ && geometry_pan_ == pan_ && geometry_size_ == GetSize())
        return;
    int64 started = usecs();
    RebuildViewGeometry();
    last_geometry_prepare_usecs_ = max<int64>(0, usecs() - started);
}

void UiNodeGraph::BuildViewNodeGeometry(const UiGraphNode& node, NodeGeometry& out,
                                            bool record_evidence)
{
    Point p = WorldToScreen(GetDisplayNodePosition(node));
    Size sz(max(1, fround(node.size.cx * zoom_)), max(1, fround(node.size.cy * zoom_)));
    if(node.shape == UiGraphNodeShape::Square || node.shape == UiGraphNodeShape::Circle) {
        int side = max(sz.cx, sz.cy);
        sz = Size(side, side);
    }

    // Projected size is the authoritative detail boundary. A 20px-wide node is
    // micro regardless of the abstract zoom value; constructing title/content/
    // port-hit geometry for it only creates invisible work and was the dominant
    // intermediate-zoom 10k stall exposed by the P1 live profile.
    const bool overview_geometry = UiNodeGraphDetail::IsMicroNode(sz.cx, sz.cy);

    if(!overview_geometry) {
        BuildNodeGeometry(node, out);
        if(record_evidence)
            last_geometry_path_vertex_count_ += out.hit_path.GetCount();
        return;
    }

    UiGraphVisualState node_state = GetNodeVisualState(node);
    int64 resolve_started = record_evidence ? usecs() : 0;
    UiGraphNodeStyle style = ResolveNodeStyle(node, node_state);
    int64 resolve_us = record_evidence ? max<int64>(0, usecs() - resolve_started) : 0;
    int64 scale_started = record_evidence ? usecs() : 0;
    StyledMetrics metrics = ScaleNodeMetrics(style.metrics, zoom_);
    int64 scale_us = record_evidence ? max<int64>(0, usecs() - scale_started) : 0;
    if(record_evidence) {
        last_geometry_style_resolve_usecs_ += resolve_us;
        last_geometry_style_scale_usecs_ += scale_us;
        last_geometry_style_usecs_ += resolve_us + scale_us;
    }

    out.ref = node.ref;
    out.rect = RectC(p.x, p.y, sz.cx, sz.cy);
    out.paint_bounds = out.rect;
    out.z_order = node.z_order;
    out.compact = true;
    out.micro = true;
    out.surface = UiStyledSurfaceRect(out.rect, metrics);

    int64 silhouette_started = record_evidence ? usecs() : 0;
    int cached_path = -1;
    for(int i = 0; i < micro_path_cache_.GetCount(); i++) {
        const MicroPathCacheEntry& entry = micro_path_cache_[i];
        if(entry.shape == node.shape
           && entry.surface_size == out.surface.GetSize()
           && entry.source_cx == node.size.cx
           && entry.source_cy == node.size.cy
           && entry.corner_radius == node.corner_radius) {
            cached_path = i;
            break;
        }
    }

    if(cached_path >= 0) {
        const Vector<Pointf>& local = micro_path_cache_[cached_path].local_path;
        out.hit_path.SetCount(local.GetCount());
        for(int i = 0; i < local.GetCount(); i++)
            out.hit_path[i] = Pointf(local[i].x + out.surface.left,
                                     local[i].y + out.surface.top);
        if(record_evidence)
            last_geometry_path_cache_hit_count_++;
    }
    else {
        out.hit_path = LodNodeShapePath(node, out.surface);
        if(record_evidence)
            last_geometry_path_cache_miss_count_++;
        if(micro_path_cache_.GetCount() < kLodMicroPathCacheLimit) {
            MicroPathCacheEntry& entry = micro_path_cache_.Add();
            entry.shape = node.shape;
            entry.surface_size = out.surface.GetSize();
            entry.source_cx = node.size.cx;
            entry.source_cy = node.size.cy;
            entry.corner_radius = node.corner_radius;
            entry.local_path.SetCount(out.hit_path.GetCount());
            for(int i = 0; i < out.hit_path.GetCount(); i++)
                entry.local_path[i] = Pointf(out.hit_path[i].x - out.surface.left,
                                             out.hit_path[i].y - out.surface.top);
        }
    }
    if(record_evidence)
        last_geometry_silhouette_usecs_ += max<int64>(0, usecs() - silhouette_started);

    // Visible edges still need stable authored-side anchors, but micro geometry
    // deliberately carries no text/content/control/label/hit rectangles. Reuse
    // four index buffers so each node does not allocate side-port temporaries.
    int64 anchor_started = record_evidence ? usecs() : 0;
    for(int si = 0; si < 4; si++)
        micro_side_port_scratch_[si].SetCount(0);

    for(int i = 0; i < node.ports.GetCount(); i++) {
        if(!node.ports[i].visible)
            continue;
        UiGraphPortSide side = ResolvePortSide(node.ports[i]);
        int si = side == UiGraphPortSide::Left ? 0
               : side == UiGraphPortSide::Right ? 1
               : side == UiGraphPortSide::Top ? 2 : 3;
        micro_side_port_scratch_[si].Add(i);
    }
    for(int si = 0; si < 4; si++)
        Sort(micro_side_port_scratch_[si], [&](int a, int b) {
            return node.ports[a].order != node.ports[b].order
                 ? node.ports[a].order < node.ports[b].order : a < b;
        });

    for(int si = 0; si < 4; si++) {
        const Vector<int>& indexes = micro_side_port_scratch_[si];
        int count = indexes.GetCount();
        if(count == 0)
            continue;
        bool vertical = si < 2;
        UiGraphPortSide side = si == 0 ? UiGraphPortSide::Left
                             : si == 1 ? UiGraphPortSide::Right
                             : si == 2 ? UiGraphPortSide::Top
                                       : UiGraphPortSide::Bottom;
        double span = vertical ? out.surface.GetHeight() : out.surface.GetWidth();
        double inset = count <= 1 ? 0.0
                     : min(span * 0.22, max(4.0, style.port_spacing * zoom_ * 0.30));
        double start = vertical ? out.surface.top + inset : out.surface.left + inset;
        double end = vertical ? out.surface.bottom - inset : out.surface.right - inset;
        if(end < start) {
            double centre = vertical ? (out.surface.top + out.surface.bottom) * 0.5
                                     : (out.surface.left + out.surface.right) * 0.5;
            start = end = centre;
        }
        double step = count <= 1 ? 0.0 : (end - start) / (count - 1);
        for(int n = 0; n < count; n++) {
            const UiGraphPort& port = node.ports[indexes[n]];
            double axis = count == 1 ? (start + end) * 0.5 : start + step * n;
            out.anchors.Add(port.id, ShapeBoundaryAnchor(out.hit_path, out.surface, side, axis));
        }
    }
    if(record_evidence)
        last_geometry_anchor_usecs_ += max<int64>(0, usecs() - anchor_started);

    int inflate = max(1, fround(2 * min(1.0, max(0.25, zoom_))));
    int state_index = VisualStateIndex(node_state);
    Color paint_frame = node_state == UiGraphVisualState::Selected
                      ? SelectedFrameColor(style) : style.palette.frame[state_index];
    int paint_frame_width = IsNull(paint_frame) ? 0 : max(1, metrics.frame_width);
    if(node_state == UiGraphVisualState::Selected)
        paint_frame_width = max(paint_frame_width,
                                max(DPI(2), DPI(fround(lod_policy_.selection_outline_width))));
    if(paint_frame_width > 0)
        inflate = max(inflate, LodMicroStrokePad(paint_frame_width));
    if(IsNodeSelected(node.ref))
        inflate = max(inflate, LodMicroStrokePad(
            max(paint_frame_width, DPI(fround(lod_policy_.selection_outline_width)))));
    out.paint_bounds = out.surface.Inflated(inflate);
    out.paint_bounds |= out.rect;

    if(record_evidence) {
        last_geometry_lod_node_count_++;
        last_geometry_path_vertex_count_ += out.hit_path.GetCount();
    }
}

void UiNodeGraph::RebuildViewGeometry()
{
    int64 reset_started = usecs();
    node_geometry_.Clear();
    edge_geometry_.Clear();
    micro_path_cache_.Clear();
    last_node_candidate_count_ = 0;
    last_edge_candidate_count_ = 0;
    last_hidden_edge_count_ = 0;
    last_geometry_lod_node_count_ = 0;
    last_geometry_path_vertex_count_ = 0;
    last_geometry_spatial_usecs_ = 0;
    last_geometry_query_usecs_ = 0;
    last_geometry_sort_usecs_ = 0;
    last_geometry_node_usecs_ = 0;
    last_geometry_style_usecs_ = 0;
    last_geometry_style_resolve_usecs_ = 0;
    last_geometry_style_scale_usecs_ = 0;
    last_geometry_silhouette_usecs_ = 0;
    last_geometry_anchor_usecs_ = 0;
    last_geometry_edge_usecs_ = 0;
    last_geometry_path_cache_hit_count_ = 0;
    last_geometry_path_cache_miss_count_ = 0;
    last_geometry_reset_usecs_ = max<int64>(0, usecs() - reset_started);

    if(!model_) {
        model_revision_ = -1;
        geometry_zoom_ = zoom_;
        geometry_pan_ = pan_;
        geometry_size_ = GetSize();
        geometry_dirty_ = false;
        geometry_build_serial_++;
        prepared_geometry_revision_++;
        return;
    }

    int64 spatial_started = usecs();
    EnsureSpatialIndex();
    last_geometry_spatial_usecs_ = max<int64>(0, usecs() - spatial_started);

    int64 query_started = usecs();
    for(int i = selected_nodes_.GetCount() - 1; i >= 0; --i)
        if(!model_->Contains(UiGraphNodeRef{selected_nodes_[i]}))
            selected_nodes_.Remove(i);
    for(int i = selected_edges_.GetCount() - 1; i >= 0; --i)
        if(!model_->Contains(UiGraphEdgeRef{selected_edges_[i]}))
            selected_edges_.Remove(i);

    Index<UiGraphId> node_candidates;
    Index<UiGraphId> edge_candidates;
    QuerySpatial(GetViewportWorldBounds(GetPaintQueryMargin()), node_candidates, edge_candidates,
                 SPATIAL_QUERY_NODES | SPATIAL_QUERY_EDGES | SPATIAL_QUERY_REDUCE_OVERVIEW);

    for(int i = 0; i < drag_preview_positions_.GetCount(); i++) {
        UiGraphNodeRef ref{drag_preview_positions_.GetKey(i)};
        node_candidates.FindAdd(ref.id);
        if(zoom_ >= lod_policy_.edge_hide_zoom)
            for(UiGraphEdgeRef edge : model_->GetNodeEdges(ref))
                edge_candidates.FindAdd(edge.id);
    }
    if(interaction_ == InteractionMode::EdgeRouteDrag && route_edge_.IsValid())
        edge_candidates.FindAdd(route_edge_.id);

    if(zoom_ >= lod_policy_.edge_hide_zoom)
        for(int i = 0; i < edge_candidates.GetCount(); i++) {
            const UiGraphEdge* edge = model_->FindEdge(UiGraphEdgeRef{edge_candidates[i]});
            if(edge) {
                node_candidates.FindAdd(edge->source.node.id);
                node_candidates.FindAdd(edge->target.node.id);
            }
        }

    last_node_candidate_count_ = node_candidates.GetCount();
    last_edge_candidate_count_ = edge_candidates.GetCount();
    last_geometry_query_usecs_ = max<int64>(0, usecs() - query_started);

    int64 sort_started = usecs();
    Vector<ViewNodeOrderRecord> order;
    order.Reserve(node_candidates.GetCount());
    for(int i = 0; i < node_candidates.GetCount(); i++) {
        UiGraphNodeRef ref{node_candidates[i]};
        const UiGraphNode* node = model_->FindNode(ref);
        if(node && node->visible) {
            ViewNodeOrderRecord& record = order.Add();
            record.node = node;
            record.ref = ref;
            record.z_order = node->z_order;
        }
    }
    Sort(order, [](const ViewNodeOrderRecord& a, const ViewNodeOrderRecord& b) {
        return a.z_order != b.z_order ? a.z_order < b.z_order : a.ref.id < b.ref.id;
    });
    last_geometry_sort_usecs_ = max<int64>(0, usecs() - sort_started);

    int64 node_started = usecs();
    for(const ViewNodeOrderRecord& record : order) {
        NodeGeometry g;
        BuildViewNodeGeometry(*record.node, g);
        node_geometry_.Add(record.ref.id, pick(g));
    }
    last_geometry_node_usecs_ = max<int64>(0, usecs() - node_started);

    int64 edge_started = usecs();
    if(zoom_ < lod_policy_.edge_hide_zoom)
        last_hidden_edge_count_ = edge_candidates.GetCount();
    else
        for(int i = 0; i < edge_candidates.GetCount(); i++) {
            UiGraphEdgeRef ref{edge_candidates[i]};
            const UiGraphEdge* edge = model_->FindEdge(ref);
            if(!edge || !edge->visible || !model_->FindPort(edge->source) || !model_->FindPort(edge->target))
                continue;
            EdgeGeometry g;
            BuildEdgeGeometry(*edge, g);
            edge_geometry_.Add(ref.id, pick(g));
        }
    last_geometry_edge_usecs_ = max<int64>(0, usecs() - edge_started);

    model_revision_ = model_->GetRevision();
    geometry_zoom_ = zoom_;
    geometry_pan_ = pan_;
    geometry_size_ = GetSize();
    geometry_dirty_ = false;
    geometry_build_serial_++;
    prepared_geometry_revision_++;
}

void UiNodeGraph::FlushViewUpdate()
{
    ASSERT(view_update_depth_ == 0);

    const bool dirty = view_update_dirty_;
    const bool viewport_changed = view_update_viewport_changed_;
    const bool selection_changed = view_update_selection_changed_;
    const bool scope_changed = view_update_scope_changed_;
    const UiGraphScopeRef scope = view_update_scope_;

    view_update_dirty_ = false;
    view_update_viewport_changed_ = false;
    view_update_selection_changed_ = false;
    view_update_scope_changed_ = false;
    view_update_scope_ = UiGraphScopeRef();

    if(dirty) {
        PrepareViewGeometry();
        UpdateAttachedCtrls();
        Refresh();
    }

    last_view_update_geometry_build_count_ = max(0, geometry_build_serial_ - view_update_geometry_serial_start_);
    last_view_update_spatial_build_count_ = max(0, spatial_build_serial_ - view_update_spatial_serial_start_);

    if(dirty || viewport_changed || selection_changed || scope_changed)
        view_update_flush_serial_++;

    // Publish events only after the final exact scene exists. This prevents a
    // host callback from observing an intermediate model/scope/camera state.
    if(selection_changed)
        WhenSelection();
    if(scope_changed)
        WhenScopeChanged(scope);
    if(viewport_changed)
        WhenViewport();
}

UiNodeGraph& UiNodeGraph::SetZoom(double zoom, Point anchor)
{
    const Style& style = GetEffectiveStyle();
    zoom = minmax(zoom, max(0.01, style.min_zoom), max(style.min_zoom, style.max_zoom));
    const bool had_live_projection = CancelLiveViewProjection();
    if(abs(zoom - zoom_) < 1e-9 && !had_live_projection)
        return *this;
    if(anchor.x < 0 || anchor.y < 0)
        anchor = Point(GetSize().cx / 2, GetSize().cy / 2);

    Pointf world = ScreenToWorld(anchor);
    zoom_ = zoom;
    pan_ = Pointf(anchor.x - world.x * zoom_, anchor.y - world.y * zoom_);

    if(DeferViewUpdate(true, false))
        return *this;

    PrepareViewGeometry();
    UpdateAttachedCtrls();
    Refresh();
    WhenViewport();
    return *this;
}

UiNodeGraph& UiNodeGraph::SetPan(Pointf pan)
{
    const bool had_live_projection = CancelLiveViewProjection();
    if(pan_ == pan && !had_live_projection)
        return *this;
    pan_ = pan;

    if(DeferViewUpdate(true, false))
        return *this;

    PrepareViewGeometry();
    UpdateAttachedCtrls();
    Refresh();
    WhenViewport();
    return *this;
}

UiNodeGraph& UiNodeGraph::PanBy(Pointf delta)
{
    return SetPan(pan_ + delta);
}

UiNodeGraph& UiNodeGraph::ResetView()
{
    const bool had_live_projection = CancelLiveViewProjection();
    if(abs(zoom_ - 1.0) < 1e-9 && pan_ == Pointf(0, 0) && !had_live_projection)
        return *this;
    zoom_ = 1.0;
    pan_ = Pointf(0, 0);

    if(DeferViewUpdate(true, false))
        return *this;

    PrepareViewGeometry();
    UpdateAttachedCtrls();
    Refresh();
    WhenViewport();
    return *this;
}

} // namespace Upp

