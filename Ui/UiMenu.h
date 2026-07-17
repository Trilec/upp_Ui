#ifndef _Ui_UiMenu_h_
#define _Ui_UiMenu_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.
    UiMenu
    ======

    Purpose
    - Styled, model-driven menu and menu-bar control backed by UiMenuModel.

    Intent
    - Keep command/menu structure in the model while the control owns only
      popup stack, hot-item, anchor, and transient open-state behavior.
    - Render popup rows directly and share one menu core for popup and top-bar
      usage.

    Thread context
    - GUI thread only.

    Usage
    - Bind an external model with SetModel(...) or populate GetInternalModel().
    - Use SetMenuBarMode(true) for an embedded top bar or PopUp(...) for popup
      menu usage.

    Changelog
    - 2026-03: introduced as the first-pass Ui menu foundation.
    - 2026-03-31: fixed a Win32 submenu first-row paint artifact by buffering
      radio glyph rendering before compositing popup rows.
*/

#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>
#include <Ui/UiDataModels.h>

namespace Upp {

class UiMenu : public Ctrl, public CtrlStyled<UiMenu> {
public:
    typedef UiMenu CLASSNAME;

    struct Style : ChStyle<Style> {
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin skin;

        Font font = StdFont();
        Font bar_font = StdFont().Bold();
        int row_height = DPI(28);
        int bar_height = DPI(30);
        int icon_size = DPI(16);
        int check_size = DPI(14);
        int arrow_size = DPI(12);
        int left_padding = DPI(10);
        int right_padding = DPI(10);
        int content_gap = DPI(8);
        int item_spacing = 0;
        int right_gap = DPI(16);
        int popup_padding = DPI(6);
        int popup_min_width = DPI(180);
        int popup_max_height = DPI(320);
        int popup_shadow_margin = DPI(6);
        int submenu_overlap = DPI(4);
        bool show_icons = true;
        bool show_checks = true;
        bool show_descriptions = false;
        bool show_shortcuts = true;
        bool show_separators = true;

        Color popup_bg = SColorPaper();
        Color bar_bg = SColorFace();
        Color separator_color = Blend(SColorShadow(), SColorPaper(), 210);
        Color item_ink = SColorText();
        Color disabled_ink = SColorDisabled();
        Color right_ink = Color(100, 116, 139);
        Color hot_bg = Color(239, 246, 255);
        Color hot_frame = Color(191, 219, 254);
        Color pressed_bg = Color(219, 234, 254);
        Color pressed_frame = Color(96, 165, 250);
        Color active_bar_bg = Color(232, 242, 255);
        Color check_color = Color(17, 24, 39);
        Color arrow_color = Color(100, 116, 139);
        Color shadow_color = Color(148, 163, 184);

        void Serialize(Stream& s)
        {
            s % palette % metrics % skin
              % font % bar_font
              % row_height % bar_height % icon_size % check_size % arrow_size
              % left_padding % right_padding % content_gap % right_gap % item_spacing
              % popup_padding % popup_min_width % popup_max_height
              % popup_shadow_margin % submenu_overlap
              % show_icons % show_checks % show_descriptions % show_shortcuts % show_separators
              % popup_bg % bar_bg % separator_color
              % item_ink % disabled_ink % right_ink
              % hot_bg % hot_frame % pressed_bg % pressed_frame % active_bar_bg
              % check_color % arrow_color % shadow_color;
        }
    };

    static const Style& StyleDefault();

    UiMenu();

    StyledPalette& StyledPaletteRef() { return StyleEdit().palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().metrics; }
    StyledSkin& StyledSkinRef() { return StyleEdit().skin; }
    void OnStyleChanged();

    UiMenu& SetCustomStyle(const Style& s);
    UiMenu& ClearCustomStyle();
    bool HasCustomStyle() const { return has_custom_style_; }
    const Style& GetStyle() const { return GetEffectiveStyle(); }
    const Style& GetCustomStyle() const { return style_; }

    UiMenu& SetModel(UiMenuModel& model);
    UiMenu& UseInternalModel();
    UiMenu& EnableInternalMutation(bool on = true);
    bool IsInternalMutationEnabled() const { return internal_mutation_enabled_; }
    UiMenuModel& GetInternalModel() { return internal_model_; }
    UiMenuModel& GetModel() { return *model_; }
    const UiMenuModel& GetModel() const { return *model_; }

    UiMenu& SetMenuBarMode(bool on = true);
    bool IsMenuBarMode() const { return menu_bar_mode_; }

    UiMenu& PopUp(Ctrl* owner, Point screen_pt);
    UiMenu& PopUp(Point screen_pt) { return PopUp(GetActiveCtrl(), screen_pt); }
    UiMenu& CloseMenu();
    bool IsMenuOpen() const { return popup_levels_.GetCount() > 0; }

