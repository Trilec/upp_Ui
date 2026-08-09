#ifndef _Ui_UiColorMatrix_h_
#define _Ui_UiColorMatrix_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    UiColorMatrix
    =============

    Purpose
    - Compact, styled multi-colour field for one to eight related colours.

    Intent
    - Provide a first-class multi-colour value control rather than a generic
      property-row/composite abstraction.
    - Treat the swatches as one contiguous value: activating any swatch opens
      one UiColorPicker containing the complete colour set.
    - Fit the swatches to the available rectangle and wrap automatically when
      a single row would make the cells too small.
    - Use the normal Ui theme/style primitives for the surrounding surface,
      frames, radius, and shadows. The swatch face itself is always its colour.
    - Keep the current public capacity aligned with UiColorPicker's eight-slot
      editing contract; the layout algorithm itself is not hard-coded to one row.

    Thread context
    - GUI thread only.
*/

#include <CtrlCore/CtrlCore.h>
#include <Ui/UiStyle.h>

namespace Upp {

enum class UiRole : byte;

class UiColorMatrix : public Ctrl {
public:
    typedef UiColorMatrix CLASSNAME;

    static constexpr int MAX_COLORS = 8;

    struct Style : ChStyle<Style> {
        StyledPalette surface_palette;
        StyledMetrics surface_metrics;
        StyledSkin    surface_skin;

        StyledPalette slot_palette;
        StyledMetrics slot_metrics;
        StyledSkin    slot_skin;

        StyledPalette active_palette;

        int slot_gap = DPI(4);
        int minimum_slot_size = DPI(18);
        int maximum_slot_size = DPI(42);

        void Serialize(Stream& s)
        {
            s % surface_palette % surface_metrics % surface_skin
              % slot_palette % slot_metrics % slot_skin
              % active_palette
              % slot_gap % minimum_slot_size % maximum_slot_size;
        }
    };

    UiColorMatrix();

    static const Style& StyleDefault();
    UiColorMatrix& SetCustomStyle(const Style& style);
    UiColorMatrix& ClearCustomStyle();
    bool HasCustomStyle() const { return has_custom_style_; }
    const Style& GetStyle() const;
    const Style& GetCustomStyle() const { return style_; }

    UiColorMatrix& SetRole(UiRole role);
    UiColorMatrix& SetActiveRole(UiRole role);

    UiColorMatrix& SetColorCount(int count);
    int GetColorCount() const { return colors_.GetCount(); }

    UiColorMatrix& SetColor(int index, Color color, bool fire = false);
    Color GetColor(int index) const;
    UiColorMatrix& SetColors(const Vector<Color>& colors, bool fire = false);
    Vector<Color> GetColors() const { return clone(colors_); }

    UiColorMatrix& SetColorLabel(int index, const String& label);
    String GetColorLabel(int index) const;

    UiColorMatrix& SetActiveIndex(int index, bool fire = false);
    int GetActiveIndex() const { return active_; }

    UiColorMatrix& SetSlotGap(int px);
    UiColorMatrix& SetSlotRadius(int px);
    UiColorMatrix& SetSlotFrameWidth(int px);
    UiColorMatrix& ShowSlotFrame(bool on = true);
    UiColorMatrix& SetSlotShadow(bool on = true);
    UiColorMatrix& SetSurfaceRadius(int px);
    UiColorMatrix& ShowSurface(bool on = true);
    UiColorMatrix& ShowSurfaceFrame(bool on = true);
    UiColorMatrix& SetSurfaceShadow(bool on = true);
    UiColorMatrix& SetMinimumSlotSize(int px);
    UiColorMatrix& SetMaximumSlotSize(int px);

    UiColorMatrix& EnablePicker(bool on = true);
    bool IsPickerEnabled() const { return picker_enabled_; }
    UiColorMatrix& SetPickerTitle(const String& title);
    bool EditColors();

    Rect GetSlotRect(int index) const;
    int HitTest(Point p) const;

    virtual void SetData(const Value& value) override;
    virtual Value GetData() const override;
    virtual Size GetMinSize() const override;
    virtual void Paint(Draw& draw) override;
    virtual void LeftDown(Point p, dword flags) override;
    virtual void LeftUp(Point p, dword flags) override;
    virtual void MouseMove(Point p, dword flags) override;
    virtual void MouseLeave() override;
    virtual bool Key(dword key, int count) override;
    virtual void GotFocus() override;
    virtual void LostFocus() override;

    Event<int> WhenSelect;
    Event<> WhenChanging;
    Event<> WhenAction;

private:
    void InvalidateThemeStyle();
    void SyncThemeStyle() const;
    Style ResolveThemeStyle() const;
    Style& StyleEdit();

    void EnsureStorage(int count);
    void SetHover(int index);
    void ActivateIndex(int index);
    void ResolveGrid(const Rect& inner, int& columns, int& rows, int& slot_size) const;

    mutable Style themed_style_;
    Style style_;
    mutable uint64 theme_revision_ = 0;
    bool has_custom_style_ = false;

    UiRole role_;
    UiRole active_role_;

    Vector<Color> colors_;
    Vector<String> labels_;
    int active_ = 0;
    int hover_ = -1;
    int pressed_ = -1;
    bool picker_enabled_ = true;
    String picker_title_ = "Colors";
};

} // namespace Upp

#endif
