#include <Ui/Ui.h>

using namespace Upp;

namespace {

const char *PresentationLevelName(UiGraphPresentationLevel level)
{
    switch(level) {
    case UiGraphPresentationLevel::Normal: return "Normal";
    case UiGraphPresentationLevel::Lod1:   return "LOD 1";
    case UiGraphPresentationLevel::Lod2:   return "LOD 2";
    case UiGraphPresentationLevel::Lod3:   return "LOD 3";
    default:                               return "?";
    }
}

String FeatureSummary(const UiGraphNodePresentation& p, bool visible)
{
    String out;
    auto add = [&](bool on, const char *name) {
        if(on != visible)
            return;
        if(!out.IsEmpty())
            out << " ";
        out << name;
    };
    add(p.show_control, "C");
    add(p.show_title, "T");
    add(p.show_subtitle, "S");
    add(p.show_icon, "I");
    add(p.show_badge, "B");
    add(p.show_media, "M");
    add(p.show_description, "D");
    add(p.show_footer, "F");
    add(p.show_port_labels, "P");
    if(out.IsEmpty())
        out = visible ? "silhouette only" : "none";
    return out;
}

UiGraphPort MatrixPort(const char *id, const char *title,
                       UiGraphPortDirection direction, UiGraphPortSide side)
{
    UiGraphPort port;
    port.id = id;
    port.title = title;
    port.direction = direction;
    port.type = UiGraphDataType::Flow;
    port.side = side;
    port.multiplicity = UiGraphPortMultiplicity::Multiple;
    return port;
}

} // namespace


const UiGraphNodeShape shapes[] = { UiGraphNodeShape::Rectangle, UiGraphNodeShape::Ellipse,
    UiGraphNodeShape::Diamond, UiGraphNodeShape::Triangle, UiGraphNodeShape::Hexagon,
    UiGraphNodeShape::Cloud, UiGraphNodeShape::Document, UiGraphNodeShape::Database };
const char *shape_names[] = { "Rectangle", "Ellipse", "Diamond", "Triangle", "Hexagon",
                             "Cloud", "Document", "Database" };
const double row_zooms[] = { 1.0, 0.55, 0.32, 0.13 };
const char *row_names[] = { "Normal", "LOD 1", "LOD 2", "LOD 3" };

