#ifndef _Ui_UiCompositeLabel_h_
#define _Ui_UiCompositeLabel_h_

#include <Ui/UiLabel.h>
#include <Ui/UiCompositeSlider.h>
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

private:
    void SyncThemeStyle();

    UiLabel label_;
    UiLabel value_;
    UiRole label_role_ = UiRole::Subtle;
    UiRole value_role_ = UiRole::Accent;
    int label_width_ = DPI(112);
    int field_gap_ = DPI(8);
};

}

#endif
