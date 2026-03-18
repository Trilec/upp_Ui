#include <Ui/UiList.h>
#include <Ui/UiTheme.h>

namespace Upp {

bool UiList::InlineEditor::Key(dword key, int count)
{
    if(key == K_ENTER) {
        if(WhenAccept)
            WhenAccept();
        return true;
    }
    if(key == K_ESCAPE) {
        if(WhenAbort)
            WhenAbort();
        return true;
    }
    return EditString::Key(key, count);
}

void UiList::InlineEditor::LostFocus()
{
    EditString::LostFocus();
    if(WhenBlur)
        WhenBlur();
}

static StyledState UiListState_(bool enabled, bool pressed, bool hot)
{
    if(!enabled)
        return ST_DISABLED;
    if(pressed)
        return ST_PRESSED;
    if(hot)
        return ST_HOT;
    return ST_NORMAL;
}

const UiList::Style& UiList::StyleDefault()
{
    static Style s;
    ONCELOCK {
        const Color text_primary = Color(17, 24, 39);
        const Color text_muted = Color(148, 163, 184);
        const Color text_secondary = Color(100, 116, 139);

        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(White());
            s.palette.frame[i] = Color(226, 232, 240);
            s.palette.ink[i] = text_primary;
            s.palette.icon[i] = text_secondary;
        }

        s.palette.face[ST_HOT] = UiFill::Solid(Color(248, 250, 252));
        s.palette.face[ST_PRESSED] = UiFill::Solid(Color(241, 245, 249));
        s.palette.face[ST_DISABLED] = UiFill::Solid(Color(248, 250, 252));
        s.palette.frame[ST_DISABLED] = Color(241, 245, 249);
        s.palette.ink[ST_DISABLED] = text_muted;
        s.palette.icon[ST_DISABLED] = text_muted;

        s.metrics = StyledMetrics();
        s.metrics.text_font = StdFont();
        s.metrics.use_text_font = false;
        s.metrics.face_enabled = true;
        s.metrics.frame_enabled = true;
        s.metrics.frame_width = DPI(1);
        s.metrics.radius = 0;
        s.metrics.content_padding = Rect(DPI(8), DPI(8), DPI(8), DPI(8));
        s.metrics.focus_enabled = true;
        s.metrics.focus_margin = DPI(2);
        s.metrics.focus_alpha = 180;
        s.metrics.focus_color = Color(65, 167, 248);
        s.skin = StyledSkin();

        s.font = StdFont();
        s.row_height = DPI(26);
        s.icon_size = DPI(16);
        s.check_size = DPI(14);
        s.label_gap = DPI(6);
        s.h_padding = DPI(8);
        s.v_padding = DPI(6);
        s.row_radius = DPI(4);
        s.metadata_size = DPI(8);
        s.metadata_gap = DPI(6);
        s.right_gap = DPI(8);
        s.show_icons = true;
        s.show_checks = true;
        s.show_metadata_marker = true;

        s.ink = text_primary;
        s.disabled_ink = text_muted;
        s.muted_ink = text_secondary;
        s.hot_face = Color(245, 247, 250);
        s.hot_frame = Color(226, 232, 240);
        s.hot_ink = text_primary;
        s.selected_face = Color(232, 242, 255);
        s.selected_frame = Color(65, 167, 248);
        s.selected_ink = text_primary;
        s.separator_color = Color(226, 232, 240);
        s.metadata_default = Color(65, 167, 248);
        s.check_frame = Color(148, 163, 184);
        s.check_fill = Color(17, 24, 39);
    }
    return s;
}

UiList::UiList()
    : style_(StyleDefault())
    , themed_style_(StyleDefault())
    , model_(&internal_model_)
{
    BackPaint();
    WantFocus();

    inline_editor_.Hide();
    inline_editor_.WhenAccept = [=] { CommitRename(); };
    inline_editor_.WhenAbort = [=] { CancelRename(); };
    inline_editor_.WhenBlur = [=] {
        if(editing_)
            CommitRename();
    };
    Add(inline_editor_);

    SyncThemeStyle();
    SyncModel();
}

UiList::Style& UiList::StyleEdit()
{
    if(!has_style_override_) {
        style_ = GetEffectiveStyle();
        has_style_override_ = true;
    }
    theme_revision_ = 0;
    return style_;
}

const UiList::Style& UiList::GetEffectiveStyle() const
{
    if(has_style_override_)
        return style_;
    const_cast<UiList*>(this)->SyncThemeStyle();
    return themed_style_;
}