// Each cell is an ordinary production graph. Only the enclosing demo UI is laid
// out here; every node-content rectangle comes from GetNodePresentation.
class MatrixCell : public Ctrl {
    struct MatrixButton : Button {
        Size GetMinSize() const override { return Size(DPI(72), DPI(24)); }
    } child;
    UiNodeGraph graph;
    UiGraphNodeRef ref;
    UiLabel evidence;
    String expected;
public:
    MatrixCell() {
        Add(graph); Add(evidence);
        evidence.SetAlign(UiAlign::LEFT, UiAlign::TOP);
        graph.WhenViewport = [=] { Sync(); };
    }
    void Layout() override {
        evidence.SetRect(6, 2, max(0, GetSize().cx - 12), DPI(116));
        graph.SetRect(0, DPI(120), GetSize().cx, max(0, GetSize().cy - DPI(120)));
        if(ref.IsValid()) graph.CenterOnNode(ref);
        Sync();
    }
    void Sync() {
        UiGraphNodePresentation p;
        if(!graph.GetNodePresentation(ref, p)) return;
        const UiGraphNode *n = graph.Model().FindNode(ref);
        if(!n) return;
        String capacity = p.safe.IsEmpty() ? "not assessed (micro)" : p.fits ? "fits" : "CAPACITY LIMITED";
        evidence.SetText(Format("%s / expected %s\nactual %s | %.2f x | %d x %d px\n%s\nshown: %s\nhidden: %s",
            n->title, expected, PresentationLevelName(p.level), graph.GetZoom(),
            fround(n->size.cx * graph.GetZoom()), fround(n->size.cy * graph.GetZoom()),
            capacity, FeatureSummary(p, true), FeatureSummary(p, false)));
    }
    void Configure(int shape, int scenario, bool spacious, double zoom, const char *row) {
        expected = row;
        graph.SetAutoFitOnFirstPaint(false)
             .SetEditable(false)
             .EnableInternalMutation(false);

        UiNodeGraph::Style style = UiNodeGraph::StyleDefault();
        style.show_grid = false;
        style.min_zoom = 0.04;
        style.max_zoom = 4.0;
        style.node.show_header_band = true;
        style.node.show_description = true;
        style.node.show_port_labels = true;
        style.node.header_height = DPI(46);
        style.node.metrics.content_margin = Rect(DPI(10), DPI(8), DPI(10), DPI(8));
        style.node.metrics.shadow.enabled = true;
        style.node.metrics.shadow.distance = DPI(3);
        style.node.metrics.shadow.offset_x = DPI(1);
        style.node.metrics.shadow.offset_y = DPI(2);
        style.node.metrics.shadow.alpha = 28;
        for(int i = 0; i < 4; i++) {
            style.canvas_palette.face[i] = UiFill::Solid(Color(248, 250, 252));
            style.node.palette.face[i] = UiFill::Solid(Color(250, 251, 253));
            style.node.palette.frame[i] = Color(92, 108, 132);
            style.node.header_face[i] = Color(240, 244, 248);
            style.node.title_ink[i] = Color(36, 46, 61);
            style.node.subtitle_ink[i] = Color(91, 105, 124);
            style.node.description_ink[i] = Color(91, 105, 124);
            style.node.port_frame[i] = Color(78, 97, 124);
            style.node.port_label_ink[i] = Color(91, 105, 124);
        }
        graph.SetCustomStyle(style);

        graph.WhenResolveNodePresentation = [scenario](const UiGraphNode&, const UiGraphNodeStyle&,
                                               UiGraphPresentationRequest& request) {
            request.profile = scenario == 1 ? UiGraphPresentationProfile::Centred
                            : scenario == 2 ? UiGraphPresentationProfile::MediaCard
                                            : UiGraphPresentationProfile::Standard;
            request.badge_height = scenario >= 2 ? DPI(14) : 0;
            request.footer_height = DPI(14);
            request.media_min_height = scenario == 2 ? DPI(20) : 0;
        };

        UiNodeGraph *owner = &graph;
        graph.WhenPaintNodeContent = [owner](Draw& w, const UiGraphNode& node, const Rect& media,
                                             const UiGraphNodeStyle&, UiGraphVisualState) {
            UiGraphNodePresentation p;
            if(!owner->GetNodePresentation(node.ref, p))
                return;
            double zoom = owner->GetZoom();
            Font small = SansSerifZ(max(1, fround(DPI(8) * zoom)));
            Font strong = small;
            strong.Bold();
            const Color ink(55, 65, 81);

            if(p.show_badge && !p.badge.IsEmpty()) {
                w.DrawRect(p.badge, Color(226, 232, 240));
                DrawTextEllipsis(w, p.badge.left + max(1, fround(DPI(4) * zoom)), p.badge.top,
                                 max(0, p.badge.GetWidth() - max(2, fround(DPI(8) * zoom))),
                                 "BADGE", "...", strong, ink);
            }
            if(p.show_media && !media.IsEmpty()) {
                w.DrawRect(media, Color(219, 234, 254));
                int inset = max(1, fround(DPI(3) * zoom));
                Rect inner = media.Deflated(inset, inset);
                if(inner.GetHeight() >= small.GetCy())
                    DrawTextEllipsis(w, inner.left, inner.top + max(0, (inner.GetHeight() - small.GetCy()) / 2),
                                     inner.GetWidth(), "prepared media slot", "...", small, Color(55, 92, 140));
            }
            if(p.show_footer && !p.footer.IsEmpty()) {
                w.DrawRect(p.footer, Color(241, 245, 249));
                DrawTextEllipsis(w, p.footer.left + max(1, fround(DPI(4) * zoom)), p.footer.top,
                                 max(0, p.footer.GetWidth() - max(2, fround(DPI(8) * zoom))),
                                 "status: ready", "...", small, Color(71, 85, 105));
            }
        };

        UiGraphNode node;
        node.title = shape_names[shape];
        node.subtitle = "shared presentation";
        node.description = "One prepared layout owns text, media, badge, footer and port labels.";
        node.position = Pointf(0, 0);
        node.size = spacious ? Sizef(DPI(480), DPI(360)) : Sizef(DPI(260), DPI(170));
        node.shape = shapes[shape];
        node.role = UiGraphNodeRole::Accent;
        node.corner_radius = DPI(10);
        node.icon = ICON_DESIGN_WIDGETS_48();
        node.icon_size = Size(DPI(18), DPI(18));
        node.ports.Add(MatrixPort("in", "Input", UiGraphPortDirection::Input, UiGraphPortSide::Left));
        node.ports.Add(MatrixPort("out", "Output", UiGraphPortDirection::Output, UiGraphPortSide::Right));

        graph.ClearNodeCtrl(ref);
        graph.Model().Clear();
        ref = graph.Model().AddNode(node);
        if(scenario == 3) {
            child.SetLabel("Run");
            graph.SetNodeCtrl(ref, child);
        }
        graph.SetZoom(zoom);
        graph.InvalidateNodePresentation();

        Layout();
    }
};

class UiGraphDesignMatrix : public TopWindow {
    Label heading, help, legend;
    DropList scenario, authored, selected_shape;
    Button reset, compare;
    Ctrl viewport, sheet;
    ScrollBar horizontal, vertical;
    Array<MatrixCell> cells;
    int column_width = 0;
    int sheet_height = 0;

