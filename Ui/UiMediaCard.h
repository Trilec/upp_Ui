#ifndef _Ui_UiMediaCard_h_
#define _Ui_UiMediaCard_h_

/*
    UiMediaCard
    ===========

    Purpose
    - Reusable media-first card for small interactive references and a shared
      presentation contract for later model-view renderers.

    Intent
    - Keep media, labels and compact overlay badges domain-neutral.
    - Prepare image scaling, text fitting and badge geometry outside Paint().
    - Keep file import, AI assets and application validation in the host.
*/

#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <Ui/UiBadge.h>
#include <Ui/UiMediaFit.h>

namespace Upp {

enum class UiMediaBadgeAnchor : byte {
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight
};

struct UiMediaCardBadge : Moveable<UiMediaCardBadge> {
    UiBadgeData badge;
    UiMediaBadgeAnchor anchor = UiMediaBadgeAnchor::TopLeft;
};

struct UiMediaCardData {
    Image image;
    Image fallback_icon;
    String title;
    String subtitle;
    String metadata;
    Image empty_icon;
    String empty_text = "+";
    WithDeepCopy<Vector<UiMediaCardBadge>> badges;
    bool enabled = true;
    Value value;
    Value data;
};

struct UiMediaCardPresentation {
    Rect outer;
    Rect content;
    Rect media;
    Rect text;
    Rect title;
    Rect subtitle;
    Rect metadata;
    Rect media_image_rect;
    Rect empty_icon_rect;
    Rect empty_text_rect;

    Image media_image;
    WString prepared_title;
    WString prepared_subtitle;
    WString prepared_metadata;
    WString prepared_empty_text;

    WithDeepCopy<Vector<UiBadgePresentation>> badges;
    WithDeepCopy<Vector<int>> badge_source_indices;
};

class UiMediaCard : public Ctrl, public CtrlStyled<UiMediaCard> {
public:
    typedef UiMediaCard CLASSNAME;

    struct Style : ChStyle<Style> {
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin    skin;

        StyledPalette media_palette;
        StyledMetrics media_metrics;
        StyledSkin    media_skin;

        Font title_font = SansSerifZ(11).Bold();
        Font subtitle_font = SansSerifZ(9);
        Font metadata_font = SansSerifZ(8);
        Color title_ink[4] = { Null, Null, Null, Null };
        Color subtitle_ink[4] = { Null, Null, Null, Null };
        Color metadata_ink[4] = { Null, Null, Null, Null };

        UiBadgeStyle badge_style[4];

        UiAlign label_side = UiAlign::BOTTOM;
        UiMediaFit media_fit = UiMediaFit::Cover;
        Size media_aspect = Size(1, 1);

        int media_text_gap = DPI(7);
        int text_gap = DPI(2);
        int badge_gap = DPI(4);
        int badge_inset = DPI(6);
        int empty_icon_size = DPI(28);
        int min_media_extent = DPI(56);

        void Serialize(Stream& s)
        {
            int ls = (int)label_side;
            int mf = (int)media_fit;
            s % palette % metrics % skin
              % media_palette % media_metrics % media_skin
              % title_font % subtitle_font % metadata_font;
            for(int st = 0; st < 4; st++)
                s % title_ink[st] % subtitle_ink[st] % metadata_ink[st];
            for(int role = 0; role < 4; role++)
                badge_style[role].Serialize(s);
            s % ls % mf % media_aspect
              % media_text_gap % text_gap % badge_gap % badge_inset
              % empty_icon_size % min_media_extent;
            label_side = (UiAlign)ls;
            media_fit = (UiMediaFit)mf;
        }
    };

    UiMediaCard();

    static const Style& StyleDefault();

    UiMediaCard& SetCustomStyle(const Style& style);
    UiMediaCard& ClearCustomStyle();
    bool HasCustomStyle() const { return has_custom_style_; }
    const Style& GetStyle() const { return GetEffectiveStyle(); }
    const Style& GetCustomStyle() const { return style_; }

    StyledPalette& StyledPaletteRef() { return StyleEdit().palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().metrics; }
    StyledSkin& StyledSkinRef() { return StyleEdit().skin; }

    UiMediaCard& SetRole(UiRole role);
    UiRole GetRole() const { return role_; }

    UiMediaCard& SetCardData(const UiMediaCardData& data);
    const UiMediaCardData& GetCardData() const { return data_; }

    UiMediaCard& SetImage(const Image& image);
    UiMediaCard& ClearImage();
    UiMediaCard& SetFallbackIcon(const Image& image);
    UiMediaCard& SetTitle(const String& text);
    UiMediaCard& SetSubTitle(const String& text);
    UiMediaCard& SetMetadata(const String& text);
    UiMediaCard& SetEmptyCue(const String& text, const Image& icon = Image());

    UiMediaCard& ClearBadges();
    UiMediaCard& AddBadge(const UiBadgeData& badge, UiMediaBadgeAnchor anchor);
    int GetBadgeCount() const { return data_.badges.GetCount(); }

    UiMediaCard& SetLabelSide(UiAlign side);
    UiMediaCard& SetMediaFit(UiMediaFit fit);
    UiMediaCard& SetMediaAspect(Size ratio);

    UiMediaCard& SetSelected(bool selected = true);
    bool IsSelected() const { return selected_; }
    UiMediaCard& SetSelectable(bool selectable = true);
    bool IsSelectable() const { return selectable_; }

    const UiMediaCardPresentation& GetPresentation() const { return presentation_; }
    int HitTestBadge(Point p) const;

    Event<> WhenAction;
    Event<String, Value> WhenBadgeAction;

    virtual Size GetMinSize() const override;
    virtual void Layout() override;
    virtual void Paint(Draw& w) override;
    virtual void MouseEnter(Point p, dword flags) override;
    virtual void MouseLeave() override;
    virtual void LeftDown(Point p, dword flags) override;
    virtual void LeftUp(Point p, dword flags) override;
    virtual bool Key(dword key, int count) override;
    virtual void GotFocus() override;
    virtual void LostFocus() override;
    virtual void CancelMode() override;

private:
    void InvalidateStyleCache();
    Style& StyleEdit();
    void SyncThemeStyle();
    Style ResolveThemeStyle() const;
    const Style& GetEffectiveStyle() const;
    void OnStyleChanged();

    void InvalidatePresentation();
    void RebuildPresentation();
    StyledState ResolveState() const;
    Size MeasureTextBlock(const Style& style) const;
    void LayoutText(const Rect& rect, const Style& style);
    void PrepareMedia(const Style& style);
    void PrepareBadges(const Style& style);

private:
    Style style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool has_custom_style_ = false;
    UiRole role_;

    UiMediaCardData data_;
    UiMediaCardPresentation presentation_;

    bool presentation_dirty_ = true;
    bool hot_ = false;
    bool pressed_ = false;
    bool selected_ = false;
    bool selectable_ = true;
    int pressed_badge_ = -1;
};

} // namespace Upp

#endif
