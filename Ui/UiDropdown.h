#ifndef _Ui_UiDropdown_h_
#define _Ui_UiDropdown_h_

/*
    UiDropdown
    ==========

    Purpose
    - Styled dropdown/select control backed directly by UiListModel.

    Intent
    - Keep one authoritative item state: UiListModel.
    - Present collapsed content and popup rows through UiItemRender.
    - Keep popup lifetime, selection/check semantics, drag reorder, indicator,
      badge and popup chrome in UiDropdown.
    - Prepare popup renderer geometry outside Paint().

    Thread context
    - GUI thread only.
*/

#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>
#include <Ui/UiDataModels.h>
#include <Ui/UiItemRender.h>
#include <Ui/UiLabel.h>
#include <Ui/UiIcons.h>
#include <Ui/UiScrollBar.h>

namespace Upp {

enum class UiRole : byte;

class UiDropdown : public Ctrl, public CtrlStyled<UiDropdown> {
public:
    typedef UiDropdown CLASSNAME;
    // Convenience spelling only: there is no Dropdown-specific item object or
    // mirror. UiModelItem remains the one data type stored by UiListModel.
    using Item = UiModelItem;

    struct Style : ChStyle<Style> {
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin skin;

        UiAlign align_h = UiAlign::LEFT;
        UiAlign align_v = UiAlign::CENTER;

        bool show_indicator = true;
        UiAlign indicator_side = UiAlign::RIGHT;
        Image glyph_closed;
        Image glyph_opened;
        int indicator_size = 0;
        int content_gap = DPI(6);
        Font font = StdFont();
        bool transparent = true;

        UiLabel::Style popup_item_style;
        int popup_max_height = DPI(300);
        int popup_min_width = DPI(120);
        int popup_item_height = DPI(32);
        int item_spacing = 0;
        bool popup_show_scrollbar = true;
        int popup_space = DPI(5);
        int popup_max_items = 10;
        int drag_size = DPI(14);
        int drag_gap = DPI(6);
        bool show_drag_handle = true;
        UiAlign drag_side = UiAlign::RIGHT;
        Image drag_glyph;
        Color drag_marker = Color(56, 146, 255);

        bool show_popup_selection_marker = false;
        Image popup_selection_icon;
        Image popup_check_checked_icon;
        Image popup_check_unchecked_icon;
        UiIconRenderMode popup_marker_render_mode = UiIconRenderMode::MonoTint;
        UiAlign popup_marker_side = UiAlign::RIGHT;
        bool show_selection_badge = true;
        int selection_badge_radius = DPI(10);
        Color selection_badge_face = Color(65, 126, 232);
        Color selection_badge_ink = White();

        int popup_frame_width = DPI(1);
        int popup_radius = DPI(4);
        Color popup_frame_color;
        Color popup_background_color = SColorPaper();
        bool popup_use_main_skin = false;

        void Serialize(Stream& s)
        {
            s % palette % metrics % skin
              % align_h % align_v
              % show_indicator % indicator_side
              % glyph_closed % glyph_opened % indicator_size
              % content_gap % font % transparent
              % popup_item_style
              % popup_max_height % popup_min_width % popup_item_height
              % item_spacing % popup_show_scrollbar % popup_space % popup_max_items
              % drag_size % drag_gap % show_drag_handle % drag_side % drag_glyph % drag_marker
              % show_popup_selection_marker % popup_selection_icon
              % popup_check_checked_icon % popup_check_unchecked_icon
              % popup_marker_render_mode % popup_marker_side
              % show_selection_badge % selection_badge_radius
              % selection_badge_face % selection_badge_ink
              % popup_frame_width % popup_radius % popup_frame_color
              % popup_background_color % popup_use_main_skin;
        }
    };

    static const Style& StyleDefault();

    UiDropdown();

    UiDropdown& SetModel(UiListModel& model);
    UiDropdown& UseInternalModel();
    UiListModel& GetInternalModel() { return internal_model_; }
    UiListModel& GetModel() { return *model_; }
    const UiListModel& GetModel() const { return *model_; }

