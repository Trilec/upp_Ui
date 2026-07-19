#include "UiDesignerPreview.h"
#include <Ui/UiIcons.h>
#include <Ui/UiColorPicker.h>

namespace Upp {

static UiRole ParseRole(const Value& value)
{
    const String role = value;
    if(role == "Subtle") return UiRole::Subtle;
    if(role == "Accent") return UiRole::Accent;
    if(role == "Alert") return UiRole::Alert;
    return UiRole::Standard;
}

static UiCrossAlign ParseCrossAlign(const Value& value)
{
    const String align = value;
    if(align == "Start") return UiCrossAlign::Start;
    if(align == "End") return UiCrossAlign::End;
    if(align == "Stretch" || align == "Fill") return UiCrossAlign::Stretch;
    if(align == "Center") return UiCrossAlign::Center;
    return UiCrossAlign::Auto;
}

static UiSpacerLineOrientation ParseLineOrientation(const Value& value)
{
    const String orientation = value;
    if(orientation == "Vertical") return UiSpacerLineOrientation::Vertical;
    if(orientation == "Horizontal") return UiSpacerLineOrientation::Horizontal;
    return UiSpacerLineOrientation::Auto;
}

static UiLineStyle ParseLineDash(const Value& value)
{
    const String dash = value;
    if(dash == "Dash") return DASHED;
    if(dash == "Dot") return DOTTED;
    return SOLID;
}

static One<Ctrl> CreateRuntime(UiDesignerRuntimeKind kind)
{
    switch(kind) {
    case UiDesignerRuntimeKind::UiLabel: return MakeOne<UiLabel>();
    case UiDesignerRuntimeKind::UiCheckBox: return MakeOne<UiCheckBox>();
    case UiDesignerRuntimeKind::UiRadioButton: return MakeOne<UiRadioButton>();
    case UiDesignerRuntimeKind::UiToggle: return MakeOne<UiToggle>();
    case UiDesignerRuntimeKind::UiPanel: return MakeOne<UiPanel>();
    case UiDesignerRuntimeKind::UiDirectContentHost: return MakeOne<UiDirectContentHost>();
    case UiDesignerRuntimeKind::UiGroupPanel: return MakeOne<UiGroupPanel>();
    case UiDesignerRuntimeKind::UiStack: return MakeOne<UiStack>();
    case UiDesignerRuntimeKind::UiAccordion: return MakeOne<UiAccordion>();
    case UiDesignerRuntimeKind::UiScrollPanel: return MakeOne<UiScrollPanel>();
    case UiDesignerRuntimeKind::UiTab: return MakeOne<UiTab>();
    case UiDesignerRuntimeKind::UiTitleCard: return MakeOne<UiTitleCard>();
    case UiDesignerRuntimeKind::UiGridLayout: return MakeOne<UiGridLayout>();
    case UiDesignerRuntimeKind::UiBoxLayout: return MakeOne<UiBoxLayout>();
    case UiDesignerRuntimeKind::UiAbsoluteLayout: return MakeOne<UiAbsoluteLayout>();
    case UiDesignerRuntimeKind::UiButton: return MakeOne<UiButton>();
    case UiDesignerRuntimeKind::UiToolButton: return MakeOne<UiToolButton>();
    case UiDesignerRuntimeKind::UiSplitButton: return MakeOne<UiSplitButton>();
    case UiDesignerRuntimeKind::UiLineEdit: return MakeOne<UiLineEdit>();
    case UiDesignerRuntimeKind::UiIntEdit: return MakeOne<UiIntEdit>();
    case UiDesignerRuntimeKind::UiFloatEdit: return MakeOne<UiFloatEdit>();
    case UiDesignerRuntimeKind::UiPasswordEdit: return MakeOne<UiPasswordEdit>();
    case UiDesignerRuntimeKind::UiMultiEdit: return MakeOne<UiMultiEdit>();
    case UiDesignerRuntimeKind::UiMaskEdit: return MakeOne<UiMaskEdit>();
    case UiDesignerRuntimeKind::UiProgressBar: return MakeOne<UiProgressBar>();
    case UiDesignerRuntimeKind::UiSlider: return MakeOne<UiSlider>();
    case UiDesignerRuntimeKind::UiBreadcrumbs: return MakeOne<UiBreadcrumbs>();
    case UiDesignerRuntimeKind::UiSliderEdit: return MakeOne<UiSliderEdit>();
    case UiDesignerRuntimeKind::UiScrollBar: return MakeOne<UiScrollBar>(UiDirection::V);
    case UiDesignerRuntimeKind::UiSplitter: return MakeOne<UiSplitter>();
    case UiDesignerRuntimeKind::UiQuadSplitter: return MakeOne<UiQuadSplitter>();
    case UiDesignerRuntimeKind::UiTable: return MakeOne<UiTable>();
    case UiDesignerRuntimeKind::UiDoc: return MakeOne<UiDoc>();
    case UiDesignerRuntimeKind::UiTree: return MakeOne<UiTree>();
    case UiDesignerRuntimeKind::UiList: return MakeOne<UiList>();
    case UiDesignerRuntimeKind::UiBezierCurveEditor: return MakeOne<UiBezierCurveEditor>();
    case UiDesignerRuntimeKind::UiBezierCurveField: return MakeOne<UiBezierCurveField>();
    case UiDesignerRuntimeKind::UiDropdown: return MakeOne<UiDropdown>();
    case UiDesignerRuntimeKind::UiMenu: return MakeOne<UiMenu>();
    case UiDesignerRuntimeKind::UiColorPicker: return MakeOne<UiColorPicker>();
    case UiDesignerRuntimeKind::UiCompositeSlider: return MakeOne<UiCompositeSlider>();
    case UiDesignerRuntimeKind::UiCompositeToggle: return MakeOne<UiCompositeToggle>();
    case UiDesignerRuntimeKind::UiCompositeColor: return MakeOne<UiCompositeColor>();
    case UiDesignerRuntimeKind::UiCompositeDropdown: return MakeOne<UiCompositeDropdown>();
    case UiDesignerRuntimeKind::UiCompositeLabel: return MakeOne<UiCompositeLabel>();
    case UiDesignerRuntimeKind::UiCompositeEdit: return MakeOne<UiCompositeEdit>();
    case UiDesignerRuntimeKind::UppLabel: return MakeOne<Label>();
    case UiDesignerRuntimeKind::UppButton: return MakeOne<Button>();
    case UiDesignerRuntimeKind::UppOption: return MakeOne<Option>();
    case UiDesignerRuntimeKind::UppEditString: return MakeOne<EditString>();
    case UiDesignerRuntimeKind::UppEditInt: return MakeOne<EditInt>();
    case UiDesignerRuntimeKind::UppEditDouble: return MakeOne<EditDouble>();
    case UiDesignerRuntimeKind::UppLineEdit: return MakeOne<LineEdit>();
    case UiDesignerRuntimeKind::UppDropList: return MakeOne<DropList>();
    case UiDesignerRuntimeKind::UppArrayCtrl: return MakeOne<ArrayCtrl>();
    case UiDesignerRuntimeKind::UppTreeCtrl: return MakeOne<TreeCtrl>();
    case UiDesignerRuntimeKind::UppTabCtrl: return MakeOne<TabCtrl>();
    case UiDesignerRuntimeKind::UppProgressIndicator: return MakeOne<ProgressIndicator>();
    case UiDesignerRuntimeKind::UppSliderCtrl: return MakeOne<SliderCtrl>();
    case UiDesignerRuntimeKind::UppColorPusher: return MakeOne<ColorPusher>();
    case UiDesignerRuntimeKind::UppParentCtrl: return MakeOne<ParentCtrl>();
    case UiDesignerRuntimeKind::UppStaticRect: return MakeOne<StaticRect>();
    case UiDesignerRuntimeKind::UppSplitter: return MakeOne<Splitter>();
    case UiDesignerRuntimeKind::UppHScrollBar: return MakeOne<HScrollBar>();
    case UiDesignerRuntimeKind::UppVScrollBar: return MakeOne<VScrollBar>();
    case UiDesignerRuntimeKind::SemanticSpacer: return One<Ctrl>();
    default: return MakeOne<UiPanel>();
    }
}

static void InitializeRuntime(Ctrl& ctrl, const UiDesignerControlSpec& spec)
{
    ctrl.Tip(spec.help.IsEmpty() ? spec.display_name : spec.help);
    if(auto *button = dynamic_cast<UiButton *>(&ctrl)) button->SetText(spec.display_name);
    if(auto *label = dynamic_cast<UiLabel *>(&ctrl)) label->SetText(spec.display_name);
    if(auto *group = dynamic_cast<UiGroupPanel *>(&ctrl)) group->SetTitle(spec.display_name);
    if(auto *title = dynamic_cast<UiTitleCard *>(&ctrl)) title->SetTitle(spec.display_name);
    if(auto *check = dynamic_cast<UiCheckBox *>(&ctrl)) check->SetText(spec.display_name);
    if(auto *radio = dynamic_cast<UiRadioButton *>(&ctrl)) radio->SetText(spec.display_name);
    if(auto *split = dynamic_cast<UiSplitButton *>(&ctrl))
        split->SetText(spec.display_name).Add("First", 1).Add("Second", 2);
    if(auto *drop = dynamic_cast<UiDropdown *>(&ctrl)) {
        drop->UseInternalModel().Clear().Add("First", 1).Add("Second", 2).Add("Third", 3);
        drop->Select(0);
    }
    if(auto *progress = dynamic_cast<UiProgressBar *>(&ctrl)) progress->Percent(true).Set(50, 100);
    if(auto *slider = dynamic_cast<UiSlider *>(&ctrl)) slider->SetRange(0, 100).SetValue(50);
    if(auto *breadcrumbs = dynamic_cast<UiBreadcrumbs *>(&ctrl)) {
        breadcrumbs->AddCrumb("Home", "0");
        breadcrumbs->AddCrumb("Current", "1");
        breadcrumbs->SetCurrentIndex(1);
    }
    if(auto *tree = dynamic_cast<UiTree *>(&ctrl)) {
        tree->GetInternalModel().AddChild(tree->GetInternalModel().Root(),
                                          UiModelItem("Workspace", "workspace"));
        tree->ShowConnectorLines(true);
    }
    if(auto *table = dynamic_cast<UiTable *>(&ctrl)) {
        table->UseInternalModel();
        table->GetInternalModel().SetSize(3, 3);
    }
    if(auto *doc = dynamic_cast<UiDoc *>(&ctrl)) doc->SetText("UiDoc sample");
    if(auto *menu = dynamic_cast<UiMenu *>(&ctrl)) menu->SetMenuBarMode(true);
    if(auto *composite = dynamic_cast<UiCompositeSlider *>(&ctrl)) {
        composite->SetLabel(spec.display_name);
        composite->SetData(50);
    }
    if(auto *label = dynamic_cast<Label *>(&ctrl)) label->SetLabel(spec.display_name);
    if(auto *button = dynamic_cast<Button *>(&ctrl)) button->SetLabel(spec.display_name);
    if(auto *option = dynamic_cast<Option *>(&ctrl)) option->SetLabel(spec.display_name);
}

static UiDesignerApplyResult ApplyRuntime(
    Ctrl& ctrl, const UiDesignerControlSpec& spec,
    const String& property, const Value& value)
{
    if(property == "visible") {
        ctrl.Show((bool)value);
        return UiDesignerApplyResult::AppliedControlState;
    }
    if(property == "enabled") {
        ctrl.Enable((bool)value);
        return UiDesignerApplyResult::AppliedControlState;
    }
    if(property == "role") {
        const UiRole role = ParseRole(value);
        if(auto *panel = dynamic_cast<UiPanel *>(&ctrl)) panel->SetCustomStyle(UiTheme::ResolvePanel(role));
        if(auto *button = dynamic_cast<UiButton *>(&ctrl)) button->SetCustomStyle(UiTheme::ResolveButton(role));
        if(auto *label = dynamic_cast<UiLabel *>(&ctrl)) label->SetCustomStyle(UiTheme::ResolveLabel(role));
        if(auto *group = dynamic_cast<UiGroupPanel *>(&ctrl)) group->SetCustomStyle(UiTheme::ResolveGroupPanel(role));
        ctrl.Refresh();
        return UiDesignerApplyResult::AppliedPaint;
    }
    if(property == "text") {
        const String text = value;
        if(auto *label = dynamic_cast<UiLabel *>(&ctrl)) label->SetText(text);
        else if(auto *button = dynamic_cast<UiButton *>(&ctrl)) button->SetText(text);
        else if(auto *check = dynamic_cast<UiCheckBox *>(&ctrl)) check->SetText(text);
        else if(auto *radio = dynamic_cast<UiRadioButton *>(&ctrl)) radio->SetText(text);
        else if(auto *split = dynamic_cast<UiSplitButton *>(&ctrl)) split->SetText(text);
        else if(auto *edit = dynamic_cast<UiLineEdit *>(&ctrl)) edit->SetData(text);
        else if(auto *label = dynamic_cast<Label *>(&ctrl)) label->SetLabel(text);
        else if(auto *button = dynamic_cast<Button *>(&ctrl)) button->SetLabel(text);
        else if(auto *option = dynamic_cast<Option *>(&ctrl)) option->SetLabel(text);
        else return UiDesignerApplyResult::Rejected;
        ctrl.RefreshLayout();
        return UiDesignerApplyResult::AppliedLocalLayout;
    }
    if(property == "title") {
        const String title = value;
        if(auto *group = dynamic_cast<UiGroupPanel *>(&ctrl)) group->SetTitle(title);
        else if(auto *card = dynamic_cast<UiTitleCard *>(&ctrl)) card->SetTitle(title);
        else return UiDesignerApplyResult::Rejected;
        ctrl.RefreshLayout();
        return UiDesignerApplyResult::AppliedLocalLayout;
    }
    if(property == "icon") {
        auto *card = dynamic_cast<UiTitleCard *>(&ctrl);
        if(!card)
            return UiDesignerApplyResult::Rejected;
        const String icon = value;
        if(icon == "ICON_DESIGN_DESCRIPTION_48")
            card->SetMedia(ICON_DESIGN_DESCRIPTION_48(), Size(DPI(18), DPI(18)));
        else
            card->ClearMedia();
        ctrl.RefreshLayout();
        return UiDesignerApplyResult::AppliedLocalLayout;
    }
    if(property == "checked") {
        ctrl.SetData(value);
        ctrl.Refresh();
        return UiDesignerApplyResult::AppliedControlState;
    }
    if(property == "value") {
        const double number = value;
        if(auto *slider = dynamic_cast<UiSlider *>(&ctrl)) slider->SetValue(number);
        else if(auto *progress = dynamic_cast<UiProgressBar *>(&ctrl)) progress->Set((int)number, 100);
        else if(auto *intedit = dynamic_cast<UiIntEdit *>(&ctrl)) intedit->SetValue((int)number);
        else if(auto *floatedit = dynamic_cast<UiFloatEdit *>(&ctrl)) floatedit->SetValue(number);
        else ctrl.SetData(value);
        ctrl.Refresh();
        return UiDesignerApplyResult::AppliedControlState;
    }
    if(property == "color") {
        ctrl.SetData(value);
        ctrl.Refresh();
        return UiDesignerApplyResult::AppliedPaint;
    }
    if(property == "rows" || property == "columns" || property == "direction")
        return UiDesignerApplyResult::RequiresSubtreeRebuild;
    if(property == "x" || property == "y" || property == "width" || property == "height" ||
       property.StartsWith("minimum_") || property.StartsWith("maximum_") ||
       property == "grid_row" || property == "grid_column")
        return UiDesignerApplyResult::AppliedAncestorLayout;
    if(property == "minimum" || property == "maximum" || property == "name")
        return UiDesignerApplyResult::AppliedControlState;
    return UiDesignerApplyResult::Rejected;
}

UiDesignerPreviewAdapterRegistry& UiDesignerPreviewAdapterRegistry::Global()
{
    static UiDesignerPreviewAdapterRegistry registry;
    return registry;
}

void UiDesignerPreviewAdapterRegistry::Register(UiDesignerPreviewAdapter adapter)
{
    for(int i = 0; i < adapters_.GetCount(); i++)
        if(adapters_[i].id == adapter.id) {
            adapters_[i] = pick(adapter);
            return;
        }
    adapters_.Add(pick(adapter));
}

const UiDesignerPreviewAdapter* UiDesignerPreviewAdapterRegistry::Find(const String& id) const
{
    for(const UiDesignerPreviewAdapter& adapter : adapters_)
        if(adapter.id == id)
            return &adapter;
    return nullptr;
}

void UiDesignerPreviewAdapterRegistry::EnsureBuiltins()
{
    if(builtins_registered_)
        return;
    builtins_registered_ = true;
    UiDesignerPreviewAdapter spacer;
    spacer.id = "spacer";
    spacer.semantic = true;
    Register(pick(spacer));
}

const UiDesignerPreviewAdapter* UiDesignerPreviewFactory::Adapter(
    const UiDesignerControlSpec& spec)
{
    UiDesignerPreviewAdapterRegistry& registry = UiDesignerPreviewAdapterRegistry::Global();
    registry.EnsureBuiltins();
    if(const UiDesignerPreviewAdapter* existing = registry.Find(spec.preview_adapter_id))
        return existing;

    UiDesignerPreviewAdapter adapter;
    adapter.id = spec.preview_adapter_id;
    adapter.semantic = spec.IsSemanticItem();
    if(!adapter.semantic) {
        const UiDesignerRuntimeKind kind = spec.runtime_kind;
        adapter.create = [=] { return CreateRuntime(kind); };
        adapter.initialize = InitializeRuntime;
        adapter.apply = ApplyRuntime;
    }
    registry.Register(pick(adapter));
    return registry.Find(spec.preview_adapter_id);
}

One<Ctrl> UiDesignerPreviewFactory::Create(const UiDesignerControlSpec& spec)
{
    const UiDesignerPreviewAdapter* adapter = Adapter(spec);
    return adapter && adapter->create ? adapter->create() : One<Ctrl>();
}

void UiDesignerPreviewFactory::Initialize(Ctrl& ctrl,
                                          const UiDesignerControlSpec& spec)
{
    const UiDesignerPreviewAdapter* adapter = Adapter(spec);
    if(adapter && adapter->initialize)
        adapter->initialize(ctrl, spec);
}

UiDesignerApplyResult UiDesignerPreviewFactory::Apply(
    Ctrl& ctrl, const UiDesignerControlSpec& spec,
    const String& property, const Value& value)
{
    const UiDesignerPreviewAdapter* adapter = Adapter(spec);
    return adapter && adapter->apply
        ? adapter->apply(ctrl, spec, property, value)
        : UiDesignerApplyResult::Rejected;
}

String UiDesignerNodesDragText(const Vector<UiDesignerNodeId>& nodes)
{
    String out = "uidesigner/nodes/v1:";
    for(int i = 0; i < nodes.GetCount(); i++) {
        if(i) out << ',';
        out << nodes[i];
    }
    return out;
}

bool UiDesignerParseNodesDragText(const String& text,
                                  Vector<UiDesignerNodeId>& nodes)
{
    const String prefix = "uidesigner/nodes/v1:";
    if(!text.StartsWith(prefix))
        return false;
    nodes.Clear();
    for(const String& item : Split(text.Mid(prefix.GetCount()), ',')) {
        const int64 id = ScanInt64(item);
        if(id <= 0)
            return false;
        nodes.Add(id);
    }
    return !nodes.IsEmpty();
}

bool UiDesignerReadDragText(PasteClip& clip, String& text)
{
    text.Clear();
    if(clip.IsAvailable("text")) {
        text = clip.Get("text");
        return !text.IsEmpty();
    }
    if(clip.IsAvailable("wtext")) {
        const String wide = clip.Get("wtext");
        text = ToUtf8((const char16 *)~wide,
                      strlen16((const char16 *)~wide));
        return !text.IsEmpty();
    }
    return false;
}

UiDesignerPreviewCanvas::UiDesignerPreviewCanvas()
{
    BackPaint();
}

void UiDesignerPreviewCanvas::Bind(
    const UiDesignerDocument *document, const UiDesignerCatalog *catalog,
    const UiDesignerTransientOverlay *overlay,
    const UiDesignerSelection *selection)
{
    document_ = document;
    catalog_ = catalog;
    overlay_ = overlay;
    selection_ = selection;
}

void UiDesignerPreviewCanvas::SetCatalog(const UiDesignerCatalog *catalog) { catalog_ = catalog; }
void UiDesignerPreviewCanvas::SetDocument(const UiDesignerDocument *document) { document_ = document; }
void UiDesignerPreviewCanvas::SetOverlay(const UiDesignerTransientOverlay *overlay) { overlay_ = overlay; }
void UiDesignerPreviewCanvas::SetSelection(const UiDesignerSelection *selection)
{
    selection_ = selection;
    Refresh();
}

int UiDesignerPreviewCanvas::FindInstance(UiDesignerNodeId node) const
{
    for(int i = 0; i < instances_.GetCount(); i++)
        if(instances_[i].node == node)
            return i;
    return -1;
}

Ctrl* UiDesignerPreviewCanvas::FindRuntime(UiDesignerNodeId node)
{
    const int q = FindInstance(node);
    return q >= 0 ? instances_[q].control.Get() : nullptr;
}

const Ctrl* UiDesignerPreviewCanvas::FindRuntime(UiDesignerNodeId node) const
{
    const int q = FindInstance(node);
    return q >= 0 ? instances_[q].control.Get() : nullptr;
}

uint64 UiDesignerPreviewCanvas::GetInstanceGeneration(UiDesignerNodeId node) const
{
    const int q = FindInstance(node);
    return q >= 0 ? instances_[q].generation : 0;
}

Rect UiDesignerPreviewCanvas::GetNodeRect(UiDesignerNodeId node) const
{
    const int q = rects_.Find(node);
    return q >= 0 ? rects_[q] : Rect(0, 0, 0, 0);
}

UiDesignerNodeId UiDesignerPreviewCanvas::HitNode(Point p) const
{
    for(int i = rects_.GetCount() - 1; i >= 0; i--)
        if(rects_[i].Contains(p))
            return rects_.GetKey(i);
    return 0;
}

void UiDesignerPreviewCanvas::DestroyInstances()
{
    for(UiDesignerPreviewInstance& instance : instances_)
        if(instance.control)
            instance.control->Remove();
    instances_.Clear();
    rects_.Clear();
}

Value UiDesignerPreviewCanvas::Effective(const UiDesignerNode& node,
                                         const String& property,
                                         const Value& fallback) const
{
    const Value canonical = node.GetProperty(property, fallback);
    return overlay_ ? overlay_->Resolve(node.id, property, canonical) : canonical;
}

void UiDesignerPreviewCanvas::ApplyAllProperties(
    UiDesignerPreviewInstance& instance, const UiDesignerNode& node)
{
    const UiDesignerControlSpec* spec = catalog_ ? catalog_->Find(node.type) : nullptr;
    if(!spec || !instance.control)
        return;
    for(const UiDesignerPropertySpec& property : spec->properties)
        UiDesignerPreviewFactory::Apply(*instance.control, *spec,
            property.id, Effective(node, property.id, property.default_value));
}

static void ConfigureBoxSpacer(UiBoxLayout& box,
                               UiBoxLayout::ItemRef item,
                               const UiDesignerNode& node)
{
    if(!node.GetProperty("layout_break", false)) {
        const bool horizontal = box.GetDirection() == UiDirection::H;
        const String main_mode = node.GetProperty(
            horizontal ? "h_sizing" : "v_sizing", "Auto");
        const String cross_mode = node.GetProperty(
            horizontal ? "v_sizing" : "h_sizing", "Auto");
        const int fixed_main = node.GetProperty(
            horizontal ? "fixed_width" : "fixed_height", 0);
        const int fixed_cross = node.GetProperty(
            horizontal ? "fixed_height" : "fixed_width", 0);
        const int min_main = node.GetProperty(
            horizontal ? "min_width" : "min_height", 0);
        const int max_main = node.GetProperty(
            horizontal ? "max_width" : "max_height", 0);
        const int min_cross = node.GetProperty(
            horizontal ? "min_height" : "min_width", 0);
        const int max_cross = node.GetProperty(
            horizontal ? "max_height" : "max_width", 0);

        if(main_mode == "Fixed" && fixed_main > 0)
            item.Fixed(fixed_main);
        else
            item.Expand(max(1, (int)(double)node.GetProperty("weight", 1.0)));
        if(min_main || max_main)
            item.MinMaxMain(min_main, max_main ? max_main : INT_MAX);
        if(cross_mode == "Fixed" && fixed_cross > 0)
            item.MinMaxCross(fixed_cross, fixed_cross);
        else if(min_cross || max_cross)
            item.MinMaxCross(min_cross, max_cross ? max_cross : INT_MAX);
        if(cross_mode == "Fill")
            item.AlignSelf(UiCrossAlign::Stretch);
    }
    item.LineEnabled(node.GetProperty("line_enabled", false))
        .LineOrientation(ParseLineOrientation(node.GetProperty("line_orientation", "Horizontal")))
        .LineAlign(ParseCrossAlign(node.GetProperty("line_align", "Center")))
        .LineThickness((int)node.GetProperty("line_thickness", 1))
        .LineDash(ParseLineDash(node.GetProperty("line_dash", "Solid")))
        .LineInset((int)node.GetProperty("line_inset", 0))
        .LineColorEnabled(node.GetProperty("line_color_enabled", false))
        .LineColor(node.GetProperty("line_color", Color(128, 128, 128)));
}

static void ConfigureGridSpacer(UiGridLayout::BlankRef item,
                                const UiDesignerNode& node)
{
    if((String)node.GetProperty("h_sizing", "Auto") == "Fill") item.ExpandX();
    if((String)node.GetProperty("v_sizing", "Auto") == "Fill") item.ExpandY();
    const int fw = node.GetProperty("fixed_width", 0);
    const int fh = node.GetProperty("fixed_height", 0);
    if(fw) item.FixedWidth(fw);
    if(fh) item.FixedHeight(fh);
    const int minw = node.GetProperty("min_width", 0);
    const int minh = node.GetProperty("min_height", 0);
    const int maxw = node.GetProperty("max_width", 0);
    const int maxh = node.GetProperty("max_height", 0);
    if(minw) item.MinWidth(minw);
    if(minh) item.MinHeight(minh);
    if(maxw) item.MaxWidth(maxw);
    if(maxh) item.MaxHeight(maxh);
    item.LineEnabled(node.GetProperty("line_enabled", false))
        .LineOrientation(ParseLineOrientation(node.GetProperty("line_orientation", "Horizontal")))
        .LineAlign(ParseCrossAlign(node.GetProperty("line_align", "Center")))
        .LineThickness((int)node.GetProperty("line_thickness", 1))
        .LineDash(ParseLineDash(node.GetProperty("line_dash", "Solid")))
        .LineInset((int)node.GetProperty("line_inset", 0))
        .LineColorEnabled(node.GetProperty("line_color_enabled", false))
        .LineColor(node.GetProperty("line_color", Color(128, 128, 128)));
}

void UiDesignerPreviewCanvas::AttachSemanticItem(
    UiDesignerPreviewInstance& instance, const UiDesignerNode& node,
    UiDesignerPreviewInstance& parent_instance)
{
    instance.semantic = true;
    instance.runtime_parent = parent_instance.node;
    if(auto *box = dynamic_cast<UiBoxLayout *>(parent_instance.control.Get())) {
        instance.layout_item_index = box->GetItemCount();
        UiBoxLayout::ItemRef item = node.GetProperty("layout_break", false)
            ? box->AddBreak(max(1, (int)(double)node.GetProperty("weight", 1.0)))
            : box->AddSpacer(max(1, (int)(double)node.GetProperty("weight", 1.0)));
        ConfigureBoxSpacer(*box, item, node);
    }
    else if(auto *grid = dynamic_cast<UiGridLayout *>(parent_instance.control.Get())) {
        instance.layout_item_index = grid->GetItemCount();
        UiGridLayout::BlankRef item = grid->AddBlank(
            node.GetProperty("grid_row", 0), node.GetProperty("grid_column", 0));
        ConfigureGridSpacer(item, node);
    }
}

static void AttachRuntimeChild(Ctrl& parent, Ctrl& child,
                               const UiDesignerNode& node,
                               const String& adapter,
                               int& layout_item_index)
{
    if(adapter == "single") {
        parent.Add(child.SizePos());
        return;
    }
    if(auto *absolute = dynamic_cast<UiAbsoluteLayout *>(&parent)) {
        layout_item_index = absolute->GetItemCount();
        absolute->Add(child,
                      node.GetProperty("x", 20),
                      node.GetProperty("y", 20),
                      max(0, (int)node.GetProperty("width", 160)),
                      max(0, (int)node.GetProperty("height", 32)));
    }
    else if(auto *box = dynamic_cast<UiBoxLayout *>(&parent)) {
        layout_item_index = box->GetItemCount();
        UiBoxLayout::ItemRef item = box->Add(child);
        const bool horizontal = box->GetDirection() == UiDirection::H;
        const String main_mode = node.GetProperty(
            horizontal ? "h_sizing" : "v_sizing", "Auto");
        const int fixed_main = node.GetProperty(
            horizontal ? "fixed_width" : "fixed_height", 0);
        if(main_mode == "Fill")
            item.Expand(max(1, (int)(double)node.GetProperty("weight", 1.0)));
        else if(main_mode == "Fixed")
            item.Fixed(max(0, fixed_main));
        else
            item.Fit();

        const int min_main = node.GetProperty(
            horizontal ? "min_width" : "min_height", 0);
        const int max_main = node.GetProperty(
            horizontal ? "max_width" : "max_height", 0);
        const int min_cross = node.GetProperty(
            horizontal ? "min_height" : "min_width", 0);
        const int max_cross = node.GetProperty(
            horizontal ? "max_height" : "max_width", 0);
        item.MinMaxMain(min_main, max_main > 0 ? max_main : INT_MAX);
        item.MinMaxCross(min_cross, max_cross > 0 ? max_cross : INT_MAX);
    }
    else if(auto *grid = dynamic_cast<UiGridLayout *>(&parent)) {
        layout_item_index = grid->GetItemCount();
        grid->Add(child, node.GetProperty("grid_row", 0),
                  node.GetProperty("grid_column", 0), true);
    }
    else if(auto *tab = dynamic_cast<UiTab *>(&parent))
        tab->Add(child, node.GetProperty("title", node.name));
    else if(auto *tab = dynamic_cast<TabCtrl *>(&parent))
        tab->Add(child, AsString(node.GetProperty("title", node.name)));
    else if(auto *stack = dynamic_cast<UiStack *>(&parent))
        stack->Add(child, node.name);
    else if(auto *accordion = dynamic_cast<UiAccordion *>(&parent)) {
        const int section = accordion->AddSection(
            node.GetProperty("title", node.name), true);
        accordion->GetSectionContent(section).Add(child.SizePos());
    }
    else if(auto *split = dynamic_cast<UiSplitter *>(&parent))
        *split << child;
    else if(auto *quad = dynamic_cast<UiQuadSplitter *>(&parent))
        *quad << child;
    else if(auto *split = dynamic_cast<Splitter *>(&parent))
        *split << child;
    else
        parent.Add(child);
}

void UiDesignerPreviewCanvas::BuildNode(
    UiDesignerNodeId node_id, ParentCtrl& fallback_parent, int depth,
    UiDesignerNodeId runtime_parent)
{
    if(!document_ || !catalog_)
        return;
    const UiDesignerNode* node = document_->Find(node_id);
    if(!node)
        return;
    if(node_id == document_->GetRootId()) {
        for(UiDesignerNodeId child : node->children)
            BuildNode(child, fallback_parent, depth + 1, 0);
        return;
    }

    const UiDesignerControlSpec* spec = catalog_->Find(node->type);
    if(!spec)
        return;
    UiDesignerPreviewInstance& instance = instances_.Add();
    instance.node = node_id;
    instance.runtime_parent = runtime_parent;
    instance.type = node->type;
    instance.adapter_id = spec->preview_adapter_id;
    instance.semantic = spec->IsSemanticItem();
    instance.generation = ++generation_sequence_;

    UiDesignerPreviewInstance* parent_instance = nullptr;
    const UiDesignerControlSpec* parent_spec = nullptr;
    if(runtime_parent) {
        const int p = FindInstance(runtime_parent);
        if(p >= 0) {
            parent_instance = &instances_[p];
            parent_spec = catalog_->Find(parent_instance->type);
        }
    }

    if(instance.semantic) {
        if(parent_instance && parent_instance->control)
            AttachSemanticItem(instance, *node, *parent_instance);
        return;
    }

    instance.control = UiDesignerPreviewFactory::Create(*spec);
    if(!instance.control)
        return;
    UiDesignerPreviewFactory::Initialize(*instance.control, *spec);
    if(parent_instance && parent_instance->control)
        AttachRuntimeChild(*parent_instance->control, *instance.control,
                           *node,
                           parent_spec ? parent_spec->child_adapter_id : "add",
                           instance.layout_item_index);
    else
        fallback_parent.Add(*instance.control);
    ApplyAllProperties(instance, *node);

    ParentCtrl* child_fallback = dynamic_cast<ParentCtrl *>(instance.control.Get());
    if(!child_fallback)
        child_fallback = &fallback_parent;
    const UiDesignerNodeId next_runtime_parent =
        (spec->node_flags & UiDesignerNodeContainer) ? node_id : runtime_parent;
    for(UiDesignerNodeId child : node->children)
        BuildNode(child, *child_fallback, depth + 1, next_runtime_parent);
}

void UiDesignerPreviewCanvas::RebuildDocument()
{
    DestroyInstances();
    if(document_ && catalog_)
        BuildNode(document_->GetRootId(), *this, 0, 0);
    stats_.full_rebuilds++;
    Layout();
    Refresh();
}

bool UiDesignerPreviewCanvas::IsRuntimeDescendant(
    UiDesignerNodeId candidate, UiDesignerNodeId ancestor) const
{
    UiDesignerNodeId current = candidate;
    while(current) {
        const int q = FindInstance(current);
        if(q < 0)
            return false;
        current = instances_[q].runtime_parent;
        if(current == ancestor)
            return true;
    }
    return false;
}

void UiDesignerPreviewCanvas::RemoveInstanceTree(
    UiDesignerNodeId node, bool include_root)
{
    for(int i = instances_.GetCount() - 1; i >= 0; i--) {
        const UiDesignerNodeId candidate = instances_[i].node;
        if((include_root && candidate == node) || IsRuntimeDescendant(candidate, node)) {
            if(instances_[i].control)
                instances_[i].control->Remove();
            rects_.RemoveKey(candidate);
            instances_.Remove(i);
        }
    }
}

bool UiDesignerPreviewCanvas::RebuildSubtree(UiDesignerNodeId root)
{
    if(!document_ || !catalog_)
        return false;
    const UiDesignerNode* node = document_->Find(root);
    if(!node)
        return false;
    if(root == document_->GetRootId()) {
        RebuildDocument();
        return true;
    }

    const int q = FindInstance(root);
    if(q < 0)
        return false;
    const UiDesignerNodeId runtime_parent = instances_[q].runtime_parent;
    ParentCtrl* fallback = this;
    if(runtime_parent) {
        if(ParentCtrl* candidate = dynamic_cast<ParentCtrl *>(FindRuntime(runtime_parent)))
            fallback = candidate;
    }
    RemoveInstanceTree(root, true);
    BuildNode(root, *fallback, 0, runtime_parent);
    stats_.subtree_rebuilds++;
    Layout();
    Refresh();
    return true;
}

UiDesignerApplyResult UiDesignerPreviewCanvas::ApplyProperty(
    UiDesignerNodeId node_id, const String& property, const Value& value)
{
    const int q = FindInstance(node_id);
    const UiDesignerNode* node = document_ ? document_->Find(node_id) : nullptr;
    const UiDesignerControlSpec* spec = node && catalog_ ? catalog_->Find(node->type) : nullptr;
    if(q < 0 || !spec) {
        stats_.rejected++;
        return UiDesignerApplyResult::Rejected;
    }

    UiDesignerApplyResult result;
    if(instances_[q].semantic) {
        result = UiDesignerApplyResult::RequiresSubtreeRebuild;
        const UiDesignerNodeId parent = node->parent;
        if(!RebuildSubtree(parent))
            RebuildDocument();
    }
    else if(instances_[q].control) {
        result = UiDesignerPreviewFactory::Apply(*instances_[q].control,
                                                  *spec, property, value);
        switch(result) {
        case UiDesignerApplyResult::AppliedPaint: stats_.paint_updates++; break;
        case UiDesignerApplyResult::AppliedLocalLayout:
            stats_.local_layouts++; Layout(); break;
        case UiDesignerApplyResult::AppliedAncestorLayout:
            stats_.ancestor_layouts++; Layout(); break;
        case UiDesignerApplyResult::RequiresSubtreeRebuild:
            RebuildSubtree(node_id); break;
        case UiDesignerApplyResult::RequiresFullRebuild:
            RebuildDocument(); break;
        case UiDesignerApplyResult::Rejected: stats_.rejected++; break;
        default: break;
        }
    }
    else {
        result = UiDesignerApplyResult::Rejected;
        stats_.rejected++;
    }
    stats_.live_applies++;
    Refresh();
    return result;
}

void UiDesignerPreviewCanvas::ApplyChangeSet(const UiDesignerChangeSet& changes)
{
    if(HasUiDesignerImpact(changes.CombinedImpact(), UiDesignerImpactFullPreview) ||
       changes.schema_changed) {
        RebuildDocument();
        return;
    }
    if(!changes.structure.IsEmpty()) {
        Index<UiDesignerNodeId> affected;
        for(const UiDesignerStructureChange& change : changes.structure) {
            UiDesignerNodeId root = change.new_parent ? change.new_parent : change.old_parent;
            if(!root) root = document_ ? document_->GetRootId() : 0;
            if(root && affected.Find(root) < 0) affected.Add(root);
        }
        for(int i = 0; i < affected.GetCount(); i++)
            if(!RebuildSubtree(affected[i])) {
                RebuildDocument();
                break;
            }
        return;
    }
    if(changes.virtual_size_changed) {
        Layout();
        Refresh();
    }
    for(const UiDesignerPropertyChange& change : changes.properties)
        ApplyProperty(change.node, change.property, change.new_value);
    if(!changes.behaviors.IsEmpty())
        Refresh();
}

void UiDesignerPreviewCanvas::UpdateSemanticRect(
    UiDesignerPreviewInstance& instance, const UiDesignerNode& node)
{
    Rect local;
    if(Ctrl* parent = FindRuntime(instance.runtime_parent)) {
        if(auto *box = dynamic_cast<UiBoxLayout *>(parent))
            local = box->GetItemRect(instance.layout_item_index);
        else if(auto *grid = dynamic_cast<UiGridLayout *>(parent))
            local = grid->GetItemRect(instance.layout_item_index);
    }
    Rect parent_rect = GetNodeRect(instance.runtime_parent);
    rects_.GetAdd(node.id) = local.Offseted(parent_rect.TopLeft());
}

void UiDesignerPreviewCanvas::LayoutNode(
    UiDesignerNodeId node_id, int ordinal, int depth)
{
    const int q = FindInstance(node_id);
    const UiDesignerNode* node = document_ ? document_->Find(node_id) : nullptr;
    if(q < 0 || !node)
        return;
    UiDesignerPreviewInstance& instance = instances_[q];
    if(instance.semantic) {
        UpdateSemanticRect(instance, *node);
        return;
    }
    if(!instance.control)
        return;

    const UiDesignerNode* parent_node = document_->Find(node->parent);
    const UiDesignerControlSpec* parent_spec = parent_node && catalog_
        ? catalog_->Find(parent_node->type) : nullptr;
    const String adapter = parent_spec ? parent_spec->child_adapter_id : "root";
    const bool managed = adapter == "absolute" ||
                         adapter == "box" || adapter == "grid" ||
                         adapter == "tab" || adapter == "stack" ||
                         adapter == "accordion" || adapter == "splitter" ||
                         adapter == "quad" || adapter == "single" ||
                         adapter == "upp_tab" || adapter == "upp_splitter";
    if(adapter == "absolute" && instance.runtime_parent) {
        Ctrl* parent = FindRuntime(instance.runtime_parent);
        if(auto *absolute = dynamic_cast<UiAbsoluteLayout *>(parent))
            absolute->SetItemRect(
                instance.layout_item_index,
                (int)Effective(*node, "x", 20),
                (int)Effective(*node, "y", 20),
                max(0, (int)Effective(*node, "width", 160)),
                max(0, (int)Effective(*node, "height", 32)));
    }
    if(!managed) {
        Rect available = node->parent == document_->GetRootId()
            ? rects_.Get(node->parent) : RectC(0, 0, GetSize().cx, GetSize().cy);
        const UiDesignerNode* host = document_->Find(node->parent);
        const int inset = host ? max(0, (int)host->GetProperty("inset", 0)) : 0;
        available = available.Deflated(DPI(inset));
        auto Axis = [&](const char *mode_id, const char *fixed_id,
                        const char *min_id, const char *max_id,
                        int natural, int extent) {
            const String mode = Effective(*node, mode_id, "Expand");
            int value = mode == "Expand" ? extent :
                        mode == "Fixed" ? (int)Effective(*node, fixed_id, natural) : natural;
            value = max(value, (int)Effective(*node, min_id, 0));
            const int limit = (int)Effective(*node, max_id, 0);
            if(limit > 0) value = min(value, limit);
            return max(1, value);
        };
        const int cx = Axis("width_mode", "fixed_width", "min_width", "max_width",
                            max(1, instance.control->GetMinSize().cx), available.Width());
        const int cy = Axis("height_mode", "fixed_height", "min_height", "max_height",
                            max(1, instance.control->GetMinSize().cy), available.Height());
        auto Align = [](const String& align, int extent, int size) {
            if(align == "Center") return max(0, (extent - size) / 2);
            if(align == "Right" || align == "Bottom") return max(0, extent - size);
            return 0;
        };
        const String ax = Effective(*node, "cell_align_x", "Auto");
        const String ay = Effective(*node, "cell_align_y", "Auto");
        const Rect root_rect = rects_.Get(node->parent);
        const int x = node->parent == document_->GetRootId()
            ? available.left - root_rect.left + Align(ax == "Auto" ? "Left" : ax, available.Width(), cx)
            : (int)Effective(*node, "x", 20 + depth * 18);
        const int y = node->parent == document_->GetRootId()
            ? available.top - root_rect.top + Align(ay == "Auto" ? "Top" : ay, available.Height(), cy)
            : (int)Effective(*node, "y", 20 + ordinal * 44);
        instance.control->SetRect(x, y, cx, cy);
    }

    Point origin(0, 0);
    if(instance.runtime_parent) {
        const int p = rects_.Find(instance.runtime_parent);
        if(p >= 0)
            origin = rects_[p].TopLeft();
    }
    const Rect local = instance.control->GetRect();
    rects_.GetAdd(node_id) = local.Offseted(origin);

    int child_ordinal = 0;
    for(UiDesignerNodeId child : node->children)
        LayoutNode(child, child_ordinal++, depth + 1);
}

void UiDesignerPreviewCanvas::Layout()
{
    if(!document_)
        return;
    const UiDesignerNode* root = document_->Find(document_->GetRootId());
    if(!root)
        return;
    rects_.Clear();
    // Window is an implicit document host, not another runtime Ctrl. Its
    // rectangle is nevertheless real so hierarchy selection and resize
    // handles describe the same bounded form the user sees.
    rects_.GetAdd(root->id) = RectC(0, 0, GetSize().cx, GetSize().cy);
    for(UiDesignerPreviewInstance& instance : instances_)
        if(instance.control)
            instance.control->Layout();
    int ordinal = 0;
    for(UiDesignerNodeId child : root->children)
        LayoutNode(child, ordinal++, 0);
}

void UiDesignerPreviewCanvas::PaintSemantic(
    Draw& w, const UiDesignerPreviewInstance& instance,
    const UiDesignerNode& node) const
{
    (void)instance;
    Rect r = GetNodeRect(node.id);
    if(r.IsEmpty())
        return;
    const bool selected = selection_ && selection_->Contains(node.id);
    const Color frame = selected ? accent_ : Blend(SColorText(), SColorPaper(), 150);
    w.DrawRect(r, Blend(SColorPaper(), frame, 235));
    w.DrawRect(r.left, r.top, r.Width(), 1, frame);
    w.DrawRect(r.left, r.bottom - 1, r.Width(), 1, frame);
    w.DrawRect(r.left, r.top, 1, r.Height(), frame);
    w.DrawRect(r.right - 1, r.top, 1, r.Height(), frame);
    const String label = node.GetProperty("layout_break", false) ? "Break" : "Spacer";
    w.DrawText(r.left + DPI(5), r.top + DPI(3), label, StdFont().Height(DPI(11)), frame);
    if(node.GetProperty("line_enabled", false)) {
        const Color line = node.GetProperty("line_color_enabled", false)
            ? (Color)node.GetProperty("line_color", frame) : frame;
        const int thickness = max(1, (int)node.GetProperty("line_thickness", 1));
        const int inset = max(0, (int)node.GetProperty("line_inset", 0));
        const String orientation = node.GetProperty("line_orientation", "Horizontal");
        if(orientation == "Vertical")
            w.DrawRect(r.CenterPoint().x, r.top + inset, thickness,
                       max(0, r.Height() - inset * 2), line);
        else
            w.DrawRect(r.left + inset, r.CenterPoint().y,
                       max(0, r.Width() - inset * 2), thickness, line);
    }
}

void UiDesignerPreviewCanvas::Paint(Draw& w)
{
    w.DrawRect(GetSize(), SColorPaper());
    if(document_)
        for(const UiDesignerPreviewInstance& instance : instances_)
            if(instance.semantic)
                if(const UiDesignerNode* node = document_->Find(instance.node))
                    PaintSemantic(w, instance, *node);
}

}
