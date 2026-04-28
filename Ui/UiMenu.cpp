#include "UiMenu.h"
#include <Ui/UiTheme.h>

namespace Upp {

namespace {

void DrawMenuArrowGlyph(Draw& w, const Rect& r, Color c)
{
    int midy = r.top + r.GetHeight() / 2;
    int left = r.left + max(1, r.GetWidth() / 4);
    int right = r.right - max(1, r.GetWidth() / 4) - 1;
    w.DrawLine(left, midy - 4, right, midy, 2, c);
    w.DrawLine(left, midy + 4, right, midy, 2, c);
}

void DrawMenuCheckGlyph(Draw& w, const Rect& r, Color c)
{
    UiPaintIndicatorCheckStroke(w, r, c, 2, max(1, r.GetWidth() / 5), 0,
                                max(1, r.GetHeight() / 5) + 1,
                                max(1, r.GetWidth() / 5) + 1,
                                max(1, r.GetHeight() / 5));
}

void DrawMenuRadioGlyph(Draw& w, const Rect& r, Color c)
{
    UiPaintIndicatorRadioDot(w, r, c, max(1, r.GetWidth() / 4), 100, 1);
}

Color BlendDisabledMenu(Color c)
{
    return Blend(c, SColorPaper(), 160);
}

}

UiMenu::PopupLevel::PopupLevel()
{
    BackPaint();
    Add(vscroll_);
    vscroll_.WhenScroll = [=] { Refresh(); };
    SetFrame(NullFrame());
    WantFocus();
}

void UiMenu::PopupLevel::Init(UiMenu* owner, int level)
{
    owner_ = owner;
    level_ = level;
}

void UiMenu::PopupLevel::SetParentNode(UiMenuNodeRef parent)
{
    parent_node_ = parent;
    hot_index_ = -1;
    pressed_index_ = -1;
    vscroll_.Set(0);
    for(int i = 0; i < GetItemCount(); i++) {
        UiMenuNodeRef child = owner_->GetChildNode(parent_node_, i);
        if(child.IsValid() && owner_->IsSelectable(owner_->GetModel().Get(child), child)) {
            hot_index_ = i;
            break;
        }
    }
    SyncScrollBar();
    Refresh();
}

void UiMenu::PopupLevel::SetHotIndex(int index)
{
    hot_index_ = index;
    EnsureVisible(index);
    Refresh();
}

int UiMenu::PopupLevel::GetItemCount() const
{
    return owner_ && owner_->model_ ? owner_->model_->GetChildCount(parent_node_) : 0;
}

Rect UiMenu::PopupLevel::GetRowRect(int index) const
{
    const Style& style = owner_->GetEffectiveStyle();
    Rect r = GetSize();
    int sb = vscroll_.IsShown() ? ScrollBarSize() : 0;
    int scroll = vscroll_.IsShown() ? vscroll_.Get() : 0;
    int extent = style.row_height + max(0, style.item_spacing);
    int row_top = style.popup_padding - scroll + index * extent;
    return Rect(style.popup_padding, row_top, r.right - style.popup_padding - sb, row_top + style.row_height);
}

int UiMenu::PopupLevel::HitTestRow(Point p) const
{
    if(!owner_ || !Rect(GetSize()).Contains(p))
        return -1;
    for(int i = GetVisibleStart(), end = min(GetItemCount(), GetVisibleStart() + GetVisibleCount() + 1); i < end; i++) {
        Rect rr = GetRowRect(i);
        if(rr.Contains(p))
            return i;
    }
    return -1;
}

bool UiMenu::PopupLevel::IsOverRow(Point p) const
{
    return HitTestRow(p) >= 0;
}

int UiMenu::PopupLevel::GetVisibleStart() const
{
    const Style& style = owner_->GetEffectiveStyle();
    int scroll = vscroll_.IsShown() ? vscroll_.Get() : 0;
    int extent = max(DPI(18), style.row_height + max(0, style.item_spacing));
    return max(0, scroll / max(1, extent));
}

int UiMenu::PopupLevel::GetVisibleCount() const
{
    const Style& style = owner_->GetEffectiveStyle();
    int h = max(0, GetSize().cy - style.popup_padding * 2);
    return max(1, h / max(DPI(18), style.row_height + max(0, style.item_spacing)));
}

Size UiMenu::PopupLevel::ComputeNaturalSize() const
{
    return owner_ ? owner_->ComputePopupSize(parent_node_) : Size(DPI(180), DPI(80));
}

void UiMenu::PopupLevel::EnsureVisible(int index)
{
    if(index < 0 || index >= GetItemCount())
        return;
    const Style& style = owner_->GetEffectiveStyle();
    int top = index * style.row_height;
    int bottom = top + style.row_height;
    int page = max(style.row_height, GetSize().cy - style.popup_padding * 2);
    if(top < vscroll_.Get())
        vscroll_.Set(top);
    else if(bottom > vscroll_.Get() + page)
        vscroll_.Set(bottom - page);
}

void UiMenu::PopupLevel::SyncScrollBar()
{
    const Style& style = owner_->GetEffectiveStyle();
    int total = GetItemCount() * style.row_height;
    int page = max(style.row_height, GetSize().cy - style.popup_padding * 2);
    bool show = total > page;
    vscroll_.Show(show);
    if(show) {
        vscroll_.Set(0);
        vscroll_.SetTotal(total);
        vscroll_.SetPage(page);
        vscroll_.SetLine(style.row_height);
    }
    else
        vscroll_.Set(0);
    Layout();
}

void UiMenu::PopupLevel::SyncWindowRegion()
{
    Refresh();
    Sync();
    UpdateRefresh();
    Update();
}

void UiMenu::PopupLevel::Paint(Draw& w)
{
    if(!owner_)
        return;
    const Style& style = owner_->GetEffectiveStyle();
    Size sz = GetSize();
    ImageDraw ib(sz.cx, sz.cy);
    Draw& dw = ib;

    dw.DrawRect(sz, style.popup_bg);
    dw.DrawRect(0, 0, sz.cx, 1, style.palette.frame[ST_NORMAL]);
    dw.DrawRect(0, sz.cy - 1, sz.cx, 1, style.palette.frame[ST_NORMAL]);
    dw.DrawRect(0, 0, 1, sz.cy, style.palette.frame[ST_NORMAL]);
    dw.DrawRect(sz.cx - 1, 0, 1, sz.cy, style.palette.frame[ST_NORMAL]);

    int start = GetVisibleStart();
    int end = min(GetItemCount(), start + GetVisibleCount() + 1);
    for(int i = start; i < end; i++) {
        UiMenuNodeRef node = owner_->GetChildNode(parent_node_, i);
        if(!node.IsValid())
            continue;
        owner_->PaintMenuRow(dw, GetRowRect(i), node, owner_->GetModel().Get(node), i == hot_index_, i == pressed_index_, false);
    }
    w.DrawImage(0, 0, ib);
}

void UiMenu::PopupLevel::Layout()
{
    int sb = vscroll_.IsShown() ? ScrollBarSize() : 0;
    vscroll_.SetRect(GetSize().cx - sb, 0, sb, GetSize().cy);
}

void UiMenu::PopupLevel::LeftDown(Point p, dword)
{
    SetFocus();
    int row = HitTestRow(p);
    if(row < 0)
        return;
    pressed_index_ = row;
    hot_index_ = row;
    UiMenuNodeRef node = owner_->GetChildNode(parent_node_, row);
    if(!node.IsValid())
        return;
    const UiMenuItem& item = owner_->GetModel().Get(node);
    if(!owner_->IsSelectable(item, node)) {
        Refresh();
        return;
    }
    if(owner_->HasSubMenu(node))
        owner_->OpenSubMenu(level_, row);
    else
        owner_->ActivateItem(node);
    pressed_index_ = -1;
    Refresh();
}

void UiMenu::PopupLevel::MouseMove(Point p, dword)
{
    int row = HitTestRow(p);
    if(row != hot_index_) {
        hot_index_ = row;
        Refresh();
    }
    if(row >= 0) {
        UiMenuNodeRef node = owner_->GetChildNode(parent_node_, row);
        if(node.IsValid() && owner_->HasSubMenu(node) && owner_->IsSelectable(owner_->GetModel().Get(node), node))
            owner_->OpenSubMenu(level_, row);
        else
            owner_->CloseLevelsFrom(level_ + 1);
    }
}

void UiMenu::PopupLevel::MouseLeave()
{
    if(hot_index_ >= 0) {
        hot_index_ = -1;
        Refresh();
    }
}

void UiMenu::PopupLevel::MouseWheel(Point, int zdelta, dword)
{
    if(!vscroll_.IsShown())
        return;
    int extent = owner_->GetEffectiveStyle().row_height;
    int step = extent * max(1, GetVisibleCount() / 2);
    vscroll_.Set(vscroll_.Get() - sgn(zdelta) * step);
    Refresh();
}

bool UiMenu::PopupLevel::Key(dword key, int)
{
    if(owner_)
        owner_->HandlePopupKey(level_, key);
    return true;
}

void UiMenu::PopupLevel::Deactivate()
{
    Ctrl::Deactivate();
    if(owner_)
        owner_->OnPopupDeactivate(level_);
}

const UiMenu::Style& UiMenu::StyleDefault()
{
    static Style s;
    ONCELOCK {
        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(White());
            s.palette.frame[i] = Color(226, 232, 240);
            s.palette.ink[i] = Color(17, 24, 39);
            s.palette.icon[i] = Color(100, 116, 139);
        }
        s.palette.face[ST_HOT] = UiFill::Solid(Color(248, 250, 252));
        s.palette.face[ST_PRESSED] = UiFill::Solid(Color(241, 245, 249));
        s.metrics = StyledMetrics();
        s.metrics.face_enabled = true;
        s.metrics.frame_enabled = true;
        s.metrics.frame_width = DPI(1);
        s.metrics.radius = DPI(8);
        s.metrics.content_margin = Rect(DPI(6), DPI(6), DPI(6), DPI(6));
        s.skin = StyledSkin();
    }
    return s;
}

