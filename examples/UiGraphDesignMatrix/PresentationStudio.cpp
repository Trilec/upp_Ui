#include "PresentationStudio.h"

namespace Upp {
namespace {

void SetSmallLabelStyle(UiLabel& label, int height = 9, bool bold = true)
{
    label.ClearCustomStyle();
    UiLabel::Style style = label.GetStyle();
    style.font = SansSerifZ(DPI(height));
    if(bold)
        style.font.Bold();
    style.transparent = true;
    label.SetCustomStyle(style);
}

} // namespace

void StudioGuideBar::Paint(Draw& w)
{
    const bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
    const Color face = dark ? Color(29, 37, 48) : White();
    const Color line = dark ? Color(44, 54, 66) : Color(233, 238, 243);
    const Color muted = dark ? Color(151, 166, 183) : Color(99, 114, 133);
    const Color accent = dark ? Color(82, 169, 229) : Color(12, 127, 211);
    w.DrawRect(GetSize(), face);
    w.DrawRect(0, 0, GetSize().cx, 1, line);
    w.DrawRect(0, GetSize().cy - 1, GetSize().cx, 1, line);

    Font text = SansSerifZ(DPI(7));
    Font strong = text; strong.Bold();
    int x = DPI(7);
    int y = max(0, (GetSize().cy - text.GetCy()) / 2);
    String guide = "LOD scale left · cell top = projected resolution / size / actual level · cell bottom = requested feature policy";
    w.DrawText(x, y, guide, text, muted);
    x += GetTextSize(guide, text).cx + DPI(12);
    w.DrawText(x, y, "FEATURES", strong, muted);
    x += GetTextSize("FEATURES", strong).cx + DPI(5);

    for(int i = 0; i < STUDIO_FEATURE_COUNT; i++) {
        String tag = kStudioFeatures[i].tag;
        int width = max(DPI(24), GetTextSize(tag, strong).cx + DPI(6));
        if(x + width > GetSize().cx - DPI(300))
            break;
        Rect chip = RectC(x, DPI(5), width, max(DPI(17), GetSize().cy - DPI(10)));
        Color chip_face = Blend(face, accent, dark ? 20 : 10);
        Color chip_frame = Blend(line, accent, 90);
        w.DrawRect(chip, chip_face);
        w.DrawRect(chip.left, chip.top, chip.GetWidth(), 1, chip_frame);
        w.DrawRect(chip.left, chip.bottom - 1, chip.GetWidth(), 1, chip_frame);
        w.DrawRect(chip.left, chip.top, 1, chip.GetHeight(), chip_frame);
        w.DrawRect(chip.right - 1, chip.top, 1, chip.GetHeight(), chip_frame);
        Size ts = GetTextSize(tag, strong);
        w.DrawText(chip.left + max(0, (chip.GetWidth() - ts.cx) / 2),
                   chip.top + max(0, (chip.GetHeight() - ts.cy) / 2), tag, strong, accent);
        x += width + DPI(2);
    }

    String note = diagnostics_.IsEmpty()
                ? "green shown · red off · amber requested but suppressed"
                : diagnostics_ + " · green shown · red off · amber suppressed";
    Size ns = GetTextSize(note, text);
    w.DrawText(max(x + DPI(8), GetSize().cx - ns.cx - DPI(8)), y, note, text, muted);
}

UiGraphPresentationStudio::UiGraphPresentationStudio()
{
    Title("UiGraph / Presentation Studio");
    Sizeable().Zoomable();
    SetRect(0, 0, DPI(1580), DPI(860));

    UiThemeContext context = UiTheme::GetContext();
    context.preset = UiThemePreset::Minimal;
    context.mode = UiThemeMode::Light;
    UiTheme::Set(context);

    document_ = StudioMakeDefaultDocument();
    BuildShell();
    ConnectEvents();
    ApplyShellStyles();
    ConfigureFromDocument();

#ifdef _DEBUG
    String smoke_error;
    if(!RunSelectorProjectionSmoke(smoke_error)) {
        LOG("UIGRAPH_STUDIO_SELECTOR_SMOKE FAIL: " << smoke_error);
        ASSERT(false);
    }
    else
        LOG("UIGRAPH_STUDIO_SELECTOR_SMOKE checks=4 failed=0");
#endif
}

void UiGraphPresentationStudio::Paint(Draw& w)
{
    const bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
    w.DrawRect(GetSize(), dark ? Color(20, 26, 34) : Color(238, 242, 246));
}

void UiGraphPresentationStudio::Layout()
{
    Rect r = GetSize();
    const int margin = DPI(10);
    const int gap = DPI(7);
    const int header_h = DPI(48);
    const int toolbar_h = DPI(66);
    const int guide_h = DPI(28);

    int y = DPI(6);
    header_.SetRect(margin, y, max(0, r.GetWidth() - margin * 2), header_h);
    y += header_h + DPI(5);
    toolbar_.SetRect(margin, y, max(0, r.GetWidth() - margin * 2), toolbar_h);
    y += toolbar_h + DPI(5);
    guide_.SetRect(margin, y, max(0, r.GetWidth() - margin * 2), guide_h);
    y += guide_h + DPI(6);

    int body_h = max(0, r.GetHeight() - y - margin);
    lod_view_.SetRect(margin, y, DPI(84), body_h);
    viewport_.SetRect(margin + DPI(84) + gap, y,
                      max(0, r.GetWidth() - (margin * 2 + DPI(84) + gap)), body_h);
    horizontal_.SetPage(viewport_.GetSize().cx);
    vertical_.SetPage(viewport_.GetSize().cy);
    LayoutToolbar();
    ArrangeMatrix();
}

void UiGraphPresentationStudio::BuildShell()
{
    Add(header_);
    header_.SetTitle("UiGraph / Presentation Studio")
           .SetSubTitle("Design · Compare · Tune · Export")
           .SetMedia(ICON_DESIGN_WIDGETS_48())
           .SetMediaSide(UiAlign::LEFT)
           .SetMediaAlign(UiAlign::CENTER, UiAlign::CENTER)
           .SetMediaAutoFit(true)
           .ShowTitleLine(false)
           .SetContentInset(DPI(6))
           .SetContentCell(header_actions_);

    header_actions_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    header_actions_.AddSpacer(1).Expand(1);
    intent_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
    btn_copy_json_.SetText("Copy JSON").Tip("Copy the complete versioned Presentation Studio policy");
    btn_import_.SetText("Import...").Tip("Import a Presentation Studio JSON policy");
    btn_export_.SetText("Export...").Tip("Export the complete Presentation Studio JSON policy");
    btn_theme_.SetIcon(ICON_ACTION_DARK_MODE_48()).SetIconSize(DPI(16), DPI(16)).Tip("Toggle light/dark theme");
    header_actions_.Add(intent_).Fixed(DPI(285));
    header_actions_.Add(btn_copy_json_).Fixed(DPI(82));
    header_actions_.Add(btn_import_).Fixed(DPI(70));
    header_actions_.Add(btn_export_).Fixed(DPI(70));
    header_actions_.Add(btn_theme_).Fixed(DPI(34));

    Add(toolbar_);
    toolbar_.Add(lbl_template_); toolbar_.Add(template_);
    toolbar_.Add(lbl_shape_); toolbar_.Add(shape_filter_);
    toolbar_.Add(lbl_size_); toolbar_.Add(authored_);
    toolbar_.Add(lbl_ports_); toolbar_.Add(ports_);
    toolbar_.Add(lbl_range_); toolbar_.Add(range_scope_);
    toolbar_.Add(btn_global_); toolbar_.Add(btn_use_global_);
    toolbar_.Add(range_); toolbar_.Add(btn_reset_range_);

    lbl_template_.SetText("TEMPLATE");
    lbl_shape_.SetText("SHAPE");
    lbl_size_.SetText("AUTHORED SIZE");
    lbl_ports_.SetText("PORTS");
    lbl_range_.SetText("LOD RESOLUTION");
    range_scope_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);

