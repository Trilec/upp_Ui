#ifndef _Ui_UiTab_h_
#define _Ui_UiTab_h_

/*
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
        int  affordance_gap = DPI(4);
        int  affordance_size = DPI(12);
        int  min_tab_main = DPI(72);
        int  indicator_thickness = DPI(3);
        UiSpan indicator_span = LARGE;
        bool fill_tabs = false;
        UiTabVisual visual = UITAB_CLASSIC;

        void Serialize(Stream& s)
        {
            s % palette % metrics % skin
              % tab_palette % tab_metrics % tab_skin
              % tab_font
              % tab_extent % item_spacing % body_gap % tab_padding % strip_inset % content_gap
              % affordance_gap % affordance_size
              % min_tab_main % indicator_thickness;
            int sp = (int)indicator_span;
            s % sp;
            indicator_span = (UiSpan)sp;
            s % fill_tabs;
            int vv = (int)visual;
            s % vv;
            visual = (UiTabVisual)vv;
        }
    };

    static const Style& StyleDefault();

    UiTab();

    UiTab& SetStyle(const Style& s);
    UiTab& ClearStyleOverride();
    bool   HasStyleOverride() const { return has_style_override_; }
    const Style& GetStyle() const { return GetEffectiveStyle(); }

    StyledPalette& StyledPaletteRef() { return StyleEdit().palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().metrics; }
    StyledSkin&    StyledSkinRef()    { return StyleEdit().skin; }
    void OnStyleChanged();

    UiTab& SetPlacement(UiAlign side);
    UiAlign GetPlacement() const { return placement_; }
    UiTab& SetVisual(UiTabVisual visual);
    UiTabVisual GetVisual() const { return visual_; }

    UiTab& SetFillTabs(bool on = true) { StyleEdit().fill_tabs = on; RefreshLayout(); Refresh(); return *this; }
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
    bool has_style_override_ = false;
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