void UiList::SyncThemeStyle()
{
    if(has_style_override_)
        return;
    uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;
    themed_style_ = UiTheme::ResolveList();
    theme_revision_ = revision;
}

UiList& UiList::SetStyle(const Style& s)
{
    style_ = s;
    has_style_override_ = true;
    OnStyleChanged();
    return *this;
}

UiList& UiList::ClearStyleOverride()
{
    if(!has_style_override_)
        return *this;
    has_style_override_ = false;
    style_ = StyleDefault();
    theme_revision_ = 0;
    OnStyleChanged();
    return *this;
}

void UiList::OnStyleChanged()
{
    RefreshLayout();
    Refresh();
}

UiList& UiList::SetModel(UiListModel& model)
{
    CancelRename();
    model_ = &model;
    model_revision_ = -1;
    selected_.Clear();
    cursor_ = -1;
    anchor_ = -1;
    hot_ = -1;
    pressed_ = -1;
    scroll_y_ = 0;
    SyncModel();
    RefreshLayout();
    Refresh();
    return *this;
}

UiList& UiList::SetSelectionMode(UiListSelectionMode mode)
{
    if(selection_mode_ == mode)
        return *this;
    selection_mode_ = mode;
    if(selection_mode_ == UILISTSEL_SINGLE && selected_.GetCount() > 1)
        SelectSingle(cursor_);
    Refresh();
    return *this;
}

UiList& UiList::ClearSelection()
{
    if(selected_.IsEmpty())
        return *this;
    selected_.Clear();
    anchor_ = -1;
    NotifySelectionChange();
    return *this;
}

UiList& UiList::Select(int index, bool additive)
{
    SyncModel();
    if(!model_ || index < 0 || index >= model_->GetCount())
        return *this;
    if(selection_mode_ != UILISTSEL_MULTI || !additive)
        SelectSingle(index);
    else
        ToggleSelection(index);
    return *this;
}

UiList& UiList::SelectAll()
{
    SyncModel();
    if(selection_mode_ != UILISTSEL_MULTI || !model_)
        return *this;
    selected_.Clear();
    for(int i = 0; i < model_->GetCount(); i++)
        if(model_->Get(i).enabled && !model_->Get(i).group_header)
            selected_.FindAdd(i);
    if(selected_.GetCount() > 0) {
        cursor_ = selected_[0];
        anchor_ = cursor_;
    }
    NotifySelectionChange();
    return *this;
}

bool UiList::IsSelected(int index) const
{
    return selected_.Find(index) >= 0;
}

Vector<int> UiList::GetSelection() const
{
    Vector<int> out;
    for(int i = 0; i < selected_.GetCount(); i++)
        out.Add(selected_[i]);
    Sort(out);
    return out;
}

UiList& UiList::EnableRenameOnDblClick(bool on)
{
    rename_on_dblclick_ = on;
    return *this;
}

UiList& UiList::SetCursor(int index)
{
    SyncModel();
    if(!model_ || index < 0 || index >= model_->GetCount())
        return *this;
    SelectSingle(index);
    ScrollTo(index);
    return *this;
}

void UiList::SyncModel()
{
    if(!model_)
        return;
    int rev = model_->GetRevision();
    if(rev == model_revision_)
        return;
    model_revision_ = rev;

    int count = model_->GetCount();
    for(int i = selected_.GetCount() - 1; i >= 0; i--) {
        int index = selected_[i];
        if(index < 0 || index >= count)
            selected_.Remove(i);
    }
    if(cursor_ >= count)
        cursor_ = count - 1;
    if(anchor_ >= count)
        anchor_ = cursor_;
    if(hot_ >= count)
        hot_ = -1;
    if(pressed_ >= count)
        pressed_ = -1;
    if(editing_ && editing_index_ >= count)
        CancelRename();
    ClampScroll();
}

void UiList::ClampScroll()
{
    Rect vp = GetViewportRect();
    int max_scroll = max(0, GetTotalHeight() - max(0, vp.GetHeight()));
    scroll_y_ = clamp(scroll_y_, 0, max_scroll);
}

Rect UiList::GetViewportRect() const
{
    return UiStyledInnerRect(GetSize(), GetEffectiveStyle().metrics, GetEffectiveStyle().skin);
}

int UiList::GetTotalHeight() const
{
    return model_ ? model_->GetCount() * max(DPI(18), GetEffectiveStyle().row_height) : 0;
}

