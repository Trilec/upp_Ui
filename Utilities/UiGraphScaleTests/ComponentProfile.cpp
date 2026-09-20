#include <Ui/Ui.h>

using namespace Upp;

namespace {
// Matched diagnostic fixtures, not an absolute FPS certificate. Time the input
// event as well as Paint: exact preparation and baseline copying happen BEFORE
// Paint and were invisible to a paint-only profile. No authoring package here.
struct ProfileChecks {
    int checks = 0, failed = 0;
    void Expect(bool ok, const char* message)
    {
        checks++;
        if(!ok) failed++;
        Cout() << (ok ? "PASS: " : "FAIL: ") << message << '\n';
    }
};

struct Samples {
    Vector<int64> event, paint;
    int rebuilds = 0, prepared_changes = 0;
    void Add(int64 event_us, int64 paint_us, int builds, int revisions)
    {
        event.Add(event_us); paint.Add(paint_us);
        rebuilds += builds; prepared_changes += revisions;
    }
};

int64 Percentile(const Vector<int64>& samples, int percent)
{
    if(samples.IsEmpty()) return 0;
    Vector<int64> ordered = clone(samples);
    Sort(ordered);
    int at = min(ordered.GetCount() - 1, max(0, (ordered.GetCount() * percent + 99) / 100 - 1));
    return ordered[at];
}

void Report(const char* fixture, const char* mode, const char* phase, const char* operation,
            const Samples& samples, const UiNodeGraph& graph)
{
    Cout() << "UIGRAPH_COMPONENT_PROFILE fixture=" << fixture << " mode=" << mode
           << " phase=" << phase << " operation=" << operation
           << " samples=" << samples.event.GetCount()
           << " event_first_us=" << (samples.event.IsEmpty() ? 0 : samples.event[0])
           << " event_median_us=" << Percentile(samples.event, 50)
           << " event_p95_us=" << Percentile(samples.event, 95)
           << " paint_median_us=" << Percentile(samples.paint, 50)
           << " paint_p95_us=" << Percentile(samples.paint, 95)
           << " full_rebuilds=" << samples.rebuilds
           << " prepared_changes=" << samples.prepared_changes
           << " prepared_nodes=" << graph.GetPreparedNodeCount()
           << " prepared_edges=" << graph.GetPreparedEdgeCount()
           << " painted_nodes=" << graph.GetLastPaintedNodeCount()
           << " painted_edges=" << graph.GetLastPaintedEdgeCount()
           << " components=" << graph.GetLastPaintedComponentCount()
           << " micro_hints=" << graph.GetLastMicroHintCount()
           << " micro_primitives=" << graph.GetLastMicroHintPrimitiveCount()
           << " paint_path=" << (int)graph.GetLastPaintPath()
           << " fallback=" << (int)graph.GetLastPaintFallbackReason() << '\n';
}

UiGraphPort MakePort(const char* id, UiGraphPortDirection direction)
{
    UiGraphPort port;
    port.id = id; port.direction = direction;
    port.type = UiGraphDataType::Flow;
    port.multiplicity = UiGraphPortMultiplicity::Multiple;
    port.side = direction == UiGraphPortDirection::Input ? UiGraphPortSide::Left : UiGraphPortSide::Right;
    return port;
}

void BuildGrid(UiGraphModel& model, Vector<UiGraphNodeRef>& nodes, Size size)
{
    const UiGraphNodeShape shapes[] = {
        UiGraphNodeShape::Rectangle, UiGraphNodeShape::Ellipse, UiGraphNodeShape::Diamond,
        UiGraphNodeShape::Triangle, UiGraphNodeShape::Hexagon, UiGraphNodeShape::Cloud,
        UiGraphNodeShape::Document, UiGraphNodeShape::Database
    };
    const Image icon = ICON_DESIGN_WIDGETS_48();
    nodes.Reserve(10000);
    for(int y = 0; y < 100; y++) for(int x = 0; x < 100; x++) {
        UiGraphNode node;
        node.title = "Asset " + AsString(y * 100 + x) + " / processing";
        node.style_class = "profile";
        node.shape = shapes[(y * 100 + x) % 8];
        node.size = Sizef(size.cx, size.cy);
        node.position = Pointf(x * size.cx * 1.5, y * size.cy * 1.6);
        node.icon = icon; node.icon_size = Size(12, 12);
        ValueMap data; data.Add("progress", (x % 10) / 10.0); node.data = data;
        node.ports.Add(MakePort("in", UiGraphPortDirection::Input));
        node.ports.Add(MakePort("out", UiGraphPortDirection::Output));
        nodes.Add(model.AddNode(node));
    }
    for(int y = 0; y < 100; y++) for(int x = 0; x < 99; x++) {
        int i = y * 100 + x;
        UiGraphEdge edge;
        edge.source = UiGraphPortRef{nodes[i], "out"};
        edge.target = UiGraphPortRef{nodes[i + 1], "in"};
        edge.route = UiGraphRouteStyle::Straight;
        edge.arrow = UiGraphArrowStyle::None;
        model.AddEdge(edge);
    }
}

UiGraphNodeTemplate MakeProfileTemplate()
{
    UiGraphNodeTemplate t;
    t.SetHeaderHeight(0).SetFooterHeight(0).SetLodWidths(160, 80, 48);
    t.micro_hints = true; t.micro_hint_budget = 3;
    // Exactly one shared template; no factory or JSON execution per node.
    auto& icon = t.slots[t.slot_count++];
    icon.id = "icon"; icon.feature = UiGraphNodeSlotFeature::Icon;
    icon.component_kind = UiGraphNodeComponentKind::Icon;
    icon.region = UiGraphNodeSlotRegion::ContentMain;
    icon.placement = UiGraphNodeSlotPlacement::Left; icon.extent = 12;
    icon.gap_after = 2; icon.small = UiGraphNodeSmallMode::Dot;
    auto& text = t.slots[t.slot_count++];
    text.id = "title"; text.feature = UiGraphNodeSlotFeature::Title;
    text.component_kind = UiGraphNodeComponentKind::Text;
    text.region = UiGraphNodeSlotRegion::ContentMain;
    text.small = UiGraphNodeSmallMode::BarThenDot;
    text.font_height = 14; text.readable_min_px = 9;
    auto& progress = t.slots[t.slot_count++];
    progress.id = "progress"; progress.component_kind = UiGraphNodeComponentKind::Progress;
    progress.region = UiGraphNodeSlotRegion::OverlayMain;
    progress.placement = UiGraphNodeSlotPlacement::Bottom; progress.extent = 4;
    progress.data_key = "progress"; progress.small = UiGraphNodeSmallMode::Bar;
    return t;
}

void RunPhase(ProfileChecks& checks, UiNodeGraph& graph, UiGraphModel& model,
              UiGraphNodeRef centre, ImageDraw& draw, const char* fixture,
              const char* mode, const char* phase, double zoom, bool micro)
{
    // Reset only the sample value before each matched phase, outside timing.
    UiGraphNode initial = *model.FindNode(centre);
    ValueMap initial_data = initial.data; initial_data.Set("progress", 0.0); initial.data = initial_data;
    model.UpdateNode(centre, initial);
    graph.SetZoom(zoom, Point(600, 400));
    graph.CenterOnNode(centre);
    graph.SelectNode(centre); // keep this node in any overview sampling population
    graph.Paint(draw);       // one warm-up, not described as a cold-cache benchmark
    Samples idle;
    int builds = graph.GetGeometryBuildSerial(), spatial = graph.GetSpatialBuildSerial();
    for(int i = 0; i < 9; i++) {
        int64 started = usecs(); graph.Paint(draw);
        idle.Add(0, usecs() - started, 0, 0);
    }
    checks.Expect(graph.GetGeometryBuildSerial() == builds && graph.GetSpatialBuildSerial() == spatial,
                  "warm repaint does not rebuild geometry or spatial index");
    Report(fixture, mode, phase, "repaint", idle, graph);

    UiGraphNodePresentation before;
    bool snapshot = graph.GetNodePresentation(centre, before);
    Pointf pan = graph.GetPan();
    Samples pans;
    graph.MiddleDown(Point(600, 400), 0);
    for(int i = 0; i < 9; i++) {
        int b = graph.GetGeometryBuildSerial(), r = graph.GetPreparedGeometryRevision();
        Point to(i % 2 ? 600 : 612, i % 2 ? 400 : 408);
        int64 started = usecs(); graph.MouseMove(to, 0); int64 event_us = usecs() - started;
        started = usecs(); graph.Paint(draw);
        pans.Add(event_us, usecs() - started, graph.GetGeometryBuildSerial() - b,
                 graph.GetPreparedGeometryRevision() - r);
    }
    graph.MiddleUp(Point(612, 408), 0);
    checks.Expect(pans.rebuilds == 0 && graph.GetSpatialBuildSerial() == spatial,
                  "small live pans reuse the prepared scene and world index");
    UiGraphNodePresentation after;
    bool translated = snapshot && graph.GetNodePresentation(centre, after);
    if(translated) {
        Point delta(fround(graph.GetPan().x - pan.x), fround(graph.GetPan().y - pan.y));
        translated = before.components.GetCount() == after.components.GetCount();
        for(int i = 0; translated && i < before.components.GetCount(); i++) {
            const auto& a = before.components[i]; const auto& b = after.components[i];
            translated = a.id == b.id && a.representation == b.representation
                      && (a.footprint.IsEmpty() ? b.footprint.IsEmpty() : b.footprint == a.footprint.Offseted(delta.x, delta.y));
        }
    }
    // Legacy Micro has an intentionally empty presentation, but still has a node.
    checks.Expect(translated, "pan preserves component identity and exact translated footprints");
    Report(fixture, mode, phase, "pan", pans, graph);
    checks.Expect(graph.GetActiveNodeCtrlCount() == 0, "painted components create no live controls");
    if(micro) checks.Expect(graph.GetLastPaintPath() == UiNodeGraph::PaintPath::Micro
                           && graph.GetLastPaintFallbackReason() == UiNodeGraph::PaintFallbackReason::None,
                           "overview pan keeps the native Micro backend");

    // Intentionally report (do not bless) the named-component exact wheel path.
    // A 3% step avoids deliberately jumping whole LOD ranges. Individual shape
    // capacity/pixel boundaries still apply, and all actual rebuilds are counted.
    Samples wheels;
    for(int i = 0; i < 9; i++) {
        int b = graph.GetGeometryBuildSerial(), r = graph.GetPreparedGeometryRevision();
        int64 started = usecs(); graph.MouseWheel(Point(600, 400), i % 2 ? -120 : 120, 0);
        int64 event_us = usecs() - started;
        started = usecs(); graph.Paint(draw);
        wheels.Add(event_us, usecs() - started, graph.GetGeometryBuildSerial() - b,
                   graph.GetPreparedGeometryRevision() - r);
    }
    checks.Expect(graph.GetSpatialBuildSerial() == spatial, "wheel camera changes keep the world spatial index");
    Report(fixture, mode, phase, "wheel", wheels, graph);

    graph.SetZoom(zoom, Point(600, 400)); graph.CenterOnNode(centre);
    Samples updates;
    for(int i = 0; i < 3; i++) {
        UiGraphNode value = *model.FindNode(centre);
        ValueMap data = value.data; data.Set("progress", 0.1 + i * 0.2); value.data = data;
        int b = graph.GetGeometryBuildSerial(), r = graph.GetPreparedGeometryRevision();
        int64 started = usecs(); model.UpdateNode(centre, value); int64 event_us = usecs() - started;
        started = usecs(); graph.Paint(draw);
        updates.Add(event_us, usecs() - started, graph.GetGeometryBuildSerial() - b,
                    graph.GetPreparedGeometryRevision() - r);
    }
    Report(fixture, mode, phase, "one_node_update", updates, graph);

    // Hover is separate from pan. Current hot-state changes can prepare a whole
    // scene; record that honestly instead of inferring locality from node edits.
    Samples hover;
    for(int i = 0; i < 3; i++) {
        int b = graph.GetGeometryBuildSerial(), r = graph.GetPreparedGeometryRevision();
        int64 started = usecs(); graph.MouseMove(i % 2 ? Point(600, 400) : Point(-100, -100), 0);
        int64 event_us = usecs() - started;
        started = usecs(); graph.Paint(draw);
        hover.Add(event_us, usecs() - started, graph.GetGeometryBuildSerial() - b,
                  graph.GetPreparedGeometryRevision() - r);
    }
    Report(fixture, mode, phase, "hover", hover, graph);
    checks.Expect(model.GetNodeCount() == 10000 && model.GetEdgeCount() == 9900,
                  "profiling does not change graph topology");
}
} // namespace

