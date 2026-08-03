#ifndef _Ui_UiBreadcrumbs_h_
#define _Ui_UiBreadcrumbs_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    UiBreadcrumbs
    =============

    Purpose
    - Public header for the UiBreadcrumbs component.

    Intent
    - Define the runtime API, style contract, and integration points used by the rest of the Ui package.

    Thread context
    - GUI thread only.

    Usage
    - Include this header where the component is used or extended. Keep implementation details in the matching .cpp when present.

    Changelog
    - 2026-06: normalized the top-level header documentation.
*/

#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>
#include <Ui/UiTheme.h>

namespace Upp {

class UiBreadcrumbs : public Ctrl, public CtrlStyled<UiBreadcrumbs> {
public:
    typedef UiBreadcrumbs CLASSNAME;

    struct Item : Moveable<Item> {
        String text;
        Value  data;
    };

    struct Style : ChStyle<Style> {
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin    skin;

        UiRole text_role = UiRole::Standard;
        UiRole current_role = UiRole::Accent;
        UiTextSize text_size = UiTextSize::Body;
        UiTextSize current_size = UiTextSize::Body;

        Font font = SansSerifZ(10);
        Font current_font = SansSerifZ(10).Bold();
        Color text_ink = Null;
        Color current_ink = Null;
        Color divider_ink = Null;
        Color current_underline = Null;

        String divider = "/";
        Image divider_icon;
        Size divider_icon_size = Size(0, 0);
        UiIconRenderMode divider_icon_render = UiIconRenderMode::MonoTint;

        Image path_icon;
        UiAlign path_icon_side = UiAlign::LEFT;
        Size path_icon_size = Size(DPI(18), DPI(18));
        UiIconRenderMode path_icon_render = UiIconRenderMode::PreserveColor;

        int item_gap = DPI(6);
        int divider_gap = DPI(8);
        int content_gap = DPI(5);
        int min_height = 0;
        bool current_bold = true;
        bool current_underline_enabled = false;
        int current_underline_width = DPI(2);

        void Serialize(Stream& s)
        {
            s % palette % metrics % skin
              % text_role % current_role % text_size % current_size
              % font % current_font % text_ink % current_ink % divider_ink
              % current_underline
              % divider % divider_icon % divider_icon_size % divider_icon_render
              % path_icon % path_icon_side % path_icon_size % path_icon_render
              % item_gap % divider_gap % content_gap % min_height % current_bold
              % current_underline_enabled % current_underline_width;
        }
    };

    UiBreadcrumbs();

    UiBreadcrumbs& AddCrumb(const String& text, const Value& data = Value());
    UiBreadcrumbs& SetPath(const Vector<String>& path);
    UiBreadcrumbs& SetPath(const String& path, const String& separator = "/");
    UiBreadcrumbs& SetItems(const Vector<Item>& items);
    UiBreadcrumbs& ClearItems();
    int GetCount() const { return items_.GetCount(); }
    const Item& GetItem(int i) const { return items_[i]; }
    Value GetItemData(int i) const;
    String GetPathText(const String& separator = "/") const;

    UiBreadcrumbs& SetCurrentIndex(int i);
    int GetCurrentIndex() const { return current_; }
    Value GetCurrentData() const;
    UiBreadcrumbs& SetTrimOnSelect(bool b = true);
    bool IsTrimOnSelect() const { return trim_on_select_; }

    UiBreadcrumbs& SetDivider(const String& text);
    UiBreadcrumbs& SetDividerIcon(const Image& icon, Size size = Size(0, 0));
    UiBreadcrumbs& ClearDividerIcon();
    UiBreadcrumbs& SetPathIcon(const Image& icon, UiAlign side = UiAlign::LEFT, Size size = Size(0, 0));
    UiBreadcrumbs& ClearPathIcon();
    UiBreadcrumbs& SetPathIconSide(UiAlign side);
    UiBreadcrumbs& SetPathIconSize(Size size);
    UiBreadcrumbs& SetRoles(UiRole text_role, UiRole current_role);
    UiBreadcrumbs& SetTextSizes(UiTextSize text_size, UiTextSize current_size);

    UiBreadcrumbs& SetCustomStyle(const Style& s);
    UiBreadcrumbs& ClearCustomStyle();
    bool HasCustomStyle() const { return has_custom_style_; }
    const Style& GetStyle() const { return GetEffectiveStyle(); }
    const Style& GetCustomStyle() const { return style_; }
    static const Style& StyleDefault();
    static Style ResolveThemeStyle();

    StyledPalette& StyledPaletteRef() { return StyleEdit().palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().metrics; }
    StyledSkin& StyledSkinRef() { return StyleEdit().skin; }
    void OnStyleChanged();

    virtual Size GetMinSize() const override;
    virtual void Layout() override;
    virtual void Paint(Draw& w) override;
    virtual void LeftDown(Point p, dword keyflags) override;
    virtual void MouseMove(Point p, dword keyflags) override;
    virtual void MouseLeave() override;

    Event<int> WhenAction;

private:
    struct Piece : Moveable<Piece> {
        Rect rect;
        bool item = false;
        int index = -1;
    };

    Style& StyleEdit();
    void InvalidateStyleCache();
    void SyncThemeStyle();
    static void ResolveRoleStyle(Style& s, UiThemeMode mode);
    const Style& GetEffectiveStyle() const;
    void RebuildLayout(Size sz) const;
    Size GetItemSize(const Item& item, bool current) const;
    Size GetPathIconSize() const;
    int HitTest(Point p) const;

    Style style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool has_custom_style_ = false;

    Vector<Item> items_;
    int current_ = -1;
    int hot_ = -1;
    bool trim_on_select_ = false;

    mutable bool layout_dirty_ = true;
    mutable Size layout_size_;
    mutable Vector<Piece> pieces_;
};

}

#endif