Rect UiList::GetRowRect(int row) const
{
    Rect vp = GetViewportRect();
    int rh = max(DPI(18), GetEffectiveStyle().row_height);
    int y = vp.top - scroll_y_ + row * rh;
    return Rect(vp.left, y, vp.right, y + rh);
}

int UiList::HitTestRow(Point p) const
{
    Rect vp = GetViewportRect();
    if(!vp.Contains(p) || !model_)
        return -1;
    int rh = max(DPI(18), GetEffectiveStyle().row_height);
    int row = (p.y - vp.top + scroll_y_) / rh;
    return row >= 0 && row < model_->GetCount() ? row : -1;
}

Rect UiList::GetCheckRect(const Rect& row) const
{
    const Style& style = GetEffectiveStyle();
    int size = min(style.check_size, row.GetHeight() - DPI(6));
    int y = row.top + (row.GetHeight() - size) / 2;
    int x = row.left + style.h_padding;
    return RectC(x, y, size, size);
}

Rect UiList::GetIconRect(const Rect& row, bool has_check) const
{
    const Style& style = GetEffectiveStyle();
    int size = min(style.icon_size, row.GetHeight() - DPI(6));
    int y = row.top + (row.GetHeight() - size) / 2;
    int x = row.left + style.h_padding;
    if(has_check)
        x = GetCheckRect(row).right + style.label_gap;
    return RectC(x, y, size, size);
}

Rect UiList::GetMetadataRect(const Rect& row, bool has_check, bool has_icon) const
{
    const Style& style = GetEffectiveStyle();
    int size = min(style.metadata_size, row.GetHeight() - DPI(8));
    int y = row.top + (row.GetHeight() - size) / 2;
    int x = row.left + style.h_padding;
    if(has_check)
        x = GetCheckRect(row).right + style.label_gap;
    if(has_icon)
        x = GetIconRect(row, has_check).right + style.label_gap;
    return RectC(x, y, size, size);
}

Rect UiList::GetRightTextRect(const Rect& row, const UiModelItem& item) const
{
    if(item.right_text.IsEmpty())
        return Rect(0, 0, 0, 0);
    const Style& style = GetEffectiveStyle();
    Font font = item.use_custom_font ? item.custom_font : style.font;
    Size sz = GetTextSize(item.right_text, font);
    int w = min(sz.cx + DPI(4), max(0, row.GetWidth() / 2));
    return Rect(row.right - style.h_padding - w, row.top, row.right - style.h_padding, row.bottom);
}

Rect UiList::GetTextRect(const Rect& row, bool has_check, bool has_icon, bool has_metadata, const UiModelItem& item) const
{
    const Style& style = GetEffectiveStyle();
    int left = row.left + style.h_padding;
    if(has_check)
        left = GetCheckRect(row).right + style.label_gap;
    if(has_icon)
        left = GetIconRect(row, has_check).right + style.label_gap;
    if(has_metadata)
        left = GetMetadataRect(row, has_check, has_icon).right + style.metadata_gap;
    Rect right = GetRightTextRect(row, item);
    int right_edge = right.IsEmpty() ? row.right - style.h_padding : right.left - style.right_gap;
    return Rect(left, row.top, max(left, right_edge), row.bottom);
}

void UiList::PaintCheck(Draw& w, const Rect& r, const UiModelItem& item, bool selected) const
{
    if(r.IsEmpty())
        return;
    const Style& style = GetEffectiveStyle();
    StyledPalette p;
    StyledMetrics m;
    m.face_enabled = true;
    m.frame_enabled = true;
    m.frame_width = DPI(1);
    m.radius = DPI(3);
    for(int i = 0; i < 4; i++) {
        p.face[i] = UiFill::Solid(White());
        p.frame[i] = style.check_frame;
        p.ink[i] = style.check_fill;
    }
    if(selected) {
        for(int i = 0; i < 4; i++)
            p.frame[i] = style.selected_frame;
    }
    if(item.checked) {
        for(int i = 0; i < 4; i++)
            p.face[i] = UiFill::Solid(selected ? style.selected_frame : style.check_fill);
    }
    UiPaintFaceFrameDash(w, r, p, m, item.enabled ? ST_NORMAL : ST_DISABLED);
    if(item.checked) {
        Color ink = White();
        int pad = max(2, r.GetWidth() / 5);
        int x1 = r.left + pad;
        int y1 = r.top + r.GetHeight() / 2;
        int x2 = r.left + r.GetWidth() / 2 - 1;
        int y2 = r.bottom - pad - 1;
        int x3 = r.right - pad - 1;
        int y3 = r.top + pad;
        w.DrawLine(x1, y1, x2, y2, 2, ink);
        w.DrawLine(x2, y2, x3, y3, 2, ink);
    }
}

