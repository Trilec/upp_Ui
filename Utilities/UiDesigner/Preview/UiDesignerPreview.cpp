#include "UiDesignerPreview.h"
#include "UiDesignerVisuals.h"
#include <Utilities/UiDesigner/Core/UiDesignerSizing.h>
#include <Utilities/UiDesigner/Theme/UiDesignerThemeAdapter.h>
#include <Ui/UiIcons.h>
#include <Ui/UiColorPicker.h>
#include "UiDesignerColorPickerContract.h"

namespace Upp {

static UiRole ParseRole(const Value& value)
{
    const String role = value;
    if(role == "Subtle") return UiRole::Subtle;
    if(role == "Accent") return UiRole::Accent;
    if(role == "Alert") return UiRole::Alert;
    return UiRole::Standard;
}

static Image ResolveButtonIcon(const String& name)
{
    if(name.IsEmpty() || name == "None")
        return Image();
    if(name == "ICON_DESIGN_DESCRIPTION_48") return ICON_DESIGN_DESCRIPTION_48();
    if(name == "ICON_DESIGN_WIDGETS_48") return ICON_DESIGN_WIDGETS_48();
    if(name == "ICON_DESIGN_ACCOUNT_TREE_48") return ICON_DESIGN_ACCOUNT_TREE_48();
    if(name == "ICON_DESIGN_TUNE_48") return ICON_DESIGN_TUNE_48();
    return Image();
}

static UiIconRenderMode ParseIconRenderMode(const Value& value)
{
    const String mode = value;
    if(mode == "Auto")
        return UiIconRenderMode::Auto;
    if(mode == "PreserveColor")
        return UiIconRenderMode::PreserveColor;
    return UiIconRenderMode::MonoTint;
}

static UiAlign ParseSideAlignChoice(const Value& value)
{
    const String align = value;
    if(align == "Right") return UiAlign::RIGHT;
    if(align == "Top") return UiAlign::TOP;
    if(align == "Bottom") return UiAlign::BOTTOM;
    return UiAlign::LEFT;
}

static UiAlign ParseHorizontalAlignChoice(const Value& value)
{
    const String align = value;
    if(align == "Left") return UiAlign::LEFT;
    if(align == "Right") return UiAlign::RIGHT;
    return UiAlign::CENTER;
}

static UiAlign ParseVerticalAlignChoice(const Value& value)
{
    const String align = value;
    if(align == "Top") return UiAlign::TOP;
    if(align == "Bottom") return UiAlign::BOTTOM;
    return UiAlign::CENTER;
}

static UiSpan ParseUiSpanChoice(const Value& value)
{
    const String span = value;
    if(span == "None") return NONE;
    if(span == "Small") return SMALL;
    return LARGE;
}

static UiLineStyle ParseUiLineStyleChoice(const Value& value)
{
    const String style = value;
    if(style == "Dashed") return DASHED;
    if(style == "Dotted") return DOTTED;
    return SOLID;
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

static String LayoutNodeName(const UiDesignerCatalog *catalog, const UiDesignerNode& node)
{
    if(catalog) {
        const UiDesignerControlSpec* spec = catalog->Find(node.type);
        if(spec && !spec->display_name.IsEmpty())
            return spec->display_name;
    }
    return node.type;
}

static Rect ExpandVisualRect(Rect rect, int amount = DPI(1))
{
    if(amount <= 0)
        return rect;
    return rect.Inflated(amount);
}

static String BoxGapLabel(const UiDesignerCatalog *catalog, const UiDesignerNode& node,
                          const UiDesignerNode* prev, const UiDesignerNode* next,
                          int index)
{
    String base = LayoutNodeName(catalog, node);
    if(prev && next)
        return Format("%s \"%s\" -- between %s and %s", base, node.name, prev->name, next->name);
    if(prev)
        return Format("%s \"%s\" -- after %s", base, node.name, prev->name);
    if(next)
        return Format("%s \"%s\" -- before %s", base, node.name, next->name);
    return Format("%s \"%s\" -- slot %d", base, node.name, index);
}

static UiDesignerCueKind ResolveCueKind(const UiDesignerControlSpec& spec,
                                        const UiDesignerNode& node)
{
    if(node.type == "Spacer")
        return UiDesignerCueKind::SemanticItemBounds;
    if(node.type == "UiBoxLayout" || node.type == "UiGridLayout" ||
       node.type == "UiAbsoluteLayout")
        return UiDesignerCueKind::LayoutBounds;
    if(node.type == "UiPanel" || node.type == "UiDirectContentHost" ||
       node.type == "UiGroupPanel" || node.type == "UiScrollPanel" ||
       node.type == "UiStack" || node.type == "UiAccordion" ||
       node.type == "UiTab" || node.type == "UiTitleCard")
        return UiDesignerCueKind::ContainerBounds;
    if(spec.IsSemanticItem())
        return UiDesignerCueKind::SemanticItemBounds;
    return UiDesignerCueKind::ControlBounds;
}

static void AddRegion(UiDesignerGeometrySnapshotBuilder& snapshot,
                      UiDesignerDropRegion region)
{
    if(region.visual_rect.IsEmpty())
        region.visual_rect = region.rect;
    snapshot.AddRegion(pick(region));
}

static void AddWindowDropRegion(UiDesignerGeometrySnapshotBuilder& snapshot,
                                const UiDesignerNode& node,
                                const UiDesignerGeometryRecord& record)
{
    UiDesignerDropRegion region;
    region.owner = node.id;
    region.kind = UiDesignerDropRegionKind::WindowContent;
    region.rect = record.rect;
    region.visual_rect = record.rect;
    region.depth = record.depth;
    region.paint_order = record.order * 100;
    region.label = "Window";
    AddRegion(snapshot, pick(region));
}

static void AddPanelDropRegion(UiDesignerGeometrySnapshotBuilder& snapshot,
                               const UiDesignerNode& node,
                               const UiDesignerGeometryRecord& record)
{
    UiDesignerDropRegion region;
    region.owner = node.id;
    region.kind = UiDesignerDropRegionKind::PanelBody;
    region.rect = record.body;
    region.visual_rect = record.body;
    region.depth = record.depth + 1;
    region.paint_order = record.order * 100;
    region.label = LayoutNodeName(nullptr, node) + " body";
    AddRegion(snapshot, pick(region));
}

static void AddGridDropRegions(UiDesignerGeometrySnapshotBuilder& snapshot,
                               const UiDesignerDocument& document,
                               const UiDesignerCatalog* catalog,
                               const UiDesignerNode& node,
                               const UiDesignerGeometryRecord& record)
{
    const int rows = max(1, (int)node.GetProperty("rows", 1));
    const int cols = max(1, (int)node.GetProperty("columns", 1));
    const int cell_count = min(rows * cols, record.cell_rects.GetCount());
    if(cell_count <= 0)
        return;
    Vector<bool> occupied;
    occupied.SetCount(rows * cols, false);
    for(UiDesignerNodeId child_id : node.children) {
        const UiDesignerNode* child = document.Find(child_id);
        if(!child)
            continue;
        const int row = (int)child->GetProperty("grid_row", -1);
        const int col = (int)child->GetProperty("grid_column", -1);
        if(row < 0 || col < 0 || row >= rows || col >= cols)
            continue;
        occupied[row * cols + col] = true;
    }
    int order = 0;
    for(int row = 0; row < rows; row++) {
        for(int col = 0; col < cols; col++) {
            const int index = row * cols + col;
            if(index >= cell_count)
                continue;
            UiDesignerDropRegion region;
            region.owner = node.id;
            region.kind = UiDesignerDropRegionKind::GridCell;
            region.rect = record.cell_rects[index];
            region.visual_rect = region.rect;
            region.grid_row = row;
            region.grid_column = col;
            region.depth = record.depth + 1;
            region.paint_order = record.order * 1000 + order++;
            region.occupied = occupied[index];
            region.label = Format("%s \"%s\" -- row %d, column %d",
                                  LayoutNodeName(catalog, node), node.name, row, col);
            AddRegion(snapshot, pick(region));
        }
    }
}

static void AddBoxDropRegions(UiDesignerGeometrySnapshotBuilder& snapshot,
                              const UiDesignerDocument& document,
                              const UiDesignerCatalog* catalog,
                              const UiDesignerNode& node,
                              const UiDesignerGeometryRecord& record,
                              const UiDesignerPreviewInstance* instance)
{
    const UiBoxLayout *box = instance && instance->control
        ? dynamic_cast<const UiBoxLayout *>(instance->control.Get()) : nullptr;
    if(!box)
        return;

    const int count = record.item_rects.GetCount();
    if(count == 0) {
        UiDesignerDropRegion region;
        region.owner = node.id;
        region.kind = UiDesignerDropRegionKind::BoxEmptyBody;
        region.rect = record.rect;
        region.visual_rect = record.rect;
        region.depth = record.depth + 1;
        region.paint_order = record.order * 100;
        region.label = Format("%s \"%s\" -- empty body", LayoutNodeName(catalog, node), node.name);
        AddRegion(snapshot, pick(region));
        return;
    }

    UiDesignerDropRegion body;
    body.owner = node.id;
    body.kind = UiDesignerDropRegionKind::BoxBody;
    body.rect = record.body;
    body.visual_rect = record.body;
    body.depth = record.depth + 1;
    body.paint_order = record.order * 100 + 5;
    body.label = Format("%s \"%s\" -- body", LayoutNodeName(catalog, node), node.name);
    AddRegion(snapshot, pick(body));

    UiDesignerDropRegion frame;
    frame.owner = node.id;
    frame.kind = UiDesignerDropRegionKind::BoxFrame;
    frame.rect = record.rect;
    frame.visual_rect = record.rect;
    frame.depth = record.depth + 1;
    frame.paint_order = record.order * 100 + 4;
    frame.label = Format("%s \"%s\" -- inset frame", LayoutNodeName(catalog, node), node.name);
    AddRegion(snapshot, pick(frame));

    const bool horizontal = node.GetProperty("direction", "V") == "H";
    if(count > 0) {
        const Rect first = record.item_rects[0];
        Rect before;
        if(horizontal) {
            const int width = max(0, first.left - record.body.left);
            if(width > 0)
                before = RectC(record.body.left, first.top, width, first.Height());
        }
        else {
            const int height = max(0, first.top - record.body.top);
            if(height > 0)
                before = RectC(first.left, record.body.top, first.Width(), height);
        }
        if(!before.IsEmpty()) {
            UiDesignerDropRegion region;
            region.owner = node.id;
            region.kind = UiDesignerDropRegionKind::BoxBeforeItem;
            region.rect = before;
            region.visual_rect = ExpandVisualRect(before);
            region.insertion_index = 0;
            region.depth = record.depth + 1;
            region.paint_order = record.order * 100 + 1;
            region.label = BoxGapLabel(catalog, node, nullptr,
                                       document.Find(node.children[0]), 0);
            AddRegion(snapshot, pick(region));
        }
    }

    for(int i = 1; i < count; i++) {
        const Rect prev = record.item_rects[i - 1];
        const Rect next = record.item_rects[i];
        Rect gap;
        if(horizontal) {
            const int width = max(0, next.left - prev.right);
            const int top = max(prev.top, next.top);
            const int bottom = min(prev.bottom, next.bottom);
            if(width > 0 && bottom > top)
                gap = RectC(prev.right, top, width, bottom - top);
        }
        else {
            const int height = max(0, next.top - prev.bottom);
            const int left = max(prev.left, next.left);
            const int right = min(prev.right, next.right);
            if(height > 0 && right > left)
                gap = RectC(left, prev.bottom, right - left, height);
        }
        if(gap.IsEmpty())
            continue;
        UiDesignerDropRegion region;
        region.owner = node.id;
        region.kind = UiDesignerDropRegionKind::BoxGap;
        region.rect = gap;
        region.visual_rect = ExpandVisualRect(gap);
        region.insertion_index = i;
        region.depth = record.depth + 1;
        region.paint_order = record.order * 100 + 10 + i;
        const UiDesignerNode* prev_node = i - 1 < node.children.GetCount()
            ? document.Find(node.children[i - 1]) : nullptr;
        const UiDesignerNode* next_node = i < node.children.GetCount()
            ? document.Find(node.children[i]) : nullptr;
        region.label = BoxGapLabel(catalog, node, prev_node, next_node, i);
        AddRegion(snapshot, pick(region));
    }

    const Rect last = record.item_rects.Top();
    Rect after;
    if(horizontal) {
        const int width = max(0, record.body.right - last.right);
        if(width > 0)
            after = RectC(last.right, last.top, width, last.Height());
    }
    else {
        const int height = max(0, record.body.bottom - last.bottom);
        if(height > 0)
            after = RectC(last.left, last.bottom, last.Width(), height);
    }
    if(!after.IsEmpty()) {
        UiDesignerDropRegion region;
        region.owner = node.id;
        region.kind = UiDesignerDropRegionKind::BoxAfterItem;
        region.rect = after;
        region.visual_rect = ExpandVisualRect(after);
        region.insertion_index = count;
        region.depth = record.depth + 1;
        region.paint_order = record.order * 100 + 90;
        region.label = BoxGapLabel(catalog, node,
                                   document.Find(node.children[count - 1]), nullptr,
                                   count);
        AddRegion(snapshot, pick(region));
    }
}

static void AddLayoutDropRegions(UiDesignerGeometrySnapshotBuilder& snapshot,
                                 const UiDesignerDocument& document,
                                 const UiDesignerCatalog* catalog,
                                 const UiDesignerNode& node,
                                 const UiDesignerGeometryRecord& record,
                                 const UiDesignerPreviewInstance* instance)
{
    if(node.id == document.GetRootId()) {
        AddWindowDropRegion(snapshot, node, record);
        return;
    }
    if(node.type == "UiPanel") {
        AddPanelDropRegion(snapshot, node, record);
        return;
    }
    if(node.type == "UiGridLayout") {
        AddGridDropRegions(snapshot, document, catalog, node, record);
        return;
    }
    if(node.type == "UiBoxLayout") {
        AddBoxDropRegions(snapshot, document, catalog, node, record, instance);
        return;
    }
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
    if(auto *edit = dynamic_cast<UiLineEdit *>(&ctrl))
        edit->SetTextUtf8("Line edit");
    if(auto *edit = dynamic_cast<UiMultiEdit *>(&ctrl))
        edit->SetTextUtf8("Multi-line\nfollowed by text on a second line");
    if(auto *edit = dynamic_cast<UiIntEdit *>(&ctrl))
        edit->SetValue(0);
    if(auto *edit = dynamic_cast<UiFloatEdit *>(&ctrl))
        edit->Precision(2).SetValue(0.0);
    if(auto *edit = dynamic_cast<UiPasswordEdit *>(&ctrl))
        edit->SetPlaceholder("Password");
    if(auto *edit = dynamic_cast<UiPasswordEdit *>(&ctrl))
        edit->SetPasswordChar(0x2022);
    if(auto *edit = dynamic_cast<UiPasswordEdit *>(&ctrl))
        edit->SetPlainTextVisible(false).EnableVisibilityIcon(true).SetTextUtf8("password");
    if(auto *edit = dynamic_cast<UiMaskEdit *>(&ctrl))
        edit->SetMask("##/##/####", '_')
            .ShowError(false)
            .SetTextUtf8("01/02/2026");
    if(auto *drop = dynamic_cast<UiDropdown *>(&ctrl)) {
        drop->UseInternalModel().Clear().Add("First", 1).Add("Second", 2).Add("Third", 3);
        drop->Select(0);
    }
    if(auto *progress = dynamic_cast<UiProgressBar *>(&ctrl)) {
        progress->Percent(true);
        progress->SetText("Loading assets");
        progress->Set(50, 100);
    }
    if(auto *slider = dynamic_cast<UiSlider *>(&ctrl)) slider->SetRange(0, 100).SetValue(50);
    if(auto *breadcrumbs = dynamic_cast<UiBreadcrumbs *>(&ctrl)) {
        breadcrumbs->AddCrumb("Home", "0");
        breadcrumbs->AddCrumb("Current", "1");
        breadcrumbs->SetCurrentIndex(1);
    }
    if(auto *slider = dynamic_cast<UiSliderEdit *>(&ctrl))
        slider->SetValue(50);
    if(auto *progress = dynamic_cast<ProgressIndicator *>(&ctrl)) {
        progress->Percent(true);
        progress->Set(50, 100);
    }
    if(auto *slider = dynamic_cast<SliderCtrl *>(&ctrl))
        slider->MinMax(0, 100).SetData(50);
    if(auto *text = dynamic_cast<EditString *>(&ctrl))
        text->SetText("Edit string");
    if(auto *text = dynamic_cast<EditInt *>(&ctrl))
        text->SetData(0);
    if(auto *text = dynamic_cast<EditDouble *>(&ctrl))
        text->SetData(0.0);
    if(auto *text = dynamic_cast<LineEdit *>(&ctrl))
        text->SetData("Line edit");
    if(auto *drop = dynamic_cast<DropList *>(&ctrl))
        drop->Add("First", 1).Add("Second", 2).SetData(1);
    if(auto *tab = dynamic_cast<TabCtrl *>(&ctrl)) {
        tab->Add("Overview");
        tab->Add("Details");
        tab->SetData(0);
    }
    if(auto *rect = dynamic_cast<StaticRect *>(&ctrl))
        rect->Background(Color(240, 240, 240));
    if(auto *parent = dynamic_cast<ParentCtrl *>(&ctrl))
        parent->SetMinSize(Size(DPI(80), DPI(48)));
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
    if(auto *composite = dynamic_cast<UiCompositeToggle *>(&ctrl)) {
        composite->SetLabel(spec.display_name);
        composite->SetData(true);
    }
    if(auto *composite = dynamic_cast<UiCompositeColor *>(&ctrl)) {
        composite->SetLabel(spec.display_name);
        Vector<Color> colors;
        colors.Add(Color(58, 132, 255));
        composite->SetColors(colors).SetValueText("Blue");
    }
    if(auto *composite = dynamic_cast<UiCompositeDropdown *>(&ctrl)) {
        composite->SetLabel(spec.display_name);
        composite->Clear().Add("First", 1).Add("Second", 2).Select(0);
    }
    if(auto *composite = dynamic_cast<UiCompositeLabel *>(&ctrl)) {
        composite->SetLabel(spec.display_name);
        composite->SetData("Value");
    }
    if(auto *composite = dynamic_cast<UiCompositeEdit *>(&ctrl)) {
        composite->SetLabel(spec.display_name);
        composite->SetData("Editable value");
    }
    if(auto *label = dynamic_cast<Label *>(&ctrl)) label->SetLabel(spec.display_name);
    if(auto *button = dynamic_cast<Button *>(&ctrl)) button->SetLabel(spec.display_name);
    if(auto *option = dynamic_cast<Option *>(&ctrl)) option->SetLabel(spec.display_name);
}

static UiDesignerApplyResult ApplyRuntime(
    Ctrl& ctrl, const UiDesignerControlSpec& spec,
    const String& property, const Value& value)
{
    if(auto *picker = dynamic_cast<UiColorPicker *>(&ctrl)) {
        if(property == "color") { picker->SetColor((Color)value, false); return UiDesignerApplyResult::AppliedPaint; }
        if(property == "alpha") { picker->SetAlpha(minmax((int)value, 0, 255), false); return UiDesignerApplyResult::AppliedPaint; }
        if(property == "alpha_enabled") { picker->SetAlphaEnabled((bool)value); return UiDesignerApplyResult::AppliedPaint; }
        if(property == "page_mode") { UiColorPicker::PageMode m; if(!UiDesignerColorPickerChoiceId(String(value), m)) return UiDesignerApplyResult::Rejected; picker->SetPageMode(m); return UiDesignerApplyResult::AppliedLocalLayout; }
        if(property == "channel_mode") { UiColorPicker::ChannelMode m; if(!UiDesignerColorPickerChoiceId(String(value), m)) return UiDesignerApplyResult::Rejected; picker->SetChannelMode(m); return UiDesignerApplyResult::AppliedLocalLayout; }
        if(property == "spectrum_mode") { UiColorPicker::SpectrumMode m; if(!UiDesignerColorPickerChoiceId(String(value), m)) return UiDesignerApplyResult::Rejected; picker->SetSpectrumMode(m); return UiDesignerApplyResult::AppliedLocalLayout; }
        if(property == "harmony_mode") { UiColorPicker::HarmonyMode m; if(!UiDesignerColorPickerChoiceId(String(value), m)) return UiDesignerApplyResult::Rejected; picker->SetHarmonyMode(m); return UiDesignerApplyResult::AppliedLocalLayout; }
        if(property == "slot_count") { picker->SetSlotCount(minmax((int)value, 1, 4)); return UiDesignerApplyResult::AppliedLocalLayout; }
        if(property == "active_slot") { picker->SetActiveSlot(minmax((int)value, 0, 3)); return UiDesignerApplyResult::AppliedPaint; }
    }
    if(property == "visible") {
        ctrl.Show((bool)value);
        return UiDesignerApplyResult::AppliedControlState;
    }
    if(property == "enabled") {
        ctrl.Enable((bool)value);
        return UiDesignerApplyResult::AppliedControlState;
    }
    if(property == "inset" || property == "gap") {
        const int amount = max(0, (int)value);
        if(auto *box = dynamic_cast<UiBoxLayout *>(&ctrl)) {
            if(property == "inset") box->SetInset(DPI(amount));
            else box->SetGap(DPI(amount));
        }
        else if(auto *grid = dynamic_cast<UiGridLayout *>(&ctrl)) {
            if(property == "inset") grid->SetInset(DPI(amount));
            else grid->SetGap(DPI(amount));
        }
        else return UiDesignerApplyResult::Rejected;
        return UiDesignerApplyResult::AppliedAncestorLayout;
    }
    if(property == "debug_layout") {
        const bool on = (bool)value;
        if(auto *box = dynamic_cast<UiBoxLayout *>(&ctrl)) {
            box->SetDebug(on);
            return UiDesignerApplyResult::AppliedPaint;
        }
        if(auto *grid = dynamic_cast<UiGridLayout *>(&ctrl)) {
            grid->SetDebug(on);
            return UiDesignerApplyResult::AppliedPaint;
        }
        return UiDesignerApplyResult::Rejected;
    }
    if(property == "role") {
        const UiRole role = ParseRole(value);
        if(auto *button = dynamic_cast<UiToolButton *>(&ctrl))
            button->SetCustomStyle(UiTheme::ResolveToolButton(role));
        else if(auto *button = dynamic_cast<UiButton *>(&ctrl))
            button->SetCustomStyle(UiTheme::ResolveButton(role));
        if(auto *panel = dynamic_cast<UiPanel *>(&ctrl)) panel->SetCustomStyle(UiTheme::ResolvePanel(role));
        if(auto *label = dynamic_cast<UiLabel *>(&ctrl)) label->SetCustomStyle(UiTheme::ResolveLabel(role));
        if(auto *group = dynamic_cast<UiGroupPanel *>(&ctrl)) group->SetCustomStyle(UiTheme::ResolveGroupPanel(role));
        ctrl.Refresh();
        return UiDesignerApplyResult::AppliedPaint;
    }
    if(property == "tooltip") {
        ctrl.Tip(AsString(value));
        ctrl.Refresh();
        return UiDesignerApplyResult::AppliedPaint;
    }
    if(property == "icon") {
        if(auto *button = dynamic_cast<UiButton *>(&ctrl)) {
            button->SetIcon(ResolveButtonIcon(AsString(value)));
            ctrl.RefreshLayout();
            return UiDesignerApplyResult::AppliedLocalLayout;
        }
        return UiDesignerApplyResult::Rejected;
    }
    if(property == "icon_width" || property == "icon_height") {
        if(auto *button = dynamic_cast<UiButton *>(&ctrl)) {
            Size icon_size = button->GetIconSize();
            const int v = max(0, (int)value);
            if(property == "icon_width")
                icon_size.cx = DPI(v);
            else
                icon_size.cy = DPI(v);
            button->SetIconSize(icon_size);
            ctrl.RefreshLayout();
            return UiDesignerApplyResult::AppliedLocalLayout;
        }
        return UiDesignerApplyResult::Rejected;
    }
    if(property == "icon_render_mode") {
        if(auto *button = dynamic_cast<UiButton *>(&ctrl)) {
            button->SetIconRenderMode(ParseIconRenderMode(value));
            ctrl.Refresh();
            return UiDesignerApplyResult::AppliedPaint;
        }
        return UiDesignerApplyResult::Rejected;
    }
    if(property == "icon_side") {
        if(auto *button = dynamic_cast<UiButton *>(&ctrl)) {
            button->SetIconSide(ParseSideAlignChoice(value));
            ctrl.RefreshLayout();
            return UiDesignerApplyResult::AppliedLocalLayout;
        }
        return UiDesignerApplyResult::Rejected;
    }
    if(property == "align_h" || property == "align_v") {
        if(auto *button = dynamic_cast<UiButton *>(&ctrl)) {
            const String current_h = property == "align_h"
                ? AsString(value)
                : String(button->GetStyle().align_h == UiAlign::LEFT ? "Left" :
                         button->GetStyle().align_h == UiAlign::RIGHT ? "Right" : "Center");
            const String current_v = property == "align_v"
                ? AsString(value)
                : String(button->GetStyle().align_v == UiAlign::TOP ? "Top" :
                         button->GetStyle().align_v == UiAlign::BOTTOM ? "Bottom" : "Center");
            const UiAlign align_h =
                current_h == "Left" ? UiAlign::LEFT :
                current_h == "Right" ? UiAlign::RIGHT : UiAlign::CENTER;
            const UiAlign align_v =
                current_v == "Top" ? UiAlign::TOP :
                current_v == "Bottom" ? UiAlign::BOTTOM : UiAlign::CENTER;
            button->SetAlign(align_h, align_v);
            ctrl.RefreshLayout();
            return UiDesignerApplyResult::AppliedLocalLayout;
        }
        return UiDesignerApplyResult::Rejected;
    }
    if(property == "content_gap") {
        if(auto *button = dynamic_cast<UiButton *>(&ctrl)) {
            button->SetContentGap(max(0, (int)value));
            ctrl.RefreshLayout();
            return UiDesignerApplyResult::AppliedLocalLayout;
        }
        return UiDesignerApplyResult::Rejected;
    }
    if(property == "content_inset_left" ||
       property == "content_inset_top" ||
       property == "content_inset_right" ||
       property == "content_inset_bottom") {
        if(auto *button = dynamic_cast<UiButton *>(&ctrl)) {
            Rect inset = button->GetContentInset();
            const int v = max(0, (int)value);
            if(property == "content_inset_left") inset.left = DPI(v);
            else if(property == "content_inset_top") inset.top = DPI(v);
            else if(property == "content_inset_right") inset.right = DPI(v);
            else inset.bottom = DPI(v);
            button->SetContentInset(inset);
            ctrl.RefreshLayout();
            return UiDesignerApplyResult::AppliedLocalLayout;
        }
        return UiDesignerApplyResult::Rejected;
    }
    if(property == "click_focus") {
        if(auto *button = dynamic_cast<UiButton *>(&ctrl)) {
            button->ClickFocus((bool)value);
            ctrl.Refresh();
            return UiDesignerApplyResult::AppliedPaint;
        }
        return UiDesignerApplyResult::Rejected;
    }
    if(property == "checkable") {
        if(auto *button = dynamic_cast<UiButton *>(&ctrl)) {
            button->SetCheckable((bool)value);
            ctrl.Refresh();
            return UiDesignerApplyResult::AppliedPaint;
        }
        return UiDesignerApplyResult::Rejected;
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
    if(property == "subtitle") {
        if(auto *card = dynamic_cast<UiTitleCard *>(&ctrl)) {
            card->SetSubTitle(AsString(value));
            ctrl.RefreshLayout();
            return UiDesignerApplyResult::AppliedLocalLayout;
        }
        return UiDesignerApplyResult::Rejected;
    }
    if(property == "copy") {
        if(auto *card = dynamic_cast<UiTitleCard *>(&ctrl)) {
            card->SetCopyText(AsString(value));
            ctrl.RefreshLayout();
            return UiDesignerApplyResult::AppliedLocalLayout;
        }
        return UiDesignerApplyResult::Rejected;
    }
    if(property == "text_align_h" || property == "text_align_v") {
        if(auto *card = dynamic_cast<UiTitleCard *>(&ctrl)) {
            const String current_h = property == "text_align_h"
                ? AsString(value)
                : String(card->GetStyle().text_align_h == UiAlign::LEFT ? "Left" :
                         card->GetStyle().text_align_h == UiAlign::RIGHT ? "Right" : "Center");
            const String current_v = property == "text_align_v"
                ? AsString(value)
                : String(card->GetStyle().text_align_v == UiAlign::TOP ? "Top" :
                         card->GetStyle().text_align_v == UiAlign::BOTTOM ? "Bottom" : "Center");
            card->SetTextAlign(ParseHorizontalAlignChoice(current_h), ParseVerticalAlignChoice(current_v));
            ctrl.RefreshLayout();
            return UiDesignerApplyResult::AppliedLocalLayout;
        }
        return UiDesignerApplyResult::Rejected;
    }
    if(property == "media_side") {
        if(auto *card = dynamic_cast<UiTitleCard *>(&ctrl)) {
            card->SetMediaSide(ParseSideAlignChoice(value));
            ctrl.RefreshLayout();
            return UiDesignerApplyResult::AppliedLocalLayout;
        }
        return UiDesignerApplyResult::Rejected;
    }
    if(property == "media_align_h" || property == "media_align_v") {
        if(auto *card = dynamic_cast<UiTitleCard *>(&ctrl)) {
            const String current_h = property == "media_align_h"
                ? AsString(value)
                : String(card->GetStyle().media_align_h == UiAlign::LEFT ? "Left" :
                         card->GetStyle().media_align_h == UiAlign::RIGHT ? "Right" : "Center");
            const String current_v = property == "media_align_v"
                ? AsString(value)
                : String(card->GetStyle().media_align_v == UiAlign::TOP ? "Top" :
                         card->GetStyle().media_align_v == UiAlign::BOTTOM ? "Bottom" : "Center");
            card->SetMediaAlign(ParseHorizontalAlignChoice(current_h), ParseVerticalAlignChoice(current_v));
            ctrl.RefreshLayout();
            return UiDesignerApplyResult::AppliedLocalLayout;
        }
        return UiDesignerApplyResult::Rejected;
    }
    if(property == "media_reserve") {
        if(auto *card = dynamic_cast<UiTitleCard *>(&ctrl)) {
            card->SetMediaReserve(max(0, (int)value));
            ctrl.RefreshLayout();
            return UiDesignerApplyResult::AppliedLocalLayout;
        }
        return UiDesignerApplyResult::Rejected;
    }
    if(property == "media_min") {
        if(auto *card = dynamic_cast<UiTitleCard *>(&ctrl)) {
            card->SetMediaMin(max(0, (int)value));
            ctrl.RefreshLayout();
            return UiDesignerApplyResult::AppliedLocalLayout;
        }
        return UiDesignerApplyResult::Rejected;
    }
    if(property == "media_gap") {
        if(auto *card = dynamic_cast<UiTitleCard *>(&ctrl)) {
            card->SetMediaGap(max(0, (int)value));
            ctrl.RefreshLayout();
            return UiDesignerApplyResult::AppliedLocalLayout;
        }
        return UiDesignerApplyResult::Rejected;
    }
    if(property == "media_auto_fit") {
        if(auto *card = dynamic_cast<UiTitleCard *>(&ctrl)) {
            card->SetMediaAutoFit((bool)value);
            ctrl.RefreshLayout();
            return UiDesignerApplyResult::AppliedPaint;
        }
        return UiDesignerApplyResult::Rejected;
    }
    if(property == "media_share_percent") {
        if(auto *card = dynamic_cast<UiTitleCard *>(&ctrl)) {
            card->SetMediaSharePercent(max(0, (int)value));
            ctrl.RefreshLayout();
            return UiDesignerApplyResult::AppliedLocalLayout;
        }
        return UiDesignerApplyResult::Rejected;
    }
    if(property == "content_inset") {
        if(auto *card = dynamic_cast<UiTitleCard *>(&ctrl)) {
            card->SetContentInset(max(0, (int)value));
            ctrl.RefreshLayout();
            return UiDesignerApplyResult::AppliedLocalLayout;
        }
        return UiDesignerApplyResult::Rejected;
    }
    if(property == "content_cell_gap") {
        if(auto *card = dynamic_cast<UiTitleCard *>(&ctrl)) {
            card->SetContentCellGap(max(0, (int)value));
            ctrl.RefreshLayout();
            return UiDesignerApplyResult::AppliedLocalLayout;
        }
        return UiDesignerApplyResult::Rejected;
    }
    if(property == "show_title_line") {
        if(auto *card = dynamic_cast<UiTitleCard *>(&ctrl)) {
            card->ShowTitleLine((bool)value);
            ctrl.Refresh();
            return UiDesignerApplyResult::AppliedPaint;
        }
        return UiDesignerApplyResult::Rejected;
    }
    if(property == "title_line_length" ||
       property == "title_line_thickness" ||
       property == "title_line_style") {
        if(auto *card = dynamic_cast<UiTitleCard *>(&ctrl)) {
            const UiSpan length = property == "title_line_length"
                ? ParseUiSpanChoice(value) : card->GetStyle().title_line_length;
            const int thickness = property == "title_line_thickness"
                ? max(0, (int)value) : card->GetStyle().title_line_thickness;
            const UiLineStyle style = property == "title_line_style"
                ? ParseUiLineStyleChoice(value) : card->GetStyle().title_line_style;
            card->SetTitleLine(length, thickness, style);
            ctrl.Refresh();
            return UiDesignerApplyResult::AppliedPaint;
        }
        return UiDesignerApplyResult::Rejected;
    }
    if(property == "show_card_line") {
        if(auto *card = dynamic_cast<UiTitleCard *>(&ctrl)) {
            card->ShowCardLine((bool)value);
            ctrl.Refresh();
            return UiDesignerApplyResult::AppliedPaint;
        }
        return UiDesignerApplyResult::Rejected;
    }
    if(property == "card_line_side") {
        if(auto *card = dynamic_cast<UiTitleCard *>(&ctrl)) {
            card->SetCardLineSide(ParseSideAlignChoice(value));
            ctrl.Refresh();
            return UiDesignerApplyResult::AppliedPaint;
        }
        return UiDesignerApplyResult::Rejected;
    }
    if(property == "card_line_length" ||
       property == "card_line_thickness" ||
       property == "card_line_gap") {
        if(auto *card = dynamic_cast<UiTitleCard *>(&ctrl)) {
            const UiSpan length = property == "card_line_length"
                ? ParseUiSpanChoice(value) : card->GetStyle().card_line_length;
            const int thickness = property == "card_line_thickness"
                ? max(0, (int)value) : card->GetStyle().card_line_thickness;
            const int gap = property == "card_line_gap"
                ? max(0, (int)value) : card->GetStyle().card_line_gap;
            card->SetCardLine(length, thickness);
            card->SetCardLineGap(gap);
            ctrl.Refresh();
            return UiDesignerApplyResult::AppliedPaint;
        }
        return UiDesignerApplyResult::Rejected;
    }
    if(property == "hover_enabled") {
        if(auto *card = dynamic_cast<UiTitleCard *>(&ctrl)) {
            card->EnableHover((bool)value);
            ctrl.Refresh();
            return UiDesignerApplyResult::AppliedPaint;
        }
        return UiDesignerApplyResult::Rejected;
    }
    if(property == "selectable") {
        if(auto *card = dynamic_cast<UiTitleCard *>(&ctrl)) {
            card->SetSelectable((bool)value);
            ctrl.Refresh();
            return UiDesignerApplyResult::AppliedPaint;
        }
        return UiDesignerApplyResult::Rejected;
    }
    if(property == "tooltip") {
        ctrl.Tip(AsString(value));
        ctrl.Refresh();
        return UiDesignerApplyResult::AppliedPaint;
    }
    if(property == "icon") {
        if(auto *button = dynamic_cast<UiButton *>(&ctrl)) {
            button->SetIcon(ResolveButtonIcon(AsString(value)));
            ctrl.RefreshLayout();
            return UiDesignerApplyResult::AppliedLocalLayout;
        }
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
        if(auto *button = dynamic_cast<UiButton *>(&ctrl)) {
            button->SetChecked((bool)value);
            ctrl.Refresh();
            return UiDesignerApplyResult::AppliedPaint;
        }
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
    return geometry_.Hit(p);
}

void UiDesignerPreviewCanvas::DestroyInstances()
{
    int destroyed = 0;
    for(UiDesignerPreviewInstance& instance : instances_)
        if(instance.control) {
            instance.control->Remove();
            destroyed++;
        }
    instances_.Clear();
    rects_.Clear();
    geometry_ = UiDesignerGeometrySnapshot();
    stats_.live_instance_destructions += destroyed;
}

void UiDesignerPreviewCanvas::ResetPerformance()
{
    stats_.Clear();
    resize_history_.Clear();
}

void UiDesignerPreviewCanvas::SetTransientVirtualSize(const Size& size)
{
    transient_virtual_size_ = Size(max(1, size.cx), max(1, size.cy));
    transient_virtual_size_set_ = true;
    if(capture_paused_)
        return;
    stats_.resize_events++;
    stats_.immediate_live_rect_updates++;
    stats_.transient_root_size_updates++;
}

void UiDesignerPreviewCanvas::ClearTransientVirtualSize()
{
    transient_virtual_size_set_ = false;
}

Size UiDesignerPreviewCanvas::GetEffectiveVirtualSize() const
{
    if(transient_virtual_size_set_)
        return transient_virtual_size_;
    return document_ ? document_->GetVirtualSize() : Size(0, 0);
}

double UiDesignerPreviewCanvas::GetGridLayoutDurationTotalMs() const
{
    return stats_.grid_layout_time_ms;
}

double UiDesignerPreviewCanvas::GetBoxLayoutDurationTotalMs() const
{
    return stats_.box_layout_time_ms;
}

void UiDesignerPreviewCanvas::RecordResizeSample(const UiDesignerResizeSample& sample)
{
    resize_history_.Add(sample);
}

Value UiDesignerPreviewCanvas::Effective(const UiDesignerNode& node,
                                         const String& property,
                                         const Value& fallback) const
{
    const Value canonical = node.GetProperty(property, fallback);
    return overlay_ ? overlay_->Resolve(node.id,
                                        UiDesignerTransientValueKind::NormalProperty,
                                        property, canonical) : canonical;
}

void UiDesignerPreviewCanvas::ApplyAllProperties(
    UiDesignerPreviewInstance& instance, const UiDesignerNode& node)
{
    const UiDesignerControlSpec* spec = catalog_ ? catalog_->Find(node.type) : nullptr;
    if(!spec || !instance.control)
        return;
    const UiDesignerThemeAdapter* adapter = UiDesignerGetThemeAdapter(*spec);
    UiDesignerNode effective = node;
    if(overlay_) {
        const Value canonical_role = node.GetProperty("role", "Standard");
        const Value transient_role = overlay_->Resolve(
            node.id, UiDesignerTransientValueKind::NormalProperty,
            "role", canonical_role);
        if(transient_role != canonical_role)
            effective.SetProperty("role", transient_role);
    }
    if(adapter)
        adapter->ApplyPreviewStyle(*instance.control, effective, *spec, overlay_);
    for(const UiDesignerPropertySpec& property : spec->properties)
        if(property.id == "role" && adapter)
            continue;
        else
        UiDesignerPreviewFactory::Apply(*instance.control, *spec,
            property.id, Effective(effective, property.id, property.default_value));
}

static void ConfigureBoxSpacer(UiBoxLayout& box,
                               UiBoxLayout::ItemRef item,
                               const UiDesignerNode& node)
{
    if(!node.GetProperty("layout_break", false)) {
        const bool horizontal = box.GetDirection() == UiDirection::H;
        const String main_mode = node.GetProperty(
            horizontal ? "width_mode" : "height_mode",
            node.GetProperty(horizontal ? "h_sizing" : "v_sizing", "Fit"));
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
    if(node.type == "UiTabPage") {
        if(auto *tab = dynamic_cast<UiTab *>(parent_instance.control.Get())) {
            instance.control = MakeOne<ParentCtrl>();
            tab->Add(*instance.control, node.GetProperty("title", node.name));
            tab->EnableTab(tab->GetCount() - 1, node.GetProperty("enabled", true));
        }
        return;
    }
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
                               int& layout_item_index,
                               Size catalog_size = Size(160, 32))
{
    if(adapter == "single") {
        parent.Add(child.SizePos());
        return;
    }
    if(auto *absolute = dynamic_cast<UiAbsoluteLayout *>(&parent)) {
        layout_item_index = absolute->GetItemCount();
        absolute->Add(child, 0, 0, 0, 0);
    }
    else if(auto *box = dynamic_cast<UiBoxLayout *>(&parent)) {
        layout_item_index = box->GetItemCount();
        box->Add(child);
    }
    else if(auto *grid = dynamic_cast<UiGridLayout *>(&parent)) {
        layout_item_index = grid->GetItemCount();
        grid->Add(child, node.GetProperty("grid_row", 0),
                  node.GetProperty("grid_column", 0), false, false);
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

static UiGridLayout::Align ParseGridAlign(const String& align)
{
    if(align == "Center")
        return UiGridLayout::Align::Center;
    if(align == "Right" || align == "Bottom" || align == "End")
        return UiGridLayout::Align::End;
    if(align == "Stretch" || align == "Fill")
        return UiGridLayout::Align::Stretch;
    return UiGridLayout::Align::Start;
}

static bool IsManagedLayoutProperty(const String& property)
{
    return property == "x" || property == "y" ||
           property == "width" || property == "height" ||
           property == "width_mode" || property == "height_mode" ||
           property == "fixed_width" || property == "fixed_height" ||
           property == "min_width" || property == "min_height" ||
           property == "max_width" || property == "max_height" ||
           property == "cell_align_x" || property == "cell_align_y" ||
           property == "grid_row" || property == "grid_column";
}

void UiDesignerPreviewCanvas::UpdateManagedLayoutItem(
    UiDesignerPreviewInstance& instance, const UiDesignerNode& node)
{
    if(!instance.control || instance.layout_item_index < 0 || !document_ || !catalog_)
        return;

    const UiDesignerNode* parent_node = document_->Find(node.parent);
    const UiDesignerControlSpec* parent_spec = parent_node ? catalog_->Find(parent_node->type) : nullptr;
    if(!parent_spec)
        return;

    if(auto *absolute = dynamic_cast<UiAbsoluteLayout *>(FindRuntime(instance.runtime_parent))) {
        absolute->SetItemRect(
            instance.layout_item_index,
            (int)Effective(node, "x", 20),
            (int)Effective(node, "y", 20),
            max(0, (int)Effective(node, "width", 160)),
            max(0, (int)Effective(node, "height", 32)));
        stats_.layout_item_updates++;
        stats_.absolute_layout_updates++;
        return;
    }

    if(auto *box = dynamic_cast<UiBoxLayout *>(FindRuntime(instance.runtime_parent))) {
        const bool horizontal = box->GetDirection() == UiDirection::H;
        const Size natural = max(box->GetMinSize(), instance.control->GetMinSize());
        const UiDesignerBoxSizing sizing = UiDesignerResolveBoxSizing(
            node, horizontal, max(1, horizontal ? natural.cx : natural.cy),
            max(1, horizontal ? natural.cy : natural.cx));
        UiBoxLayout::PauseScope pause(*box);
        UiBoxLayout::ItemRef item = box->ItemAt(instance.layout_item_index);

        if(sizing.main.mode == "Expand")
            item.Expand(max(1, sizing.weight));
        else if(sizing.main.mode == "Fixed")
            item.Fixed(max(1, sizing.main.fixed > 0 ? sizing.main.fixed : sizing.main.natural));
        else
            item.Fit();
        item.MinMaxMain(max(sizing.main.min, sizing.main.mode == "Fixed"
                                      ? max(1, sizing.main.fixed > 0 ? sizing.main.fixed : sizing.main.natural)
                                      : sizing.main.natural),
                        sizing.main.max > 0 ? max(sizing.main.max,
                                                  max(sizing.main.min, sizing.main.natural)) : INT_MAX);

        if(sizing.cross.mode == "Expand") {
            item.MinMaxCross(max(sizing.cross.min, sizing.cross.natural),
                             sizing.cross.max > 0 ? max(sizing.cross.max,
                                                        max(sizing.cross.min, sizing.cross.natural))
                                                  : INT_MAX)
                .AlignSelf(UiCrossAlign::Stretch);
        }
        else {
            const int extent = max(sizing.cross.min,
                                   sizing.cross.mode == "Fixed"
                                       ? max(1, sizing.cross.fixed > 0 ? sizing.cross.fixed : sizing.cross.natural)
                                       : sizing.cross.natural);
            item.MinMaxCross(extent, sizing.cross.max > 0 ? max(sizing.cross.max, extent) : extent);
            item.AlignSelf(UiDesignerResolveBoxAlign(sizing.cross_align));
        }
        stats_.layout_item_updates++;
        return;
    }

    if(auto *grid = dynamic_cast<UiGridLayout *>(FindRuntime(instance.runtime_parent))) {
        const UiDesignerGridSizing sizing = UiDesignerResolveGridSizing(node);
        const Size natural = max(instance.control->GetMinSize(), Size(1, 1));
        const Size fixed = Size(
            sizing.fixed.cx > 0 ? sizing.fixed.cx : natural.cx,
            sizing.fixed.cy > 0 ? sizing.fixed.cy : natural.cy);
        grid->PauseLayout();
        grid->SetItem(instance.layout_item_index,
                      max(0, (int)node.GetProperty("grid_row", 0)),
                      max(0, (int)node.GetProperty("grid_column", 0)),
                      sizing.scale_x, sizing.scale_y, fixed);
        grid->SetItemAlign(instance.layout_item_index, ParseGridAlign(sizing.align_x),
                           ParseGridAlign(sizing.align_y));
        grid->SetItemMinSize(instance.layout_item_index, Size(
            max(sizing.min.cx, natural.cx), max(sizing.min.cy, natural.cy)));
        grid->SetItemMaxSize(instance.layout_item_index, Size(
            sizing.max.cx > 0 ? max(sizing.max.cx, max(sizing.min.cx, natural.cx)) : INT_MAX,
            sizing.max.cy > 0 ? max(sizing.max.cy, max(sizing.min.cy, natural.cy)) : INT_MAX));
        grid->ResumeLayout(true);
        stats_.layout_item_updates++;
        return;
    }
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
        if(instance.control) {
            ParentCtrl *host = dynamic_cast<ParentCtrl *>(instance.control.Get());
            for(UiDesignerNodeId child : node->children)
                BuildNode(child, host ? *host : fallback_parent, depth + 1, node_id);
        }
        return;
    }

    instance.control = UiDesignerPreviewFactory::Create(*spec);
    if(!instance.control)
        return;
    stats_.live_instance_creations++;
    UiDesignerPreviewFactory::Initialize(*instance.control, *spec);
    if(parent_instance && parent_instance->control)
        AttachRuntimeChild(*parent_instance->control, *instance.control,
                           *node,
                           parent_spec ? parent_spec->child_adapter_id : "add",
                           instance.layout_item_index, spec->default_size);
    else
        fallback_parent.Add(*instance.control);
    ApplyAllProperties(instance, *node);
    if(instance.layout_item_index >= 0)
        UpdateManagedLayoutItem(instance, *node);

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
    ApplyActiveTabProjection();
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
    if(const UiDesignerNode *rebuilt = document_->Find(root))
        if(rebuilt->type == "UiTab")
            ApplyActiveTabProjection();
    stats_.subtree_rebuilds++;
    Layout();
    Refresh();
    return true;
}

UiDesignerApplyResult UiDesignerPreviewCanvas::ApplyProperty(
    UiDesignerNodeId node_id, const String& property, const Value& value,
    UiDesignerTransientValueKind kind)
{
    const int q = FindInstance(node_id);
    const UiDesignerNode* node = document_ ? document_->Find(node_id) : nullptr;
    const UiDesignerControlSpec* spec = node && catalog_ ? catalog_->Find(node->type) : nullptr;
    if(q < 0 || !spec) {
        stats_.rejected++;
        return UiDesignerApplyResult::Rejected;
    }

    if(kind == UiDesignerTransientValueKind::NormalProperty &&
       property == "active_page" && node->type == "UiTab") {
        UiTab *tab = dynamic_cast<UiTab *>(instances_[q].control.Get());
        if(tab) {
            for(int i = 0; i < node->children.GetCount(); i++)
                if(node->children[i] == (UiDesignerNodeId)value) {
                    const UiDesignerNode *page = document_->Find(node->children[i]);
                    if(page && page->type == "UiTabPage" && i < tab->GetCount()) {
                        tab->SetActiveTab(i);
                        stats_.live_applies++;
                        Refresh();
                        return UiDesignerApplyResult::AppliedLocalLayout;
                    }
                    break;
                }
        }
        stats_.rejected++;
        return UiDesignerApplyResult::Rejected;
    }

    if(kind == UiDesignerTransientValueKind::ThemeOverride) {
        const UiDesignerThemeOverrideSpec* override_spec = spec->FindThemeOverride(property);
        const UiDesignerThemeAdapter* adapter = UiDesignerGetThemeAdapter(*spec);
        if(!override_spec || !adapter || !adapter->Supports(spec->runtime_kind) ||
           !adapter->HasField(override_spec->adapter_field_id)) {
            stats_.rejected++;
            return UiDesignerApplyResult::Rejected;
        }
        UiDesignerPreviewInstance& instance = instances_[q];
        if(!instance.control) {
            stats_.rejected++;
            return UiDesignerApplyResult::Rejected;
        }
        adapter->ApplyPreviewStyle(*instance.control, *node, *spec, overlay_);
        const UiDesignerApplyResult result = adapter->FieldAffectsLayout(
            override_spec->adapter_field_id)
                ? UiDesignerApplyResult::AppliedAncestorLayout
                : UiDesignerApplyResult::AppliedPaint;
        if(result == UiDesignerApplyResult::AppliedAncestorLayout) {
            stats_.ancestor_layouts++;
            Layout();
        }
        else
            stats_.paint_updates++;
        stats_.live_applies++;
        Refresh();
        return result;
    }

    const UiDesignerThemeAdapter* adapter = UiDesignerGetThemeAdapter(*spec);
    if(adapter && property == "role") {
        UiDesignerPreviewInstance& instance = instances_[q];
        if(!instance.control) {
            stats_.rejected++;
            return UiDesignerApplyResult::Rejected;
        }
        ApplyAllProperties(instance, *node);
        stats_.paint_updates++;
        stats_.live_applies++;
        Layout();
        Refresh();
        return UiDesignerApplyResult::AppliedPaint;
    }
    UiDesignerApplyResult result;
    if(instances_[q].semantic) {
        result = UiDesignerApplyResult::RequiresSubtreeRebuild;
        const UiDesignerNodeId parent = node->parent;
        if(!RebuildSubtree(parent))
            RebuildDocument();
    }
    else if(instances_[q].control) {
        if(property == "direction") {
            if(auto *box = dynamic_cast<UiBoxLayout *>(instances_[q].control.Get())) {
                const bool horizontal = AsString(value) == "H";
                box->SetDirection(horizontal ? UiDirection::H : UiDirection::V);
                stats_.ancestor_layouts++;
                stats_.live_applies++;
                Layout();
                Refresh();
                return UiDesignerApplyResult::AppliedAncestorLayout;
            }
        }
        else if(property == "rows" || property == "columns") {
            if(auto *grid = dynamic_cast<UiGridLayout *>(instances_[q].control.Get())) {
                const int rows = max(1, (int)node->GetProperty("rows", 1));
                const int cols = max(1, (int)node->GetProperty("columns", 1));
                grid->SetGridSize(cols, rows);
                stats_.ancestor_layouts++;
                stats_.live_applies++;
                Layout();
                Refresh();
                return UiDesignerApplyResult::AppliedAncestorLayout;
            }
        }
        else if(property == "min_cell_width" || property == "min_cell_height") {
            if(auto *grid = dynamic_cast<UiGridLayout *>(instances_[q].control.Get())) {
                const int width = max(0, (int)node->GetProperty("min_cell_width", 10));
                const int height = max(0, (int)node->GetProperty("min_cell_height", 10));
                grid->SetMinCellSize(Size(DPI(width), DPI(height)));
                stats_.ancestor_layouts++;
                stats_.live_applies++;
                Layout();
                Refresh();
                return UiDesignerApplyResult::AppliedAncestorLayout;
            }
        }

        result = UiDesignerPreviewFactory::Apply(*instances_[q].control,
                                                  *spec, property, value);
        if(result == UiDesignerApplyResult::Rejected && IsManagedLayoutProperty(property)) {
            UpdateManagedLayoutItem(instances_[q], *node);
            result = UiDesignerApplyResult::AppliedAncestorLayout;
            stats_.ancestor_layouts++;
        }
        switch(result) {
        case UiDesignerApplyResult::AppliedPaint: stats_.paint_updates++; break;
        case UiDesignerApplyResult::AppliedLocalLayout:
            stats_.local_layouts++; Layout(); break;
        case UiDesignerApplyResult::AppliedAncestorLayout:
            Layout(); break;
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
        UiDesignerNodeId tab = 0;
        bool tab_only = true;
        for(const UiDesignerStructureChange& change : changes.structure) {
            const UiDesignerNodeId parent = change.new_parent ? change.new_parent : change.old_parent;
            const UiDesignerNode *page = document_->Find(change.node);
            const UiDesignerNode *owner = document_->Find(parent);
            if(!owner || owner->type != "UiTab" ||
               (page && page->type != "UiTabPage") || (tab && tab != parent)) {
                tab_only = false;
                break;
            }
            tab = parent;
        }
        if(tab_only && tab && RebuildSubtree(tab))
            return;
        // Managed Box/Grid controls retain internal item references. A complete
        // rebuild is the safe structural boundary until those items have an
        // explicit removal API; correctness beats a stale partial tree here.
        RebuildDocument();
        return;
    }
    if(changes.virtual_size_changed) {
        Layout();
        Refresh();
    }
    for(const UiDesignerPropertyChange& change : changes.properties)
        ApplyProperty(change.node, change.property, change.new_value,
                      change.kind == UiDesignerPropertyChangeKind::ThemeOverride
                          ? UiDesignerTransientValueKind::ThemeOverride
                          : UiDesignerTransientValueKind::NormalProperty);
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
    if(node->parent == document_->GetRootId() || !managed) {
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
    else if(instance.runtime_parent) {
        Ctrl* parent = FindRuntime(instance.runtime_parent);
        if(auto *box = dynamic_cast<UiBoxLayout *>(parent))
            instance.control->SetRect(box->GetItemRect(instance.layout_item_index));
        else if(auto *grid = dynamic_cast<UiGridLayout *>(parent))
            instance.control->SetRect(grid->GetItemRect(instance.layout_item_index));
    }

    // Parent layout has already assigned managed children. Layout this
    // control only after its own rectangle is authoritative, then recurse.
    const bool measure = detailed_timing_enabled_ && !capture_paused_;
    const bool is_grid = dynamic_cast<UiGridLayout *>(instance.control.Get());
    const bool is_box = dynamic_cast<UiBoxLayout *>(instance.control.Get());
    const int64 control_layout_start = (measure && (is_grid || is_box)) ? usecs() : 0;
    const Rect parent_assigned_rect = instance.control->GetRect();
    instance.control->Layout();
    if(instance.runtime_parent && managed && adapter != "absolute")
        instance.control->SetRect(parent_assigned_rect);
    if(control_layout_start) {
        const double elapsed = (double)usecs(control_layout_start) / 1000.0;
        if(is_grid) {
            if(stats_.grid_layout_time_ms < 0) stats_.grid_layout_time_ms = 0;
            stats_.grid_layout_time_ms += elapsed;
        }
        if(is_box) {
            if(stats_.box_layout_time_ms < 0) stats_.box_layout_time_ms = 0;
            stats_.box_layout_time_ms += elapsed;
        }
    }

    Point origin(0, 0);
    if(instance.runtime_parent) {
        const int p = rects_.Find(instance.runtime_parent);
        if(p >= 0)
            origin = rects_[p].TopLeft();
    }
    rects_.GetAdd(node_id) = instance.control->GetRect().Offseted(origin);

    int child_ordinal = 0;
    for(UiDesignerNodeId child : node->children)
        LayoutNode(child, child_ordinal++, depth + 1);
}

void UiDesignerPreviewCanvas::Layout()
{
    if(!document_)
        return;
    const bool measure = detailed_timing_enabled_ && !capture_paused_;
    const int64 layout_start = measure ? usecs() : 0;
    const int64 geometry_walk_start = measure ? usecs() : 0;
    const UiDesignerNode* root = document_->Find(document_->GetRootId());
    if(!root)
        return;
    if(!capture_paused_) {
        stats_.layout_count++;
        stats_.preview_layout_calls++;
        stats_.full_geometry_walks++;
    }
    rects_.Clear();
    // Window is an implicit document host, not another runtime Ctrl. Its
    // rectangle is nevertheless real so hierarchy selection and resize
    // handles describe the same bounded form the user sees.
    const Size virtual_size = GetEffectiveVirtualSize();
    rects_.GetAdd(root->id) = RectC(0, 0, virtual_size.cx, virtual_size.cy);
    int ordinal = 0;
    for(UiDesignerNodeId child : root->children)
        LayoutNode(child, ordinal++, 0);
    const double geometry_walk_ms = measure ? (double)usecs(geometry_walk_start) / 1000.0 : -1;

    const int64 snapshot_start = measure ? usecs() : 0;
    UiDesignerGeometrySnapshotBuilder snapshot;
    UiDesignerGeometryRecord root_record;
    root_record.node = root->id;
    root_record.rect = rects_.Get(root->id);
    root_record.body = root_record.rect;
    root_record.selectable = true;
    root_record.drop_target = true;
    root_record.cue_kind = UiDesignerCueKind::ContainerBounds;
    snapshot.Add(pick(root_record));
    {
        UiDesignerDropRegion region;
        region.owner = root->id;
        region.kind = UiDesignerDropRegionKind::WindowContent;
        region.rect = root_record.rect;
        region.visual_rect = root_record.rect;
        region.depth = 0;
        region.paint_order = 0;
        region.label = "Window";
        snapshot.AddRegion(pick(region));
    }
    int order = 0;
    for(const UiDesignerPreviewInstance& instance : instances_) {
        const UiDesignerNode* node = document_->Find(instance.node);
        if(!node)
            continue;
        UiDesignerGeometryRecord record;
        record.node = node->id;
        record.parent = node->parent;
        record.rect = GetNodeRect(node->id);
        for(UiDesignerNodeId parent = node->parent; parent; ) {
            const UiDesignerNode* p = document_->Find(parent);
            if(!p)
                break;
            record.depth++;
            parent = p->parent;
        }
        record.order = order++;
        record.selectable = true;
        record.drop_target = node->id == document_->GetRootId() ||
            node->type == "UiBoxLayout" || node->type == "UiGridLayout" ||
            node->type == "UiPanel";
        const UiDesignerControlSpec* spec = catalog_ ? catalog_->Find(node->type) : nullptr;
        record.cue_kind = spec
            ? ResolveCueKind(*spec, *node)
            : UiDesignerCueKind::ControlBounds;
        record.debug_layout = node->GetProperty("debug_layout", false);
        if(node->type == "UiBoxLayout" || node->type == "UiGridLayout")
            record.debug_color = UiDesignerStableLayoutColor(node->id, record.depth);
        record.inset = max(0, (int)node->GetProperty("inset", 0));
        record.gap = max(0, (int)node->GetProperty("gap", 0));
        record.body = record.inset ? record.rect.Deflated(DPI(record.inset)) : record.rect;
        const int q = FindInstance(node->id);
        if(q >= 0 && instances_[q].control &&
           (node->type == "UiBoxLayout" || node->type == "UiGridLayout")) {
            if(auto *box = dynamic_cast<UiBoxLayout *>(instances_[q].control.Get()))
                for(int i = 0; i < box->GetItemCount(); i++)
                    record.item_rects.Add(box->GetItemRect(i).Offseted(record.rect.TopLeft()));
            if(auto *grid = dynamic_cast<UiGridLayout *>(instances_[q].control.Get())) {
                grid->GetCellRects(record.cell_rects);
                stats_.cached_grid_geometry_reads++;
                for(Rect& cell : record.cell_rects)
                    cell = cell.Offseted(record.rect.TopLeft());
                for(int i = 0; i < grid->GetItemCount(); i++)
                    record.item_rects.Add(grid->GetItemRect(i).Offseted(record.rect.TopLeft()));
                stats_.cached_grid_geometry_publications++;
            }
            for(int i = 1; i < record.item_rects.GetCount(); i++) {
                Rect a = record.item_rects[i - 1], b = record.item_rects[i];
                if(a.right < b.left)
                    record.gap_rects.Add(RectC(a.right, max(a.top, b.top),
                        b.left - a.right, max(0, min(a.bottom, b.bottom) - max(a.top, b.top))));
                else if(a.bottom < b.top)
                    record.gap_rects.Add(RectC(max(a.left, b.left), a.bottom,
                        max(0, min(a.right, b.right) - max(a.left, b.left)), b.top - a.bottom));
            }
            if(record.gap_rects.IsEmpty() && record.gap > 0 && record.item_rects.GetCount() > 1) {
                Rect a = record.item_rects[0], b = record.item_rects[1];
                if(a.CenterPoint().x <= b.CenterPoint().x)
                    record.gap_rects.Add(RectC((a.right + b.left) / 2, a.top, 1, max(1, a.Height())));
                else
                    record.gap_rects.Add(RectC(a.left, (a.bottom + b.top) / 2, max(1, a.Width()), 1));
            }
        }
        if(record.inset > 0) {
            record.inset_rects.Add(RectC(record.rect.left, record.rect.top,
                                         record.rect.Width(), DPI(record.inset)));
            record.inset_rects.Add(RectC(record.rect.left, record.body.bottom,
                                         record.rect.Width(), DPI(record.inset)));
            record.inset_rects.Add(RectC(record.rect.left, record.body.top,
                                         DPI(record.inset), record.body.Height()));
            record.inset_rects.Add(RectC(record.body.right, record.body.top,
                                         DPI(record.inset), record.body.Height()));
        }
        AddLayoutDropRegions(snapshot, *document_, catalog_, *node, record,
                             q >= 0 ? &instances_[q] : nullptr);
        snapshot.Add(pick(record));
    }
    geometry_ = snapshot.Publish();
    if(!capture_paused_) {
        stats_.snapshot_publications++;
        stats_.drop_region_publications++;
    }
    const double snapshot_ms = measure ? (double)usecs(snapshot_start) / 1000.0 : -1;
    int grid_builds = 0;
    int grid_queries = 0;
    int grid_layout_calls = 0;
    int box_layout_calls = 0;
    for(const UiDesignerPreviewInstance& instance : instances_)
        if(const UiGridLayout *grid = instance.control
            ? dynamic_cast<const UiGridLayout *>(instance.control.Get()) : nullptr) {
            grid_builds += grid->GetResolvedCellGeometryBuildCount();
            grid_queries += grid->GetResolvedCellGeometryQueryCount();
            grid_layout_calls += grid->GetLayoutCallCount();
        }
        else if(const UiBoxLayout *box = instance.control
            ? dynamic_cast<const UiBoxLayout *>(instance.control.Get()) : nullptr) {
            box_layout_calls += box->GetLayoutCallCount();
        }
    if(!capture_paused_) {
        stats_.track_size_calculations = grid_builds;
        stats_.cached_grid_geometry_reads = grid_queries;
        stats_.grid_layout_passes = grid_layout_calls;
        stats_.box_layout_passes = box_layout_calls;
        stats_.geometry_walk_time_ms = geometry_walk_ms;
        stats_.snapshot_time_ms = snapshot_ms;
        stats_.layout_time_ms = measure ? (double)usecs(layout_start) / 1000.0 : -1;
    }
}

void UiDesignerPreviewCanvas::ApplyActiveTabProjection()
{
    if(!document_)
        return;
    for(UiDesignerPreviewInstance& instance : instances_) {
        if(instance.type != "UiTab" || !instance.control)
            continue;
        UiTab *tab = dynamic_cast<UiTab *>(instance.control.Get());
        const UiDesignerNode *node = document_->Find(instance.node);
        if(!tab || !node)
            continue;
        const UiDesignerNodeId active = node->GetProperty("active_page", (UiDesignerNodeId)0);
        for(int i = 0; i < node->children.GetCount(); i++)
            if(node->children[i] == active) {
                const UiDesignerNode *page = document_->Find(active);
                if(page && page->type == "UiTabPage" && i < tab->GetCount())
                    tab->SetActiveTab(i);
                break;
            }
    }
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
    const bool measure = detailed_timing_enabled_ && !capture_paused_;
    const int64 paint_start = measure ? usecs() : 0;
    stats_.full_canvas_repaints++;
    w.DrawRect(GetSize(), SColorPaper());
    if(document_)
        for(const UiDesignerPreviewInstance& instance : instances_)
            if(instance.semantic)
                if(const UiDesignerNode* node = document_->Find(instance.node))
                    PaintSemantic(w, instance, *node);
    stats_.canvas_paint_time_ms = measure ? (double)usecs(paint_start) / 1000.0 : -1;
}

}
