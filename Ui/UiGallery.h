#ifndef _Ui_UiGallery_h_
#define _Ui_UiGallery_h_

/*
    UiGallery
    =========

    Purpose
    - High-scale, model-backed gallery for uniformly sized visual items.

    Intent
    - Keep logical item count independent of Ctrl count: ordinary items are painted,
      hit-tested, selected, and scrolled arithmetically without one Ctrl per item.
    - Make the uniform-item fast path the default so very large models remain
      responsive and resize into a fluid wrapping column layout.
    - Keep semantic state in UiListModel; the gallery owns only interaction,
      viewport, selection, and transient visual state.
    - Expose a visible/prefetch range so expensive thumbnails can be prepared
      outside Paint() and only for useful items.

    Thread context
    - GUI thread only while bound to a live control.

    Usage
    - Bind an external UiListModel with SetModel(...) or populate GetInternalModel().
    - SetItemSize(), SetGap(), and SetInset() mirror the geometry vocabulary used
      by UiGridLayout while retaining model-view rather than child-layout semantics.
    - Use WhenVisibleRange to prepare lazy assets and WhenPaintItem for render-only
      custom item painting.
*/

#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>
#include <Ui/UiDataModels.h>
#include <Ui/UiModelView.h>
#include <Ui/UiScrollBar.h>

namespace Upp {

enum UiGallerySelectionMode : byte {
    UIGALLERYSEL_SINGLE = 0,
    UIGALLERYSEL_MULTI,
};

enum UiGalleryItemVisualState : byte {
    UIGALLERYITEM_NORMAL = 0,
    UIGALLERYITEM_HOT,
    UIGALLERYITEM_SELECTED,
    UIGALLERYITEM_DISABLED,
};

class UiGallery : public Ctrl, public CtrlStyled<UiGallery> {
public:
    typedef UiGallery CLASSNAME;

    struct Style : ChStyle<Style> {
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin skin;

        StyledPalette item_palette;
        StyledMetrics item_metrics;

        Font title_font = StdFont();
        Font description_font = StdFont();
        int icon_size = DPI(36);
        int content_gap = DPI(6);
        int text_gap = DPI(3);
        int item_padding = DPI(8);
        int metadata_size = DPI(8);
        int metadata_inset = DPI(7);
        Color description_ink = Color(100, 116, 139);
        Color metadata_default = Color(65, 167, 248);
        bool show_icons = true;
        bool show_description = true;
        bool show_metadata_marker = true;

        void Serialize(Stream& s)
        {
            s % palette % metrics % skin
              % item_palette % item_metrics
              % title_font % description_font
              % icon_size % content_gap % text_gap % item_padding
              % metadata_size % metadata_inset
              % description_ink % metadata_default
              % show_icons % show_description % show_metadata_marker;
        }
    };

    static const Style& StyleDefault();

    UiGallery();

    StyledPalette& StyledPaletteRef() { return StyleEdit().palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().metrics; }
    StyledSkin& StyledSkinRef() { return StyleEdit().skin; }
    void OnStyleChanged();

    UiGallery& SetCustomStyle(const Style& style);
    UiGallery& ClearCustomStyle();
    bool HasCustomStyle() const { return has_custom_style_; }
    const Style& GetStyle() const { return GetEffectiveStyle(); }
    const Style& GetCustomStyle() const { return style_; }

    UiGallery& SetModel(UiListModel& model);
    UiListModel& GetInternalModel() { return internal_model_; }
    const UiListModel& GetModel() const { return *model_; }
    UiListModel& GetModel() { return *model_; }

    // Uniform cell geometry is the high-scale default and V1 layout contract.
    UiGallery& SetItemSize(Size size);
    UiGallery& SetUnifiedItemSize(Size size) { return SetItemSize(size); }
    Size GetItemSize() const { return item_size_; }
    UiGallery& SetGap(int px);
    int GetGap() const { return gap_; }
    UiGallery& SetInset(int all);
    UiGallery& SetInset(int w, int h);
    UiGallery& SetInset(int l, int t, int r, int b);
    Rect GetInset() const { return inset_; }
    UiGallery& SetOverscanRows(int rows);
    int GetOverscanRows() const { return overscan_rows_; }