UiMenu::UiMenu()
{
    model_ = &internal_model_;
    BackPaint();
    WantFocus();
    SyncThemeStyle();
}

UiMenu::Style& UiMenu::StyleEdit()
{
    if(!has_style_override_) {
        style_ = GetEffectiveStyle();
        has_style_override_ = true;
    }
    theme_revision_ = 0;
    return style_;
}

const UiMenu::Style& UiMenu::GetEffectiveStyle() const
{
    if(has_style_override_)
        return style_;
    const_cast<UiMenu*>(this)->SyncThemeStyle();
    return themed_style_;
}

void UiMenu::SyncThemeStyle()
{
    if(has_style_override_)
        return;
    uint64 rev = UiTheme::GetRevision();
    if(theme_revision_ == rev)
        return;
    themed_style_ = UiTheme::ResolveMenu();
    theme_revision_ = rev;
}

void UiMenu::OnStyleChanged()
{
    RefreshLayout();
    Refresh();
}

UiMenu& UiMenu::SetStyle(const Style& s)
{
    style_ = s;
    has_style_override_ = true;
    OnStyleChanged();
    return *this;
}

UiMenu& UiMenu::ClearStyleOverride()
{
    has_style_override_ = false;
    theme_revision_ = 0;
    OnStyleChanged();
    return *this;
}

