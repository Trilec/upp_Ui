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

One<Ctrl> UiDesignerPreviewFactory::Create(const UiDesignerControlSpec& spec)
{
    switch(spec.runtime_kind) {
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
    default: {
        One<UiGroupPanel> placeholder = MakeOne<UiGroupPanel>();
        placeholder->SetTitle(spec.display_name);
        return pick(placeholder);
    }
    }
}

void UiDesignerPreviewFactory::Initialize(Ctrl& ctrl,
                                          const UiDesignerControlSpec& spec)
{
    ctrl.Tip(spec.help.IsEmpty() ? spec.display_name : spec.help);
    if(auto *button = dynamic_cast<UiButton *>(&ctrl))
        button->SetText(spec.display_name);
    if(auto *label = dynamic_cast<UiLabel *>(&ctrl))
        label->SetText(spec.display_name);
    if(auto *group = dynamic_cast<UiGroupPanel *>(&ctrl))
        group->SetTitle(spec.display_name);
    if(auto *title = dynamic_cast<UiTitleCard *>(&ctrl))
        title->SetTitle(spec.display_name);
    if(auto *check = dynamic_cast<UiCheckBox *>(&ctrl))
        check->SetText(spec.display_name);
    if(auto *radio = dynamic_cast<UiRadioButton *>(&ctrl))
        radio->SetText(spec.display_name);
    if(auto *split = dynamic_cast<UiSplitButton *>(&ctrl))
        split->SetText(spec.display_name).Add("First", 1).Add("Second", 2);
    if(auto *drop = dynamic_cast<UiDropdown *>(&ctrl)) {
        drop->UseInternalModel().Clear()
            .Add("First", 1).Add("Second", 2).Add("Third", 3);
        drop->Select(0);
    }
    if(auto *progress = dynamic_cast<UiProgressBar *>(&ctrl))
        progress->Percent(true).Set(50, 100);
    if(auto *slider = dynamic_cast<UiSlider *>(&ctrl))
        slider->SetRange(0, 100).SetValue(50);
    if(auto *breadcrumbs = dynamic_cast<UiBreadcrumbs *>(&ctrl)) {
        breadcrumbs->AddCrumb("Home", "0");
        breadcrumbs->AddCrumb("Current", "1");
        breadcrumbs->SetCurrentIndex(1);
    }
    if(auto *tree = dynamic_cast<UiTree *>(&ctrl)) {
        tree->GetInternalModel().AddChild(
            tree->GetInternalModel().Root(),
            UiModelItem("Workspace", "workspace"));
        tree->ShowConnectorLines(true);
    }
    if(auto *table = dynamic_cast<UiTable *>(&ctrl)) {
        table->UseInternalModel();
        table->GetInternalModel().SetSize(3, 3);
    }
    if(auto *doc = dynamic_cast<UiDoc *>(&ctrl))
        doc->SetText("UiDoc sample");
    if(auto *menu = dynamic_cast<UiMenu *>(&ctrl))
        menu->SetMenuBarMode(true);
    if(auto *composite = dynamic_cast<UiCompositeSlider *>(&ctrl)) {
        composite->SetLabel(spec.display_name);
        composite->SetData(50);
    }
    if(auto *label = dynamic_cast<Label *>(&ctrl))
        label->SetLabel(spec.display_name);
    if(auto *button = dynamic_cast<Button *>(&ctrl))
        button->SetLabel(spec.display_name);
    if(auto *option = dynamic_cast<Option *>(&ctrl))
        option->SetLabel(spec.display_name);
}

