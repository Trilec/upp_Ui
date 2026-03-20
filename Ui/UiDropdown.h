#ifndef _Ui_UiDropdown_h_
#define _Ui_UiDropdown_h_

/*
    UiDropdown
    ========== 

    Purpose
    - Styled dropdown/select control with optional rich popup rows and
      multi-selection support.

    Intent
    - Keep the popup lifecycle, item rendering, and model binding explicit
      while avoiding one-off API growth for each row decoration case.

    Thread context
    - GUI thread only.

    Usage
    - Use the internal model by default, or bind explicitly with
      SetModel(UiListModel&) and UseInternalModel().
    - Use SetData/GetData for generic selection binding.

    Changelog
    - 2026-03: normalized external model binding semantics and documented the
      release-standard control contract.
*/

#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>   // Access keys, DPI helpers, etc.
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>
#include <Ui/UiDataModels.h>
#include <Ui/UiLabel.h>
#include <Ui/UiIcons.h>
#include <Ui/UiScrollBar.h>

namespace Upp {

class UiDropdown : public Ctrl, public CtrlStyled<UiDropdown> {
public:
    typedef UiDropdown CLASSNAME;

    // ------------------------------------------------------------------------
    // Style
    // ------------------------------------------------------------------------
    struct Style : ChStyle<Style> {
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin    skin;

        // Main button/label style
        UiAlign align_h = UiAlign::LEFT;
        UiAlign align_v = UiAlign::CENTER;

        // Dropdown indicator (chevron) configuration
        bool show_indicator   = true;
        UiAlign indicator_side = UiAlign::RIGHT;  // LEFT or RIGHT
        Image glyph_closed;   // Default: arrow_drop_down
        Image glyph_opened;   // Default: arrow_drop_up
        bool indicator_scale  = true;
        int  indicator_size   = 0; // px, used when indicator_scale=true (0 -> default)

        // Per-block margins (thickness-rect semantics; negative expands)
        Rect indicator_margin = Rect(DPI(2), 0, DPI(4), 0);
        Rect label_margin     = Rect(0, 0, 0, 0);

        // Base font (used unless metrics.use_text_font == true)
        Font font = StdFont();

        // If true, dropdown does not paint its own background
        bool transparent = true;

        // Popup list style
        UiLabel::Style popup_item_style;
        int popup_max_height = DPI(300);
        int popup_item_height = DPI(32);
        bool popup_show_scrollbar = true;
        int popup_space = DPI(5);
        int popup_max_items = 10;

        // Multi-select summary + marker visuals
        bool show_selection_badge = true;
        int  selection_badge_radius = DPI(10);
        Color selection_badge_face = Color(65, 126, 232);
        Color selection_badge_ink = White();
        bool use_rounded_check_marker = true;
        int  check_marker_radius = DPI(5);
        UiAlign popup_check_side = UiAlign::RIGHT;

        // Popup frame styling
        int  popup_frame_width = DPI(1);
        int  popup_radius = DPI(4);
        Color popup_frame_color;
        Color popup_background_color = SColorPaper();
        bool popup_use_main_skin = false;

        void Serialize(Stream& s)
        {
            s % palette % metrics % skin
              % align_h % align_v
              % show_indicator % indicator_side
              % glyph_closed % glyph_opened
              % indicator_scale % indicator_size
              % indicator_margin % label_margin
              % font % transparent
              % popup_item_style
              % popup_max_height % popup_item_height % popup_show_scrollbar % popup_space % popup_max_items
              % show_selection_badge % selection_badge_radius
              % selection_badge_face % selection_badge_ink
              % use_rounded_check_marker % check_marker_radius % popup_check_side
              % popup_frame_width % popup_radius
              % popup_frame_color % popup_background_color
              % popup_use_main_skin;
        }
    };

