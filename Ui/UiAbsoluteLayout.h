#ifndef _Ui_UiAbsoluteLayout_h_
#define _Ui_UiAbsoluteLayout_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    UiAbsoluteLayout
    ================

    Purpose
    - Transparent layout engine for exact child rectangles in local coordinates.

    Contract
    - The layout owns child placement but paints no chrome.
    - Rectangles are exact runtime pixel coordinates relative to this control.
    - Children may overlap; insertion order remains the stacking order.
    - Snapping, DPI conversion, anchoring, and proportional placement belong to
      callers such as UiDesigner, not to this runtime layout.

    Thread context
    - GUI thread only.
*/

#include <CtrlLib/CtrlLib.h>

namespace Upp {

class UiAbsoluteLayout : public Ctrl {
public:
    typedef UiAbsoluteLayout CLASSNAME;

    struct ItemRef {
        ItemRef() = default;
        ItemRef(UiAbsoluteLayout* owner, int index)
            : owner(owner), index(index) {}

        ItemRef& SetRect(const Rect& rect);
        ItemRef& SetRect(int x, int y, int cx, int cy)
        {
            return SetRect(RectC(x, y, cx, cy));
        }
        Rect GetRect() const;
        int  GetIndex() const { return index; }

    private:
        bool IsValid() const;

        UiAbsoluteLayout* owner = nullptr;
        int               index = -1;
    };

    UiAbsoluteLayout();

    ItemRef Add(Ctrl& child, const Rect& rect);
    ItemRef Add(Ctrl& child, int x, int y, int cx, int cy)
    {
        return Add(child, RectC(x, y, cx, cy));
    }

    UiAbsoluteLayout& SetItemRect(int index, const Rect& rect);
    UiAbsoluteLayout& SetItemRect(int index, int x, int y, int cx, int cy)
    {
        return SetItemRect(index, RectC(x, y, cx, cy));
    }

    int   Find(const Ctrl& child) const;
    Ctrl* GetItemCtrl(int index) const;
    Rect  GetItemRect(int index) const;
    int   GetItemCount() const { return items_.GetCount(); }

    bool Remove(int index);
    bool Remove(Ctrl& child);
    void Clear();

    Size GetContentSize() const;
    Size GetMinSize() const override;
    void Layout() override;

private:
    struct Item : Moveable<Item> {
        Ctrl* child = nullptr;
        Rect  rect;
    };

    static Rect NormalizeRect(const Rect& rect);
    void Relayout();

    Vector<Item> items_;
};

} // namespace Upp

#endif
