#include "WorkspaceWindow.h"
#include <plugin/png/png.h>
#include <plugin/jpg/jpg.h>

namespace Upp {
namespace GraphWorkspace {
namespace {
void CompactLabel(UiLabel& label, const String& text, bool bold = false)
{
    label.SetText(text);
    auto s = label.GetStyle(); s.font = StdFont().Height(DPI(bold ? 12 : 11));
    if(bold) s.font = s.font.Bold();
    label.SetCustomStyle(s);
}
Image SampleImage()
{
    ImageBuffer b(96, 64); b.SetKind(IMAGE_OPAQUE);
    for(int y = 0; y < 64; y++) for(int x = 0; x < 96; x++) {
        RGBA& p = b[y][x]; p.a = 255;
        bool land = y > 39 + (x / 9) % 8;
        p.r = land ? 43 : 128 + y; p.g = land ? 101 : 180 + y / 2; p.b = land ? 91 : 219;
    }
    return Image(b);
}
}

UiGraphNodeTemplate* NodeWorkspace::EditableLayout(Document& d)
{
    if(d.edit_base) return &d.family.base_layout;
    return d.family.layout_override[d.shape] ? &d.family.shape_layout[d.shape] : nullptr;
}
Appearance* NodeWorkspace::EditableStyle(Document& d)
{
    if(d.edit_base) return &d.family.base_style;
    return d.family.style_override[d.shape] ? &d.family.shape_style[d.shape] : nullptr;
}
NodeWorkspace::NodeWorkspace()
{
    Title("UiGraph / Node Design Workspace"); Sizeable().Zoomable();
    SetRect(0, 0, DPI(1560), DPI(980));
    UiThemeContext context = UiTheme::GetContext();
    context.preset = UiThemePreset::Minimal; context.mode = UiThemeMode::Light; UiTheme::Set(context);
    document_ = MakeDocument(); preview_image_ = SampleImage(); preview_small_ = CachedRescale(preview_image_, Size(2, 2));
    RegisterPropertyEditorEditors(factory_);
    factory_.RegisterPicker("workspace-image", [this](Value& v, Ctrl* owner) { return PickImage(v, owner); });
    factory_.RegisterThumbnailProvider("workspace-image", [](const Value& v) { return v.Is<Image>() ? (Image)v : Image(); });
    BuildShell(); Connect();
    ready_ = true; ApplyDocument(); RebuildInspector(); Layout();
}
NodeWorkspace::~NodeWorkspace()
{
    inspector_.SetModel(nullptr); preview_.WhenViewport.Clear();
}
void NodeWorkspace::BuildShell()
{
    Add(root_.SizePos());
    root_.SetInset(DPI(8)).SetGap(DPI(6)).SetAlignItems(UiCrossAlign::Stretch);
    root_.Add(header_).Fixed(DPI(48)); root_.Add(body_).Expand(1); root_.Add(status_).Fixed(DPI(22));
    header_.SetGap(DPI(5)).SetAlignItems(UiCrossAlign::Center);
    CompactLabel(heading_, "UiGraph / Node Design Workspace", true); header_.Add(heading_).Fixed(DPI(290));
    CompactLabel(current_, "Media"); header_.Add(current_).Expand(1);
    UiButton* actions[] = { &new_, &clone_, &open_, &save_, &save_as_, &undo_button_, &theme_ };
    const char* titles[] = { "New", "Clone", "Open JSON", "Save", "Save As", "Undo", "Theme" };
    for(int i = 0; i < 7; i++) { actions[i]->SetText(titles[i]); header_.Add(*actions[i]).Fixed(DPI(i == 2 ? 86 : 65)); }
    body_.SetGap(DPI(8)).SetAlignItems(UiCrossAlign::Stretch);
    body_.Add(left_).Fixed(DPI(210)); body_.Add(center_split_).Expand(1); body_.Add(rail_).Fixed(DPI(350));
    left_.SetGap(DPI(5)).SetInset(DPI(5)).SetAlignItems(UiCrossAlign::Stretch);
    CompactLabel(family_label_, "TEMPLATE FAMILY", true); left_.Add(family_label_).Fixed(DPI(22));
    for(int i = 1; i <= 7; i++) family_.Add(UiGraphNodeTemplateName((UiGraphNodeTemplateKind)i), i);
    left_.Add(family_).Fixed(DPI(29));
    CompactLabel(shape_label_, "SHAPE PREVIEW", true); left_.Add(shape_label_).Fixed(DPI(22));
    for(int y = 0; y < 3; y++) {
        shape_rows_[y].SetDirection(UiDirection::H).SetGap(DPI(4));
        for(int x = 0; x < 3; x++) {
            int i = y * 3 + x;
            shape_buttons_[i].SetText(i == 8 ? "BASE" : shape_names[i]).SetCheckable();
            shape_rows_[y].Add(shape_buttons_[i]).Expand(1);
        }
        left_.Add(shape_rows_[y]).Fixed(DPI(34));
    }
    CompactLabel(scope_label_, "Editing Base"); left_.Add(scope_label_).Fixed(DPI(38));
    left_.Add(detach_layout_).Fixed(DPI(25)); left_.Add(detach_style_).Fixed(DPI(25));
    copy_all_.SetText("Copy current to all shapes..."); left_.Add(copy_all_).Fixed(DPI(25));
    CompactLabel(preview_data_label_, "PREVIEW DATA", true); left_.Add(preview_data_label_).Fixed(DPI(22));
    UiDropdown* dropdowns[] = { &connector_, &inputs_, &outputs_ };
    const char* labels[] = { "Connector", "Inputs", "Outputs" };
    for(int i = 0; i < 3; i++) {
        preview_rows_[i].SetDirection(UiDirection::H).SetGap(DPI(5));
        CompactLabel(preview_names_[i], labels[i]); preview_rows_[i].Add(preview_names_[i]).Fixed(DPI(66));
        preview_rows_[i].Add(*dropdowns[i]).Expand(1); left_.Add(preview_rows_[i]).Fixed(DPI(26));
    }
    connector_.Add("Straight").Add("Bezier").Add("Orthogonal");
    for(int i = 0; i <= 32; i++) { inputs_.Add(AsString(i)); outputs_.Add(AsString(i)); }
    expand_.SetText("Expand specimen"); left_.Add(expand_).Fixed(DPI(25));
    CompactLabel(palette_label_, "COMPONENTS / drag or click", true); left_.Add(palette_label_).Fixed(DPI(24));
    for(int y = 0; y < 4; y++) {
        palette_rows_[y].SetDirection(UiDirection::H).SetGap(DPI(5));
        for(int x = 0; x < 2 && y * 2 + x < 7; x++) {
            int i = y * 2 + x;
            palette_[i].SetText(kind_names[i + 1]);
            palette_[i].Tip(i == 6 ? "Painted actions only. Live Ctrl objects require a host SetNodeCtrl binding." : "Drag onto a region diagram or structure row; click adds to selected region.");
            palette_rows_[y].Add(palette_[i]).Expand(1);
        }
        left_.Add(palette_rows_[y]).Fixed(DPI(43));
    }
    center_split_.Vert(center_top_, table_box_).SetSplitPercent(38).SetMinPixels(0, DPI(265)).SetMinPixels(1, DPI(260));
    center_top_.SetGap(DPI(5)).SetAlignItems(UiCrossAlign::Stretch);
    center_top_.Add(threshold_row_).Fixed(DPI(62)); center_top_.Add(three_up_).Expand(1);
    threshold_row_.SetGap(DPI(8)); threshold_row_.Add(range_).Expand(1);
    range_.SetRange(1, 320).SetStep(1).SetMinimumSegmentSpan(1).SetReverse(true)
          .ShowLabels().ShowBoundaryValues().ShowEndpointValues().SetValuePrecision(0);
    three_up_.SetGap(DPI(6)).SetAlignItems(UiCrossAlign::Stretch);
    UiBoxLayout* panels[] = { &region_box_, &overlay_box_, &preview_box_ };
    UiLabel* captions[] = { &region_label_, &overlay_label_, &preview_label_ };
    Ctrl* views[] = { &region_, &overlay_, &preview_ };
    for(int i = 0; i < 3; i++) {
        panels[i]->SetGap(DPI(3)).SetAlignItems(UiCrossAlign::Stretch);
        panels[i]->Add(*captions[i]).Fixed(DPI(24));
        if(i == 2) panels[i]->Add(camera_tools_).Fixed(DPI(25));
        panels[i]->Add(*views[i]).Expand(1); three_up_.Add(*panels[i]).Expand(i == 2 ? 3 : 2);
    }
    CompactLabel(region_label_, "Node Region", true); CompactLabel(overlay_label_, "Node Overlay", true);
    overlay_.SetOverlay(true);
    camera_tools_.SetGap(DPI(2));
    const char* camera[] = { "N", "L1", "L2", "L3", "1:1", "Fit" };
    for(int i = 0; i < 6; i++) { camera_buttons_[i].SetText(camera[i]); if(i < 4) camera_buttons_[i].SetCheckable(); camera_tools_.Add(camera_buttons_[i]).Expand(1); }
    preview_.SetAutoFitOnFirstPaint(false).SetEditable(false);
    auto style = preview_.GetStyle(); style.show_grid = false; for(int i = 0; i < 4; i++) style.canvas_palette.face[i] = UiFill::Solid(Color(243, 249, 253)); style.min_zoom = 0.01; style.max_zoom = 8;
    style.node.title_font = StdFont().Height(DPI(18)).Bold(); style.node.subtitle_font = StdFont().Height(DPI(13)); style.node.description_font = StdFont().Height(DPI(13));
    preview_.SetCustomStyle(style); preview_.SetModel(model_);
    table_box_.SetGap(DPI(4)).SetAlignItems(UiCrossAlign::Stretch);
    CompactLabel(table_label_, "Structure / component placement + LOD visibility", true);
    table_box_.Add(table_label_).Fixed(DPI(30)); table_box_.Add(table_).Expand(1);
    rail_.SetGap(DPI(4)).SetAlignItems(UiCrossAlign::Stretch);
    rail_.Add(tools_).Fixed(DPI(36)); rail_.Add(selection_label_).Fixed(DPI(36));
    tools_.SetGap(DPI(4));
    Image icons[] = { ICON_DESIGN_TUNE_48(), ICON_DESIGN_WIDGETS_48(), ICON_DESIGN_FORMAT_PAINT_48(), ICON_DESIGN_CODE_BLOCKS_48() };
    const char* modes[] = { "Inspector", "Template / Layout", "Style overrides", "Generated C++" };
    for(int i = 0; i < 4; i++) { mode_[i].SetIcon(icons[i]).SetIconSize(DPI(18), DPI(18)).SetCheckable().Tip(modes[i]); tools_.Add(mode_[i]).Fixed(DPI(36)); }
    remove_.SetText("Remove"); tools_.AddSpacer(1).Expand(1); tools_.Add(remove_).Fixed(DPI(66));
    rail_.Add(inspector_).Expand(1); rail_.Add(code_tools_).Fixed(DPI(28)); rail_.Add(code_).Expand(1);
    copy_code_.SetText("Copy C++"); save_code_.SetText("Save .cpp"); code_tools_.SetGap(DPI(5));
    code_tools_.Add(copy_code_).Expand(1); code_tools_.Add(save_code_).Expand(1);
    code_.SetReadOnly(); code_.Hide(); code_tools_.Hide();
    inspector_.SetFactory(&factory_); inspector_.SetModel(&properties_);
    inspector_.SetLabelRatio(42);
    auto ps = inspector_.GetStyle(); ps.row_height = DPI(27); ps.group_height = DPI(29); inspector_.SetStyle(ps);
}
void NodeWorkspace::Connect()
{
    new_.WhenAction = [this] { NewFamily(1, true); };
    clone_.WhenAction = [this] { CloneFamily(); }; open_.WhenAction = [this] { Open(); };
    save_.WhenAction = [this] { Save(false); }; save_as_.WhenAction = [this] { Save(true); };
    undo_button_.WhenAction = [this] { Undo(); };
    theme_.WhenAction = [this] {
        FinishProperty(); auto c = UiTheme::GetContext(); c.mode = c.mode == UiThemeMode::Dark ? UiThemeMode::Light : UiThemeMode::Dark;
        UiTheme::Set(c); ApplyDocument(); RebuildInspector(); Refresh();
    };
    family_.WhenSelect = [this](int i) { if(!building_ && i >= 0) NewFamily(i + 1); };
    for(int i = 0; i < 9; i++) shape_buttons_[i].WhenAction = [this, i] {
        FinishProperty(); document_.revision++; document_.edit_base = i == 8;
        if(i < 8) document_.shape = i;
        selection_.id.Clear(); Changed();
    };
    detach_layout_.WhenAction = [this] { Detach(false); }; detach_style_.WhenAction = [this] { Detach(true); };
    copy_all_.WhenAction = [this] { CopyAll(); };
    UiDropdown* dropdowns[] = { &connector_, &inputs_, &outputs_ };
    for(int i = 0; i < 3; i++) dropdowns[i]->WhenSelect = [this, i](int n) {
        if(building_ || n < 0) return;
        FinishProperty(); PushUndo(document_);
        if(i == 0) document_.connector = n; else if(i == 1) document_.inputs = n; else document_.outputs = n;
        document_.dirty = true; document_.revision++; Changed();
    };
    for(int i = 0; i < 7; i++) {
        palette_[i].WhenAction = [this, i] { AddComponent((UiGraphNodeComponentKind)(i + 1)); };
        palette_[i].WhenDrag = [this, i] { DragComponent(palette_[i], String(), (UiGraphNodeComponentKind)(i + 1)); };
    }
    region_.WhenSelect = overlay_.WhenSelect = table_.WhenSelect = preview_.WhenComponentSelect = [this](String id, int r) { Select(id, r); };
    region_.WhenDrag = [this](String id) { DragComponent(region_, id, UiGraphNodeComponentKind::Auto); };
    overlay_.WhenDrag = [this](String id) { DragComponent(overlay_, id, UiGraphNodeComponentKind::Auto); };
    table_.WhenDrag = [this](String id) { DragComponent(table_, id, UiGraphNodeComponentKind::Auto); };
    region_.WhenDrop = [this](PasteClip& d, int r, String before) { return Drop(d, r, before); };
    overlay_.WhenDrop = [this](PasteClip& d, int r, String before) { return Drop(d, r, before); };
    table_.WhenDrop = [this](PasteClip& d, int r, String before) { return Drop(d, r, before); };
    table_.WhenLod = [this](String id, int l) { ToggleLod(id, l); };
    remove_.WhenAction = [this] {
        FinishProperty(); Document next = document_; String error;
        if(!RemoveComponent(next, Scope(), selection_.id, error)) { status_.SetText(error); return; }
        PushUndo(document_); document_ = next; selection_.id.Clear(); Changed();
    };
    expand_.WhenAction = [this] {
        FinishProperty(); PushUndo(document_);
        if(!expanded_) { compact_size_ = document_.size; document_.size = Size(min(2048, compact_size_.cx * 3 / 2), min(2048, compact_size_.cy * 3 / 2)); }
        else document_.size = compact_size_;
        expanded_ = !expanded_; expand_.SetText(expanded_ ? "Compact specimen" : "Expand specimen");
        document_.dirty = true; document_.revision++; Changed();
    };
    range_.WhenChanging = range_.WhenAction = [this] { ChangeThresholds(); };
    for(int i = 0; i < 6; i++) camera_buttons_[i].WhenAction = [this, i] { if(i < 4) Jump(i); else ResetCamera(i == 5); };
    preview_.WhenViewport = [this] {
        if(applying_ || !ready_) return;
        document_.zoom = preview_.GetZoom(); document_.pan = preview_.GetPan();
        if(!camera_action_) jump_ = -1;
        Reports();
    };
    for(int i = 0; i < 4; i++) mode_[i].WhenAction = [this, i] { SelectPage(i); };
    copy_code_.WhenAction = [this] { String error, code = GenerateCpp(document_, error); if(error.IsEmpty()) WriteClipboardText(code); else status_.SetText(error); };
    save_code_.WhenAction = [this] { ExportCode(); };
    inspector_.WhenBeginEdit = [this](String, Value) { property_origin_ = new Document(document_); };
    inspector_.WhenPreview = [this](String id, Value v) { ApplyProperty(id, v, false); };
    inspector_.WhenCommit = [this](String id, Value v) { ApplyProperty(id, v, true); };
    inspector_.WhenCancel = [this](String, Value) { CancelProperty(); };
    inspector_.WhenOverride = [this](String id, bool active) { OverrideProperty(id, active); };
    inspector_.WhenUndoRequest = [this](String) { Undo(); };
}
void NodeWorkspace::Paint(Draw& w)
{
    bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
    w.DrawRect(GetSize(), dark ? Color(24, 31, 40) : Color(238, 242, 246));
}
void NodeWorkspace::Layout()
{
    root_.SetRect(GetSize());
    if(ready_ && !started_) { started_ = true; ResetCamera(true); RunSmoke(); }
}
void NodeWorkspace::ApplyDocument(bool update_range)
{
    if(!ready_) return;
    applying_ = true;
    preview_.BeginBatchUpdate();
    String error;
    if(!preview_.SetNodeTemplateClass("workspace", EffectiveLayout(), error)) { status_.SetText(error); preview_.EndBatchUpdate(); applying_ = false; return; }
    UiGraphNodeStyle style = preview_.GetStyle().node;
    document_.family.Style(Scope()).Apply(style);
    int labels = document_.data.Find("show_port_labels");
    style.show_port_labels = labels >= 0 && document_.data.GetValue(labels).Is<bool>() && (bool)document_.data.GetValue(labels);
    preview_.SetNodeStyleClass("workspace", style);
    UiGraphNode n;
    n.title = document_.title; n.subtitle = document_.subtitle; n.description = document_.description;
    n.style_class = "workspace"; n.shape = shapes[document_.shape];
    n.size = Sizef(DPI(document_.size.cx), DPI(document_.size.cy)); n.icon = ICON_DESIGN_WIDGETS_48();
    document_.family.Style(Scope()).Apply(n);
    ValueMap data = document_.data; data.Set("image", preview_image_); data.Set("image_overview", preview_small_); n.data = data;
    for(int side = 0; side < 2; side++) for(int i = 0; i < (side ? document_.outputs : document_.inputs); i++) {
        UiGraphPort p; p.id = (side ? "out_" : "in_") + AsString(i); p.title = p.id;
        p.side = side ? UiGraphPortSide::Right : UiGraphPortSide::Left;
        p.direction = side ? UiGraphPortDirection::Output : UiGraphPortDirection::Input;
        p.multiplicity = UiGraphPortMultiplicity::Multiple; n.ports.Add(p);
    }
    // Rebuild preview-only topology. Component layout, ports and edges still use
    // the production model and renderer; none of these samples are exported.
    model_.Clear();
    node_ = model_.AddNode(n);
    for(int side = 0; side < 2; side++) {
        int count = side ? document_.outputs : document_.inputs;
        if(!count) continue;
        UiGraphNode peer;
        peer.size = Sizef(DPI(6), DPI(6));
        peer.position = Pointf(side ? n.size.cx + DPI(45) : -DPI(51), n.size.cy * 0.5 - DPI(3));
        UiGraphPort port; port.id = "endpoint";
        port.side = side ? UiGraphPortSide::Left : UiGraphPortSide::Right;
        port.direction = side ? UiGraphPortDirection::Input : UiGraphPortDirection::Output;
        port.multiplicity = UiGraphPortMultiplicity::Multiple; peer.ports.Add(port);
        UiGraphNodeRef endpoint = model_.AddNode(peer);
        for(int i = 0; i < count; i++) {
            UiGraphEdge e; UiGraphPortRef own{node_, (side ? "out_" : "in_") + AsString(i)};
            UiGraphPortRef other{endpoint, "endpoint"};
            e.source = side ? own : other; e.target = side ? other : own;
            e.route = (UiGraphRouteStyle)(document_.connector + 1);
            e.arrow = UiGraphArrowStyle::None;
            model_.AddEdge(e);
        }
    }
    preview_.EndBatchUpdate(); applying_ = false;
    if(update_range) SyncRange();
    SyncLeft(); Reports();
}
void NodeWorkspace::Reports()
{
    if(!ready_ || !preview_.GetNodePresentation(node_, snapshot_)) return;
    preview_.GetNodeOutline(node_, outline_, surface_);
    preview_.snapshot = snapshot_; preview_.selected = selection_; preview_.Refresh();
    region_.Set(EffectiveLayout(), snapshot_, outline_, surface_, selection_);
    overlay_.Set(EffectiveLayout(), snapshot_, outline_, surface_, selection_);
    table_.Set(EffectiveLayout(), snapshot_, selection_);
    String actual = snapshot_.level == UiGraphPresentationLevel::Normal ? "Normal" : "LOD " + AsString((int)snapshot_.level);
    CompactLabel(preview_label_, actual + Format(" / %.2fx", preview_.GetZoom()), true);
    region_label_.SetText("Node Region / " + actual); overlay_label_.SetText("Node Overlay / " + actual);
    for(int i = 0; i < 4; i++) camera_buttons_[i].SetChecked(jump_ == i);
    static const char* reps[] = { "Hidden", "Text", "Icon", "Bar", "Dot", "Image", "Progress", "Fields", "Tags", "Actions", "Mosaic" };
    String report = Format("%d px / %s / %s", surface_.GetWidth(), ~actual, document_.edit_base ? "editing Base" : "shape scope");
    if(auto c = snapshot_.FindComponent(selection_.id)) report << " / " << c->id << ": " << reps[(int)c->representation] << " / reason " << AsString((int)c->reason);
    if(!snapshot_.fits) report << " / capacity limited";
    if(!snapshot_.template_error.IsEmpty()) report << " / " << snapshot_.template_error;
    status_.SetText(report);
    current_.SetText(document_.family.name + (document_.dirty ? " *" : "") + " / " + shape_names[document_.shape]);
}
void NodeWorkspace::SyncLeft()
{
    building_ = true;
    family_.Select(max(0, (int)EffectiveLayout().kind - 1));
    connector_.Select(document_.connector); inputs_.Select(document_.inputs); outputs_.Select(document_.outputs);
    for(int i = 0; i < 9; i++) shape_buttons_[i].SetChecked(i == 8 ? document_.edit_base : !document_.edit_base && document_.shape == i);
    String scope = document_.edit_base ? "Editing Base defaults" : String(shape_names[document_.shape]) + " / " + (document_.family.layout_override[document_.shape] ? "layout override" : "inherits Base layout");
    scope_label_.SetText(scope);
    detach_layout_.SetText(document_.family.layout_override[document_.shape] ? "Reset layout to Base" : "Create layout override");
    detach_style_.SetText(document_.family.style_override[document_.shape] ? "Reset style to Base" : "Create style override");
    detach_layout_.Enable(!document_.edit_base); detach_style_.Enable(!document_.edit_base);
    remove_.Enable(!selection_.id.IsEmpty() && EditableLayout(document_) != nullptr);
    range_.Enable(EditableLayout(document_) != nullptr); building_ = false;
}
void NodeWorkspace::SyncRange()
{
    building_ = true;
    const auto& t = EffectiveLayout().lod_widths;
    int high = max(320, t.normal + max(32, t.normal / 2));
    Vector<UiRangeSegment> segments;
    segments << UiRangeSegment(t.lod2, "LOD 3", Color(239, 165, 11))
             << UiRangeSegment(t.lod1 - t.lod2, "LOD 2", Color(23, 177, 132))
             << UiRangeSegment(t.normal - t.lod1, "LOD 1", Color(20, 144, 210))
             << UiRangeSegment(high - t.normal, "Normal", Color(107, 113, 124));
    range_.SetRange(0, high).SetSegments(segments);
    building_ = false;
}
void NodeWorkspace::ChangeThresholds()
{
    if(building_) return;
    FinishProperty(); Document next = document_; auto* t = EditableLayout(next); if(!t) return;
    auto values = range_.GetBoundaryValues(); if(values.GetCount() != 3) return;
    t->SetLodWidths(fround(values[2]), fround(values[1]), fround(values[0]));
    if(!ValidateCandidate(next)) return;
    // A range drag can emit many updates; avoid one undo record per pixel.
    if(!document_.dirty) PushUndo(document_);
    document_ = next; document_.dirty = true; document_.revision++;
    ApplyDocument(false);
}
void NodeWorkspace::Select(const String& id, int r)
{
    FinishProperty(); selection_.id = id; selection_.region = r; page_ = 0; RebuildInspector(); Reports();
}
void NodeWorkspace::SelectPage(int page)
{
    FinishProperty(); page_ = page; RebuildInspector();
}
void NodeWorkspace::Changed(bool inspector)
{
    ApplyDocument(); if(inspector) RebuildInspector();
}
bool NodeWorkspace::ValidateCandidate(const Document& d)
{
    String error; if(Validate(d, error)) return true; status_.SetText(error); return false;
}
void NodeWorkspace::PushUndo(const Document& d)
{
    if(undo_.GetCount() >= 16) undo_.Remove(0);
    undo_.Add(d);
}
void NodeWorkspace::Undo()
{
    FinishProperty(); if(undo_.IsEmpty()) return;
    int revision = document_.revision + 1; document_ = undo_.Top(); undo_.Drop(); document_.revision = revision; document_.dirty = true;
    Changed();
}
void NodeWorkspace::Jump(int level)
{
    const auto& t = EffectiveLayout().lod_widths;
    double width = level == 0 ? t.normal + max(20, t.normal / 4)
                 : level == 1 ? (t.normal + t.lod1) * 0.5 : level == 2 ? (t.lod1 + t.lod2) * 0.5 : t.lod2 * 0.5;
    camera_action_ = true; jump_ = level;
    preview_.SetZoom(width / max(1, DPI(document_.size.cx)), Point(0, 0)); preview_.CenterOnNode(node_);
    camera_action_ = false; Reports();
}
void NodeWorkspace::ResetCamera(bool fit)
{
    camera_action_ = true; jump_ = -1;
    double z = fit ? min((double)max(1, preview_.GetSize().cx - DPI(44)) / DPI(document_.size.cx),
                        (double)max(1, preview_.GetSize().cy - DPI(36)) / DPI(document_.size.cy)) : 1.0;
    preview_.SetZoom(minmax(z, 0.01, 8.0), Point(0, 0)); preview_.CenterOnNode(node_);
    camera_action_ = false; Reports();
}
void NodeWorkspace::ToggleLod(const String& id, int level)
{
    FinishProperty(); Document next = document_; auto* t = EditableLayout(next); if(!t) return;
    int i = t->FindComponent(id); if(i < 0) return; auto& r = t->slots[i]; byte bit = byte(1 << level);
    auto value = r.force_off & bit ? UiGraphNodeLodOverride::Inherit : r.force_on & bit ? UiGraphNodeLodOverride::Off : UiGraphNodeLodOverride::On;
    r.Override((UiGraphPresentationLevel)level, value);
    PushUndo(document_); document_ = next; document_.dirty = true; document_.revision++; Changed();
}
void NodeWorkspace::DragComponent(Ctrl& source, const String& id, UiGraphNodeComponentKind kind)
{
    FinishProperty(); Drag payload; payload.owner = this; payload.scope = Scope(); payload.revision = document_.revision;
    payload.id = id; payload.move = !id.IsEmpty(); payload.kind = kind;
    ImageDraw ghost(DPI(130), DPI(28));
    ghost.DrawRect(Size(DPI(130), DPI(28)), Color(238, 248, 253));
    DrawWorkspaceFrame(ghost, Rect(Size(DPI(130), DPI(28))), Color(12, 127, 211));
    ghost.DrawText(DPI(7), DPI(6), payload.move ? id : String(kind_names[(int)kind]), StdFont().Height(DPI(12)), Color(40, 94, 130));
    source.DoDragAndDrop(InternalClip(payload, "uigraph-workspace"), ghost, payload.move ? DND_MOVE : DND_COPY);
    region_.DragLeave(); overlay_.DragLeave(); table_.DragLeave();
}
bool NodeWorkspace::Drop(PasteClip& clip, int region, String before)
{
    if(!AcceptInternal<Drag>(clip, "uigraph-workspace")) return false;
    const Drag* drag = GetInternalPtr<Drag>(clip, "uigraph-workspace");
    if(!drag || drag->owner != this || drag->scope != Scope() || drag->revision != document_.revision || !EditableLayout(document_)) { clip.SetAction(0); return false; }
    Document next = document_;
    auto r = NewComponent(drag->kind == UiGraphNodeComponentKind::Auto ? UiGraphNodeComponentKind::Text : drag->kind, EffectiveLayout());
    if(drag->move) r.id = drag->id;
    String error;
    if(!PlaceComponent(next, Scope(), drag->revision, r, drag->move, (UiGraphNodeSlotRegion)region, before, error)) { clip.SetAction(0); return false; }
    clip.SetAction(drag->move ? DND_MOVE : DND_COPY);
    if(!clip.IsPaste()) return true;
    auto* t = EditableLayout(next);
    int* reserve = region == 0 ? &t->header_height : region == 7 ? &t->footer_height
                 : region == 1 ? &t->content_left_width : region == 3 ? &t->content_right_width
                 : region == 4 ? &t->overlay_left_width : region == 6 ? &t->overlay_right_width : nullptr;
    if(reserve && *reserve == 0) {
        if(!PromptYesNo("This region has no reservation. Create a 40-unit reservation and place the component?")) { clip.SetAction(0); return false; }
        *reserve = DPI(40);
    }
    if(!ValidateCandidate(next)) { clip.SetAction(0); return false; }
    PushUndo(document_); document_ = next; selection_.id = r.id; selection_.region = region;
    // Deferring reconstruction keeps native DND's source/target controls alive.
    PostCallback([this] { Changed(); }); return true;
}
void NodeWorkspace::AddComponent(UiGraphNodeComponentKind kind)
{
    FinishProperty(); Document next = document_; String error;
    auto r = NewComponent(kind, EffectiveLayout());
    if(!PlaceComponent(next, Scope(), next.revision, r, false, (UiGraphNodeSlotRegion)selection_.region, String(), error)) { status_.SetText(error); return; }
    auto* t = EditableLayout(next); int region = selection_.region;
    int* reserve = region == 0 ? &t->header_height : region == 7 ? &t->footer_height
                 : region == 1 ? &t->content_left_width : region == 3 ? &t->content_right_width
                 : region == 4 ? &t->overlay_left_width : region == 6 ? &t->overlay_right_width : nullptr;
    if(reserve && *reserve == 0) {
        if(!PromptYesNo("This region is unreserved. Create a 40-unit reservation?")) return;
        *reserve = DPI(40);
    }
    if(!ValidateCandidate(next)) return;
    PushUndo(document_); document_ = next; selection_.id = r.id; Changed();
}
void NodeWorkspace::RunSmoke()
{
#ifdef _DEBUG
    double zoom = preview_.GetZoom(); Pointf pan = preview_.GetPan(); Size size = document_.size;
    int normal = document_.family.base_layout.lod_widths.normal;
    document_.family.base_layout.lod_widths.normal += 3; ApplyDocument();
    bool ok = preview_.GetZoom() == zoom && preview_.GetPan() == pan && document_.size == size;
    document_.family.base_layout.lod_widths.normal = normal; ApplyDocument();
    LOG("UIGRAPH_WORKSPACE_UI_SMOKE checks=1 failed=" << (ok ? 0 : 1)); ASSERT(ok);
#endif
}
} // namespace GraphWorkspace
} // namespace Upp
