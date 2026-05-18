#ifndef _Ui_UiCompositeEdit_h_
#define _Ui_UiCompositeEdit_h_

#include <Ui/UiLabel.h>
#include <Ui/UiLineEdit.h>
#include <Ui/UiCompositeSlider.h>
#include <Ui/UiTheme.h>

namespace Upp {

class UiCompositeEdit : public Ctrl {
public:
    typedef UiCompositeEdit CLASSNAME;

    UiCompositeEdit();

    UiCompositeEdit& SetLayoutMode(UiCompositeLayoutMode mode);
    UiCompositeEdit& SetLabel(const String& text);
    UiCompositeEdit& SetLabelWidth(int cx);
    UiCompositeEdit& SetFieldGap(int px);
    UiCompositeEdit& SetStackGap(int px);
    UiCompositeEdit& SetLabelRole(UiRole role);
    UiCompositeEdit& SetEditRole(UiRole role);
    UiCompositeEdit& SetLabelStyle(const UiLabel::Style& style);
    UiCompositeEdit& SetEditStyle(const UiBaseEdit::Style& style);

    UiLabel& LabelCtrl() { return label_; }
    const UiLabel& LabelCtrl() const { return label_; }
    UiLineEdit& Edit() { return edit_; }
    const UiLineEdit& Edit() const { return edit_; }

    virtual void SetData(const Value& v) override;
    virtual Value GetData() const override;
    virtual Size GetMinSize() const override;
    virtual void Layout() override;

    Event<> WhenAction;
    Event<> WhenChange;

private:
    void SyncThemeStyle();

    UiLabel label_;
    UiLineEdit edit_;
    UiCompositeLayoutMode layout_mode_ = UICOMPOSITE_INLINE;
    UiRole label_role_ = UiRole::Subtle;
    UiRole edit_role_ = UiRole::Standard;
    int label_width_ = DPI(112);
    int field_gap_ = DPI(8);
    int stack_gap_ = DPI(4);
};

}

#endif