    for(int i = 0; i < STUDIO_TEMPLATE_COUNT; i++)
        template_.Add(kStudioTemplates[i].name, i);
    shape_filter_.Add("All shapes", 0);
    for(int i = 0; i < STUDIO_SHAPE_COUNT; i++)
        shape_filter_.Add(kStudioShapeNames[i], i + 1);
    authored_.Add("Compact", 0).Add("Reference", 1).Add("Spacious", 2);
    ports_.Add("None", 0).Add("1 IN / 1 OUT", 1).Add("3 IN / 2 OUT", 2).Add("4 IN / 4 OUT", 3);
    template_.SetPopupMaxItems(8);
    shape_filter_.SetPopupMaxItems(10);

    btn_global_.SetText("Global").SetCheckable().Tip("Edit the active template's global resolution thresholds");
    btn_use_global_.SetText("Use global").Tip("Remove the selected shape threshold override");
    btn_reset_range_.SetText("Reset").Tip("Reset the currently edited thresholds to 160 / 80 / 48 px");

    range_.SetRange(STUDIO_RESOLUTION_MIN, STUDIO_RESOLUTION_MAX)
          .SetStep(1.0)
          .SetMinimumSegmentSpan(STUDIO_RESOLUTION_MIN_SPAN)
          .SetReverse(true)
          .SetValueDisplay(UiRangeSegments::ValueDisplay::Domain)
          .SetValuePrecision(0)
          .ShowBoundaryValues(true)
          .ShowEndpointValues(true)
          .ShowValuesOnInteraction(true)
          .ShowLabels(true)
          .ShowDividers(true);
    Vector<Color> lod_palette;
    lod_palette << Color(169, 195, 221) << Color(109, 161, 208)
                << Color(46, 127, 195) << Color(10, 102, 181);
    range_.SetPalette(lod_palette);

