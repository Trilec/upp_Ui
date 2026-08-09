#ifndef _examples_DemoPropertyRows_h_
#define _examples_DemoPropertyRows_h_

// Transitional demo-only property rows.
// These are intentionally outside the Ui package: production code should use
// primitive Ui controls, UiSliderEdit, UiColorMatrix, and PropertyEditor.

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.
    DemoSliderRow
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
#include <Ui/UiLineEdit.h>

namespace Upp {

enum DemoRowLayoutMode : byte {
    DEMO_ROW_INLINE = 0,
    DEMO_ROW_STACKED,
};

class DemoSliderRow : public Ctrl {
public:
    typedef DemoSliderRow CLASSNAME;

    DemoSliderRow();

    DemoSliderRow& SetLayoutMode(DemoRowLayoutMode mode);
    DemoRowLayoutMode GetLayoutMode() const { return layout_mode_; }

    DemoSliderRow& SetLabel(const String& text);
    DemoSliderRow& SetValueText(const String& text);
    DemoSliderRow& ShowValue(bool show = true);
    bool IsValueShown() const { return show_value_; }

    DemoSliderRow& SetValueSelectable(bool selectable = true);
    bool IsValueSelectable() const { return value_selectable_; }
    DemoSliderRow& EnableValueEdit(bool on = true);
    bool IsValueEditEnabled() const { return value_edit_enabled_; }

    DemoSliderRow& SetLabelWidth(int cx);
    DemoSliderRow& SetValueWidth(int cx);
    DemoSliderRow& SetFieldGap(int px);
    DemoSliderRow& SetStackGap(int px);

    DemoSliderRow& SetLabelStyle(const UiLabel::Style& style);
    DemoSliderRow& SetValueStyle(const UiLabel::Style& style);

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
    virtual void LeftDouble(Point p, dword keyflags) override;

    Event<> WhenAction;
    Event<> WhenChanging;

private:
    struct ValueLabel : UiLabel {
        Event<Point, dword> WhenDoubleClick;
        virtual void LeftDouble(Point p, dword keyflags) override
        {
            if(WhenDoubleClick)
                WhenDoubleClick(p, keyflags);
        }
    };

    struct ValueEdit : UiLineEdit {
        Event<> WhenEscape;
        Event<> WhenLoseFocus;
        virtual bool Key(dword key, int count) override
        {
            if(key == K_ESCAPE) {
                if(WhenEscape)
                    WhenEscape();
                return true;
            }
            return UiLineEdit::Key(key, count);
        }
        virtual void LostFocus() override
        {
            UiLineEdit::LostFocus();
            if(WhenLoseFocus)
                WhenLoseFocus();
        }
    };

    void SyncValueVisibility();
    void BeginValueEdit();
    void CommitValueEdit();
    void CancelValueEdit();
    void SyncValueEditRect();
    bool IsEditingValue() const { return editing_value_; }

private:
    UiLabel label_;
    UiSlider slider_;
    ValueLabel value_;
    ValueEdit value_edit_;

    DemoRowLayoutMode layout_mode_ = DEMO_ROW_INLINE;
    bool show_value_ = true;
    bool value_selectable_ = false;
    bool value_edit_enabled_ = true;
    bool editing_value_ = false;
    int label_width_ = DPI(112);
    int value_width_ = DPI(48);
    int field_gap_ = DPI(8);
    int stack_gap_ = DPI(4);
};

}




/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    DemoToggleRow
    =================

    Purpose
    - Public header for the DemoToggleRow component.

    Intent
    - Define the runtime API, style contract, and integration points used by the rest of the Ui package.

    Thread context
    - GUI thread only.

    Usage
    - Include this header where the component is used or extended. Keep implementation details in the matching .cpp when present.

    Changelog
    - 2026-06: normalized the top-level header documentation.
*/

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.
    DemoToggleRow
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

namespace Upp {

class DemoToggleRow : public Ctrl {
public:
    typedef DemoToggleRow CLASSNAME;

    DemoToggleRow();

    DemoToggleRow& SetLayoutMode(DemoRowLayoutMode mode);
    DemoRowLayoutMode GetLayoutMode() const { return layout_mode_; }

    DemoToggleRow& SetLabel(const String& text);
    DemoToggleRow& SetValueText(const String& text);
    DemoToggleRow& ShowValue(bool show = true);
    DemoToggleRow& SetValueSelectable(bool selectable = true);
    DemoToggleRow& SetLabelWidth(int cx);
    DemoToggleRow& SetValueWidth(int cx);
    DemoToggleRow& SetFieldGap(int px);
    DemoToggleRow& SetStackGap(int px);
    DemoToggleRow& SetLabelStyle(const UiLabel::Style& style);
    DemoToggleRow& SetValueStyle(const UiLabel::Style& style);

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

    DemoRowLayoutMode layout_mode_ = DEMO_ROW_INLINE;
    bool show_value_ = false;
    bool value_selectable_ = false;
    int label_width_ = DPI(112);
    int value_width_ = DPI(42);
    int field_gap_ = DPI(8);
    int stack_gap_ = DPI(4);
};

}




/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    DemoColorRow
    ================

    Purpose
    - Public header for the DemoColorRow component.

    Intent
    - Define the runtime API, style contract, and integration points used by the rest of the Ui package.

    Thread context
    - GUI thread only.

    Usage
    - Include this header where the component is used or extended. Keep implementation details in the matching .cpp when present.

    Changelog
    - 2026-06: normalized the top-level header documentation.
*/

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.
    DemoColorRow
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

namespace Upp {

class DemoColorRowSwatch : public Ctrl {
public:
    typedef DemoColorRowSwatch CLASSNAME;

    DemoColorRowSwatch();

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

class DemoColorRow : public Ctrl {
public:
    typedef DemoColorRow CLASSNAME;

    DemoColorRow();

    DemoColorRow& SetLayoutMode(DemoRowLayoutMode mode);
    DemoColorRow& SetLabel(const String& text);
    DemoColorRow& SetValueText(const String& text);
    DemoColorRow& ShowValue(bool show = true);
    DemoColorRow& SetValueSelectable(bool selectable = true);
    DemoColorRow& SetLabelWidth(int cx);
    DemoColorRow& SetValueWidth(int cx);
    DemoColorRow& SetFieldGap(int px);
    DemoColorRow& SetStackGap(int px);
    DemoColorRow& SetColorCount(int count);
    int GetColorCount() const { return color_count_; }
    DemoColorRow& SetLabelStyle(const UiLabel::Style& style);
    DemoColorRow& SetValueStyle(const UiLabel::Style& style);
    DemoColorRow& SetColor(int index, Color color);
    Color GetColor(int index) const;
    DemoColorRow& SetColors(const Vector<Color>& colors);
    Vector<Color> GetColors() const;
    DemoColorRow& SetColorLabel(int index, const String& label);
    String GetColorLabel(int index) const;
    DemoColorRow& SetSeparatorBefore(int index, bool on = true);
    bool HasSeparatorBefore(int index) const;