    // Item data structure
    struct Item : Moveable<Item> {
        String text;
        Value  data;
        bool   enabled = true;           // Selectable row state.
        bool   group_header = false;     // Non-selectable visual section header.
        bool   separator_before = false; // Draw separator above this row.
        String description;              // Secondary text line.
        String right_text;               // Right-aligned meta text.
        Image  icon;                     // Optional left icon.
        bool   mono_icon = false;        // Tint icon using resolved ink/icon color.
        bool   checked = false;          // Used by multi-select and checkmark visualization.
        Color  custom_ink_color;

        Item() {}
        Item(const String& text, const Value& data = Value(), bool enabled = true)
            : text(text), data(data), enabled(enabled) {}
    };

private:
    // Style/theme state stays local; item content can come from an external model.
    Style  style_;
    mutable uint64 theme_revision_ = 0;
    bool has_style_override_ = false;
    String text_;                 // Current displayed text
    Image  indicator_;           // Current indicator image (open/closed state)
    
    Vector<Item> items_;
    int selected_index_ = -1;    // Current selection (-1 = none)
    int highlight_index_ = -1;   // Highlighted item in popup
    
    bool popup_open_ = false;
    bool popup_pinned_ = false; // Keep popup open after selection
    bool popup_auto_close_ = true;
    bool suppress_next_open_ = false;
    
    bool hot_ = false;
    bool pressed_ = false;
    bool multi_select_ = false;
    int  checked_count_cache_ = -1;
    String type_search_;
    TimeStop type_search_clock_;
    
    // Cached layout for the collapsed control face; popup rows are laid out separately.
    mutable bool           layout_dirty_ = true;
    mutable UiBlocksLayout layout_;
    mutable Rect           layout_content_ = Rect(0, 0, 0, 0);
    mutable Size           cached_minsize_;
    Size                   user_min_size_ = Size(0, 0);
    
    // Popup window owns transient list interaction and scrolling while the dropdown is open.
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
        int  HitTest(Point p) const;
        Rect GetItemRect(int index) const;
        void EnsureVisible(int index);
        void SyncWindowRegion();
        
    private:
        void SyncScrollBarState();

        int scroll_pos_ = 0;
        int total_height_ = 0;
        bool scrollbar_visible_ = false;
        int  scrollbar_width_ = DPI(12);
        UiScrollBar vscroll_ { UiDirection::V };
    };
    
    PopupWindow popup_;
    UiListModel internal_model_;
    UiListModel* model_ = nullptr;
    Vector<UiListModel*> bound_models_;
    
    // Internal helpers keep theme sync, popup lifetime, and model mirroring out of Paint().
    void InvalidateStyleCache();
    Style& StyleEdit();
    void SyncThemeStyle();
    const Style& GetEffectiveStyle() const;
    void RebuildIndicator();
    Size ComputeNaturalSize() const;
    void OpenPopupInternal();
    void ClosePopupInternal(bool apply_selection = true);
    void UpdatePopupPosition();
    void SyncPopupSelection();
    
    // Hit testing and display text helpers work against the collapsed face only.
    Rect GetIndicatorRect() const;
    Rect GetLabelRect() const;
    bool IsOverIndicator(Point p) const;
    void UpdateDisplayText();
    
    // Item/model helpers keep the public API explicit even when using the internal model.
    int  FindItem(const String& text, bool case_sensitive = false) const;
    int  FindItemByData(const Value& data) const;
    int  FindItemByPrefix(const String& prefix, int start_index = 0) const;
    bool HandleTypeAhead(int chr);
    bool IsSelectableItem(int index) const;
    String QueryItemSearchText(const Item& it, int index) const;
    void NotifyCheckedCountIfChanged(bool force = false);
    void SyncItemsFromModel();
    void BindModel(UiListModel& model);
    UiModelItem ToModelItem(const Item& it) const;
    Item FromModelItem(const UiModelItem& it) const;
    
public:
    UiDropdown();
    
