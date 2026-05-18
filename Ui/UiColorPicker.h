#ifndef _Ui_UiColorPicker_h_ 
#define _Ui_UiColorPicker_h_ 

#include <Ui/Ui.h>

namespace Upp {

class ReadoutRow;

class UiColorPicker : public Ctrl, public CtrlStyled<UiColorPicker> {
public:
    typedef UiColorPicker CLASSNAME;

    struct SlotValue : Moveable<SlotValue> {
        Color  color = Black();
        int    alpha = 255;
        String label;
    };

    struct Style : ChStyle<Style> {
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin    skin;

        int slot_size = DPI(42);
        int slot_gap = DPI(8);
        int page_gap = DPI(14);
        int right_panel_width = DPI(320);
        int section_gap = DPI(12);
        int readout_row_height = DPI(22);
        int tab_height = DPI(34);
        int section_title_height = DPI(18);

        void Serialize(Stream& s)
        {
            s % palette % metrics % skin
              % slot_size % slot_gap
              % page_gap % right_panel_width % section_gap % readout_row_height
              % tab_height % section_title_height;
        }
    };

    enum TabPage : byte {
        PAGE_PICKER = 0,
        PAGE_SWATCHES,
        PAGE_MIXER
    };

    enum SpectrumMode : byte {
        SPECTRUM_HSV_RECT = 0,
        SPECTRUM_HUE_STRIP,
        SPECTRUM_RGB_SPECTRUM
    };

    static const Style& StyleDefault();

    UiColorPicker();
    virtual ~UiColorPicker();

    UiColorPicker& SetCustomStyle(const Style& s);
    UiColorPicker& ClearCustomStyle();
    bool           HasCustomStyle() const { return has_custom_style_; }
    const Style&   GetStyle() const { return GetEffectiveStyle(); }
    const Style& GetCustomStyle() const { return style_; }

    StyledPalette& StyledPaletteRef() { return StyleEdit().palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().metrics; }
    StyledSkin&    StyledSkinRef()    { return StyleEdit().skin; }
    void           OnStyleChanged();

    UiColorPicker& SetSlotCount(int n);
    int            GetSlotCount() const { return slot_count_; }

    UiColorPicker& SetActiveSlot(int i);
    int            GetActiveSlot() const { return active_slot_; }

    UiColorPicker& SetSlotColor(int i, Color c, bool fire = true);
    Color          GetSlotColor(int i) const;

    UiColorPicker& SetSlotAlpha(int i, int alpha, bool fire = true);
    int            GetSlotAlpha(int i) const;

    UiColorPicker& SetSlot(int i, Color c, int alpha, bool fire = true);
    SlotValue      GetSlot(int i) const;
    Vector<SlotValue> GetSlots() const;

    UiColorPicker& SetColor(Color c, bool fire = true);
    Color          GetColor() const { return GetSlotColor(active_slot_); }
    UiColorPicker& SetAlpha(int alpha, bool fire = true);
    int            GetAlpha() const { return GetSlotAlpha(active_slot_); }

    UiColorPicker& SetSlotLabel(int i, const String& s);
    String         GetSlotLabel(int i) const;

    UiColorPicker& SetAlphaEnabled(bool on = true);
    bool           IsAlphaEnabled() const { return alpha_enabled_; }

    UiColorPicker& SetSpectrumMode(SpectrumMode m);
    SpectrumMode   GetSpectrumMode() const { return spectrum_mode_; }

    UiColorPicker& AddUserSwatch(Color c);
    UiColorPicker& ClearUserSwatches();
    UiColorPicker& ClearRecentSwatches();

    int            GetUserSwatchCount() const;
    int            GetRecentSwatchCount() const;

    String         FormatActiveHex() const;
    String         FormatActiveHex8() const;
    String         FormatSlotHex8(int i) const;
    String         FormatActiveRgb8() const;
    String         FormatActiveRgbUnit() const;
    String         FormatActiveHsv() const;
    String         FormatActiveAlpha() const;

    virtual void   Paint(Draw& w) override;
    virtual void   Layout() override;
    virtual Size   GetMinSize() const override;
    virtual void   SetData(const Value& v) override;
    virtual Value  GetData() const override;

    Event<>        WhenAction;
    Event<>        WhenChanging;
    Event<>        WhenAccept;
    Event<>        WhenCancel;
    Event<int>     WhenSlotChanged;

private:
    struct SlotData : Moveable<SlotData> {
        Color  color = Black();
        int    alpha = 255;
        String label;
    };

    class ColorField;
    class SwatchGrid;

    Style&         StyleEdit();
    void           InvalidateStyleCache();
    void           SyncThemeStyle();
    const Style&   GetEffectiveStyle() const;

