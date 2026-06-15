#ifndef _Ui_UiCompositeLabel_h_
#define _Ui_UiCompositeLabel_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    UiCompositeLabel
    ================

    Purpose
    - Public header for the UiCompositeLabel component.

    Intent
    - Define the runtime API, style contract, and integration points used by the rest of the Ui package.

    Thread context
    - GUI thread only.

    Usage
    - Include this header where the component is used or extended. Keep implementation details in the matching .cpp when present.

    Changelog
    - 2026-06: normalized the top-level header documentation.
*/

#include <Ui/UiLabel.h>
#include <Ui/Composites/UiCompositeSlider.h>
#include <Ui/UiTheme.h>

namespace Upp {

class UiCompositeLabel : public Ctrl {
public:
    typedef UiCompositeLabel CLASSNAME;

    UiCompositeLabel();

    UiCompositeLabel& SetLabel(const String& text);
    UiCompositeLabel& SetValueText(const String& text);
    UiCompositeLabel& SetLabelWidth(int cx);
    UiCompositeLabel& SetFieldGap(int px);
    UiCompositeLabel& SetLabelRole(UiRole role);
    UiCompositeLabel& SetValueRole(UiRole role);
    UiCompositeLabel& SetLabelStyle(const UiLabel::Style& style);
    UiCompositeLabel& SetValueStyle(const UiLabel::Style& style);

    UiLabel& LabelCtrl() { return label_; }
    const UiLabel& LabelCtrl() const { return label_; }
    UiLabel& ValueCtrl() { return value_; }
    const UiLabel& ValueCtrl() const { return value_; }

    virtual void SetData(const Value& v) override;
    virtual Value GetData() const override;
    virtual Size GetMinSize() const override;
    virtual void Layout() override;
    virtual void Paint(Draw& w) override;

private:
    void SyncThemeStyle();

    UiLabel label_;
    UiLabel value_;
    UiRole label_role_ = UiRole::Subtle;
    UiRole value_role_ = UiRole::Accent;
    int label_width_ = DPI(112);
    int field_gap_ = DPI(8);
    uint64 theme_revision_ = 0;
    bool custom_label_style_ = false;
    bool custom_value_style_ = false;
};

}

#endif