UiMenu& UiMenu::SetModel(UiMenuModel& model)
{
    if(model_ == &model)
        return *this;
    CloseMenu();
    model_ = &model;
    BindModel(model);
    model_revision_ = -1;
    hot_top_index_ = -1;
    SyncModel();
    RefreshLayout();
    Refresh();
    return *this;
}

UiMenu& UiMenu::UseInternalModel()
{
    return SetModel(internal_model_);
}

UiMenu& UiMenu::SetMenuBarMode(bool on)
{
    if(menu_bar_mode_ == on)
        return *this;
    CloseMenu();
    menu_bar_mode_ = on;
    hot_top_index_ = -1;
    RefreshLayout();
    Refresh();
    return *this;
}

void UiMenu::SyncModel()
{
    if(!model_)
        return;
    int rev = model_->GetRevision();
    if(rev == model_revision_)
        return;
    model_revision_ = rev;
    if(hot_top_index_ >= GetTopItemCount())
        hot_top_index_ = -1;
    if(IsMenuOpen())
        CloseMenu();
}

void UiMenu::BindModel(UiMenuModel& model)
{
    for(int i = 0; i < bound_models_.GetCount(); i++) {
        if(bound_models_[i] == &model)
            return;
    }

    bound_models_.Add(&model);
    Ptr<UiMenu> self = this;
    UiMenuModel* observed = &model;
    model.WhenChange << [self, observed](const UiModelChange& ch) {
        if(self)
            self->OnBoundModelChange(observed, ch);
    };
}

void UiMenu::OnBoundModelChange(UiMenuModel* observed, const UiModelChange&)
{
    if(model_ != observed)
        return;
    SyncModel();
    RefreshLayout();
    Refresh();
}

UiMenu& UiMenu::PopUp(Ctrl* owner, Point screen_pt)
{
    popup_owner_ = owner;
    popup_origin_ = screen_pt;
    OpenRootPopup(screen_pt);
    return *this;
}

UiMenu& UiMenu::CloseMenu()
{
    EndSession(true);
    return *this;
}

void UiMenu::BeginSession()
{
    KillTimeCallback(VERIFY_SESSION_CB);
    session_verifying_ = false;
    if(!session_open_) {
        session_open_ = true;
        WhenOpen();
    }
}

void UiMenu::EndSession(bool notify_close)
{
    KillTimeCallback(VERIFY_SESSION_CB);
    session_verifying_ = false;
    bool was_open = session_open_ || popup_levels_.GetCount() > 0;
    session_open_ = false;
    session_switching_ = false;
    CloseLevelsFrom(0);
    if(notify_close && was_open)
        WhenClose();
}

