#ifndef _Ui_UiGallery_h_
#define _Ui_UiGallery_h_

/*
    UiGallery
    =========

    Purpose
    - High-scale, model-backed gallery for uniformly sized visual items.

    Intent
    - Keep logical item count independent of Ctrl/renderer count.
    - Keep ordinary viewport geometry arithmetic and bounded by visible/overscan
      content even for hundred-thousand-item models.
    - Present useful tiles through recycled UiItemRender instances prepared
      outside Paint().
    - Own gallery interaction: selection, marquee, zoom, scrolling and focus;
      semantic item state stays in UiListModel.

    Thread context
    - GUI thread only while bound to a live control.

    Usage
    - Bind UiListModel with SetModel(...). The default vertical
      UiItemRenderImage works without configuration and falls back to item.icon
      when item.image is empty.
    - Replace presentation with SetItemRender(...).
*/

#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>
#include <Ui/UiDataModels.h>
#include <Ui/UiItemRender.h>
#include <Ui/UiModelView.h>
#include <Ui/UiScrollBar.h>

namespace Upp {

enum UiGallerySelectionMode : byte {
    UIGALLERYSEL_SINGLE = 0,
    UIGALLERYSEL_MULTI,
};

class UiGallery : public Ctrl, public CtrlStyled<UiGallery> {
public:
    typedef UiGallery CLASSNAME;

    struct Style : ChStyle<Style> {
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin skin;

        Color marquee_fill = Color(239, 246, 255);
        Color marquee_frame = Color(59, 130, 246);
        int marquee_frame_width = DPI(2);
        Color selection_frame = Color(59, 130, 246);
        int selection_frame_width = DPI(2);

        void Serialize(Stream& s)
        {
            s % palette % metrics % skin
              % marquee_fill % marquee_frame % marquee_frame_width
              % selection_frame % selection_frame_width;
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

    UiGallery& SetItemRender(const UiItemRender& render);
    const UiItemRender& GetItemRender() const;
    int GetLiveItemRenderCount() const { return item_render_pool_.GetCount(); }
    int GetLastRenderLayoutCount() const { return last_render_layout_count_; }

    // Uniform cell geometry is the high-scale default and scale contract.
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

    // Semantic tile zoom. SetItemSize establishes the 1.0 base size.
    UiGallery& SetZoom(double zoom, Point anchor = Point(-1, -1));
    UiGallery& ZoomBy(double factor, Point anchor = Point(-1, -1));
    UiGallery& SetZoomRange(double minimum, double maximum, double step = 1.12);
    double GetZoom() const { return zoom_; }
    double GetMinZoom() const { return min_zoom_; }
    double GetMaxZoom() const { return max_zoom_; }

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
    bool IsMarqueeSelecting() const { return marquee_active_; }
    Rect GetMarqueeRect() const;

    virtual void Paint(Draw& w) override;
    virtual void Layout() override;
    virtual Size GetMinSize() const override;
    virtual void LeftDown(Point p, dword flags) override;
    virtual void LeftDrag(Point p, dword flags) override;
    virtual void LeftUp(Point p, dword flags) override;
    virtual void LeftDouble(Point p, dword flags) override;
    virtual void MouseMove(Point p, dword flags) override;
    virtual void MouseLeave() override;
    virtual void MouseWheel(Point p, int zdelta, dword keyflags) override;
    virtual bool Key(dword key, int count) override;
    virtual void CancelMode() override;
    virtual void GotFocus() override;
    virtual void LostFocus() override;
    virtual void SetData(const Value& v) override;
    virtual Value GetData() const override;

    Event<> WhenSelection;
    Event<> WhenAction;
    Event<double> WhenZoom;
    Event<int, int> WhenVisibleRange;

private:
    struct ItemRenderSlot {
        One<UiItemRender> render;
        int index = -1;
    };

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
    UiItemRenderState GetItemRenderState(int index) const;

    void EnsureItemRender() const;
    void ResetItemRenderPool();
    void InvalidateItemRenderData(int first = -1, int last = -1);
    void PrepareItemRenders();
    UiItemRender* FindPreparedItemRender(int index);
    const UiItemRender* FindPreparedItemRender(int index) const;

    bool IsSelectableIndex(int index) const;
    void SelectSingle(int index);
    void ToggleSelection(int index);
    void SelectRangeTo(int index, bool additive);
    void MoveCursor(int delta);
    void MoveCursorRows(int rows);
    void NotifySelectionChange();

    void BeginMarquee(Point p, dword flags);
    void UpdateMarquee(Point p, dword flags);
    void UpdateMarqueeSelection();
    void AutoScrollMarquee(Point p);
    void EndMarquee(bool cancel, bool release_capture = true);
    Rect GetMarqueeContentRect() const;
    Point ToContentPoint(Point p) const;

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

    One<UiItemRender> item_render_;
    Array<ItemRenderSlot> item_render_pool_;
    UiListModel* prepared_render_model_ = nullptr;
    UiVisibleRange prepared_render_range_;
    int last_render_layout_count_ = 0;

    Size base_item_size_ = Size(DPI(96), DPI(92));
    Size item_size_ = Size(DPI(96), DPI(92));
    double zoom_ = 1.0;
    double min_zoom_ = 0.50;
    double max_zoom_ = 2.50;
    double zoom_step_ = 1.12;

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

    bool marquee_candidate_ = false;
    bool marquee_active_ = false;
    bool marquee_capture_owned_ = false;
    Point marquee_start_content_ = Point(0, 0);
    Point marquee_current_content_ = Point(0, 0);
    Vector<int> marquee_open_selection_;
    int marquee_open_anchor_ = -1;
    dword marquee_flags_ = 0;
    int marquee_threshold_ = DPI(4);

    UiVisibleRange notified_range_;
    mutable int last_paint_item_count_ = 0;
    int geometry_build_count_ = 0;
};

} // namespace Upp

#endif
