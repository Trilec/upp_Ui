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

String FeatureSummary(const UiGraphNodePresentation& p)
{
    String out;
    auto add = [&](bool on, const char *name) {
        if(!on)
            return;
        if(!out.IsEmpty())
            out << "  ";
        out << name;
    };
    add(p.show_title, "title");
    add(p.show_subtitle, "subtitle");
    add(p.show_icon, "icon");
    add(p.show_badge, "badge");
    add(p.show_media, "media");
    add(p.show_description, "description");
    add(p.show_footer, "footer");
    add(p.show_port_labels, "port labels");
    if(out.IsEmpty())
        out = "silhouette only";
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

class UiGraphDesignMatrix : public TopWindow {
public:
    typedef UiGraphDesignMatrix CLASSNAME;

    UiGraphDesignMatrix()
    {
        Title("UiGraph Design Matrix — Rectangle proof");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1380), DPI(860));

        Add(header_);
        Add(btn_reset_);
        Add(btn_close_);

        header_.SetTitle("UiGraph Design Matrix — Rectangle proof")
               .SetSubTitle("Same authored Rectangle, same production presentation contract; only projected size changes")
               .SetMedia(ICON_DESIGN_WIDGETS_48())
               .SetMediaSide(UiAlign::LEFT)
               .SetMediaAlign(UiAlign::CENTER, UiAlign::CENTER)
               .SetMediaAutoFit(true)
               .ShowTitleLine(false)
               .SetContentInset(DPI(8));

        btn_reset_.SetText("Reset 1:1 matrix");
        btn_close_.SetText("Close");
        btn_reset_.WhenAction = [=] { ResetViews(); };
        btn_close_.WhenAction = [=] { Close(); };

        UiNodeGraph *graphs[] = { &graph_normal_, &graph_lod1_, &graph_lod2_, &graph_lod3_ };
        UiLabel *labels[] = { &lbl_normal_, &lbl_lod1_, &lbl_lod2_, &lbl_lod3_ };
        for(int i = 0; i < 4; i++) {
            Add(*labels[i]);
            Add(*graphs[i]);
            labels[i]->SetAlign(UiAlign::LEFT, UiAlign::CENTER);
            ConfigureRow(*graphs[i], refs_[i], kZooms_[i]);
            graphs[i]->WhenViewport = [=] { SyncLabels(); };
        }

        SyncLabels();
    }

    void Layout() override
    {
        Size client = GetSize();
        const int pad = DPI(12);
        const int gap = DPI(8);
        const int header_h = DPI(72);
        const int action_h = DPI(30);
        const int action_gap = DPI(6);

        header_.SetRect(pad, pad, max(0, client.cx - 2 * pad), header_h);
        btn_close_.SetRect(max(pad, client.cx - pad - DPI(74)),
                           pad + (header_h - action_h) / 2,
                           DPI(74), action_h);
        btn_reset_.SetRect(max(pad, client.cx - pad - DPI(74) - action_gap - DPI(138)),
                           pad + (header_h - action_h) / 2,
                           DPI(138), action_h);

        int top = pad + header_h + gap;
        int body_h = max(0, client.cy - top - pad);
        int label_w = min(DPI(360), max(DPI(250), client.cx * 28 / 100));
        int graph_x = pad + label_w + gap;
        int graph_w = max(0, client.cx - graph_x - pad);

        UiNodeGraph *graphs[] = { &graph_normal_, &graph_lod1_, &graph_lod2_, &graph_lod3_ };
        UiLabel *labels[] = { &lbl_normal_, &lbl_lod1_, &lbl_lod2_, &lbl_lod3_ };
        const int weights[] = { 4, 3, 2, 2 };
        const int weight_total = 11;
        int usable_h = max(0, body_h - gap * 3);
        int y = top;
        int remaining_h = usable_h;
        int remaining_weight = weight_total;

        for(int i = 0; i < 4; i++) {
            int row_h = i == 3 ? remaining_h
                               : remaining_weight > 0 ? usable_h * weights[i] / weight_total : 0;
            row_h = max(0, row_h);
            labels[i]->SetRect(pad, y, label_w, row_h);
            graphs[i]->SetRect(graph_x, y, graph_w, row_h);
            if(refs_[i].IsValid())
                graphs[i]->CenterOnNode(refs_[i]);
            y += row_h + gap;
            remaining_h = max(0, remaining_h - row_h);
            remaining_weight -= weights[i];
        }
        SyncLabels();
    }