UiDesignerApplyResult UiDesignerPreviewFactory::Apply(
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
        if(auto *panel = dynamic_cast<UiPanel *>(&ctrl))
            panel->SetCustomStyle(UiTheme::ResolvePanel(role));
        if(auto *button = dynamic_cast<UiButton *>(&ctrl))
            button->SetCustomStyle(UiTheme::ResolveButton(role));
        if(auto *label = dynamic_cast<UiLabel *>(&ctrl))
            label->SetCustomStyle(UiTheme::ResolveLabel(role));
        if(auto *group = dynamic_cast<UiGroupPanel *>(&ctrl))
            group->SetCustomStyle(UiTheme::ResolveGroupPanel(role));
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
    if(property == "checked") {
        ctrl.SetData(value);
        ctrl.Refresh();
        return UiDesignerApplyResult::AppliedControlState;
    }
    if(property == "value") {
        const double number = value;
        if(auto *slider = dynamic_cast<UiSlider *>(&ctrl))
            slider->SetValue(number);
        else if(auto *progress = dynamic_cast<UiProgressBar *>(&ctrl))
            progress->Set((int)number, 100);
        else if(auto *intedit = dynamic_cast<UiIntEdit *>(&ctrl))
            intedit->SetValue((int)number);
        else if(auto *floatedit = dynamic_cast<UiFloatEdit *>(&ctrl))
            floatedit->SetValue(number);
        else
            ctrl.SetData(value);
        ctrl.Refresh();
        return UiDesignerApplyResult::AppliedControlState;
    }
    if(property == "minimum" || property == "maximum") {
        return UiDesignerApplyResult::AppliedControlState;
    }
    if(property == "color") {
        ctrl.SetData(value);
        ctrl.Refresh();
        return UiDesignerApplyResult::AppliedPaint;
    }
    if(property == "rows" || property == "columns" ||
       property == "direction")
        return UiDesignerApplyResult::RequiresSubtreeRebuild;
    if(property == "x" || property == "y" ||
       property == "width" || property == "height" ||
       property.StartsWith("minimum_") ||
       property.StartsWith("maximum_"))
        return UiDesignerApplyResult::AppliedAncestorLayout;
    if(property == "name")
        return UiDesignerApplyResult::AppliedControlState;
    return UiDesignerApplyResult::Rejected;
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

void UiDesignerPreviewCanvas::SetCatalog(const UiDesignerCatalog *catalog)
{
    catalog_ = catalog;
}

void UiDesignerPreviewCanvas::SetDocument(const UiDesignerDocument *document)
{
    document_ = document;
}

void UiDesignerPreviewCanvas::SetOverlay(const UiDesignerTransientOverlay *overlay)
{
    overlay_ = overlay;
}

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
    for(const UiDesignerPropertySpec& property : spec->properties) {
        Value value = Effective(node, property.id, property.default_value);
        UiDesignerPreviewFactory::Apply(*instance.control, *spec,
                                        property.id, value);
    }
}

void UiDesignerPreviewCanvas::BuildNode(UiDesignerNodeId node_id,
                                        ParentCtrl& parent, int depth,
                                        UiDesignerNodeId runtime_parent)
{
    if(!document_ || !catalog_)
        return;
    const UiDesignerNode* node = document_->Find(node_id);
    if(!node)
        return;

    if(node_id != document_->GetRootId()) {
        const UiDesignerControlSpec* spec = catalog_->Find(node->type);
        if(!spec)
            return;

        UiDesignerPreviewInstance& instance = instances_.Add();
        instance.node = node_id;
        instance.runtime_parent = runtime_parent;
        instance.type = node->type;
        instance.control = UiDesignerPreviewFactory::Create(*spec);
        instance.generation = ++generation_sequence_;
        UiDesignerPreviewFactory::Initialize(*instance.control, *spec);
        parent.Add(*instance.control);
        ApplyAllProperties(instance, *node);

        ParentCtrl* child_parent = dynamic_cast<ParentCtrl *>(instance.control.Get());
        const UiDesignerNodeId child_runtime_parent = child_parent
            ? node_id : runtime_parent;
        if(!child_parent)
            child_parent = &parent;
        for(UiDesignerNodeId child : node->children)
            BuildNode(child, *child_parent, depth + 1,
                      child_runtime_parent);
    }
    else {
        for(UiDesignerNodeId child : node->children)
            BuildNode(child, parent, depth + 1, 0);
    }
}

void UiDesignerPreviewCanvas::RebuildDocument()
{
    DestroyInstances();
    if(document_ && catalog_)
        BuildNode(document_->GetRootId(), *this, 0);
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
        if((include_root && candidate == node) ||
           IsRuntimeDescendant(candidate, node)) {
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
        RemoveInstanceTree(root, false);
        for(UiDesignerNodeId child : node->children)
            BuildNode(child, *this, 0, 0);
    }
    else {
        const int q = FindInstance(root);
        if(q < 0)
            return false;
        ParentCtrl* root_parent = dynamic_cast<ParentCtrl *>(instances_[q].control.Get());
        if(root_parent) {
            RemoveInstanceTree(root, false);
            ApplyAllProperties(instances_[q], *node);
            for(UiDesignerNodeId child : node->children)
                BuildNode(child, *root_parent, 1, root);
        }
        else {
            const UiDesignerNodeId runtime_parent = instances_[q].runtime_parent;
            ParentCtrl* parent = this;
            if(runtime_parent) {
                Ctrl* parent_ctrl = FindRuntime(runtime_parent);
                ParentCtrl* candidate = dynamic_cast<ParentCtrl *>(parent_ctrl);
                if(candidate)
                    parent = candidate;
            }
            RemoveInstanceTree(root, true);
            BuildNode(root, *parent, 0, runtime_parent);
        }
    }
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
    const UiDesignerControlSpec* spec =
        node && catalog_ ? catalog_->Find(node->type) : nullptr;
    if(q < 0 || !spec || !instances_[q].control) {
        stats_.rejected++;
        return UiDesignerApplyResult::Rejected;
    }

    UiDesignerApplyResult result = UiDesignerPreviewFactory::Apply(
        *instances_[q].control, *spec, property, value);
    stats_.live_applies++;

    switch(result) {
    case UiDesignerApplyResult::AppliedPaint:
        stats_.paint_updates++;
        break;
    case UiDesignerApplyResult::AppliedLocalLayout:
        stats_.local_layouts++;
        Layout();
        break;
    case UiDesignerApplyResult::AppliedAncestorLayout:
        stats_.ancestor_layouts++;
        Layout();
        break;
    case UiDesignerApplyResult::RequiresSubtreeRebuild:
        RebuildSubtree(node_id);
        break;
    case UiDesignerApplyResult::RequiresFullRebuild:
        RebuildDocument();
        break;
    case UiDesignerApplyResult::Rejected:
        stats_.rejected++;
        break;
    default:
        break;
    }
    Refresh();
    return result;
}

