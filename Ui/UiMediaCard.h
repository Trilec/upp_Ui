#ifndef _Ui_UiMediaCard_h_
#define _Ui_UiMediaCard_h_

/*
    UiMediaCard
    ===========

    Media-centric presentation with four structural ideas:
      Header (optional, consumes space)
      Media  (main image/content region)
      Overlay + top/bottom Tags (do not consume media space)
      Footer (optional, consumes space)

    The live Ctrl and UiMediaCardRender share one prepared presentation.
*/

#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <Ui/UiTag.h>
#include <Ui/UiMediaFit.h>
#include <Ui/UiItemRender.h>

namespace Upp {

struct UiMediaBandData {
    String title;
    String subtitle;
    String metadata;
    bool visible = true;

    bool HasContent() const
    {
        return visible
            && (!title.IsEmpty() || !subtitle.IsEmpty() || !metadata.IsEmpty());
    }

    void Clear()
    {
        title.Clear();
        subtitle.Clear();
        metadata.Clear();
    }
};

struct UiMediaCardTag : Moveable<UiMediaCardTag> {
    UiTagData tag;
    UiAlign align = UiAlign::LEFT;
};

struct UiMediaOverlayData {
    UiTagData content;
    UiAlign align_h = UiAlign::CENTER;
    UiAlign align_v = UiAlign::CENTER;
    bool visible = false;
};

struct UiMediaCardData {
    UiMediaBandData header;

    Image image;
    Image fallback_icon;
    Image empty_icon;
    String empty_text = "+";

    WithDeepCopy<Vector<UiMediaCardTag>> top_tags;
    WithDeepCopy<Vector<UiMediaCardTag>> bottom_tags;
    UiMediaOverlayData overlay;

    UiMediaBandData footer;

    bool enabled = true;
    Value value;
    Value data;
};

struct UiMediaBandStyle : Moveable<UiMediaBandStyle> {
    StyledPalette palette;
    StyledMetrics metrics;
    StyledSkin skin;

    Font title_font = SansSerifZ(11).Bold();
    Font subtitle_font = SansSerifZ(9);
    Font metadata_font = SansSerifZ(8);

    Color title_ink[4] = { Null, Null, Null, Null };
    Color subtitle_ink[4] = { Null, Null, Null, Null };
    Color metadata_ink[4] = { Null, Null, Null, Null };

    int text_gap = DPI(2);

    void Serialize(Stream& s)
    {
        s % palette % metrics % skin
          % title_font % subtitle_font % metadata_font;
        for(int st = 0; st < 4; st++)
            s % title_ink[st] % subtitle_ink[st] % metadata_ink[st];
        s % text_gap;
    }
};

struct UiMediaBandPresentation : Moveable<UiMediaBandPresentation> {
    Rect bounds;
    Rect title;
    Rect subtitle;
    Rect metadata;

    WString prepared_title;
    WString prepared_subtitle;
    WString prepared_metadata;

    bool visible = false;
};

struct UiMediaCardPresentation {
    Rect outer;
    Rect content;
    Rect media;
    Rect media_content;

    UiMediaBandPresentation header;
    UiMediaBandPresentation footer;

    Rect media_image_rect;
    Rect empty_icon_rect;
    Rect empty_text_rect;

    Image media_image;
    WString prepared_empty_text;

    WithDeepCopy<Vector<UiTagPresentation>> top_tags;
    WithDeepCopy<Vector<UiTagPresentation>> bottom_tags;
    UiTagPresentation overlay;
};

class UiMediaCard : public Ctrl, public CtrlStyled<UiMediaCard> {
public:
    typedef UiMediaCard CLASSNAME;

    struct Style : ChStyle<Style> {
        // Whole-card surface. Transparent/frameless is the default.
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin skin;

        // Media well. This is independently styled and bordered.
        StyledPalette media_palette;
        StyledMetrics media_metrics;
        StyledSkin media_skin;

        // Header/footer are structural bands with independent optional surfaces.
        UiMediaBandStyle header_style;
        UiMediaBandStyle footer_style;

        // Semantic tags and the one optional media overlay.
        UiTagStyle tag_style[4];
        UiTagStyle overlay_style[4];

        UiMediaFit media_fit = UiMediaFit::Cover;
        Size media_aspect = Size(1, 1);

        int section_gap = DPI(6);
        int tag_gap = DPI(4);
        int tag_inset = DPI(6);
        int overlay_inset = DPI(8);
        int empty_icon_size = DPI(28);
        int min_media_extent = DPI(56);