private:
    static constexpr double kZooms_[4] = { 1.00, 0.55, 0.32, 0.13 };
    static constexpr UiGraphPresentationLevel kExpected_[4] = {
        UiGraphPresentationLevel::Normal,
        UiGraphPresentationLevel::Lod1,
        UiGraphPresentationLevel::Lod2,
        UiGraphPresentationLevel::Lod3,
    };

    void ConfigureRow(UiNodeGraph& graph, UiGraphNodeRef& ref, double zoom)
    {
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

        graph.WhenResolveNodePresentation = [](const UiGraphNode&, const UiGraphNodeStyle&,
                                               UiGraphPresentationRequest& request) {
            request.profile = UiGraphPresentationProfile::MediaCard;
            request.badge_height = DPI(14);
            request.footer_height = DPI(14);
            request.media_min_height = DPI(20);
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
                if(!inner.IsEmpty())
                    DrawTextEllipsis(w, inner.left, inner.top + max(0, (inner.GetHeight() - small.GetHeight()) / 2),
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
        node.title = "Rectangle";
        node.subtitle = "shared presentation";
        node.description = "One prepared layout owns text, media, badge, footer and port labels.";
        node.position = Pointf(0, 0);
        node.size = Sizef(DPI(260), DPI(170));
        node.shape = UiGraphNodeShape::Rectangle;
        node.role = UiGraphNodeRole::Accent;
        node.corner_radius = DPI(10);
        node.icon = ICON_DESIGN_WIDGETS_48();
        node.icon_size = Size(DPI(18), DPI(18));
        node.ports.Add(MatrixPort("in", "Input", UiGraphPortDirection::Input, UiGraphPortSide::Left));
        node.ports.Add(MatrixPort("out", "Output", UiGraphPortDirection::Output, UiGraphPortSide::Right));

        graph.Model().Clear();
        ref = graph.Model().AddNode(node);
        graph.SetZoom(zoom);
        graph.InvalidateNodePresentation();
    }

    void ResetViews()
    {
        UiNodeGraph *graphs[] = { &graph_normal_, &graph_lod1_, &graph_lod2_, &graph_lod3_ };
        for(int i = 0; i < 4; i++) {
            graphs[i]->BeginViewUpdate();
            graphs[i]->SetZoom(kZooms_[i]);
            if(refs_[i].IsValid())
                graphs[i]->CenterOnNode(refs_[i]);
            graphs[i]->EndViewUpdate();
        }
        SyncLabels();
    }

    void SyncLabels()
    {
        UiNodeGraph *graphs[] = { &graph_normal_, &graph_lod1_, &graph_lod2_, &graph_lod3_ };
        UiLabel *labels[] = { &lbl_normal_, &lbl_lod1_, &lbl_lod2_, &lbl_lod3_ };
        const char *row_names[] = { "NORMAL", "LOD 1", "LOD 2", "LOD 3" };

        for(int i = 0; i < 4; i++) {
            UiGraphNodePresentation p;
            bool prepared = refs_[i].IsValid() && graphs[i]->GetNodePresentation(refs_[i], p);
            const UiGraphNode *node = refs_[i].IsValid() ? graphs[i]->Model().FindNode(refs_[i]) : nullptr;
            int width = node ? max(1, fround(node->size.cx * graphs[i]->GetZoom())) : 0;
            int height = node ? max(1, fround(node->size.cy * graphs[i]->GetZoom())) : 0;
            String actual = prepared ? PresentationLevelName(p.level) : "not prepared";
            String features = prepared ? FeatureSummary(p) : String("-");
            String fit = prepared ? (p.fits ? "fits" : "capacity limited") : "";
            String match = prepared && p.level == kExpected_[i] ? "OK" : "CHECK";
            labels[i]->SetText(Format("%s   [%s]\nzoom %.2f  ·  projected %d × %d px\nprepared %s  ·  %s\n%s",
                                      row_names[i], match, graphs[i]->GetZoom(), width, height,
                                      actual, fit, features));
        }
    }

private:
    UiTitleCard header_;
    UiButton btn_reset_, btn_close_;

    UiLabel lbl_normal_, lbl_lod1_, lbl_lod2_, lbl_lod3_;
    UiNodeGraph graph_normal_, graph_lod1_, graph_lod2_, graph_lod3_;
    UiGraphNodeRef refs_[4];
};

constexpr double UiGraphDesignMatrix::kZooms_[4];
constexpr UiGraphPresentationLevel UiGraphDesignMatrix::kExpected_[4];

GUI_APP_MAIN
{
    UiGraphDesignMatrix().Run();
}
