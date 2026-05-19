#ifndef _Ui_UiCompositeColor_h_
#define _Ui_UiCompositeColor_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.
    UiCompositeColor
    ================

    Purpose
    - Reusable composite field for one-to-four color swatches plus optional
      value text.

    Intent
    - Move common labeled color-swatch inspector composition out of demos while
      keeping direct color access and compact property-sheet behavior.

    Changelog
    - 2026-04-13: first reusable composite color field extracted from demo patterns.
*/

#include <CtrlLib/CtrlLib.h>
#include <Ui/UiLabel.h>
#include <Ui/UiCompositeSlider.h>

namespace Upp {

class UiCompositeColorSwatch : public Ctrl {
public:
    typedef UiCompositeColorSwatch CLASSNAME;

    UiCompositeColorSwatch();

    void SetColor(Color color);
    Color GetColor() const { return color_; }
    void SetRadius(int radius);
    void SetSlotLabel(const String& label);
    String GetSlotLabel() const { return label_; }

    Event<> WhenAction;

    virtual Size GetMinSize() const override;
    virtual void Paint(Draw& w) override;
    virtual void LeftDown(Point p, dword keyflags) override;
    virtual void MouseEnter(Point p, dword keyflags) override;
    virtual void MouseLeave() override;

private:
    Color color_;
    int radius_ = DPI(6);
    bool hot_ = false;
    String label_;
};

class UiCompositeColor : public Ctrl {
public:
    typedef UiCompositeColor CLASSNAME;

    UiCompositeColor();

    UiCompositeColor& SetLayoutMode(UiCompositeLayoutMode mode);
    UiCompositeColor& SetLabel(const String& text);
    UiCompositeColor& SetValueText(const String& text);
    UiCompositeColor& ShowValue(bool show = true);
    UiCompositeColor& SetValueSelectable(bool selectable = true);
    UiCompositeColor& SetLabelWidth(int cx);
    UiCompositeColor& SetValueWidth(int cx);
    UiCompositeColor& SetFieldGap(int px);
    UiCompositeColor& SetStackGap(int px);
    UiCompositeColor& SetColorCount(int count);
    int GetColorCount() const { return color_count_; }
    UiCompositeColor& SetLabelStyle(const UiLabel::Style& style);
    UiCompositeColor& SetValueStyle(const UiLabel::Style& style);
    UiCompositeColor& SetColor(int index, Color color);
    Color GetColor(int index) const;
    UiCompositeColor& SetColors(const Vector<Color>& colors);
    Vector<Color> GetColors() const;
    UiCompositeColor& SetColorLabel(int index, const String& label);
    String GetColorLabel(int index) const;
    UiCompositeColor& SetSeparatorBefore(int index, bool on = true);
    bool HasSeparatorBefore(int index) const;

    UiLabel& LabelCtrl() { return label_; }
    const UiLabel& LabelCtrl() const { return label_; }
    UiLabel& ValueCtrl() { return value_; }
    const UiLabel& ValueCtrl() const { return value_; }
    UiCompositeColorSwatch& ColorCtrl(int index) { return color_[index]; }
    const UiCompositeColorSwatch& ColorCtrl(int index) const { return color_[index]; }

    virtual Size GetMinSize() const override;
    virtual void Paint(Draw& w) override;
    virtual void Layout() override;

    Event<> WhenAction;

private:
    void SyncValueVisibility();
    void SyncColorVisibility();
    void OpenColorPicker(int active);

private:
    UiLabel label_;
    UiLabel value_;
    UiCompositeColorSwatch color_[4];
    bool separator_before_[4] = { false, false, false, false };

    UiCompositeLayoutMode layout_mode_ = UICOMPOSITE_INLINE;
    bool show_value_ = true;
    bool value_selectable_ = false;
    int color_count_ = 1;
    int label_width_ = DPI(112);
    int value_width_ = DPI(76);
    int field_gap_ = DPI(8);
    int stack_gap_ = DPI(4);
};

}

#endif

