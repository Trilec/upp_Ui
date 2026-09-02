#ifndef _Ui_UiRingChart_h_
#define _Ui_UiRingChart_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    UiRingChart
    ===========

    Purpose
    - Compact donut/ring chart for several proportional values.

    Intent
    - Keep composition data separate from UiProgressRing's one-value progress
      semantics while sharing the exact ring renderer.
    - Normalize against the segment sum by default; an explicit larger total
      leaves the remainder visible as themed track.
    - Keep V1 presentation-only: no selection, legends or per-segment child
      controls.
*/

#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>

namespace Upp {

enum class UiRole : byte;

struct UiRingSegment : Moveable<UiRingSegment> {
    double value = 0.0;
    String label;
    Color  color = Null;

    UiRingSegment() = default;
    UiRingSegment(double v, const String& l = String(), Color c = Null)
        : value(v), label(l), color(c) {}
};

class UiRingChart : public Ctrl {
public:
    typedef UiRingChart CLASSNAME;
    static constexpr int MAX_SERIES_COLORS = 8;

    struct Style : ChStyle<Style> {
        StyledPalette track_palette;
        StyledPalette text_palette;
        Color series[MAX_SERIES_COLORS];
        int series_count = 6;

        Font font;
        int thickness = DPI(12);
        int cap_roundness = 100;
        int ring_inset = DPI(2);
        int segment_gap = DPI(3);
        int min_text_height = DPI(7);

        void Serialize(Stream& s)
        {
            s % track_palette % text_palette;
            for(int i = 0; i < MAX_SERIES_COLORS; i++)
                s % series[i];
            s % series_count % font % thickness % cap_roundness
              % ring_inset % segment_gap % min_text_height;
        }
    };

    struct SegmentGeometry : Moveable<SegmentGeometry> {
        int index = -1;
        double value = 0.0;
        double start_angle = 0.0;
        double sweep_angle = 0.0;
        Color color = Null;
        bool visible = false;
    };

    struct Geometry : Moveable<Geometry> {
        Rect outer;
        Rect square;
        Rect text_rect;
        Pointf center;
        double radius = 0.0;
        double data_sum = 0.0;
        double total = 0.0;
        double remainder = 0.0;
        int thickness = 0;
        int cap_roundness = 100;
        int segment_gap = 0;
        int text_font_height = 0;
        bool text_visible = false;
        Vector<SegmentGeometry> segments;
    };

    static const Style& StyleDefault();

    UiRingChart();

    UiRingChart& SetCustomStyle(const Style& s);
    UiRingChart& ClearCustomStyle();
    bool         HasCustomStyle() const { return has_custom_style_; }
    const Style& GetStyle() const { return GetEffectiveStyle(); }
    const Style& GetCustomStyle() const { return style_; }

    UiRingChart& SetRole(UiRole role);
    UiRole       GetRole() const { return role_; }

    UiRingChart& AddSegment(double value, const String& label = String(), Color color = Null);
    UiRingChart& SetSegments(const Vector<UiRingSegment>& segments);
    UiRingChart& ClearSegments();
    int          GetSegmentCount() const { return segments_.GetCount(); }
    const UiRingSegment& GetSegment(int index) const;

    UiRingChart& SetTotal(double total);
    UiRingChart& ClearTotal() { return SetTotal(0.0); }
    bool         HasExplicitTotal() const { return explicit_total_ > 0.0; }
    double       GetExplicitTotal() const { return explicit_total_; }
    double       GetDataSum() const;
    double       GetTotal() const;

    UiRingChart& SetCenterText(const String& text);
    UiRingChart& ClearCenterText();
    String       GetCenterText() const { return center_text_; }

    UiRingChart& SetTrackColor(Color c);
    UiRingChart& SetTextColor(Color c);
    UiRingChart& SetSeriesColor(int index, Color c);
    Color        GetSeriesColor(int index) const;
    UiRingChart& SetThickness(int px);
    UiRingChart& SetCapRoundness(int percent);
    UiRingChart& SetRingInset(int px);
    UiRingChart& SetSegmentGap(int px);
    UiRingChart& SetFont(Font f);
    UiRingChart& SetFontSize(int height);

    int  GetThickness() const { return max(1, GetEffectiveStyle().thickness); }
    int  GetCapRoundness() const { return clamp(GetEffectiveStyle().cap_roundness, 0, 100); }
    int  GetRingInset() const { return max(0, GetEffectiveStyle().ring_inset); }
    int  GetSegmentGap() const { return max(0, GetEffectiveStyle().segment_gap); }
    Font GetFont() const { return GetEffectiveStyle().font; }

    virtual void Paint(Draw& w) override;
    virtual Size GetMinSize() const override;

    Geometry GetGeometry(Size size) const;

private:
    void         InvalidateStyleCache();
    Style&       StyleEdit();
    void         SyncThemeStyle();
    Style        ResolveThemeStyle() const;
    const Style& GetEffectiveStyle() const;
    void         OnStyleChanged();

    Geometry BuildGeometry(Size size) const;
    Font ResolveTextFont(const Rect& text_rect, bool& visible) const;
    Color ResolveSegmentColor(int index, const UiRingSegment& segment, bool enabled) const;
    Image RenderRaster(const Geometry& g, Color track) const;

    Style style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool has_custom_style_ = false;
    UiRole role_;

    Vector<UiRingSegment> segments_;
    double explicit_total_ = 0.0;
    String center_text_;
};

} // namespace Upp

#endif