    Add(guide_);
    Add(lod_view_);
    lod_view_.Add(lod_rail_);
    Add(viewport_);
    viewport_.Add(sheet_);
    viewport_.AddFrame(horizontal_.Horz());
    viewport_.AddFrame(vertical_);

    for(int i = 0; i < STUDIO_SHAPE_COUNT; i++) {
        UiButton& button = shape_headers_.Add();
        button.SetText(kStudioShapeNames[i]).SetCheckable();
        button.SetIcon(StudioShapeIcon(kStudioShapes[i], Color(12, 127, 211))).SetIconSize(DPI(18), DPI(18));
        button.Tip(String("Edit ") + kStudioShapeNames[i] + " thresholds");
        sheet_.Add(button);
        button.WhenAction = [=] { SelectThresholdShape(i); };
    }

    for(int i = 0; i < STUDIO_SHAPE_COUNT * STUDIO_LOD_COUNT; i++) {
        StudioMatrixCell& cell = cells_.Add();
        sheet_.Add(cell);
        int lod = i / STUDIO_SHAPE_COUNT;
        int shape = i % STUDIO_SHAPE_COUNT;
        cell.WhenFeatureToggle = [=](int feature) { ToggleFeature(shape, lod, feature); };
    }
}

void UiGraphPresentationStudio::ApplyShellStyles()
{
    SetSmallLabelStyle(lbl_template_);
    SetSmallLabelStyle(lbl_shape_);
    SetSmallLabelStyle(lbl_size_);
    SetSmallLabelStyle(lbl_ports_);
    SetSmallLabelStyle(lbl_range_);
    SetSmallLabelStyle(range_scope_, 8, true);

    const bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
    intent_.ClearCustomStyle();
    UiLabel::Style style = intent_.GetStyle();
    style.font = SansSerifZ(DPI(8)).Bold();
    style.transparent = false;
    style.metrics.frame_enabled = true;
    style.metrics.frame_width = 1;
    style.metrics.radius = DPI(10);
    for(int i = 0; i < 4; i++) {
        style.palette.face[i] = UiFill::Solid(dark ? Color(23, 30, 39) : Color(248, 250, 252));
        style.palette.frame[i] = dark ? Color(52, 64, 78) : Color(216, 224, 233);
        style.palette.ink[i] = dark ? Color(151, 166, 183) : Color(99, 114, 133);
    }
    intent_.SetCustomStyle(style);
}

void UiGraphPresentationStudio::LayoutToolbar()
{
    Size size = toolbar_.GetSize();
    const int label_y = DPI(2);
    const int label_h = DPI(14);
    const int control_y = DPI(19);
    const int control_h = DPI(31);
    const int gap = DPI(8);
    int x = DPI(5);

    auto field = [&](UiLabel& label, Ctrl& control, int width) {
        label.SetRect(x, label_y, width, label_h);
        control.SetRect(x, control_y, width, control_h);
        x += width + gap;
    };
    field(lbl_template_, template_, DPI(144));
    field(lbl_shape_, shape_filter_, DPI(132));
    field(lbl_size_, authored_, DPI(128));
    field(lbl_ports_, ports_, DPI(130));

    int reset_w = DPI(58);
    int info_w = DPI(116);
    int range_x = x + info_w;
    int range_w = max(DPI(300), size.cx - range_x - reset_w - DPI(10));

    lbl_range_.SetRect(x, label_y, info_w, label_h);
    range_scope_.SetRect(x, DPI(16), info_w, DPI(18));
    btn_global_.SetRect(x, DPI(39), DPI(52), DPI(20));
    btn_use_global_.SetRect(x + DPI(55), DPI(39), DPI(61), DPI(20));
    range_.SetRect(range_x, DPI(3), range_w, DPI(57));
    btn_reset_range_.SetRect(range_x + range_w + DPI(4), control_y, reset_w, control_h);
}

