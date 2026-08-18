#ifndef _Ui_UiMenu_h_
#define _Ui_UiMenu_h_

/*
    UiMenu
    ======

    Purpose
    - Styled, model-driven menu and menu-bar control backed by UiMenuModel.

    Intent
    - Keep command/menu structure in UiMenuModel.
    - Reuse UiItemRender for ordinary popup icon/title/description/right-content
      composition while Menu retains check/radio/submenu/command chrome.
    - Keep popup renderer instances bounded to visible popup rows and prepare
      their geometry outside Paint().

    Thread context
    - GUI thread only.

    Model ownership
    - Model() always returns the model currently driving the Menu. The control
      owns an internal UiMenuModel by default; SetModel(...) switches to an
      external model and UseInternalModel() switches back without copying.
*/

#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>
#include <Ui/UiDataModels.h>
#include <Ui/UiItemRender.h>

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
    bool IsUsingInternalModel() const { return model_ == &internal_model_; }
    UiMenuModel& Model() { return *model_; }
    const UiMenuModel& Model() const { return *model_; }
    UiMenu& ClearModel() { Model().Clear(); return *this; }

    UiMenu& EnableInternalMutation(bool on = true);
    bool IsInternalMutationEnabled() const { return internal_mutation_enabled_; }

    UiMenu& SetItemRender(const UiItemRender& render);
    const UiItemRender& GetItemRender() const;
    int GetLiveItemRenderCount() const;
    int GetLastRenderLayoutCount() const;
    int GetLastPaintItemCount() const;

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

        struct RenderSlot {
            One<UiItemRender> render;
            int index = -1;
        };

        PopupLevel();

        void Init(UiMenu* owner, int level);
        void SetParentNode(UiMenuNodeRef parent);
        void SetHotIndex(int index);
        int GetHotIndex() const { return hot_index_; }
        UiMenuNodeRef GetParentNode() const { return parent_node_; }
        int GetItemCount() const;
        bool IsOverRow(Point p) const;
        Rect GetRowRect(int index) const;
        Rect GetContentRect(int index) const;
        int HitTestRow(Point p) const;
        int GetVisibleStart() const;
        int GetVisibleCount() const;
        Size ComputeNaturalSize() const;
        void EnsureVisible(int index);
        void SyncScrollBar();
        void SyncWindowRegion();
        void ResetRenderPool();
        void PrepareItemRenders();
        const UiItemRender* FindPreparedRender(int index) const;
        int GetLiveItemRenderCount() const { return render_pool_.GetCount(); }
        int GetLastRenderLayoutCount() const { return last_render_layout_count_; }
        int GetLastPaintItemCount() const { return last_paint_item_count_; }

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
        Array<RenderSlot> render_pool_;
        int prepared_first_ = -1;
        int prepared_last_ = -1;
        int last_render_layout_count_ = 0;
        mutable int last_paint_item_count_ = 0;
    };

    Style& StyleEdit();
    const Style& GetEffectiveStyle() const;
    void SyncThemeStyle();
    void SyncModel();
    void BindModel(UiMenuModel& model);
    void OnBoundModelChange(UiMenuModel* observed, const UiModelChange& change);

    void ConfigureDefaultItemRender();
    void EnsureItemRender();
    void ResetItemRenderPools();
    UiItemRenderData MakeMenuRenderData(UiMenuNodeRef node, const UiMenuItem& item) const;
    Rect GetPopupContentRect(const Rect& row, UiMenuNodeRef node, const UiMenuItem& item) const;
    void PaintPopupRowChrome(Draw& w, const Rect& row, UiMenuNodeRef node,
                             const UiMenuItem& item, bool hot, bool pressed) const;

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
    void PaintMenuRow(Draw& w, const Rect& row, UiMenuNodeRef node,
                      const UiMenuItem& item, bool hot, bool pressed, bool top_bar) const;
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
    UiModelObserverSet<UiMenuModel> bound_models_;
    mutable int model_revision_ = -1;

    One<UiItemRender> item_render_;
    bool custom_item_render_ = false;

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
