// Scene admission and projected-micro software paint. Preflight completes before
// drawing; unsupported features choose PaintGraphRich with an explicit reason.
// Reuses prepared silhouettes and bounded raster caches; adds no node controls.

namespace Upp {
namespace {

constexpr int kLodMicroRasterCacheLimit = 32;


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
        PaintGraphRich(w);
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

} // namespace Upp
