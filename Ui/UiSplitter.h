#ifndef _Ui_UiSplitter_h_
#define _Ui_UiSplitter_h_

/*
    UiSplitter
    ==========

    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    Purpose
    - Styled splitter container for resizable panes.

    Intent
    - Keep the proven U++ splitter behavior while replacing Chameleon handle
      drawing with the Ui shared style contract.
    - Provide one control for application layouts; frame-mounted splitter
      behavior can be added later if a concrete Ui use case needs it.

    Thread context
    - GUI thread only.

    Usage
    - Add panes with Add()/operator<<(), choose Horz() or Vert(), and style the
      handle through UiTheme or SetCustomStyle().

    Changelog
    - 2026-05: initial Ui style-system splitter.
    - 2026-05: public split position API uses percentages; raw internal split
      units stay private.
    - 2026-06: added explicit grip visuals for lines, dots, and icon modes.
    - 2026-06: added hover/pressed feedback-band thickness controls to make the
      interactive splitter area visually obvious.
*/

#include <CtrlCore/CtrlCore.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>

namespace Upp {

enum UiSplitterGripVisual {
    UISPLITTER_GRIP_NONE,
    UISPLITTER_GRIP_LINES,
    UISPLITTER_GRIP_DOTS,
    UISPLITTER_GRIP_ICON,
};

class UiSplitter : public Ctrl, public CtrlStyled<UiSplitter> {
public:
    typedef UiSplitter CLASSNAME;

    struct Style : ChStyle<Style> {
        StyledPalette track_palette;
        StyledMetrics track_metrics;
        StyledSkin    track_skin;

        StyledPalette thumb_palette;
        StyledMetrics thumb_metrics;
        StyledSkin    thumb_skin;

        StyledPalette background_palette;
        StyledMetrics background_metrics;
        StyledSkin    background_skin;

        int  hit_width = DPI(8);
        int  track_thickness = DPI(1);
        int  hot_track_thickness = 0;
        int  pressed_track_thickness = 0;
        bool expand_track_on_hot = true;
        bool expand_track_on_pressed = true;
        Rect track_inset = Rect(0, 0, 0, 0);

        int  thumb_main = DPI(42);
        int  thumb_cross = DPI(8);
        Rect thumb_inset = Rect(0, 0, 0, 0);
        bool paint_background = false;
        bool show_grip = true;
        UiSplitterGripVisual grip_visual = UISPLITTER_GRIP_LINES;
        int  grip_count = 2;
        int  grip_size = DPI(2);
        int  grip_gap = DPI(3);
        Color grip_color = Null;
        int  grip_dot_count = 6;
        int  grip_dot_gap = DPI(3);
        int  grip_dot_size = DPI(2);

        String label;
        Font   label_font;
        Color  label_color = Null;
        int    label_gap = DPI(6);
        Image  thumb_icon;
        int    thumb_icon_size = DPI(14);
        UiIconRenderMode thumb_icon_render_mode = UiIconRenderMode::MonoTint;

        void Serialize(Stream& s)
        {
            int version = 3;
            s / version;
            s % track_palette
              % track_metrics
              % track_skin
              % thumb_palette
              % thumb_metrics
              % thumb_skin
              % background_palette
              % background_metrics
              % background_skin
              % hit_width
              % track_thickness;
            if(version >= 3) {
                s % hot_track_thickness
                  % pressed_track_thickness
                  % expand_track_on_hot
                  % expand_track_on_pressed;
            }
            else if(s.IsLoading()) {
                hot_track_thickness = 0;
                pressed_track_thickness = 0;
                expand_track_on_hot = true;
                expand_track_on_pressed = true;
            }
            s % track_inset
              % thumb_main
              % thumb_cross
              % thumb_inset
              % paint_background
              % show_grip;
            if(version >= 2) {
                int gv = (int)grip_visual;
                s % gv
                  % grip_count
                  % grip_size
                  % grip_gap
                  % grip_color;
                if(s.IsLoading())
                    grip_visual = (UiSplitterGripVisual)minmax(gv, (int)UISPLITTER_GRIP_NONE, (int)UISPLITTER_GRIP_ICON);
            }
            else {
                s % grip_dot_count
                  % grip_dot_gap
                  % grip_dot_size
                  % grip_color;
                if(s.IsLoading())
                    grip_visual = show_grip ? UISPLITTER_GRIP_DOTS : UISPLITTER_GRIP_NONE;
                grip_count = grip_dot_count > 0 ? grip_dot_count : 2;
                grip_size = grip_dot_size > 0 ? grip_dot_size : DPI(2);
                grip_gap = grip_dot_gap > 0 ? grip_dot_gap : DPI(3);
            }
            s % label
              % label_font
              % label_color
              % label_gap
              % thumb_icon
              % thumb_icon_size
              % thumb_icon_render_mode;
            if(version < 2 && s.IsLoading() && !IsNull(thumb_icon))
                grip_visual = UISPLITTER_GRIP_ICON;
        }
    };

private:
    Style style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool has_custom_style_ = false;