    void           BuildChildTree();
    void           SyncFromActiveSlot(bool fire = false);
    void           SyncReadouts();
    void           SyncSlotButtons();
    void           SyncSpectrumMode();
    void           PushRecentColor(Color c);
    void           CommitColor(Color c, bool final_commit);
    void           CommitAlpha(bool final_commit);
    void           SyncControlsFromColor(Color c);
    void           ApplySliderColor(bool final_commit);
    void           HandleSlotButton(int index);
    void           HandleRecentPick(Color c);
    void           HandleUserPick(int i, Color c);
    void           HandleSaveActiveSwatch();
    void           HandleSavePaletteColor();
    void           HandleUsePaletteColor();
    void           HandleUseStashColor();
    void           UpdateTabVisibility();
    void           SyncThemeToChildren();

private:
    Style          style_;
    mutable Style  themed_style_;
    mutable uint64 theme_revision_ = 0;
    uint64         children_theme_revision_ = 0;
    bool           children_style_dirty_ = true;
    bool           has_custom_style_ = false;
    dword          live_update_ms_ = 0;
    int            action_separator_y_ = -1;

    int            slot_count_ = 2;
    int            active_slot_ = 0;
    bool           alpha_enabled_ = true;
    SpectrumMode   spectrum_mode_ = SPECTRUM_HSV_RECT;

    Vector<SlotData> slots_;
    Vector<SlotData> previous_slots_;
    Vector<Color>    recent_swatches_;
    Vector<Color>    user_swatches_;
    Color            pending_transfer_color_ = Null;
    Color            selected_palette_color_ = Null;
    Color            selected_stash_color_ = Null;
    int              selected_stash_index_ = -1;

    UiButton         slot_button_[4];
    UiTab            tabs_;
    ParentCtrl       picker_page_;
    ParentCtrl       swatches_page_;
    ParentCtrl       mixer_page_;
    UiBoxLayout      picker_root_ { UiBoxLayout::Direction::V };
    UiBoxLayout      picker_columns_ { UiBoxLayout::Direction::H };
    UiBoxLayout      picker_actions_ { UiBoxLayout::Direction::H };
    Ctrl             picker_action_spacer_;
    ParentCtrl       picker_left_;
    ParentCtrl       picker_right_;

    One<ColorField>  color_field_;
    UiDropdown       spectrum_mode_drop_;
    UiDropdown       library_palette_drop_;
    UiSlider         slider_hue_axis_;
    UiSlider         slider_value_axis_;
    UiSlider         slider_alpha_axis_;
    UiSlider         slider_r_;
    UiSlider         slider_g_;
    UiSlider         slider_b_;
    UiSlider         slider_a_;
    UiSlider         slider_h_;
    UiSlider         slider_s_;
    UiSlider         slider_v_;
    UiSlider         slider_c_;
    UiSlider         slider_m_;
    UiSlider         slider_y_;
    UiSlider         slider_k_;
    UiButton         add_user_swatch_button_;
    UiButton         transfer_to_active_button_;
    UiButton         push_user_swatch_button_;
    UiButton         use_stash_swatch_button_;
    UiButton         accept_button_;
    UiButton         cancel_button_;

    One<SwatchGrid>  recent_grid_;
    One<SwatchGrid>  user_grid_;

    Label            picker_section_title_;
    ParentCtrl       current_slot_card_;
    ParentCtrl       previous_slot_card_;
    Label            current_slot_title_;
    Label            previous_slot_title_;
    UiButton         current_slot_preview_;
    UiButton         previous_slot_preview_;
    Label            hue_axis_title_;
    Label            value_axis_title_;
    Label            alpha_axis_title_;
    UiFloatEdit      hue_axis_value_;
    UiFloatEdit      value_axis_value_;
    UiFloatEdit      alpha_axis_value_;
    Label            rgb_section_title_;
    Label            hsv_section_title_;
    Label            cmyk_section_title_;
    Label            live_section_title_;
    One<ReadoutRow>  readout_hex_;
    One<ReadoutRow>  readout_rgb_unit_;
    One<ReadoutRow>  readout_hsv_;
    One<ReadoutRow>  readout_alpha_;
    Label            channel_r_;
    Label            channel_g_;
    Label            channel_b_;
    Label            channel_a_;
    UiFloatEdit      channel_r_value_;
    UiFloatEdit      channel_g_value_;
    UiFloatEdit      channel_b_value_;
    UiFloatEdit      channel_a_value_;
    Label            channel_h_;
    Label            channel_s_;
    Label            channel_v_;
    Label            channel_ha_;
    Label            channel_sa_;
    Label            channel_va_;
    Label            channel_aa_;
    Label            channel_c_;
    Label            channel_m_;
    Label            channel_y_;
    Label            channel_k_;
    UiFloatEdit      channel_h_value_;
    UiFloatEdit      channel_s_value_;
    UiFloatEdit      channel_v_value_;
    UiFloatEdit      channel_ha_value_;
    UiFloatEdit      channel_sa_value_;
    UiFloatEdit      channel_va_value_;
    UiFloatEdit      channel_aa_value_;
    UiFloatEdit      channel_c_value_;
    UiFloatEdit      channel_m_value_;
    UiFloatEdit      channel_y_value_;
    UiFloatEdit      channel_k_value_;
    Label            swatches_palette_title_;
    Label            swatches_user_title_;
    Label            swatch_hint_;
    Label            mixer_placeholder_;
};

}
#endif	
