#ifndef _Ui_UiColorPicker_h_
#define _Ui_UiColorPicker_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    UiColorPicker
    =============

    Purpose
    - Professional multi-slot colour picker with spectrum, palette, RGB curves,
      harmony generation, image extraction, drag-and-drop, and screen sampling.

    Architecture
    - Four permanent navigation buttons drive a headless UiStack.
    - Slot controls and the OK/Cancel footer live outside the stack and therefore
      never move or disappear when pages change.
    - All user colour changes converge through one committed model path.
    - GUI thread only.

    Changelog
    - 2026-07: four-mode stack refactor, persistent footer and slots, full
      colour-expression editing, native internal drag-and-drop, corrected
      palette registry, curves, harmony/image generation, and eyedropper.
*/

#include <Ui/Ui.h>

namespace Upp {

class UiColorPicker : public Ctrl, public CtrlStyled<UiColorPicker> {
public:
    typedef UiColorPicker CLASSNAME;

    struct SlotValue : Moveable<SlotValue> {
        Color  color = Black();
        int    alpha = 255;
        String label;

        bool operator==(const SlotValue& b) const
        {
            return color == b.color && alpha == b.alpha && label == b.label;
        }

        bool operator!=(const SlotValue& b) const { return !(*this == b); }
    };

    struct Style : ChStyle<Style> {
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin    skin;

        int navigation_height = DPI(58);
        int footer_height = DPI(48);
        int slot_size = DPI(26);
        int slot_gap = DPI(4);
        int page_gap = DPI(10);
        int right_panel_width = DPI(350);
        int section_gap = DPI(8);
        int readout_row_height = DPI(28);
        int channel_row_height = DPI(28);
        int button_height = DPI(30);

        void Serialize(Stream& s)
        {
            s % palette % metrics % skin
              % navigation_height % footer_height
              % slot_size % slot_gap
              % page_gap % right_panel_width % section_gap
              % readout_row_height % channel_row_height % button_height;
        }
    };

    enum PageMode : byte {
        PAGE_COLOR = 0,
        PAGE_PALETTES,
        PAGE_CURVES,
        PAGE_GENERATOR,
        PAGE_COUNT
    };

    // Kept numerically aligned with the previous public page values.
    enum TabPage : byte {
        PAGE_PICKER = PAGE_COLOR,
        PAGE_SWATCHES = PAGE_PALETTES,
        PAGE_MIXER = PAGE_CURVES
    };

    enum SpectrumMode : byte {
        SPECTRUM_HSV_RECT = 0,
        SPECTRUM_HUE_STRIP,
        SPECTRUM_RGB_SPECTRUM
    };

    enum ChannelMode : byte {
        CHANNEL_RGB_FLOAT = 0,
        CHANNEL_RGB_INT,
        CHANNEL_HSV,
        CHANNEL_HSL,
        CHANNEL_TMI,
        CHANNEL_CMYK,
        CHANNEL_LAB,
        CHANNEL_COUNT
    };

    enum HarmonyMode : byte {
        HARMONY_CUSTOM = 0,
        HARMONY_ANALOGOUS,
        HARMONY_COMPLEMENTARY,
        HARMONY_SPLIT_COMPLEMENTARY,
        HARMONY_TRIAD,
        HARMONY_SQUARE,
        HARMONY_COMPOUND,
        HARMONY_SHADES,
        HARMONY_MONOCHROMATIC,
        HARMONY_IMAGE_EXTRACT
    };

    static const Style& StyleDefault();

    UiColorPicker();
    virtual ~UiColorPicker();

    UiColorPicker& SetCustomStyle(const Style& s);
    UiColorPicker& ClearCustomStyle();
    bool           HasCustomStyle() const { return has_custom_style_; }
    const Style&   GetStyle() const { return GetEffectiveStyle(); }
    const Style&   GetCustomStyle() const { return style_; }

    StyledPalette& StyledPaletteRef() { return StyleEdit().palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().metrics; }
    StyledSkin&    StyledSkinRef()    { return StyleEdit().skin; }
    void           OnStyleChanged();

    UiColorPicker& SetPageMode(PageMode mode);
    PageMode       GetPageMode() const { return page_mode_; }

    UiColorPicker& SetChannelMode(ChannelMode mode);
    ChannelMode    GetChannelMode() const { return channel_mode_; }

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

    UiColorPicker& SetSpectrumMode(SpectrumMode mode);
    SpectrumMode   GetSpectrumMode() const { return spectrum_mode_; }

    UiColorPicker& SetHarmonyMode(HarmonyMode mode);
    HarmonyMode    GetHarmonyMode() const { return harmony_mode_; }

    UiColorPicker& SetGeneratorImage(const Image& image);
    const Image&   GetGeneratorImage() const { return generator_image_; }
    UiColorPicker& ExtractGeneratorPalette(int count = 12);

    UiColorPicker& AddUserSwatch(Color c);
    UiColorPicker& AddUserSwatch(Color c, int alpha);
    UiColorPicker& ClearUserSwatches();
    UiColorPicker& ClearRecentSwatches();