void UiGraphPresentationStudio::ConnectEvents()
{
    template_.WhenSelect = [=](int index) {
        if(index < 0 || index >= STUDIO_TEMPLATE_COUNT)
            return;
        document_.active_template = kStudioTemplates[index].id;
        ConfigureCells();
        SyncThresholdEditor();
    };
    shape_filter_.WhenSelect = [=](int index) { SetShapeFilter(index); };
    authored_.WhenSelect = [=](int index) {
        document_.authored_size = index == 0 ? "compact" : index == 2 ? "spacious" : "reference";
        ConfigureCells();
    };
    ports_.WhenSelect = [=](int index) {
        static const char *ids[] = { "none", "1x1", "3x2", "4x4" };
        index = minmax(index, 0, 3);
        document_.port_preset = ids[index];
        ConfigureCells();
    };
    btn_global_.WhenAction = [=] { SelectThresholdShape(-1); };
    btn_use_global_.WhenAction = [=] { UseGlobalThresholds(); };
    btn_reset_range_.WhenAction = [=] { ResetThresholds(); };
    range_.WhenChanging = [=] { ApplyThresholdsFromControl(); PreviewThresholdChange(); };
    range_.WhenAction = [=] { ApplyThresholdsFromControl(); PreviewThresholdChange(); };
    horizontal_.WhenScroll = vertical_.WhenScroll = [=] { Scroll(); };
    btn_theme_.WhenAction = [=] { ToggleTheme(); };
    btn_copy_json_.WhenAction = [=] { WriteClipboardText(StoreAsJson(document_, true)); };
    btn_export_.WhenAction = [=] { ExportJson(); };
    btn_import_.WhenAction = [=] { ImportJson(); };
}

int UiGraphPresentationStudio::ActiveTemplateIndex() const
{
    return StudioFindTemplateIndex(document_.active_template);
}

StudioTemplatePolicy& UiGraphPresentationStudio::ActivePolicy()
{
    return document_.templates[ActiveTemplateIndex()];
}

const StudioTemplatePolicy& UiGraphPresentationStudio::ActivePolicy() const
{
    return document_.templates[ActiveTemplateIndex()];
}

StudioThresholdSet UiGraphPresentationStudio::EffectiveThresholds(int shape) const
{
    shape = minmax(shape, 0, STUDIO_SHAPE_COUNT - 1);
    const StudioShapePolicy& policy = ActivePolicy().shapes[shape];
    return policy.threshold_override ? policy.thresholds : ActivePolicy().global_thresholds;
}

StudioThresholdSet UiGraphPresentationStudio::EditorThresholds() const
{
    return selected_shape_index_ >= 0 ? EffectiveThresholds(selected_shape_index_)
                                      : ActivePolicy().global_thresholds;
}

Sizef UiGraphPresentationStudio::CurrentAuthoredSize() const
{
    int index = authored_.GetSelection();
    if(index == 0) return Sizef(DPI(220), DPI(145));
    if(index == 2) return Sizef(DPI(320), DPI(210));
    return Sizef(DPI(260), DPI(170));
}

int UiGraphPresentationStudio::CurrentPortPreset() const
{
    return minmax(ports_.GetSelection(), 0, 3);
}

int UiGraphPresentationStudio::SampleResolution(int shape, int lod) const
{
    StudioThresholdSet t = EffectiveThresholds(shape);
    if(lod == 0) return fround((t.normal + STUDIO_RESOLUTION_MAX) * 0.5);
    if(lod == 1) return fround((t.lod1 + t.normal) * 0.5);
    if(lod == 2) return fround(t.lod2 + (t.lod1 - t.lod2) * 0.95);
    return fround(min((STUDIO_RESOLUTION_MIN + t.lod2) * 0.5, 36.0));
}

