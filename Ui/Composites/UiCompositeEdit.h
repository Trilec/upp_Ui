#ifndef _Ui_UiCompositeEdit_h_
#define _Ui_UiCompositeEdit_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    UiCompositeEdit
    ===============

    Purpose
    - Public header for the UiCompositeEdit component.

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
#include <Ui/UiLineEdit.h>
#include <Ui/Composites/UiCompositeSlider.h>
#include <Ui/UiTheme.h>

namespace Upp {

class UiCompositeEdit : public Ctrl {
public:
    typedef UiCompositeEdit CLASSNAME;

    struct ValueEdit : UiLineEdit {
        Event<> WhenLoseFocus;
        virtual void LostFocus() override
        {
            UiLineEdit::LostFocus();
            if(WhenLoseFocus)
                WhenLoseFocus();
        }
    };

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
    ValueEdit& Edit() { return edit_; }
    const ValueEdit& Edit() const { return edit_; }

    virtual void SetData(const Value& v) override;
    virtual Value GetData() const override;
    virtual Size GetMinSize() const override;
    virtual void Layout() override;
    virtual void Paint(Draw& w) override;

    Event<> WhenAction;
    Event<> WhenChange;
    Event<> WhenLoseFocus;

private:
    void SyncThemeStyle();

    UiLabel label_;
    ValueEdit edit_;
    UiCompositeLayoutMode layout_mode_ = UICOMPOSITE_INLINE;
    UiRole label_role_ = UiRole::Subtle;
    UiRole edit_role_ = UiRole::Standard;
    int label_width_ = DPI(112);
    int field_gap_ = DPI(8);
    int stack_gap_ = DPI(4);
    uint64 theme_revision_ = 0;
    bool custom_label_style_ = false;
    bool custom_edit_style_ = false;
};

}

#endif
