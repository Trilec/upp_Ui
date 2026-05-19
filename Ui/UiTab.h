#ifndef _Ui_UiTab_h_
#define _Ui_UiTab_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.
    UiTab
    -----

    Purpose
    - Styled tab-strip and page container for the Ui control family.

    Intent
    - Public API uses explicit active-tab naming instead of generic Set/Get.
    - SetData/GetData mirrors the active tab index for generic control binding.

    Thread context
    - GUI thread only.

    Usage
    - Add pages with Add(...), switch with SetActiveTab(...), and observe
      user changes with WhenAction.

    Changelog
    - 2026-03: renamed active-tab accessors for release API cleanup.
    - 2026-04: standardized repeated-tab spacing as item_spacing and primary
      tab content spacing as content_gap.
*/

#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>

namespace Upp {

enum UiTabVisual : byte {
    UITAB_CLASSIC = 0,
    UITAB_UNDERLINE,
    UITAB_SEGMENTED,
    UITAB_RAIL,
    UITAB_DOCUMENT,
};

class UiTab : public Ctrl, public CtrlStyled<UiTab> {
public:
    typedef UiTab CLASSNAME;

    struct Style : ChStyle<Style> {
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin    skin;

        StyledPalette tab_palette;
        StyledMetrics tab_metrics;
        StyledSkin    tab_skin;

        Font tab_font = StdFont();

        int  tab_extent = DPI(32);
        int  item_spacing = DPI(4);
        int  body_gap = DPI(4);
        Rect tab_padding = Rect(DPI(10), DPI(6), DPI(10), DPI(6));
        Rect strip_inset = Rect(0, 0, 0, 0);
        int  content_gap = DPI(6);
        int  icon_size = 0;
        int  affordance_gap = DPI(4);
        int  min_tab_main = DPI(72);
        int  indicator_thickness = DPI(3);
        int  active_frame_width = DPI(2);
        int  open_corner_radius = 0;
        Color active_frame_color = Null;
        UiSpan indicator_span = LARGE;
        bool expand_tabs = false;
        bool fill_tabs = false; // Deprecated storage alias; prefer expand_tabs.
        bool active_tab_uses_body_face = true;
        UiTabVisual visual = UITAB_CLASSIC;

        void Serialize(Stream& s)
        {
            s % palette % metrics % skin
              % tab_palette % tab_metrics % tab_skin
              % tab_font
              % tab_extent % item_spacing % body_gap % tab_padding % strip_inset % content_gap
              % icon_size
              % affordance_gap
              % min_tab_main % indicator_thickness
              % active_frame_width % open_corner_radius % active_frame_color;
            int sp = (int)indicator_span;
            s % sp;
            indicator_span = (UiSpan)sp;
            s % expand_tabs % fill_tabs % active_tab_uses_body_face;
            int vv = (int)visual;
            s % vv;
            visual = (UiTabVisual)vv;
        }
    };

    static const Style& StyleDefault();

    UiTab();

    UiTab& SetCustomStyle(const Style& s);
    UiTab& ClearCustomStyle();
    bool   HasCustomStyle() const { return has_custom_style_; }
    const Style& GetStyle() const { return GetEffectiveStyle(); }
    const Style& GetCustomStyle() const { return style_; }

    StyledPalette& StyledPaletteRef() { return StyleEdit().palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().metrics; }
    StyledSkin&    StyledSkinRef()    { return StyleEdit().skin; }
    void OnStyleChanged();

    UiTab& SetPlacement(UiAlign side);
    UiAlign GetPlacement() const { return placement_; }
    UiTab& SetVisual(UiTabVisual visual);
    UiTabVisual GetVisual() const { return visual_; }

