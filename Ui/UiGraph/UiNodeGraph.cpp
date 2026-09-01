#include <Ui/UiGraph/UiNodeGraph.h>
#include <Ui/Ui.h>
#include <Ui/UiRenderLayer.h>

// Preserve the validated R9.2/R9.3B implementation in this translation unit,
// but keep the three paint stages under legacy names. R9.3C can therefore
// replace only the visual/render-policy layer without rewriting spatial/model/
// interaction code or losing the previous implementation as a recovery path.
//
// R10A keeps the retained implementation source-compatible with its historical
// enum vocabulary while canonical authored code uses Rectangle/Ellipse. Mapping
// old Rectangle to wire-0 LegacyRectangle here is important: canonical Rectangle
// has a distinct wire value and owns corner_radius semantics.
//
// The retained implementation historically called RefreshLayout() after view,
// model and style methods that had already synchronously rebuilt geometry and
// attached controls. Layout() then invalidated that fresh geometry and rebuilt it
// again. Keep layout scheduling only for the one case that genuinely requires a
// later Layout pass: pending first-paint auto-fit. Natural host resize Layout
// callbacks are unaffected by this source-level compatibility shim.
#define RefreshLayout() do { if(auto_fit_first_paint_ && !first_paint_done_) Ctrl::RefreshLayout(); } while(0)
#define Rectangle                    LegacyRectangle
#define UsesRectangularStyledSurface UsesRectangularStyledSurfaceLegacy
#define Paint                         PaintLegacy
#define PaintGraphGeometry            PaintGraphGeometryLegacy
#define PaintNodeDetails              PaintNodeDetailsLegacy
#include "UiNodeGraphBase.inc"
#undef PaintNodeDetails
#undef PaintGraphGeometry
#undef Paint
#undef UsesRectangularStyledSurface
#undef Rectangle
#undef RefreshLayout

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
    // The old stock Graph style enabled a soft drop shadow on every node. Keep
    // shadows fully supported for explicit presets/styles, but make the stock
    // unclassified node flat. Matching the complete old shadow recipe avoids
    // silently disabling a deliberately authored custom shadow.
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
    return max(0, fround(node.corner_radius * zoom));
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
        FillPath(p, RoundedPolygon(header, max(0.0, node.corner_radius * zoom_ * 0.55), 5),
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
        Vector<Pointf> ring = EllipsePath(pr, 20);

        Color frame = style.port_frame[si];
        if(IsNull(frame))
            frame = style.palette.frame[si];
        if(IsNull(frame))
            frame = Color(148, 163, 184);

        bool active = hot_port_ == candidate || connection_source_ == candidate;
        if(connection_source_.IsValid() && candidate == connection_target_) {
            frame = connection_decision_.IsAllowed() ? Color(34, 197, 94)
                                                      : Color(239, 68, 68);
            active = true;
        }
        else if(active) {
            Color semantic = IsNull(port->color) ? UiGraphDefaultTypeColor(port->type)
                                                 : port->color;
            frame = BlendGraphColor(frame, semantic, 0.55);
        }
        else if(!IsNull(port->color))
            frame = BlendGraphColor(frame, port->color, 0.22);

        // The centre deliberately uses the graph background rather than the node
        // fill. At the node boundary this reads as a small hollow socket/cutout,
        // while the independent port_hit_radius keeps interaction forgiving.
        FillPath(p, ring, canvas);
        StrokePath(p, ring, active ? max(1.5, normal_stroke * 1.5) : normal_stroke,
                   frame, true);
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
    QuerySpatial(paint_world, node_ids, edge_ids);
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

    // Resolve presentation once for each visible object. R9.2 resolved node
    // styles independently in surface/details/content passes; expensive host
    // resolvers therefore ran three or more times for the same node/frame.
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

    auto needs_node_details = [&](const UiGraphNode& node,
                                  const UiGraphNodeStyle& style,
                                  const NodeGeometry& geometry) {
        if(!UsesRectangularStyledSurface(node.shape))
            return true;
        if(zoom_ >= lod_policy_.secondary_text_zoom && !geometry.micro
           && style.show_header_band && !node.collapsed && geometry.header.GetHeight() > 0)
            return true;
        double port_floor = max(0.12, lod_policy_.port_zoom - 0.04);
        double port_full = max(port_floor + 0.08, lod_policy_.port_zoom + 0.16);
        return SmoothUnit(zoom_, port_floor, port_full) > 0.01 && !geometry.anchors.IsEmpty();
    };

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

        // Stock R9.3C nodes keep the retained face rectangle after their old
        // default shadow is suppressed. Explicit authored shadows still use the
        // canonical outer geometry and established Ui background compositor.
        Rect target = stock_shadow_was_suppressed ? g.surface : g.rect;
        UiPaintStyledBackground(draw, target, palette, metrics, skin,
                                ToStyledState(state), false);
    };

    int64 edge_started = usecs();
    if(!paint_edges.IsEmpty() || interaction_ == InteractionMode::Connect) {
        Rect edge_layer;
        for(int q : paint_edges)
            edge_layer |= edge_geometry_[q].bounds & paint;
        // Connection preview is transient and may extend beyond prepared edge
        // bounds. Keep the old full dirty-rect fallback only for that gesture.
        if(interaction_ == InteractionMode::Connect)
            edge_layer = paint;
        UiPaintRenderLayer(w, edge_layer, [&](Painter& ep) {
            for(int i = 0; i < paint_edges.GetCount(); i++) {
                const EdgeGeometry& g = edge_geometry_[paint_edges[i]];
                const UiGraphEdge* edge = model_->FindEdge(g.ref);
                if(!edge)
                    continue;
                last_painted_edge_count_++;
                if(g.simplified)
                    last_simplified_edge_count_++;
                PaintEdge(ep, *edge, g, edge_styles[i], edge_states[i]);
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
            if(WhenPaintEdgeOverlay && zoom_ >= lod_policy_.edge_simplify_zoom)
                WhenPaintEdgeOverlay(w, *edge, g.points, edge_states[i]);
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
    // Details are local to a node. The old path allocated/cleared one viewport-
    // sized AA surface merely to draw a handful of ports and irregular node
    // outlines. Paint each required node into its retained local damage bounds
    // instead; at overview the existing LOD still skips the phase entirely.
    for(int i = 0; i < paint_nodes.GetCount(); i++) {
        const NodeGeometry& g = node_geometry_[paint_nodes[i]];
        const UiGraphNode* node = model_->FindNode(g.ref);
        if(!node || !needs_node_details(*node, node_styles[i], g))
            continue;
        Rect target = g.paint_bounds.Inflated(DPI(2)) & paint;
        UiPaintRenderLayer(w, target, [&](Painter& np) {
            PaintNodeDetails(np, *node, g, node_styles[i], node_states[i]);
        });
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
                Vector<Pointf> circle = EllipsePath(rr, 20);
                FillPath(hp, circle, Color(219, 234, 254));
                StrokePath(hp, circle, max(1.0, (double)DPI(1)), blue, true);
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

    // R9.3C hierarchical dot grid, now emitted as one sparse alpha raster for the
    // current dirty rectangle. This preserves the same world-space hierarchy and
    // fade without issuing thousands of individual DrawRect calls per frame.
    if(style.show_grid && style.grid_size > 0) {
        Rect grid_paint = w.GetPaintRect() & outer;
        const int hierarchy = max(2, style.major_grid_every);
        const double base_spacing = style.grid_size * zoom_;
        if(!grid_paint.IsEmpty() && base_spacing > 0.0) {
            Color paper = ResolveFace(style.canvas_palette.face[ST_NORMAL], SColorPaper());
            Color minor = BlendGraphColor(paper, style.grid_minor, 0.45);
            Color major = BlendGraphColor(paper, style.grid_major, 0.58);

            ImageBuffer grid(grid_paint.GetSize());
            grid.SetKind(IMAGE_ALPHA);
            Fill(~grid, RGBAZero(), grid.GetLength());
            RGBA *pixels = ~grid;
            const int stride = grid_paint.GetWidth();

            auto raster_dots = [&](double step, Color color) {
                if(step < 4.0 || IsNull(color))
                    return;
                double ox = std::fmod(pan_.x, step);
                double oy = std::fmod(pan_.y, step);
                if(ox < 0) ox += step;
                if(oy < 0) oy += step;
                RGBA dot = PremultipliedRGBA(color, 255);
                for(double y = outer.top + oy; y < outer.bottom; y += step) {
                    int py = fround(y);
                    if(py < grid_paint.top || py >= grid_paint.bottom)
                        continue;
                    int row = (py - grid_paint.top) * stride;
                    for(double x = outer.left + ox; x < outer.right; x += step) {
                        int px = fround(x);
                        if(px >= grid_paint.left && px < grid_paint.right)
                            pixels[row + px - grid_paint.left] = dot;
                    }
                }
            };

            int level = 0;
            double anchor_spacing = base_spacing;
            while(anchor_spacing < 50.0 && level < 8) {
                anchor_spacing *= hierarchy;
                level++;
            }

            if(level == 0) {
                raster_dots(anchor_spacing, minor);
                raster_dots(anchor_spacing * hierarchy, major);
            }
            else {
                double finer_spacing = anchor_spacing / hierarchy;
                double finer_factor = SmoothUnit(finer_spacing, 8.0, 18.0);
                if(finer_factor > 0.02)
                    raster_dots(finer_spacing, BlendGraphColor(paper, minor, finer_factor));
                raster_dots(anchor_spacing, major);
            }
            w.DrawImage(grid_paint.left, grid_paint.top, grid);
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