void UiMenu::ScheduleSessionVerify()
{
    if(closing_all_ || session_switching_)
        return;
    session_verifying_ = true;
    KillTimeCallback(VERIFY_SESSION_CB);
    SetTimeCallback(0, [=] { VerifySessionState(); }, VERIFY_SESSION_CB);
}

void UiMenu::VerifySessionState()
{
    session_verifying_ = false;
    if(closing_all_ || session_switching_ || !session_open_)
        return;
    if(IsSessionTarget(Ctrl::GetFocusCtrl()) || IsSessionTarget(Ctrl::GetActiveCtrl()))
        return;
    if(IsMenuCtrl(Ctrl::GetMouseCtrl()))
        return;
    EndSession(true);
}

bool UiMenu::IsSessionTarget(const Ctrl* ctrl) const
{
    return IsMenuCtrl(ctrl);
}

void UiMenu::SuppressTopHoverUntilMouseMoves()
{
    suppress_top_hover_until_mouse_moves_ = true;
    suppressed_hover_mouse_pos_ = GetMousePos();
}

void UiMenu::AfterPopupOpen(PopupLevel& popup)
{
    Rect r = popup.GetRect();
    popup.SetRect(r);
    popup.RefreshFrame();
    popup.Refresh();
    popup.Sync();
    Ctrl::ProcessEvents();
    popup.RefreshFrame();
    popup.Refresh();
    popup.Sync();
}

void UiMenu::CloseLevelsFrom(int level, bool clear_root_state)
{
    if(level < 0)
        level = 0;
    if(level >= popup_levels_.GetCount())
        return;
    closing_all_ = true;
    for(int i = popup_levels_.GetCount() - 1; i >= level; i--)
        popup_levels_[i].Close();
    popup_levels_.SetCount(level);
    closing_all_ = false;
    if(level == 0 && clear_root_state) {
        hot_top_index_ = -1;
        active_top_index_ = -1;
        suppress_top_hover_until_mouse_moves_ = false;
        suppressed_hover_mouse_pos_ = Point(-99999, -99999);
    }
    Refresh();
}

void UiMenu::OpenRootPopup(Point screen_pt)
{
    SyncModel();
    if(!model_ || model_->GetChildCount(model_->Root()) == 0)
        return;

    session_switching_ = true;
    BeginSession();
    CloseLevelsFrom(0, false);
    PopupLevel& level = popup_levels_.Add();
    level.Init(this, 0);
    level.SetParentNode(model_->Root());
    Size sz = level.ComputeNaturalSize();
    Rect screen = GetVirtualScreenArea();
    Point pos = screen_pt;
    if(pos.x + sz.cx > screen.right)
        pos.x = max(screen.left, screen.right - sz.cx);
    if(pos.y + sz.cy > screen.bottom)
        pos.y = max(screen.top, screen.bottom - sz.cy);
    level.SetRect(pos.x, pos.y, sz.cx, sz.cy);
    level.PopUp(popup_owner_, true, true, false);
    GuiPlatformAfterMenuPopUp();
    level.SyncScrollBar();
    if(level.GetHotIndex() >= 0)
        level.SetHotIndex(level.GetHotIndex());
    AfterPopupOpen(level);
    if(menu_bar_mode_)
        SetFocus();
    else
        level.SetFocus();
    session_switching_ = false;
}

void UiMenu::OpenMenuBarPopup(int index)
{
    if(!menu_bar_mode_ || !model_)
        return;
    UiMenuNodeRef node = GetChildNode(model_->Root(), index);
    if(!node.IsValid())
        return;
    if(!HasSubMenu(node)) {
        active_top_index_ = index;
        hot_top_index_ = index;
        ActivateItem(node);
        return;
    }

    session_switching_ = true;
    BeginSession();
    Rect r = GetTopItemRect(index);
    Point pt = r.BottomLeft() + GetScreenView().TopLeft();
    CloseLevelsFrom(0, false);
    active_top_index_ = index;
    hot_top_index_ = index;

    PopupLevel& level = popup_levels_.Add();
    level.Init(this, 0);
    level.SetParentNode(node);
    Size sz = level.ComputeNaturalSize();
    Rect screen = GetVirtualScreenArea();
    int x = pt.x;
    int y = pt.y;
    if(x + sz.cx > screen.right)
        x = max(screen.left, screen.right - sz.cx);
    if(y + sz.cy > screen.bottom)
        y = max(screen.top, screen.bottom - sz.cy);
    level.SetRect(x, y, sz.cx, sz.cy);
    level.PopUp(this, true, true, false);
    GuiPlatformAfterMenuPopUp();
    level.SyncScrollBar();
    if(level.GetHotIndex() >= 0)
        level.SetHotIndex(level.GetHotIndex());
    AfterPopupOpen(level);
    level.SetFocus();
    session_switching_ = false;
    Refresh();
}

