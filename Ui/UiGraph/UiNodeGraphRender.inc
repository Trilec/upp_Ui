namespace Upp {
namespace {

bool IsStockGraphShadow(const StyledShadow& shadow)
{
    return shadow.enabled
        && !shadow.inset
        && shadow.mode == SHADOW_CURVE
        && shadow.distance == DPI(5)
        && shadow.offset_x == DPI(2)
        && shadow.offset_y == DPI(2)
        && shadow.alpha == 42
        && shadow.color == Color(15, 23, 42);
}

bool ApplyR93CNodePresentation(const UiGraphNode& node, UiGraphNodeStyle& style)
{
    bool suppressed = node.style_class.IsEmpty() && IsStockGraphShadow(style.metrics.shadow);
    if(suppressed)
        style.metrics.shadow.enabled = false;
    return suppressed;
}

bool UsesRectangularStyledSurface(UiGraphNodeShape shape)
{
    const byte wire = (byte)shape;
    return shape == UiGraphNodeShape::Rectangle
        || wire == 0   // historical flat Rectangle
        || wire == 1   // historical RoundedRectangle
        || wire == 2   // historical Square
        || wire == 8;  // historical Capsule
}

int ResolveGraphRectangleRadius(const UiGraphNode& node, const Rect& surface, double zoom)
{
    const byte wire = (byte)node.shape;
    if(wire == 0)
        return 0;
    if(wire == 8)
        return max(0, min(surface.GetWidth(), surface.GetHeight()) / 2);
    return max(0, min(fround(node.corner_radius * zoom),
                      min(surface.GetWidth(), surface.GetHeight()) / 2));
}

UiLineStyle ResolveDirectLineStyle(const UiGraphEdge& edge,
                                   const UiGraphEdgeStyle& style,
                                   bool simplified,
                                   double zoom,
                                   double simplify_zoom)
{
    UiLineStyle line_style = style.line_style;
    if(edge.stroke == UiGraphStrokeStyle::Solid)
        line_style = SOLID;
    else if(edge.stroke == UiGraphStrokeStyle::Dashed)
        line_style = DASHED;
    else if(edge.stroke == UiGraphStrokeStyle::Dotted)
        line_style = DOTTED;
    if(simplified || zoom < simplify_zoom)
        line_style = SOLID;
    return line_style;
}

void DrawPatternedPolyline(Draw& w, const Vector<Pointf>& points,
                           int width, Color color, UiLineStyle line_style,
                           double dash_length, double dash_gap)
{
    if(points.GetCount() < 2 || width <= 0 || IsNull(color))
        return;

    if(line_style == SOLID) {
        for(int i = 1; i < points.GetCount(); i++)
            w.DrawLine(fround(points[i - 1].x), fround(points[i - 1].y),
                       fround(points[i].x), fround(points[i].y), width, color);
        return;
    }

    double on = line_style == DOTTED ? max(1.0, (double)width)
                                      : max(1.0, dash_length);
    double off = line_style == DOTTED ? max(2.0, dash_gap * 0.55)
                                       : max(1.0, dash_gap);
    const double cycle = on + off;
    const double eps = 1e-9;
    double phase = 0.0;

    for(int i = 1; i < points.GetCount(); i++) {
        Pointf a(points[i - 1].x, points[i - 1].y);
        Pointf b(points[i].x, points[i].y);
        Pointf d = b - a;
        double len = VectorLength(d);
        if(len <= eps)
            continue;
        d /= len;

        double pos = 0.0;
        while(pos < len - eps) {
            const bool draw = phase < on;
            const double boundary = draw ? on : cycle;
            double remain = boundary - phase;

            // Snap an exact/near-exact boundary before computing the next
            // segment. Progress is therefore mathematical and deterministic;
            // no clock-dependent remainder workaround is required.
            if(remain <= eps) {
                phase = draw ? on : 0.0;
                if(!draw)
                    continue;
                remain = cycle - phase;
            }

            const double step = min(remain, len - pos);
            if(step <= eps) {
                phase = draw ? on : 0.0;
                continue;
            }

            if(draw && step > 0.05) {
                Pointf p0 = a + d * pos;
                Pointf p1 = a + d * (pos + step);
                w.DrawLine(fround(p0.x), fround(p0.y),
                           fround(p1.x), fround(p1.y), width, color);
            }

            pos += step;
            phase += step;
            if(phase >= cycle - eps)
                phase = 0.0;
            else if(draw && phase >= on - eps)
                phase = on;
        }
    }
}

bool PaintEdgeDirect(Draw& w, const UiGraphEdge& edge,
                     const Vector<Pointf>& points, bool simplified,
                     const UiGraphEdgeStyle& style, UiGraphVisualState state,
                     double zoom, const UiNodeGraph::LodPolicy& lod)
{
    if(zoom < lod.edge_hide_zoom || points.GetCount() < 2)
        return true;

    int si = minmax((int)state, 0, 3);
    Color color = style.color[si];
    double detail = SmoothUnit(zoom, lod.edge_hide_zoom, lod.full_detail_zoom);
    double widthf = style.width[si] * min(1.0, max(0.01, zoom))
                  * (0.35 + 0.65 * detail);
    if(zoom < lod.edge_simplify_zoom)
        widthf = min(widthf, 1.0);
    if(widthf <= 0.02 || IsNull(color))
        return true;

    UiGraphArrowStyle arrow = edge.arrow == UiGraphArrowStyle::Inherit ? style.arrow : edge.arrow;
    if(edge.directed && arrow != UiGraphArrowStyle::None && arrow != UiGraphArrowStyle::Open)
        return false;

    UiLineStyle line_style = ResolveDirectLineStyle(edge, style, simplified,
                                                    zoom, lod.edge_simplify_zoom);
    int width = max(1, fround(widthf));
    DrawPatternedPolyline(w, points, width, color, line_style,
                          style.dash_length * zoom, style.dash_gap * zoom);

    double arrow_factor = SmoothUnit(zoom, lod.arrow_zoom, lod.full_detail_zoom);
    if(edge.directed && arrow == UiGraphArrowStyle::Open && arrow_factor > 0.01) {
        Pointf tip(points.Top().x, points.Top().y);
        Pointf prev(points[points.GetCount() - 2].x, points[points.GetCount() - 2].y);
        Pointf d = tip - prev;
        double len = VectorLength(d);
        if(len > 1e-9) {
            d /= len;
            Pointf perp(-d.y, d.x);
            double sz = style.arrow_size * min(1.0, max(0.01, zoom)) * arrow_factor;
            Pointf back = tip - d * sz;
            Pointf a = back + perp * (sz * 0.48);
            Pointf b = back - perp * (sz * 0.48);
            w.DrawLine(fround(tip.x), fround(tip.y), fround(a.x), fround(a.y), width, color);
            w.DrawLine(fround(tip.x), fround(tip.y), fround(b.x), fround(b.y), width, color);
        }
    }
    return true;
}

void PaintCachedPortMarker(Draw& w, const Rect& target,
                           Color canvas, Color frame, double stroke)
{
    if(target.IsEmpty() || IsNull(frame))
        return;
    UiRasterCachePolicy policy = UiRasterPolicyIcon("aa/nodegraph-port");
    UiRasterCacheKeyBuilder kb("aa/nodegraph-port");
    kb.Add(target.GetSize()).Add(canvas).Add(frame).Add(stroke, 100);
    Image cached = UiRasterCache::Get(kb.Build(), policy, [=] {
        ImageBuffer ib(target.GetSize());
        ib.SetKind(IMAGE_ALPHA);
        Fill(~ib, RGBAZero(), ib.GetLength());
        BufferPainter p(ib, MODE_ANTIALIASED);
        p.Begin();
        p.Ellipse(0.5, 0.5,
                  max(1.0, target.GetWidth() - 1.0),
                  max(1.0, target.GetHeight() - 1.0));
        if(!IsNull(canvas))
            p.Fill(canvas);
        p.Stroke(stroke, frame);
        p.End();
        p.Finish();
        return Image(ib);
    });
    UiDrawCachedRaster(w, target, cached);
}

void PaintCachedRoundedFill(Draw& w, const Rect& target, Color color, int radius)
{
    if(target.IsEmpty() || IsNull(color))
        return;
    radius = max(0, min(radius, min(target.GetWidth(), target.GetHeight()) / 2));
    if(radius <= 0) {
        w.DrawRect(target, color);
        return;
    }
    UiRasterCachePolicy policy = UiRasterPolicyAA("aa/nodegraph-header");
    policy.allow_scale_from_bucket = false;
    UiRasterCacheKeyBuilder kb("aa/nodegraph-header");
    kb.Add(target.GetSize()).Add(color).Add(radius);
    Image cached = UiRasterCache::Get(kb.Build(), policy, [=] {
        ImageBuffer ib(target.GetSize());
        ib.SetKind(IMAGE_ALPHA);
        Fill(~ib, RGBAZero(), ib.GetLength());
        BufferPainter p(ib, MODE_ANTIALIASED);
        p.RoundedRectangle(0.5, 0.5,
                           max(1.0, target.GetWidth() - 1.0),
                           max(1.0, target.GetHeight() - 1.0), radius)
         .Fill(color);
        p.Finish();
        return Image(ib);
    });
    UiDrawCachedRaster(w, target, cached);
}

bool PaintCachedShapeBody(Draw& w, const UiGraphNode& node, const Rect& surface,
                          const UiGraphNodeStyle& style, UiGraphVisualState state,
                          const StyledMetrics& metrics, double zoom,
                          double selection_outline_width)
{
    if(surface.IsEmpty() || node.shape == UiGraphNodeShape::Custom)
        return false;
    if(UsesRectangularStyledSurface(node.shape))
        return true;

    const StyledShadow& shadow = metrics.shadow;
    bool shadow_active = shadow.enabled && !shadow.inset && shadow.alpha > 0
                      && shadow.distance > 0;
    if(shadow_active || metrics.dashed)
        return false;

    int si = minmax((int)state, 0, 3);
    Color face = ResolveFace(style.palette.face[si], White());
    Color frame = state == UiGraphVisualState::Selected ? SelectedFrameColor(style)
                                                        : style.palette.frame[si];
    int frame_width = max(1, metrics.frame_width);
    if(state == UiGraphVisualState::Selected)
        frame_width = max(frame_width, max(DPI(2), DPI(fround(selection_outline_width))));
    int pad = max(2, (frame_width + 1) / 2 + 1);
    Rect target = surface.Inflated(pad);
    Size image_size = target.GetSize();
    if(image_size.cx <= 0 || image_size.cy <= 0)
        return true;

    const UiGraphNodeShape shape = node.shape;
    const Size surface_size = surface.GetSize();
    int radius_px = max(0, fround(node.corner_radius * zoom));
    UiRasterCachePolicy policy = UiRasterPolicyAA("aa/nodegraph-shape");
    policy.allow_scale_from_bucket = false;
    policy.max_axis = 512;
    policy.max_single_image_bytes = 1024 * 1024;
    UiRasterCacheKeyBuilder kb("aa/nodegraph-shape");
    kb.Add((int)(byte)shape).Add(image_size).Add(surface_size)
      .Add(radius_px).Add(face).Add(frame).Add(frame_width);

    Image cached = UiRasterCache::Get(kb.Build(), policy, [=] {
        ImageBuffer ib(image_size);
        ib.SetKind(IMAGE_ALPHA);
        Fill(~ib, RGBAZero(), ib.GetLength());
        BufferPainter p(ib, MODE_ANTIALIASED);
        UiGraphNode local_node;
        local_node.shape = shape;
        local_node.size = Sizef(surface_size.cx, surface_size.cy);
        local_node.corner_radius = radius_px;
        Rect local_surface = RectC(pad, pad, surface_size.cx, surface_size.cy);
        Vector<Pointf> path = NodeShapePath(local_node, local_surface);
        FillPath(p, path, face);
        if(!IsNull(frame) && frame_width > 0)
            StrokePath(p, path, (double)frame_width, frame, true);
        p.Finish();
        return Image(ib);
    });
    UiDrawCachedRaster(w, target, cached);
    return true;
}

Image GetGraphGridTile(int minor_step, int major_step, bool draw_minor,
                       Color minor, Color major)
{
    minor_step = max(1, minor_step);
    int hierarchy = max(1, fround((double)max(minor_step, major_step) / minor_step));
    major_step = minor_step * hierarchy;
    int repeats = max(1, min(8, 512 / major_step));
    int tile_size = major_step * repeats;
    UiRasterCachePolicy policy = UiRasterPolicyAA("grid/nodegraph-dots");
    policy.allow_scale_from_bucket = false;
    policy.max_axis = 512;
    policy.max_single_image_bytes = 2 * 1024 * 1024;
    UiRasterCacheKeyBuilder kb("grid/nodegraph-dots");
    kb.Add(minor_step).Add(major_step).Add(tile_size).Add(draw_minor)
      .Add(minor).Add(major);
    return UiRasterCache::Get(kb.Build(), policy, [=] {
        ImageBuffer ib(Size(tile_size, tile_size));
        ib.SetKind(IMAGE_ALPHA);
        Fill(~ib, RGBAZero(), ib.GetLength());
        RGBA *pixels = ~ib;
        RGBA minor_px = PremultipliedRGBA(minor, 255);
        RGBA major_px = PremultipliedRGBA(major, 255);
        if(draw_minor && !IsNull(minor))
            for(int y = 0; y < tile_size; y += minor_step)
                for(int x = 0; x < tile_size; x += minor_step)
                    pixels[y * tile_size + x] = minor_px;
        if(!IsNull(major))
            for(int y = 0; y < tile_size; y += major_step)
                for(int x = 0; x < tile_size; x += major_step)
                    pixels[y * tile_size + x] = major_px;
        return Image(ib);
    });
}

} // namespace

void UiNodeGraph::PaintNodeDetails(Painter& p, const UiGraphNode& node, const NodeGeometry& g,
                                   const UiGraphNodeStyle& style, UiGraphVisualState state)
{
    int si = VisualStateIndex(state);
    Rect rect = g.surface;
    bool rectangular = UsesRectangularStyledSurface(node.shape);
    bool custom_body = node.shape == UiGraphNodeShape::Custom && WhenPaintCustomShape
                    && WhenPaintCustomShape(p, node, rect, style, state);
    StyledMetrics metrics = ScaleNodeMetrics(style.metrics, zoom_);

    if(!rectangular && !custom_body && !g.hit_path.IsEmpty()) {
        const StyledShadow& shadow = metrics.shadow;
        double shadow_detail = ShadowDetailFactor();
        if(!g.micro && shadow_detail > 0.01 && shadow.enabled && !shadow.inset
           && shadow.alpha > 0 && shadow.distance > 0) {
            int layers = shadow.mode == SHADOW_HARD ? 1
                       : shadow_detail >= 0.72 ? 5
                       : shadow_detail >= 0.36 ? 3 : 1;
            int outer_target = 0;
            for(int layer = layers; layer >= 1; --layer) {
                double t = ((double)layer - 0.5) / max(1, layers);
                double spread = shadow.mode == SHADOW_HARD ? 0.0
                              : shadow.distance * (double)layer / layers;
                double curve_alpha = shadow.mode == SHADOW_HARD
                                   ? 1.0
                                   : 1.0 - UiShadowCurveEval(shadow.curve, t);
                int target_alpha = clamp(fround(shadow.alpha * shadow_detail * curve_alpha), 0, 255);
                int layer_alpha = shadow.mode == SHADOW_HARD
                                ? clamp(fround(shadow.alpha * shadow_detail), 0, 255)
                                : max(0, target_alpha - outer_target);
                outer_target = target_alpha;
                if(layer_alpha <= 0)
                    continue;
                Vector<Pointf> shape = InflatePath(g.hit_path, spread);
                Pointf off(shadow.offset_x, shadow.offset_y);
                for(Pointf& q : shape)
                    q += off;
                FillPath(p, shape, PremultipliedRGBA(shadow.color, layer_alpha));
            }
        }

        FillPath(p, g.hit_path, ResolveFace(style.palette.face[si], White()));
        Color frame = state == UiGraphVisualState::Selected ? SelectedFrameColor(style)
                                                            : style.palette.frame[si];
        double frame_width = max(0.50, (double)metrics.frame_width);
        if(state == UiGraphVisualState::Selected)
            frame_width = max(frame_width,
                              (double)max(DPI(2), DPI(fround(lod_policy_.selection_outline_width))));
        StrokeStyledPath(p, g.hit_path, frame_width, frame, metrics);
    }

    if(zoom_ >= lod_policy_.secondary_text_zoom && !g.micro && !custom_body
       && style.show_header_band && !node.collapsed && g.header.GetHeight() > 0) {
        Rect hr = g.header.Deflated(max(1, fround(2 * zoom_)), max(1, fround(2 * zoom_)));
        Vector<Pointf> header;
        header << Pointf(hr.left, hr.top) << Pointf(hr.right, hr.top)
               << Pointf(hr.right, hr.bottom) << Pointf(hr.left, hr.bottom);
        FillPath(p, UiGeometry::RoundedPolygon(header, max(0.0, node.corner_radius * zoom_ * 0.55)),
                 style.header_face[si]);
    }

    double port_visual_floor = max(0.12, lod_policy_.port_zoom - 0.04);
    double port_visual_full = max(port_visual_floor + 0.08, lod_policy_.port_zoom + 0.16);
    double port_factor = SmoothUnit(zoom_, port_visual_floor, port_visual_full);
    if(port_factor <= 0.01)
        return;

    const Style& graph_style = GetEffectiveStyle();
    Color canvas = ResolveFace(graph_style.canvas_palette.face[ST_NORMAL], SColorPaper());
    int port_radius = max(1, fround(style.port_radius * min(1.0, max(0.24, zoom_))));
    double normal_stroke = max(1.0, (double)DPI(1));

    for(int i = 0; i < g.anchors.GetCount(); i++) {
        String port_id = g.anchors.GetKey(i);
        UiGraphPortRef candidate{node.ref, port_id};
        const UiGraphPort* port = model_->FindPort(candidate);
        if(!port)
            continue;
        Point anchor = g.anchors[i];
        Rect pr = RectC(anchor.x - port_radius, anchor.y - port_radius,
                        port_radius * 2 + 1, port_radius * 2 + 1);
        Color frame = style.port_frame[si];
        if(IsNull(frame)) frame = style.palette.frame[si];
        if(IsNull(frame)) frame = Color(148, 163, 184);
        bool active = hot_port_ == candidate || connection_source_ == candidate;
        if(connection_source_.IsValid() && candidate == connection_target_) {
            frame = connection_decision_.IsAllowed() ? Color(34, 197, 94) : Color(239, 68, 68);
            active = true;
        }
        else if(active) {
            Color semantic = IsNull(port->color) ? UiGraphDefaultTypeColor(port->type) : port->color;
            frame = BlendGraphColor(frame, semantic, 0.55);
        }
        else if(!IsNull(port->color))
            frame = BlendGraphColor(frame, port->color, 0.22);
        p.Begin();
        p.Ellipse(pr.left + 0.5, pr.top + 0.5,
                  max(1.0, pr.GetWidth() - 1.0),
                  max(1.0, pr.GetHeight() - 1.0));
        if(!IsNull(canvas))
            p.Fill(canvas);
        p.Stroke(active ? max(1.5, normal_stroke * 1.5) : normal_stroke, frame);
        p.End();
    }
}

void UiNodeGraph::PaintGraphGeometry(Draw& w)
{
    Size size = GetSize();
    if(size.cx <= 0 || size.cy <= 0)
        return;
    Rect viewport(Point(0, 0), size);
    Rect paint = w.GetPaintRect() & viewport;
    if(paint.IsEmpty())
        return;

    last_paint_node_visit_count_ = 0;
    last_paint_edge_visit_count_ = 0;
    last_painted_node_count_ = 0;
    last_painted_edge_count_ = 0;
    last_simplified_edge_count_ = 0;
    last_edge_paint_usecs_ = 0;
    last_node_paint_usecs_ = 0;
    last_node_surface_paint_usecs_ = 0;
    last_node_details_paint_usecs_ = 0;
    last_node_content_paint_usecs_ = 0;

    WorldRect paint_world;
    paint_world.Include(ScreenToWorld(paint.TopLeft()));
    paint_world.Include(ScreenToWorld(paint.BottomRight()));
    paint_world = paint_world.Inflated(GetPaintQueryMargin() / max(zoom_, 0.01));
    Index<UiGraphId> node_ids;
    Index<UiGraphId> edge_ids;
    QuerySpatial(paint_world, node_ids, edge_ids,
                 SPATIAL_QUERY_NODES | SPATIAL_QUERY_EDGES | SPATIAL_QUERY_REDUCE_OVERVIEW);
    for(int i = 0; i < drag_preview_positions_.GetCount(); i++) {
        UiGraphNodeRef ref{drag_preview_positions_.GetKey(i)};
        node_ids.FindAdd(ref.id);
        if(model_ && zoom_ >= lod_policy_.edge_hide_zoom)
            for(UiGraphEdgeRef edge : model_->GetNodeEdges(ref))
                edge_ids.FindAdd(edge.id);
    }
    if(interaction_ == InteractionMode::EdgeRouteDrag && route_edge_.IsValid())
        edge_ids.FindAdd(route_edge_.id);

    Vector<int> paint_nodes;
    paint_nodes.Reserve(node_ids.GetCount());
    for(int i = 0; i < node_ids.GetCount(); i++) {
        int q = node_geometry_.Find(node_ids[i]);
        if(q >= 0 && !(node_geometry_[q].paint_bounds & paint).IsEmpty())
            paint_nodes.Add(q);
    }
    Sort(paint_nodes, [&](int a, int b) {
        const NodeGeometry& ga = node_geometry_[a];
        const NodeGeometry& gb = node_geometry_[b];
        return ga.z_order != gb.z_order ? ga.z_order < gb.z_order : ga.ref.id < gb.ref.id;
    });

    Vector<int> paint_edges;
    paint_edges.Reserve(edge_ids.GetCount());
    if(zoom_ >= lod_policy_.edge_hide_zoom)
        for(int i = 0; i < edge_ids.GetCount(); i++) {
            int q = edge_geometry_.Find(edge_ids[i]);
            if(q >= 0 && !(edge_geometry_[q].bounds & paint).IsEmpty())
                paint_edges.Add(q);
        }

    last_paint_node_visit_count_ = paint_nodes.GetCount();
    last_paint_edge_visit_count_ = paint_edges.GetCount();

    Vector<UiGraphVisualState> node_states;
    Vector<UiGraphNodeStyle> node_styles;
    Vector<byte> stock_shadow_suppressed;
    node_states.Reserve(paint_nodes.GetCount());
    node_styles.Reserve(paint_nodes.GetCount());
    stock_shadow_suppressed.Reserve(paint_nodes.GetCount());
    for(int q : paint_nodes) {
        const UiGraphNode* node = model_->FindNode(node_geometry_[q].ref);
        UiGraphVisualState state = node ? GetNodeVisualState(*node) : UiGraphVisualState::Normal;
        node_states.Add(state);
        UiGraphNodeStyle style = node ? ResolveNodeStyle(*node, state) : GetEffectiveStyle().node;
        bool suppressed = node ? ApplyR93CNodePresentation(*node, style) : false;
        stock_shadow_suppressed.Add(suppressed ? 1 : 0);
        node_styles.Add(pick(style));
    }

    Vector<UiGraphVisualState> edge_states;
    Vector<UiGraphEdgeStyle> edge_styles;
    edge_states.Reserve(paint_edges.GetCount());
    edge_styles.Reserve(paint_edges.GetCount());
    for(int q : paint_edges) {
        const UiGraphEdge* edge = model_->FindEdge(edge_geometry_[q].ref);
        UiGraphVisualState state = edge ? GetEdgeVisualState(*edge) : UiGraphVisualState::Normal;
        edge_states.Add(state);
        UiGraphEdgeStyle style = edge ? ResolveEdgeStyle(*edge, state) : GetEffectiveStyle().edge;
        edge_styles.Add(pick(style));
    }

    auto paint_surface = [&](Draw& draw, const UiGraphNode& node, const NodeGeometry& g,
                             const UiGraphNodeStyle& style, UiGraphVisualState state,
                             bool stock_shadow_was_suppressed) {
        const bool canonical_rectangle = node.shape == UiGraphNodeShape::Rectangle;
        if(!canonical_rectangle &&
           (!stock_shadow_was_suppressed || !UsesRectangularStyledSurface(node.shape))) {
            PaintNodeSurface(draw, node, g, style, state);
            return;
        }
        bool handled = false;
        if(WhenPaintNodeBackground)
            WhenPaintNodeBackground(draw, node, g.rect, style, state, handled);
        if(handled)
            return;
        StyledPalette palette = style.palette;
        StyledMetrics metrics = ScaleNodeMetrics(style.metrics, zoom_);
        StyledSkin skin = ScaleNodeSkin(style.skin, zoom_);
        if(stock_shadow_was_suppressed)
            metrics.shadow.enabled = false;
        metrics.radius = ResolveGraphRectangleRadius(node, g.surface, zoom_);
        if(state == UiGraphVisualState::Selected) {
            metrics.frame_enabled = true;
            metrics.frame_width = max(metrics.frame_width,
                                      max(DPI(2), DPI(fround(lod_policy_.selection_outline_width))));
            palette.frame[ST_PRESSED] = SelectedFrameColor(style);
        }
        Rect target = stock_shadow_was_suppressed ? g.surface : g.rect;
        UiPaintStyledBackground(draw, target, palette, metrics, skin,
                                ToStyledState(state), false);
    };

    int64 edge_started = usecs();
    Vector<int> painter_edges;
    painter_edges.Reserve(paint_edges.GetCount());
    for(int i = 0; i < paint_edges.GetCount(); i++) {
        const EdgeGeometry& g = edge_geometry_[paint_edges[i]];
        const UiGraphEdge* edge = model_->FindEdge(g.ref);
        if(!edge)
            continue;
        if(PaintEdgeDirect(w, *edge, g.points, g.simplified, edge_styles[i], edge_states[i],
                           zoom_, lod_policy_)) {
            last_painted_edge_count_++;
            if(g.simplified)
                last_simplified_edge_count_++;
        }
        else
            painter_edges.Add(i);
    }

    if(!painter_edges.IsEmpty() || interaction_ == InteractionMode::Connect) {
        Rect edge_layer;
        for(int index : painter_edges)
            edge_layer |= edge_geometry_[paint_edges[index]].bounds & paint;
        if(interaction_ == InteractionMode::Connect)
            edge_layer = paint;
        UiPaintRenderLayer(w, edge_layer, [&](Painter& ep) {
            for(int index : painter_edges) {
                const EdgeGeometry& g = edge_geometry_[paint_edges[index]];
                const UiGraphEdge* edge = model_->FindEdge(g.ref);
                if(!edge)
                    continue;
                last_painted_edge_count_++;
                if(g.simplified)
                    last_simplified_edge_count_++;
                PaintEdge(ep, *edge, g, edge_styles[index], edge_states[index]);
            }
            if(interaction_ == InteractionMode::Connect)
                PaintConnectionPreview(ep);
        });
    }

    if(zoom_ >= lod_policy_.edge_label_zoom)
        for(int i = 0; i < paint_edges.GetCount(); i++) {
            const EdgeGeometry& g = edge_geometry_[paint_edges[i]];
            const UiGraphEdge* edge = model_->FindEdge(g.ref);
            if(!edge)
                continue;
            const UiGraphEdgeStyle& style = edge_styles[i];
            int si = VisualStateIndex(edge_states[i]);
            if(!edge->title.IsEmpty()) {
                Font label_font = ScaleGraphFont(style.label_font, zoom_);
                Size ts = GetTextSize(edge->title, label_font);
                Rect lr = RectC(g.label_point.x - ts.cx / 2 - DPI(4),
                                g.label_point.y - ts.cy / 2 - DPI(2),
                                ts.cx + DPI(8), ts.cy + DPI(4));
                if(style.draw_label_background)
                    w.DrawRect(lr, style.label_background);
                w.DrawText(lr.left + DPI(4), lr.top + DPI(2), edge->title,
                           label_font, style.label_ink[si]);
            }
            if(WhenPaintEdgeOverlay && zoom_ >= lod_policy_.edge_simplify_zoom) {
                Vector<Point> overlay_points;
                overlay_points.Reserve(g.points.GetCount());
                for(const Pointf& q : g.points)
                    overlay_points.Add(Point(fround(q.x), fround(q.y)));
                WhenPaintEdgeOverlay(w, *edge, overlay_points, edge_states[i]);
            }
        }
    last_edge_paint_usecs_ = max<int64>(0, usecs() - edge_started);

    int64 node_started = usecs();
    int64 surface_started = usecs();
    for(int i = 0; i < paint_nodes.GetCount(); i++) {
        const NodeGeometry& g = node_geometry_[paint_nodes[i]];
        const UiGraphNode* node = model_->FindNode(g.ref);
        if(!node)
            continue;
        last_painted_node_count_++;
        paint_surface(w, *node, g, node_styles[i], node_states[i], stock_shadow_suppressed[i] != 0);
    }
    last_node_surface_paint_usecs_ = max<int64>(0, usecs() - surface_started);

    int64 details_started = usecs();
    const Style& graph_style = GetEffectiveStyle();
    Color canvas = ResolveFace(graph_style.canvas_palette.face[ST_NORMAL], SColorPaper());
    double port_floor = max(0.12, lod_policy_.port_zoom - 0.04);
    double port_full = max(port_floor + 0.08, lod_policy_.port_zoom + 0.16);
    bool show_ports = SmoothUnit(zoom_, port_floor, port_full) > 0.01;

    for(int i = 0; i < paint_nodes.GetCount(); i++) {
        const NodeGeometry& g = node_geometry_[paint_nodes[i]];
        const UiGraphNode* node = model_->FindNode(g.ref);
        if(!node)
            continue;
        const UiGraphNodeStyle& style = node_styles[i];
        UiGraphVisualState state = node_states[i];
        int si = VisualStateIndex(state);
        StyledMetrics metrics = ScaleNodeMetrics(style.metrics, zoom_);

        bool body_done = PaintCachedShapeBody(w, *node, g.surface, style, state,
                                              metrics, zoom_, lod_policy_.selection_outline_width);
        bool custom = node->shape == UiGraphNodeShape::Custom;
        if(!body_done && (custom || !UsesRectangularStyledSurface(node->shape))) {
            int fallback_pad = DPI(2);
            const StyledShadow& shadow = metrics.shadow;
            if(shadow.enabled && !shadow.inset && shadow.alpha > 0 && shadow.distance > 0) {
                int extent = max(0, shadow.distance);
                int offset = max(abs(shadow.offset_x), abs(shadow.offset_y));
                fallback_pad = max(fallback_pad, extent + offset + DPI(3));
            }
            if(state == UiGraphVisualState::Selected)
                fallback_pad = max(fallback_pad,
                                   DPI(fround(lod_policy_.selection_outline_width)) + DPI(3));
            Rect target = g.paint_bounds.Inflated(fallback_pad) & paint;
            UiPaintRenderLayer(w, target, [&](Painter& np) {
                bool custom_body = custom && WhenPaintCustomShape
                                && WhenPaintCustomShape(np, *node, g.surface, style, state);
                if(!custom_body && !UsesRectangularStyledSurface(node->shape)
                   && !g.hit_path.IsEmpty()) {
                    const StyledShadow& local_shadow = metrics.shadow;
                    double shadow_detail = ShadowDetailFactor();
                    if(!g.micro && shadow_detail > 0.01 && local_shadow.enabled && !local_shadow.inset
                       && local_shadow.alpha > 0 && local_shadow.distance > 0) {
                        int layers = local_shadow.mode == SHADOW_HARD ? 1
                                   : shadow_detail >= 0.72 ? 5
                                   : shadow_detail >= 0.36 ? 3 : 1;
                        int outer_target = 0;
                        for(int layer = layers; layer >= 1; --layer) {
                            double t = ((double)layer - 0.5) / max(1, layers);
                            double spread = local_shadow.mode == SHADOW_HARD ? 0.0
                                          : local_shadow.distance * (double)layer / layers;
                            double curve_alpha = local_shadow.mode == SHADOW_HARD
                                               ? 1.0 : 1.0 - UiShadowCurveEval(local_shadow.curve, t);
                            int target_alpha = clamp(fround(local_shadow.alpha * shadow_detail * curve_alpha), 0, 255);
                            int layer_alpha = local_shadow.mode == SHADOW_HARD
                                            ? clamp(fround(local_shadow.alpha * shadow_detail), 0, 255)
                                            : max(0, target_alpha - outer_target);
                            outer_target = target_alpha;
                            if(layer_alpha <= 0)
                                continue;
                            Vector<Pointf> shape = InflatePath(g.hit_path, spread);
                            Pointf off(local_shadow.offset_x, local_shadow.offset_y);
                            for(Pointf& q : shape)
                                q += off;
                            FillPath(np, shape, PremultipliedRGBA(local_shadow.color, layer_alpha));
                        }
                    }
                    FillPath(np, g.hit_path, ResolveFace(style.palette.face[si], White()));
                    Color frame = state == UiGraphVisualState::Selected ? SelectedFrameColor(style)
                                                                        : style.palette.frame[si];
                    double frame_width = max(0.50, (double)metrics.frame_width);
                    if(state == UiGraphVisualState::Selected)
                        frame_width = max(frame_width,
                                          (double)max(DPI(2), DPI(fround(lod_policy_.selection_outline_width))));
                    StrokeStyledPath(np, g.hit_path, frame_width, frame, metrics);
                }
            });
        }

        if(zoom_ >= lod_policy_.secondary_text_zoom && !g.micro && !custom
           && style.show_header_band && !node->collapsed && g.header.GetHeight() > 0) {
            Rect hr = g.header.Deflated(max(1, fround(2 * zoom_)), max(1, fround(2 * zoom_)));
            PaintCachedRoundedFill(w, hr, style.header_face[si],
                                   max(0, fround(node->corner_radius * zoom_ * 0.55)));
        }

        if(show_ports && !g.anchors.IsEmpty()) {
            int port_radius = max(1, fround(style.port_radius * min(1.0, max(0.24, zoom_))));
            double normal_stroke = max(1.0, (double)DPI(1));
            for(int a = 0; a < g.anchors.GetCount(); a++) {
                String port_id = g.anchors.GetKey(a);
                UiGraphPortRef candidate{node->ref, port_id};
                const UiGraphPort* port = model_->FindPort(candidate);
                if(!port)
                    continue;
                Color frame = style.port_frame[si];
                if(IsNull(frame)) frame = style.palette.frame[si];
                if(IsNull(frame)) frame = Color(148, 163, 184);
                bool active = hot_port_ == candidate || connection_source_ == candidate;
                if(connection_source_.IsValid() && candidate == connection_target_) {
                    frame = connection_decision_.IsAllowed() ? Color(34, 197, 94) : Color(239, 68, 68);
                    active = true;
                }
                else if(active) {
                    Color semantic = IsNull(port->color) ? UiGraphDefaultTypeColor(port->type) : port->color;
                    frame = BlendGraphColor(frame, semantic, 0.55);
                }
                else if(!IsNull(port->color))
                    frame = BlendGraphColor(frame, port->color, 0.22);
                Point anchor = g.anchors[a];
                Rect pr = RectC(anchor.x - port_radius, anchor.y - port_radius,
                                port_radius * 2 + 1, port_radius * 2 + 1);
                PaintCachedPortMarker(w, pr, canvas, frame,
                                      active ? max(1.5, normal_stroke * 1.5) : normal_stroke);
            }
        }
    }
    last_node_details_paint_usecs_ = max<int64>(0, usecs() - details_started);

    int64 content_started = usecs();
    double content_floor = max(0.24, lod_policy_.secondary_text_zoom - 0.08);
    double text_floor = max(0.24, lod_policy_.title_zoom - 0.04);
    for(int i = 0; i < paint_nodes.GetCount(); i++) {
        const NodeGeometry& g = node_geometry_[paint_nodes[i]];
        const UiGraphNode* node = model_->FindNode(g.ref);
        if(!node)
            continue;
        const UiGraphNodeStyle& style = node_styles[i];
        UiGraphVisualState state = node_states[i];
        if(WhenPaintNodeContent && zoom_ >= content_floor && !g.content.IsEmpty())
            WhenPaintNodeContent(w, *node, g.content, style, state);
        if(zoom_ >= text_floor && (!g.title.IsEmpty() || !g.subtitle.IsEmpty() || !g.icon.IsEmpty()
           || !g.description.IsEmpty() || !g.port_labels.IsEmpty()))
            PaintNodeText(w, *node, g, style, state);
        if(WhenPaintNodeOverlay && zoom_ >= lod_policy_.secondary_text_zoom)
            WhenPaintNodeOverlay(w, *node, g.surface, state);
        bool handled = false;
        if(WhenPaintNodeForeground)
            WhenPaintNodeForeground(w, *node, g.surface, style, state, handled);
    }
    last_node_content_paint_usecs_ = max<int64>(0, usecs() - content_started);

    Rect handle_layer;
    for(int q : paint_edges)
        if(!edge_geometry_[q].route_handle_hit.IsEmpty())
            handle_layer |= edge_geometry_[q].route_handle_hit.Inflated(DPI(2)) & paint;
    if(!handle_layer.IsEmpty()) {
        const Color blue = Color(37, 99, 235);
        UiPaintRenderLayer(w, handle_layer, [&](Painter& hp) {
            for(int q : paint_edges) {
                const EdgeGeometry& g = edge_geometry_[q];
                if(g.route_handle_hit.IsEmpty())
                    continue;
                int radius = DPI(4);
                Rect rr = RectC(g.route_handle.x - radius, g.route_handle.y - radius,
                                radius * 2 + 1, radius * 2 + 1);
                hp.Begin();
                hp.Ellipse(rr.left + 0.5, rr.top + 0.5,
                           max(1.0, rr.GetWidth() - 1.0),
                           max(1.0, rr.GetHeight() - 1.0));
                hp.Fill(Color(219, 234, 254));
                hp.Stroke(max(1.0, (double)DPI(1)), blue);
                hp.End();
            }
        });
    }

    Rect preview_layer;
    for(int q : paint_nodes)
        if(marquee_preview_nodes_.Find(node_geometry_[q].ref.id) >= 0
           && !IsNodeSelected(node_geometry_[q].ref))
            preview_layer |= node_geometry_[q].paint_bounds & paint;
    if(!preview_layer.IsEmpty()) {
        const Color selection = GetEffectiveStyle().selection_box_frame;
        RGBA wash = PremultipliedRGBA(selection, 18);
        UiPaintRenderLayer(w, preview_layer, [&](Painter& pp) {
            for(int q : paint_nodes) {
                const NodeGeometry& g = node_geometry_[q];
                if(marquee_preview_nodes_.Find(g.ref.id) >= 0 && !IsNodeSelected(g.ref)
                   && !g.hit_path.IsEmpty()) {
                    FillPath(pp, g.hit_path, wash);
                    StrokePath(pp, g.hit_path, 1.0, selection, true);
                }
            }
        });
    }

    Rect custom_selection_layer;
    for(int q : paint_nodes) {
        const UiGraphNode* node = model_->FindNode(node_geometry_[q].ref);
        if(node && node->shape == UiGraphNodeShape::Custom && IsNodeSelected(node->ref))
            custom_selection_layer |= node_geometry_[q].paint_bounds & paint;
    }
    if(!custom_selection_layer.IsEmpty()) {
        double stroke_width = max(2.0, (double)DPI(fround(lod_policy_.selection_outline_width)));
        UiPaintRenderLayer(w, custom_selection_layer, [&](Painter& sp) {
            for(int i = 0; i < paint_nodes.GetCount(); i++) {
                const NodeGeometry& g = node_geometry_[paint_nodes[i]];
                const UiGraphNode* node = model_->FindNode(g.ref);
                if(!node || node->shape != UiGraphNodeShape::Custom || !IsNodeSelected(g.ref)
                   || g.hit_path.IsEmpty())
                    continue;
                StrokePath(sp, g.hit_path, stroke_width, SelectedFrameColor(node_styles[i]), true);
            }
        });
    }

    last_node_paint_usecs_ = max<int64>(0, usecs() - node_started);
}

void UiNodeGraph::Paint(Draw& w)
{
    int64 started = usecs();
    Rect outer(Point(0, 0), GetSize());
    const Style& style = GetEffectiveStyle();
    UiPaintStyledBackground(w, outer, style.canvas_palette, style.canvas_metrics,
                            style.canvas_skin, ST_NORMAL, HasFocus());

    if(style.show_grid && style.grid_size > 0) {
        Rect grid_paint = w.GetPaintRect() & outer;
        const int hierarchy = max(2, style.major_grid_every);
        const double base_spacing = style.grid_size * zoom_;
        if(!grid_paint.IsEmpty() && base_spacing > 0.0) {
            Color paper = ResolveFace(style.canvas_palette.face[ST_NORMAL], SColorPaper());
            Color minor = BlendGraphColor(paper, style.grid_minor, 0.45);
            Color major = BlendGraphColor(paper, style.grid_major, 0.58);

            int level = 0;
            double anchor_spacing = base_spacing;
            while(anchor_spacing < 50.0 && level < 8) {
                anchor_spacing *= hierarchy;
                level++;
            }
            double minor_spacing = level == 0 ? base_spacing : anchor_spacing / hierarchy;
            double minor_factor = level == 0 ? 1.0 : SmoothUnit(minor_spacing, 8.0, 18.0);
            bool draw_minor = minor_factor > 0.02;
            if(level > 0)
                minor = BlendGraphColor(paper, minor, minor_factor);

            // Quantize the fine spacing once and derive the major spacing from
            // that exact integer period. This keeps every cached tile perfectly
            // periodic even at fractional zoom (no 1-2 px seam/drift between tiles).
            int minor_step = max(1, fround(minor_spacing));
            int major_step = minor_step * hierarchy;
            Image tile = GetGraphGridTile(minor_step, major_step, draw_minor, minor, major);
            Size ts = tile.GetSize();
            if(ts.cx > 0 && ts.cy > 0) {
                double phase_x = std::fmod(pan_.x, (double)major_step);
                double phase_y = std::fmod(pan_.y, (double)major_step);
                if(phase_x < 0) phase_x += major_step;
                if(phase_y < 0) phase_y += major_step;
                int start_x = outer.left + fround(phase_x);
                int start_y = outer.top + fround(phase_y);
                while(start_x > grid_paint.left) start_x -= ts.cx;
                while(start_y > grid_paint.top) start_y -= ts.cy;
                while(start_x + ts.cx <= grid_paint.left) start_x += ts.cx;
                while(start_y + ts.cy <= grid_paint.top) start_y += ts.cy;
                for(int y = start_y; y < grid_paint.bottom; y += ts.cy)
                    for(int x = start_x; x < grid_paint.right; x += ts.cx)
                        w.DrawImage(x, y, tile);
            }
        }

        if(style.show_origin) {
            Point origin = WorldToScreen(Pointf(0, 0));
            w.DrawLine(origin.x, outer.top, origin.x, outer.bottom, 1, Color(248, 113, 113));
            w.DrawLine(outer.left, origin.y, outer.right, origin.y, 1, Color(96, 165, 250));
        }
    }

    if(model_)
        PaintGraphGeometry(w);
    PaintMarquee(w);
    UiPaintStyledForeground(w, outer, style.canvas_palette, style.canvas_metrics,
                            style.canvas_skin, ST_NORMAL, HasFocus());
    last_paint_usecs_ = max<int64>(0, usecs() - started);
}

} // namespace Upp