void UiList::PaintRow(Draw& w, int index, const Rect& row) const
{
    if(!model_ || index < 0 || index >= model_->GetCount() || row.IsEmpty())
        return;

    const Style& style = GetEffectiveStyle();
    const UiModelItem& item = model_->Get(index);
    bool selected = IsSelected(index);
    bool hot = index == hot_;
    bool pressed = index == pressed_;
    StyledState st = UiListState_(item.enabled, pressed, hot);

    Rect rr = row;
    rr.Deflate(DPI(2), DPI(1));

    if(index > 0 && item.separator_before)
        w.DrawRect(row.left + style.h_padding, row.top, row.GetWidth() - style.h_padding * 2, 1, style.separator_color);

    if(selected || hot) {
        StyledPalette p;
        StyledMetrics m;
        m.face_enabled = true;
        m.frame_enabled = true;
        m.frame_width = DPI(1);
        m.radius = style.row_radius;
        for(int i = 0; i < 4; i++) {
            p.face[i] = UiFill::Solid(selected ? style.selected_face : style.hot_face);
            p.frame[i] = selected ? style.selected_frame : style.hot_frame;
            p.ink[i] = selected ? style.selected_ink : style.hot_ink;
        }
        UiPaintFaceFrameDash(w, rr, p, m, st);
    }

    bool has_check = style.show_checks && item.checked;
    bool has_icon = style.show_icons && !IsNull(item.icon);
    bool has_metadata = style.show_metadata_marker && item.has_metadata;

    if(has_check)
        PaintCheck(w, GetCheckRect(rr), item, selected);

    if(has_icon) {
        Color icon_ink = !IsNull(item.custom_ink_color)
                       ? item.custom_ink_color
                       : (selected ? style.selected_ink : (item.enabled ? style.muted_ink : style.disabled_ink));
        UiPaintStyledIcon(w, GetIconRect(rr, has_check), item.icon, true, item.mono_icon, icon_ink, item.enabled);
    }

    if(has_metadata) {
        Rect mr = GetMetadataRect(rr, has_check, has_icon);
        Color c = IsNull(item.metadata_color) ? style.metadata_default : item.metadata_color;
        w.DrawRect(mr, c);
    }

    Rect tx = GetTextRect(rr, has_check, has_icon, has_metadata, item);
    Rect rx = GetRightTextRect(rr, item);
    Font font = item.use_custom_font ? item.custom_font : style.font;
    if(item.group_header && !item.use_custom_font) font.Bold();
    Color ink = !IsNull(item.custom_ink_color)
              ? item.custom_ink_color
              : (selected ? style.selected_ink : (item.enabled ? style.ink : style.disabled_ink));

    int ty = tx.top + max(0, (tx.GetHeight() - GetTextSize(item.text, font).cy) / 2);
    w.DrawText(tx.left, ty, item.text, font, ink);

    if(item.underline) {
        Color uc = IsNull(item.underline_color) ? ink : item.underline_color;
        int uy = min(tx.bottom - 2, ty + GetTextSize(item.text, font).cy + 1);
        w.DrawRect(tx.left, uy, min(tx.GetWidth(), GetTextSize(item.text, font).cx), 1, uc);
    }

    if(!rx.IsEmpty()) {
        Font rf = style.font;
        if(item.group_header) rf.Bold();
        Color rink = selected ? style.selected_ink : (item.enabled ? style.muted_ink : style.disabled_ink);
        Size rsz = GetTextSize(item.right_text, rf);
        int rty = rx.top + max(0, (rx.GetHeight() - rsz.cy) / 2);
        w.DrawText(rx.right - rsz.cx, rty, item.right_text, rf, rink);
    }
}

void UiList::Paint(Draw& w)
{
    SyncModel();
    const Style& style = GetEffectiveStyle();
    UiPaintStyledSurface(w, GetSize(), style.palette, style.metrics, style.skin,
                         IsEnabled() ? ST_NORMAL : ST_DISABLED,
                         HasFocus(), false, false);

    Rect vp = GetViewportRect();
    w.Clip(vp);
    for(int i = 0; model_ && i < model_->GetCount(); i++) {
        Rect row = GetRowRect(i);
        if(row.bottom <= vp.top)
            continue;
        if(row.top >= vp.bottom)
            break;
        PaintRow(w, i, row);
    }
    w.End();
}