        void Serialize(Stream& s)
        {
            int mf = (int)media_fit;
            s % palette % metrics % skin
              % media_palette % media_metrics % media_skin;
            header_style.Serialize(s);
            footer_style.Serialize(s);
            for(int role = 0; role < 4; role++) {
                tag_style[role].Serialize(s);
                overlay_style[role].Serialize(s);
            }
            s % mf % media_aspect
              % section_gap % tag_gap % tag_inset % overlay_inset
              % empty_icon_size % min_media_extent;
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
    UiMediaCard& SetEmptyCue(const String& text,
                             const Image& icon = Image());

    UiMediaCard& SetHeader(const String& title,
                           const String& subtitle = String(),
                           const String& metadata = String());
    UiMediaCard& ClearHeader();
    UiMediaCard& ShowHeader(bool show = true);

    UiMediaCard& SetFooter(const String& title,
                           const String& subtitle = String(),
                           const String& metadata = String());
    UiMediaCard& ClearFooter();
    UiMediaCard& ShowFooter(bool show = true);

    // Convenience/compatibility: the original title fields are now the footer.
    UiMediaCard& SetTitle(const String& text);
    UiMediaCard& SetSubTitle(const String& text);
    UiMediaCard& SetMetadata(const String& text);

    UiMediaCard& ClearTopTags();
    UiMediaCard& ClearBottomTags();
    UiMediaCard& ClearTags();
    UiMediaCard& AddTopTag(const UiTagData& tag,
                           UiAlign align = UiAlign::LEFT);
    UiMediaCard& AddBottomTag(const UiTagData& tag,
                              UiAlign align = UiAlign::LEFT);
    int GetTopTagCount() const { return data_.top_tags.GetCount(); }
    int GetBottomTagCount() const { return data_.bottom_tags.GetCount(); }

    UiMediaCard& SetOverlay(const UiTagData& content,
                            UiAlign horizontal = UiAlign::CENTER,
                            UiAlign vertical = UiAlign::CENTER);
    UiMediaCard& ClearOverlay();
    UiMediaCard& ShowOverlay(bool show = true);

    UiMediaCard& SetMediaFit(UiMediaFit fit);
    UiMediaCard& SetMediaAspect(Size ratio);

    UiMediaCard& SetSelected(bool selected = true);
    bool IsSelected() const { return selected_; }
    UiMediaCard& SetSelectable(bool selectable = true);
    bool IsSelectable() const { return selectable_; }

    const UiMediaCardPresentation& GetPresentation() const
    {
        return presentation_;
    }

    const UiTagPresentation* FindTagAt(Point p) const;

    Event<> WhenAction;
    Event<String, Value> WhenTagAction;

    // Generic host-owned drop forwarding. UiMediaCard does not interpret file
    // or asset semantics; the host accepts/rejects the PasteClip and supplies
    // any resulting media through SetImage/SetCardData.
    Event<PasteClip&> WhenDrop;

    virtual Size GetMinSize() const override;
    virtual void Layout() override;
    virtual void Paint(Draw& w) override;
    virtual void MouseEnter(Point p, dword flags) override;
    virtual void MouseLeave() override;
    virtual void DragEnter() override;
    virtual void DragAndDrop(Point p, PasteClip& d) override;
    virtual void DragLeave() override;
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
    const Style& GetEffectiveStyle() const;
    void OnStyleChanged();
    void InvalidatePresentation();
    StyledState ResolveState() const;

private:
    Style style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool has_custom_style_ = false;
    UiRole role_;

    UiMediaCardData data_;
    UiMediaCardPresentation presentation_;

    bool hot_ = false;
    bool drop_hot_ = false;
    bool pressed_ = false;
    bool selected_ = false;
    bool selectable_ = true;
    String pressed_tag_id_;
};

UiMediaCard::Style UiResolveMediaCardStyle(UiRole role);
Size UiMeasureMediaCard(const UiMediaCardData& data,
                        const UiMediaCard::Style& style);
UiMediaCardPresentation UiPrepareMediaCard(const UiMediaCardData& data,
                                           const UiMediaCard::Style& style,
                                           const Rect& bounds);
void UiPaintMediaCard(Draw& w,
                      const UiMediaCardData& data,
                      const UiMediaCard::Style& style,
                      const UiMediaCardPresentation& presentation,
                      StyledState state,
                      bool focused = false);

UiMediaCardData UiMakeMediaCardData(const UiItemRenderData& item);

class UiMediaCardRender : public UiItemRender {
public:
    UiMediaCardRender();

    virtual One<UiItemRender> Clone() const override;

    UiMediaCardRender& SetCardStyle(const UiMediaCard::Style& style);
    UiMediaCardRender& ClearCardStyle();
    bool HasCardStyle() const { return has_custom_card_style_; }

    UiMediaCardRender& SetResolver(
        Function<void(const UiItemRenderData&, UiMediaCardData&)> resolver);

    const UiMediaCardPresentation& GetPresentation() const
    {
        return presentation_;
    }

    virtual Size GetContentSize() const override;
    virtual Size GetMinSize() const override;
    virtual void Paint(Draw& w, const UiItemRenderState& state) const override;
    virtual UiItemRenderHit HitTest(Point p) const override;

protected:
    virtual void Layout() override;

private:
    UiMediaCardData ResolveCardData() const;
    UiMediaCard::Style ResolveCardStyle() const;

private:
    UiMediaCard::Style custom_card_style_;
    bool has_custom_card_style_ = false;
    Function<void(const UiItemRenderData&, UiMediaCardData&)> resolver_;

    UiMediaCardData card_data_;
    UiMediaCard::Style resolved_card_style_;
    UiMediaCardPresentation presentation_;
};

} // namespace Upp

#endif
