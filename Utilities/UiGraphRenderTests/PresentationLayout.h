#ifndef _UiGraphRenderTests_PresentationLayout_h_
#define _UiGraphRenderTests_PresentationLayout_h_

// Shared by aggregate and standalone presentation packages: one test authority.
template <class Test>
void RunPreparedPresentationTests(Test& t)
{
    struct SmallControl : Ctrl {
        Size GetMinSize() const override { return Size(DPI(20), DPI(12)); }
    } child;
    UiNodeGraph graph;
    graph.SetAutoFitOnFirstPaint(false);
    graph.SetRect(0, 0, DPI(1200), DPI(900));
    UiNodeGraph::Style style = graph.GetStyle();
    style.min_zoom = 0.04;
    style.node.show_port_labels = true;
    style.node.content_cell_reserve = DPI(28);
    graph.SetCustomStyle(style);
    int resolutions = 0, content_calls = 0;
    int badge_height = DPI(12);
    graph.WhenResolveNodePresentation = [&](const UiGraphNode&, const UiGraphNodeStyle&,
                                            UiGraphPresentationRequest& request) {
        resolutions++;
        request.profile = UiGraphPresentationProfile::MediaCard;
        request.badge_height = badge_height;
        request.footer_height = DPI(14);
        request.media_min_height = DPI(12);
    };
    graph.WhenPaintNodeContent = [&](Draw&, const UiGraphNode&, const Rect&,
                                     const UiGraphNodeStyle&, UiGraphVisualState) { content_calls++; };
    UiGraphNode node;
    node.title = "Title";
    node.subtitle = "Subtitle";
    node.description = "Description";
    node.icon = ICON_DESIGN_WIDGETS_48();
    node.icon_size = Size(DPI(18), DPI(18));
    node.position = Pointf(40, 40);
    node.size = Sizef(DPI(480), DPI(360));
    node.corner_radius = DPI(50);
    for(int i = 0; i < 2; i++) {
        UiGraphPort port;
        port.id = i ? "out" : "in";
        port.title = port.id;
        port.direction = i ? UiGraphPortDirection::Output : UiGraphPortDirection::Input;
        port.side = i ? UiGraphPortSide::Right : UiGraphPortSide::Left;
        node.ports.Add(port);
    }
    UiGraphNodeRef ref = graph.Model().AddNode(node);
    graph.SetNodeCtrl(ref, child);
    graph.SetZoom(1, Point(0, 0));
    graph.SetPan(Pointf(0, 0));
    graph.InvalidateNodePresentation();
    UiGraphNodePresentation normal;
    t.Expect(graph.GetNodePresentation(ref, normal) && normal.level == UiGraphPresentationLevel::Normal,
             "prepared presentation identifies authored Normal at useful projected size");
    auto valid = [](const UiGraphNodePresentation& p) {
        const Rect leaves[] = { p.title, p.subtitle, p.icon, p.badge, p.media,
            p.description, p.control, p.footer, p.port_lanes[0], p.port_lanes[1],
            p.port_lanes[2], p.port_lanes[3] };
        for(int i = 0; i < 12; i++) {
            if(leaves[i].IsEmpty()) continue;
            if((leaves[i] & p.safe) != leaves[i]) return false;
            for(int j = i + 1; j < 12; j++)
                if(!(leaves[i] & leaves[j]).IsEmpty()) return false;
        }
        return true;
    };
    t.Expect(normal.fits && valid(normal) && normal.show_control && normal.show_media
             && !normal.badge.IsEmpty() && !normal.footer.IsEmpty(),
             "text media badge control footer and label leaves fit and do not overlap");
    graph.SetZoom(2, Point(0, 0));
    UiGraphNodePresentation large;
    graph.GetNodePresentation(ref, large);
    t.Expect(valid(large) && large.level == UiGraphPresentationLevel::Normal
             && large.text_align == normal.text_align
             && abs((large.title.top - large.safe.top) - 2 * (normal.title.top - normal.safe.top)) <= 3
             && abs(large.media.GetHeight() - 2 * normal.media.GetHeight()) <= 6,
             "Normal enlargement preserves authored composition and alignment");

    graph.SetZoom(0.30, Point(0, 0));
    UiGraphNodePresentation reduced;
    graph.GetNodePresentation(ref, reduced);
    t.Expect(reduced.level == UiGraphPresentationLevel::Lod1 && valid(reduced)
             && reduced.text_align == normal.text_align && !reduced.show_control
             && !reduced.control.IsEmpty() && !reduced.show_footer,
             "LOD 1 suppresses optional features without reclaiming their slots");
    graph.SetZoom(0.15, Point(0, 0));
    graph.GetNodePresentation(ref, reduced);
    t.Expect(reduced.level == UiGraphPresentationLevel::Lod2 && valid(reduced)
             && !reduced.title.IsEmpty() && !reduced.show_subtitle && !reduced.show_port_labels
             && reduced.text_align == normal.text_align,
             "LOD 2 retains identity alignment and removes secondary presentation");

    graph.SetZoom(1, Point(0, 0));
    UiGraphNode small;
    small.size = Sizef(20, 14);
    small.position = Pointf(600, 40);
    UiGraphNodeRef small_ref = graph.Model().AddNode(small);
    UiGraphNodePresentation tiny;
    graph.GetNodePresentation(small_ref, tiny);
    graph.GetNodePresentation(ref, normal);
    t.Expect(tiny.level == UiGraphPresentationLevel::Lod3
             && normal.level == UiGraphPresentationLevel::Normal && tiny.safe.IsEmpty(),
             "same zoom gives different presentation for different authored node sizes");

    int serial = graph.GetGeometryBuildSerial(), before = resolutions;
    graph.GetNodePresentation(ref, normal);
    graph.MiddleDown(Point(200, 200), 0);
    graph.MouseMove(Point(212, 208), 0);
    graph.MiddleUp(Point(212, 208), 0);
    graph.GetNodePresentation(ref, large);
    t.Expect(serial == graph.GetGeometryBuildSerial() && before == resolutions
             && large.title == normal.title.Offseted(12, 8)
             && large.footer == normal.footer.Offseted(12, 8),
             "reusable middle pan projects every prepared slot without layout work");
    graph.BeginBatchUpdate();
    before = resolutions;
    badge_height += DPI(8);
    graph.InvalidateNodePresentation();
    t.Expect(resolutions == before, "batched presentation invalidation defers exact layout");
    graph.EndBatchUpdate();
    graph.GetNodePresentation(ref, large);
    t.Expect(resolutions > before && large.badge.GetHeight() > normal.badge.GetHeight(),
             "explicit presentation invalidation refreshes captured request inputs");

    graph.SetZoom(0.04, Point(0, 0));
    before = resolutions;
    content_calls = 0;
    graph.InvalidateNodePresentation();
    t.Expect(resolutions == before, "exact micro invalidation skips the rich presentation resolver");
    ImageDraw draw(DPI(1200), DPI(900));
    graph.Paint(draw);
    graph.GetNodePresentation(ref, tiny);
    t.Expect(tiny.level == UiGraphPresentationLevel::Lod3 && tiny.safe.IsEmpty()
             && !tiny.show_media && !tiny.show_control && resolutions == before && !content_calls,
             "micro preparation and painting stay free of rich layout/content callbacks");

    // Exercise production silhouettes, including highly rounded canonical Rectangle.
    const UiGraphNodeShape shapes[] = { UiGraphNodeShape::Rectangle, UiGraphNodeShape::Ellipse,
        UiGraphNodeShape::Diamond, UiGraphNodeShape::Triangle, UiGraphNodeShape::Hexagon,
        UiGraphNodeShape::Cloud, UiGraphNodeShape::Document, UiGraphNodeShape::Database };
    bool shapes_valid = true;
    graph.ClearNodeCtrl(ref);
    graph.SetZoom(1, Point(0, 0));
    for(UiGraphNodeShape shape : shapes) {
        UiGraphNode value = clone(node);
        value.shape = shape;
        value.size = Sizef(DPI(640), DPI(480));
        UiGraphNodeRef r = graph.Model().AddNode(value);
        UiGraphNodePresentation p;
        graph.GetNodePresentation(r, p);
        shapes_valid &= valid(p) && !p.safe.IsEmpty();
        // ShapeContains uses the production outline, not the layout's own checker.
        const UiGraphNode* actual = graph.Model().FindNode(r);
        Point origin = graph.WorldToScreen(actual->position);
        Rect surface = UiStyledSurfaceRect(Rect(origin, Size(DPI(640), DPI(480))), style.node.metrics);
        for(int y = p.safe.top; y < p.safe.bottom; y += max(1, p.safe.GetHeight() / 8))
            for(int x = p.safe.left; x < p.safe.right; x += max(1, p.safe.GetWidth() / 8))
                shapes_valid &= UiNodeGraph::ShapeContains(*actual, surface, Point(x, y));
        graph.Model().RemoveNode(r);
    }
    t.Expect(shapes_valid, "all eight canonical shapes keep allocated leaves inside their production interior");
    graph.WhenResolveNodePresentation = [](const UiGraphNode&, const UiGraphNodeStyle&,
                                          UiGraphPresentationRequest& r) {
        r.profile = UiGraphPresentationProfile::Centred;
    };
    graph.InvalidateNodePresentation();
    graph.GetNodePresentation(ref, large);
    t.Expect(large.text_align == UiAlign::CENTER && valid(large),
             "centred profile uses the same bounded allocation contract");

    // Sweep the complete shape/profile matrix through real camera sizes. The
    // thresholds are exercised rather than copied into a preview/test allocator.
    bool matrix_safe = true, matrix_levels = true, matrix_progression = true;
    bool matrix_enlargement = true, stress_safe = true, line_boxes_fit = true;
    const UiGraphPresentationProfile profiles[] = { UiGraphPresentationProfile::Standard,
        UiGraphPresentationProfile::Centred, UiGraphPresentationProfile::MediaCard };
    graph.Model().Clear();
    for(auto profile : profiles) {
        graph.WhenResolveNodePresentation = [profile](const UiGraphNode&, const UiGraphNodeStyle&,
                                                      UiGraphPresentationRequest& r) {
            r.profile = profile;
            r.badge_height = DPI(12); r.footer_height = DPI(14); r.media_min_height = DPI(12);
        };
        for(auto shape : shapes) {
            UiGraphNode value = clone(node);
            value.shape = shape;
            value.size = Sizef(DPI(640), DPI(480));
            value.position = Pointf(0, 0);
            auto r = graph.Model().AddNode(value);
            int seen = 0;
            UiGraphNodePresentation base;
            for(double z : { 0.04, 0.08, 0.13, 0.20, 0.30, 0.42, 0.55, 0.75, 1.0, 1.5, 2.0 }) {
                graph.SetZoom(z, Point(0, 0));
                graph.SetPan(Pointf(0, 0));
                graph.InvalidateNodePresentation();
                UiGraphNodePresentation p;
                matrix_safe &= graph.GetNodePresentation(r, p) && valid(p);
                seen |= 1 << (int)p.level;
                auto measured = [&](Font f) { return f.Height(max(1, fround(max(1, f.GetHeight()) * z))).GetCy(); };
                line_boxes_fit &= (!p.show_title || p.title.GetHeight() >= measured(style.node.title_font))
                    && (!p.show_subtitle || p.subtitle.GetHeight() >= measured(style.node.subtitle_font))
                    && (!p.show_description || p.description.GetHeight() >= measured(style.node.description_font));
                bool full = p.level == UiGraphPresentationLevel::Normal;
                bool secondary = full || p.level == UiGraphPresentationLevel::Lod1;
                matrix_progression &= (!p.show_footer || full) && (!p.show_description || full)
                    && (!p.show_subtitle || secondary) && (!p.show_badge || secondary)
                    && (!p.show_port_labels || secondary)
                    && (p.level != UiGraphPresentationLevel::Lod3
                        || !(p.show_title || p.show_media || p.show_icon || p.show_control));
                if(z == 1.0) base = p;
                if(z > 1.0) {
                    const Rect a[] = { base.header, base.title, base.subtitle, base.badge,
                        base.media, base.footer, base.port_lanes[0], base.port_lanes[1], base.icon };
                    const Rect b[] = { p.header, p.title, p.subtitle, p.badge,
                        p.media, p.footer, p.port_lanes[0], p.port_lanes[1], p.icon };
                    matrix_enlargement &= p.level == base.level && p.text_align == base.text_align;
                    for(int i = 0; i < 9; i++) {
                        matrix_enlargement &= a[i].IsEmpty() == b[i].IsEmpty();
                        if(a[i].IsEmpty() || b[i].IsEmpty()) continue;
                        matrix_enlargement &= abs((b[i].left - p.safe.left) - z * (a[i].left - base.safe.left)) <= 6
                            && abs((b[i].top - p.safe.top) - z * (a[i].top - base.safe.top)) <= 6
                            && abs(b[i].GetWidth() - z * a[i].GetWidth()) <= 6
                            && abs(b[i].GetHeight() - z * a[i].GetHeight()) <= 6;
                    }
                }
            }
            matrix_levels &= seen == 15;
            graph.Model().RemoveNode(r);
            value.size = Sizef(DPI(260), DPI(170));
            r = graph.Model().AddNode(value);
            for(double z : { 1.0, 0.55, 0.32, 0.13 }) {
                graph.SetZoom(z, Point(0, 0));
                graph.InvalidateNodePresentation();
                UiGraphNodePresentation p;
                stress_safe &= graph.GetNodePresentation(r, p) && valid(p);
            }
            graph.Model().RemoveNode(r);
        }
    }
    t.Expect(line_boxes_fit, "visible stock text slots contain measured Windows line boxes at every matrix scale");
    t.Expect(stress_safe, "compact matrix capacity failures retain contained non-overlapping leaves at all reference zooms");
    t.Expect(matrix_safe, "8 shapes x 3 profiles x 11 projected sizes keep all leaf slots contained and disjoint");
    t.Expect(matrix_levels, "every canonical shape/profile reaches all four levels through actual projected sizes");
    t.Expect(matrix_progression, "full matrix feature progression suppresses secondary and rich micro content coherently");
    t.Expect(matrix_enlargement, "all shape/profile Normal layouts retain proportional regions and alignment at 1.5x and 2x");
    // Restore a rich node for the impossible-request test below.
    graph.SetZoom(1, Point(0, 0));
    ref = graph.Model().AddNode(node);

    graph.WhenResolveNodePresentation = [](const UiGraphNode&, const UiGraphNodeStyle&,
                                          UiGraphPresentationRequest& r) { r.badge_height = DPI(10000); };
    graph.InvalidateNodePresentation();
    graph.GetNodePresentation(ref, large);
    t.Expect(!large.fits && valid(large), "impossible host requests report capacity failure without overlapping slots");
}
#endif