    // ------------------------------------------------------------------------
    // Content - Items
    // ------------------------------------------------------------------------
    UiDropdown& Add(const String& text, const Value& data = Value(), bool enabled = true);
    UiDropdown& Add(const Item& item);
    UiDropdown& AddGroupHeader(const String& text);
    UiDropdown& Insert(int pos, const String& text, const Value& data = Value(), bool enabled = true);
    UiDropdown& Insert(int pos, const Item& item);
    
    UiDropdown& Remove(int index);
    UiDropdown& Remove(const String& text, bool case_sensitive = false);
    UiDropdown& Clear();
    
    int         GetCount() const { return items_.GetCount(); }
    const Item& GetItem(int index) const;
    Item&       GetItem(int index);
    
    UiDropdown& SetItem(int index, const String& text, const Value& data = Value(), bool enabled = true);
    UiDropdown& SetItemText(int index, const String& text);
    UiDropdown& SetItemData(int index, const Value& data);
    UiDropdown& SetItemEnabled(int index, bool enabled);
    UiDropdown& SetItemIcon(int index, const Image& icon, bool mono = false);
    UiDropdown& SetItemDescription(int index, const String& desc);
    UiDropdown& SetItemRightText(int index, const String& text);
    UiDropdown& SetItemChecked(int index, bool checked = true);
    UiDropdown& SetItemGroupHeader(int index, bool on = true);
    UiDropdown& SetItemSeparatorBefore(int index, bool on = true);
    UiDropdown& SetItemInkColor(int index, Color color);
    
    String      GetItemText(int index) const;
    Value       GetItemData(int index) const;
    bool        IsItemEnabled(int index) const;
    bool        IsItemChecked(int index) const;
    bool        IsItemGroupHeader(int index) const;
    bool        HasItemSeparatorBefore(int index) const;
    Image       GetItemIcon(int index) const;
    String      GetItemDescription(int index) const;
    String      GetItemRightText(int index) const;
    
    // ------------------------------------------------------------------------
    // Selection
    // ------------------------------------------------------------------------
    UiDropdown& Select(int index);
    UiDropdown& Select(const String& text, bool case_sensitive = false);
    UiDropdown& SelectByData(const Value& data);
    UiDropdown& ClearSelection();
    
    int         GetSelection() const { return selected_index_; }
    bool        HasSelection() const { return selected_index_ >= 0; }
    String      GetSelectedText() const;
    Value       GetSelectedData() const;
    const Item& GetSelectedItem() const;
    Item&       GetSelectedItem();

    UiDropdown& SetMultiSelect(bool on = true);
    bool        IsMultiSelect() const { return multi_select_; }
    UiDropdown& ToggleItemChecked(int index, bool fire_event = true);
    UiDropdown& SetCheckedByData(const Value& data, bool checked = true);
    UiDropdown& ClearChecked();
    int         GetCheckedCount() const;
    Vector<int> GetCheckedIndices() const;
    Vector<Value> GetCheckedData() const;
    
    // ------------------------------------------------------------------------
    // Configuration
    // ------------------------------------------------------------------------
    UiDropdown& SetIndicatorSide(UiAlign side);
    UiDropdown& ShowIndicator(bool on = true);
    UiDropdown& SetIndicatorGlyphs(const Image& closed, const Image& opened);
    UiDropdown& SetIndicatorScale(bool on = true, int size = 0);
    UiDropdown& SetIndicatorMargin(const Rect& margin);
    UiDropdown& SetLabelMargin(const Rect& margin);
    
    UiDropdown& SetPopupMaxHeight(int height);
    UiDropdown& SetPopupMaxItems(int count);
    UiDropdown& SetPopupItemHeight(int height);
    UiDropdown& SetPopupShowScrollbar(bool on = true);
    UiDropdown& SetPopupSpace(int space);
    UiDropdown& SetPopupFrame(int width, int radius = DPI(4), Color frame_color = Null);
    UiDropdown& SetPopupBackground(Color color);
    UiDropdown& SetPopupUseMainSkin(bool on = true);
    UiDropdown& SetPopupCheckSide(UiAlign side);
    
