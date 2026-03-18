#ifndef _Ui_UiAccordion_h_
#define _Ui_UiAccordion_h_

#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>
#include <Ui/UiTitleCard.h>
#include <Ui/UiPanel.h>

namespace Upp {

class UiAccordion : public Ctrl, public CtrlStyled<UiAccordion> {
public:
    typedef UiAccordion CLASSNAME;

    enum class Lock : byte {
        None = 0,
        Open,
        Closed,
    };

    struct Style : ChStyle<Style> {
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin    skin;

        UiTitleCard::Style header_style;
        UiPanel::Style     body_style;

        int  header_height   = DPI(72);
        int  section_gap     = DPI(8);
        int  header_body_gap = DPI(4);
        int  body_min_height = DPI(88);

        bool single_open   = false;
        bool enforce_one   = false;
        bool show_chevron  = true;
        UiAlign chevron_side = UiAlign::RIGHT;
        Image glyph_open;
        Image glyph_closed;
        Image glyph_lock;
        bool chevron_scale = false;
        int  chevron_size  = 0;

        bool unified_section_frame = false;
        int  unified_section_radius = DPI(7);
        int  unified_section_frame_width = 1;

        UiSpan body_line_extent = NONE;
        UiLineStyle body_line_style = SOLID;
        int         body_line_thickness = 1;
        Color       body_line_color;

        bool transparent = false;

        bool animation_enabled = true;
        int  anim_open_ms      = 120;
        int  anim_close_ms     = 0;

        void Serialize(Stream& s)
        {
            int blex = (int)body_line_extent;
            int blst = (int)body_line_style;

            s % palette % metrics % skin
              % header_style % body_style
              % header_height % section_gap % header_body_gap % body_min_height
              % single_open % enforce_one % show_chevron % chevron_side
              % glyph_open % glyph_closed % glyph_lock
              % chevron_scale % chevron_size
              % unified_section_frame % unified_section_radius % unified_section_frame_width
              % blex % blst % body_line_thickness % body_line_color
              % animation_enabled % anim_open_ms % anim_close_ms
              % transparent;

            body_line_extent = (UiSpan)blex;
            body_line_style = (UiLineStyle)blst;
        }
    };

private:
    class SectionHeader : public UiTitleCard {
    public:
        UiAccordion* owner = nullptr;
        int          index = -1;
        bool         down  = false;

        void LeftDown(Point p, dword keyflags) override;
        void MouseMove(Point p, dword keyflags) override;
        void LeftUp(Point p, dword keyflags) override;
        bool Key(dword key, int count) override;
    };

    struct Section {
        String       title;
        String       subtitle;
        String       copy;

        SectionHeader header;
        UiPanel       body;
        ParentCtrl    content;

        bool open = true;
        Lock lock = Lock::None;
        int  body_height = -1;

        int  current_body_cy = 0;
        int  target_body_cy  = 0;
        int  anim_from_cy    = 0;
        int  anim_start_ms   = 0;
        int  anim_ms         = 0;
        bool animating       = false;
    };

public:
    UiAccordion();
    virtual ~UiAccordion();

    static const Style& StyleDefault();

    UiAccordion& SetStyle(const Style& s);
    UiAccordion& ClearStyleOverride();
    bool         HasStyleOverride() const { return has_style_override_; }
    const Style& GetStyle() const { return GetEffectiveStyle(); }

    StyledPalette& StyledPaletteRef() { return StyleEdit().palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().metrics; }
    StyledSkin&    StyledSkinRef()    { return StyleEdit().skin; }
    void OnStyleChanged();

    int AddSection(const String& title,
                   const String& subtitle = String(),
                   const String& copy = String(),
                   bool open = true);
    int AddSection(const String& title, bool open);
    void Remove(int i);

    void Clear();
    int  GetCount() const { return sections_.GetCount(); }

    ParentCtrl&  Content(int i);
    UiTitleCard& Header(int i);
    UiPanel&     Body(int i);

    UiAccordion& SetSectionText(int i, const String& title,
                                const String& subtitle = String(),
                                const String& copy = String());
    UiAccordion& SetSectionBodyHeight(int i, int h);

    UiAccordion& Open(int i, bool on = true);
    UiAccordion& Toggle(int i);
    UiAccordion& OpenAll(bool on = true);
    bool         IsOpen(int i) const;

    UiAccordion& SetSingleOpen(bool on = true);
    UiAccordion& SetEnforceOne(bool on = true);
    UiAccordion& ShowChevron(bool on = true);
    UiAccordion& SetChevronSide(UiAlign side);
    UiAccordion& SetChevronGlyphs(const Image& open, const Image& closed, const Image& lock = Image());
    UiAccordion& SetHeaderRuleExtent(UiSpan ex);
    UiAccordion& SetBodyLine(UiSpan ex, int thickness = 1, UiLineStyle style = SOLID, Color c = Null);

    UiAccordion& SetLockMode(int i, Lock mode);
    Lock         GetLockMode(int i) const;

    UiAccordion& SetAnimation(bool enabled, int open_ms = 120, int close_ms = 0);
    UiAccordion& EnableDragReorder(bool on = true);
    bool         IsDragReorderEnabled() const { return drag_reorder_enabled_; }

    void BeginHeaderDrag(int i, Point start_screen);
    void ContinueHeaderDrag(Point p_screen);
    void EndHeaderDrag(bool cancel);
    bool FocusHeader(int i);

    virtual Size GetMinSize() const override;
    virtual void Layout() override;
    virtual void Paint(Draw& w) override;
    virtual void  SetData(const Value& v) override;
    virtual Value GetData() const override;

    Event<int, bool> WhenSectionToggled;
    Event<int, int>  WhenReordered;
    Event<int>       WhenRemoved;
    Event<int>       WhenAdded;

    Event<Draw&, const Rect&,
          const StyledPalette&, const StyledMetrics&, const StyledSkin&,
          StyledState, bool> WhenPaintBackground;

    Event<Draw&, const Rect&,
          const StyledPalette&, const StyledMetrics&, const StyledSkin&,
          StyledState, bool> WhenPaintForeground;

private:
    void InvalidateStyleCache();
    Style& StyleEdit();
    void SyncThemeStyle();
    const Style& GetEffectiveStyle() const;
    int  MeasureSectionBodyHeight(const Section& s, int width) const;
    void ApplySectionStyle(Section& s, int index);
    void RefreshChevron(Section& s);
    void StartSectionAnimation(int i, bool opening);
    void StopAllAnimations();
    void AnimationStep();
    void NormalizePolicyAfterBulkChange();
    void MoveSectionTo(int from, int before);
    void ReindexSections();
    int  ResolveLineWidth(UiSpan ex, int avail) const;
    void PaintRuleLine(Draw& w, int x, int y, int cx, int thickness, UiLineStyle style, Color c) const;

private:
    Style style_;
    uint64 theme_revision_ = 0;
    bool has_style_override_ = false;
    Array<Section> sections_;

    bool drag_reorder_enabled_ = false;
    int  drag_threshold_px_    = DPI(10);
    bool drag_candidate_       = false;
    bool dragging_             = false;
    bool drag_moved_           = false;
    int  drag_from_            = -1;
    int  drag_insert_before_   = -1;
    Point drag_start_screen_   = Point(0, 0);
    StaticRect drag_marker_;

    enum { ANIM_CB_ID = 1 };
};

}

#endif
