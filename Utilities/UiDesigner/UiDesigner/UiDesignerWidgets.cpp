#include "UiDesignerWidgets.h"
#include <Ui/UiIcons.h>

namespace Upp {

Image UiDesignerResolveCatalogIcon(const String& key)
{
    if(key == "layouts" || key == "spacer") return ICON_DESIGN_LAYOUTS_CATEGORY_48();
    if(key == "containers") return ICON_DESIGN_TAB_GROUP_48();
    if(key == "composites") return ICON_DESIGN_DYNAMIC_FORM_48();
    if(key == "presets") return ICON_DESIGN_DASHBOARD_EDIT_48();
    if(key == "data") return ICON_EDITOR_FORMAT_LIST_BULLETED_48();
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

    int insert = items_.GetCount();
    while(insert > 0 && items_[insert - 1].trailing)
        --insert;
    Item& item = items_.Insert(insert);
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

UiDesignerPillBar& UiDesignerPillBar::AddTrailingControl(Ctrl& ctrl, int extent)
{
    Add(ctrl);
    Item& item = items_.Add();
    item.ctrl = &ctrl;
    item.extent = max(DPI(24), extent);
    item.trailing = true;
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
    const int horizontal_h = min(DPI(28), max(DPI(24), cy - DPI(8)));
    const int item_gap = DPI(8);

    int trailing_width = 0;
    if(!vertical_ && show_auxiliary_) {
        for(const Item& item : items_)
            if(item.ctrl && item.trailing)
                trailing_width += item.extent + item_gap;
    }
    int trailing_start = max(inset_, cx - inset_ - trailing_width);
    int row_count = 1;
    if(!vertical_) {
        int probe = inset_;
        for(const Item& item : items_) {
            if(!item.ctrl || item.trailing || (!item.section && !show_auxiliary_))
                continue;
            if(probe + item.extent > trailing_start && probe > inset_) {
                probe = inset_;
                row_count++;
            }
            probe += item.extent + item_gap;
        }
    }

    int cursor = inset_;
    const int rows_height = row_count * horizontal_h +
                            max(0, row_count - 1) * DPI(4);
    int row_y = vertical_ ? inset_ : max(DPI(4), (cy - rows_height) / 2);

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
        else if(item.trailing) {
            PutCtrl(*item.ctrl, trailing_start, (cy - horizontal_h) / 2,
                    item.extent, horizontal_h);
            trailing_start += item.extent + item_gap;
        }
        else {
            if(cursor + item.extent > trailing_start && cursor > inset_) {
                cursor = inset_;
                row_y += horizontal_h + DPI(4);
            }
            PutCtrl(*item.ctrl, cursor, row_y, item.extent, horizontal_h);
            cursor += item.extent + item_gap;
        }
    }
}

UiDesignerSideColumn::UiDesignerSideColumn()
{
    tool_grid_.SetGridSize(2, 1)
              .SetMinCellSize(Size(DPI(10), DPI(10)))
              .SetGap(DPI(0))
              .SetInset(DPI(0));

    UiPanel::Style tool_style = UiTheme::ResolvePanel(UiRole::Subtle);
    tool_style.metrics.face_enabled = true;
    tool_style.palette.face[ST_NORMAL] = UiFill::Solid(Color(243, 243, 243));
    tool_style.metrics.frame_enabled = true;
    for(int i = 0; i < 4; i++)
        tool_style.palette.frame[i] = Color(216, 216, 216);
    tool_style.metrics.frame_width = DPI(1);
    tool_style.metrics.radius = DPI(15);
    tool_style.metrics.shadow.enabled = true;
    tool_style.metrics.shadow.distance = DPI(9);
    tool_style.metrics.shadow.offset_x = DPI(0);
    tool_style.metrics.shadow.offset_y = DPI(0);
    tool_style.metrics.shadow.alpha = 40;
    tool_style.metrics.shadow.color = Black();
    tool_style.metrics.shadow.mode = SHADOW_CURVE;
    tool_style.metrics.shadow.curve = ShadowSoft();
    tool_panel_.SetCustomStyle(tool_style).SetInset(DPI(4));

    tool_layout_.SetDirection(UiDirection::H)
                .SetGap(DPI(4), DPI(4))
                .SetInset(DPI(0))
                .SetWrap(UiBoxWrap::Flow)
                .SetWrapAutoResize(true);
    action_layout_.SetDirection(UiDirection::H)
                  .SetGap(DPI(4), DPI(4))
                  .SetInset(DPI(0))
                  .SetWrap(UiBoxWrap::None);
    tool_panel_.Add(tool_layout_);
    tool_grid_.Add(tool_panel_, 0, 0, true, true);
    tool_grid_.Add(action_layout_, 0, 1, false, true, Size(DPI(52), DPI(0)));

    content_surface_.SetCustomStyle(UiDesignerSurfaceStyle());
    content_surface_.Add(pages_.SizePos());

    close_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
    close_.SetIcon(ICON_DESIGN_LEFT_PANEL_CLOSE_48())
          .SetIconSize(DPI(16), DPI(16))
          .SetContentInset(DPI(4))
          .SetAlign(UiAlign::CENTER, UiAlign::CENTER);
    close_.Tip("Collapse panel");
    close_.WhenAction = [=] { Close(); };

    expand_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
    expand_.SetIcon(ICON_DESIGN_UNFOLD_MORE_48())
           .SetIconSize(DPI(16), DPI(16))
           .SetContentInset(DPI(4))
           .SetAlign(UiAlign::CENTER, UiAlign::CENTER);
    expand_.Tip("Cycle panel width");
    expand_.WhenAction = [=] { Cycle(); };

    action_layout_.Add(expand_).Fixed(DPI(24)).MinCross(DPI(24));
    action_layout_.Add(close_).Fixed(DPI(24)).MinCross(DPI(24));

    Add(tool_grid_);
    Add(content_surface_);
}

UiDesignerHierarchyView::UiDesignerHierarchyView()
{
    WantFocus();
}

UiDesignerSideColumn& UiDesignerSideColumn::RightColumn(bool on)
{
    right_ = on;
    close_.SetIcon(on ? ICON_DESIGN_RIGHT_PANEL_CLOSE_48()
                      : ICON_DESIGN_LEFT_PANEL_CLOSE_48());
    return *this;
}

UiDesignerSideColumn& UiDesignerSideColumn::AddSection(
    const String& tip, const Image& icon, Ctrl& content)
{
    const int section_index = section_buttons_.GetCount();
    UiToolButton& button = section_buttons_.Add();
    button.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
    button.SetIcon(icon).SetIconSize(DPI(16), DPI(16))
          .SetContentInset(DPI(4))
          .SetAlign(UiAlign::CENTER, UiAlign::CENTER);
    button.SetCheckable();
    button.Tip(tip);
    button.WhenAction = [=] { Select(section_index); };
    tool_layout_.Add(button).Fixed(DPI(24)).MinCross(DPI(24));
    pages_.Add(content, tip);
    if(pages_.GetCount() == 1)
        pages_.SetActivePage(0);
    UpdateToolSelection();
    return *this;
}

UiDesignerSideColumn& UiDesignerSideColumn::ApplyTheme(
    const UiDesignerThemeSnapshot& theme)
{
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
    UpdateToolSelection();
    Layout();
    WhenSectionChanged(index);
    WhenWidthChanged();
}

void UiDesignerSideColumn::UpdateToolSelection()
{
    for(int i = 0; i < section_buttons_.GetCount(); i++)
        section_buttons_[i].SetChecked(i == active_section_)
                              .SetCustomStyle(UiTheme::ResolveToolButton(
                                  i == active_section_ ? UiRole::Accent : UiRole::Subtle));
}

int UiDesignerSideColumn::GetToolRowHeight(int width) const
{
    const int action_width = DPI(52);
    const int panel_width = max(DPI(32), width - action_width);
    // The reference panel has both its explicit four-pixel inset and the
    // resolved panel content margin. Use the same effective 12px side inset
    // here rather than pinning the flow against the painted frame.
    const int content_width = max(DPI(1), panel_width - DPI(24));
    return max(UiDesignerStyleMetrics::DesignerToolbarHeight(),
               tool_layout_.MeasureHeightForWidth(content_width) + DPI(8));
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
        tool_grid_.SetRect(0, 0, w, UiDesignerStyleMetrics::DesignerToolbarHeight());
        tool_panel_.Show();
        action_layout_.Show();
        content_surface_.Hide();
    }
    else {
        const int pill_h = GetToolRowHeight(w);
        tool_grid_.SetRect(0, 0, w, pill_h);
        PutCtrl(content_surface_, 0, pill_h, w, max(0, h - pill_h));
        content_surface_.Show();
    }

