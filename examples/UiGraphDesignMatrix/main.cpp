#include <Ui/Ui.h>

using namespace Upp;

namespace {

const UiGraphNodeShape kShapes[] = {
    UiGraphNodeShape::Rectangle, UiGraphNodeShape::Ellipse,
    UiGraphNodeShape::Diamond, UiGraphNodeShape::Triangle,
    UiGraphNodeShape::Hexagon, UiGraphNodeShape::Cloud,
    UiGraphNodeShape::Document, UiGraphNodeShape::Database
};

const char *kShapeIds[] = {
    "rectangle", "ellipse", "diamond", "triangle",
    "hexagon", "cloud", "document", "database"
};

const char *kShapeNames[] = {
    "Rectangle", "Ellipse", "Diamond", "Triangle",
    "Hexagon", "Cloud", "Document", "Database"
};

const char *kRowNames[] = { "Normal", "LOD 1", "LOD 2", "LOD 3" };
const char *kFeatureTags[] = { "TLE", "SUB", "ICO", "BGE", "MED", "DES", "FOOT", "PLAB", "CONT" };
const char *kFeatureNames[] = { "Title", "Subtitle", "Icon", "Badge", "Media", "Description", "Footer", "Port labels", "Native control" };

String PresentationLevelName(UiGraphPresentationLevel level)
{
    switch(level) {
    case UiGraphPresentationLevel::Normal: return "Normal";
    case UiGraphPresentationLevel::Lod1:   return "LOD 1";
    case UiGraphPresentationLevel::Lod2:   return "LOD 2";
    case UiGraphPresentationLevel::Lod3:   return "LOD 3";
    default:                               return "?";
    }
}

struct ThresholdSet : Moveable<ThresholdSet> {
    double normal = 56.0;
    double lod1 = 33.0;
    double lod2 = 13.0;

    void Normalize()
    {
        normal = minmax(normal, 8.0, 96.0);
        lod1 = minmax(lod1, 4.0, normal - 4.0);
        lod2 = minmax(lod2, 1.0, lod1 - 4.0);
    }

    void Jsonize(JsonIO& io)
    {
        io("normal", normal)("lod1", lod1)("lod2", lod2);
        if(io.IsLoading())
            Normalize();
    }
};

struct FeatureSet : Moveable<FeatureSet> {
    bool title = false;
    bool subtitle = false;
    bool icon = false;
    bool badge = false;
    bool media = false;
    bool description = false;
    bool footer = false;
    bool port_labels = false;
    bool control = false;

    bool Get(int i) const
    {
        switch(i) {
        case 0: return title;
        case 1: return subtitle;
        case 2: return icon;
        case 3: return badge;
        case 4: return media;
        case 5: return description;
        case 6: return footer;
        case 7: return port_labels;
        case 8: return control;
        default: return false;
        }
    }

    void Set(int i, bool value)
    {
        switch(i) {
        case 0: title = value; break;
        case 1: subtitle = value; break;
        case 2: icon = value; break;
        case 3: badge = value; break;
        case 4: media = value; break;
        case 5: description = value; break;
        case 6: footer = value; break;
        case 7: port_labels = value; break;
        case 8: control = value; break;
        }
    }

    void Jsonize(JsonIO& io)
    {
        io("title", title)
          ("subtitle", subtitle)
          ("icon", icon)
          ("badge", badge)
          ("media", media)
          ("description", description)
          ("footer", footer)
          ("port_labels", port_labels)
          ("control", control);
    }
};

struct ShapePolicy : Moveable<ShapePolicy> {
    String shape;
    bool threshold_override = false;
    ThresholdSet thresholds;
    Vector<FeatureSet> lod;

    void Jsonize(JsonIO& io)
    {
        io("shape", shape)
          ("threshold_override", threshold_override)
          ("thresholds", thresholds)
          ("lod", lod);
    }
};

struct TemplatePolicy : Moveable<TemplatePolicy> {
    String id;
    String name;
    ThresholdSet global_thresholds;
    Vector<ShapePolicy> shapes;

    void Jsonize(JsonIO& io)
    {
        io("id", id)
          ("name", name)
          ("global_thresholds", global_thresholds)
          ("shapes", shapes);
    }
};

struct MatrixDocument : Moveable<MatrixDocument> {
    int schema_version = 1;
    String active_template = "media_card";
    String authored_size = "compact";
    String port_preset = "1x1";
    Vector<TemplatePolicy> templates;