    UiDropdown& Add(const String& text, const Value& data = Value(), bool enabled = true);
    UiDropdown& Add(const UiModelItem& item);
    UiDropdown& AddGroupHeader(const String& text);
    UiDropdown& Insert(int pos, const String& text, const Value& data = Value(), bool enabled = true);
    UiDropdown& Insert(int pos, const UiModelItem& item);
    UiDropdown& Remove(int index);
    UiDropdown& Remove(const String& text, bool case_sensitive = false);
    UiDropdown& Clear();

    int GetCount() const { return model_ ? model_->GetCount() : 0; }
    const UiModelItem& GetItem(int index) const;

    UiDropdown& SetItem(int index, const String& text, const Value& data = Value(), bool enabled = true);
    UiDropdown& SetItemText(int index, const String& text);
    UiDropdown& SetItemData(int index, const Value& data);
    UiDropdown& SetItemEnabled(int index, bool enabled);
    UiDropdown& SetItemIcon(int index, const Image& icon,
                            UiIconRenderMode render_mode = UiIconRenderMode::PreserveColor);
    UiDropdown& SetItemDescription(int index, const String& desc);
    UiDropdown& SetItemRightText(int index, const String& text);
    UiDropdown& SetItemChecked(int index, bool checked = true);
    UiDropdown& SetItemGroupHeader(int index, bool on = true);
    UiDropdown& SetItemSeparatorBefore(int index, bool on = true);
    UiDropdown& SetItemInkColor(int index, Color color);

    String GetItemText(int index) const;
    Value GetItemData(int index) const;
    bool IsItemEnabled(int index) const;
    bool IsItemChecked(int index) const;
    bool IsItemGroupHeader(int index) const;
    bool HasItemSeparatorBefore(int index) const;
    Image GetItemIcon(int index) const;
    String GetItemDescription(int index) const;
    String GetItemRightText(int index) const;

    UiDropdown& SetItemRender(const UiItemRender& render);
    const UiItemRender& GetItemRender() const;
    int GetLiveItemRenderCount() const;
    int GetLastRenderLayoutCount() const;
    int GetLastPaintItemCount() const;

    UiDropdown& Select(int index);
    UiDropdown& Select(const String& text, bool case_sensitive = false);
    UiDropdown& SelectByData(const Value& data);
    UiDropdown& SetDataSilently(const Value& data);
    UiDropdown& ClearSelection();

    int GetSelection() const { return selected_index_; }
    bool HasSelection() const { return selected_index_ >= 0; }
    String GetSelectedText() const;
    Value GetSelectedData() const;
    const UiModelItem& GetSelectedItem() const;

    UiDropdown& SetMultiSelect(bool on = true);
    bool IsMultiSelect() const { return multi_select_; }
    UiDropdown& ToggleItemChecked(int index, bool fire_event = true);
    UiDropdown& SetCheckedByData(const Value& data, bool checked = true);
    UiDropdown& ClearChecked();
    int GetCheckedCount() const;
    Vector<int> GetCheckedIndices() const;
    Vector<Value> GetCheckedData() const;

    UiDropdown& SetIndicatorSide(UiAlign side);
    UiDropdown& ShowIndicator(bool on = true);
    UiDropdown& SetIndicatorGlyphs(const Image& closed, const Image& opened);
    UiDropdown& SetIndicatorSize(int size);
    UiDropdown& SetContentGap(int gap);

    UiDropdown& SetPopupMaxHeight(int height);
    UiDropdown& SetPopupMinWidth(int width);
    UiDropdown& SetPopupMaxItems(int count);
    UiDropdown& SetPopupItemHeight(int height);
    UiDropdown& SetPopupShowScrollbar(bool on = true);
    UiDropdown& SetPopupSpace(int space);
    UiDropdown& SetPopupFrame(int width, int radius = DPI(4), Color frame_color = Null);
    UiDropdown& SetPopupBackground(Color color);
    UiDropdown& SetPopupUseMainSkin(bool on = true);
    UiDropdown& SetPopupMarkerSide(UiAlign side);
    UiDropdown& SetPopupSelectionMarker(bool on = true);
    UiDropdown& SetPopupSelectionIcon(const Image& icon);
    UiDropdown& SetPopupCheckIcons(const Image& checked, const Image& unchecked = Image());
    UiDropdown& SetPopupMarkerRenderMode(UiIconRenderMode mode);
    UiDropdown& ShowSelectionBadge(bool on = true);
    UiDropdown& SetPopupAutoClose(bool on = true);
    UiDropdown& SetPopupPinned(bool on = false);