    void Scroll() {
        sheet.SetRect(-horizontal.Get(), -vertical.Get(), column_width * 8, sheet_height);
    }
    void Arrange() {
        if(cells.GetCount() != 32) return; // Frame insertion can trigger Layout during construction.
        bool spacious = authored.GetIndex() == 1;
        column_width = DPI(spacious ? 520 : 340);
        int y = 0;
        for(int row = 0; row < 4; row++) {
            int h = DPI(150) + fround(DPI(spacious ? 360 : 170) * row_zooms[row]);
            for(int col = 0; col < 8; col++)
                cells[row * 8 + col].SetRect(col * column_width, y, column_width - DPI(8), h);
            y += h + DPI(8);
        }
        sheet_height = y;
        horizontal.SetTotal(column_width * 8);
        vertical.SetTotal(sheet_height);
        Scroll();
    }
    void Configure() {
        for(int row = 0; row < 4; row++)
            for(int col = 0; col < 8; col++)
                cells[row * 8 + col].Configure(col, scenario.GetIndex(), authored.GetIndex() == 1,
                                              row_zooms[row], row_names[row]);
        Arrange();
    }
    void Compare() {
        TopWindow window;
        MatrixCell normal, enlarged;
        int shape = selected_shape.GetIndex();
        window.Title(String(shape_names[shape]) + " — Normal composition: 1.00x / 1.50x");
        window.Sizeable().Zoomable();
        window.SetRect(0, 0, DPI(1320), DPI(760));
        window.Add(normal.LeftPos(0, DPI(540)).VSizePos());
        window.Add(enlarged.HSizePos(DPI(548), 0).VSizePos());
        // Spacious authored content is identical in both views. Constrained
        // shapes may still report capacity limits; enlargement never repairs it.
        normal.Configure(shape, scenario.GetIndex(), true, 1.0, "Normal");
        enlarged.Configure(shape, scenario.GetIndex(), true, 1.5, "Normal enlarged");
        window.OpenMain();
        window.Run();
    }
public:
    UiGraphDesignMatrix() {
        Title("UiGraph Design Matrix — eight production shapes");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1500), DPI(980));
        Add(heading); Add(help); Add(legend); Add(scenario); Add(authored); Add(selected_shape); Add(reset); Add(compare);
        heading.SetLabel("UiGraph / production Design Matrix").SetFont(SansSerif(22).Bold());
        help.SetLabel("Columns: eight shapes. Rows: Normal / LOD 1 / LOD 2 / LOD 3. Scroll both axes; wheel inside a cell changes its real camera.");
        legend.SetLabel("Features: T title   S subtitle   I icon   B badge   M media   D description   F footer   P port labels   C native control. Micro capacity is not assessed.");
        scenario.Add("Standard / text").Add("Centred / text").Add("MediaCard / badge + media + footer").Add("Standard / native control capacity");
        scenario.SetIndex(2);
        authored.Add("Authored 260 x 170 / capacity stress").Add("Authored 480 x 360 / spacious");
        authored.SetIndex(0);
        for(const char *name : shape_names) selected_shape.Add(name);
        selected_shape.SetIndex(0);
        reset.SetLabel("Reset cameras"); compare.SetLabel("Compare 1x / 1.5x");
        scenario.WhenAction = authored.WhenAction = reset.WhenAction = [=] { Configure(); };
        compare.WhenAction = [=] { Compare(); };
        Add(viewport); viewport.Add(sheet);
        viewport.AddFrame(horizontal.Horz()); viewport.AddFrame(vertical);
        horizontal.WhenScroll = vertical.WhenScroll = [=] { Scroll(); };
        for(int i = 0; i < 32; i++) sheet.Add(cells.Add());
        Configure();
    }
    void Layout() override {
        int w = GetSize().cx;
        heading.SetRect(DPI(12), DPI(6), w - DPI(24), DPI(30));
        scenario.SetRect(DPI(12), DPI(42), DPI(300), DPI(28));
        authored.SetRect(DPI(322), DPI(42), DPI(290), DPI(28));
        selected_shape.SetRect(DPI(622), DPI(42), DPI(135), DPI(28));
        compare.SetRect(DPI(767), DPI(42), DPI(155), DPI(28));
        reset.SetRect(DPI(932), DPI(42), DPI(125), DPI(28));
        help.SetRect(DPI(12), DPI(76), w - DPI(24), DPI(24));
        legend.SetRect(DPI(12), DPI(100), w - DPI(24), DPI(24));
        viewport.SetRect(DPI(12), DPI(130), max(0, w - DPI(24)), max(0, GetSize().cy - DPI(142)));
        horizontal.SetPage(viewport.GetSize().cx); vertical.SetPage(viewport.GetSize().cy);
        Arrange();
    }
};

GUI_APP_MAIN
{
    UiGraphDesignMatrix().Run();
}