    void Jsonize(JsonIO& io)
    {
        io("schema_version", schema_version)
          ("active_template", active_template)
          ("authored_size", authored_size)
          ("port_preset", port_preset)
          ("templates", templates);
    }
};

struct TemplateSpec {
    const char *id;
    const char *name;
    UiGraphPresentationProfile profile;
    bool badge;
    bool media;
    bool footer;
    bool control;
};

const TemplateSpec kTemplates[] = {
    { "minimal",        "Minimal",        UiGraphPresentationProfile::Centred,  false, false, false, false },
    { "compact",        "Compact",        UiGraphPresentationProfile::Standard, false, false, false, false },
    { "standard",       "Standard",       UiGraphPresentationProfile::Standard, false, false, false, false },
    { "status_card",    "Status Card",    UiGraphPresentationProfile::Standard, true,  false, true,  false },
    { "media_card",     "Media Card",     UiGraphPresentationProfile::MediaCard,true,  true,  true,  false },
    { "parameter_node", "Parameter Node", UiGraphPresentationProfile::Standard, false, false, false, true  },
};

int FindTemplateIndex(const String& id)
{
    for(int i = 0; i < (int)__countof(kTemplates); i++)
        if(id == kTemplates[i].id)
            return i;
    return 0;
}

FeatureSet DefaultFeatures(int template_index, int row)
{
    FeatureSet f;
    switch(template_index) {
    case 0: // Minimal
        if(row == 0) { f.title = true; f.icon = true; }
        else if(row == 1) f.title = true;
        break;
    case 1: // Compact
        if(row == 0) { f.title = f.subtitle = f.icon = f.port_labels = true; }
        else if(row == 1) { f.title = f.icon = f.port_labels = true; }
        else if(row == 2) f.title = true;
        break;
    case 2: // Standard
        if(row == 0) { f.title = f.subtitle = f.icon = f.description = f.port_labels = true; }
        else if(row == 1) { f.title = f.subtitle = f.icon = f.port_labels = true; }
        else if(row == 2) { f.title = f.icon = true; }
        break;
    case 3: // Status Card
        if(row == 0) { f.title = f.subtitle = f.icon = f.badge = f.description = f.footer = f.port_labels = true; }
        else if(row == 1) { f.title = f.icon = f.badge = f.port_labels = true; }
        else if(row == 2) { f.title = f.badge = true; }
        break;
    case 4: // Media Card
        if(row == 0) { f.title = f.subtitle = f.icon = f.badge = f.media = f.description = f.footer = f.port_labels = true; }
        else if(row == 1) { f.title = f.subtitle = f.icon = f.badge = f.media = f.port_labels = true; }
        else if(row == 2) { f.title = f.icon = f.media = true; }
        break;
    case 5: // Parameter Node
        if(row == 0) { f.title = f.subtitle = f.icon = f.description = f.port_labels = f.control = true; }
        else if(row == 1) { f.title = f.icon = f.port_labels = true; }
        else if(row == 2) f.title = true;
        break;
    }
    return f;
}

ShapePolicy MakeShapePolicy(int template_index, int shape_index)
{
    ShapePolicy p;
    p.shape = kShapeIds[shape_index];
    p.lod.SetCount(4);
    for(int row = 0; row < 4; row++)
        p.lod[row] = DefaultFeatures(template_index, row);
    return p;
}

MatrixDocument MakeDefaultDocument()
{
    MatrixDocument doc;
    doc.templates.SetCount(__countof(kTemplates));
    for(int t = 0; t < doc.templates.GetCount(); t++) {
        TemplatePolicy& p = doc.templates[t];
        p.id = kTemplates[t].id;
        p.name = kTemplates[t].name;
        p.shapes.SetCount(__countof(kShapes));
        for(int s = 0; s < p.shapes.GetCount(); s++)
            p.shapes[s] = MakeShapePolicy(t, s);
    }
    return doc;
}

bool ValidateDocument(const MatrixDocument& doc, String& error)
{
    if(doc.schema_version != 1) {
        error = "Unsupported schema_version (expected 1).";
        return false;
    }
    if(doc.templates.GetCount() != (int)__countof(kTemplates)) {
        error = "Policy must contain the six built-in presentation templates.";
        return false;
    }
    for(int t = 0; t < doc.templates.GetCount(); t++) {
        const TemplatePolicy& p = doc.templates[t];
        if(p.id != kTemplates[t].id || p.shapes.GetCount() != (int)__countof(kShapes)) {
            error = "Template or shape catalogue does not match this UiGraph matrix version.";
            return false;
        }
        for(int s = 0; s < p.shapes.GetCount(); s++)
            if(p.shapes[s].shape != kShapeIds[s] || p.shapes[s].lod.GetCount() != 4) {
                error = "Shape/LOD policy is incomplete.";
                return false;
            }
    }
    return true;
}

UiGraphPort MatrixPort(const String& id, const String& title,
                       UiGraphPortDirection direction, UiGraphPortSide side, int order)
{
    UiGraphPort port;
    port.id = id;
    port.title = title;
    port.direction = direction;
    port.type = UiGraphDataType::Flow;
    port.side = side;
    port.order = order;
    port.multiplicity = UiGraphPortMultiplicity::Multiple;
    return port;
}

Image ShapeIcon(UiGraphNodeShape shape, Color ink)
{
    const int side_px = DPI(22);
    ImageBuffer ib(side_px, side_px);
    ib.SetKind(IMAGE_ALPHA);
    Fill(~ib, RGBAZero(), ib.GetLength());
    BufferPainter p(ib, MODE_ANTIALIASED);
    const double stroke = max(1.0, (double)DPI(1));
    const double l = DPI(3), t = DPI(4), r = side_px - DPI(3), b = side_px - DPI(4);
    const double cx = (l + r) * 0.5, cy = (t + b) * 0.5;
    p.Begin();
    switch(shape) {
    case UiGraphNodeShape::Ellipse:
        p.Ellipse(cx, cy, (r - l) * 0.5, (b - t) * 0.5);
        break;
    case UiGraphNodeShape::Diamond:
        p.Move(cx, t).Line(r, cy).Line(cx, b).Line(l, cy).Close();
        break;
    case UiGraphNodeShape::Triangle:
        p.Move(cx, t).Line(r, b).Line(l, b).Close();
        break;
    case UiGraphNodeShape::Hexagon: {
        double dx = (r - l) * 0.23;
        p.Move(l + dx, t).Line(r - dx, t).Line(r, cy)
         .Line(r - dx, b).Line(l + dx, b).Line(l, cy).Close();
        break;
    }
    case UiGraphNodeShape::Cloud:
        p.Move(l + DPI(2), cy + DPI(3))
         .Cubic(Pointf(l, cy - DPI(1)), Pointf(l + DPI(3), t + DPI(2)), Pointf(l + DPI(7), t + DPI(3)))
         .Cubic(Pointf(l + DPI(9), t - DPI(1)), Pointf(r - DPI(5), t), Pointf(r - DPI(4), t + DPI(4)))
         .Cubic(Pointf(r + DPI(1), t + DPI(4)), Pointf(r + DPI(1), b - DPI(2)), Pointf(r - DPI(3), b - DPI(1)))
         .Line(l + DPI(4), b)
         .Cubic(Pointf(l, b), Pointf(l, cy + DPI(5)), Pointf(l + DPI(2), cy + DPI(3))).Close();
        break;
    case UiGraphNodeShape::Document: {
        double fold = DPI(5);
        p.Move(l, t).Line(r - fold, t).Line(r, t + fold).Line(r, b).Line(l, b).Close();
        p.Move(r - fold, t).Line(r - fold, t + fold).Line(r, t + fold);
        break;
    }
    case UiGraphNodeShape::Database:
        p.Ellipse(cx, t + DPI(3), (r - l) * 0.5, DPI(3));
        p.Move(l, t + DPI(3)).Line(l, b - DPI(3));
        p.Move(r, t + DPI(3)).Line(r, b - DPI(3));
        p.Move(l, b - DPI(3)).Cubic(Pointf(l + DPI(2), b + DPI(1)), Pointf(r - DPI(2), b + DPI(1)), Pointf(r, b - DPI(3)));
        break;
    case UiGraphNodeShape::Rectangle:
    default:
        p.RoundedRectangle(l, t, r - l, b - t, DPI(2));
        break;
    }
    p.Stroke(stroke, ink);
    p.End();
    p.Finish();
    return Image(ib);
}

enum class ChipState : byte { Off, Visible, Suppressed };

class FeatureChip : public Ctrl {
public:
    Event<> WhenAction;

    void SetTag(const String& text) { tag_ = text; Refresh(); }
    void SetState(ChipState state) { state_ = state; Refresh(); }
    ChipState GetState() const { return state_; }

    void Paint(Draw& w) override
    {
        const bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
        Color face, frame, ink;
        if(state_ == ChipState::Visible) {
            face = dark ? Color(22, 101, 52) : Color(220, 252, 231);
            frame = dark ? Color(74, 222, 128) : Color(74, 222, 128);
            ink = dark ? Color(220, 252, 231) : Color(22, 101, 52);
        }
        else if(state_ == ChipState::Suppressed) {
            face = dark ? Color(120, 53, 15) : Color(255, 247, 237);
            frame = dark ? Color(251, 146, 60) : Color(251, 146, 60);
            ink = dark ? Color(255, 237, 213) : Color(154, 52, 18);
        }
        else {
            face = dark ? Color(127, 29, 29) : Color(254, 226, 226);
            frame = dark ? Color(248, 113, 113) : Color(248, 113, 113);
            ink = dark ? Color(254, 226, 226) : Color(185, 28, 28);
        }
        if(hot_)
            face = Blend(face, dark ? White() : Black(), dark ? 24 : 14);
        Rect r = GetSize();
        w.DrawRect(r, face);
        w.DrawRect(r.left, r.top, r.GetWidth(), 1, frame);
        w.DrawRect(r.left, r.bottom - 1, r.GetWidth(), 1, frame);
        w.DrawRect(r.left, r.top, 1, r.GetHeight(), frame);
        w.DrawRect(r.right - 1, r.top, 1, r.GetHeight(), frame);
        Font f = SansSerifZ(max(1, DPI(7))).Bold();
        Size ts = GetTextSize(tag_, f);
        w.DrawText(max(0, (r.GetWidth() - ts.cx) / 2), max(0, (r.GetHeight() - ts.cy) / 2), tag_, f, ink);
    }

