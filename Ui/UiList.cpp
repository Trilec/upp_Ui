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

static void DrawAlignedListText(Draw& w, const Rect& r, const String& text, Font font, Color ink, int align)
{
    if(r.IsEmpty() || text.IsEmpty())
        return;
    Size sz = GetTextSize(text, font);
    int x = r.left;
    if(align == ALIGN_RIGHT)
        x = max(r.left, r.right - sz.cx);
    else if(align == ALIGN_CENTER)
        x = max(r.left, r.left + (r.GetWidth() - sz.cx) / 2);
    int y = r.top + max(0, (r.GetHeight() - sz.cy) / 2);
    w.DrawText(x, y, text, font, ink);
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
        s.metrics.content_margin = Rect(DPI(8), DPI(8), DPI(8), DPI(8));
        s.metrics.focus_enabled = true;
        s.metrics.focus_margin = DPI(2);
        s.metrics.focus_alpha = 180;
        s.metrics.focus_color = Color(65, 167, 248);
        s.skin = StyledSkin();

        s.font = StdFont();
        s.row_height = DPI(26);
        s.item_spacing = 0;
        s.icon_size = DPI(16);
        s.check_size = DPI(14);
        s.content_gap = DPI(6);
        s.h_padding = DPI(8);
        s.v_padding = DPI(6);
        s.row_radius = DPI(4);
        s.metadata_size = DPI(8);
        s.metadata_gap = DPI(6);
        s.right_gap = DPI(8);
        s.drag_size = DPI(14);
        s.drag_gap = DPI(6);
        s.show_icons = true;
        s.show_checks = true;
        s.show_metadata_marker = true;
        s.show_drag_handle = true;
        s.drag_side = UiAlign::RIGHT;
        s.drag_glyph = ICON_DESIGN_DRAG_INDICATOR_48();
        s.hot_as_underline = false;
        s.selected_as_underline = false;
        s.state_underline_thickness = DPI(2);

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
        s.row_even_face = Null;
        s.row_odd_face = Null;
        s.show_row_separator = false;
        s.row_state_frame_enabled = false;
        s.right_text_as_badge = false;
        s.badge_face = Color(241, 245, 249);
        s.badge_frame = Null;
        s.badge_ink = Color(51, 65, 85);
        s.badge_radius = DPI(999);
        s.badge_h_padding = DPI(6);
        s.metadata_default = Color(65, 167, 248);
        s.check_frame = Color(148, 163, 184);
        s.check_fill = Color(17, 24, 39);
        s.drag_marker = Color(56, 146, 255);
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
    Add(drag_marker_);
    drag_marker_.Color(Color(56, 146, 255)).IgnoreMouse().Hide();

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
    if(!has_custom_style_) {
        style_ = GetEffectiveStyle();
        has_custom_style_ = true;
    }
    theme_revision_ = 0;
    return style_;
}

const UiList::Style& UiList::GetEffectiveStyle() const
{
    if(has_custom_style_)
        return style_;
    const_cast<UiList*>(this)->SyncThemeStyle();
    return themed_style_;
}

void UiList::SyncThemeStyle()
{
    if(has_custom_style_)
        return;
    uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;
    themed_style_ = UiTheme::ResolveList();
    theme_revision_ = revision;
}

UiList& UiList::SetCustomStyle(const Style& s)
{
    style_ = s;
    has_custom_style_ = true;
    OnStyleChanged();
    return *this;
}

UiList& UiList::ClearCustomStyle()
{
    if(!has_custom_style_)
        return *this;
    has_custom_style_ = false;
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
    if(model_ == &model)
        return *this;
    model_ = &model;
    BindModel(model);
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
    cursor_ = -1;
    anchor_ = -1;
    NotifySelectionChange();
    return *this;
}