void UiGraphPresentationStudio::ConfigureFromDocument()
{
    template_.SetDataSilently(ActiveTemplateIndex());
    shape_filter_.SetDataSilently(shape_filter_index_);
    authored_.SetDataSilently(document_.authored_size == "compact" ? 0 : document_.authored_size == "spacious" ? 2 : 1);
    ports_.SetDataSilently(document_.port_preset == "none" ? 0 : document_.port_preset == "3x2" ? 2 : document_.port_preset == "4x4" ? 3 : 1);
    SyncThresholdEditor();
    ConfigureCells();
}

bool UiGraphPresentationStudio::RunSelectorProjectionSmoke(String& error)
{
    String failure;
    if(cells_.GetCount() != STUDIO_SHAPE_COUNT * STUDIO_LOD_COUNT)
        failure = "matrix cell catalogue is incomplete";

    if(failure.IsEmpty()) {
        template_.Select(0);
        if(document_.active_template != "minimal" || cells_[0].GetTemplateIndex() != 0)
            failure = "template selection did not reconfigure matrix cells";
    }

    if(failure.IsEmpty()) {
        authored_.Select(0);
        Sizef authored = cells_[0].GetAuthoredSize();
        if(abs(authored.cx - DPI(220)) > 0.01 || abs(authored.cy - DPI(145)) > 0.01)
            failure = "authored-size selection did not reach matrix cells";
    }

    if(failure.IsEmpty()) {
        ports_.Select(3);
        if(cells_[0].GetPortInputCount() != 4 || cells_[0].GetPortOutputCount() != 4)
            failure = "port preset selection did not rebuild matrix topology";
    }

    if(failure.IsEmpty()) {
        shape_filter_.Select(3);
        if(shape_filter_index_ != 3 || selected_shape_index_ != 2 || !cells_[2].IsStudioSelected())
            failure = "shape selection did not update matrix selection/filter state";
    }

    document_ = StudioMakeDefaultDocument();
    selected_shape_index_ = -1;
    shape_filter_index_ = 0;
    ConfigureFromDocument();

    error = failure;
    return failure.IsEmpty();
}

void UiGraphPresentationStudio::ConfigureCells()
{
    const int template_index = ActiveTemplateIndex();
    Sizef authored = CurrentAuthoredSize();
    int ports = CurrentPortPreset();
    for(int lod = 0; lod < STUDIO_LOD_COUNT; lod++)
        for(int shape = 0; shape < STUDIO_SHAPE_COUNT; shape++)
            cells_[lod * STUDIO_SHAPE_COUNT + shape].Configure(
                shape, lod, ActivePolicy().shapes[shape].lod[lod], template_index,
                authored, SampleResolution(shape, lod), ports,
                shape == selected_shape_index_);

    intent_.SetText(kStudioTemplates[template_index].intent);
    for(int i = 0; i < shape_headers_.GetCount(); i++)
        shape_headers_[i].SetChecked(i == selected_shape_index_);
    UpdateRangeScope();
    ArrangeMatrix();
    UpdateDiagnostics();
}

void UiGraphPresentationStudio::PreviewThresholdChange()
{
    Sizef authored = CurrentAuthoredSize();
    for(int lod = 0; lod < STUDIO_LOD_COUNT; lod++)
        for(int shape = 0; shape < STUDIO_SHAPE_COUNT; shape++)
            cells_[lod * STUDIO_SHAPE_COUNT + shape].SetSampleResolution(
                SampleResolution(shape, lod), authored);
    ArrangeMatrix();
    UpdateDiagnostics();
}

void UiGraphPresentationStudio::SyncThresholdEditor()
{
    StudioThresholdSet t = EditorThresholds();
    Vector<UiRangeSegment> segments;
    segments.Add(UiRangeSegment(t.lod2 - STUDIO_RESOLUTION_MIN, "LOD 3"));
    segments.Add(UiRangeSegment(t.lod1 - t.lod2, "LOD 2"));
    segments.Add(UiRangeSegment(t.normal - t.lod1, "LOD 1"));
    segments.Add(UiRangeSegment(STUDIO_RESOLUTION_MAX - t.normal, "Normal"));
    syncing_range_ = true;
    range_.SetSegments(segments);
    syncing_range_ = false;
    UpdateRangeScope();
    ArrangeMatrix();
}