    void LeftDown(Point, dword) override { WhenAction(); }
    void MouseMove(Point, dword) override { if(!hot_) { hot_ = true; Refresh(); } }
    void MouseLeave() override { hot_ = false; Refresh(); }
    Image CursorImage(Point, dword) override { return Image::Hand(); }

private:
    String tag_;
    ChipState state_ = ChipState::Off;
    bool hot_ = false;
};

class MatrixCell : public Ctrl {
public:
    Event<int> WhenFeatureToggle;

    MatrixCell()
    {
        Add(meta_);
        Add(graph_);
        meta_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        for(int i = 0; i < 9; i++) {
            chips_.Add().SetTag(kFeatureTags[i]);
            Add(chips_[i]);
            chips_[i].WhenAction = [=] { WhenFeatureToggle(i); };
        }
        graph_.WhenViewport = [=] { Sync(); };
    }

    void Paint(Draw& w) override
    {
        const bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
        Color face = dark ? Color(17, 24, 39) : White();
        Color frame = selected_ ? Color(30, 144, 255) : (dark ? Color(55, 65, 81) : Color(220, 226, 234));
        w.DrawRect(GetSize(), face);
        w.DrawRect(0, 0, GetSize().cx, 1, frame);
        w.DrawRect(0, GetSize().cy - 1, GetSize().cx, 1, frame);
        w.DrawRect(0, 0, 1, GetSize().cy, frame);
        w.DrawRect(GetSize().cx - 1, 0, 1, GetSize().cy, frame);
    }

    void Layout() override
    {
        const int meta_h = DPI(24);
        const int chip_h = DPI(21);
        meta_.SetRect(DPI(6), DPI(2), max(0, GetSize().cx - DPI(12)), meta_h - DPI(2));
        graph_.SetRect(DPI(2), meta_h, max(0, GetSize().cx - DPI(4)), max(0, GetSize().cy - meta_h - chip_h - DPI(4)));
        int y = max(meta_h, GetSize().cy - chip_h - DPI(2));
        int gap = DPI(2);
        int usable = max(0, GetSize().cx - DPI(8) - gap * 8);
        int cw = usable / 9;
        int x = DPI(4);
        for(int i = 0; i < chips_.GetCount(); i++) {
            int w = i == chips_.GetCount() - 1 ? max(0, GetSize().cx - DPI(4) - x) : cw;
            chips_[i].SetRect(x, y, w, chip_h);
            x += w + gap;
        }
        if(main_.IsValid())
            graph_.CenterOnNode(main_);
        Sync();
    }

