#include "UiDesignerWidgets.h"
#include <Ui/UiIcons.h>

namespace Upp {

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

UiDesignerSideColumn& UiDesignerSideColumn::RightColumn(bool on)
{
    right_ = on;
    close_.SetIcon(on ? ICON_DESIGN_RIGHT_PANEL_CLOSE_48()
                      : ICON_DESIGN_LEFT_PANEL_CLOSE_48());
    return *this;
}

UiDesignerSideColumn& UiDesignerSideColumn::AddSection(
    const String& title, const Image& icon, Ctrl& content, const String& tip)
{
    const int section_index = section_buttons_.GetCount();
    UiToolButton& button = section_buttons_.Add();
    button.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
    button.SetIcon(icon).SetIconSize(DPI(16), DPI(16))
          .SetContentInset(DPI(4))
          .SetAlign(UiAlign::CENTER, UiAlign::CENTER);
    button.SetCheckable();
    button.Tip(tip.IsEmpty() ? title : tip);
    button.WhenAction = [=] { Select(section_index); };
    tool_layout_.Add(button).Fixed(DPI(24)).MinCross(DPI(24));
    pages_.Add(content, title);
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

}