    UiDropdown& EnableDragReorder(bool on = true);
    bool IsDragReorderEnabled() const { return drag_reorder_enabled_; }
    UiDropdown& EnableInternalMutation(bool on = true);
    bool IsInternalMutationEnabled() const { return internal_mutation_enabled_; }
    UiDropdown& ShowDragHandle(bool on = true);
    UiDropdown& SetDragSide(UiAlign side);
    UiDropdown& SetDragGlyph(const Image& glyph);

    UiDropdown& SetRole(UiRole role);
    UiRole GetRole() const { return (UiRole)role_; }
    UiDropdown& SetCustomStyle(const Style& s);
    UiDropdown& ClearCustomStyle();
    bool HasCustomStyle() const { return has_custom_style_; }
    const Style& GetStyle() const { return GetEffectiveStyle(); }
    const Style& GetCustomStyle() const { return style_; }

    StyledPalette& StyledPaletteRef() { return StyleEdit().palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().metrics; }
    StyledSkin& StyledSkinRef() { return StyleEdit().skin; }
    void OnStyleChanged();

    Size GetMinSize() const override;
    void Layout() override;
    UiDropdown& SetSizeMin(Size sz);
    UiDropdown& SetSizeMin(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }
    UiDropdown& SetSizeFixed(Size sz) { return SetSizeMin(sz); }
    UiDropdown& SetSizeFixed(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }
    UiDropdown& SetPlaceholderText(const String& text);
    UiDropdown& SetEmptyText(const String& text);

    Event<Draw&, const Rect&, const StyledPalette&, const StyledMetrics&,
          const StyledSkin&, StyledState, bool> WhenPaintBackground;
    Event<Draw&, const Rect&, const StyledPalette&, const StyledMetrics&,
          const StyledSkin&, StyledState, bool> WhenPaintForeground;
    Event<String&, const UiModelItem&, int> WhenQueryItemText;
    Event<Draw&, const Rect&, int, const Style&> WhenPaintSelectionBadge;

    void Paint(Draw& w) override;
    void LeftDown(Point p, dword flags) override;
    void LeftUp(Point p, dword flags) override;
    void MouseMove(Point p, dword flags) override;
    void MouseLeave() override;
    bool Key(dword key, int count) override;
    void GotFocus() override;
    void LostFocus() override;
    void SetData(const Value& v) override;
    Value GetData() const override;

    Event<int> WhenSelect;
    Event<const String&> WhenSelectText;
    Event<const Value&> WhenSelectData;
    Event<> WhenOpen;
    Event<> WhenClose;
    Event<int, bool> WhenItemState;
    Event<int, bool> WhenItemCheck;
    Event<int> WhenCheckedCount;
    Event<UiReorderRequest&> WhenReorderRequest;
    Event<int, int> WhenReordered;

    bool IsPopupOpen() const { return popup_open_; }
    UiDropdown& OpenPopup();
    UiDropdown& ClosePopup();
    UiDropdown& TogglePopup();

private:
    struct PopupRenderSlot {
        One<UiItemRender> render;
        int index = -1;
    };

    class PopupWindow : public TopWindow {
    public:
        UiDropdown* owner = nullptr;

        virtual bool Key(dword key, int count) override;
        virtual void Deactivate() override;
        virtual void Paint(Draw& w) override;
        virtual void Layout() override;
        virtual void LeftDown(Point p, dword flags) override;
        virtual void LeftUp(Point p, dword flags) override;
        virtual void MouseMove(Point p, dword flags) override;
        virtual void MouseWheel(Point p, int zdelta, dword keyflags) override;

        void Init(UiDropdown* dropdown_owner);
        void SetHighlight(int index);
        int HitTest(Point p) const;
        int HitTestDrag(Point p) const;
        Rect GetItemRect(int index) const;
        Rect GetItemContentRect(int index) const;
        void EnsureVisible(int index);
        void SyncWindowRegion();
        void ResetRenderPool();
        void PrepareItemRenders();
        const UiItemRender* FindPreparedRender(int index) const;
        int GetLiveItemRenderCount() const { return render_pool_.GetCount(); }
        int GetLastRenderLayoutCount() const { return last_render_layout_count_; }
        int GetLastPaintItemCount() const { return last_paint_item_count_; }