    void Configure(int shape_index, int row, const FeatureSet& requested,
                   int template_index, Sizef authored, double zoom, int port_preset,
                   bool selected)
    {
        shape_index_ = shape_index;
        row_ = row;
        requested_ = requested;
        selected_ = selected;
        template_index_ = template_index;
        sample_zoom_ = zoom;

        graph_.ClearNodeCtrls();
        graph_.Model().Clear();
        graph_.SetAutoFitOnFirstPaint(false).SetEditable(false).EnableInternalMutation(false);

        const bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
        UiNodeGraph::Style style = UiNodeGraph::StyleDefault();
        style.show_grid = false;
        style.min_zoom = 0.03;
        style.max_zoom = 4.0;
        style.node.show_header_band = false;
        style.node.show_description = requested.description;
        style.node.show_port_labels = requested.port_labels;
        style.node.header_height = DPI(46);
        style.node.metrics.content_margin = Rect(DPI(10), DPI(8), DPI(10), DPI(8));
        style.node.metrics.shadow.enabled = true;
        style.node.metrics.shadow.distance = DPI(2);
        style.node.metrics.shadow.offset_x = DPI(1);
        style.node.metrics.shadow.offset_y = DPI(1);
        style.node.metrics.shadow.alpha = dark ? 42 : 24;
        for(int i = 0; i < 4; i++) {
            style.canvas_palette.face[i] = UiFill::Solid(dark ? Color(15, 23, 42) : Color(248, 250, 252));
            style.node.palette.face[i] = UiFill::Solid(dark ? Color(18, 74, 123) : Color(13, 126, 218));
            style.node.palette.frame[i] = dark ? Color(96, 165, 250) : Color(3, 105, 161);
            style.node.header_face[i] = dark ? Color(15, 23, 42) : Color(239, 246, 255);
            style.node.title_ink[i] = White();
            style.node.subtitle_ink[i] = dark ? Color(191, 219, 254) : Color(219, 234, 254);
            style.node.description_ink[i] = dark ? Color(226, 232, 240) : Color(51, 65, 85);
            style.node.port_frame[i] = dark ? Color(147, 197, 253) : Color(14, 116, 217);
            style.node.port_label_ink[i] = dark ? Color(203, 213, 225) : Color(71, 85, 105);
            style.edge.color[i] = dark ? Color(148, 163, 184) : Color(100, 116, 139);
        }
        graph_.SetCustomStyle(style);

        const TemplateSpec spec = kTemplates[template_index];
        graph_.WhenResolveNodePresentation = [=](const UiGraphNode&, const UiGraphNodeStyle&,
                                                 UiGraphPresentationRequest& r) {
            r.profile = spec.profile;
            r.badge_height = requested.badge ? DPI(14) : 0;
            r.footer_height = requested.footer ? DPI(14) : 0;
            r.media_min_height = requested.media ? DPI(22) : 0;
        };

        UiNodeGraph *owner = &graph_;
        graph_.WhenPaintNodeContent = [owner](Draw& w, const UiGraphNode& node, const Rect& media,
                                              const UiGraphNodeStyle&, UiGraphVisualState) {
            UiGraphNodePresentation p;
            if(!owner->GetNodePresentation(node.ref, p))
                return;
            double z = owner->GetZoom();
            Font small = SansSerifZ(max(1, fround(DPI(8) * z)));
            Font strong = small; strong.Bold();
            const bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
            Color ink = dark ? Color(226, 232, 240) : Color(55, 65, 81);
            if(p.show_badge && !p.badge.IsEmpty()) {
                w.DrawRect(p.badge, dark ? Color(30, 64, 175) : Color(219, 234, 254));
                DrawTextEllipsis(w, p.badge.left + max(1, fround(DPI(3) * z)), p.badge.top,
                                 max(0, p.badge.GetWidth() - max(2, fround(DPI(6) * z))),
                                 "BADGE", "...", strong, ink);
            }
            if(p.show_media && !media.IsEmpty()) {
                w.DrawRect(media, dark ? Color(30, 58, 95) : Color(219, 234, 254));
                int inset = max(1, fround(DPI(3) * z));
                Rect inner = media.Deflated(inset, inset);
                if(inner.GetHeight() >= small.GetCy())
                    DrawTextEllipsis(w, inner.left, inner.top + max(0, (inner.GetHeight() - small.GetCy()) / 2),
                                     inner.GetWidth(), "Media area", "...", small,
                                     dark ? Color(191, 219, 254) : Color(55, 92, 140));
            }
            if(p.show_footer && !p.footer.IsEmpty()) {
                w.DrawRect(p.footer, dark ? Color(31, 41, 55) : Color(241, 245, 249));
                DrawTextEllipsis(w, p.footer.left + max(1, fround(DPI(3) * z)), p.footer.top,
                                 max(0, p.footer.GetWidth() - max(2, fround(DPI(6) * z))),
                                 "status: ready", "...", small,
                                 dark ? Color(203, 213, 225) : Color(71, 85, 105));
            }
        };

        UiGraphNode node;
        node.title = requested.title ? kShapeNames[shape_index] : String();
        node.subtitle = requested.subtitle ? "Subtitle" : String();
        node.description = requested.description ? "One prepared layout owns this presentation." : String();
        node.position = Pointf(0, 0);
        node.size = authored;
        node.shape = kShapes[shape_index];
        node.role = UiGraphNodeRole::Accent;
        node.corner_radius = DPI(10);
        if(requested.icon) {
            node.icon = ICON_DESIGN_WIDGETS_48();
            node.icon_size = Size(DPI(18), DPI(18));
        }

        int inputs = 1, outputs = 1;
        if(port_preset == 0) inputs = outputs = 0;
        else if(port_preset == 2) { inputs = 3; outputs = 2; }
        else if(port_preset == 3) { inputs = 4; outputs = 4; }
        for(int i = 0; i < inputs; i++)
            node.ports.Add(MatrixPort(Format("in%d", i), Format("In %d", i + 1), UiGraphPortDirection::Input, UiGraphPortSide::Left, i));
        for(int i = 0; i < outputs; i++)
            node.ports.Add(MatrixPort(Format("out%d", i), Format("Out %d", i + 1), UiGraphPortDirection::Output, UiGraphPortSide::Right, i));

        main_ = graph_.Model().AddNode(node);
        if(requested.control) {
            child_.SetText(template_index == 5 ? "Gain 1.00" : "Run");
            graph_.SetNodeCtrl(main_, child_);
        }

        const double helper_x = authored.cx * 0.72 + 180.0;
        const double helper_y_span = max(60.0, authored.cy * 0.65);
        auto helper_y = [&](int index, int count) {
            if(count <= 1) return authored.cy * 0.5;
            return authored.cy * 0.5 - helper_y_span * 0.5 + helper_y_span * index / max(1, count - 1);
        };
        for(int i = 0; i < inputs; i++) {
            UiGraphNode helper;
            helper.position = Pointf(-helper_x, helper_y(i, inputs));
            helper.size = Sizef(18, 18);
            helper.shape = UiGraphNodeShape::Ellipse;
            helper.role = UiGraphNodeRole::Subtle;
            helper.selectable = false;
            helper.movable = false;
            helper.ports.Add(MatrixPort("out", "", UiGraphPortDirection::Output, UiGraphPortSide::Right, 0));
            UiGraphNodeRef h = graph_.Model().AddNode(helper);
            UiGraphEdge e;
            e.source = UiGraphPortRef{h, "out"};
            e.target = UiGraphPortRef{main_, Format("in%d", i)};
            e.route = UiGraphRouteStyle::Straight;
            e.arrow = UiGraphArrowStyle::None;
            e.selectable = false;
            graph_.Model().AddEdge(e);
        }
        for(int i = 0; i < outputs; i++) {
            UiGraphNode helper;
            helper.position = Pointf(authored.cx + helper_x, helper_y(i, outputs));
            helper.size = Sizef(18, 18);
            helper.shape = UiGraphNodeShape::Ellipse;
            helper.role = UiGraphNodeRole::Subtle;
            helper.selectable = false;
            helper.movable = false;
            helper.ports.Add(MatrixPort("in", "", UiGraphPortDirection::Input, UiGraphPortSide::Left, 0));
            UiGraphNodeRef h = graph_.Model().AddNode(helper);
            UiGraphEdge e;
            e.source = UiGraphPortRef{main_, Format("out%d", i)};
            e.target = UiGraphPortRef{h, "in"};
            e.route = UiGraphRouteStyle::Straight;
            e.arrow = UiGraphArrowStyle::Open;
            e.selectable = false;
            graph_.Model().AddEdge(e);
        }

        graph_.SetZoom(sample_zoom_);
        graph_.InvalidateNodePresentation();
        Layout();
    }

    void SetSampleZoom(double zoom, const char *expected)
    {
        sample_zoom_ = zoom;
        expected_ = expected;
        graph_.SetZoom(zoom);
        graph_.InvalidateNodePresentation();
        Sync();
    }

    void SetExpected(const char *expected) { expected_ = expected; Sync(); }
    int SuppressedCount() const { return suppressed_count_; }

private:
    void Sync()
    {
        if(!main_.IsValid())
            return;
        UiGraphNodePresentation p;
        if(!graph_.GetNodePresentation(main_, p))
            return;
        const UiGraphNode *n = graph_.Model().FindNode(main_);
        if(!n)
            return;
        meta_.SetText(Format("%.2fx   %d x %d   •   %s%s",
                             graph_.GetZoom(), fround(n->size.cx * graph_.GetZoom()),
                             fround(n->size.cy * graph_.GetZoom()), PresentationLevelName(p.level),
                             p.fits ? "" : "   • capacity"));
        const bool actual[] = {
            p.show_title, p.show_subtitle, p.show_icon, p.show_badge, p.show_media,
            p.show_description, p.show_footer, p.show_port_labels, p.show_control
        };
        suppressed_count_ = 0;
        for(int i = 0; i < 9; i++) {
            bool requested = requested_.Get(i);
            if(!requested)
                chips_[i].SetState(ChipState::Off);
            else if(actual[i])
                chips_[i].SetState(ChipState::Visible);
            else {
                chips_[i].SetState(ChipState::Suppressed);
                suppressed_count_++;
            }
            chips_[i].Tip(String(kFeatureNames[i]) + (requested ? actual[i] ? " — requested and visible" : " — requested but suppressed by current production/capacity" : " — disabled by policy"));
        }
        Refresh();
    }

