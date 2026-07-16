#include "UiDesignerWidgets.h"
#include <Ui/UiIcons.h>

namespace Upp {

Image UiDesignerResolveCatalogIcon(const String& key)
{
    if(key == "layouts" || key == "spacer") return ICON_DESIGN_LAYOUTS_CATEGORY_48();
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
    Add(filter_edit_);
    filter_edit_.SetPlaceholder("Search controls...");
    filter_edit_.WhenChanging = [=] {
        filter_ = AsString(filter_edit_.GetData());
        RebuildMatches();
        WhenFilter(filter_);
    };
}

void UiDesignerCatalogList::SetCatalog(const UiDesignerCatalog *catalog)
{
    catalog_ = catalog;
    RebuildMatches();
}

void UiDesignerCatalogList::SetCategory(const String& category)
{
    category_ = category;
    presets_ = false;
    RebuildMatches();
}

void UiDesignerCatalogList::SetPresets(bool on)
{
    presets_ = on;
    RebuildMatches();
}

void UiDesignerCatalogList::SetFilter(const String& filter)
{
    filter_ = filter;
    filter_edit_.SetData(filter);
    RebuildMatches();
}

void UiDesignerCatalogList::RebuildMatches()
{
    matches_.Clear();
    if(catalog_) {
        const String needle = ToLower(TrimBoth(filter_));
        if(presets_) {
            for(int i = 0; i < catalog_->GetPresets().GetCount(); i++) {
                const UiDesignerPreset& preset = catalog_->GetPresets()[i];
                if(needle.IsEmpty() ||
                   ToLower(preset.display_name).Find(needle) >= 0 ||
                   ToLower(preset.help).Find(needle) >= 0)
                    matches_.Add(i);
            }
        }
        else
            matches_ = catalog_->Search(filter_, category_.IsEmpty() ? "All" : category_);
    }
    hover_ = -1;
    selected_ = matches_.IsEmpty() ? -1 : minmax(selected_, 0, matches_.GetCount() - 1);
    scroll_ = 0;
    Refresh();
}

int UiDesignerCatalogList::Count() const
{
    return matches_.GetCount();
}

String UiDesignerCatalogList::ItemId(int index) const
{
    if(!catalog_ || index < 0 || index >= matches_.GetCount())
        return String();
    const int source = matches_[index];
    return presets_ ? "preset:" + catalog_->GetPresets()[source].id
                    : catalog_->GetControls()[source].type_id;
}

String UiDesignerCatalogList::ItemLabel(int index) const
{
    if(!catalog_ || index < 0 || index >= matches_.GetCount())
        return String();
    const int source = matches_[index];
    return presets_ ? catalog_->GetPresets()[source].display_name
                    : catalog_->GetControls()[source].display_name;
}

String UiDesignerCatalogList::ItemHelp(int index) const
{
    if(!catalog_ || index < 0 || index >= matches_.GetCount())
        return String();
    const int source = matches_[index];
    return presets_ ? catalog_->GetPresets()[source].help
                    : catalog_->GetControls()[source].help;
}

Image UiDesignerCatalogList::ItemIcon(int index) const
{
    if(!catalog_ || index < 0 || index >= matches_.GetCount())
        return Image();
    const int source = matches_[index];
    return presets_
        ? UiDesignerResolveCatalogIcon(catalog_->GetPresets()[source].icon_key)
        : UiDesignerResolveCatalogIcon(catalog_->GetControls()[source].icon_key);
}

Rect UiDesignerCatalogList::ItemRect(int index) const
{
    const int top = DPI(40);
    const int row = DPI(42);
    return RectC(0, top + index * row - scroll_, GetSize().cx, row);
}

int UiDesignerCatalogList::RowAt(Point p) const
{
    if(p.y < DPI(40))
        return -1;
    const int index = (p.y - DPI(40) + scroll_) / DPI(42);
    return index >= 0 && index < Count() ? index : -1;
}

int UiDesignerCatalogList::GetContentHeight() const
{
    return Count() * DPI(42);
}

void UiDesignerCatalogList::Layout()
{
    filter_edit_.SetRect(DPI(6), DPI(5), max(0, GetSize().cx - DPI(12)), DPI(30));
}

void UiDesignerCatalogList::Paint(Draw& w)
{
    w.DrawRect(GetSize(), SColorPaper());
    for(int i = 0; i < Count(); i++) {
        Rect r = ItemRect(i);
        if(r.bottom < DPI(40) || r.top > GetSize().cy)
            continue;
        const bool current = i == selected_;
        Color face = current
            ? Blend(SColorHighlight(), SColorPaper(), 75)
            : i == hover_ ? Blend(SColorHighlight(), SColorPaper(), 35)
            : (i & 1 ? Blend(SColorFace(), SColorPaper(), 70) : SColorPaper());
        w.DrawRect(r, face);
        Image icon = ItemIcon(i);
        if(!icon.IsEmpty())
            w.DrawImage(r.left + DPI(10), r.top + DPI(11), DPI(18), DPI(18), icon);
        w.DrawText(r.left + DPI(38), r.top + DPI(7), ItemLabel(i),
                   SansSerifZ(11).Bold(current), SColorText());
        const String help = ItemHelp(i);
        if(!help.IsEmpty())
            w.DrawText(r.left + DPI(38), r.top + DPI(23),
                       help.Left(54), SansSerifZ(8), SColorDisabled());
        w.DrawLine(r.left, r.bottom - 1, r.right, r.bottom - 1, 1, SColorShadow());
    }
    if(Count() == 0)
        w.DrawText(DPI(12), DPI(54), "No matching controls", SansSerifZ(10), SColorDisabled());
}

void UiDesignerCatalogList::Activate(int index)
{
    if(index >= 0 && index < Count())
        WhenActivate(ItemId(index));
}

void UiDesignerCatalogList::LeftDown(Point p, dword)
{
    pressed_ = selected_ = RowAt(p);
    dragging_ = false;
    SetFocus();
    Refresh();
}

void UiDesignerCatalogList::LeftUp(Point p, dword)
{
    const int index = RowAt(p);
    if(!dragging_ && pressed_ >= 0 && index == pressed_)
        Activate(index);
    pressed_ = -1;
    dragging_ = false;
}

void UiDesignerCatalogList::LeftDouble(Point p, dword)
{
    Activate(RowAt(p));
}

void UiDesignerCatalogList::LeftDrag(Point, dword)
{
    if(pressed_ < 0 || pressed_ >= Count() || presets_)
        return;
    dragging_ = true;
    const String type = ItemId(pressed_);
    DoDragAndDrop(TextClip(UiDesignerCatalogDragText(type)),
                  ItemIcon(pressed_), DND_COPY);
    pressed_ = -1;
    dragging_ = false;
}

void UiDesignerCatalogList::MouseMove(Point p, dword)
{
    const int next = RowAt(p);
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
    const int list_height = max(0, GetSize().cy - DPI(40));
    const int maximum = max(0, GetContentHeight() - list_height);
    scroll_ = minmax(scroll_ - zdelta / 4, 0, maximum);
    Refresh();
}

bool UiDesignerCatalogList::Key(dword key, int)
{
    if(key == K_UP && Count()) {
        selected_ = max(0, selected_ - 1);
        Refresh();
        return true;
    }
    if(key == K_DOWN && Count()) {
        selected_ = min(Count() - 1, selected_ + 1);
        Refresh();
        return true;
    }
    if(key == K_ENTER && selected_ >= 0) {
        Activate(selected_);
        return true;
    }
    if(key == K_CTRL_F) {
        filter_edit_.SetFocus();
        return true;
    }
    return ParentCtrl::Key(key, 1);
}

void UiDesignerHierarchyView::SetDocument(const UiDesignerDocument *document)
{
    document_ = document;
    Rebuild();
}

void UiDesignerHierarchyView::SetSelection(const UiDesignerSelection *selection)
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

Rect UiDesignerHierarchyView::RowRect(int index) const
{
    return RectC(0, index * DPI(30) - scroll_, GetSize().cx, DPI(30));
}

int UiDesignerHierarchyView::RowAt(Point p) const
{
    const int index = (p.y + scroll_) / DPI(30);
    return index >= 0 && index < rows_.GetCount() ? index : -1;
}

void UiDesignerHierarchyView::Paint(Draw& w)
{
    w.DrawRect(GetSize(), SColorPaper());
    for(int i = 0; i < rows_.GetCount(); i++) {
        Rect r = RowRect(i);
        if(r.bottom < 0 || r.top > GetSize().cy)
            continue;
        const UiDesignerNode* node = document_ ? document_->Find(rows_[i].node) : nullptr;
        if(!node)
            continue;
        const bool selected = selection_ && selection_->Contains(node->id);
        if(selected)
            w.DrawRect(r, Blend(SColorHighlight(), SColorPaper(), 80));
        const int x = DPI(8) + rows_[i].depth * DPI(16);
        w.DrawText(x, r.top + DPI(7), node->name + "  [" + node->type + "]",
                   SansSerifZ(10), SColorText());
    }
    if(drop_row_ >= 0 && drop_row_ < rows_.GetCount()) {
        Rect r = RowRect(drop_row_);
        const Color color = drop_plan_.valid ? Color(34, 197, 94) : Color(220, 38, 38);
        if(drop_edge_ < 0)
            w.DrawRect(r.left, r.top, r.Width(), DPI(2), color);
        else if(drop_edge_ > 0)
            w.DrawRect(r.left, r.bottom - DPI(2), r.Width(), DPI(2), color);
        else {
            w.DrawRect(r.left, r.top, r.Width(), DPI(2), color);
            w.DrawRect(r.left, r.bottom - DPI(2), r.Width(), DPI(2), color);
            w.DrawRect(r.left, r.top, DPI(2), r.Height(), color);
            w.DrawRect(r.right - DPI(2), r.top, DPI(2), r.Height(), color);
        }
    }
}

void UiDesignerHierarchyView::LeftDown(Point p, dword flags)
{
    pressed_ = RowAt(p);
    if(pressed_ >= 0)
        WhenSelectNode(rows_[pressed_].node, (flags & K_CTRL) != 0);
    SetFocus();
}

void UiDesignerHierarchyView::LeftDrag(Point, dword)
{
    if(pressed_ < 0 || pressed_ >= rows_.GetCount() || !document_)
        return;
    const UiDesignerNodeId pressed_node = rows_[pressed_].node;
    if(pressed_node == document_->GetRootId())
        return;
    Vector<UiDesignerNodeId> nodes;
    if(selection_ && selection_->Contains(pressed_node))
        nodes = clone(selection_->nodes);
    else
        nodes.Add(pressed_node);
    DoDragAndDrop(TextClip(UiDesignerNodesDragText(nodes)),
                  ICON_DESIGN_ACCOUNT_TREE_48(), DND_MOVE);
    pressed_ = -1;
}

void UiDesignerHierarchyView::MouseWheel(Point, int zdelta, dword)
{
    const int maximum = max(0, rows_.GetCount() * DPI(30) - GetSize().cy);
    scroll_ = minmax(scroll_ - zdelta / 4, 0, maximum);
    Refresh();
}

void UiDesignerHierarchyView::ClearDrop()
{
    drop_row_ = -1;
    drop_edge_ = 0;
    drag_payload_.Clear();
    drop_plan_ = UiDesignerDropPlan();
    Refresh();
}

void UiDesignerHierarchyView::UpdateDrop(Point p, const String& payload)
{
    if(!document_ || !PlanDrop)
        return;
    Vector<UiDesignerNodeId> nodes;
    if(!UiDesignerParseNodesDragText(payload, nodes)) {
        ClearDrop();
        return;
    }
    const int row = RowAt(p);
    if(row < 0) {
        ClearDrop();
        return;
    }
    const UiDesignerNode* target = document_->Find(rows_[row].node);
    if(!target) {
        ClearDrop();
        return;
    }
    Rect rr = RowRect(row);
    const int third = max(1, rr.Height() / 3);
    drop_edge_ = p.y < rr.top + third ? -1
               : p.y >= rr.bottom - third ? 1 : 0;
    UiDesignerNodeId parent = target->id;
    int index = -1;
    if(drop_edge_ != 0 || !(target->flags & UiDesignerNodeContainer)) {
        parent = target->parent;
        const UiDesignerNode* parent_node = document_->Find(parent);
        index = parent_node ? FindIndex(parent_node->children, target->id) : -1;
        if(drop_edge_ >= 0 && index >= 0)
            index++;
        drop_edge_ = drop_edge_ == 0 ? 1 : drop_edge_;
    }
    drop_plan_ = PlanDrop(nodes, parent, index);
    drop_row_ = row;
    drag_payload_ = payload;
    if(WhenDropStatus)
        WhenDropStatus(drop_plan_.valid ? drop_plan_.label : drop_plan_.reason);
    Refresh();
}

void UiDesignerHierarchyView::DragEnter()
{
    Refresh();
}

void UiDesignerHierarchyView::DragAndDrop(Point p, PasteClip& d)
{
    if(!AcceptText(d)) {
        ClearDrop();
        return;
    }
    const String payload = GetString(d);
    UpdateDrop(p, payload);
    if(!drop_plan_.valid)
        return;
    d.SetAction(DND_MOVE);
    if(d.IsPaste()) {
        String error;
        const bool ok = ExecuteDrop && ExecuteDrop(drop_plan_, error);
        if(WhenDropStatus)
            WhenDropStatus(ok ? "Move completed" : error);
        ClearDrop();
    }
}

void UiDesignerHierarchyView::DragRepeat(Point p)
{
    if(!drag_payload_.IsEmpty())
        UpdateDrop(p, drag_payload_);
}

void UiDesignerHierarchyView::DragLeave()
{
    ClearDrop();
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