    UiDropdown& SetPopupAutoClose(bool on = true);
    UiDropdown& SetPopupPinned(bool on = false); // Keep open after selection

    // Explicit external model binding. Call UseInternalModel() to switch back.
    UiDropdown& SetModel(UiListModel& model);
    UiDropdown& UseInternalModel();
    UiListModel& GetInternalModel() { return internal_model_; }
    UiListModel& GetModel() { return *model_; }
    const UiListModel& GetModel() const { return *model_; }
    
    // ------------------------------------------------------------------------
    // Styling
    // ------------------------------------------------------------------------
    UiDropdown&       SetStyle(const Style& s);
    UiDropdown&       ClearStyleOverride();
    bool              HasStyleOverride() const { return has_style_override_; }
    const Style&      GetStyle() const { return GetEffectiveStyle(); }
    static const Style& StyleDefault();
    
    // CtrlStyled (CRTP) interface
    StyledPalette& StyledPaletteRef() { return StyleEdit().palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().metrics; }
    StyledSkin&    StyledSkinRef()    { return StyleEdit().skin; }
    
    void OnStyleChanged();
    
    // ------------------------------------------------------------------------
    // Layout / Sizing
    // ------------------------------------------------------------------------
    Size GetMinSize() const override;
    void Layout() override;
    
    UiDropdown& SetSizeMin(Size sz);
    UiDropdown& SetSizeMin(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }
    UiDropdown& SetSizeFixed(Size sz)        { return SetSizeMin(sz); }
    UiDropdown& SetSizeFixed(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }
    
    // ------------------------------------------------------------------------
    // Painting hooks
    // ------------------------------------------------------------------------
    Event<Draw&, const Rect&,
          const StyledPalette&, const StyledMetrics&, const StyledSkin&,
          StyledState, bool> WhenPaintBackground;
    
    Event<Draw&, const Rect&,
          const StyledPalette&, const StyledMetrics&, const StyledSkin&,
          StyledState, bool> WhenPaintForeground;

    Event<Draw&, const Rect&, const Item&, int, bool, bool, bool, const Style&> WhenPaintItem;
    Event<String&, const Item&, int> WhenQueryItemText;
    Event<Draw&, const Rect&, int, const Style&> WhenPaintSelectionBadge;
    
    // ------------------------------------------------------------------------
    // Ctrl overrides
    // ------------------------------------------------------------------------
    void Paint(Draw& w) override;
    void LeftDown(Point p, dword flags) override;
    void LeftUp(Point p, dword flags) override;
    void MouseMove(Point p, dword flags) override;
    void MouseLeave() override;
    bool Key(dword key, int count) override;
    void GotFocus() override;
    void LostFocus() override;
    
    void  SetData(const Value& v) override;
    Value GetData() const override;
    
    // ------------------------------------------------------------------------
    // Events
    // ------------------------------------------------------------------------
    Event<int>            WhenSelect;      // index
    Event<const String&>  WhenSelectText;  // text
    Event<const Value&>   WhenSelectData;  // data
    Event<>               WhenOpen;
    Event<>               WhenClose;
    Event<int, bool>      WhenItemState;   // index, enabled
    Event<int, bool>      WhenItemCheck;   // index, checked
    Event<int>            WhenCheckedCount;
    
    // ------------------------------------------------------------------------
    // Popup control
    // ------------------------------------------------------------------------
    bool IsPopupOpen() const { return popup_open_; }
    UiDropdown& OpenPopup();
    UiDropdown& ClosePopup();
    UiDropdown& TogglePopup();
    
private:
    // Prevent copying
    UiDropdown(const UiDropdown&);
    UiDropdown& operator=(const UiDropdown&);
};

} // namespace Upp

#endif