    UiTab& SetExpandTabs(bool on = true) { StyleEdit().expand_tabs = on; StyleEdit().fill_tabs = on; RefreshLayout(); Refresh(); return *this; }
    bool   IsExpandTabs() const { const Style& s = GetEffectiveStyle(); return s.expand_tabs || s.fill_tabs; }
    UiTab& SetFillTabs(bool on = true) { return SetExpandTabs(on); } // Compatibility alias; prefer SetExpandTabs.
    UiTab& SetActiveTabUsesBodyFace(bool on = true) { StyleEdit().active_tab_uses_body_face = on; Refresh(); return *this; }
    UiTab& EnableCloseButtons(bool on = true) { show_close_buttons_ = on; RefreshLayout(); Refresh(); return *this; }
    UiTab& EnableDragHandles(bool on = true)  { show_drag_handles_ = on; RefreshLayout(); Refresh(); return *this; }
    bool   IsCloseButtonsEnabled() const { return show_close_buttons_; }
    bool   IsDragHandlesEnabled() const  { return show_drag_handles_; }
    UiTab& EnableDragReorder(bool on = true) { drag_reorder_enabled_ = on; return *this; }
    bool   IsDragReorderEnabled() const { return drag_reorder_enabled_; }

    int Add(Ctrl& page, const String& title, const Image& icon = Image());
    int Add(const String& title, Ctrl& page, const Image& icon = Image()) { return Add(page, title, icon); }
    void Remove(int i);
    void Clear();

    int GetCount() const { return tabs_.GetCount(); }

    UiTab& SetTabText(int i, const String& text);
    UiTab& SetTabIcon(int i, const Image& icon);
    UiTab& EnableTab(int i, bool on = true);
    UiTab& SetTabClosable(int i, bool on = true);
    UiTab& SetTabDraggable(int i, bool on = true);
    bool   IsTabEnabled(int i) const;

    Ctrl&       GetPage(int i);
    const Ctrl& GetPage(int i) const;

    UiTab& SetActiveTab(int i);
    int    GetActiveTab() const { return active_; }

    virtual void  SetData(const Value& v) override;
    virtual Value GetData() const override;

    Event<> WhenAction;
    Event<int, int> WhenReorder;
    Event<int>      WhenClose;

    virtual Size GetMinSize() const override;
    virtual void Layout() override;
    virtual void Paint(Draw& w) override;
    virtual void LeftDown(Point p, dword flags) override;
    virtual void MouseMove(Point p, dword flags) override;
    virtual void LeftUp(Point p, dword flags) override;
    virtual void MouseLeave() override;
    virtual bool Key(dword key, int count) override;
    virtual void GotFocus() override;
    virtual void LostFocus() override;

private:
    struct TabItem : Moveable<TabItem> {
        String text;
        Image  icon;
        Ctrl*  page = nullptr;
        bool   enabled = true;
        bool   closable = true;
        bool   draggable = true;
        Size   text_size = Size(0, 0);
        Rect   tab_rect;
        Rect   close_rect;
        Rect   drag_rect;
    };

    void InvalidateStyleCache();
    Style& StyleEdit();
    void SyncThemeStyle();
    const Style& GetEffectiveStyle() const;
    void RebuildTextSize(TabItem& t);
    void RebuildAllTextSizes();
    void SyncPages();
    int  FindTabAt(Point p) const;
    int  FindCloseAt(Point p) const;
    int  FindDragAt(Point p) const;
    int  FindEnabled(int start, int step) const;
    bool IsHorizontal() const;
    void MoveTab(int from, int to);

private:
    Style       style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool has_custom_style_ = false;
    UiTabVisual visual_ = UITAB_CLASSIC;
    Vector<TabItem> tabs_;
    ParentCtrl  pane_;

    UiAlign placement_ = UiAlign::TOP;
    Rect    strip_rect_;
    Rect    tabs_rect_;

    int active_ = -1;
    int hot_ = -1;
    int hot_close_ = -1;
    int hot_drag_ = -1;

    bool show_close_buttons_ = false;
    bool show_drag_handles_ = false;

    bool drag_reorder_enabled_ = false;
    bool drag_candidate_ = false;
    bool dragging_ = false;
    int  drag_from_ = -1;
    int  drag_to_ = -1;
    Point drag_start_ = Point(0, 0);
};

}

#endif