    UiLabel& LabelCtrl() { return label_; }
    const UiLabel& LabelCtrl() const { return label_; }
    UiLabel& ValueCtrl() { return value_; }
    const UiLabel& ValueCtrl() const { return value_; }
    DemoColorRowSwatch& ColorCtrl(int index);
    const DemoColorRowSwatch& ColorCtrl(int index) const { return color_[index]; }

    virtual Size GetMinSize() const override;
    virtual void Paint(Draw& w) override;
    virtual void Layout() override;

    Event<> WhenAction;

private:
    void SyncValueVisibility();
    void SyncColorVisibility();
    void EnsureColorStorage(int count);
    void OpenColorPicker(int active);

private:
    UiLabel label_;
    UiLabel value_;
    Array<DemoColorRowSwatch> color_;
    Vector<bool> separator_before_;

    DemoRowLayoutMode layout_mode_ = DEMO_ROW_INLINE;
    bool show_value_ = true;
    bool value_selectable_ = false;
    int color_count_ = 1;
    int label_width_ = DPI(112);
    int value_width_ = DPI(76);
    int field_gap_ = DPI(8);
    int stack_gap_ = DPI(4);
};

}




/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    DemoDropdownRow
    ===================

    Purpose
    - Public header for the DemoDropdownRow component.

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
#include <Ui/UiDropdown.h>
#include <Ui/UiTheme.h>

namespace Upp {

class DemoDropdownRow : public Ctrl {
public:
    typedef DemoDropdownRow CLASSNAME;

    DemoDropdownRow();

    DemoDropdownRow& SetLayoutMode(DemoRowLayoutMode mode);
    DemoDropdownRow& SetLabel(const String& text);
    DemoDropdownRow& SetLabelWidth(int cx);
    DemoDropdownRow& SetFieldGap(int px);
    DemoDropdownRow& SetStackGap(int px);
    DemoDropdownRow& SetLabelRole(UiRole role);
    DemoDropdownRow& SetDropdownRole(UiRole role);
    DemoDropdownRow& SetLabelStyle(const UiLabel::Style& style);

    DemoDropdownRow& Add(const String& text, const Value& data = Value(), bool enabled = true);
    DemoDropdownRow& Clear();
    DemoDropdownRow& Select(int index);
    DemoDropdownRow& SelectByData(const Value& data);

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

    DemoRowLayoutMode layout_mode_ = DEMO_ROW_INLINE;
    UiRole label_role_ = UiRole::Subtle;
    UiRole dropdown_role_ = UiRole::Accent;
    int label_width_ = DPI(112);
    int field_gap_ = DPI(8);
    int stack_gap_ = DPI(4);
    uint64 theme_revision_ = 0;
    bool custom_label_style_ = false;
};

}



/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    DemoLabelRow
    ================

    Purpose
    - Public header for the DemoLabelRow component.

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
#include <Ui/UiTheme.h>

namespace Upp {

class DemoLabelRow : public Ctrl {
public:
    typedef DemoLabelRow CLASSNAME;

    DemoLabelRow();

    DemoLabelRow& SetLabel(const String& text);
    DemoLabelRow& SetValueText(const String& text);
    DemoLabelRow& SetLabelWidth(int cx);
    DemoLabelRow& SetFieldGap(int px);
    DemoLabelRow& SetLabelRole(UiRole role);
    DemoLabelRow& SetValueRole(UiRole role);
    DemoLabelRow& SetLabelStyle(const UiLabel::Style& style);
    DemoLabelRow& SetValueStyle(const UiLabel::Style& style);

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



/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    DemoEditRow
    ===============

    Purpose
    - Public header for the DemoEditRow component.

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
#include <Ui/UiTheme.h>

namespace Upp {

class DemoEditRow : public Ctrl {
public:
    typedef DemoEditRow CLASSNAME;

    struct ValueEdit : UiLineEdit {
        Event<> WhenLoseFocus;
        virtual void LostFocus() override
        {
            UiLineEdit::LostFocus();
            if(WhenLoseFocus)
                WhenLoseFocus();
        }
    };

    DemoEditRow();

