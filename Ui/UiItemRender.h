#ifndef _Ui_UiItemRender_h_
#define _Ui_UiItemRender_h_

/*
    UiItemRender
    ============

    Purpose
    - Shared lightweight presentation layer for model-backed Ui controls.

    Intent
    - Keep semantic data independent of view geometry and live Ctrl count.
    - Reuse the same presentation objects for List rows, Gallery tiles, Tree/Table
      cells and headers, and later Graph node content.
    - Keep renderer layout prepared outside Paint(); Paint and HitTest consume
      only cached geometry for the currently bound visible surface.

    Thread context
    - GUI thread while used by live controls.

    Usage
    - Ordinary controls provide a default renderer. Advanced callers create a
      UiItemRenderBasic/UiItemRenderImage (or subclass), configure it, and pass it
      to the owning view with SetItemRender(...).
*/

#include <CtrlCore/CtrlCore.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>
#include <Ui/UiDataModels.h>

namespace Upp {

enum class UiRole : byte;

enum UiItemRenderPart : byte {
    UIITEMPART_NONE = 0,
    UIITEMPART_BODY,
    UIITEMPART_CHECK,
    UIITEMPART_IMAGE,
    UIITEMPART_ICON,
    UIITEMPART_TITLE,
    UIITEMPART_SUBTITLE,
    UIITEMPART_DESCRIPTION,
    UIITEMPART_RIGHT_TEXT,
    UIITEMPART_METADATA,
};

struct UiItemRenderHit : Moveable<UiItemRenderHit> {
    UiItemRenderPart part = UIITEMPART_NONE;
    Value action;

    bool IsHit() const { return part != UIITEMPART_NONE; }
};

struct UiItemRenderState : Moveable<UiItemRenderState> {
    bool enabled = true;
    bool selected = false;
    bool hot = false;
    bool pressed = false;
    bool focused = false;
};

struct UiItemRenderData : Moveable<UiItemRenderData> {
    String title;
    String subtitle;
    String description;
    String right_text;

    Image image;
    Image icon;
    UiIconRenderMode icon_render_mode = UiIconRenderMode::PreserveColor;

    Value value;
    Value data;
    UiRole role;

    bool enabled = true;
    bool has_check = false;
    bool checked = false;
    bool emphasized = false;
    bool has_metadata = false;
    Color metadata_color = Null;
    Color custom_ink_color = Null;

    bool use_custom_font = false;
    Font custom_font = StdFont();
    bool underline = false;
    Color underline_color = Null;
    int text_align = ALIGN_LEFT;
    int right_text_align = ALIGN_RIGHT;

    UiItemRenderData();
    UiItemRenderData(const UiItemRenderData& src);
    UiItemRenderData& operator=(const UiItemRenderData& src);
};

UiItemRenderData UiMakeItemRenderData(const UiModelItem& item);

struct UiItemRenderStyle : Moveable<UiItemRenderStyle> {
    StyledPalette palette;
    StyledMetrics metrics;
    StyledSkin skin;

    Font title_font = StdFont();
    Font subtitle_font = StdFont();
    Font description_font = StdFont();
    Font right_font = StdFont();

    int icon_size = DPI(18);
    int image_extent = DPI(64);
    int check_size = DPI(14);
    int content_gap = DPI(6);
    int text_gap = DPI(2);
    int metadata_size = DPI(8);
    int metadata_gap = DPI(6);
    int image_radius = DPI(4);

    Color muted_ink = Color(100, 116, 139);
    Color metadata_default = Color(65, 167, 248);
    Color check_frame = Color(148, 163, 184);
    Color check_fill = Color(17, 24, 39);

    bool show_face = true;
    bool show_icon = true;
    bool show_image = true;
    bool show_subtitle = true;
    bool show_description = true;
    bool show_right_text = true;
    bool show_metadata = true;

    void Serialize(Stream& s)
    {
        s % palette % metrics % skin
          % title_font % subtitle_font % description_font % right_font
          % icon_size % image_extent % check_size
          % content_gap % text_gap % metadata_size % metadata_gap % image_radius
          % muted_ink % metadata_default % check_frame % check_fill
          % show_face % show_icon % show_image % show_subtitle
          % show_description % show_right_text % show_metadata;
    }
};

class UiItemRender {
public:
    UiItemRender();
    virtual ~UiItemRender() {}

    virtual One<UiItemRender> Clone() const = 0;

    UiItemRender& SetData(const UiItemRenderData& data);
    const UiItemRenderData& GetData() const { return data_; }

    UiItemRender& SetCustomStyle(const UiItemRenderStyle& style);
    UiItemRender& ClearCustomStyle();
    bool HasCustomStyle() const { return has_custom_style_; }
    const UiItemRenderStyle& GetStyle() const;

    // Safe to call whenever the view's useful range is synchronized. The
    // virtual Layout() override runs only when data/theme/configuration or the
    // assigned rectangle/direction actually changed.
    bool PrepareLayout(const Rect& rect, UiDirection direction);
    void InvalidateLayout();
    bool IsLayoutDirty() const { return layout_dirty_; }
    int GetLayoutSerial() const { return layout_serial_; }

    Rect GetBounds() const { return bounds_; }
    UiDirection GetDirection() const { return direction_; }

    virtual Size GetContentSize() const = 0;
    virtual Size GetMinSize() const = 0;
    virtual void Paint(Draw& w, const UiItemRenderState& state) const = 0;
    virtual UiItemRenderHit HitTest(Point p) const;

protected:
    virtual void Layout() = 0;

    void CopyConfigurationTo(UiItemRender& target) const;
    void SyncThemeStyle();
    const UiItemRenderStyle& EffectiveStyle() const;
    StyledState ResolveStyledState(const UiItemRenderState& state) const;

    const UiItemRenderData& Data() const { return data_; }
    Rect Bounds() const { return bounds_; }
    UiDirection Direction() const { return direction_; }

private:
    UiItemRenderData data_;
    UiItemRenderStyle custom_style_;
    UiItemRenderStyle themed_style_;
    bool has_custom_style_ = false;
    uint64 theme_revision_ = 0;
    int themed_role_ = -1;

    Rect bounds_;
    UiDirection direction_ = UiDirection::H;
    bool layout_dirty_ = true;
    int layout_serial_ = 0;
};

class UiItemRenderBasic : public UiItemRender {
public:
    UiItemRenderBasic();
    virtual One<UiItemRender> Clone() const override;
    virtual Size GetContentSize() const override;
    virtual Size GetMinSize() const override;
    virtual void Paint(Draw& w, const UiItemRenderState& state) const override;
    virtual UiItemRenderHit HitTest(Point p) const override;

protected:
    virtual void Layout() override;

private:
    Rect content_;
    Rect check_;
    Rect icon_;
    Rect metadata_;
    Rect title_;
    Rect subtitle_;
    Rect description_;
    Rect right_text_;
};

class UiItemRenderImage : public UiItemRender {
public:
    UiItemRenderImage();
    virtual One<UiItemRender> Clone() const override;
    virtual Size GetContentSize() const override;
    virtual Size GetMinSize() const override;
    virtual void Paint(Draw& w, const UiItemRenderState& state) const override;
    virtual UiItemRenderHit HitTest(Point p) const override;

protected:
    virtual void Layout() override;

private:
    Rect content_;
    Rect media_;
    Rect title_;
    Rect subtitle_;
    Rect description_;
    Rect right_text_;
    Rect metadata_;
};

} // namespace Upp

#endif