int RunComponentProfileSuite()
{
    ProfileChecks checks;
    Cout() << "UIGRAPH_COMPONENT_PROFILE_CONFIG viewport=1200x800 nodes=10000 edges=9900"
              " shapes=8 samples=9 warmup=1 wheel_step=1.03 timing_gate=MEASURE_ONLY\n";
    Cout() << "UIGRAPH_COMPONENT_RECORD_BYTES " << (int)sizeof(UiGraphNodeComponentPresentation)
           << " excludes_dynamic_payloads_and_camera_baseline\n";
    for(int fixture = 0; fixture < 2; fixture++) {
        UiGraphModel model; Vector<UiGraphNodeRef> nodes;
        BuildGrid(model, nodes, fixture ? Size(320, 210) : Size(64, 44));
        for(int named = 0; named < 2; named++) {
            UiNodeGraph graph;
            graph.SetAutoFitOnFirstPaint(false).SetEditable(false);
            graph.SetRect(0, 0, 1200, 800);
            auto style = graph.GetStyle();
            style.show_grid = false; style.zoom_step = 1.03; style.min_zoom = 0.01;
            style.node.show_port_labels = false; style.node.metrics.shadow.enabled = false;
            graph.SetCustomStyle(style);
            String error;
            if(named && !graph.SetNodeTemplateClass("profile", MakeProfileTemplate(), error)) {
                checks.Expect(false, "shared profile template registration");
                Cout() << error << '\n'; continue;
            }
            graph.SetModel(model); graph.Layout();
            ImageDraw draw(1200, 800);
            const char* size = fixture ? "card320x210" : "compact64x44";
            const char* mode = named ? "components" : "legacy";
            RunPhase(checks, graph, model, nodes[5050], draw, size, mode, "near", 1.0, false);
            RunPhase(checks, graph, model, nodes[5050], draw, size, mode, "mid", fixture ? 0.30 : 0.50, false);
            RunPhase(checks, graph, model, nodes[5050], draw, size, mode, "overview", fixture ? 0.08 : 0.20, true);
        }
    }
    Cout() << "UIGRAPH_COMPONENT_PROFILE_SUMMARY checks=" << checks.checks << " failed=" << checks.failed
           << " timings=MEASURE_ONLY\n";
    return checks.failed ? 1 : 0;
}