    private:
        void SyncScrollBarState();
        int GetItemExtent() const;
        int GetVisibleStart() const;
        int GetVisibleEnd() const;

        int scroll_pos_ = 0;
        int total_height_ = 0;
        bool scrollbar_visible_ = false;
        int scrollbar_width_ = DPI(12);
        UiScrollBar vscroll_ { UiDirection::V };
        Array<PopupRenderSlot> render_pool_;
        int prepared_first_ = -1;
        int prepared_last_ = -1;
        int last_render_layout_count_ = 0;
        mutable int last_paint_item_count_ = 0;
    };

    Style& StyleEdit();
    const Style& GetEffectiveStyle() const;
    void InvalidateStyleCache();
    void SyncThemeStyle();
    void ConfigureDefaultItemRender();
    void EnsureItemRender();
    void ResetItemRenders();
    UiItemRenderData MakePopupRenderData(int index) const;
    UiItemRenderData MakeCollapsedRenderData() const;
    void PrepareCollapsedRender();

    void RebuildIndicator();
    Size ComputeNaturalSize() const;
    void OpenPopupInternal();
    void ClosePopupInternal(bool apply_selection = true);
    bool ApplySelectionInternal(int index, bool fire_events);
    void UpdatePopupPosition();
    void SyncPopupSelection();

    Rect GetIndicatorRect() const;
    Rect GetLabelRect() const;
    bool IsOverIndicator(Point p) const;
    void UpdateDisplayText();
    Rect ComputeBadgeRect(Rect& content) const;

    int FindItem(const String& text, bool case_sensitive = false) const;
    int FindItemByData(const Value& data) const;
    int FindItemByPrefix(const String& prefix, int start_index = 0) const;
    bool HandleTypeAhead(int chr);
    bool IsSelectableItem(int index) const;
    String QueryItemSearchText(const UiModelItem& item, int index) const;
    void NotifyCheckedCountIfChanged(bool force = false);
    void BindModel(UiListModel& model);
    void HandleModelChange(const UiModelChange& change);
    void NormalizeIndexesAfterChange(const UiModelChange& change);

    void BeginPopupDrag(int row, Point start_screen);
    void ContinuePopupDrag(Point screen);
    void EndPopupDrag(bool cancel);
    void MoveItemTo(int from, int before);
    int RemapIndexAfterMove(int index, int from, int before) const;

private:
    Style style_;
    byte role_ = 0;
    mutable uint64 theme_revision_ = 0;
    bool has_custom_style_ = false;

    String text_;
    String placeholder_text_ = "Select...";
    String empty_text_ = "Empty";
    Image indicator_;

    UiListModel internal_model_;
    UiListModel* model_ = nullptr;
    Vector<UiListModel*> bound_models_;

    One<UiItemRender> item_render_;
    bool custom_item_render_ = false;
    One<UiItemRender> collapsed_render_;
    int collapsed_data_revision_ = -1;
    int collapsed_selection_key_ = INT_MIN;
    int last_collapsed_layout_count_ = 0;

    int selected_index_ = -1;
    int highlight_index_ = -1;
    bool popup_open_ = false;
    bool popup_pinned_ = false;
    bool popup_auto_close_ = true;
    bool suppress_next_open_ = false;
    bool hot_ = false;
    bool pressed_ = false;
    bool multi_select_ = false;
    int checked_count_cache_ = -1;
    String type_search_;
    TimeStop type_search_clock_;

    mutable bool layout_dirty_ = true;
    mutable UiBlocksLayout layout_;
    mutable Rect layout_content_ = Rect(0, 0, 0, 0);
    mutable Size cached_minsize_;
    Rect collapsed_content_rect_;
    Rect badge_rect_;
    Size user_min_size_ = Size(0, 0);

    PopupWindow popup_;

    int hot_drag_ = -1;
    int pressed_drag_ = -1;
    bool drag_reorder_enabled_ = false;
    bool internal_mutation_enabled_ = true;
    int drag_threshold_px_ = DPI(10);
    bool drag_candidate_ = false;
    bool dragging_ = false;
    bool drag_moved_ = false;
    int drag_from_ = -1;
    int drag_insert_before_ = -1;
    Point drag_start_screen_ = Point(0, 0);

    UiDropdown(const UiDropdown&);
    UiDropdown& operator=(const UiDropdown&);
};

} // namespace Upp

#endif