    DemoEditRow& SetLayoutMode(DemoRowLayoutMode mode);
    DemoEditRow& SetLabel(const String& text);
    DemoEditRow& SetLabelWidth(int cx);
    DemoEditRow& SetFieldGap(int px);
    DemoEditRow& SetStackGap(int px);
    DemoEditRow& SetLabelRole(UiRole role);
    DemoEditRow& SetEditRole(UiRole role);
    DemoEditRow& SetLabelStyle(const UiLabel::Style& style);
    DemoEditRow& SetEditStyle(const UiBaseEdit::Style& style);

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
    DemoRowLayoutMode layout_mode_ = DEMO_ROW_INLINE;
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


#include <Ui/UiTheme.h>

namespace Upp {

DemoSliderRow::DemoSliderRow()
{
    Add(label_);
    Add(slider_);
    Add(value_);
    Add(value_edit_);
    slider_.WhenAction = [=] { WhenAction(); };
    slider_.WhenChanging = [=] { WhenChanging(); };
    label_.NoWantFocus();
    value_.NoWantFocus();
    value_.WhenDoubleClick = [=](Point, dword) {
        if(value_edit_enabled_)
            BeginValueEdit();
    };
    value_edit_.Hide();
    value_edit_.WhenAction = [=] { CommitValueEdit(); };
    value_edit_.WhenEscape = [=] { CancelValueEdit(); };
    value_edit_.WhenLoseFocus = [=] {
        if(editing_value_)
            CommitValueEdit();
    };
    UiLabel::Style label_style = UiTheme::ResolveLabel(UiRole::Subtle);
    label_style.font = SansSerifZ(9);
    UiLabel::Style value_style = UiTheme::ResolveLabel(UiRole::Standard);
    value_style.font = SansSerifZ(9);
    label_.SetCustomStyle(label_style);
    value_.SetCustomStyle(value_style);
    slider_.SetTrackSize(Size(DPI(1000), DPI(3)));
    value_.SetSizeMin(Size(value_width_, 0));
    SyncValueVisibility();
}

DemoSliderRow& DemoSliderRow::SetLayoutMode(DemoRowLayoutMode mode)
{
    if(layout_mode_ == mode)
        return *this;
    layout_mode_ = mode;
    RefreshLayout();
    Refresh();
    return *this;
}

DemoSliderRow& DemoSliderRow::SetLabel(const String& text)
{
    label_.SetText(text);
    RefreshLayout();
    Refresh();
    return *this;
}

DemoSliderRow& DemoSliderRow::SetValueText(const String& text)
{
	String current = value_.GetText();
	if(current == text) {
		if(!editing_value_)
			value_edit_.SetTextUtf8(text);
		return *this;
	}
	value_.SetText(text);
	if(!editing_value_)
		value_edit_.SetTextUtf8(text);
	value_.Refresh();
	if(show_value_ && editing_value_)
		value_edit_.Refresh();
    return *this;
}

DemoSliderRow& DemoSliderRow::ShowValue(bool show)
{
    if(show_value_ == show)
        return *this;
    show_value_ = show;
    SyncValueVisibility();
    RefreshLayout();
    Refresh();
    return *this;
}

DemoSliderRow& DemoSliderRow::SetValueSelectable(bool selectable)
{
    value_selectable_ = selectable;
    value_.SetSelectable(selectable);
    return *this;
}

DemoSliderRow& DemoSliderRow::EnableValueEdit(bool on)
{
    value_edit_enabled_ = on;
    if(!value_edit_enabled_ && editing_value_)
        CancelValueEdit();
    return *this;
}

DemoSliderRow& DemoSliderRow::SetLabelWidth(int cx)
{
    label_width_ = max(0, cx);
    RefreshLayout();
    return *this;
}

DemoSliderRow& DemoSliderRow::SetValueWidth(int cx)
{
    value_width_ = max(0, cx);
    value_.SetSizeMin(Size(value_width_, 0));
    RefreshLayout();
    Refresh();
    return *this;
}

DemoSliderRow& DemoSliderRow::SetFieldGap(int px)
{
    field_gap_ = max(0, px);
    RefreshLayout();
    return *this;
}

DemoSliderRow& DemoSliderRow::SetStackGap(int px)
{
    stack_gap_ = max(0, px);
    RefreshLayout();
    return *this;
}

DemoSliderRow& DemoSliderRow::SetLabelStyle(const UiLabel::Style& style)
{
    label_.SetCustomStyle(style);
    Refresh();
    return *this;
}

DemoSliderRow& DemoSliderRow::SetValueStyle(const UiLabel::Style& style)
{
    value_.SetCustomStyle(style);
    Refresh();
    return *this;
}

void DemoSliderRow::SetData(const Value& v)
{
    slider_.SetData(v);
}

Value DemoSliderRow::GetData() const
{
    return slider_.GetData();
}

Size DemoSliderRow::GetMinSize() const
{
    Size label_sz = label_.GetMinSize();
    Size slider_sz = slider_.GetMinSize();
    Size value_sz = show_value_ ? value_.GetMinSize() : Size(0, 0);

    if(layout_mode_ == DEMO_ROW_STACKED) {
        int top_h = max(label_sz.cy, value_sz.cy);
        int top_w = label_sz.cx + (show_value_ ? field_gap_ + max(value_width_, value_sz.cx) : 0);
        return Size(max(top_w, slider_sz.cx), top_h + stack_gap_ + slider_sz.cy);
    }

    int h = max(label_sz.cy, max(slider_sz.cy, value_sz.cy));
    int w = max(label_width_, label_sz.cx) + field_gap_ + slider_sz.cx;
    if(show_value_)
        w += field_gap_ + max(value_width_, value_sz.cx);
    return Size(w, h);
}

void DemoSliderRow::Layout()
{
    Rect r = GetSize();
    if(layout_mode_ == DEMO_ROW_STACKED) {
        int top_h = max(label_.GetMinSize().cy, show_value_ ? value_.GetMinSize().cy : 0);
        int top_y = 0;
        int slider_y = top_h + stack_gap_;
        int slider_h = max(0, r.bottom - slider_y);
        int vw = show_value_ ? value_width_ : 0;
        label_.SetRect(0, top_y, max(0, r.GetWidth() - (show_value_ ? vw + field_gap_ : 0)), top_h);
        if(show_value_)
            value_.SetRect(max(0, r.right - vw), top_y, vw, top_h);
        slider_.SetRect(0, slider_y, r.GetWidth(), slider_h);
        SyncValueEditRect();
        return;
    }

    int lw = label_width_;
    int vw = show_value_ ? value_width_ : 0;
    int slider_x = lw + field_gap_;
    int slider_w = max(0, r.GetWidth() - slider_x - (show_value_ ? (field_gap_ + vw) : 0));
    label_.SetRect(0, 0, lw, r.GetHeight());
    slider_.SetRect(slider_x, 0, slider_w, r.GetHeight());
    if(show_value_)
        value_.SetRect(max(0, r.right - vw), 0, vw, r.GetHeight());
    SyncValueEditRect();
}

void DemoSliderRow::SyncValueVisibility()
{
    value_.SetSelectable(value_selectable_);
    value_.Show(show_value_);
    if(!show_value_)
        value_edit_.Hide();
    else if(editing_value_)
        value_edit_.Show();
}

void DemoSliderRow::LeftDouble(Point p, dword keyflags)
{
    Ctrl::LeftDouble(p, keyflags);
    if(value_edit_enabled_ && show_value_ && value_.GetRect().Contains(p))
        BeginValueEdit();
}

void DemoSliderRow::BeginValueEdit()
{
    if(!show_value_ || editing_value_)
        return;
    editing_value_ = true;
    value_edit_.SetTextUtf8(AsString((int)slider_.GetValue()));
    SyncValueEditRect();
    value_.Hide();
    value_edit_.Show();
    value_edit_.SetFocus();
    value_edit_.SetSelection(0, INT_MAX);
}

void DemoSliderRow::CommitValueEdit()
{
    if(!editing_value_)
        return;
    int v = fround(slider_.GetValue());
    String text = value_edit_.GetTextUtf8();
    if(!text.IsEmpty()) {
        v = ScanInt(text);
        v = minmax(v, (int)slider_.GetMin(), (int)slider_.GetMax());
    }
    editing_value_ = false;
    value_edit_.Hide();
    value_.Show(show_value_);
    slider_.SetValue(v);
    SetValueText(AsString(v));
    WhenAction();
    Refresh();
}

void DemoSliderRow::CancelValueEdit()
{
    if(!editing_value_)
        return;
    editing_value_ = false;
    value_edit_.Hide();
    value_.Show(show_value_);
    SetValueText(AsString((int)slider_.GetValue()));
    Refresh();
}

void DemoSliderRow::SyncValueEditRect()
{
    if(show_value_)
        value_edit_.SetRect(value_.GetRect());
}

}

#include <Ui/UiTheme.h>

namespace Upp {

DemoToggleRow::DemoToggleRow()
{
    Add(label_);
    Add(toggle_);
    Add(value_);
    toggle_.WhenAction = [=] {
        toggle_.SetData(toggle_.GetData());
        WhenAction();
    };
    label_.NoWantFocus();
    value_.NoWantFocus();
    UiLabel::Style label_style = UiTheme::ResolveLabel(UiRole::Subtle);
    label_style.font = SansSerifZ(9);
    UiLabel::Style value_style = UiTheme::ResolveLabel(UiRole::Standard);
    value_style.font = SansSerifZ(9);
    label_.SetCustomStyle(label_style);
    value_.SetCustomStyle(value_style);
    SyncValueVisibility();
}

DemoToggleRow& DemoToggleRow::SetLayoutMode(DemoRowLayoutMode mode)
{
    if(layout_mode_ == mode)
        return *this;
    layout_mode_ = mode;
    RefreshLayout();
    Refresh();
    return *this;
}

DemoToggleRow& DemoToggleRow::SetLabel(const String& text)
{
    label_.SetText(text);
    RefreshLayout();
    Refresh();
    return *this;
}

DemoToggleRow& DemoToggleRow::SetValueText(const String& text)
{
    value_.SetText(text);
    Refresh();
    return *this;
}

DemoToggleRow& DemoToggleRow::ShowValue(bool show)
{
    if(show_value_ == show)
        return *this;
    show_value_ = show;
    SyncValueVisibility();
    RefreshLayout();
    Refresh();
    return *this;
}

DemoToggleRow& DemoToggleRow::SetValueSelectable(bool selectable)
{
    value_selectable_ = selectable;
    value_.SetSelectable(selectable);
    return *this;
}

DemoToggleRow& DemoToggleRow::SetLabelWidth(int cx)
{
    label_width_ = max(0, cx);
    RefreshLayout();
    return *this;
}

DemoToggleRow& DemoToggleRow::SetValueWidth(int cx)
{
    value_width_ = max(0, cx);
    RefreshLayout();
    return *this;
}

DemoToggleRow& DemoToggleRow::SetFieldGap(int px)
{
    field_gap_ = max(0, px);
    RefreshLayout();
    return *this;
}

DemoToggleRow& DemoToggleRow::SetStackGap(int px)
{
    stack_gap_ = max(0, px);
    RefreshLayout();
    return *this;
}

DemoToggleRow& DemoToggleRow::SetLabelStyle(const UiLabel::Style& style)
{
    label_.SetCustomStyle(style);
    Refresh();
    return *this;
}

DemoToggleRow& DemoToggleRow::SetValueStyle(const UiLabel::Style& style)
{
    value_.SetCustomStyle(style);
    Refresh();
    return *this;
}

void DemoToggleRow::SetData(const Value& v)
{
    toggle_.SetData(v);
    toggle_.Refresh();
    Refresh();
}

Value DemoToggleRow::GetData() const
{
    return toggle_.GetData();
}

Size DemoToggleRow::GetMinSize() const
{
    Size label_sz = label_.GetMinSize();
    Size toggle_sz = toggle_.GetMinSize();
    Size value_sz = show_value_ ? value_.GetMinSize() : Size(0, 0);

    if(layout_mode_ == DEMO_ROW_STACKED) {
        int top_h = max(label_sz.cy, value_sz.cy);
        int top_w = label_sz.cx + (show_value_ ? field_gap_ + max(value_width_, value_sz.cx) : 0);
        return Size(max(top_w, toggle_sz.cx), top_h + stack_gap_ + toggle_sz.cy);
    }

    int h = max(label_sz.cy, max(toggle_sz.cy, value_sz.cy));
    int w = max(label_width_, label_sz.cx) + field_gap_ + toggle_sz.cx;
    if(show_value_)
        w += field_gap_ + max(value_width_, value_sz.cx);
    return Size(w, h);
}

void DemoToggleRow::Layout()
{
    Rect r = GetSize();
    if(layout_mode_ == DEMO_ROW_STACKED) {
        int top_h = max(label_.GetMinSize().cy, show_value_ ? value_.GetMinSize().cy : 0);
        int toggle_y = top_h + stack_gap_;
        int vw = show_value_ ? value_width_ : 0;
        label_.SetRect(0, 0, max(0, r.GetWidth() - (show_value_ ? vw + field_gap_ : 0)), top_h);
        if(show_value_)
            value_.SetRect(max(0, r.right - vw), 0, vw, top_h);
        Size ts = toggle_.GetMinSize();
        int tx = max(0, (r.GetWidth() - ts.cx) / 2);
        toggle_.SetRect(tx, toggle_y, min(r.GetWidth(), ts.cx), ts.cy);
        return;
    }

    int lw = label_width_;
    int vw = show_value_ ? value_width_ : 0;
    int tx = r.right - (show_value_ ? (vw + field_gap_ + toggle_.GetMinSize().cx) : toggle_.GetMinSize().cx);
    label_.SetRect(0, 0, lw, r.GetHeight());
    toggle_.SetRect(max(lw + field_gap_, tx), (r.GetHeight() - toggle_.GetMinSize().cy) / 2, toggle_.GetMinSize().cx, toggle_.GetMinSize().cy);
    if(show_value_)
        value_.SetRect(max(0, r.right - vw), 0, vw, r.GetHeight());
}

void DemoToggleRow::SyncValueVisibility()
{
    value_.SetSelectable(value_selectable_);
    value_.Show(show_value_);
}

}

#include <Ui/UiColorPicker/UiColorPicker.h>
#include <Ui/UiTheme.h>

namespace Upp {

static String CompositeColorTipText(const String& label, Color color)
{
    return label.IsEmpty() ? "Color" : label;
}

DemoColorRowSwatch::DemoColorRowSwatch()
{
    NoWantFocus();
    Tip(CompositeColorTipText(label_, color_));
}

void DemoColorRowSwatch::SetColor(Color color)
{
    color_ = color;
    Tip(CompositeColorTipText(label_, color_));
    Refresh();
}

void DemoColorRowSwatch::SetRadius(int radius)
{
    radius_ = max(0, radius);
    Refresh();
}

void DemoColorRowSwatch::SetSlotLabel(const String& label)
{
    label_ = label;
    Tip(CompositeColorTipText(label_, color_));
    Refresh();
}

Size DemoColorRowSwatch::GetMinSize() const
{
    return Size(DPI(28), DPI(24));
}

void DemoColorRowSwatch::Paint(Draw& w)
{
    Rect r = GetSize();
    bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
    Color frame = hot_ ? (dark ? Color(96, 165, 250) : Color(44, 99, 212))
                       : (dark ? Color(76, 76, 76) : Color(211, 221, 237));
    Color back = hot_ ? (dark ? Color(44, 44, 44) : Blend(Color(236, 241, 248), White(), 22))
                      : (dark ? Color(32, 32, 32) : Color(236, 241, 248));
    StyledPalette pal;
    StyledMetrics m;
    pal.face[ST_NORMAL] = UiFill::Solid(back);
    pal.frame[ST_NORMAL] = frame;
    m.face_enabled = true;
    m.frame_enabled = true;
    m.frame_width = 1;
    m.radius = radius_;
    UiPaintFaceFrameDash(w, r, pal, m, ST_NORMAL);
    Rect sw = r.Deflated(DPI(4), DPI(4));
    StyledPalette sw_pal;
    StyledMetrics sw_m;
    sw_pal.face[ST_NORMAL] = UiFill::Solid(IsNull(color_) ? (dark ? Color(25, 25, 25) : White()) : color_);
    sw_pal.frame[ST_NORMAL] = dark ? Blend(frame, Black(), 64) : Blend(frame, White(), 96);
    sw_m.face_enabled = true;
    sw_m.frame_enabled = true;
    sw_m.frame_width = 1;
    sw_m.radius = max(0, radius_ - 2);
    UiPaintFaceFrameDash(w, sw, sw_pal, sw_m, ST_NORMAL);
}

void DemoColorRowSwatch::LeftDown(Point, dword)
{
    WhenAction();
}

void DemoColorRowSwatch::MouseEnter(Point, dword)
{
    hot_ = true;
    Refresh();
}

void DemoColorRowSwatch::MouseLeave()
{
    hot_ = false;
    Refresh();
}

DemoColorRow::DemoColorRow()
{
    Add(label_);
    Add(value_);
    label_.NoWantFocus();
    value_.NoWantFocus();
    UiLabel::Style label_style = UiTheme::ResolveLabel(UiRole::Subtle);
    label_style.font = SansSerifZ(9);
    UiLabel::Style value_style = UiTheme::ResolveLabel(UiRole::Standard);
    value_style.font = SansSerifZ(9);
    label_.SetCustomStyle(label_style);
    value_.SetCustomStyle(value_style);
    EnsureColorStorage(color_count_);
    SyncValueVisibility();
    SyncColorVisibility();
    BackPaint();
}

DemoColorRow& DemoColorRow::SetLayoutMode(DemoRowLayoutMode mode)
{
    if(layout_mode_ == mode)
        return *this;
    layout_mode_ = mode;
    RefreshLayout();
    Refresh();
    return *this;
}

DemoColorRow& DemoColorRow::SetLabel(const String& text)
{
    label_.SetText(text);
    RefreshLayout();
    Refresh();
    return *this;
}

DemoColorRow& DemoColorRow::SetValueText(const String& text)
{
    value_.SetText(text);
    Refresh();
    return *this;
}

DemoColorRow& DemoColorRow::ShowValue(bool show)
{
    if(show_value_ == show)
        return *this;
    show_value_ = show;
    SyncValueVisibility();
    RefreshLayout();
    Refresh();
    return *this;
}

DemoColorRow& DemoColorRow::SetValueSelectable(bool selectable)
{
    value_selectable_ = selectable;
    value_.SetSelectable(selectable);
    return *this;
}

DemoColorRow& DemoColorRow::SetLabelWidth(int cx)
{
    label_width_ = max(0, cx);
    RefreshLayout();
    return *this;
}

DemoColorRow& DemoColorRow::SetValueWidth(int cx)
{
    value_width_ = max(0, cx);
    RefreshLayout();
    return *this;
}

DemoColorRow& DemoColorRow::SetFieldGap(int px)
{
    field_gap_ = max(0, px);
    RefreshLayout();
    return *this;
}

DemoColorRow& DemoColorRow::SetStackGap(int px)
{
    stack_gap_ = max(0, px);
    RefreshLayout();
    return *this;
}

DemoColorRow& DemoColorRow::SetColorCount(int count)
{
    color_count_ = max(1, count);
    EnsureColorStorage(color_count_);
    SyncColorVisibility();
    RefreshLayout();
    Refresh();
    return *this;
}

DemoColorRow& DemoColorRow::SetLabelStyle(const UiLabel::Style& style)
{
    label_.SetCustomStyle(style);
    Refresh();
    return *this;
}

DemoColorRow& DemoColorRow::SetValueStyle(const UiLabel::Style& style)
{
    value_.SetCustomStyle(style);
    Refresh();
    return *this;
}

DemoColorRow& DemoColorRow::SetColor(int index, Color color)
{
    if(index < 0)
        return *this;
    EnsureColorStorage(index + 1);
    if(index >= color_count_)
        color_count_ = index + 1;
    color_[index].SetColor(color);
    SyncColorVisibility();
    Refresh();
    return *this;
}

Color DemoColorRow::GetColor(int index) const
{
    return (index >= 0 && index < color_.GetCount()) ? color_[index].GetColor() : Null;
}

DemoColorRow& DemoColorRow::SetColors(const Vector<Color>& colors)
{
    int count = colors.GetCount();
    SetColorCount(max(1, count));
    for(int i = 0; i < count; i++)
        color_[i].SetColor(colors[i]);
    Refresh();
    return *this;
}

Vector<Color> DemoColorRow::GetColors() const
{
    Vector<Color> out;
    out.SetCount(color_count_);
    for(int i = 0; i < color_count_; i++)
        out[i] = color_[i].GetColor();
    return out;
}

DemoColorRow& DemoColorRow::SetColorLabel(int index, const String& label)
{
    if(index < 0)
        return *this;
    EnsureColorStorage(index + 1);
    color_[index].SetSlotLabel(label);
    Refresh();
    return *this;
}

String DemoColorRow::GetColorLabel(int index) const
{
    return (index >= 0 && index < color_.GetCount()) ? color_[index].GetSlotLabel() : String();
}

DemoColorRow& DemoColorRow::SetSeparatorBefore(int index, bool on)
{
    if(index <= 0)
        return *this;
    EnsureColorStorage(index + 1);
    separator_before_[index] = on;
    RefreshLayout();
    Refresh();
    return *this;
}

bool DemoColorRow::HasSeparatorBefore(int index) const
{
    return index > 0 && index < separator_before_.GetCount() && separator_before_[index];
}

Size DemoColorRow::GetMinSize() const
{
    int sw_w = 0;
    int sw_h = 0;
    for(int i = 0; i < color_count_; i++) {
        Size sz = color_[i].GetMinSize();
        if(i)
            sw_w += field_gap_;
        if(i > 0 && separator_before_[i])
            sw_w += DPI(7);
        sw_w += sz.cx;
        sw_h = max(sw_h, sz.cy);
    }
    Size label_sz = label_.GetMinSize();
    Size value_sz = show_value_ ? value_.GetMinSize() : Size(0, 0);

    if(layout_mode_ == DEMO_ROW_STACKED) {
        int top_h = max(label_sz.cy, value_sz.cy);
        int top_w = label_sz.cx + (show_value_ ? field_gap_ + max(value_width_, value_sz.cx) : 0);
        return Size(max(top_w, sw_w), top_h + stack_gap_ + sw_h);
    }

    int h = max(label_sz.cy, max(sw_h, value_sz.cy));
    int w = max(label_width_, label_sz.cx) + field_gap_ + sw_w;
    if(show_value_)
        w += field_gap_ + max(value_width_, value_sz.cx);
    return Size(w, h);
}

void DemoColorRow::Paint(Draw& w)
{
    Color c = SColorShadow();
    for(int i = 1; i < color_count_; i++) {
        if(!separator_before_[i])
            continue;
        Rect sr = color_[i].GetRect();
        int x = sr.left - field_gap_ / 2 - DPI(3);
        w.DrawRect(x, sr.top + DPI(3), 1, max(1, sr.GetHeight() - DPI(6)), c);
    }
}

void DemoColorRow::Layout()
{
    int sw_w = 0;
    for(int i = 0; i < color_count_; i++) {
        if(i)
            sw_w += field_gap_;
        if(i > 0 && separator_before_[i])
            sw_w += DPI(7);
        sw_w += color_[i].GetMinSize().cx;
    }

    Rect r = GetSize();
    if(layout_mode_ == DEMO_ROW_STACKED) {
        int top_h = max(label_.GetMinSize().cy, show_value_ ? value_.GetMinSize().cy : 0);
        int sw_y = top_h + stack_gap_;
        int x = 0;
        label_.SetRect(0, 0, max(0, r.GetWidth() - (show_value_ ? value_width_ + field_gap_ : 0)), top_h);
        if(show_value_)
            value_.SetRect(max(0, r.right - value_width_), 0, value_width_, top_h);
        for(int i = 0; i < color_count_; i++) {
            if(i > 0 && separator_before_[i])
                x += DPI(7);
            Size sz = color_[i].GetMinSize();
            color_[i].SetRect(x, sw_y, sz.cx, sz.cy);
            x += sz.cx + field_gap_;
        }
        return;
    }

    int lw = label_width_;
    int vw = show_value_ ? value_width_ : 0;
    int sw_x = lw + field_gap_;
    int sw_y = (r.GetHeight() - color_[0].GetMinSize().cy) / 2;
    label_.SetRect(0, 0, lw, r.GetHeight());
    int x = sw_x;
    int max_right = show_value_ ? max(x, r.right - vw - field_gap_) : r.right;
    int available = max(0, max_right - x);
    int sep_extra = 0;
    for(int i = 1; i < color_count_; i++)
        if(separator_before_[i])
            sep_extra += DPI(7);
    int slot_gap_total = max(0, color_count_ - 1) * field_gap_ + sep_extra;
    int slot_w = color_count_ > 0 ? max(DPI(20), min(DPI(44), (available - slot_gap_total) / color_count_)) : DPI(28);
    for(int i = 0; i < color_count_; i++) {
        if(i > 0 && separator_before_[i])
            x += DPI(7);
        Size sz = color_[i].GetMinSize();
        color_[i].SetRect(x, sw_y, slot_w, sz.cy);
        x += slot_w + field_gap_;
    }
    if(show_value_)
        value_.SetRect(max(0, r.right - vw), 0, vw, r.GetHeight());
}

void DemoColorRow::SyncValueVisibility()
{
    value_.SetSelectable(value_selectable_);
    value_.Show(show_value_);
}

void DemoColorRow::SyncColorVisibility()
{
    for(int i = 0; i < color_.GetCount(); i++)
        color_[i].Show(i < color_count_);
}

void DemoColorRow::EnsureColorStorage(int count)
{
    count = max(1, count);
    while(color_.GetCount() < count) {
        int ii = color_.GetCount();
        DemoColorRowSwatch& swatch = color_.Add();
        Add(swatch);
        swatch.WhenAction = [=] { OpenColorPicker(ii); };
        separator_before_.Add(false);
    }
}

DemoColorRowSwatch& DemoColorRow::ColorCtrl(int index)
{
    EnsureColorStorage(index + 1);
    return color_[index];
}

void DemoColorRow::OpenColorPicker(int active)
{
    if(active < 0 || active >= color_count_)
        return;

    TopWindow dlg;
    dlg.Title("Color");
    dlg.Sizeable().Zoomable();
    UiColorPicker picker;
    int slot_start = (active / 4) * 4;
    int picker_count = min(4, color_count_ - slot_start);
    picker.SetSlotCount(picker_count);
    picker.SetActiveSlot(active - slot_start);
    picker.SetAlphaEnabled(true);
    for(int i = 0; i < picker_count; i++) {
        int ci = slot_start + i;
        Color c = color_[ci].GetColor();
        picker.SetSlotColor(i, IsNull(c) ? White() : c, false);
        picker.SetSlotLabel(i, color_[ci].GetSlotLabel());
    }
    picker.WhenAccept = [&] {
        for(int i = 0; i < picker_count; i++)
            color_[slot_start + i].SetColor(picker.GetSlotColor(i));
        Refresh();
        WhenAction();
        dlg.Break(IDOK);
    };
    picker.WhenCancel = [&] { dlg.Break(IDCANCEL); };
    dlg.Add(picker.SizePos());
    dlg.SetRect(GetWorkArea().CenterRect(Size(DPI(760), DPI(550))));
    dlg.RunAppModal();
}

}


namespace Upp {

DemoDropdownRow::DemoDropdownRow()
{
    Ctrl::Add(label_);
    Ctrl::Add(drop_);
    label_.NoWantFocus();
    drop_.WhenSelect = [=](int i) { WhenSelect(i); };
    drop_.WhenSelectData = [=](const Value& v) { WhenSelectData(v); };
    drop_.WhenOpen = [=] { WhenOpen(); };
    drop_.WhenClose = [=] { WhenClose(); };
    SyncThemeStyle();
}

void DemoDropdownRow::SyncThemeStyle()
{
    uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;
    theme_revision_ = revision;

    if(!custom_label_style_) {
        UiLabel::Style label_style = UiTheme::ResolveLabel(label_role_);
        label_style.font = SansSerifZ(9);
        label_.SetCustomStyle(label_style);
    }
    drop_.SetRole(dropdown_role_);
    RefreshLayout();
    Refresh();
}

DemoDropdownRow& DemoDropdownRow::SetLayoutMode(DemoRowLayoutMode mode)
{
    if(layout_mode_ == mode)
        return *this;
    layout_mode_ = mode;
    RefreshLayout();
    Refresh();
    return *this;
}

DemoDropdownRow& DemoDropdownRow::SetLabel(const String& text)
{
    label_.SetText(text);
    RefreshLayout();
    Refresh();
    return *this;
}

DemoDropdownRow& DemoDropdownRow::SetLabelWidth(int cx)
{
    label_width_ = max(0, cx);
    RefreshLayout();
    return *this;
}

DemoDropdownRow& DemoDropdownRow::SetFieldGap(int px)
{
    field_gap_ = max(0, px);
    RefreshLayout();
    return *this;
}

DemoDropdownRow& DemoDropdownRow::SetStackGap(int px)
{
    stack_gap_ = max(0, px);
    RefreshLayout();
    return *this;
}

DemoDropdownRow& DemoDropdownRow::SetLabelRole(UiRole role)
{
    if(!UiIsValid(role))
        role = UiRole::Subtle;
    label_role_ = role;
    custom_label_style_ = false;
    theme_revision_ = 0;
    SyncThemeStyle();
    return *this;
}

DemoDropdownRow& DemoDropdownRow::SetDropdownRole(UiRole role)
{
    if(!UiIsValid(role))
        role = UiRole::Accent;
    dropdown_role_ = role;
    theme_revision_ = 0;
    SyncThemeStyle();
    return *this;
}

DemoDropdownRow& DemoDropdownRow::SetLabelStyle(const UiLabel::Style& style)
{
    custom_label_style_ = true;
    label_.SetCustomStyle(style);
    Refresh();
    return *this;
}

DemoDropdownRow& DemoDropdownRow::Add(const String& text, const Value& data, bool enabled)
{
    drop_.Add(text, data, enabled);
    RefreshLayout();
    return *this;
}

DemoDropdownRow& DemoDropdownRow::Clear()
{
    drop_.Clear();
    RefreshLayout();
    return *this;
}

DemoDropdownRow& DemoDropdownRow::Select(int index)
{
    drop_.Select(index);
    return *this;
}

DemoDropdownRow& DemoDropdownRow::SelectByData(const Value& data)
{
    drop_.SelectByData(data);
    return *this;
}

void DemoDropdownRow::SetData(const Value& v)
{
    drop_.SetDataSilently(v);
    drop_.Refresh();
    Refresh();
}

Value DemoDropdownRow::GetData() const
{
    return drop_.GetData();
}

Size DemoDropdownRow::GetMinSize() const
{
    const_cast<DemoDropdownRow *>(this)->SyncThemeStyle();
    Size label_sz = label_.GetMinSize();
    Size drop_sz = drop_.GetMinSize();
    if(layout_mode_ == DEMO_ROW_STACKED)
        return Size(max(label_sz.cx, drop_sz.cx), label_sz.cy + stack_gap_ + drop_sz.cy);
    return Size(max(label_width_, label_sz.cx) + field_gap_ + drop_sz.cx, max(label_sz.cy, drop_sz.cy));
}

void DemoDropdownRow::Layout()
{
    SyncThemeStyle();
    Rect r = GetSize();
    if(layout_mode_ == DEMO_ROW_STACKED) {
        int label_h = label_.GetMinSize().cy;
        label_.SetRect(0, 0, r.GetWidth(), label_h);
        drop_.SetRect(0, label_h + stack_gap_, r.GetWidth(), max(0, r.GetHeight() - label_h - stack_gap_));
        return;
    }

    int lw = min(label_width_, r.GetWidth());
    label_.SetRect(0, 0, lw, r.GetHeight());
    drop_.SetRect(lw + field_gap_, 0, max(0, r.GetWidth() - lw - field_gap_), r.GetHeight());
}

void DemoDropdownRow::Paint(Draw& w)
{
    SyncThemeStyle();
}

}


namespace Upp {

DemoLabelRow::DemoLabelRow()
{
    BackPaint();
    Ctrl::Add(label_);
    Ctrl::Add(value_);
    label_.NoWantFocus();
    value_.NoWantFocus();
    SyncThemeStyle();
}

void DemoLabelRow::SyncThemeStyle()
{
    uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;
    theme_revision_ = revision;

    if(!custom_label_style_) {
        UiLabel::Style label_style = UiTheme::ResolveLabel(label_role_);
        label_style.font = SansSerifZ(9);
        label_.SetCustomStyle(label_style);
    }
    if(!custom_value_style_) {
        UiLabel::Style value_style = UiTheme::ResolveLabel(value_role_);
        value_style.font = SansSerifZ(9);
        value_.SetCustomStyle(value_style);
    }
    RefreshLayout();
    Refresh();
}

DemoLabelRow& DemoLabelRow::SetLabel(const String& text)
{
    label_.SetText(text);
    RefreshLayout();
    Refresh();
    return *this;
}

DemoLabelRow& DemoLabelRow::SetValueText(const String& text)
{
    value_.SetText(text);
    value_.RefreshLayout();
    RefreshLayout();
    Refresh();
    return *this;
}

DemoLabelRow& DemoLabelRow::SetLabelWidth(int cx)
{
    label_width_ = max(0, cx);
    RefreshLayout();
    return *this;
}

DemoLabelRow& DemoLabelRow::SetFieldGap(int px)
{
    field_gap_ = max(0, px);
    RefreshLayout();
    return *this;
}

DemoLabelRow& DemoLabelRow::SetLabelRole(UiRole role)
{
    if(!UiIsValid(role))
        role = UiRole::Subtle;
    label_role_ = role;
    custom_label_style_ = false;
    theme_revision_ = 0;
    SyncThemeStyle();
    return *this;
}

DemoLabelRow& DemoLabelRow::SetValueRole(UiRole role)
{
    if(!UiIsValid(role))
        role = UiRole::Accent;
    value_role_ = role;
    custom_value_style_ = false;
    theme_revision_ = 0;
    SyncThemeStyle();
    return *this;
}

DemoLabelRow& DemoLabelRow::SetLabelStyle(const UiLabel::Style& style)
{
    custom_label_style_ = true;
    label_.SetCustomStyle(style);
    Refresh();
    return *this;
}

DemoLabelRow& DemoLabelRow::SetValueStyle(const UiLabel::Style& style)
{
    custom_value_style_ = true;
    value_.SetCustomStyle(style);
    Refresh();
    return *this;
}

void DemoLabelRow::SetData(const Value& v)
{
    SetValueText(AsString(v));
}

Value DemoLabelRow::GetData() const
{
    return value_.GetText();
}

Size DemoLabelRow::GetMinSize() const
{
    const_cast<DemoLabelRow *>(this)->SyncThemeStyle();
    Size label_sz = label_.GetMinSize();
    Size value_sz = value_.GetMinSize();
    return Size(max(label_width_, label_sz.cx) + field_gap_ + value_sz.cx, max(label_sz.cy, value_sz.cy));
}

void DemoLabelRow::Layout()
{
    SyncThemeStyle();
    Rect r = GetSize();
    int lw = min(label_width_, r.GetWidth());
    label_.SetRect(0, 0, lw, r.GetHeight());
    value_.SetRect(lw + field_gap_, 0, max(0, r.GetWidth() - lw - field_gap_), r.GetHeight());
}

void DemoLabelRow::Paint(Draw& w)
{
    SyncThemeStyle();
}

}


namespace Upp {

DemoEditRow::DemoEditRow()
{
    BackPaint();
    Ctrl::Add(label_);
    Ctrl::Add(edit_);
    label_.NoWantFocus();
    edit_.WhenAction = [=] { WhenAction(); };
    edit_.WhenChange = [=] { WhenChange(); };
    edit_.WhenLoseFocus = [=] { WhenLoseFocus(); };
    SyncThemeStyle();
}

void DemoEditRow::SyncThemeStyle()
{
    uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;
    theme_revision_ = revision;

    if(!custom_label_style_) {
        UiLabel::Style label_style = UiTheme::ResolveLabel(label_role_);
        label_style.font = SansSerifZ(9);
        label_.SetCustomStyle(label_style);
    }
    if(!custom_edit_style_)
        edit_.SetCustomStyle(UiTheme::ResolveEdit(edit_role_));
    RefreshLayout();
    Refresh();
}

DemoEditRow& DemoEditRow::SetLayoutMode(DemoRowLayoutMode mode)
{
    if(layout_mode_ == mode)
        return *this;
    layout_mode_ = mode;
    RefreshLayout();
    Refresh();
    return *this;
}

DemoEditRow& DemoEditRow::SetLabel(const String& text)
{
    label_.SetText(text);
    RefreshLayout();
    Refresh();
    return *this;
}

DemoEditRow& DemoEditRow::SetLabelWidth(int cx)
{
    label_width_ = max(0, cx);
    RefreshLayout();
    return *this;
}

DemoEditRow& DemoEditRow::SetFieldGap(int px)
{
    field_gap_ = max(0, px);
    RefreshLayout();
    return *this;
}

DemoEditRow& DemoEditRow::SetStackGap(int px)
{
    stack_gap_ = max(0, px);
    RefreshLayout();
    return *this;
}

DemoEditRow& DemoEditRow::SetLabelRole(UiRole role)
{
    if(!UiIsValid(role))
        role = UiRole::Subtle;
    label_role_ = role;
    custom_label_style_ = false;
    theme_revision_ = 0;
    SyncThemeStyle();
    return *this;
}

DemoEditRow& DemoEditRow::SetEditRole(UiRole role)
{
    if(!UiIsValid(role))
        role = UiRole::Standard;
    edit_role_ = role;
    custom_edit_style_ = false;
    theme_revision_ = 0;
    SyncThemeStyle();
    return *this;
}

DemoEditRow& DemoEditRow::SetLabelStyle(const UiLabel::Style& style)
{
    custom_label_style_ = true;
    label_.SetCustomStyle(style);
    Refresh();
    return *this;
}

DemoEditRow& DemoEditRow::SetEditStyle(const UiBaseEdit::Style& style)
{
    custom_edit_style_ = true;
    edit_.SetCustomStyle(style);
    RefreshLayout();
    Refresh();
    return *this;
}

void DemoEditRow::SetData(const Value& v)
{
    edit_.SetData(v);
}

Value DemoEditRow::GetData() const
{
    return edit_.GetData();
}

Size DemoEditRow::GetMinSize() const
{
    const_cast<DemoEditRow *>(this)->SyncThemeStyle();
    Size label_sz = label_.GetMinSize();
    Size edit_sz = edit_.GetMinSize();
    if(layout_mode_ == DEMO_ROW_STACKED)
        return Size(max(label_sz.cx, edit_sz.cx), label_sz.cy + stack_gap_ + edit_sz.cy);
    return Size(max(label_width_, label_sz.cx) + field_gap_ + edit_sz.cx, max(label_sz.cy, edit_sz.cy));
}

void DemoEditRow::Layout()
{
    SyncThemeStyle();
    Rect r = GetSize();
    if(layout_mode_ == DEMO_ROW_STACKED) {
        Size label_sz = label_.GetMinSize();
        int label_h = min(label_sz.cy, r.GetHeight());
        label_.SetRect(0, 0, r.GetWidth(), label_h);
        edit_.SetRect(0, label_h + stack_gap_, r.GetWidth(), max(0, r.GetHeight() - label_h - stack_gap_));
        return;
    }

    int lw = min(label_width_, r.GetWidth());
    label_.SetRect(0, 0, lw, r.GetHeight());
    edit_.SetRect(lw + field_gap_, 0, max(0, r.GetWidth() - lw - field_gap_), r.GetHeight());
}

void DemoEditRow::Paint(Draw& w)
{
    SyncThemeStyle();
}

}

#endif