void UiMenu::OpenSubMenu(int level_index, int index)
{
    if(level_index < 0 || level_index >= popup_levels_.GetCount())
        return;
    PopupLevel& level = popup_levels_[level_index];
    UiMenuNodeRef parent = level.GetParentNode();
    UiMenuNodeRef node = GetChildNode(parent, index);
    if(!node.IsValid() || !HasSubMenu(node)) {
        CloseLevelsFrom(level_index + 1, false);
        return;
    }
    if(level_index + 1 < popup_levels_.GetCount()) {
        const PopupLevel& existing = popup_levels_[level_index + 1];
        if(existing.GetParentNode().id == node.id)
            return;
    }

    session_switching_ = true;
    CloseLevelsFrom(level_index + 1, false);
    PopupLevel& child = popup_levels_.Add();
    child.Init(this, level_index + 1);
    child.SetParentNode(node);
    Size sz = child.ComputeNaturalSize();
    Rect anchor = level.GetRowRect(index);
    Rect screen_anchor = anchor + level.GetScreenView().TopLeft();
    Rect screen = GetVirtualScreenArea();
    int x = screen_anchor.right - GetEffectiveStyle().submenu_overlap;
    int y = screen_anchor.top;
    if(x + sz.cx > screen.right)
        x = max(screen.left, screen_anchor.left - sz.cx + GetEffectiveStyle().submenu_overlap);
    if(y + sz.cy > screen.bottom)
        y = max(screen.top, screen.bottom - sz.cy);
    child.SetRect(x, y, sz.cx, sz.cy);
    Ctrl *submenu_owner = menu_bar_mode_ ? static_cast<Ctrl *>(this)
                                         : (popup_owner_ ? ~popup_owner_ : static_cast<Ctrl *>(&level));
    child.PopUp(submenu_owner, false, true, false);
    GuiPlatformAfterMenuPopUp();
    child.SyncScrollBar();
    if(child.GetHotIndex() >= 0)
        child.SetHotIndex(child.GetHotIndex());
    AfterPopupOpen(child);
    child.SetFocus();
    session_switching_ = false;
    WhenSubMenuOpen(node);
}

void UiMenu::ActivateItem(UiMenuNodeRef node)
{
    if(!node.IsValid())
        return;
    UiMenuItem item = model_->Get(node);
    if(!IsSelectable(item, node) || HasSubMenu(node))
        return;
    if(item.checkable && !item.radio) {
        item.checked = !item.checked;
        model_->Set(node, item);
    }
    else if(item.radio && !item.checked) {
        UiMenuNodeRef parent = model_->GetParent(node);
        for(int i = 0; i < model_->GetChildCount(parent); i++) {
            UiMenuNodeRef sibling = model_->GetChild(parent, i);
            if(!sibling.IsValid())
                continue;
            UiMenuItem sib = model_->Get(sibling);
            if(sib.radio && sib.checked) {
                sib.checked = false;
                model_->Set(sibling, sib);
            }
        }
        item.checked = true;
        model_->Set(node, item);
    }

    UiMenuItem fire = model_->Get(node);
    EndSession(true);
    Ptr<UiMenu> self = this;
    PostCallback([self, node, fire] {
        if(self && self->WhenAction)
            self->WhenAction(node, fire);
    });
}