    int            GetUserSwatchCount() const;
    int            GetRecentSwatchCount() const;

    UiColorPicker& EnableSessionPersistence(bool on = true);
    bool           IsSessionPersistenceEnabled() const { return session_persistence_; }
    static void    ClearSharedSession();

    bool           IsScreenEyedropperAvailable() const;
    UiColorPicker& BeginScreenEyedropper();

    String         FormatActiveHex() const;
    String         FormatActiveHex8() const;
    String         FormatSlotHex8(int i) const;
    String         FormatActiveRgb8() const;
    String         FormatActiveRgbUnit() const;
    String         FormatActiveHsv() const;
    String         FormatActiveAlpha() const;

    static bool    ParseColorText(const String& text, Color& color, int& alpha);

    virtual void   Paint(Draw& w) override;
    virtual void   Layout() override;
    virtual Size   GetMinSize() const override;
    virtual void   SetData(const Value& v) override;
    virtual Value  GetData() const override;

    virtual void   LeftDown(Point p, dword flags) override;
    virtual void   LeftUp(Point p, dword flags) override;
    virtual void   MouseMove(Point p, dword flags) override;
    virtual bool   Key(dword key, int count) override;
    virtual Image  CursorImage(Point p, dword flags) override;
    virtual void   CancelMode() override;

    Event<>        WhenAction;
    Event<>        WhenChanging;
    Event<>        WhenAccept;
    Event<>        WhenCancel;
    Event<int>     WhenSlotChanged;
    Event<PageMode> WhenPageChanged;
    Event<ChannelMode> WhenChannelModeChanged;

private:
    struct SlotData : Moveable<SlotData> {
        Color  color = Black();
        int    alpha = 255;
        String label;

        SlotValue Export(bool alpha_enabled = true) const
        {
            SlotValue out;
            out.color = color;
            out.alpha = alpha_enabled ? alpha : 255;
            out.label = label;
            return out;
        }
    };

    class ColorField;
    class SwatchGrid;
    class ColorSlotButton;
    class ChannelGroup;
    class ReadoutRow;
    class ImagePreview;
    class HarmonyWheel;

    Style&         StyleEdit();
    void           InvalidateStyleCache();
    void           SyncThemeStyle();
    const Style&   GetEffectiveStyle() const;

    void           BuildChildTree();
    void           ConfigureControls();
    void           WireEvents();

    void           LoadSharedSession();
    void           SaveSharedSession(bool include_slots);
    void           HandleAccept();
    void           HandleCancel();

    void           SyncAllFromActiveSlot();
    void           SyncChannelGroups();
    void           SyncReadouts();
    void           SyncSlotButtons();
    void           SyncSpectrumMode();
    void           SyncPageButtons();
    void           SyncChannelButtons();
    void           SyncThemeToChildren();

    void           CommitColor(Color color, bool final_commit);
    void           CommitAlpha(int alpha, bool final_commit);
    void           CommitSlotValue(int slot, Color color, int alpha, bool final_commit);
    void           FinishLiveGesture();
    void           PushRecentColor(Color color, int alpha);

    void           HandlePrimarySlot(int index);
    void           HandlePreviousSlot(int index);
    void           HandleColorDrop(int slot, const SlotValue& value);
    bool           TryApplyColorText(const String& text, bool final_commit);
    bool           TryApplyReadoutText(int readout, const String& text, bool final_commit);

    void           HandleChannelValue(ChannelMode mode, int row, double value, bool final_commit);
    void           RefreshChannelModeValues(ChannelMode mode);
    void           UpdateAlphaAvailability();

    void           PopulatePaletteSelectors();
    void           SetPaletteIndex(int index);
    void           RefreshPaletteGrid();
    void           HandlePalettePick(int index, const SlotValue& value);
    void           HandleStashPick(int index, const SlotValue& value);
    void           HandlePaletteDropToStash(const SlotValue& value);
    void           SaveSelectedPaletteToStash();
    void           UseSelectedPaletteColor();
    void           UseSelectedStashColor();

    void           SelectCurveChannel(int index);
    void           CaptureCurveSource();
    void           ResetCurves();
    void           ApplyCurves(bool final_commit);

    void           RefreshGeneratorPalette();
    void           SetGeneratorMode(int mode);
    void           HandleGeneratorPick(int index, const SlotValue& value);
    void           LoadGeneratorImage();
    void           SaveGeneratedToStash();
    void           UseGeneratedColor();

    void           StartEyedropper();
    void           FinishEyedropperState(bool commit);
    void           StopEyedropper(bool commit);
    void           SampleEyedropper(bool final_commit);

private:
    Style          style_;
    mutable Style  themed_style_;
    mutable uint64 theme_revision_ = 0;
    uint64         children_theme_revision_ = 0;
    bool           children_style_dirty_ = true;
    bool           has_custom_style_ = false;

    bool           syncing_controls_ = false;
    bool           session_persistence_ = true;
    bool           accepted_ = false;
    bool           live_gesture_ = false;
    int            live_slot_ = -1;
    bool           eyedropper_active_ = false;
    bool           eyedropper_dragging_ = false;
    bool           stopping_eyedropper_ = false;
    bool           finishing_eyedropper_ = false;
    dword          live_callback_ms_ = 0;