void UiGraphPresentationStudio::ApplyThresholdsFromControl()
{
    if(syncing_range_)
        return;
    Vector<double> boundaries = range_.GetBoundaryValues();
    if(boundaries.GetCount() != 3)
        return;
    StudioThresholdSet t;
    t.lod2 = boundaries[0];
    t.lod1 = boundaries[1];
    t.normal = boundaries[2];
    t.Normalize();
    if(selected_shape_index_ >= 0) {
        StudioShapePolicy& shape = ActivePolicy().shapes[selected_shape_index_];
        shape.threshold_override = true;
        shape.thresholds = t;
    }
    else
        ActivePolicy().global_thresholds = t;
    UpdateRangeScope();
}

void UiGraphPresentationStudio::UpdateRangeScope()
{
    const int template_index = ActiveTemplateIndex();
    String scope;
    if(selected_shape_index_ < 0)
        scope = "Global";
    else {
        const StudioShapePolicy& shape = ActivePolicy().shapes[selected_shape_index_];
        scope = kStudioShapeNames[selected_shape_index_];
        scope << (shape.threshold_override ? " override" : " · global values");
    }
    scope << " · " << kStudioTemplates[template_index].name;
    range_scope_.SetText(scope);
    btn_global_.SetChecked(selected_shape_index_ < 0);
    bool can_use_global = selected_shape_index_ >= 0
                       && ActivePolicy().shapes[selected_shape_index_].threshold_override;
    btn_use_global_.Enable(can_use_global);
}

void UiGraphPresentationStudio::UpdateDiagnostics()
{
    int suppressed = 0;
    for(int lod = 0; lod < STUDIO_LOD_COUNT; lod++)
        for(int shape = 0; shape < STUDIO_SHAPE_COUNT; shape++) {
            if(shape_filter_index_ > 0 && shape != shape_filter_index_ - 1)
                continue;
            suppressed += cells_[lod * STUDIO_SHAPE_COUNT + shape].SuppressedCount();
        }
    guide_.SetDiagnostics(suppressed ? Format("%d requested feature%s suppressed", suppressed, suppressed == 1 ? "" : "s")
                                     : "all requested features shown at samples");
}

void UiGraphPresentationStudio::ToggleFeature(int shape, int lod, int feature)
{
    if(shape < 0 || shape >= STUDIO_SHAPE_COUNT || lod < 0 || lod >= STUDIO_LOD_COUNT
       || feature < 0 || feature >= STUDIO_FEATURE_COUNT)
        return;
    StudioFeatureSet& policy = ActivePolicy().shapes[shape].lod[lod];
    policy.Set(feature, !policy.Get(feature));
    cells_[lod * STUDIO_SHAPE_COUNT + shape].Configure(
        shape, lod, policy, ActiveTemplateIndex(), CurrentAuthoredSize(),
        SampleResolution(shape, lod), CurrentPortPreset(), shape == selected_shape_index_);
    UpdateDiagnostics();
}

void UiGraphPresentationStudio::SelectThresholdShape(int shape)
{
    selected_shape_index_ = shape < 0 ? -1 : minmax(shape, 0, STUDIO_SHAPE_COUNT - 1);
    for(int i = 0; i < shape_headers_.GetCount(); i++)
        shape_headers_[i].SetChecked(i == selected_shape_index_);
    SyncThresholdEditor();
    ConfigureCells();
}

void UiGraphPresentationStudio::SetShapeFilter(int dropdown_index)
{
    shape_filter_index_ = minmax(dropdown_index, 0, STUDIO_SHAPE_COUNT);
    if(shape_filter_index_ > 0)
        selected_shape_index_ = shape_filter_index_ - 1;
    ArrangeMatrix();
    SyncThresholdEditor();
    ConfigureCells();
}

void UiGraphPresentationStudio::UseGlobalThresholds()
{
    if(selected_shape_index_ < 0)
        return;
    StudioShapePolicy& shape = ActivePolicy().shapes[selected_shape_index_];
    shape.threshold_override = false;
    SyncThresholdEditor();
    PreviewThresholdChange();
}

void UiGraphPresentationStudio::ResetThresholds()
{
    StudioThresholdSet defaults;
    if(selected_shape_index_ < 0)
        ActivePolicy().global_thresholds = defaults;
    else {
        StudioShapePolicy& shape = ActivePolicy().shapes[selected_shape_index_];
        shape.threshold_override = true;
        shape.thresholds = defaults;
    }
    SyncThresholdEditor();
    PreviewThresholdChange();
}