void UiMenu::HandlePopupKey(int level, dword key)
{
    if(level < 0 || level >= popup_levels_.GetCount())
        return;
    PopupLevel& popup = popup_levels_[level];
    int count = model_->GetChildCount(popup.GetParentNode());
    if(count <= 0)
        return;

    auto next_selectable = [&](int start, int delta) {
        int i = start;
        for(int step = 0; step < count; step++) {
            i += delta;
            if(i < 0)
                i = count - 1;
            if(i >= count)
                i = 0;
            UiMenuNodeRef node = GetChildNode(popup.GetParentNode(), i);
            if(node.IsValid() && IsSelectable(model_->Get(node), node))
                return i;
        }
        return -1;
    };

    int hot = popup.GetHotIndex();
    switch(key) {
    case K_UP:
        popup.SetHotIndex(next_selectable(hot < 0 ? 0 : hot, -1));
        break;
    case K_DOWN:
        popup.SetHotIndex(next_selectable(hot < 0 ? -1 : hot, 1));
        break;
    case K_HOME:
        for(int i = 0; i < count; i++) {
            UiMenuNodeRef node = GetChildNode(popup.GetParentNode(), i);
            if(node.IsValid() && IsSelectable(model_->Get(node), node)) {
                popup.SetHotIndex(i);
                break;
            }
        }
        break;
    case K_END:
        for(int i = count - 1; i >= 0; i--) {
            UiMenuNodeRef node = GetChildNode(popup.GetParentNode(), i);
            if(node.IsValid() && IsSelectable(model_->Get(node), node)) {
                popup.SetHotIndex(i);
                break;
            }
        }
        break;
    case K_RIGHT:
        if(hot >= 0) {
            UiMenuNodeRef node = GetChildNode(popup.GetParentNode(), hot);
            if(node.IsValid() && HasSubMenu(node))
                OpenSubMenu(level, hot);
            else if(menu_bar_mode_ && GetTopItemCount() > 0 && active_top_index_ >= 0) {
                SuppressTopHoverUntilMouseMoves();
                OpenMenuBarPopup((active_top_index_ + 1) % GetTopItemCount());
            }
        }
        else if(menu_bar_mode_ && GetTopItemCount() > 0 && active_top_index_ >= 0) {
            SuppressTopHoverUntilMouseMoves();
            OpenMenuBarPopup((active_top_index_ + 1) % GetTopItemCount());
        }
        break;
    case K_LEFT:
        if(menu_bar_mode_ && GetTopItemCount() > 0 && active_top_index_ >= 0) {
            SuppressTopHoverUntilMouseMoves();
            OpenMenuBarPopup((active_top_index_ + GetTopItemCount() - 1) % GetTopItemCount());
        }
        else if(level > 0)
            CloseLevelsFrom(level);
        else
            CloseMenu();
        break;
    case K_ENTER:
    case K_SPACE:
        if(hot >= 0) {
            UiMenuNodeRef node = GetChildNode(popup.GetParentNode(), hot);
            if(node.IsValid()) {
                if(HasSubMenu(node))
                    OpenSubMenu(level, hot);
                else
                    ActivateItem(node);
            }
        }
        break;
    case K_ESCAPE:
        CloseMenu();
        break;
    }
}

void UiMenu::HandleMenuBarKey(dword key)
{
    int count = GetTopItemCount();
    if(count <= 0)
        return;
    if(hot_top_index_ < 0)
        hot_top_index_ = 0;
    switch(key) {
    case K_LEFT:
        hot_top_index_ = (hot_top_index_ + count - 1) % count;
        break;
    case K_RIGHT:
        hot_top_index_ = (hot_top_index_ + 1) % count;
        break;
    case K_DOWN:
    case K_ENTER:
    case K_SPACE: {
        OpenMenuBarPopup(hot_top_index_);
        break;
    }
    case K_ESCAPE:
        CloseMenu();
        break;
    }
    Refresh();
}

int UiMenu::GetHotTopIndex() const
{
    return hot_top_index_;
}

void UiMenu::SetHotTopIndex(int index)
{
    hot_top_index_ = index;
    Refresh();
}

int UiMenu::GetTopItemCount() const
{
    return model_ ? model_->GetChildCount(model_->Root()) : 0;
}

Rect UiMenu::GetTopItemRect(int index) const
{
    Rect content = UiStyledInnerRect(GetSize(), GetEffectiveStyle().metrics, GetEffectiveStyle().skin);
    int x = content.left;
    for(int i = 0; i < index; i++)
        x += MeasureTopItem(GetChildNode(model_->Root(), i)).cx;
    Size sz = MeasureTopItem(GetChildNode(model_->Root(), index));
    return RectC(x, content.top, sz.cx, max(GetEffectiveStyle().bar_height, content.GetHeight()));
}

int UiMenu::HitTestTopItem(Point p) const
{
    if(!menu_bar_mode_)
        return -1;
    for(int i = 0; i < GetTopItemCount(); i++)
        if(GetTopItemRect(i).Contains(p))
            return i;
    return -1;
}

Size UiMenu::MeasureTopItem(UiMenuNodeRef node) const
{
    if(!node.IsValid())
        return Size(DPI(60), GetEffectiveStyle().bar_height);
    const Style& style = GetEffectiveStyle();
    const UiMenuItem& item = model_->Get(node);
    Size tsz = GetTextSize(item.text, style.bar_font);
    return Size(style.left_padding + tsz.cx + style.right_padding, style.bar_height);
}

Size UiMenu::ComputePopupSize(UiMenuNodeRef parent) const
{
    const Style& style = GetEffectiveStyle();
    int width = style.popup_min_width;
    int count = model_->GetChildCount(parent);
    for(int i = 0; i < count; i++) {
        UiMenuNodeRef node = model_->GetChild(parent, i);
        if(!node.IsValid())
            continue;
        const UiMenuItem& item = model_->Get(node);
        int row_w = style.left_padding + style.right_padding;
        if(style.show_checks)
            row_w += style.check_size + style.content_gap;
        if(style.show_icons && !IsNull(item.icon))
            row_w += style.icon_size + style.content_gap;
        row_w += GetTextSize(item.text, style.font).cx;
        if(!item.description.IsEmpty() && style.show_descriptions)
            row_w = max(row_w, style.left_padding + style.right_padding + GetTextSize(item.description, style.font).cx);
        String right = GetRightText(item);
        if(!right.IsEmpty())
            row_w += style.right_gap + GetTextSize(right, style.font).cx;
        if(HasSubMenu(node))
            row_w += style.right_gap + style.arrow_size;
        width = max(width, row_w + style.popup_padding * 2 + DPI(14));
    }
    int content_h = count * style.row_height;
    int height = min(style.popup_max_height, max(style.row_height, content_h + style.popup_padding * 2));
    return Size(width, height);
}