    int            slot_count_ = 4;
    int            active_slot_ = 0;
    bool           alpha_enabled_ = true;
    PageMode       page_mode_ = PAGE_COLOR;
    SpectrumMode   spectrum_mode_ = SPECTRUM_HUE_STRIP;
    ChannelMode    channel_mode_ = CHANNEL_RGB_FLOAT;
    HarmonyMode    harmony_mode_ = HARMONY_TRIAD;
    int            remembered_hue_ = 0;

    int            palette_category_ = 0;
    int            palette_index_ = 0;
    int            selected_palette_index_ = -1;
    int            selected_stash_index_ = -1;
    int            selected_generated_index_ = -1;
    int            generated_base_count_ = 0;
    int            selected_curve_channel_ = 0;

    Vector<SlotData> slots_;
    Vector<SlotData> opening_slots_;
    Vector<SlotData> previous_slots_;
    Vector<SlotData> recent_swatches_;
    Vector<SlotData> user_swatches_;
    Vector<SlotData> generated_swatches_;
    SlotData         live_origin_;
    SlotValue        selected_palette_value_;
    SlotValue        selected_stash_value_;
    SlotValue        selected_generated_value_;

    Image            generator_image_;
    Color            generator_base_color_ = Color(0, 120, 212);
    int              generator_mode_ = 2;
    Color            curve_source_color_ = Black();

    UiBoxLayout      main_root_ { UiBoxLayout::Direction::V };
    UiBoxLayout      navigation_bar_ { UiBoxLayout::Direction::H };
    UiBoxLayout      footer_bar_ { UiBoxLayout::Direction::H };
    Ctrl             navigation_spacer_;
    Ctrl             footer_spacer_;

    UiButton         page_button_[PAGE_COUNT];
    UiLabel          footer_information_;
    UiButton         accept_button_;
    UiButton         cancel_button_;

    One<ColorSlotButton> current_slot_button_;
    ParentCtrl       slot_grid_host_;
    One<ColorSlotButton> primary_slot_button_[4];
    One<ColorSlotButton> previous_slot_button_[4];

    UiStack          page_stack_;
    ParentCtrl       color_page_;
    ParentCtrl       palette_page_;
    ParentCtrl       curves_page_;
    ParentCtrl       generator_page_;

    One<ColorField>  color_field_;
    UiDropdown       spectrum_mode_drop_;
    UiToolButton     eyedropper_button_;
    UiLabel          hue_axis_label_;
    UiLabel          gain_axis_label_;
    UiSlider         hue_axis_slider_ { UiDirection::H };
    UiSlider         gain_axis_slider_ { UiDirection::H };
    UiLineEdit       hue_axis_edit_;
    UiLineEdit       gain_axis_edit_;

    UiButton         channel_button_[CHANNEL_COUNT];
    UiDropdown       channel_mode_drop_;
    UiToggle         alpha_toggle_;
    UiLabel          alpha_toggle_label_;
    UiStack          channel_stack_;
    One<ChannelGroup> channel_group_[CHANNEL_COUNT];

    One<ReadoutRow>  readout_hsv_;
    One<ReadoutRow>  readout_hex_;
    One<ReadoutRow>  readout_hsl_;
    One<ReadoutRow>  readout_rgb_float_;
    One<ReadoutRow>  readout_cmyk_;
    One<ReadoutRow>  readout_rgb_int_;

    One<SwatchGrid>  recent_grid_;
    UiDropdown       palette_category_drop_;
    UiDropdown       palette_drop_;
    UiLabel          palette_badge_;
    One<SwatchGrid>  palette_grid_;
    UiLabel          stash_title_;
    UiLabel          palette_hint_;
    One<SwatchGrid>  stash_grid_;
    UiButton         palette_use_button_;
    UiButton         palette_save_button_;
    UiButton         stash_use_button_;
    UiButton         stash_save_active_button_;

    UiButton         curve_button_[4];
    UiButton         curve_capture_button_;
    UiButton         curve_reset_button_;
    UiStack          curve_stack_;
    UiBezierCurveEditor curve_editor_[4];
    UiLabel          curve_hint_;

    UiDropdown       harmony_drop_;
    UiButton         generator_mode_button_[3];
    UiButton         generator_refresh_button_;
    UiButton         generator_load_image_button_;
    UiButton         generator_clear_samples_button_;
    UiLabel          generator_gain_label_;
    UiSlider         generator_gain_slider_ { UiDirection::H };
    UiLineEdit       generator_gain_edit_;
    UiLabel          generator_count_label_;
    UiSlider         generator_count_slider_ { UiDirection::H };
    UiLineEdit       generator_count_edit_;
    UiButton         generator_use_button_;
    UiButton         generator_save_button_;
    One<ImagePreview> generator_image_preview_;
    One<HarmonyWheel> generator_wheel_;
    One<SwatchGrid>  generator_grid_;
    UiLabel          generator_hint_;
};

}

#endif