    virtual void Paint(Draw& w) override;
    virtual void Layout() override;
    virtual Size GetMinSize() const override;
    virtual void LeftDown(Point p, dword flags) override;
    virtual void MouseMove(Point p, dword flags) override;
    virtual void MouseLeave() override;
    virtual bool Key(dword key, int count) override;
    virtual void LostFocus() override;

    Event<UiMenuNodeRef, const UiMenuItem&> WhenAction;
    Event<UiMenuActionRequest&> WhenActionRequest;
    Event<> WhenOpen;
    Event<> WhenClose;
    Event<UiMenuNodeRef> WhenSubMenuOpen;
    Event<UiMenuNodeRef> WhenSubMenuClose;

private:
    class PopupLevel : public Ctrl {
    public:
        typedef PopupLevel CLASSNAME;

        PopupLevel();

        void Init(UiMenu* owner, int level);
        void SetParentNode(UiMenuNodeRef parent);
        void SetHotIndex(int index);
        int GetHotIndex() const { return hot_index_; }
        UiMenuNodeRef GetParentNode() const { return parent_node_; }
        int GetItemCount() const;
        bool IsOverRow(Point p) const;
        Rect GetRowRect(int index) const;
        int HitTestRow(Point p) const;
        int GetVisibleStart() const;
        int GetVisibleCount() const;
        Size ComputeNaturalSize() const;
        void EnsureVisible(int index);
        void SyncScrollBar();
        void SyncWindowRegion();

        virtual void Paint(Draw& w) override;
        virtual void Layout() override;
        virtual void LeftDown(Point p, dword flags) override;
        virtual void MouseMove(Point p, dword flags) override;
        virtual void MouseLeave() override;
        virtual void MouseWheel(Point p, int zdelta, dword keyflags) override;
        virtual bool Key(dword key, int count) override;
        virtual void Deactivate() override;

    private:
        UiMenu* owner_ = nullptr;
        int level_ = 0;
        UiMenuNodeRef parent_node_;
        int hot_index_ = -1;
        int pressed_index_ = -1;
        VScrollBar vscroll_;
    };

    Style& StyleEdit();
    const Style& GetEffectiveStyle() const;
    void SyncThemeStyle();
    void SyncModel();
    void BindModel(UiMenuModel& model);
    void OnBoundModelChange(UiMenuModel* observed, const UiModelChange& change);
    void BeginSession();
    void EndSession(bool notify_close = true);
    void ScheduleSessionVerify();
    void VerifySessionState();
    bool IsSessionTarget(const Ctrl* ctrl) const;
    void SuppressTopHoverUntilMouseMoves();
    void AfterPopupOpen(PopupLevel& popup);
    void CloseLevelsFrom(int level, bool clear_root_state = true);
    void OpenRootPopup(Point screen_pt);
    void OpenMenuBarPopup(int index);
    void OpenSubMenu(int level, int index);
    void ActivateItem(UiMenuNodeRef node);
    void HandlePopupKey(int level, dword key);
    void HandleMenuBarKey(dword key);
    int GetHotTopIndex() const;
    void SetHotTopIndex(int index);
    int GetTopItemCount() const;
    Rect GetTopItemRect(int index) const;
    int HitTestTopItem(Point p) const;
    Size MeasureTopItem(UiMenuNodeRef node) const;
    Size ComputePopupSize(UiMenuNodeRef parent) const;
    String GetRightText(const UiMenuItem& item) const;
    bool IsSelectable(const UiMenuItem& item, UiMenuNodeRef node) const;
    bool HasSubMenu(UiMenuNodeRef node) const;
    void PaintTopBar(Draw& w) const;
    void PaintMenuRow(Draw& w, const Rect& row, UiMenuNodeRef node, const UiMenuItem& item, bool hot, bool pressed, bool top_bar) const;
    UiMenuNodeRef GetChildNode(UiMenuNodeRef parent, int index) const;
    void OnPopupDeactivate(int level);
    bool IsMenuCtrl(const Ctrl* ctrl) const;

private:
    Style style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool has_custom_style_ = false;

    UiMenuModel internal_model_;
    UiMenuModel* model_ = nullptr;
    Vector<UiMenuModel*> bound_models_;
    mutable int model_revision_ = -1;

    // UiMenu has one control-relative timer. U++ timer ids are byte offsets
    // within the Ctrl object, so the default slot is the valid stable choice.
    static const int VERIFY_SESSION_CB = 0;

    bool menu_bar_mode_ = false;
    bool session_open_ = false;
    bool session_verifying_ = false;
    bool session_switching_ = false;
    bool internal_mutation_enabled_ = true;
    bool suppress_top_hover_until_mouse_moves_ = false;
    Point suppressed_hover_mouse_pos_ = Point(-99999, -99999);
    int hot_top_index_ = -1;
    int active_top_index_ = -1;
    int pressed_top_index_ = -1;
    Point popup_origin_ = Point(0, 0);
    Ptr<Ctrl> popup_owner_;
    bool closing_all_ = false;

    Array<PopupLevel> popup_levels_;
};

}

#endif

