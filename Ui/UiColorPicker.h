#ifndef _Ui_UiColorPicker_h_ 
#define _Ui_UiColorPicker_h_ 

#include <Ui/Ui.h>

namespace Upp {

class UiColorPicker : public Ctrl, public CtrlStyled<UiColorPicker> {
public:
    typedef UiColorPicker CLASSNAME;

    struct Style : ChStyle<Style> {
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin    skin;

        int header_height = DPI(44);
        int slot_size = DPI(30);
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
              % header_height % slot_size % slot_gap
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

    UiColorPicker& SetStyle(const Style& s);
    UiColorPicker& ClearStyleOverride();
    bool           HasStyleOverride() const { return has_style_override_; }
    const Style&   GetStyle() const { return GetEffectiveStyle(); }

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
    Event<int>     WhenSlotChanged;

private:
    struct SlotData : Moveable<SlotData> {
        Color  color = Black();
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
    void           ApplySliderColor(bool final_commit);
    void           HandleSlotButton(int index);
    void           HandleRecentPick(Color c);
    void           HandleUserPick(Color c);
    void           HandleAddUserSwatch();
    void           HandleTransferRecentToActive();
    void           UpdateTabVisibility();
    void           SyncThemeToChildren();

private:
    Style          style_;
    mutable Style  themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool           has_style_override_ = false;

    int            slot_count_ = 2;
    int            active_slot_ = 0;
    bool           alpha_enabled_ = true;
    SpectrumMode   spectrum_mode_ = SPECTRUM_HSV_RECT;

    Vector<SlotData> slots_;
    Vector<Color>    recent_swatches_;
    Vector<Color>    user_swatches_;
    Color            pending_transfer_color_ = Null;

    ParentCtrl       header_bar_;
    Label            header_title_;
    UiButton         slot_button_[4];
    UiTab            tabs_;
    ParentCtrl       picker_page_;
    ParentCtrl       swatches_page_;
    ParentCtrl       mixer_page_;
    ParentCtrl       picker_left_;
    ParentCtrl       picker_right_;

    One<ColorField>  color_field_;
    UiDropdown       spectrum_mode_drop_;
    UiSlider         slider_hue_axis_;
    UiSlider         slider_value_axis_;
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

    One<SwatchGrid>  recent_grid_;
    One<SwatchGrid>  user_grid_;

    Label            picker_section_title_;
    Label            hue_axis_title_;
    Label            value_axis_title_;
    Label            hue_axis_value_;
    Label            value_axis_value_;
    Label            rgb_section_title_;
    Label            hsv_section_title_;
    Label            cmyk_section_title_;
    Label            live_section_title_;
    Label            readout_hex_;
    Label            readout_rgb8_;
    Label            readout_rgb_unit_;
    Label            readout_hsv_;
    Label            readout_alpha_;
    Label            channel_r_;
    Label            channel_g_;
    Label            channel_b_;
    Label            channel_a_;
    Label            channel_r_value_;
    Label            channel_g_value_;
    Label            channel_b_value_;
    Label            channel_a_value_;
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
    Label            channel_h_value_;
    Label            channel_s_value_;
    Label            channel_v_value_;
    Label            channel_ha_value_;
    Label            channel_sa_value_;
    Label            channel_va_value_;
    Label            channel_aa_value_;
    Label            channel_c_value_;
    Label            channel_m_value_;
    Label            channel_y_value_;
    Label            channel_k_value_;
    Label            swatches_palette_title_;
    Label            swatches_user_title_;
    Label            swatch_hint_;
    Label            mixer_placeholder_;
};

}
#endif	
