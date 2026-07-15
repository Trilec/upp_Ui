#include "UiDesignerWidgets.h"
#include <Ui/UiIcons.h>

namespace Upp {

Image UiDesignerResolveCatalogIcon(const String& key)
{
    if(key == "layouts") return ICON_DESIGN_LAYOUTS_CATEGORY_48();
    if(key == "containers") return ICON_DESIGN_TAB_GROUP_48();
    if(key == "composites") return ICON_DESIGN_DYNAMIC_FORM_48();
    if(key == "presets") return ICON_DESIGN_DASHBOARD_EDIT_48();
    if(key == "inspector") return ICON_DESIGN_TUNE_48();
    if(key == "hierarchy") return ICON_DESIGN_ACCOUNT_TREE_48();
    if(key == "code") return ICON_DESIGN_CODE_BLOCKS_48();
    if(key == "theme") return ICON_DESIGN_FORMAT_PAINT_48();
    return ICON_DESIGN_WIDGETS_48();
}

static void PutCtrl(Ctrl& c, int x, int y, int cx, int cy)
{
    c.SetRect(x, y, max(0, cx), max(0, cy));
}

UiDesignerPillBar::UiDesignerPillBar()
{
    SetCustomStyle(UiDesignerPillStyle());
}

UiDesignerPillBar& UiDesignerPillBar::SetInset(int inset)
{
    inset_ = max(0, inset);
    Layout();
    return *this;
}

UiDesignerPillBar& UiDesignerPillBar::Vertical(bool on)
{
    vertical_ = on;
    Layout();
    return *this;
}

UiDesignerPillBar& UiDesignerPillBar::ApplyTheme(
    const UiDesignerThemeSnapshot& theme)
{
    SetCustomStyle(UiDesignerPillStyle(UiRole::Subtle, theme));
    return *this;
}

UiDesignerPillBar& UiDesignerPillBar::ShowAuxiliary(bool on)
{
    show_auxiliary_ = on;
    for(const Item& item : items_)
        if(item.ctrl)
            item.ctrl->Show(item.section || show_auxiliary_);
    Layout();
    return *this;
}

UiDesignerPillBar& UiDesignerPillBar::AddSection(
    const String& tip, const Image& icon)
{
    const int section_index = owned_buttons_.GetCount();
    UiToolButton& button = owned_buttons_.Add();
    button.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
    button.SetIcon(icon).SetIconSize(DPI(16), DPI(16));
    button.Tip(tip);
    button.WhenAction = [=] { WhenSelect(section_index); };
    Add(button);

    Item& item = items_.Add();
    item.ctrl = &button;
    item.extent = DPI(32);
    item.section = true;
    return *this;
}

UiDesignerPillBar& UiDesignerPillBar::AddControl(Ctrl& ctrl, int extent)
{
    Add(ctrl);
    Item& item = items_.Add();
    item.ctrl = &ctrl;
    item.extent = max(DPI(24), extent);
    item.section = false;
    return *this;
}

int UiDesignerPillBar::GetSectionCount() const
{
    int count = 0;
    for(const Item& item : items_)
        if(item.section)
            count++;
    return count;
}

void UiDesignerPillBar::Layout()
{
    const int cx = GetSize().cx;
    const int cy = GetSize().cy;
    int cursor = inset_;

    for(const Item& item : items_) {
        if(!item.ctrl || (!item.section && !show_auxiliary_)) {
            if(item.ctrl)
                item.ctrl->Hide();
            continue;
        }
        item.ctrl->Show();
        if(vertical_) {
            const int w = max(DPI(28), cx - DPI(12));
            PutCtrl(*item.ctrl, (cx - w) / 2, cursor, w, item.extent);
            cursor += item.extent + DPI(6);
        }
        else {
            const int h = max(DPI(24), cy - DPI(10));
            PutCtrl(*item.ctrl, cursor, (cy - h) / 2, item.extent, h);
            cursor += item.extent + DPI(6);
        }
    }
}

UiDesignerSideColumn::UiDesignerSideColumn()
{
    tools_.SetInset(UiDesignerStyleMetrics::LeftPillInset());
    content_surface_.SetCustomStyle(UiDesignerSurfaceStyle());
    content_surface_.Add(pages_.SizePos());

    close_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
    close_.SetIcon(ICON_DESIGN_LEFT_PANEL_CLOSE_48())
          .SetIconSize(DPI(16), DPI(16));
    close_.Tip("Collapse panel");
    close_.WhenAction = [=] { Close(); };

    expand_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
    expand_.SetIcon(ICON_DESIGN_UNFOLD_MORE_48())
           .SetIconSize(DPI(16), DPI(16));
    expand_.Tip("Cycle panel width");
    expand_.WhenAction = [=] { Cycle(); };

    tools_.AddControl(close_, DPI(32));
    tools_.AddControl(expand_, DPI(32));
    tools_.WhenSelect = [=](int i) { Select(i); };

    Add(tools_);
    Add(content_surface_);
}

UiDesignerSideColumn& UiDesignerSideColumn::RightColumn(bool on)
{
    right_ = on;
    tools_.SetInset(on ? UiDesignerStyleMetrics::RightPillInset()
                       : UiDesignerStyleMetrics::LeftPillInset());
    close_.SetIcon(on ? ICON_DESIGN_RIGHT_PANEL_CLOSE_48()
                      : ICON_DESIGN_LEFT_PANEL_CLOSE_48());
    return *this;
}

UiDesignerSideColumn& UiDesignerSideColumn::AddSection(
    const String& tip, const Image& icon, Ctrl& content)
{
    tools_.AddSection(tip, icon);
    pages_.Add(content, tip);
    if(pages_.GetCount() == 1)
        pages_.SetActivePage(0);
    return *this;
}

UiDesignerSideColumn& UiDesignerSideColumn::ApplyTheme(
    const UiDesignerThemeSnapshot& theme)
{
    tools_.ApplyTheme(theme);
    content_surface_.SetCustomStyle(
        UiDesignerSurfaceStyle(UiRole::Subtle, theme));
    Refresh();
    return *this;
}

void UiDesignerSideColumn::SetPaneWidth(UiDesignerPaneWidth width)
{
    if(width_ == width)
        return;
    width_ = width;
    Layout();
    WhenWidthChanged();
}

int UiDesignerSideColumn::GetDesiredWidth() const
{
    if(width_ == PANE_CLOSED)
        return UiDesignerStyleMetrics::RailWidth();
    switch(width_) {
    case PANE_NORMAL: return UiDesignerStyleMetrics::PanelNormalWidth();
    case PANE_MEDIUM: return UiDesignerStyleMetrics::PanelMediumWidth();
    case PANE_WIDE:   return UiDesignerStyleMetrics::PanelWideWidth();
    default:          return UiDesignerStyleMetrics::PanelNormalWidth();
    }
}

void UiDesignerSideColumn::SetActiveSection(int index)
{
    Select(index);
}

void UiDesignerSideColumn::Select(int index)
{
    if(index < 0 || index >= pages_.GetCount())
        return;
    active_section_ = index;
    pages_.SetActivePage(index);
    if(width_ == PANE_CLOSED)
        width_ = PANE_NORMAL;
    Layout();
    WhenSectionChanged(index);
    WhenWidthChanged();
}

void UiDesignerSideColumn::Cycle()
{
    switch(width_) {
    case PANE_CLOSED: width_ = PANE_NORMAL; break;
    case PANE_NORMAL: width_ = PANE_MEDIUM; break;
    case PANE_MEDIUM: width_ = PANE_WIDE; break;
    case PANE_WIDE: width_ = PANE_NORMAL; break;
    }
    Layout();
    WhenWidthChanged();
}

void UiDesignerSideColumn::Close()
{
    width_ = PANE_CLOSED;
    Layout();
    WhenWidthChanged();
}

void UiDesignerSideColumn::Layout()
{
    const int w = GetSize().cx;
    const int h = GetSize().cy;
    if(width_ == PANE_CLOSED) {
        tools_.Vertical(true).ShowAuxiliary(false).SetInset(DPI(9));
        PutCtrl(tools_, 0, 0, w, h);
        content_surface_.Hide();
    }
    else {
        tools_.Vertical(false).ShowAuxiliary(true)
              .SetInset(right_ ? UiDesignerStyleMetrics::RightPillInset()
                               : UiDesignerStyleMetrics::LeftPillInset());
        const int pill_h = UiDesignerStyleMetrics::DesignerToolbarHeight();
        PutCtrl(tools_, 0, 0, w, pill_h);
        PutCtrl(content_surface_, 0,
                pill_h + UiDesignerStyleMetrics::Gap(), w,
                max(0, h - pill_h - UiDesignerStyleMetrics::Gap()));
        content_surface_.Show();
    }
}

UiDesignerCatalogList::UiDesignerCatalogList()
{
    BackPaint();
}

void UiDesignerCatalogList::SetCatalog(const UiDesignerCatalog *catalog)
{
    catalog_ = catalog;
    Refresh();
}

void UiDesignerCatalogList::SetCategory(const String& category)
{
    category_ = category;
    presets_ = false;
    scroll_ = 0;
    Refresh();
}

void UiDesignerCatalogList::SetPresets(bool on)
{
    presets_ = on;
    scroll_ = 0;
    Refresh();
}

int UiDesignerCatalogList::Count() const
{
    if(!catalog_)
        return 0;
    return presets_ ? catalog_->GetPresets().GetCount()
                    : catalog_->FindCategory(category_).GetCount();
}

String UiDesignerCatalogList::ItemId(int index) const
{
    if(!catalog_)
        return String();
    if(presets_)
        return index >= 0 && index < catalog_->GetPresets().GetCount()
                   ? "preset:" + catalog_->GetPresets()[index].id
                   : String();
    Vector<int> matches = catalog_->FindCategory(category_);
    return index >= 0 && index < matches.GetCount()
               ? catalog_->GetControls()[matches[index]].type_id
               : String();
}

String UiDesignerCatalogList::ItemLabel(int index) const
{
    if(!catalog_)
        return String();
    if(presets_)
        return index >= 0 && index < catalog_->GetPresets().GetCount()
                   ? catalog_->GetPresets()[index].display_name
                   : String();
    Vector<int> matches = catalog_->FindCategory(category_);
    return index >= 0 && index < matches.GetCount()
               ? catalog_->GetControls()[matches[index]].display_name
               : String();
}

String UiDesignerCatalogList::ItemHelp(int index) const
{
    if(!catalog_)
        return String();
    if(presets_)
        return index >= 0 && index < catalog_->GetPresets().GetCount()
                   ? catalog_->GetPresets()[index].help
                   : String();
    Vector<int> matches = catalog_->FindCategory(category_);
    return index >= 0 && index < matches.GetCount()
               ? catalog_->GetControls()[matches[index]].help
               : String();
}

Image UiDesignerCatalogList::ItemIcon(int index) const
{
    if(!catalog_)
        return Image();
    if(presets_)
        return index >= 0 && index < catalog_->GetPresets().GetCount()
                   ? UiDesignerResolveCatalogIcon(catalog_->GetPresets()[index].icon_key)
                   : Image();
    Vector<int> matches = catalog_->FindCategory(category_);
    return index >= 0 && index < matches.GetCount()
               ? UiDesignerResolveCatalogIcon(catalog_->GetControls()[matches[index]].icon_key)
               : Image();
}

Rect UiDesignerCatalogList::ItemRect(int index) const
{
    const int row = DPI(38);
    return RectC(0, index * row - scroll_, GetSize().cx, row);
}

int UiDesignerCatalogList::GetContentHeight() const
{
    return Count() * DPI(38);
}

void UiDesignerCatalogList::Paint(Draw& w)
{
    w.DrawRect(GetSize(), SColorPaper());
    const int count = Count();
    for(int i = 0; i < count; i++) {
        Rect r = ItemRect(i);
        if(r.bottom < 0 || r.top > GetSize().cy)
            continue;
        Color face = i == hover_
                         ? Blend(SColorHighlight(), SColorPaper(), 40)
                         : (i & 1 ? Blend(SColorFace(), SColorPaper(), 70)
                                  : SColorPaper());
        w.DrawRect(r, face);
        Image icon = ItemIcon(i);
        if(!icon.IsEmpty())
            w.DrawImage(r.left + DPI(10), r.top + DPI(10),
                        DPI(18), DPI(18), icon);
        w.DrawText(r.left + DPI(38), r.top + DPI(10),
                   ItemLabel(i), SansSerifZ(11), SColorText());
        w.DrawLine(r.left, r.bottom - 1, r.right, r.bottom - 1,
                   1, SColorShadow());
    }
}

void UiDesignerCatalogList::LeftDown(Point p, dword)
{
    const int index = (p.y + scroll_) / DPI(38);
    if(index >= 0 && index < Count())
        WhenActivate(ItemId(index));
}

void UiDesignerCatalogList::MouseMove(Point p, dword)
{
    const int index = (p.y + scroll_) / DPI(38);
    const int next = index >= 0 && index < Count() ? index : -1;
    if(next != hover_) {
        hover_ = next;
        Tip(next >= 0 ? ItemHelp(next) : String());
        Refresh();
    }
}

void UiDesignerCatalogList::MouseLeave()
{
    hover_ = -1;
    Tip(String());
    Refresh();
}

void UiDesignerCatalogList::MouseWheel(Point, int zdelta, dword)
{
    const int maximum = max(0, GetContentHeight() - GetSize().cy);
    scroll_ = minmax(scroll_ - zdelta / 4, 0, maximum);
    Refresh();
}

void UiDesignerHierarchyView::SetDocument(
    const UiDesignerDocument *document)
{
    document_ = document;
    Rebuild();
}

void UiDesignerHierarchyView::SetSelection(
    const UiDesignerSelection *selection)
{
    selection_ = selection;
    Refresh();
}

void UiDesignerHierarchyView::AddRows(UiDesignerNodeId node, int depth)
{
    if(!document_)
        return;
    const UiDesignerNode* n = document_->Find(node);
    if(!n)
        return;
    Row& row = rows_.Add();
    row.node = node;
    row.depth = depth;
    for(UiDesignerNodeId child : n->children)
        AddRows(child, depth + 1);
}

void UiDesignerHierarchyView::BuildRows()
{
    rows_.Clear();
    if(document_)
        AddRows(document_->GetRootId(), 0);
}

void UiDesignerHierarchyView::Rebuild()
{
    BuildRows();
    Refresh();
}

void UiDesignerHierarchyView::Paint(Draw& w)
{
    w.DrawRect(GetSize(), SColorPaper());
    const int row_h = DPI(30);
    for(int i = 0; i < rows_.GetCount(); i++) {
        const int y = i * row_h - scroll_;
        if(y + row_h < 0 || y > GetSize().cy)
            continue;
        const UiDesignerNode* node =
            document_ ? document_->Find(rows_[i].node) : nullptr;
        if(!node)
            continue;
        const bool selected =
            selection_ && selection_->Contains(node->id);
        if(selected)
            w.DrawRect(0, y, GetSize().cx, row_h,
                       Blend(SColorHighlight(), SColorPaper(), 80));
        const int x = DPI(8) + rows_[i].depth * DPI(16);
        w.DrawText(x, y + DPI(7),
                   node->name + "  [" + node->type + "]",
                   SansSerifZ(10), SColorText());
    }
}

void UiDesignerHierarchyView::LeftDown(Point p, dword flags)
{
    const int index = (p.y + scroll_) / DPI(30);
    if(index >= 0 && index < rows_.GetCount())
        WhenSelectNode(rows_[index].node, (flags & K_CTRL) != 0);
}

void UiDesignerHierarchyView::MouseWheel(Point, int zdelta, dword)
{
    const int maximum = max(0, rows_.GetCount() * DPI(30) - GetSize().cy);
    scroll_ = minmax(scroll_ - zdelta / 4, 0, maximum);
    Refresh();
}

UiDesignerCodeView::UiDesignerCodeView()
{
    SetReadOnly();
}

void UiDesignerCodeView::SetCode(const String& code)
{
    SetData(code);
}

}