    // The reference grid mirrors its two cells on the right: expand/close is
    // on the inner edge, while its tool panel remains against the outer edge.
    const int toolbar_h = width_ == PANE_CLOSED
        ? UiDesignerStyleMetrics::DesignerToolbarHeight() : GetToolRowHeight(w);
    const int action_w = min(DPI(52), max(0, w));
    const int panel_w = max(0, w - action_w);
    if(right_) {
        action_layout_.SetRect(0, 0, action_w, toolbar_h);
        tool_panel_.SetRect(action_w, 0, panel_w, toolbar_h);
    }
    else {
        tool_panel_.SetRect(0, 0, panel_w, toolbar_h);
        action_layout_.SetRect(panel_w, 0, action_w, toolbar_h);
    }

    // UiPanel owns painting, while the flow layout owns the wrapped children.
    // Keep its natural rows vertically centered inside the explicit inset.
    const Size panel_size = tool_panel_.GetSize();
    const int panel_content_inset = DPI(12);
    const int tool_w = max(0, panel_size.cx - panel_content_inset * 2);
    const int tool_h = min(max(0, panel_size.cy - DPI(8)),
                           tool_layout_.MeasureHeightForWidth(max(1, tool_w)));
    const int preferred_w = tool_layout_.GetPreferredSize().cx;
    const int tool_x = right_ && tool_h <= DPI(28)
        ? max(panel_content_inset,
              panel_size.cx - panel_content_inset - min(tool_w, preferred_w))
        : panel_content_inset;
    tool_layout_.SetRect(tool_x, max(DPI(4), (panel_size.cy - tool_h) / 2),
                         min(tool_w, panel_size.cx - tool_x - panel_content_inset), tool_h);