void UiList::Layout()
{
    SyncModel();
    if(editing_ && editing_index_ >= 0 && model_ && editing_index_ < model_->GetCount()) {
        Rect row = GetRowRect(editing_index_);
        const UiModelItem& item = model_->Get(editing_index_);
        bool has_check = GetEffectiveStyle().show_checks && item.checked;
        bool has_icon = GetEffectiveStyle().show_icons && !IsNull(item.icon);
        bool has_metadata = GetEffectiveStyle().show_metadata_marker && item.has_metadata;
        Rect tx = GetTextRect(row.Deflated(DPI(2), DPI(1)), has_check, has_icon, has_metadata, item);
        inline_editor_.SetRect(tx.left - DPI(2), tx.top + DPI(2), max(DPI(80), tx.GetWidth() + DPI(4)), max(DPI(22), tx.GetHeight() - DPI(4)));
    }
    else
        inline_editor_.Hide();
}

Size UiList::GetMinSize() const
{
    const Style& style = GetEffectiveStyle();
    return UiStyledOuterSizeFromContent(Size(DPI(180), max(DPI(80), style.row_height * 4)), style.metrics, style.skin);
}

void UiList::LeftDown(Point p, dword flags)
{
    SetFocus();
    CommitRenameIfNeeded(p);
    SyncModel();
    int row = HitTestRow(p);
    pressed_ = row;
    if(row < 0) {
        Refresh();
        return;
    }

    bool shift = (flags & K_SHIFT) != 0;
    bool ctrl = (flags & K_CTRL) != 0;

    if(selection_mode_ == UILISTSEL_MULTI) {
        if(shift)
            SelectRangeTo(row, ctrl);
        else if(ctrl)
            ToggleSelection(row);
        else
            SelectSingle(row);
    }
    else
        SelectSingle(row);

    cursor_ = row;
    anchor_ = row;
    ScrollTo(row);
    Refresh();
}

void UiList::LeftDouble(Point p, dword flags)
{
    CommitRenameIfNeeded(p);
    int row = HitTestRow(p);
    if(row < 0 || !model_)
        return;
    const UiModelItem& item = model_->Get(row);
    bool has_check = GetEffectiveStyle().show_checks && item.checked;
    bool has_icon = GetEffectiveStyle().show_icons && !IsNull(item.icon);
    bool has_metadata = GetEffectiveStyle().show_metadata_marker && item.has_metadata;
    Rect tx = GetTextRect(GetRowRect(row).Deflated(DPI(2), DPI(1)), has_check, has_icon, has_metadata, item);
    if(rename_on_dblclick_ && item.editable && tx.Contains(p))
        BeginRename(row);
    else if(WhenAction)
        WhenAction();
}

void UiList::MouseMove(Point p, dword)
{
    int row = HitTestRow(p);
    if(hot_ != row) {
        hot_ = row;
        Refresh();
    }
}

void UiList::MouseLeave()
{
    if(hot_ >= 0 || pressed_ >= 0) {
        hot_ = -1;
        pressed_ = -1;
        Refresh();
    }
}

void UiList::MouseWheel(Point, int zdelta, dword)
{
    Rect vp = GetViewportRect();
    int rows = max(1, vp.GetHeight() / max(DPI(18), GetEffectiveStyle().row_height));
    int step = max(1, rows / 2) * max(DPI(18), GetEffectiveStyle().row_height);
    scroll_y_ -= sgn(zdelta) * step;
    ClampScroll();
    Layout();
    Refresh();
}

bool UiList::Key(dword key, int)
{
    SyncModel();
    if(!model_ || model_->IsEmpty())
        return false;

    switch(key) {
    case K_UP: MoveCursorBy(-1); return true;
    case K_DOWN: MoveCursorBy(1); return true;
    case K_HOME: MoveCursorToEdge(false); return true;
    case K_END: MoveCursorToEdge(true); return true;
    case K_PAGEUP: {
        int rows = max(1, GetViewportRect().GetHeight() / max(DPI(18), GetEffectiveStyle().row_height));
        MoveCursorBy(-rows);
        return true;
    }
    case K_PAGEDOWN: {
        int rows = max(1, GetViewportRect().GetHeight() / max(DPI(18), GetEffectiveStyle().row_height));
        MoveCursorBy(rows);
        return true;
    }
    case K_ENTER:
    case K_SPACE:
        if(WhenAction)
            WhenAction();
        return true;
    case K_F2:
        if(cursor_ >= 0 && cursor_ < model_->GetCount() && model_->Get(cursor_).editable) {
            BeginRename(cursor_);
            return true;
        }
        break;
    case K_CTRL_A:
        if(selection_mode_ == UILISTSEL_MULTI) {
            SelectAll();
            return true;
        }
        break;
    }
    return false;
}