void UiDesignerPreviewCanvas::ApplyChangeSet(
    const UiDesignerChangeSet& changes)
{
    if(HasUiDesignerImpact(changes.CombinedImpact(),
                           UiDesignerImpactFullPreview) ||
       changes.schema_changed) {
        RebuildDocument();
        return;
    }

    if(!changes.structure.IsEmpty()) {
        Index<UiDesignerNodeId> affected;
        for(const UiDesignerStructureChange& change : changes.structure) {
            UiDesignerNodeId root = change.new_parent
                ? change.new_parent : change.old_parent;
            if(!root)
                root = document_ ? document_->GetRootId() : 0;
            if(root && affected.Find(root) < 0)
                affected.Add(root);
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
}

void UiDesignerPreviewCanvas::LayoutNode(UiDesignerNodeId node_id,
                                         int ordinal, int depth)
{
    const int q = FindInstance(node_id);
    const UiDesignerNode* node = document_ ? document_->Find(node_id) : nullptr;
    if(q < 0 || !node)
        return;

    const int x = (int)Effective(*node, "x", 20 + depth * 18);
    const int y = (int)Effective(*node, "y", 20 + ordinal * 44);
    const int cx = max(20, (int)Effective(*node, "width", 160));
    const int cy = max(20, (int)Effective(*node, "height", 32));

    instances_[q].control->SetRect(x, y, cx, cy);
    Point origin(0, 0);
    const UiDesignerNodeId runtime_parent = instances_[q].runtime_parent;
    if(runtime_parent) {
        const int parent_rect = rects_.Find(runtime_parent);
        if(parent_rect >= 0)
            origin = rects_[parent_rect].TopLeft();
    }
    rects_.GetAdd(node_id) = RectC(origin.x + x, origin.y + y, cx, cy);

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
    int ordinal = 0;
    for(UiDesignerNodeId child : root->children)
        LayoutNode(child, ordinal++, 0);
}

void UiDesignerPreviewCanvas::Paint(Draw& w)
{
    w.DrawRect(GetSize(), SColorPaper());
    const Size doc_size = document_ ? document_->GetVirtualSize() : GetSize();
    w.DrawRect(0, 0, min(doc_size.cx, GetSize().cx),
               min(doc_size.cy, GetSize().cy),
               Blend(SColorPaper(), SColorFace(), 180));

    if(selection_) {
        for(UiDesignerNodeId id : selection_->nodes) {
            Rect r = GetNodeRect(id);
            if(!r.IsEmpty()) {
                Color color = id == selection_->primary
                                  ? accent_
                                  : Blend(accent_, White(), 115);
                w.DrawRect(r.left, r.top, r.Width(), 2, color);
                w.DrawRect(r.left, r.bottom - 2, r.Width(), 2, color);
                w.DrawRect(r.left, r.top, 2, r.Height(), color);
                w.DrawRect(r.right - 2, r.top, 2, r.Height(), color);
            }
        }
    }
}

void UiDesignerPreviewCanvas::LeftDown(Point p, dword keyflags)
{
    UiDesignerNodeId hit = 0;
    for(int i = rects_.GetCount() - 1; i >= 0; i--)
        if(rects_[i].Contains(p)) {
            hit = rects_.GetKey(i);
            break;
        }
    WhenSelectNode(hit, (keyflags & K_CTRL) != 0);
    SetFocus();
}

}