UiList& UiList::Select(int index, bool additive)
{
    SyncModel();
    if(!IsSelectableIndex(index))
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

void UiList::SetData(const Value& v)
{
    SyncModel();

    if(IsNull(v)) {
        ClearSelection();
        return;
    }

    if(selection_mode_ == UILISTSEL_MULTI || v.Is<ValueArray>()) {
        selected_.Clear();
        ValueArray values;
        if(v.Is<ValueArray>())
            values = v;
        else
            values.Add(v);

        for(int i = 0; i < values.GetCount(); i++) {
            int index = ResolveSelectionIndex(values[i]);
            if(index >= 0)
                selected_.FindAdd(index);
        }

        Vector<int> selection = GetSelection();
        anchor_ = selection.IsEmpty() ? -1 : selection[0];
        cursor_ = selection.IsEmpty() ? -1 : selection.Top();
        NotifySelectionChange();
        return;
    }

    int index = ResolveSelectionIndex(v);
    if(index >= 0)
        SelectSingle(index);
    else
        ClearSelection();
}

Value UiList::GetData() const
{
    if(selection_mode_ == UILISTSEL_MULTI) {
        ValueArray values;
        Vector<int> selection = GetSelection();
        for(int i = 0; i < selection.GetCount(); i++)
            values.Add(GetSelectionToken(selection[i]));
        return values;
    }

    return selected_.GetCount() > 0 ? GetSelectionToken(selected_[0]) : Value();
}

UiList& UiList::EnableRenameOnDblClick(bool on)
{
    rename_on_dblclick_ = on;
    return *this;
}

UiList& UiList::EnableDragReorder(bool on)
{
    drag_reorder_enabled_ = on;
    if(!on)
        EndRowDrag(true);
    Refresh();
    return *this;
}

void UiList::BindModel(UiListModel& model)
{
    for(int i = 0; i < bound_models_.GetCount(); i++) {
        if(bound_models_[i] == &model)
            return;
    }

    bound_models_.Add(&model);
    Ptr<UiList> self = this;
    UiListModel* observed = &model;
    model.WhenChange << [self, observed](const UiModelChange&) {
        if(self && self->model_ == observed) {
            self->SyncModel();
            self->RefreshLayout();
            self->Refresh();
        }
    };
}

UiList& UiList::ShowDragHandle(bool on)
{
    StyleEdit().show_drag_handle = on;
    RefreshLayout();
    Refresh();
    return *this;
}

UiList& UiList::SetDragSide(UiAlign side)
{
    if(side != UiAlign::LEFT && side != UiAlign::RIGHT)
        side = UiAlign::RIGHT;
    StyleEdit().drag_side = side;
    RefreshLayout();
    Refresh();
    return *this;
}

UiList& UiList::SetDragGlyph(const Image& glyph)
{
    StyleEdit().drag_glyph = glyph;
    Refresh();
    return *this;
}

UiList& UiList::SetCursor(int index)
{
    SyncModel();
    if(!IsSelectableIndex(index))
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
    if(!model_)
        return 0;
    int count = model_->GetCount();
    if(count <= 0)
        return 0;
    int rh = max(DPI(18), GetEffectiveStyle().row_height);
    int sp = max(0, GetEffectiveStyle().item_spacing);
    return count * rh + max(0, count - 1) * sp;
}

Rect UiList::GetRowRect(int row) const
{
    Rect vp = GetViewportRect();
    int rh = max(DPI(18), GetEffectiveStyle().row_height);
    int sp = max(0, GetEffectiveStyle().item_spacing);
    int extent = rh + sp;
    int y = vp.top - scroll_y_ + row * extent;
    return Rect(vp.left, y, vp.right, y + rh);
}

int UiList::HitTestRow(Point p) const
{
    Rect vp = GetViewportRect();
    if(!vp.Contains(p) || !model_)
        return -1;
    int rh = max(DPI(18), GetEffectiveStyle().row_height);
    int sp = max(0, GetEffectiveStyle().item_spacing);
    int extent = rh + sp;
    int local = p.y - vp.top + scroll_y_;
    int row = local / max(1, extent);
    if(row < 0 || row >= model_->GetCount())
        return -1;
    if(local % max(1, extent) >= rh)
        return -1;
    return row;
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
        x = GetCheckRect(row).right + style.content_gap;
    return RectC(x, y, size, size);
}

Rect UiList::GetMetadataRect(const Rect& row, bool has_check, bool has_icon) const
{
    const Style& style = GetEffectiveStyle();
    int size = min(style.metadata_size, row.GetHeight() - DPI(8));
    int y = row.top + (row.GetHeight() - size) / 2;
    int x = row.left + style.h_padding;
    if(drag_reorder_enabled_ && style.show_drag_handle && style.drag_side == UiAlign::LEFT)
        x = GetDragRect(row).right + style.drag_gap;
    if(has_check)
        x = GetCheckRect(row).right + style.content_gap;
    if(has_icon)
        x = GetIconRect(row, has_check).right + style.content_gap;
    return RectC(x, y, size, size);
}

Rect UiList::GetDragRect(const Rect& row) const
{
    const Style& style = GetEffectiveStyle();
    if(!drag_reorder_enabled_ || !style.show_drag_handle)
        return Rect(0, 0, 0, 0);
    int size = min(style.drag_size, row.GetHeight() - DPI(6));
    int y = row.top + (row.GetHeight() - size) / 2;
    int x = style.drag_side == UiAlign::LEFT
          ? row.left + style.h_padding
          : row.right - style.h_padding - size;
    return RectC(x, y, size, size);
}

Rect UiList::GetRightTextRect(const Rect& row, const UiModelItem& item) const
{
    if(item.right_text.IsEmpty())
        return Rect(0, 0, 0, 0);
    const Style& style = GetEffectiveStyle();
    Font font = item.use_custom_font ? item.custom_font : style.font;
    Size sz = GetTextSize(item.right_text, font);
    int extra = style.right_text_as_badge ? style.badge_h_padding * 2 : DPI(4);
    int w = min(sz.cx + extra, max(0, row.GetWidth() / 2));
    int right = row.right - style.h_padding;
    if(drag_reorder_enabled_ && style.show_drag_handle && style.drag_side == UiAlign::RIGHT)
        right = GetDragRect(row).left - style.drag_gap;
    return Rect(max(row.left, right - w), row.top, right, row.bottom);
}

Rect UiList::GetTextRect(const Rect& row, bool has_check, bool has_icon, bool has_metadata, const UiModelItem& item) const
{
    const Style& style = GetEffectiveStyle();
    int left = row.left + style.h_padding;
    if(drag_reorder_enabled_ && style.show_drag_handle && style.drag_side == UiAlign::LEFT)
        left = GetDragRect(row).right + style.drag_gap;
    if(has_check)
        left = GetCheckRect(row).right + style.content_gap;
    if(has_icon)
        left = GetIconRect(row, has_check).right + style.content_gap;
    if(has_metadata)
        left = GetMetadataRect(row, has_check, has_icon).right + style.metadata_gap;
    Rect right = GetRightTextRect(row, item);
    int right_edge = right.IsEmpty() ? row.right - style.h_padding : right.left - style.right_gap;
    if(drag_reorder_enabled_ && style.show_drag_handle && style.drag_side == UiAlign::RIGHT)
        right_edge = min(right_edge, GetDragRect(row).left - style.drag_gap);
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

    if(index > 0 && item.separator_before)
        w.DrawRect(row.left, row.top, row.GetWidth(), 1, style.separator_color);

    bool underline_state = (selected && style.selected_as_underline) || (!selected && hot && style.hot_as_underline);

    if(selected || hot) {
        Color accent = selected ? style.selected_frame : style.hot_frame;
        if(underline_state) {
            int thickness = max(DPI(1), style.state_underline_thickness);
            w.DrawRect(rr.left, rr.bottom - thickness, rr.GetWidth(), thickness, accent);
        }
        else {
        StyledPalette p;
        StyledMetrics m;
        m.face_enabled = true;
        m.frame_enabled = style.row_state_frame_enabled;
        m.frame_width = DPI(1);
        m.radius = style.row_radius;
        for(int i = 0; i < 4; i++) {
            p.face[i] = UiFill::Solid(selected ? style.selected_face : style.hot_face);
            p.frame[i] = selected ? style.selected_frame : style.hot_frame;
            p.ink[i] = selected ? style.selected_ink : style.hot_ink;
        }
        UiPaintFaceFrameDash(w, rr, p, m, st);
        }
    }

    bool has_check = style.show_checks && (item.has_check || item.checked);
    bool has_icon = style.show_icons && !IsNull(item.icon);
    bool has_metadata = style.show_metadata_marker && item.has_metadata;
    bool has_drag = drag_reorder_enabled_ && style.show_drag_handle;

    if(has_drag) {
        Rect dr = GetDragRect(rr);
        Color drag_ink = style.muted_ink;
        if(index == drag_from_ && dragging_)
            drag_ink = style.selected_frame;
        else if(index == hot_drag_ || index == pressed_drag_)
            drag_ink = style.hot_ink;
        UiPaintStyledIcon(w, dr, IsNull(style.drag_glyph) ? ICON_DESIGN_DRAG_INDICATOR_48() : style.drag_glyph,
                          true, true, UiIconRenderMode::MonoTint, drag_ink, item.enabled);
    }

    if(has_check)
        PaintCheck(w, GetCheckRect(rr), item, selected);

    if(has_icon) {
        Color icon_ink = !IsNull(item.custom_ink_color)
                       ? item.custom_ink_color
                       : (selected ? style.selected_ink : (item.enabled ? style.muted_ink : style.disabled_ink));
                UiPaintStyledIcon(w, GetIconRect(rr, has_check), item.icon, true, true, item.icon_render_mode, icon_ink, item.enabled);
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

    DrawAlignedListText(w, tx, item.text, font, ink, item.text_align);

    if(item.underline) {
        Color uc = IsNull(item.underline_color) ? ink : item.underline_color;
        Size tsz = GetTextSize(item.text, font);
        int ux = tx.left;
        if(item.text_align == ALIGN_RIGHT)
            ux = max(tx.left, tx.right - tsz.cx);
        else if(item.text_align == ALIGN_CENTER)
            ux = max(tx.left, tx.left + (tx.GetWidth() - tsz.cx) / 2);
        int uy = min(tx.bottom - 2, tx.top + max(0, (tx.GetHeight() - tsz.cy) / 2) + tsz.cy + 1);
        w.DrawRect(ux, uy, min(tx.right - ux, tsz.cx), 1, uc);
    }

    if(!rx.IsEmpty()) {
        Font rf = style.font;
        if(item.group_header) rf.Bold();
        Color rink = selected ? style.selected_ink : (item.enabled ? style.muted_ink : style.disabled_ink);
        Rect text_rx = rx;
        if(style.right_text_as_badge) {
            StyledPalette p;
            StyledMetrics m;
            m.face_enabled = !IsNull(style.badge_face);
            m.frame_enabled = !IsNull(style.badge_frame);
            m.frame_width = DPI(1);
            m.radius = style.badge_radius;
            m.focus_enabled = false;
            for(int i = 0; i < 4; i++) {
                p.face[i] = UiFill::Solid(style.badge_face);
                p.frame[i] = style.badge_frame;
                p.ink[i] = style.badge_ink;
            }
            UiPaintFaceFrameDash(w, rx.Deflated(0, DPI(2)), p, m, st);
            rink = item.enabled ? style.badge_ink : style.disabled_ink;
            text_rx = rx.Deflated(style.badge_h_padding, 0);
        }
        DrawAlignedListText(w, text_rx, item.right_text, rf, rink, item.right_text_align);
    }

    if(style.show_row_separator && model_ && index + 1 < model_->GetCount())
        w.DrawRect(row.left, row.bottom - 1, row.GetWidth(), 1, style.separator_color);
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
        bool has_check = GetEffectiveStyle().show_checks && (item.has_check || item.checked);
        bool has_icon = GetEffectiveStyle().show_icons && !IsNull(item.icon);
        bool has_metadata = GetEffectiveStyle().show_metadata_marker && item.has_metadata;
        Rect tx = GetTextRect(row.Deflated(DPI(2), DPI(1)), has_check, has_icon, has_metadata, item);
        inline_editor_.SetRect(tx.left - DPI(2), tx.top + DPI(2), max(DPI(80), tx.GetWidth() + DPI(4)), max(DPI(22), tx.GetHeight() - DPI(4)));
    }
    else
        inline_editor_.Hide();

    UpdateDragMarker();
}

Size UiList::GetMinSize() const
{
    const Style& style = GetEffectiveStyle();
    int rows_h = style.row_height * 4 + max(0, style.item_spacing) * 3;
    return UiStyledOuterSizeFromContent(Size(DPI(180), max(DPI(80), rows_h)), style.metrics, style.skin);
}

void UiList::LeftDown(Point p, dword flags)
{
    SetFocus();
    CommitRenameIfNeeded(p);
    SyncModel();
    int row = HitTestRow(p);
    pressed_ = row;
    pressed_drag_ = -1;
    if(row < 0) {
        Refresh();
        return;
    }
    if(!IsSelectableIndex(row)) {
        pressed_ = -1;
        Refresh();
        return;
    }

    int drag_row = HitTestDrag(p);
    if(drag_row >= 0) {
        pressed_drag_ = drag_row;
        if(!IsSelected(drag_row))
            SelectSingle(drag_row);
        cursor_ = drag_row;
        anchor_ = drag_row;
        BeginRowDrag(drag_row, GetMousePos());
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

void UiList::LeftDrag(Point, dword)
{
    if(drag_candidate_)
        ContinueRowDrag(GetMousePos());
}

void UiList::LeftUp(Point, dword)
{
    if(drag_candidate_) {
        EndRowDrag(false);
        return;
    }

    if(pressed_ >= 0 || pressed_drag_ >= 0) {
        pressed_ = -1;
        pressed_drag_ = -1;
        Refresh();
    }
}

void UiList::LeftDouble(Point p, dword flags)
{
    CommitRenameIfNeeded(p);
    int row = HitTestRow(p);
    if(row < 0 || !model_)
        return;
    const UiModelItem& item = model_->Get(row);
    if(rename_on_dblclick_ && item.editable && item.enabled && !item.group_header)
        BeginRename(row);
    else if(WhenAction)
        WhenAction();
}

void UiList::MouseMove(Point p, dword)
{
    if(drag_candidate_) {
        ContinueRowDrag(GetMousePos());
        return;
    }

    int row = HitTestRow(p);
    int drag_row = HitTestDrag(p);
    if(hot_ != row || hot_drag_ != drag_row) {
        hot_ = row;
        hot_drag_ = drag_row;
        Refresh();
    }
}

void UiList::MouseLeave()
{
    if(drag_candidate_)
        return;
    if(hot_ >= 0 || hot_drag_ >= 0 || pressed_ >= 0 || pressed_drag_ >= 0) {
        hot_ = -1;
        hot_drag_ = -1;
        pressed_ = -1;
        pressed_drag_ = -1;
        Refresh();
    }
}

void UiList::MouseWheel(Point, int zdelta, dword)
{
    Rect vp = GetViewportRect();
    int extent = max(DPI(18), GetEffectiveStyle().row_height) + max(0, GetEffectiveStyle().item_spacing);
    int rows = max(1, vp.GetHeight() / max(1, extent));
    int step = max(1, rows / 2) * extent;
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
        int extent = max(DPI(18), GetEffectiveStyle().row_height) + max(0, GetEffectiveStyle().item_spacing);
        int rows = max(1, GetViewportRect().GetHeight() / max(1, extent));
        MoveCursorBy(-rows);
        return true;
    }
    case K_PAGEDOWN: {
        int extent = max(DPI(18), GetEffectiveStyle().row_height) + max(0, GetEffectiveStyle().item_spacing);
        int rows = max(1, GetViewportRect().GetHeight() / max(1, extent));
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
    if(editing_ && !HasFocusDeep())
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

bool UiList::IsSelectableIndex(int index) const
{
    if(!model_ || index < 0 || index >= model_->GetCount())
        return false;
    const UiModelItem& item = model_->Get(index);
    return item.enabled && !item.group_header;
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
    while(next >= 0 && next < model_->GetCount() && !IsSelectableIndex(next))
        next += delta >= 0 ? 1 : -1;
    if(IsSelectableIndex(next)) {
        SelectSingle(next);
        ScrollTo(next);
    }
}

void UiList::MoveCursorToEdge(bool end)
{
    SyncModel();
    if(!model_ || model_->IsEmpty())
        return;
    int index = end ? model_->GetCount() - 1 : 0;
    while(index >= 0 && index < model_->GetCount() && !IsSelectableIndex(index))
        index += end ? -1 : 1;
    if(IsSelectableIndex(index)) {
        SelectSingle(index);
        ScrollTo(index);
    }
}

void UiList::SelectSingle(int index)
{
    if(!IsSelectableIndex(index))
        return;
    selected_.Clear();
    selected_.FindAdd(index);
    cursor_ = index;
    anchor_ = index;
    NotifySelectionChange();
}

void UiList::ToggleSelection(int index)
{
    if(!IsSelectableIndex(index))
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
    if(!IsSelectableIndex(index))
        return;
    int start = anchor_ >= 0 ? anchor_ : (cursor_ >= 0 ? cursor_ : index);
    if(!additive)
        selected_.Clear();
    int a = min(start, index);
    int b = max(start, index);
    for(int i = a; i <= b; i++)
        if(IsSelectableIndex(i))
            selected_.FindAdd(i);
    cursor_ = index;
    anchor_ = start;
    NotifySelectionChange();
}

Value UiList::GetSelectionToken(int index) const
{
    if(!model_ || index < 0 || index >= model_->GetCount())
        return Value();

    const UiModelItem& item = model_->Get(index);
    return IsNull(item.data) ? Value(index) : item.data;
}

int UiList::ResolveSelectionIndex(const Value& token) const
{
    if(!model_)
        return -1;

    for(int i = 0; i < model_->GetCount(); i++) {
        const UiModelItem& item = model_->Get(i);
        if(!IsNull(item.data) && item.data == token && IsSelectableIndex(i))
            return i;
    }

    if(token.Is<int>()) {
        int index = token;
        return IsSelectableIndex(index) ? index : -1;
    }
    if(token.Is<int64>()) {
        int64 index = token;
        return index >= 0 && index <= INT_MAX && IsSelectableIndex((int)index) ? (int)index : -1;
    }
    return -1;
}

void UiList::NotifySelectionChange()
{
    Refresh();
    if(WhenSelection)
        WhenSelection();
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

int UiList::HitTestDrag(Point p) const
{
    int row = HitTestRow(p);
    if(row < 0 || !model_)
        return -1;
    Rect rr = GetRowRect(row).Deflated(DPI(2), DPI(1));
    return GetDragRect(rr).Contains(p) ? row : -1;
}

void UiList::BeginRowDrag(int row, Point start_screen)
{
    if(!drag_reorder_enabled_ || !model_ || row < 0 || row >= model_->GetCount() || model_->GetCount() < 2) {
        drag_candidate_ = false;
        return;
    }

    drag_candidate_ = true;
    dragging_ = false;
    drag_moved_ = false;
    drag_from_ = row;
    drag_insert_before_ = row;
    drag_start_screen_ = start_screen;
    drag_marker_.Hide();
}

void UiList::ContinueRowDrag(Point p_screen)
{
    if(!drag_candidate_)
        return;

    if(!dragging_) {
        int dx = p_screen.x - drag_start_screen_.x;
        int dy = p_screen.y - drag_start_screen_.y;
        if(abs(dy) < drag_threshold_px_ || abs(dy) < abs(dx))
            return;
        dragging_ = true;
        drag_moved_ = true;
        drag_marker_.Show();
        drag_marker_.Remove();
        Add(drag_marker_);
    }

    Rect self = GetScreenRect();
    int y = p_screen.y - self.top;
    Rect vp = GetViewportRect();
    int before = model_ ? model_->GetCount() : 0;
    if(model_) {
        for(int i = 0; i < model_->GetCount(); i++) {
            Rect rr = GetRowRect(i);
            int mid = rr.top + rr.GetHeight() / 2;
            if(y < mid) {
                before = i;
                break;
            }
        }
    }
    drag_insert_before_ = before;
    UpdateDragMarker();
    Refresh();
}

void UiList::EndRowDrag(bool cancel)
{
    if(!drag_candidate_) {
        dragging_ = false;
        drag_moved_ = false;
        return;
    }

    if(!cancel && dragging_ && drag_from_ >= 0)
        MoveRowTo(drag_from_, drag_insert_before_);

    drag_candidate_ = false;
    dragging_ = false;
    drag_moved_ = false;
    drag_from_ = -1;
    drag_insert_before_ = -1;
    pressed_drag_ = -1;
    drag_marker_.Hide();
    Refresh();
}

void UiList::MoveRowTo(int from, int before)
{
    if(!model_ || from < 0 || from >= model_->GetCount())
        return;
    if(before < 0 || before > model_->GetCount())
        return;
    if(before == from || before == from + 1)
        return;

    const int original_before = before;
    if(!model_->Move(from, before))
        return;

    Index<int> remapped;
    for(int i = 0; i < selected_.GetCount(); i++)
        remapped.FindAdd(RemapIndexAfterMove(selected_[i], from, before));
    selected_ = pick(remapped);
    cursor_ = RemapIndexAfterMove(cursor_, from, before);
    anchor_ = RemapIndexAfterMove(anchor_, from, before);
    hot_ = RemapIndexAfterMove(hot_, from, before);
    pressed_ = RemapIndexAfterMove(pressed_, from, before);
    hot_drag_ = RemapIndexAfterMove(hot_drag_, from, before);
    pressed_drag_ = RemapIndexAfterMove(pressed_drag_, from, before);
    editing_index_ = RemapIndexAfterMove(editing_index_, from, before);
    model_revision_ = model_->GetRevision();

    if(WhenReordered)
        WhenReordered(from, original_before);

    Layout();
    Refresh();
}

void UiList::UpdateDragMarker()
{
    if(!dragging_ || !model_ || drag_from_ < 0 || drag_from_ >= model_->GetCount()) {
        drag_marker_.Hide();
        return;
    }

    Rect vp = GetViewportRect();
    int line_y = vp.top;
    if(drag_insert_before_ >= 0 && drag_insert_before_ < model_->GetCount())
        line_y = GetRowRect(drag_insert_before_).top;
    else if(model_->GetCount() > 0)
        line_y = GetRowRect(model_->GetCount() - 1).bottom;

    int cy = DPI(2);
    int x = vp.left + GetEffectiveStyle().h_padding;
    int cx = max(DPI(24), vp.GetWidth() - GetEffectiveStyle().h_padding * 2);
    drag_marker_.Color(GetEffectiveStyle().drag_marker);
    drag_marker_.SetRect(x, line_y - cy / 2, cx, cy);
    drag_marker_.Show();
}

int UiList::RemapIndexAfterMove(int index, int from, int before) const
{
    if(index < 0)
        return index;
    if(before < from) {
        if(index == from)
            return before;
        if(index >= before && index < from)
            return index + 1;
        return index;
    }
    if(before > from + 1) {
        if(index == from)
            return before - 1;
        if(index > from && index < before)
            return index - 1;
        return index;
    }
    return index;
}

}