String UiMenu::GetRightText(const UiMenuItem& item) const
{
    if(!item.shortcut_text.IsEmpty())
        return item.shortcut_text;
    return item.right_text;
}

bool UiMenu::IsSelectable(const UiMenuItem& item, UiMenuNodeRef) const
{
    return item.visible && item.enabled && !item.separator;
}

bool UiMenu::HasSubMenu(UiMenuNodeRef node) const
{
    return node.IsValid() && model_ && model_->GetChildCount(node) > 0;
}

void UiMenu::PaintTopBar(Draw& w) const
{
    const Style& style = GetEffectiveStyle();
    w.DrawRect(GetSize(), style.bar_bg);
    for(int i = 0; i < GetTopItemCount(); i++) {
        UiMenuNodeRef node = GetChildNode(model_->Root(), i);
        if(!node.IsValid())
            continue;
        bool hot = i == hot_top_index_ || i == active_top_index_;
        bool pressed = popup_levels_.GetCount() > 0 && i == active_top_index_;
        PaintMenuRow(w, GetTopItemRect(i), node, model_->Get(node), hot, pressed, true);
    }
}

void UiMenu::PaintMenuRow(Draw& w, const Rect& row, UiMenuNodeRef node, const UiMenuItem& item, bool hot, bool pressed, bool top_bar) const
{
    if(row.IsEmpty() || !item.visible)
        return;
    const Style& style = GetEffectiveStyle();
    Rect rr = row.Deflated(top_bar ? 0 : DPI(1), top_bar ? DPI(1) : 0);

    if(!top_bar)
        w.DrawRect(rr, style.popup_bg);

    if(!top_bar && item.separator_before && style.show_separators)
        w.DrawRect(rr.left, rr.top, rr.GetWidth(), 1, style.separator_color);

    if(item.separator) {
        if(style.show_separators)
            w.DrawRect(rr.left + style.left_padding, rr.top + rr.GetHeight() / 2, rr.GetWidth() - style.left_padding * 2, 1, style.separator_color);
        return;
    }

    if(hot || pressed) {
        Color bg = pressed ? style.pressed_bg : (top_bar ? style.active_bar_bg : style.hot_bg);
        Color frame = pressed ? style.pressed_frame : style.hot_frame;
        w.DrawRect(rr, bg);
        if(!top_bar) {
            w.DrawRect(rr.left, rr.top, rr.GetWidth(), 1, frame);
            w.DrawRect(rr.left, rr.bottom - 1, rr.GetWidth(), 1, frame);
            w.DrawRect(rr.left, rr.top, 1, rr.GetHeight(), frame);
            w.DrawRect(rr.right - 1, rr.top, 1, rr.GetHeight(), frame);
        }
    }

    Font font = top_bar ? style.bar_font : style.font;
    Color ink = item.enabled ? style.item_ink : BlendDisabledMenu(style.disabled_ink);
    Color right_ink = item.enabled ? style.right_ink : BlendDisabledMenu(style.right_ink);

    if(top_bar) {
        Size tsz = GetTextSize(item.text, font);
        int ty = rr.top + max(0, (rr.GetHeight() - tsz.cy) / 2);
        w.DrawText(rr.left + style.left_padding, ty, item.text, font, ink);
        return;
    }

    int x = rr.left + style.left_padding;
    Rect check_rect = RectC(x, rr.top + max(0, (rr.GetHeight() - style.check_size) / 2), style.check_size, style.check_size);
    if(style.show_checks) {
        if(item.radio && item.checked)
            DrawMenuRadioGlyph(w, check_rect, style.check_color);
        else if(item.checkable && item.checked)
            DrawMenuCheckGlyph(w, check_rect, style.check_color);
        x = check_rect.right + style.content_gap;
    }

    if(style.show_icons && !IsNull(item.icon)) {
        Rect ir = RectC(x, rr.top + max(0, (rr.GetHeight() - style.icon_size) / 2), style.icon_size, style.icon_size);
        Color icon_ink = item.enabled ? style.item_ink : BlendDisabledMenu(style.disabled_ink);
                UiPaintStyledIcon(w, ir, item.icon, true, true, item.icon_render_mode, icon_ink, item.enabled);
        x = ir.right + style.content_gap;
    }

    String right = GetRightText(item);
    int right_edge = rr.right - style.right_padding;
    if(HasSubMenu(node))
        right_edge -= style.arrow_size + style.content_gap;
    if(!right.IsEmpty()) {
        Size rsz = GetTextSize(right, font);
        w.DrawText(max(x, right_edge - rsz.cx), rr.top + max(0, (rr.GetHeight() - rsz.cy) / 2), right, font, right_ink);
        right_edge -= rsz.cx + style.right_gap;
    }

    DrawTextEllipsis(w, x, rr.top + max(0, (rr.GetHeight() - font.GetHeight()) / 2), max(0, right_edge - x), item.text, "...", font, ink);

    if(HasSubMenu(node)) {
        Rect ar = RectC(rr.right - style.right_padding - style.arrow_size, rr.top + max(0, (rr.GetHeight() - style.arrow_size) / 2), style.arrow_size, style.arrow_size);
        DrawMenuArrowGlyph(w, ar, style.arrow_color);
    }
}

