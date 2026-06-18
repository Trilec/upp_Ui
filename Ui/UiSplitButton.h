#ifndef _Ui_UiSplitButton_h_
#define _Ui_UiSplitButton_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    UiSplitButton
    =============

    Purpose
    - Button-family control with one primary command area and one dropdown
      command area.

    Intent
    - Keep "default command plus related choices" as a first-class control
      without folding menu/dropdown state into UiButton or UiToolButton.
    - Reuse UiButton's theme, text/icon layout, focus, and primary-action
      behavior, then add only the split hit target and lightweight popup list.
    - Keep popup sizing independent from the closed control size so compact
      toolbar/header buttons can still expose readable recent/history choices.

    Thread context
    - GUI thread only.

    Usage
    - Configure text/icon like UiButton.
    - Use WhenAction for the primary command.
    - Add popup rows with Add(), then handle WhenSelect for dropdown choices.
    - Use SetPopupMinWidth() when popup row text is wider than the button.
    - Use SetSplitContentGap() and SetSplitIconSize() to tune the split lane.

    Changelog
    - 2026-05: introduced as a dedicated split-button control after Designer
      save/load recent-file handling needed a compact primary action plus a
      wider history popup.
    - 2026-05: added independent split-gap and split-chevron sizing so the
      Designer can expose body inset and split lane spacing without hacks.
*/

#include <Ui/UiButton.h>

namespace Upp {

class UiSplitButton : public UiButton {
public:
    typedef UiSplitButton CLASSNAME;
    using Style = UiButton::Style;

    struct Item : Moveable<Item> {
        String text;
        Value  data;
        bool   enabled = true;
        bool   separator_before = false;
        bool   group_header = false;
        String description;
        Image  icon;
        UiIconRenderMode icon_render_mode = UiIconRenderMode::PreserveColor;

        Item() {}
        Item(const String& text, const Value& data = Value(), bool enabled = true)
            : text(text), data(data), enabled(enabled) {}
    };

private:
    class PopupWindow : public TopWindow {
    public:
        UiSplitButton *owner = nullptr;

        void Init(UiSplitButton *button);
        void SetHot(int index);
        int  HitTest(Point p) const;
        Rect GetItemRect(int index) const;

        virtual void Paint(Draw& w) override;
        virtual void LeftDown(Point p, dword flags) override;
        virtual void MouseMove(Point p, dword flags) override;
        virtual void MouseLeave() override;
        virtual bool Key(dword key, int count) override;
        virtual void Deactivate() override;
    };

    Vector<Item> items_;
    PopupWindow popup_;

    bool split_pressed_ = false;
    bool split_hot_ = false;
    bool popup_open_ = false;
    int hot_item_ = -1;
    bool pending_separator_ = false;

    int split_width_ = DPI(30);
    int split_icon_size_ = DPI(16);
    int split_content_gap_ = DPI(4);
    int popup_min_width_ = DPI(180);
    int popup_max_items_ = 10;
    int popup_item_height_ = DPI(30);
    int popup_space_ = DPI(5);

    Rect GetSplitRect() const;
    Rect GetMainRect() const;
    int  GetPopupRowHeight() const;
    int  GetPopupHeightForVisibleItems() const;
    void OpenPopupInternal();
    void ClosePopupInternal(int select_index = -1, bool fire_select = false);
    void UpdatePopupPosition();
    void SelectPopupItem(int index);
    bool IsSelectableItem(int index) const;
    int  FindNextSelectable(int start, int step) const;
    void DrawSplitAffordance(Draw& w, const Rect& r);

protected:
    virtual Style ResolveThemeStyle() const override;
    virtual Rect GetContentLayoutRect(const Rect& outer, const Style& style) const override;

public:
    UiSplitButton();

    UiSplitButton& SetText(const String& text) { UiButton::SetText(text); return *this; }
    UiSplitButton& SetIcon(const Image& img) { UiButton::SetIcon(img); return *this; }
    UiSplitButton& SetIconState(const Image& img, StyledState state) { UiButton::SetIconState(img, state); return *this; }
    UiSplitButton& SetIcons(const Image& normal,
                            const Image& hot      = Image(),
                            const Image& pressed  = Image(),
                            const Image& disabled = Image())
    {
        UiButton::SetIcons(normal, hot, pressed, disabled);
        return *this;
    }
    UiSplitButton& ClearIcon() { UiButton::ClearIcon(); return *this; }
    UiSplitButton& SetIconSize(Size sz) { UiButton::SetIconSize(sz); return *this; }
    UiSplitButton& SetIconSize(int cx, int cy) { UiButton::SetIconSize(cx, cy); return *this; }
    UiSplitButton& SetIconRenderMode(UiIconRenderMode mode) { UiButton::SetIconRenderMode(mode); return *this; }
    UiSplitButton& SetIconSide(UiAlign side) { UiButton::SetIconSide(side); return *this; }
    UiSplitButton& SetAlign(UiAlign h, UiAlign v) { UiButton::SetAlign(h, v); return *this; }
    UiSplitButton& SetContentGap(int gap) { UiButton::SetContentGap(gap); return *this; }
    UiSplitButton& SetCustomStyle(const Style& s) { UiButton::SetCustomStyle(s); return *this; }
    UiSplitButton& ClearCustomStyle() { UiButton::ClearCustomStyle(); return *this; }

    UiSplitButton& Add(const String& text, const Value& data = Value(), bool enabled = true);
    UiSplitButton& Add(const Item& item);
    UiSplitButton& AddSeparator();
    UiSplitButton& AddGroupHeader(const String& text);
    UiSplitButton& ClearItems();
    int            GetCount() const { return items_.GetCount(); }
    const Item&    GetItem(int i) const { return items_[i]; }

    UiSplitButton& SetItemDescription(int index, const String& desc);
    UiSplitButton& SetItemIcon(int index, const Image& icon, UiIconRenderMode mode = UiIconRenderMode::PreserveColor);
    UiSplitButton& SetItemEnabled(int index, bool enabled = true);
    UiSplitButton& SetItemSeparatorBefore(int index, bool on = true);
    UiSplitButton& SetItemGroupHeader(int index, bool on = true);

    UiSplitButton& SetSplitWidth(int width);
    UiSplitButton& SetSplitIconSize(int size);
    int            GetSplitIconSize() const { return split_icon_size_; }
    UiSplitButton& SetSplitContentGap(int gap);
    int            GetSplitContentGap() const { return split_content_gap_; }
    UiSplitButton& SetPopupMinWidth(int width);
    UiSplitButton& SetPopupMaxItems(int count);
    UiSplitButton& SetPopupItemHeight(int height);

    bool IsPopupOpen() const { return popup_open_; }
    UiSplitButton& OpenPopup();
    UiSplitButton& ClosePopup();
    UiSplitButton& TogglePopup();
    void DebugSelectPopupItem(int index) { SelectPopupItem(index); }

    // Popup callbacks are posted after popup teardown so callers can rebuild
    // surrounding UI safely. Primary button actions still follow UiButton's
    // normal synchronous action model.
    Event<int, const Value&> WhenSelect;
    Event<> WhenOpen;
    Event<> WhenClose;

    virtual void Paint(Draw& w) override;
    virtual void LeftDown(Point p, dword keyflags) override;
    virtual void LeftUp(Point p, dword keyflags) override;
    virtual void MouseMove(Point p, dword keyflags) override;
    virtual void MouseLeave() override;
    virtual void CancelMode() override;
    virtual bool Key(dword key, int count) override;
    virtual Size GetMinSize() const override;
    virtual String GetDesc() const override;
};

} // namespace Upp

#endif