    const int action_h = DPI(28);
    action_layout_.SetRect(action_layout_.GetRect().left,
                           max(0, (toolbar_h - action_h) / 2),
                           action_layout_.GetSize().cx, action_h);
}

UiDesignerCatalogList::UiDesignerCatalogList()
{
    BackPaint();
    Add(filter_edit_);
    Add(scope_label_);
    filter_edit_.SetPlaceholder("Filter controls...");
    scope_label_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
    UpdateScopeLabel();
    filter_edit_.WhenChange = [=] {
        filter_ = AsString(filter_edit_.GetData());
        RebuildMatches();
        WhenFilter(filter_);
    };
}

void UiDesignerCatalogList::SetCatalog(const UiDesignerCatalog *catalog)
{
    catalog_ = catalog;
    UpdateScopeLabel();
    RebuildMatches();
}

void UiDesignerCatalogList::SetCategory(const String& category)
{
    category_ = category;
    presets_ = false;
    UpdateScopeLabel();
    RebuildMatches();
}

void UiDesignerCatalogList::SetPresets(bool on)
{
    presets_ = on;
    UpdateScopeLabel();
    RebuildMatches();
}

void UiDesignerCatalogList::SetFilter(const String& filter)
{
    filter_ = filter;
    filter_edit_.SetData(filter);
    RebuildMatches();
}

void UiDesignerCatalogList::UpdateScopeLabel()
{
    const String scope = presets_
        ? "Presets"
        : (category_.IsEmpty() ? "All controls" : category_);
    scope_label_.SetText(scope);
    scope_label_.Tip("Current catalog scope");
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
    const int top = DPI(72);
    const int row = DPI(42);
    return RectC(0, top + index * row - scroll_, GetSize().cx, row);
}