void UiList::GotFocus()
{
    Refresh();
}

void UiList::LostFocus()
{
    if(editing_)
        CommitRename();
    Refresh();
}

void UiList::ScrollTo(int index)
{
    if(!model_ || index < 0 || index >= model_->GetCount())
        return;
    Rect vp = GetViewportRect();
    Rect row = GetRowRect(index);
    if(row.top < vp.top)
        scroll_y_ -= vp.top - row.top;
    else if(row.bottom > vp.bottom)
        scroll_y_ += row.bottom - vp.bottom;
    ClampScroll();
    Layout();
    Refresh();
}

void UiList::ScrollToSelection()
{
    if(cursor_ >= 0)
        ScrollTo(cursor_);
}

void UiList::MoveCursorBy(int delta)
{
    SyncModel();
    if(!model_ || model_->IsEmpty())
        return;
    int next = cursor_ >= 0 ? cursor_ + delta : (delta >= 0 ? 0 : model_->GetCount() - 1);
    next = clamp(next, 0, model_->GetCount() - 1);
    SelectSingle(next);
    ScrollTo(next);
}

void UiList::MoveCursorToEdge(bool end)
{
    SyncModel();
    if(!model_ || model_->IsEmpty())
        return;
    int index = end ? model_->GetCount() - 1 : 0;
    SelectSingle(index);
    ScrollTo(index);
}

void UiList::SelectSingle(int index)
{
    if(index < 0 || !model_ || index >= model_->GetCount())
        return;
    selected_.Clear();
    selected_.FindAdd(index);
    cursor_ = index;
    anchor_ = index;
    NotifySelectionChange();
}

void UiList::ToggleSelection(int index)
{
    if(index < 0 || !model_ || index >= model_->GetCount())
        return;
    int fi = selected_.Find(index);
    if(fi >= 0)
        selected_.Remove(fi);
    else
        selected_.FindAdd(index);
    cursor_ = index;
    anchor_ = index;
    NotifySelectionChange();
}

void UiList::SelectRangeTo(int index, bool additive)
{
    if(index < 0 || !model_ || index >= model_->GetCount())
        return;
    int start = anchor_ >= 0 ? anchor_ : (cursor_ >= 0 ? cursor_ : index);
    if(!additive)
        selected_.Clear();
    int a = min(start, index);
    int b = max(start, index);
    for(int i = a; i <= b; i++)
        selected_.FindAdd(i);
    cursor_ = index;
    NotifySelectionChange();
}

void UiList::NotifySelectionChange()
{
    Refresh();
    if(WhenSel)
        WhenSel();
}

bool UiList::CommitRenameIfNeeded(Point p)
{
    if(!editing_)
        return false;
    if(inline_editor_.IsShown() && inline_editor_.GetRect().Contains(p))
        return false;
    CommitRename();
    return true;
}

void UiList::BeginRename(int index)
{
    if(!model_ || index < 0 || index >= model_->GetCount())
        return;
    const UiModelItem& item = model_->Get(index);
    if(!item.editable)
        return;
    editing_ = true;
    editing_index_ = index;
    inline_editor_.SetData(item.text);
    inline_editor_.Show();
    inline_editor_.SetFocus();
    Layout();
    inline_editor_.SelectAll();
}

void UiList::CommitRename()
{
    if(!editing_ || !model_ || editing_index_ < 0 || editing_index_ >= model_->GetCount()) {
        CancelRename();
        return;
    }
    String text = AsString(inline_editor_.GetData());
    UiModelItem item = model_->Get(editing_index_);
    item.text = text;
    model_->Set(editing_index_, item);
    if(WhenRename)
        WhenRename(editing_index_, text);
    editing_ = false;
    editing_index_ = -1;
    inline_editor_.Hide();
    Refresh();
}

void UiList::CancelRename()
{
    editing_ = false;
    editing_index_ = -1;
    inline_editor_.Hide();
    Refresh();
}

}