    Vector<int> pos_;
    Vector<int> mins_;
    Vector<int> minpx_;
    int zoom_index_ = -1;
    bool vertical_ = false;
    int drag_index_ = -1;
    int hot_index_ = -1;

    void InvalidateStyleCache();
    Style& StyleEdit();
    void SyncThemeStyle();

    int FindIndex(Point p) const;
    int GetMins(int i) const;
    void SyncMin();
    Rect GetHitRect(int index) const;
    Rect GetTrackRect(int index) const;
    Rect GetThumbRect(int index) const;
    void PaintGrip(Draw& w, const Rect& r, StyledState st) const;
    UiSplitter& SetSplitUnits(int units, int index = 0);
    int GetSplitUnits(int index = 0) const { return index < pos_.GetCount() ? pos_[index] : 10000; }

public:
    UiSplitter();
    virtual ~UiSplitter() {}

    static const Style& StyleDefault();

    UiSplitter& SetCustomStyle(const Style& s);
    UiSplitter& ClearCustomStyle();
    bool HasCustomStyle() const { return has_custom_style_; }
    const Style& GetStyle() const { return GetEffectiveStyle(); }
    const Style& GetCustomStyle() const { return style_; }
    const Style& GetEffectiveStyle() const;

    StyledPalette& StyledPaletteRef() { return StyleEdit().thumb_palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().thumb_metrics; }
    StyledSkin& StyledSkinRef() { return StyleEdit().thumb_skin; }

    void OnStyleChanged();

    virtual void Layout() override;
    virtual void Paint(Draw& w) override;
    virtual void MouseMove(Point p, dword keyflags) override;
    virtual void MouseLeave() override;
    virtual void LeftDown(Point p, dword keyflags) override;
    virtual void LeftUp(Point p, dword keyflags) override;
    virtual Image CursorImage(Point p, dword keyflags) override;
    virtual Size GetMinSize() const override;
    Size GetContentSize() const;
    virtual void Serialize(Stream& s) override;

    int ClientToPos(Point p) const;
    int PosToClient(int pos) const;

    UiSplitter& SetSplitPercent(double percent, int index = 0);
    double GetSplitPercent(int index = 0) const;

    int GetCount() const { return GetChildCount(); }
    int GetSplitWidth() const { return max(1, GetEffectiveStyle().hit_width); }
    Rect GetFeedbackTrackRect(int index, StyledState st) const;

    void Add(Ctrl& pane);
    UiSplitter& operator<<(Ctrl& pane) { Add(pane); return *this; }
    void Insert(int pos, Ctrl& pane);
    void Remove(Ctrl& pane);
    void Swap(Ctrl& pane, Ctrl& newpane);
    void Clear();
    void Reset();

    UiSplitter& Set(Ctrl& a, Ctrl& b);
    UiSplitter& Horz(Ctrl& left, Ctrl& right);
    UiSplitter& Vert(Ctrl& top, Ctrl& bottom);
    UiSplitter& Horz() { vertical_ = false; Layout(); Refresh(); return *this; }
    UiSplitter& Vert() { vertical_ = true; Layout(); Refresh(); return *this; }
    bool IsHorz() const { return !vertical_; }
    bool IsVert() const { return vertical_; }

    void Zoom(int i);
    void NoZoom() { Zoom(-1); }
    int GetZoom() const { return zoom_index_; }

    UiSplitter& SetMin(int i, int w) { mins_.At(i, 0) = w; SyncMin(); return *this; }
    UiSplitter& SetMinPixels(int i, int w) { minpx_.At(i, 0) = w; SyncMin(); return *this; }
    UiSplitter& SetLabel(const String& s);
    UiSplitter& SetThumbIcon(const Image& img);
    UiSplitter& SetGripVisual(UiSplitterGripVisual visual);
    UiSplitterGripVisual GetGripVisual() const { return GetEffectiveStyle().grip_visual; }
    UiSplitter& SetGripCount(int count);
    UiSplitter& SetGripSize(int size);
    UiSplitter& SetGripGap(int gap);
    UiSplitter& SetGripColor(Color c);
    UiSplitter& SetThumbSize(int main, int cross);
    UiSplitter& SetThumbSizePixels(int width, int height);
    UiSplitter& SetTrackThickness(int px);

    Event<> WhenSplitFinish;
};

} // namespace Upp

#endif