void UiGraphPresentationStudio::ArrangeMatrix()
{
    if(cells_.GetCount() != STUDIO_SHAPE_COUNT * STUDIO_LOD_COUNT
       || shape_headers_.GetCount() != STUDIO_SHAPE_COUNT)
        return;

    Vector<int> visible;
    if(shape_filter_index_ == 0) {
        for(int i = 0; i < STUDIO_SHAPE_COUNT; i++)
            visible.Add(i);
    }
    else
        visible.Add(shape_filter_index_ - 1);

    const int header_h = DPI(34);
    const int row_heights[STUDIO_LOD_COUNT] = { DPI(220), DPI(170), DPI(132), DPI(96) };
    column_width_ = visible.GetCount() == 1
                  ? min(DPI(620), max(DPI(420), viewport_.GetSize().cx - DPI(4)))
                  : DPI(252);

    for(int shape = 0; shape < STUDIO_SHAPE_COUNT; shape++) {
        shape_headers_[shape].Hide();
        for(int lod = 0; lod < STUDIO_LOD_COUNT; lod++)
            cells_[lod * STUDIO_SHAPE_COUNT + shape].Hide();
    }

    for(int c = 0; c < visible.GetCount(); c++) {
        int shape = visible[c];
        shape_headers_[shape].Show();
        shape_headers_[shape].SetRect(c * column_width_, 0, column_width_ - DPI(1), header_h - DPI(1));
    }

    Vector<int> row_y, row_h;
    int y = header_h;
    for(int lod = 0; lod < STUDIO_LOD_COUNT; lod++) {
        int h = row_heights[lod];
        row_y.Add(y);
        row_h.Add(h);
        for(int c = 0; c < visible.GetCount(); c++) {
            int shape = visible[c];
            StudioMatrixCell& cell = cells_[lod * STUDIO_SHAPE_COUNT + shape];
            cell.Show();
            cell.SetRect(c * column_width_, y, column_width_ - DPI(1), h - DPI(1));
        }
        y += h;
    }

    sheet_width_ = max(1, column_width_ * visible.GetCount());
    sheet_height_ = y;
    sheet_.SetRect(-horizontal_.Get(), -vertical_.Get(), sheet_width_, sheet_height_);
    horizontal_.SetTotal(sheet_width_);
    vertical_.SetTotal(sheet_height_);
    horizontal_.SetPage(viewport_.GetSize().cx);
    vertical_.SetPage(viewport_.GetSize().cy);
    lod_rail_.SetRect(0, -vertical_.Get(), lod_view_.GetSize().cx, sheet_height_);
    lod_rail_.SetLayout(row_y, row_h, EditorThresholds());
    Scroll();
}

void UiGraphPresentationStudio::Scroll()
{
    sheet_.SetRect(-horizontal_.Get(), -vertical_.Get(), sheet_width_, sheet_height_);
    lod_rail_.SetRect(0, -vertical_.Get(), lod_view_.GetSize().cx, sheet_height_);
}

void UiGraphPresentationStudio::ToggleTheme()
{
    UiThemeContext context = UiTheme::GetContext();
    context.mode = context.mode == UiThemeMode::Dark ? UiThemeMode::Light : UiThemeMode::Dark;
    UiTheme::Set(context);
    ApplyShellStyles();
    ConfigureCells();
    RefreshLayout();
    Refresh();
}

void UiGraphPresentationStudio::ExportJson()
{
    FileSel fs;
    fs.Type("UiGraph Presentation Studio policy", "*.json");
    fs.DefaultName("uigraph_presentation_policy.json");
    if(!fs.ExecuteSaveAs("Export UiGraph Presentation Studio policy"))
        return;
    if(!SaveFile(~fs, StoreAsJson(document_, true)))
        Exclamation("Unable to save the Presentation Studio policy file.");
}

void UiGraphPresentationStudio::ImportJson()
{
    FileSel fs;
    fs.Type("UiGraph Presentation Studio policy", "*.json");
    if(!fs.ExecuteOpen("Import UiGraph Presentation Studio policy"))
        return;
    StudioDocument loaded;
    if(!LoadFromJson(loaded, LoadFile(~fs))) {
        Exclamation("The selected file is not valid Presentation Studio JSON.");
        return;
    }
    String error;
    if(!StudioValidateDocument(loaded, error)) {
        Exclamation(error);
        return;
    }
    document_ = pick(loaded);
    selected_shape_index_ = -1;
    shape_filter_index_ = 0;
    ConfigureFromDocument();
}

} // namespace Upp
