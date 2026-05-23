#ifndef _Ui_UiCompositeToggle_h_
#define _Ui_UiCompositeToggle_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.
    UiCompositeToggle
    =================

    Purpose
    - Reusable composite field that pairs a UiToggle with label and optional
      value text.

    Intent
    - Keep labeled toggle inspector/settings composition out of the primitive
      UiToggle control while still allowing direct access to the inner toggle.

    Changelog
    - 2026-04-13: first reusable composite toggle extracted from demo patterns.
*/

#include <Ui/UiLabel.h>
#include <Ui/UiToggle.h>
#include <Ui/Composites/UiCompositeSlider.h>

namespace Upp {

class UiCompositeToggle : public Ctrl {
public:
    typedef UiCompositeToggle CLASSNAME;

    UiCompositeToggle();

    UiCompositeToggle& SetLayoutMode(UiCompositeLayoutMode mode);
    UiCompositeLayoutMode GetLayoutMode() const { return layout_mode_; }

    UiCompositeToggle& SetLabel(const String& text);
    UiCompositeToggle& SetValueText(const String& text);
    UiCompositeToggle& ShowValue(bool show = true);
    UiCompositeToggle& SetValueSelectable(bool selectable = true);
    UiCompositeToggle& SetLabelWidth(int cx);
    UiCompositeToggle& SetValueWidth(int cx);
    UiCompositeToggle& SetFieldGap(int px);
    UiCompositeToggle& SetStackGap(int px);
    UiCompositeToggle& SetLabelStyle(const UiLabel::Style& style);
    UiCompositeToggle& SetValueStyle(const UiLabel::Style& style);

    UiLabel& LabelCtrl() { return label_; }
    const UiLabel& LabelCtrl() const { return label_; }
    UiToggle& Toggle() { return toggle_; }
    const UiToggle& Toggle() const { return toggle_; }
    UiLabel& ValueCtrl() { return value_; }
    const UiLabel& ValueCtrl() const { return value_; }

    virtual void SetData(const Value& v) override;
    virtual Value GetData() const override;
    virtual Size GetMinSize() const override;
    virtual void Layout() override;

    Event<> WhenAction;

private:
    void SyncValueVisibility();

private:
    UiLabel label_;
    UiToggle toggle_;
    UiLabel value_;

    UiCompositeLayoutMode layout_mode_ = UICOMPOSITE_INLINE;
    bool show_value_ = false;
    bool value_selectable_ = false;
    int label_width_ = DPI(112);
    int value_width_ = DPI(42);
    int field_gap_ = DPI(8);
    int stack_gap_ = DPI(4);
};

}

#endif