    UiButton child_;
    UiNodeGraph graph_;
    UiGraphNodeRef main_;
    UiLabel meta_;
    Array<FeatureChip> chips_;
    FeatureSet requested_;
    String expected_;
    int shape_index_ = 0;
    int row_ = 0;
    int template_index_ = 0;
    double sample_zoom_ = 1.0;
    bool selected_ = false;
    int suppressed_count_ = 0;
};

class LodRail : public Ctrl {
public:
    void SetLayout(const Vector<int>& row_y, const Vector<int>& row_h, const ThresholdSet& thresholds)
    {
        row_y_ = clone(row_y);
        row_h_ = clone(row_h);
        thresholds_ = thresholds;
        Refresh();
    }

    void Paint(Draw& w) override
    {
        const bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
        w.DrawRect(GetSize(), dark ? Color(15, 23, 42) : Color(248, 250, 252));
        if(row_y_.GetCount() != 4 || row_h_.GetCount() != 4)
            return;
        const Color faces[] = {
            Color(0, 116, 217), Color(35, 151, 235), Color(104, 189, 239), Color(191, 226, 248)
        };
        const char *desc[] = { "Full detail", "Simplified", "Minimal", "Silhouette" };
        String ranges[] = {
            Format("≥ %.0f%%", thresholds_.normal),
            Format("%.0f–%.0f%%", thresholds_.lod1, thresholds_.normal),
            Format("%.0f–%.0f%%", thresholds_.lod2, thresholds_.lod1),
            Format("≤ %.0f%%", thresholds_.lod2)
        };
        for(int i = 0; i < 4; i++) {
            Rect r = RectC(0, row_y_[i], GetSize().cx, row_h_[i] - DPI(2));
            w.DrawRect(r, faces[i]);
            Color ink = i < 2 || dark ? White() : Color(30, 64, 94);
            Font strong = SansSerifZ(DPI(10)).Bold();
            Font small = SansSerifZ(DPI(8));
            w.DrawText(DPI(10), r.top + DPI(12), kRowNames[i], strong, ink);
            w.DrawText(DPI(10), r.top + DPI(36), ranges[i], small, ink);
            w.DrawText(DPI(10), r.top + DPI(56), desc[i], small, ink);
        }
    }

private:
    Vector<int> row_y_, row_h_;
    ThresholdSet thresholds_;
};

class UiGraphDesignMatrix : public TopWindow {
public:
    UiGraphDesignMatrix()
    {
        Title("UiGraph / production design matrix");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1536), DPI(820));

        UiThemeContext context = UiTheme::GetContext();
        context.preset = UiThemePreset::Minimal;
        context.mode = UiThemeMode::Light;
        UiTheme::Set(context);

        document_ = MakeDefaultDocument();
        BuildShell();
        ConnectEvents();
        ConfigureFromDocument();
    }

    void Layout() override
    {
        Rect r = GetSize();
        const int margin = DPI(10);
        const int gap = DPI(8);
        header_.SetRect(margin, DPI(6), max(0, r.cx - 2 * margin), DPI(46));
        toolbar_.SetRect(margin, DPI(58), max(0, r.cx - 2 * margin), DPI(36));
        threshold_panel_.SetRect(margin, DPI(100), max(0, r.cx - 2 * margin), DPI(98));
        legend_panel_.SetRect(margin, DPI(204), max(0, r.cx - 2 * margin), DPI(30));
        int top = DPI(240);
        int body_h = max(0, r.cy - top - margin);
        lod_view_.SetRect(margin, top, DPI(88), body_h);
        viewport_.SetRect(margin + DPI(88) + gap, top, max(0, r.cx - (margin * 2 + DPI(88) + gap)), body_h);
        horizontal_.SetPage(viewport_.GetSize().cx);
        vertical_.SetPage(viewport_.GetSize().cy);
        LayoutToolbar();
        LayoutThresholdPanel();
        LayoutLegend();
        ArrangeMatrix();
    }