UiMenuNodeRef UiMenu::GetChildNode(UiMenuNodeRef parent, int index) const
{
    return model_ ? model_->GetChild(parent, index) : UiMenuNodeRef{-1};
}

void UiMenu::OnPopupDeactivate(int level)
{
    if(closing_all_)
        return;
    if(level <= popup_levels_.GetCount() - 1)
        ScheduleSessionVerify();
}

bool UiMenu::IsMenuCtrl(const Ctrl* ctrl) const
{
    const Ctrl* c = ctrl;
    int guard = 0;
    while(c && guard++ < 64) {
        if(c == this)
            return true;
        for(int i = 0; i < popup_levels_.GetCount(); i++)
            if(c == &popup_levels_[i])
                return true;
        const Ctrl* next = c->GetParent();
        if(!next)
            next = c->GetOwner();
        if(next == c)
            break;
        c = next;
    }
    return false;
}

void UiMenu::Paint(Draw& w)
{
    SyncModel();
    if(!menu_bar_mode_)
        return;
    const Style& style = GetEffectiveStyle();
    UiPaintStyledSurface(w, GetSize(), style.palette, style.metrics, style.skin,
                         IsEnabled() ? ST_NORMAL : ST_DISABLED,
                         HasFocus(), false, false);
    PaintTopBar(w);
}

void UiMenu::Layout()
{
}

Size UiMenu::GetMinSize() const
{
    if(!menu_bar_mode_)
        return Size(DPI(80), DPI(24));
    int w = 0;
    for(int i = 0; i < GetTopItemCount(); i++)
        w += MeasureTopItem(GetChildNode(model_->Root(), i)).cx;
    return UiStyledOuterSizeFromContent(Size(max(DPI(180), w), GetEffectiveStyle().bar_height), GetEffectiveStyle().metrics, GetEffectiveStyle().skin);
}

void UiMenu::LeftDown(Point p, dword)
{
    if(!menu_bar_mode_ || !model_)
        return;
    SetFocus();
    int index = HitTestTopItem(p);
    if(index < 0)
        return;
    hot_top_index_ = index;
    OpenMenuBarPopup(index);
    Refresh();
}

void UiMenu::MouseMove(Point p, dword)
{
    if(!menu_bar_mode_)
        return;
    int index = HitTestTopItem(p);
    if(GetMousePos() != suppressed_hover_mouse_pos_) {
        suppress_top_hover_until_mouse_moves_ = false;
        suppressed_hover_mouse_pos_ = Point(-99999, -99999);
    }
    if(index != hot_top_index_) {
        hot_top_index_ = index;
        Refresh();
    }
    if(index >= 0 && IsMenuOpen()) {
        if(index != active_top_index_ && !suppress_top_hover_until_mouse_moves_)
            OpenMenuBarPopup(index);
    }
}

void UiMenu::MouseLeave()
{
    suppress_top_hover_until_mouse_moves_ = false;
    if(menu_bar_mode_ && hot_top_index_ >= 0 && !IsMenuOpen()) {
        hot_top_index_ = -1;
        Refresh();
    }
}

bool UiMenu::Key(dword key, int)
{
    if(IsMenuOpen()) {
        HandlePopupKey(popup_levels_.GetCount() - 1, key);
        return true;
    }
    if(menu_bar_mode_) {
        HandleMenuBarKey(key);
        return true;
    }
    return false;
}

void UiMenu::LostFocus()
{
    if(!HasFocusDeep() && !IsMenuOpen()) {
        hot_top_index_ = -1;
        active_top_index_ = -1;
    }
    else if(session_open_)
        ScheduleSessionVerify();
    Refresh();
}

}