    UiGallery& SetSelectionMode(UiGallerySelectionMode mode);
    UiGallerySelectionMode GetSelectionMode() const { return selection_mode_; }
    UiGallery& ClearSelection();
    UiGallery& Select(int index, bool additive = false);
    UiGallery& SelectAll();
    bool IsSelected(int index) const;
    Vector<int> GetSelection() const;
    int GetSelectionCount() const { return selected_.GetCount(); }

    UiGallery& SetCursor(int index);
    int GetCursor() const { return cursor_; }
    int GetHotIndex() const { return hot_; }

    void ScrollTo(int index);
    void ScrollToSelection();
    UiGallery& SetScrollPos(int y);
    int GetScrollPos() const { return scroll_y_; }

    int GetColumnCount() const { return columns_; }
    int GetRowCount() const { return rows_; }
    Size GetContentSize() const { return content_size_; }
    Rect GetViewportRect() const { return viewport_; }
    Rect GetItemRect(int index) const;
    UiVisibleRange GetVisibleRange(bool include_overscan = false) const;
    int GetLastPaintItemCount() const { return last_paint_item_count_; }
    int GetGeometryBuildCount() const { return geometry_build_count_; }

    virtual void Paint(Draw& w) override;
    virtual void Layout() override;
    virtual Size GetMinSize() const override;
    virtual void LeftDown(Point p, dword flags) override;
    virtual void LeftUp(Point p, dword flags) override;
    virtual void LeftDouble(Point p, dword flags) override;
    virtual void MouseMove(Point p, dword flags) override;
    virtual void MouseLeave() override;
    virtual void MouseWheel(Point p, int zdelta, dword keyflags) override;
    virtual bool Key(dword key, int count) override;
    virtual void GotFocus() override;
    virtual void LostFocus() override;
    virtual void SetData(const Value& v) override;
    virtual Value GetData() const override;

    Event<> WhenSelection;
    Event<> WhenAction;
    Event<int, int> WhenVisibleRange;

    // Render-only hook. Set handled=true to replace the default tile painter.
    Event<Draw&, int, const UiModelItem&, const Rect&, UiGalleryItemVisualState, bool&> WhenPaintItem;

private:
    Style& StyleEdit();
    const Style& GetEffectiveStyle() const;
    void SyncThemeStyle();

    void BindModel(UiListModel& model);
    void HandleModelChange(const UiModelChange& change);
    void SyncModel();
    void InvalidateGeometry();
    void UpdateGeometry();
    void UpdateVisibleRangeNotification();
    void ClampScroll();
    int GetMaxScroll() const;

    Rect GetBaseViewportRect() const;
    int HitTestItem(Point p) const;
    void PaintDefaultItem(Draw& w, int index, const Rect& rect, UiGalleryItemVisualState state) const;
    UiGalleryItemVisualState GetItemVisualState(int index) const;

    bool IsSelectableIndex(int index) const;
    void SelectSingle(int index);
    void ToggleSelection(int index);
    void SelectRangeTo(int index, bool additive);
    void MoveCursor(int delta);
    void MoveCursorRows(int rows);
    void NotifySelectionChange();

    Value GetSelectionToken(int index) const;
    int ResolveSelectionIndex(const Value& token) const;

private:
    Style style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool has_custom_style_ = false;

    UiListModel internal_model_;
    UiListModel* model_ = nullptr;
    Vector<UiListModel*> bound_models_;
    mutable int model_revision_ = -1;

    Size item_size_ = Size(DPI(96), DPI(92));
    int gap_ = DPI(8);
    Rect inset_ = Rect(DPI(8), DPI(8), DPI(8), DPI(8));
    int overscan_rows_ = 2;

    Rect viewport_;
    Size content_size_ = Size(0, 0);
    int columns_ = 1;
    int rows_ = 0;
    bool geometry_valid_ = false;

    UiScrollBar vscroll_;
    int scroll_y_ = 0;
    bool updating_scrollbar_ = false;

    Index<int> selected_;
    UiGallerySelectionMode selection_mode_ = UIGALLERYSEL_SINGLE;
    int cursor_ = -1;
    int anchor_ = -1;
    int hot_ = -1;
    int pressed_ = -1;

    UiVisibleRange notified_range_;
    mutable int last_paint_item_count_ = 0;
    int geometry_build_count_ = 0;
};

} // namespace Upp

#endif
