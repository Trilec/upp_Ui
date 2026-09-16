#include "PresentationStudioCell.h"

namespace Upp {
namespace {

UiGraphPort StudioMatrixPort(const String& id, const String& title,
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

void DrawFrame(Draw& w, Rect r, Color color, int width = 1)
{
    if(r.IsEmpty() || width <= 0)
        return;
    width = min(width, min(r.GetWidth(), r.GetHeight()) / 2);
    w.DrawRect(r.left, r.top, r.GetWidth(), width, color);
    w.DrawRect(r.left, r.bottom - width, r.GetWidth(), width, color);
    w.DrawRect(r.left, r.top + width, width, max(0, r.GetHeight() - 2 * width), color);
    w.DrawRect(r.right - width, r.top + width, width, max(0, r.GetHeight() - 2 * width), color);
}

void DrawPill(Draw& w, int& x, int y, const String& text, Color ink, Color frame, Color face)
{
    Font font = SansSerifZ(DPI(7)).Bold();
    Size ts = GetTextSize(text, font);
    int width = ts.cx + DPI(10);
    Rect r = RectC(x, y, width, DPI(18));
    w.DrawRect(r, face);
    DrawFrame(w, r, frame);
    w.DrawText(r.left + DPI(5), r.top + max(0, (r.GetHeight() - ts.cy) / 2), text, font, ink);
    x += width + DPI(4);
}

bool IsSecondaryLevel(UiGraphPresentationLevel level)
{
    return level == UiGraphPresentationLevel::Normal || level == UiGraphPresentationLevel::Lod1;
}

bool IsFullLevel(UiGraphPresentationLevel level)
{
    return level == UiGraphPresentationLevel::Normal;
}

} // namespace

Image StudioShapeIcon(UiGraphNodeShape shape, Color ink)
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

void StudioFeatureChip::Paint(Draw& w)
{
    const bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
    Color face, frame, ink;
    if(state_ == StudioChipState::Visible) {
        face = dark ? Color(25, 52, 38) : Color(233, 247, 238);
        frame = dark ? Color(53, 111, 73) : Color(159, 210, 174);
        ink = dark ? Color(114, 214, 141) : Color(24, 119, 58);
    }
    else if(state_ == StudioChipState::Suppressed) {
        face = dark ? Color(62, 50, 28) : Color(255, 245, 220);
        frame = dark ? Color(112, 90, 40) : Color(233, 202, 124);
        ink = dark ? Color(255, 210, 120) : Color(147, 96, 0);
    }
    else {
        face = dark ? Color(60, 34, 35) : Color(255, 240, 238);
        frame = dark ? Color(115, 64, 63) : Color(239, 182, 176);
        ink = dark ? Color(255, 141, 133) : Color(180, 35, 24);
    }
    if(hot_)
        face = Blend(face, dark ? White() : Black(), dark ? 22 : 12);
    Rect r = GetSize();
    w.DrawRect(r, face);
    DrawFrame(w, r, frame);
    Font font = SansSerifZ(max(1, DPI(5))).Bold();
    Size ts = GetTextSize(tag_, font);
    w.DrawText(max(0, (r.GetWidth() - ts.cx) / 2), max(0, (r.GetHeight() - ts.cy) / 2), tag_, font, ink);
}

void StudioFeatureChip::MouseMove(Point, dword)
{
    if(!hot_) {
        hot_ = true;
        Refresh();
    }
}

void StudioFeatureChip::MouseLeave()
{
    hot_ = false;
    Refresh();
}

StudioMatrixCell::StudioMatrixCell()
{
    Add(graph_);
    for(int i = 0; i < STUDIO_FEATURE_COUNT; i++) {
        chips_.Add().SetTag(kStudioFeatures[i].tag);
        Add(chips_[i]);
        chips_[i].WhenAction = [=] { WhenFeatureToggle(i); };
    }
    graph_.WhenViewport = [=] { Sync(); };
}

void StudioMatrixCell::Paint(Draw& w)
{
    const bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
    Color face = dark ? Color(29, 37, 48) : White();
    Color frame = selected_ ? Color(12, 127, 211) : (dark ? Color(52, 64, 78) : Color(216, 224, 233));
    w.DrawRect(GetSize(), face);
    DrawFrame(w, Rect(GetSize()), frame, selected_ ? DPI(2) : 1);

    const Color meta_face = dark ? Color(23, 30, 39) : Color(248, 250, 252);
    const Color meta_frame = dark ? Color(52, 64, 78) : Color(216, 224, 233);
    const Color meta_ink = dark ? Color(151, 166, 183) : Color(99, 114, 133);
    const Color level_ink = dark ? Color(82, 169, 229) : Color(12, 127, 211);
    int x = DPI(7);
    const int y = DPI(3);
    DrawPill(w, x, y, Format("%dpx", resolution_px_), meta_ink, meta_frame, meta_face);
    DrawPill(w, x, y, Format("%dx%d", resolution_px_, projected_h_), meta_ink, meta_frame, meta_face);
    String level = StudioPresentationLevelName(actual_level_);
    if(!fits_)
        level << " · capacity";
    DrawPill(w, x, y, level, level_ink,
             Blend(level_ink, meta_frame, 90),
             Blend(meta_face, level_ink, dark ? 28 : 12));
}

void StudioMatrixCell::Layout()
{
    const int meta_h = DPI(24);
    const int chip_h = DPI(17);
    const int gap = DPI(1);
    const int margin = DPI(4);
    int chips_y = max(meta_h, GetSize().cy - chip_h - DPI(3));
    graph_.SetRect(DPI(4), meta_h, max(0, GetSize().cx - DPI(8)),
                   max(0, chips_y - meta_h - DPI(4)));

    int usable = max(0, GetSize().cx - margin * 2 - gap * (STUDIO_FEATURE_COUNT - 1));
    int base = STUDIO_FEATURE_COUNT ? usable / STUDIO_FEATURE_COUNT : 0;
    int remainder = STUDIO_FEATURE_COUNT ? usable % STUDIO_FEATURE_COUNT : 0;
    int x = margin;
    for(int i = 0; i < chips_.GetCount(); i++) {
        int width = base + (i < remainder ? 1 : 0);
        chips_[i].SetRect(x, chips_y, width, chip_h);
        x += width + gap;
    }
    if(main_.IsValid())
        graph_.CenterOnNode(main_);
    Sync();
}

void StudioMatrixCell::ConfigureGraphStyle(const StudioTemplateSpec& spec)
{
    const bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
    const bool minimal = template_index_ == 0;
    UiNodeGraph::Style style = UiNodeGraph::StyleDefault();
    style.show_grid = false;
    style.min_zoom = 0.03;
    style.max_zoom = 4.0;
    style.node.show_header_band = spec.header_band;
    style.node.show_icon = requested_.Get(StudioFeature::Icon);
    style.node.show_description = requested_.Get(StudioFeature::Description);
    style.node.show_port_labels = requested_.Get(StudioFeature::PortLabels);
    style.node.header_height = template_index_ >= 5 ? DPI(44) : DPI(40);
    style.node.metrics.content_margin = Rect(DPI(10), DPI(8), DPI(10), DPI(8));
    style.node.metrics.shadow.enabled = !minimal;
    style.node.metrics.shadow.distance = DPI(2);
    style.node.metrics.shadow.offset_x = DPI(1);
    style.node.metrics.shadow.offset_y = DPI(1);
    style.node.metrics.shadow.alpha = dark ? 38 : 22;
    style.node.content_cell_reserve = template_index_ >= 5 ? DPI(30) : DPI(26);
    style.node.content_cell_min_zoom = 0.52;

    const Color canvas = dark ? Color(23, 30, 39) : Color(248, 250, 252);
    const Color face = minimal ? canvas : (dark ? DkColor(spec.accent, 18) : spec.accent);
    const Color frame = dark ? LtColor(spec.accent, 18) : DkColor(spec.accent, 16);
    const Color title_ink = minimal ? spec.accent : White();
    const Color sub_ink = minimal ? DkColor(spec.accent, 10) : Color(224, 236, 248);
    const Color body_ink = minimal ? (dark ? Color(214, 222, 232) : Color(54, 70, 88))
                                   : (dark ? Color(230, 237, 245) : White());
    const Color header = dark ? DkColor(spec.accent, 38) : DkColor(spec.accent, 30);

    for(int i = 0; i < 4; i++) {
        style.canvas_palette.face[i] = UiFill::Solid(canvas);
        style.node.palette.face[i] = UiFill::Solid(face);
        style.node.palette.frame[i] = frame;
        style.node.header_face[i] = header;
        style.node.title_ink[i] = title_ink;
        style.node.subtitle_ink[i] = sub_ink;
        style.node.description_ink[i] = body_ink;
        style.node.port_frame[i] = minimal ? spec.accent : LtColor(spec.accent, dark ? 35 : 18);
        style.node.port_label_ink[i] = dark ? Color(203, 213, 225) : Color(71, 85, 105);
        style.edge.color[i] = dark ? Color(148, 163, 184) : Color(100, 116, 139);
    }
    graph_.SetCustomStyle(style);
}

void StudioMatrixCell::BuildGraph(Sizef authored, int port_preset)
{
    graph_.ClearNodeCtrls();
    graph_.Model().Clear();
    graph_.SetAutoFitOnFirstPaint(false).SetEditable(false).EnableInternalMutation(false);

    const StudioTemplateSpec& spec = kStudioTemplates[template_index_];
    ConfigureGraphStyle(spec);

    UiGraphNode node;
    node.title = requested_.Get(StudioFeature::Title) ? String(kStudioShapeNames[shape_index_]) : String();
    node.subtitle = requested_.Get(StudioFeature::Subtitle)
                  ? (template_index_ == 3 ? "Process agent" : template_index_ == 4 ? "Content asset" : template_index_ == 6 ? "Graph operator" : "UiGraph node")
                  : String();
    node.description = requested_.Get(StudioFeature::Description)
                     ? (template_index_ == 4 ? "Harbour wide shot · selected take" : "Concise context without opening the inspector.")
                     : String();
    node.position = Pointf(0, 0);
    node.size = authored;
    node.shape = kStudioShapes[shape_index_];
    node.role = UiGraphNodeRole::Accent;
    node.corner_radius = DPI(8);
    if(requested_.Get(StudioFeature::Icon)) {
        node.icon = ICON_DESIGN_WIDGETS_48();
        node.icon_size = Size(DPI(18), DPI(18));
    }

    port_inputs_ = 1;
    port_outputs_ = 1;
    if(port_preset == 0)
        port_inputs_ = port_outputs_ = 0;
    else if(port_preset == 2) {
        port_inputs_ = 3;
        port_outputs_ = 2;
    }
    else if(port_preset == 3) {
        port_inputs_ = 4;
        port_outputs_ = 4;
    }

    for(int i = 0; i < port_inputs_; i++)
        node.ports.Add(StudioMatrixPort(Format("in%d", i), i ? Format("I%d", i + 1) : "In",
                                        UiGraphPortDirection::Input, UiGraphPortSide::Left, i));
    for(int i = 0; i < port_outputs_; i++)
        node.ports.Add(StudioMatrixPort(Format("out%d", i), i ? Format("O%d", i + 1) : "Out",
                                        UiGraphPortDirection::Output, UiGraphPortSide::Right, i));

    main_ = graph_.Model().AddNode(node);
    if(requested_.Get(StudioFeature::Controls)) {
        child_.SetText(template_index_ == 5 ? "Gain 1.00" : template_index_ == 6 ? "Run" : "Control");
        graph_.SetNodeCtrl(main_, child_);
    }

    graph_.WhenResolveNodePresentation = [=](const UiGraphNode& candidate, const UiGraphNodeStyle&,
                                             UiGraphPresentationRequest& request) {
        if(candidate.ref != main_)
            return;
        request.profile = spec.profile;
        request.badge_height = requested_.Get(StudioFeature::Badge) ? DPI(14) : 0;
        request.footer_height = requested_.Get(StudioFeature::Footer) ? DPI(14) : 0;
        int content_min = 0;
        if(requested_.Get(StudioFeature::Media))      content_min = max(content_min, DPI(34));
        if(requested_.Get(StudioFeature::Fields))     content_min = max(content_min, DPI(38));
        if(requested_.Get(StudioFeature::Status))     content_min = max(content_min, DPI(18));
        if(requested_.Get(StudioFeature::Progress))   content_min = max(content_min, DPI(24));
        if(requested_.Get(StudioFeature::Actions))    content_min = max(content_min, DPI(18));
        if(requested_.Get(StudioFeature::PortSummary))content_min = max(content_min, DPI(14));
        request.media_min_height = content_min;
    };

    graph_.WhenPaintNodeContent = [=](Draw& w, const UiGraphNode& candidate, const Rect& media,
                                      const UiGraphNodeStyle&, UiGraphVisualState) {
        if(candidate.ref != main_)
            return;
        UiGraphNodePresentation p;
        if(!graph_.GetNodePresentation(candidate.ref, p) || p.level == UiGraphPresentationLevel::Lod3)
            return;

        const bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
        const Color ink = dark ? Color(230, 237, 245) : White();
        const Color subtle = dark ? Color(195, 208, 222) : Color(226, 236, 247);
        const Color panel = dark ? Color(255, 255, 255) : White();
        const int alpha_mix = dark ? 32 : 44;
        Font small = SansSerifZ(max(1, fround(DPI(7) * graph_.GetZoom())));
        Font strong = small; strong.Bold();

        if(p.show_badge && requested_.Get(StudioFeature::Badge) && !p.badge.IsEmpty()) {
            Color badge_face = Blend(kStudioTemplates[template_index_].accent, White(), 205);
            w.DrawRect(p.badge, badge_face);
            DrawTextEllipsis(w, p.badge.left + 2, p.badge.top,
                             max(0, p.badge.GetWidth() - 4),
                             template_index_ == 3 ? "RUN" : template_index_ == 4 ? "VFX" : "TYPE",
                             "...", strong, DkColor(kStudioTemplates[template_index_].accent, 18));
        }

        Rect area = media;
        if(area.IsEmpty())
            return;
        int pad = max(1, fround(DPI(4) * graph_.GetZoom()));
        area.Deflate(pad, pad);
        int y = area.top;
        int line_h = max(1, small.GetCy());

        if(requested_.Get(StudioFeature::Media) && p.show_media) {
            w.DrawRect(area, Blend(kStudioTemplates[template_index_].accent, panel, 170));
            Rect inner = area.Deflated(max(1, pad));
            if(inner.GetHeight() >= line_h)
                DrawTextEllipsis(w, inner.left, inner.top + max(0, (inner.GetHeight() - line_h) / 2),
                                 inner.GetWidth(), "MEDIA", "...", strong,
                                 DkColor(kStudioTemplates[template_index_].accent, 12));
        }
        else {
            if(requested_.Get(StudioFeature::Status) && p.show_media && y + line_h <= area.bottom) {
                w.DrawRect(area.left, y + max(1, line_h / 3), max(2, line_h / 3), max(2, line_h / 3), Color(157, 240, 178));
                w.DrawText(area.left + line_h, y, "Active · healthy", strong, ink);
                y += line_h + pad;
            }
            if(requested_.Get(StudioFeature::Progress) && p.show_media && IsSecondaryLevel(p.level) && y + max(4, pad * 2) <= area.bottom) {
                int h = max(3, fround(DPI(5) * graph_.GetZoom()));
                Rect bar(area.left, y, area.right, min(area.bottom, y + h));
                w.DrawRect(bar, Blend(kStudioTemplates[template_index_].accent, panel, alpha_mix));
                Rect fill = bar;
                fill.right = fill.left + fill.GetWidth() * 72 / 100;
                w.DrawRect(fill, panel);
                y += h + pad;
            }
            if(requested_.Get(StudioFeature::Fields) && p.show_media && IsSecondaryLevel(p.level)) {
                const char *keys[] = { "Frequency", "Amplitude", "Seed" };
                const char *vals[] = { "1.25", "0.80", "42" };
                int rows = p.level == UiGraphPresentationLevel::Normal ? 3 : 1;
                for(int i = 0; i < rows && y + line_h <= area.bottom; i++) {
                    w.DrawText(area.left, y, keys[i], small, subtle);
                    Size vs = GetTextSize(vals[i], strong);
                    w.DrawText(max(area.left, area.right - vs.cx), y, vals[i], strong, ink);
                    y += line_h + 1;
                }
                y += pad;
            }
            if(requested_.Get(StudioFeature::Actions) && p.show_media && IsFullLevel(p.level) && y + line_h <= area.bottom) {
                int bw = max(DPI(22), area.GetWidth() / 3);
                Rect a = RectC(area.left, y, min(bw, area.GetWidth()), line_h + 2);
                w.DrawRect(a, Blend(kStudioTemplates[template_index_].accent, White(), 185));
                w.DrawText(a.left + 2, a.top, "Preview", small, DkColor(kStudioTemplates[template_index_].accent, 20));
                y += a.GetHeight() + pad;
            }
            if(requested_.Get(StudioFeature::PortSummary) && p.show_media &&
               (port_inputs_ || port_outputs_) && y + line_h <= area.bottom) {
                String summary = Format("%d IN · %d OUT", port_inputs_, port_outputs_);
                DrawTextEllipsis(w, area.left, max(y, area.bottom - line_h), area.GetWidth(),
                                 summary, "...", strong, ink);
            }
        }

        if(p.show_footer && requested_.Get(StudioFeature::Footer) && !p.footer.IsEmpty()) {
            w.DrawRect(p.footer, dark ? Color(31, 41, 55) : Blend(kStudioTemplates[template_index_].accent, White(), 218));
            DrawTextEllipsis(w, p.footer.left + 2, p.footer.top, max(0, p.footer.GetWidth() - 4),
                             template_index_ == 4 ? "v012 · Ready" : "Ready",
                             "...", small, dark ? Color(203, 213, 225) : DkColor(kStudioTemplates[template_index_].accent, 24));
        }
    };

    const double helper_x = authored.cx * 0.72 + 180.0;
    const double helper_y_span = max(60.0, authored.cy * 0.65);
    auto helper_y = [&](int index, int count) {
        if(count <= 1)
            return authored.cy * 0.5;
        return authored.cy * 0.5 - helper_y_span * 0.5
             + helper_y_span * index / max(1, count - 1);
    };

    for(int i = 0; i < port_inputs_; i++) {
        UiGraphNode helper;
        helper.position = Pointf(-helper_x, helper_y(i, port_inputs_));
        helper.size = Sizef(18, 18);
        helper.shape = UiGraphNodeShape::Ellipse;
        helper.role = UiGraphNodeRole::Subtle;
        helper.selectable = false;
        helper.movable = false;
        helper.ports.Add(StudioMatrixPort("out", "", UiGraphPortDirection::Output, UiGraphPortSide::Right, 0));
        UiGraphNodeRef ref = graph_.Model().AddNode(helper);
        UiGraphEdge edge;
        edge.source = UiGraphPortRef{ref, "out"};
        edge.target = UiGraphPortRef{main_, Format("in%d", i)};
        edge.route = UiGraphRouteStyle::Straight;
        edge.arrow = UiGraphArrowStyle::None;
        edge.selectable = false;
        graph_.Model().AddEdge(edge);
    }

    for(int i = 0; i < port_outputs_; i++) {
        UiGraphNode helper;
        helper.position = Pointf(authored.cx + helper_x, helper_y(i, port_outputs_));
        helper.size = Sizef(18, 18);
        helper.shape = UiGraphNodeShape::Ellipse;
        helper.role = UiGraphNodeRole::Subtle;
        helper.selectable = false;
        helper.movable = false;
        helper.ports.Add(StudioMatrixPort("in", "", UiGraphPortDirection::Input, UiGraphPortSide::Left, 0));
        UiGraphNodeRef ref = graph_.Model().AddNode(helper);
        UiGraphEdge edge;
        edge.source = UiGraphPortRef{main_, Format("out%d", i)};
        edge.target = UiGraphPortRef{ref, "in"};
        edge.route = UiGraphRouteStyle::Straight;
        edge.arrow = UiGraphArrowStyle::Open;
        edge.selectable = false;
        graph_.Model().AddEdge(edge);
    }
}

void StudioMatrixCell::Configure(int shape_index, int lod_index, const StudioFeatureSet& requested,
                                 int template_index, Sizef authored, int resolution_px,
                                 int port_preset, bool selected)
{
    shape_index_ = minmax(shape_index, 0, STUDIO_SHAPE_COUNT - 1);
    lod_index_ = minmax(lod_index, 0, STUDIO_LOD_COUNT - 1);
    template_index_ = minmax(template_index, 0, STUDIO_TEMPLATE_COUNT - 1);
    requested_ = requested;
    selected_ = selected;
    resolution_px_ = max(1, resolution_px);
    sample_zoom_ = minmax((double)resolution_px_ / max(1.0, authored.cx), 0.03, 4.0);
    projected_h_ = max(1, fround(authored.cy * sample_zoom_));

    BuildGraph(authored, port_preset);
    graph_.SetZoom(sample_zoom_);
    graph_.InvalidateNodePresentation();
    Layout();
}

void StudioMatrixCell::SetSampleResolution(int resolution_px, Sizef authored)
{
    resolution_px_ = max(1, resolution_px);
    sample_zoom_ = minmax((double)resolution_px_ / max(1.0, authored.cx), 0.03, 4.0);
    projected_h_ = max(1, fround(authored.cy * sample_zoom_));
    graph_.SetZoom(sample_zoom_);
    graph_.InvalidateNodePresentation();
    Sync();
}

void StudioMatrixCell::Sync()
{
    if(!main_.IsValid())
        return;
    UiGraphNodePresentation p;
    if(!graph_.GetNodePresentation(main_, p))
        return;

    actual_level_ = p.level;
    fits_ = p.fits;
    bool actual[STUDIO_FEATURE_COUNT] = {};
    actual[(int)StudioFeature::Title] = p.show_title;
    actual[(int)StudioFeature::Subtitle] = p.show_subtitle;
    actual[(int)StudioFeature::Icon] = p.show_icon;
    actual[(int)StudioFeature::Badge] = p.show_badge;
    actual[(int)StudioFeature::Status] = p.show_media && p.level != UiGraphPresentationLevel::Lod3;
    actual[(int)StudioFeature::Progress] = p.show_media && IsSecondaryLevel(p.level);
    actual[(int)StudioFeature::Description] = p.show_description;
    actual[(int)StudioFeature::Media] = p.show_media;
    actual[(int)StudioFeature::Fields] = p.show_media && IsSecondaryLevel(p.level);
    actual[(int)StudioFeature::Controls] = p.show_control;
    actual[(int)StudioFeature::Actions] = p.show_media && IsFullLevel(p.level);
    actual[(int)StudioFeature::PortLabels] = p.show_port_labels;
    actual[(int)StudioFeature::PortSummary] = p.show_media && p.level != UiGraphPresentationLevel::Lod3 && (port_inputs_ || port_outputs_);
    actual[(int)StudioFeature::Footer] = p.show_footer;

    suppressed_count_ = 0;
    for(int i = 0; i < STUDIO_FEATURE_COUNT; i++) {
        bool requested = requested_.Get(i);
        if(!requested)
            chips_[i].SetState(StudioChipState::Off);
        else if(actual[i])
            chips_[i].SetState(StudioChipState::Visible);
        else {
            chips_[i].SetState(StudioChipState::Suppressed);
            suppressed_count_++;
        }
        String tip = kStudioFeatures[i].name;
        tip << (requested ? actual[i] ? " — requested and visible"
                                      : " — requested but suppressed by production/capacity"
                          : " — disabled by authored policy");
        chips_[i].Tip(tip);
    }
    Refresh();
}

void StudioLodRail::SetLayout(const Vector<int>& row_y, const Vector<int>& row_h,
                              const StudioThresholdSet& thresholds)
{
    row_y_ = clone(row_y);
    row_h_ = clone(row_h);
    thresholds_ = thresholds;
    Refresh();
}

void StudioLodRail::Paint(Draw& w)
{
    const bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
    w.DrawRect(GetSize(), dark ? Color(29, 37, 48) : White());
    if(row_y_.GetCount() != STUDIO_LOD_COUNT || row_h_.GetCount() != STUDIO_LOD_COUNT)
        return;

    const Color faces[] = {
        Color(10, 102, 181), Color(46, 127, 195), Color(109, 161, 208), Color(169, 195, 221)
    };
    const char *desc[] = { "Full detail", "Simplified", "Minimal", "Silhouette" };
    String ranges[] = {
        Format("%.0f–%.0f px", thresholds_.normal, STUDIO_RESOLUTION_MAX),
        Format("%.0f–%.0f px", thresholds_.lod1, thresholds_.normal - 1),
        Format("%.0f–%.0f px", thresholds_.lod2, thresholds_.lod1 - 1),
        Format("%.0f–%.0f px", STUDIO_RESOLUTION_MIN, thresholds_.lod2 - 1),
    };
    Font strong = SansSerifZ(DPI(9)).Bold();
    Font small = SansSerifZ(DPI(7));
    for(int i = 0; i < STUDIO_LOD_COUNT; i++) {
        int y = row_y_[i];
        int h = row_h_[i];
        Rect row = RectC(0, y, GetSize().cx, max(0, h - DPI(1)));
        if(!row.IsEmpty()) {
            Color row_face = dark ? Color(29, 37, 48) : White();
            w.DrawRect(row, row_face);
            w.DrawRect(row.left, row.bottom - 1, row.GetWidth(), 1,
                       dark ? Color(52, 64, 78) : Color(229, 234, 240));
        }
        Rect pill = RectC(DPI(12), y + DPI(9), max(0, GetSize().cx - DPI(24)), DPI(23));
        w.DrawRect(pill, faces[i]);
        Color pill_ink = i < 2 || dark ? White() : Color(40, 68, 93);
        Size ts = GetTextSize(kStudioRowNames[i], strong);
        w.DrawText(pill.left + max(0, (pill.GetWidth() - ts.cx) / 2),
                   pill.top + max(0, (pill.GetHeight() - ts.cy) / 2),
                   kStudioRowNames[i], strong, pill_ink);
        Color meta_ink = dark ? Color(151, 166, 183) : Color(99, 114, 133);
        Size rs = GetTextSize(ranges[i], small);
        w.DrawText(max(0, (GetSize().cx - rs.cx) / 2), y + DPI(38), ranges[i], small, meta_ink);
        Size ds = GetTextSize(desc[i], small);
        w.DrawText(max(0, (GetSize().cx - ds.cx) / 2), y + DPI(53), desc[i], small, meta_ink);
    }
}

} // namespace Upp
