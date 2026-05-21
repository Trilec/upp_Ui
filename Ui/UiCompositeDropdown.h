#ifndef _Ui_UiCompositeDropdown_h_
#define _Ui_UiCompositeDropdown_h_

#include <Ui/UiLabel.h>
#include <Ui/UiDropdown.h>
#include <Ui/UiCompositeSlider.h>
#include <Ui/UiTheme.h>

namespace Upp {

class UiCompositeDropdown : public Ctrl {
public:
    typedef UiCompositeDropdown CLASSNAME;

    UiCompositeDropdown();

    UiCompositeDropdown& SetLayoutMode(UiCompositeLayoutMode mode);
    UiCompositeDropdown& SetLabel(const String& text);
    UiCompositeDropdown& SetLabelWidth(int cx);
    UiCompositeDropdown& SetFieldGap(int px);
    UiCompositeDropdown& SetStackGap(int px);
    UiCompositeDropdown& SetLabelRole(UiRole role);
    UiCompositeDropdown& SetDropdownRole(UiRole role);
    UiCompositeDropdown& SetLabelStyle(const UiLabel::Style& style);

    UiCompositeDropdown& Add(const String& text, const Value& data = Value(), bool enabled = true);
    UiCompositeDropdown& Clear();
    UiCompositeDropdown& Select(int index);
    UiCompositeDropdown& SelectByData(const Value& data);

    UiLabel& LabelCtrl() { return label_; }
    const UiLabel& LabelCtrl() const { return label_; }
    UiDropdown& Dropdown() { return drop_; }
    const UiDropdown& Dropdown() const { return drop_; }

    virtual void SetData(const Value& v) override;
    virtual Value GetData() const override;
    virtual Size GetMinSize() const override;
    virtual void Layout() override;
    virtual void Paint(Draw& w) override;

    Event<int> WhenSelect;
    Event<const Value&> WhenSelectData;
    Event<> WhenOpen;
    Event<> WhenClose;

private:
    void SyncThemeStyle();

    UiLabel label_;
    UiDropdown drop_;

    UiCompositeLayoutMode layout_mode_ = UICOMPOSITE_INLINE;
    UiRole label_role_ = UiRole::Subtle;
    UiRole dropdown_role_ = UiRole::Accent;
    int label_width_ = DPI(112);
    int field_gap_ = DPI(8);
    int stack_gap_ = DPI(4);
    uint64 theme_revision_ = 0;
    bool custom_label_style_ = false;
};

}

#endif
