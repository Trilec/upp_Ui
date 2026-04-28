#ifndef _Ui_UiCompositeColor_h_
#define _Ui_UiCompositeColor_h_

/*
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
    ColorPopUp popup_;
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
    UiCompositeColor& SetSwatchCount(int count);
    int GetSwatchCount() const { return swatch_count_; }
    UiCompositeColor& SetLabelStyle(const UiLabel::Style& style);
    UiCompositeColor& SetValueStyle(const UiLabel::Style& style);
    UiCompositeColor& SetSwatchColor(int index, Color color);
    Color GetSwatchColor(int index) const;

    UiLabel& LabelCtrl() { return label_; }
    const UiLabel& LabelCtrl() const { return label_; }
    UiLabel& ValueCtrl() { return value_; }
    const UiLabel& ValueCtrl() const { return value_; }
    UiCompositeColorSwatch& Swatch(int index) { return swatch_[index]; }
    const UiCompositeColorSwatch& Swatch(int index) const { return swatch_[index]; }

    virtual Size GetMinSize() const override;
    virtual void Layout() override;

    Event<> WhenAction;

private:
    void SyncValueVisibility();
    void SyncSwatchVisibility();

private:
    UiLabel label_;
    UiLabel value_;
    UiCompositeColorSwatch swatch_[4];

    UiCompositeLayoutMode layout_mode_ = UICOMPOSITE_INLINE;
    bool show_value_ = true;
    bool value_selectable_ = false;
    int swatch_count_ = 1;
    int label_width_ = DPI(102);
    int value_width_ = DPI(92);
    int field_gap_ = DPI(6);
    int stack_gap_ = DPI(4);
};

}

#endif