private:
    void BuildShell()
    {
        Add(header_);
        header_.SetTitle("UiGraph / production design matrix")
               .SetSubTitle("Design · Validate · Export")
               .SetMedia(ICON_DESIGN_WIDGETS_48())
               .SetMediaSide(UiAlign::LEFT)
               .SetMediaAlign(UiAlign::CENTER, UiAlign::CENTER)
               .SetMediaAutoFit(true)
               .ShowTitleLine(false)
               .SetContentInset(DPI(6))
               .SetContentCell(header_actions_);
        header_actions_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        header_actions_.AddSpacer(1).Expand(1);
        btn_copy_json_.SetText("Copy JSON").Tip("Copy the complete versioned LOD policy to the clipboard");
        btn_import_.SetText("Import...").Tip("Load a UiGraph LOD policy JSON file");
        btn_export_.SetText("Export...").Tip("Save the complete UiGraph LOD policy as JSON");
        btn_theme_.SetIcon(ICON_ACTION_DARK_MODE_48()).SetIconSize(DPI(16), DPI(16)).Tip("Toggle light/dark theme");
        btn_help_.SetIcon(ICON_DESIGN_HELP_48()).SetIconSize(DPI(16), DPI(16)).Tip("About the matrix editor");
        header_actions_.Add(btn_copy_json_).Fixed(DPI(86));
        header_actions_.Add(btn_import_).Fixed(DPI(76));
        header_actions_.Add(btn_export_).Fixed(DPI(76));
        header_actions_.Add(btn_theme_).Fixed(DPI(34));
        header_actions_.Add(btn_help_).Fixed(DPI(34));

        Add(toolbar_);
        toolbar_.Add(lbl_template_); toolbar_.Add(template_);
        toolbar_.Add(lbl_size_); toolbar_.Add(authored_);
        toolbar_.Add(lbl_ports_); toolbar_.Add(ports_);
        toolbar_.Add(lbl_shape_); toolbar_.Add(selected_shape_);
        toolbar_.Add(btn_reset_cameras_);
        lbl_template_.SetText("Template").SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        lbl_size_.SetText("Authored size").SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        lbl_ports_.SetText("Ports").SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        lbl_shape_.SetText("Editing shape").SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        for(const TemplateSpec& spec : kTemplates)
            template_.Add(spec.name);
        authored_.Add("260 x 170 (capacity stress)").Add("360 x 240 (reference)").Add("480 x 360 (spacious)");
        ports_.Add("No ports").Add("1 input / 1 output").Add("3 inputs / 2 outputs").Add("4 inputs / 4 outputs");
        for(const char *name : kShapeNames)
            selected_shape_.Add(name);
        btn_reset_cameras_.SetText("Reset cameras");

        Add(threshold_panel_);
        threshold_panel_.Add(threshold_title_);
        threshold_panel_.Add(threshold_help_);
        threshold_panel_.Add(range_);
        for(int i = 0; i < 3; i++) threshold_panel_.Add(boundary_labels_[i]);
        threshold_panel_.Add(endpoint_full_); threshold_panel_.Add(endpoint_micro_);
        threshold_panel_.Add(btn_global_); threshold_panel_.Add(btn_shape_override_);
        threshold_panel_.Add(copy_from_); threshold_panel_.Add(btn_copy_thresholds_);
        threshold_panel_.Add(btn_copy_features_); threshold_panel_.Add(btn_copy_all_);
        threshold_panel_.Add(btn_apply_all_); threshold_panel_.Add(btn_reset_shape_);
        threshold_title_.SetText("LOD Threshold Editor").SetFont(SansSerifZ(DPI(10)).Bold());
        threshold_help_.SetText("Drag stops to choose the proposed transition points. Matrix rows sample just below those boundaries.");
        btn_global_.SetText("Global thresholds").SetCheckable().SetChecked(true);
        btn_shape_override_.SetText("Per-shape override").SetCheckable();
        btn_copy_thresholds_.SetText("Thresholds");
        btn_copy_features_.SetText("Features");
        btn_copy_all_.SetText("All");
        btn_apply_all_.SetText("Apply to all");
        btn_reset_shape_.SetText("Reset shape");
        for(const char *name : kShapeNames) copy_from_.Add(name);
        copy_from_.SetIndex(0);
        range_.SetRange(0, 100)
              .SetMinimumSegmentSpan(4.0)
              .SetValueDisplay(UiRangeSegments::ValueDisplay::Percent)
              .SetValuePrecision(0)
              .ShowBoundaryValues(false)
              .ShowEndpointValues(false)
              .ShowValuesOnInteraction(false)
              .ShowLabels(true)
              .ShowDividers(true);
        Vector<Color> blues;
        blues << Color(0, 116, 217) << Color(35, 151, 235) << Color(104, 189, 239) << Color(191, 226, 248);
        range_.SetPalette(blues);
        endpoint_full_.SetText("100% / full size").SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        endpoint_micro_.SetText("0% / micro").SetAlign(UiAlign::RIGHT, UiAlign::CENTER);
        for(int i = 0; i < 3; i++)
            boundary_labels_[i].SetAlign(UiAlign::CENTER, UiAlign::CENTER);

        Add(legend_panel_);
        legend_panel_.Add(legend_left_); legend_panel_.Add(legend_features_); legend_panel_.Add(diagnostics_);
        legend_left_.SetText("LOD scale on left · cell top = camera / projected size / actual state · cell bottom = requested features");
        legend_features_.SetText("Green visible  ·  Red off  ·  Amber requested but suppressed");
        diagnostics_.SetAlign(UiAlign::RIGHT, UiAlign::CENTER);

        Add(lod_view_);
        lod_view_.Add(lod_rail_);
        Add(viewport_);
        viewport_.Add(sheet_);
        viewport_.AddFrame(horizontal_.Horz());
        viewport_.AddFrame(vertical_);
        for(int i = 0; i < 8; i++) {
            UiButton& b = shape_headers_.Add();
            b.SetText(kShapeNames[i]).SetCheckable();
            b.SetIcon(ShapeIcon(kShapes[i], Color(13, 126, 218))).SetIconSize(DPI(18), DPI(18));
            sheet_.Add(b);
            b.WhenAction = [=] { SelectShape(i); };
        }
        for(int i = 0; i < 32; i++) {
            MatrixCell& cell = cells_.Add();
            sheet_.Add(cell);
            int row = i / 8, col = i % 8;
            cell.WhenFeatureToggle = [=](int feature) { ToggleFeature(col, row, feature); };
        }
    }

    void LayoutToolbar()
    {
        int y = DPI(4), h = DPI(28), x = DPI(6);
        lbl_template_.SetRect(x, y, DPI(58), h); x += DPI(62);
        template_.SetRect(x, y, DPI(160), h); x += DPI(170);
        lbl_size_.SetRect(x, y, DPI(88), h); x += DPI(92);
        authored_.SetRect(x, y, DPI(205), h); x += DPI(215);
        lbl_ports_.SetRect(x, y, DPI(42), h); x += DPI(46);
        ports_.SetRect(x, y, DPI(170), h); x += DPI(180);
        lbl_shape_.SetRect(x, y, DPI(88), h); x += DPI(92);
        selected_shape_.SetRect(x, y, DPI(130), h); x += DPI(140);
        btn_reset_cameras_.SetRect(x, y, DPI(118), h);
    }

    void LayoutThresholdPanel()
    {
        Size s = threshold_panel_.GetSize();
        threshold_title_.SetRect(DPI(10), DPI(6), DPI(150), DPI(22));
        threshold_help_.SetRect(DPI(164), DPI(6), max(0, s.cx - DPI(680)), DPI(22));
        int controls_x = max(DPI(760), s.cx - DPI(610));
        btn_global_.SetRect(controls_x, DPI(4), DPI(126), DPI(26));
        btn_shape_override_.SetRect(controls_x + DPI(130), DPI(4), DPI(132), DPI(26));
        copy_from_.SetRect(controls_x + DPI(270), DPI(4), DPI(112), DPI(26));
        btn_copy_thresholds_.SetRect(controls_x + DPI(386), DPI(4), DPI(72), DPI(26));
        btn_copy_features_.SetRect(controls_x + DPI(462), DPI(4), DPI(68), DPI(26));
        btn_copy_all_.SetRect(controls_x + DPI(534), DPI(4), DPI(54), DPI(26));
        btn_apply_all_.SetRect(max(0, s.cx - DPI(222)), DPI(34), DPI(102), DPI(24));
        btn_reset_shape_.SetRect(max(0, s.cx - DPI(116)), DPI(34), DPI(106), DPI(24));
        range_.SetRect(DPI(10), DPI(34), max(0, s.cx - DPI(244)), DPI(54));
        endpoint_full_.SetRect(DPI(14), DPI(74), DPI(120), DPI(18));
        endpoint_micro_.SetRect(max(0, s.cx - DPI(354)), DPI(74), DPI(108), DPI(18));
        UpdateBoundaryLabels();
    }

    void LayoutLegend()
    {
        int w = legend_panel_.GetSize().cx;
        legend_left_.SetRect(DPI(8), DPI(4), max(0, w * 55 / 100), DPI(22));
        legend_features_.SetRect(w * 55 / 100, DPI(4), max(0, w * 27 / 100), DPI(22));
        diagnostics_.SetRect(w * 82 / 100, DPI(4), max(0, w * 18 / 100 - DPI(8)), DPI(22));
    }

    void ConnectEvents()
    {
        template_.WhenAction = [=] {
            document_.active_template = kTemplates[template_.GetIndex()].id;
            ConfigureCells();
            SyncThresholdEditor();
        };
        authored_.WhenAction = [=] {
            document_.authored_size = authored_.GetIndex() == 0 ? "compact" : authored_.GetIndex() == 1 ? "reference" : "spacious";
            ConfigureCells();
        };
        ports_.WhenAction = [=] {
            static const char *ids[] = { "none", "1x1", "3x2", "4x4" };
            document_.port_preset = ids[ports_.GetIndex()];
            ConfigureCells();
        };
        selected_shape_.WhenAction = [=] { SelectShape(selected_shape_.GetIndex()); };
        btn_reset_cameras_.WhenAction = [=] { ConfigureCells(); };
        btn_global_.WhenAction = [=] { SetThresholdScope(false); };
        btn_shape_override_.WhenAction = [=] { SetThresholdScope(true); };
        range_.WhenChanging = [=] { ApplyThresholdsFromControl(); PreviewThresholdChange(); };
        range_.WhenAction = [=] { ApplyThresholdsFromControl(); PreviewThresholdChange(); };
        btn_copy_thresholds_.WhenAction = [=] { CopyFromShape(false, true); };
        btn_copy_features_.WhenAction = [=] { CopyFromShape(true, false); };
        btn_copy_all_.WhenAction = [=] { CopyFromShape(true, true); };
        btn_apply_all_.WhenAction = [=] { ApplySelectedThresholdsToAll(); };
        btn_reset_shape_.WhenAction = [=] { ResetSelectedShape(); };
        horizontal_.WhenScroll = vertical_.WhenScroll = [=] { Scroll(); };
        btn_theme_.WhenAction = [=] { ToggleTheme(); };
        btn_export_.WhenAction = [=] { ExportJson(); };
        btn_import_.WhenAction = [=] { ImportJson(); };
        btn_copy_json_.WhenAction = [=] { WriteClipboardText(StoreAsJson(document_, true)); };
        btn_help_.WhenAction = [=] {
            PromptOK("UiGraph presentation policy editor\n\nDrag the LOD threshold stops, click feature tags in any shape/LOD cell, try the six presentation templates and different port topologies, then export the complete versioned policy as JSON.\n\nGreen = requested and visible. Red = intentionally disabled. Amber = requested but the current production layout/LOD/capacity suppresses it.");
        };
    }

    int ActiveTemplateIndex() const { return FindTemplateIndex(document_.active_template); }
    TemplatePolicy& ActivePolicy() { return document_.templates[ActiveTemplateIndex()]; }
    const TemplatePolicy& ActivePolicy() const { return document_.templates[ActiveTemplateIndex()]; }

    ThresholdSet EffectiveThresholds(int shape) const
    {
        const TemplatePolicy& t = ActivePolicy();
        const ShapePolicy& s = t.shapes[shape];
        return s.threshold_override ? s.thresholds : t.global_thresholds;
    }

    ThresholdSet EditorThresholds() const
    {
        return threshold_shape_mode_ ? EffectiveThresholds(selected_shape_index_) : ActivePolicy().global_thresholds;
    }

    void ConfigureFromDocument()
    {
        template_.SetIndex(ActiveTemplateIndex());
        authored_.SetIndex(document_.authored_size == "spacious" ? 2 : document_.authored_size == "reference" ? 1 : 0);
        ports_.SetIndex(document_.port_preset == "none" ? 0 : document_.port_preset == "3x2" ? 2 : document_.port_preset == "4x4" ? 3 : 1);
        selected_shape_.SetIndex(selected_shape_index_);
        SetThresholdScope(false);
        ConfigureCells();
    }

    Sizef CurrentAuthoredSize() const
    {
        if(authored_.GetIndex() == 2) return Sizef(DPI(480), DPI(360));
        if(authored_.GetIndex() == 1) return Sizef(DPI(360), DPI(240));
        return Sizef(DPI(260), DPI(170));
    }

    double SampleZoom(int shape, int row) const
    {
        ThresholdSet t = EffectiveThresholds(shape);
        if(row == 0) return 1.0;
        if(row == 1) return max(0.04, t.normal / 100.0 - 0.01);
        if(row == 2) return max(0.04, t.lod1 / 100.0 - 0.01);
        return max(0.03, t.lod2 / 100.0 - 0.01);
    }

    void ConfigureCells()
    {
        const int ti = ActiveTemplateIndex();
        const int port_index = ports_.GetIndex();
        Sizef authored = CurrentAuthoredSize();
        for(int row = 0; row < 4; row++)
            for(int col = 0; col < 8; col++)
                cells_[row * 8 + col].Configure(col, row, ActivePolicy().shapes[col].lod[row], ti,
                                                authored, SampleZoom(col, row), port_index,
                                                col == selected_shape_index_);
        ArrangeMatrix();
        UpdateDiagnostics();
    }

    void PreviewThresholdChange()
    {
        for(int row = 0; row < 4; row++)
            for(int col = 0; col < 8; col++)
                cells_[row * 8 + col].SetSampleZoom(SampleZoom(col, row), kRowNames[row]);
        ArrangeMatrix();
        UpdateDiagnostics();
    }

    void ArrangeMatrix()
    {
        if(cells_.GetCount() != 32 || shape_headers_.GetCount() != 8)
            return;
        Sizef authored = CurrentAuthoredSize();
        column_width_ = max(DPI(238), fround(min(authored.cx + DPI(24), (double)DPI(330))));
        const int header_h = DPI(40);
        Vector<int> row_y, row_h;
        int y = header_h;
        for(int row = 0; row < 4; row++) {
            int projected = 0;
            for(int col = 0; col < 8; col++)
                projected = max(projected, fround(authored.cy * SampleZoom(col, row)));
            int h = max(DPI(82), projected + DPI(52));
            row_y.Add(y);
            row_h.Add(h);
            for(int col = 0; col < 8; col++)
                cells_[row * 8 + col].SetRect(col * column_width_, y, column_width_ - DPI(4), h - DPI(2));
            y += h;
        }
        sheet_height_ = y;
        for(int col = 0; col < 8; col++)
            shape_headers_[col].SetRect(col * column_width_, 0, column_width_ - DPI(4), header_h - DPI(2));
        sheet_.SetRect(-horizontal_.Get(), -vertical_.Get(), column_width_ * 8, sheet_height_);
        horizontal_.SetTotal(column_width_ * 8);
        vertical_.SetTotal(sheet_height_);
        lod_rail_.SetRect(0, -vertical_.Get(), lod_view_.GetSize().cx, sheet_height_);
        lod_rail_.SetLayout(row_y, row_h, EditorThresholds());
        Scroll();
    }

    void Scroll()
    {
        sheet_.SetRect(-horizontal_.Get(), -vertical_.Get(), column_width_ * 8, sheet_height_);
        lod_rail_.SetRect(0, -vertical_.Get(), lod_view_.GetSize().cx, sheet_height_);
    }

    void SelectShape(int shape)
    {
        selected_shape_index_ = minmax(shape, 0, 7);
        selected_shape_.SetIndex(selected_shape_index_);
        copy_from_.SetIndex(selected_shape_index_);
        for(int i = 0; i < shape_headers_.GetCount(); i++)
            shape_headers_[i].SetChecked(i == selected_shape_index_);
        SyncThresholdEditor();
        ConfigureCells();
    }

    void SetThresholdScope(bool per_shape)
    {
        threshold_shape_mode_ = per_shape;
        btn_global_.SetChecked(!per_shape);
        btn_shape_override_.SetChecked(per_shape);
        if(per_shape) {
            ShapePolicy& s = ActivePolicy().shapes[selected_shape_index_];
            if(!s.threshold_override) {
                s.thresholds = ActivePolicy().global_thresholds;
                s.threshold_override = true;
            }
        }
        SyncThresholdEditor();
    }

    void SyncThresholdEditor()
    {
        ThresholdSet t = EditorThresholds();
        Vector<UiRangeSegment> seg;
        seg.Add(UiRangeSegment(100.0 - t.normal, "Normal"));
        seg.Add(UiRangeSegment(t.normal - t.lod1, "LOD 1"));
        seg.Add(UiRangeSegment(t.lod1 - t.lod2, "LOD 2"));
        seg.Add(UiRangeSegment(t.lod2, "LOD 3"));
        syncing_range_ = true;
        range_.SetSegments(seg);
        syncing_range_ = false;
        UpdateBoundaryLabels();
        ArrangeMatrix();
    }

    void ApplyThresholdsFromControl()
    {
        if(syncing_range_)
            return;
        Vector<double> b = range_.GetBoundaryValues();
        if(b.GetCount() != 3)
            return;
        ThresholdSet t;
        t.normal = 100.0 - b[0];
        t.lod1 = 100.0 - b[1];
        t.lod2 = 100.0 - b[2];
        t.Normalize();
        if(threshold_shape_mode_) {
            ShapePolicy& s = ActivePolicy().shapes[selected_shape_index_];
            s.threshold_override = true;
            s.thresholds = t;
        }
        else
            ActivePolicy().global_thresholds = t;
        UpdateBoundaryLabels();
    }

    void UpdateBoundaryLabels()
    {
        if(range_.GetSize().cx <= 0)
            return;
        ThresholdSet t = EditorThresholds();
        const double values[] = { t.normal, t.lod1, t.lod2 };
        UiRangeSegments::Geometry g = range_.GetGeometry(range_.GetSize());
        for(int i = 0; i < 3; i++) {
            boundary_labels_[i].SetText(Format("%.0f%%", values[i]));
            if(i < g.boundaries.GetCount()) {
                int x = range_.GetRect().left + g.boundaries[i].x - DPI(20);
                boundary_labels_[i].SetRect(x, DPI(28), DPI(40), DPI(18));
            }
        }
    }

    void ToggleFeature(int shape, int row, int feature)
    {
        FeatureSet& f = ActivePolicy().shapes[shape].lod[row];
        f.Set(feature, !f.Get(feature));
        int port_index = ports_.GetIndex();
        cells_[row * 8 + shape].Configure(shape, row, f, ActiveTemplateIndex(), CurrentAuthoredSize(),
                                          SampleZoom(shape, row), port_index, shape == selected_shape_index_);
        UpdateDiagnostics();
    }

    void CopyFromShape(bool features, bool thresholds)
    {
        int source = copy_from_.GetIndex();
        if(source < 0 || source >= 8 || source == selected_shape_index_)
            return;
        ShapePolicy& dst = ActivePolicy().shapes[selected_shape_index_];
        const ShapePolicy& src = ActivePolicy().shapes[source];
        if(features)
            dst.lod = clone(src.lod);
        if(thresholds) {
            dst.threshold_override = true;
            dst.thresholds = EffectiveThresholds(source);
            threshold_shape_mode_ = true;
        }
        SyncThresholdEditor();
        ConfigureCells();
    }

    void ApplySelectedThresholdsToAll()
    {
        ThresholdSet t = EditorThresholds();
        TemplatePolicy& p = ActivePolicy();
        if(threshold_shape_mode_) {
            for(ShapePolicy& s : p.shapes) {
                s.threshold_override = true;
                s.thresholds = t;
            }
        }
        else {
            p.global_thresholds = t;
            for(ShapePolicy& s : p.shapes)
                s.threshold_override = false;
        }
        ConfigureCells();
        SyncThresholdEditor();
    }

    void ResetSelectedShape()
    {
        int ti = ActiveTemplateIndex();
        ShapePolicy reset = MakeShapePolicy(ti, selected_shape_index_);
        ActivePolicy().shapes[selected_shape_index_] = reset;
        threshold_shape_mode_ = false;
        btn_global_.SetChecked(true);
        btn_shape_override_.SetChecked(false);
        SyncThresholdEditor();
        ConfigureCells();
    }

    void UpdateDiagnostics()
    {
        int suppressed = 0;
        for(const MatrixCell& c : cells_)
            suppressed += c.SuppressedCount();
        diagnostics_.SetText(suppressed ? Format("%d requested feature%s suppressed", suppressed, suppressed == 1 ? "" : "s")
                                        : "All requested features visible at samples");
    }

    void ToggleTheme()
    {
        UiThemeContext c = UiTheme::GetContext();
        c.mode = c.mode == UiThemeMode::Dark ? UiThemeMode::Light : UiThemeMode::Dark;
        UiTheme::Set(c);
        ConfigureCells();
        RefreshLayout();
        Refresh();
    }

    void ExportJson()
    {
        FileSel fs;
        fs.Type("UiGraph LOD policy", "*.json");
        fs.DefaultName("uigraph_lod_policy.json");
        if(!fs.ExecuteSaveAs("Export UiGraph LOD policy"))
            return;
        String json = StoreAsJson(document_, true);
        if(!SaveFile(~fs, json))
            Exclamation("Unable to save the policy file.");
    }

    void ImportJson()
    {
        FileSel fs;
        fs.Type("UiGraph LOD policy", "*.json");
        if(!fs.ExecuteOpen("Import UiGraph LOD policy"))
            return;
        String json = LoadFile(~fs);
        MatrixDocument loaded;
        if(!LoadFromJson(loaded, json)) {
            Exclamation("The selected file is not valid UiGraph policy JSON.");
            return;
        }
        String error;
        if(!ValidateDocument(loaded, error)) {
            Exclamation(error);
            return;
        }
        document_ = pick(loaded);
        selected_shape_index_ = 0;
        ConfigureFromDocument();
    }