int UiDesignerCatalogList::RowAt(Point p) const
{
    if(p.y < DPI(72))
        return -1;
    const int index = (p.y - DPI(72) + scroll_) / DPI(42);
    return index >= 0 && index < Count() ? index : -1;
}

int UiDesignerCatalogList::GetContentHeight() const
{
    return Count() * DPI(42);
}

void UiDesignerCatalogList::Layout()
{
    filter_edit_.SetRect(DPI(6), DPI(6), max(0, GetSize().cx - DPI(12)), DPI(34));
    scope_label_.SetRect(DPI(8), DPI(42), max(0, GetSize().cx - DPI(16)), DPI(16));
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
    drag_type_ = pressed_ >= 0 && !presets_ ? ItemId(pressed_) : String();
    drag_start_ = GetMousePos();
    drag_armed_ = pressed_ >= 0 && !presets_ && !drag_type_.IsEmpty();
    dragging_ = false;
    if(drag_armed_ && !HasCapture())
        SetCapture();
    SetFocus();
    Refresh();
}

void UiDesignerCatalogList::LeftUp(Point p, dword)
{
    const int pressed = pressed_;
    const String type = drag_type_;
    const bool was_dragging = dragging_;
    const bool was_armed = drag_armed_;
    const Point screen = GetMousePos();
    const int index = RowAt(p);
    pressed_ = -1;
    dragging_ = false;
    drag_armed_ = false;
    drag_type_.Clear();
    if(HasCapture())
        ReleaseCapture();
    if(was_dragging && was_armed && !type.IsEmpty())
        WhenToolDrop(type, screen);
    else if(pressed >= 0 && index == pressed)
        Activate(index);
}

void UiDesignerCatalogList::LeftDouble(Point p, dword)
{
    Activate(RowAt(p));
}

void UiDesignerCatalogList::LeftDrag(Point, dword)
{
    if(pressed_ < 0 || pressed_ >= Count() || presets_)
        return;
    if(!drag_armed_ && !drag_type_.IsEmpty()) {
        drag_armed_ = true;
        if(!HasCapture())
            SetCapture();
    }
    MouseMove(GetMousePos() - GetScreenRect().TopLeft(), K_MOUSELEFT);
}

void UiDesignerCatalogList::MouseMove(Point p, dword)
{
    if(drag_armed_) {
        if(!HasCapture())
            SetCapture();
        if(!dragging_ && Length(GetMousePos() - drag_start_) >= DPI(5))
            dragging_ = true;
        if(dragging_ && WhenToolDrag)
            WhenToolDrag(drag_type_, GetMousePos());
        return;
    }
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

Image UiDesignerCatalogList::CursorImage(Point p, dword flags)
{
    return dragging_ ? Image::SizeAll() : ParentCtrl::CursorImage(p, flags);
}

void UiDesignerCatalogList::CancelMode()
{
    const bool active = drag_armed_ || dragging_;
    pressed_ = -1;
    dragging_ = false;
    drag_armed_ = false;
    drag_type_.Clear();
    if(active && WhenToolCancel)
        WhenToolCancel();
    ParentCtrl::CancelMode();
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
    VectorMap<String, ClipData> payload;
    Append(payload, UiDesignerNodesDragText(nodes));
    DoDragAndDrop(payload, ICON_DESIGN_ACCOUNT_TREE_48(), DND_MOVE);
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
    String payload;
    if(!UiDesignerReadDragText(d, payload)) {
        d.Reject();
        ClearDrop();
        return;
    }
    UpdateDrop(p, payload);
    if(!drop_plan_.valid) {
        d.Reject();
        ClearDrop();
        return;
    }
    d.Accept();
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

bool UiDesignerHierarchyView::Key(dword key, int)
{
    if(key == K_DELETE) {
        if(WhenDelete)
            WhenDelete();
        return true;
    }
    return ParentCtrl::Key(key, 1);
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
