#ifndef _Ui_UiCompositeSlider_h_
#define _Ui_UiCompositeSlider_h_

/*
    UiCompositeSlider
    =================

    Purpose
    - Reusable composite field that pairs a UiSlider with label and value text.

    Intent
    - Keep common inspector/property slider composition out of demos and out of
      the primitive UiSlider control while still exposing the inner controls for
      direct styling and behavior access.

    Thread context
    - GUI thread only.

    Usage
    - Use for property sheets, inspectors, and settings panes that need a label,
      a slider, and a formatted value readout in one control.

    Changelog
    - 2026-04-13: extracted first reusable composite slider from demo row patterns.
*/

#include <Ui/UiLabel.h>
#include <Ui/UiSlider.h>

namespace Upp {

enum UiCompositeLayoutMode : byte {
    UICOMPOSITE_INLINE = 0,
    UICOMPOSITE_STACKED,
};

class UiCompositeSlider : public Ctrl {
public:
    typedef UiCompositeSlider CLASSNAME;

    UiCompositeSlider();

    UiCompositeSlider& SetLayoutMode(UiCompositeLayoutMode mode);
    UiCompositeLayoutMode GetLayoutMode() const { return layout_mode_; }

    UiCompositeSlider& SetLabel(const String& text);
    UiCompositeSlider& SetValueText(const String& text);
    UiCompositeSlider& ShowValue(bool show = true);
    bool IsValueShown() const { return show_value_; }

    UiCompositeSlider& SetValueSelectable(bool selectable = true);
    bool IsValueSelectable() const { return value_selectable_; }

    UiCompositeSlider& SetLabelWidth(int cx);
    UiCompositeSlider& SetValueWidth(int cx);
    UiCompositeSlider& SetFieldGap(int px);
    UiCompositeSlider& SetStackGap(int px);

    UiCompositeSlider& SetLabelStyle(const UiLabel::Style& style);
    UiCompositeSlider& SetValueStyle(const UiLabel::Style& style);

    UiLabel& LabelCtrl() { return label_; }
    const UiLabel& LabelCtrl() const { return label_; }
    UiSlider& Slider() { return slider_; }
    const UiSlider& Slider() const { return slider_; }
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
    UiSlider slider_;
    UiLabel value_;

    UiCompositeLayoutMode layout_mode_ = UICOMPOSITE_INLINE;
    bool show_value_ = true;
    bool value_selectable_ = false;
    int label_width_ = DPI(82);
    int value_width_ = DPI(56);
    int field_gap_ = DPI(4);
    int stack_gap_ = DPI(4);
};

}

#endif