private:
    MatrixDocument document_;
    int selected_shape_index_ = 0;
    bool threshold_shape_mode_ = false;
    bool syncing_range_ = false;
    int column_width_ = 0;
    int sheet_height_ = 0;

    UiTitleCard header_;
    UiBoxLayout header_actions_ { UiDirection::H };
    UiButton btn_copy_json_, btn_import_, btn_export_;
    UiToolButton btn_theme_, btn_help_;

    UiPanel toolbar_;
    UiLabel lbl_template_, lbl_size_, lbl_ports_, lbl_shape_;
    DropList template_, authored_, ports_, selected_shape_;
    UiButton btn_reset_cameras_;

    UiPanel threshold_panel_;
    UiLabel threshold_title_, threshold_help_;
    UiRangeSegments range_;
    UiLabel boundary_labels_[3], endpoint_full_, endpoint_micro_;
    UiButton btn_global_, btn_shape_override_;
    DropList copy_from_;
    UiButton btn_copy_thresholds_, btn_copy_features_, btn_copy_all_, btn_apply_all_, btn_reset_shape_;

    UiPanel legend_panel_;
    UiLabel legend_left_, legend_features_, diagnostics_;

    Ctrl lod_view_, viewport_, sheet_;
    LodRail lod_rail_;
    ScrollBar horizontal_, vertical_;
    Array<UiButton> shape_headers_;
    Array<MatrixCell> cells_;
};

} // namespace

GUI_APP_MAIN
{
    UiGraphDesignMatrix().Run();
}
