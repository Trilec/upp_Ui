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
    - Compact four-page colour workspace for precise colour editing, curated
      palette browsing, deterministic palette generation, and image extraction.

    Architecture
    - Color, Palettes, Generator, and Image are permanent UiStack pages.
    - Caller-facing slots, shared User Stash, readout, OK, and Cancel remain
      outside the page stack.
    - Page composition uses Ui controls and Ui layouts. Custom rendering is
      limited to the spectrum, colour wheels, swatch faces, and image canvas.
    - Deterministic palette/image algorithms are implemented outside Paint().
    - GUI thread only.

    Changelog
    - 2026-08: made caller-authored slot counts immediately authoritative and
      added grouped palette drag transfer between generated/static/image
      swatches, the User Stash, and primary slots.
    - 2026-08: Palette Lab migration; Image promoted to a page, shared 28-cell
      stash, multi-selection/group transfer, deterministic 2-12 swatch generator,
      bounded image proxy analysis, and compatibility-preserving public API.
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

        int navigation_height = DPI(34);
        int stash_height = DPI(92);
        int footer_height = DPI(42);
        int slot_size = DPI(26);
        int slot_gap = DPI(4);
        int page_gap = DPI(6);
        int right_panel_width = DPI(350); // compatibility metric
        int section_gap = DPI(6);
        int readout_row_height = DPI(28); // compatibility metric
        int channel_row_height = DPI(28); // compatibility metric
        int button_height = DPI(24);
        int wheel_minimum = DPI(190);
        int swatch_minimum = DPI(26);

        void Serialize(Stream& s)
        {
            s % palette % metrics % skin
              % navigation_height % footer_height
              % slot_size % slot_gap % page_gap % right_panel_width
              % section_gap % readout_row_height % channel_row_height
              % button_height
              % stash_height % wheel_minimum % swatch_minimum;
        }
    };

    enum PageMode : byte {
        PAGE_COLOR = 0,
        PAGE_PALETTES,
        PAGE_GENERATOR,
        PAGE_IMAGE,
        PAGE_COUNT
    };

    enum TabPage : byte {
        PAGE_PICKER = PAGE_COLOR,
        PAGE_SWATCHES = PAGE_PALETTES
    };

    enum SpectrumMode : byte {
        SPECTRUM_HSV_RECT = 0,
        SPECTRUM_HUE_STRIP,
        SPECTRUM_RGB_SPECTRUM,
        SPECTRUM_HSV_WHEEL
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
        HARMONY_IMAGE_EXTRACT // legacy persisted value; Image is now a page
    };

    enum DistributionMode : byte {
        DISTRIBUTION_BALANCED = 0,
        DISTRIBUTION_DOMINANT,
        DISTRIBUTION_ACCENT_POP,
        DISTRIBUTION_TONAL_RAMP,
        DISTRIBUTION_FREE_FORM,
        DISTRIBUTION_COUNT
    };

    enum MediumMode : byte {
        MEDIUM_UI = 0,
        MEDIUM_WEB,
        MEDIUM_PRINT,
        MEDIUM_PAINTING,
        MEDIUM_IMAGE_VFX,
        MEDIUM_COUNT
    };

    enum ImageAnalysisMode : byte {
        IMAGE_REPRESENTATIVE = 0,
        IMAGE_INTERFACE,
        IMAGE_ACCENT_FINDER,
        IMAGE_NATURE_SCENE,
        IMAGE_PAINT_MATERIAL,
        IMAGE_MANUAL_POINTS,
        IMAGE_ANALYSIS_COUNT
    };

    enum ImageCoverageMode : byte {
        COVERAGE_AREA_WEIGHTED = 0,
        COVERAGE_BALANCED,
        COVERAGE_BORDER_AWARE,
        COVERAGE_DISTINCTIVE,
        COVERAGE_COUNT
    };

    enum StashDropMode : byte {
        STASH_REPLACE = 0,
        STASH_MIX,
        STASH_ADD,
        STASH_SUBTRACT,
        STASH_MULTIPLY
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
    PageMode       GetPageMode() const;

    UiColorPicker& SetChannelMode(ChannelMode mode);
    ChannelMode    GetChannelMode() const;

    UiColorPicker& SetSlotCount(int n);
    int            GetSlotCount() const;

    UiColorPicker& SetActiveSlot(int i);
    int            GetActiveSlot() const;

    UiColorPicker& SetSlotColor(int i, Color c, bool fire = true);
    Color          GetSlotColor(int i) const;

    UiColorPicker& SetSlotAlpha(int i, int alpha, bool fire = true);
    int            GetSlotAlpha(int i) const;

    UiColorPicker& SetSlot(int i, Color c, int alpha, bool fire = true);
    SlotValue      GetSlot(int i) const;
    Vector<SlotValue> GetSlots() const;

    UiColorPicker& SetColor(Color c, bool fire = true);
    Color          GetColor() const;
    UiColorPicker& SetAlpha(int alpha, bool fire = true);
    int            GetAlpha() const;

    UiColorPicker& SetSlotLabel(int i, const String& s);
    String         GetSlotLabel(int i) const;

    UiColorPicker& SetAlphaEnabled(bool on = true);
    bool           IsAlphaEnabled() const;

    UiColorPicker& SetSpectrumMode(SpectrumMode mode);
    SpectrumMode   GetSpectrumMode() const;

    UiColorPicker& SetHarmonyMode(HarmonyMode mode);
    HarmonyMode    GetHarmonyMode() const;

    UiColorPicker& SetDistributionMode(DistributionMode mode);
    DistributionMode GetDistributionMode() const;

    UiColorPicker& SetMediumMode(MediumMode mode);
    MediumMode     GetMediumMode() const;

    UiColorPicker& SetGeneratorCount(int count);
    int            GetGeneratorCount() const;
    Vector<SlotValue> GetGeneratedPalette() const;

    UiColorPicker& SetGeneratorImage(const Image& image);
    const Image&   GetGeneratorImage() const;
    UiColorPicker& ExtractGeneratorPalette(int count = 12);
    Vector<SlotValue> GetImagePalette() const;

    UiColorPicker& AddUserSwatch(Color c);
    UiColorPicker& AddUserSwatch(Color c, int alpha);
    UiColorPicker& AddUserSwatches(const Vector<SlotValue>& values, bool transactional = true);
    UiColorPicker& ClearUserSwatches();
    UiColorPicker& ClearRecentSwatches();

    int            GetUserSwatchCount() const;
    int            GetRecentSwatchCount() const;
    Vector<SlotValue> GetUserSwatches() const;

    UiColorPicker& EnableSessionPersistence(bool on = true);
    bool           IsSessionPersistenceEnabled() const;
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

    Event<>         WhenAction;
    Event<>         WhenChanging;
    Event<>         WhenAccept;
    Event<>         WhenCancel;
    Event<int>      WhenSlotChanged;
    Event<PageMode> WhenPageChanged;
    Event<ChannelMode> WhenChannelModeChanged;

private:
    class Impl;

    Style&         StyleEdit();
    void           InvalidateStyleCache();
    void           SyncThemeStyle();
    const Style&   GetEffectiveStyle() const;

    Style          style_;
    mutable Style  themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool           has_custom_style_ = false;
    One<Impl>      impl_;
};

}

#endif
