#include "PropertyValueEditors.h"

#include <cmath>
#include <Ui/UiColorPicker/UiColorPicker.h>
#include <Ui/UiIcons.h>
#include <Ui/UiOsFileDialog/UiOsFileDialog.h>
#include <Ui/UiSliderEdit.h>
#include <Ui/UiTheme.h>
#include <Ui/UiToolButton.h>

namespace Upp {

void PropertyActionLabel::LeftDown(Point, dword)
{
    if(IsEnabled()) {
        SetFocus();
        WhenAction();
    }
}

bool PropertyActionLabel::Key(dword key, int count)
{
    if(IsEnabled() && (key == K_ENTER || key == K_SPACE)) {
        WhenAction();
        return true;
    }
    return UiLabel::Key(key, count);
}

void PropertyValueEditor::FocusEditor()
{
    SetFocus();
}

static void ConfigurePropertyAction(UiToolButton& button, const Image& icon,
                                    const char *tip)
{
    button.SetIcon(icon)
          .SetIconSize(DPI(16), DPI(16))
          .SetIconRenderMode(UiIconRenderMode::MonoTint)
          .SetContentInset(DPI(3))
          .SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
    button.Tip(tip);
}

template <class T>
class PropertyCommitEditCtrl : public T {
public:
    typedef PropertyCommitEditCtrl CLASSNAME;

    PropertyCommitEditCtrl()
    {
        T::WhenAction = [=] { EmitCommit(); };
    }

    virtual void LostFocus() override
    {
        T::LostFocus();
        EmitCommit();
    }

    Event<> WhenCommit;

    void SetCommitBaseline(const Value& value)
    {
        last_commit_ = value;
        has_last_commit_ = true;
    }

private:
    void EmitCommit()
    {
        if(!CanCommit())
            return;
        Value v = T::GetData();
        if(!has_last_commit_ || v != last_commit_) {
            last_commit_ = v;
            has_last_commit_ = true;
            WhenCommit();
        }
    }

protected:
    virtual bool CanCommit() const { return true; }

private:
    Value last_commit_;
    bool has_last_commit_ = false;
};

using PropertyCommitEdit = PropertyCommitEditCtrl<UiLineEdit>;
using PropertyCommitMultiEdit = PropertyCommitEditCtrl<UiMultiEdit>;
using PropertyCommitIntEdit = PropertyCommitEditCtrl<UiIntEdit>;
class PropertyCommitFloatEdit : public PropertyCommitEditCtrl<UiFloatEdit> {
protected:
    bool CanCommit() const override
    {
        return GetText().IsEmpty() || IsInputComplete();
    }
};

class PropertyTextValueEditor : public PropertyValueEditor {
public:
    PropertyTextValueEditor()
    {
        Add(edit_.SizePos());
        edit_.WhenChange = [=] {
            if(!syncing_)
                WhenPreview(edit_.GetData());
        };
        edit_.WhenCommit = [=] {
            if(!syncing_)
                WhenCommit(edit_.GetData());
        };
    }

    virtual void Configure(const PropertyEditorItem& item) override
    {
        edit_.Enable(item.enabled && !item.read_only);
        edit_.SetPlaceholder(item.mixed ? "<multiple values>" :
                             item.inherited ? "<inherited>" : "");
        Enable(item.enabled && !item.read_only);
    }

    virtual void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        edit_.SetData(mixed ? Value(String()) : Value(AsString(value)));
        edit_.SetCommitBaseline(edit_.GetData());
        syncing_ = false;
    }

    virtual Value GetEditorValue() const override
    {
        return edit_.GetData();
    }

    virtual void FocusEditor() override
    {
        edit_.SetFocus();
        edit_.SetSelection();
    }

private:
    PropertyCommitEdit edit_;
    bool syncing_ = false;
};

class PropertyMultilineValueEditor : public PropertyValueEditor {
public:
    PropertyMultilineValueEditor()
    {
        Add(summary_);
        Add(expand_);
        Add(dialog_);
        Add(edit_);
        summary_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        summary_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
        ConfigurePropertyAction(expand_, ICON_DESIGN_UNFOLD_MORE_48(),
                                "Expand or collapse multiline editor");
        ConfigurePropertyAction(dialog_, ICON_DESIGN_BOTTOM_PANEL_OPEN_48(),
                                "Open multiline editor dialog");
        expand_.WhenAction = [=] { WhenToggleExpanded(); };
        summary_.WhenAction = [=] {
            if(!expanded_)
                WhenToggleExpanded();
        };
        dialog_.WhenAction = [=] { OpenDialog(); };
        edit_.WhenChange = [=] {
            if(!syncing_) {
                SyncSummary();
                WhenPreview(edit_.GetData());
            }
        };
        edit_.WhenCommit = [=] {
            if(!syncing_) {
                SyncSummary();
                WhenCommit(edit_.GetData());
            }
        };
        edit_.Hide();
    }

    virtual void Configure(const PropertyEditorItem& item) override
    {
        enabled_ = item.enabled && !item.read_only;
        edit_.Enable(enabled_);
        dialog_.Enable(enabled_);
        expand_.Enable(item.expanded_row_span > 1 && enabled_);
        Enable(enabled_);
    }

    virtual void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        edit_.SetData(mixed ? Value(String()) : Value(AsString(value)));
        edit_.SetCommitBaseline(edit_.GetData());
        syncing_ = false;
        mixed_ = mixed;
        SyncSummary();
    }

    virtual Value GetEditorValue() const override
    {
        return edit_.GetData();
    }

    virtual void FocusEditor() override
    {
        expanded_ ? edit_.SetFocus() : dialog_.SetFocus();
    }

    void SetExpanded(bool expanded) override
    {
        expanded_ = expanded;
        ActionIconsChanged();
        edit_.Show(expanded_);
        Layout();
    }

    void Layout() override
    {
        const int row = min(DPI(28), GetSize().cy);
        const int action = min(DPI(28), row);
        if(expanded_) {
            const int rail = min(action, GetSize().cx);
            const int rail_x = max(0, GetSize().cx - rail);
            summary_.Hide();
            edit_.SetRect(0, 0, max(0, rail_x - DPI(3)), GetSize().cy);
            expand_.SetRect(rail_x, 0, rail, min(action, GetSize().cy));
            dialog_.SetRect(rail_x, min(action + DPI(2), GetSize().cy), rail,
                            min(action, max(0, GetSize().cy - action - DPI(2))));
        }
        else {
            const int dialog_x = max(0, GetSize().cx - action);
            const int expand_x = max(0, dialog_x - action - DPI(2));
            summary_.Show();
            summary_.SetRect(0, 0, max(0, expand_x - DPI(4)), row);
            expand_.SetRect(expand_x, 0, action, row);
            dialog_.SetRect(dialog_x, 0, action, row);
            edit_.SetRect(0, 0, 0, 0);
        }
    }

private:
    void ActionIconsChanged() override
    {
        const Image icon = expanded_ ? action_icons_.collapse : action_icons_.expand;
        if(!icon.IsEmpty())
            expand_.SetIcon(icon);
        expand_.SetIconSize(action_icons_.size, action_icons_.size);
        if(!action_icons_.dialog.IsEmpty())
            dialog_.SetIcon(action_icons_.dialog);
        dialog_.SetIconSize(action_icons_.size, action_icons_.size);
    }

    void SyncSummary()
    {
        if(mixed_) {
            summary_.SetText("<multiple values>");
            return;
        }
        String text = AsString(edit_.GetData());
        text.Replace("\r", "");
        int nl = text.Find('\n');
        if(nl >= 0)
            text = text.Left(nl) + "...";
        summary_.SetText(text.IsEmpty() ? "<empty>" : text);
    }

    void OpenDialog()
    {
        if(!enabled_)
            return;
        class MultiDialog : public TopWindow {
        public:
            UiMultiEdit edit;
            UiButton ok, cancel;
            MultiDialog()
            {
                Title("Multiline editor"); Sizeable().Zoomable();
                SetRect(0, 0, DPI(560), DPI(380));
                Add(edit); Add(ok); Add(cancel);
                ok.SetText("OK"); cancel.SetText("Cancel");
                ok.WhenAction = [=] { AcceptBreak(IDOK); };
                cancel.WhenAction = [=] { RejectBreak(IDCANCEL); };
            }
            void Layout() override
            {
                Rect r = GetSize();
                const int pad = DPI(10), h = DPI(30), gap = DPI(6), w = DPI(82);
                edit.SetRect(pad, pad, max(0, r.GetWidth() - 2 * pad),
                             max(0, r.GetHeight() - 3 * pad - h));
                cancel.SetRect(r.right - pad - w, r.bottom - pad - h, w, h);
                ok.SetRect(r.right - pad - 2 * w - gap, r.bottom - pad - h, w, h);
            }
        } dlg;
        dlg.edit.SetData(edit_.GetData());
        dlg.CenterOwner();
        if(dlg.Run() == IDOK) {
            syncing_ = true;
            edit_.SetData(dlg.edit.GetData());
            edit_.SetCommitBaseline(edit_.GetData());
            syncing_ = false;
            mixed_ = false;
            SyncSummary();
            WhenPreview(edit_.GetData());
            WhenCommit(edit_.GetData());
        }
    }

    PropertyCommitMultiEdit edit_;
    PropertyActionLabel summary_;
    UiToolButton expand_, dialog_;
    bool expanded_ = false;
    bool enabled_ = true;
    bool mixed_ = false;
    bool syncing_ = false;
};

class PropertyIntegerValueEditor : public PropertyValueEditor {
public:
    PropertyIntegerValueEditor()
    {
        Add(edit_.SizePos());
        edit_.SetTextAlign(UiAlign::RIGHT);
        edit_.WhenChange = [=] {
            if(!syncing_)
                WhenPreview(edit_.GetData());
        };
        edit_.WhenCommit = [=] {
            if(!syncing_)
                WhenCommit(edit_.GetData());
        };
    }

    virtual void Configure(const PropertyEditorItem& item) override
    {
        edit_.Enable(item.enabled && !item.read_only);
        edit_.SetPlaceholder(item.mixed ? "<mixed>" :
                             item.inherited ? "<inherited>" : "");
        edit_.Min(IsNumber(item.minimum) ? (int)item.minimum : INT_MIN);
        edit_.Max(IsNumber(item.maximum) ? (int)item.maximum : INT_MAX);
        if(IsNumber(item.step))
            edit_.Step(max(1, (int)item.step));
        Enable(item.enabled && !item.read_only);
    }

    virtual void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        if(mixed || IsNull(value))
            edit_.SetData(String());
        else
            edit_.SetValue((int)value);
        edit_.SetCommitBaseline(edit_.GetData());
        syncing_ = false;
    }

    virtual Value GetEditorValue() const override { return edit_.GetData(); }
    virtual void FocusEditor() override
    {
        edit_.SetFocus();
        edit_.SetSelection();
    }

private:
    PropertyCommitIntEdit edit_;
    bool syncing_ = false;
};

class PropertyNumericIntValueEditor : public PropertyValueEditor {
public:
    PropertyNumericIntValueEditor()
    {
        Add(edit_);
        Add(slider_);
        Add(toggle_);
        slider_.SetCustomStyle(UiTheme::ResolveSlider());
        slider_.ExpandTrack();
        toggle_.SetText("")
               .SetIcon(ICON_DESIGN_SLIDERS_48())
               .SetIconSize(DPI(16), DPI(16))
               .SetIconRenderMode(UiIconRenderMode::MonoTint)
               .SetContentInset(DPI(1));
        toggle_.Tip("Switch between numeric entry and slider");
        toggle_.SetCustomStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));
        toggle_.WhenAction = [=] {
            slider_mode_ = !slider_mode_;
            UpdateVisible();
            FocusEditor();
        };
        slider_.WhenChanging = [=] {
            if(syncing_) return;
            syncing_ = true;
            edit_.SetValue((int)slider_.GetValue());
            syncing_ = false;
            WhenPreview(edit_.GetData());
        };
        slider_.WhenAction = [=] {
            if(!syncing_)
                WhenCommit(edit_.GetData());
        };
        edit_.SetTextAlign(UiAlign::RIGHT);
        edit_.WhenChange = [=] {
            if(!syncing_)
                WhenPreview(edit_.GetData());
        };
        edit_.WhenCommit = [=] {
            if(!syncing_)
                WhenCommit(edit_.GetData());
        };
    }

    virtual void Configure(const PropertyEditorItem& item) override
    {
        enabled_ = item.enabled && !item.read_only;
        edit_.SetPlaceholder(item.mixed ? "<mixed>" :
                             item.inherited ? "<inherited>" : "");
        minimum_ = IsNumber(item.minimum) ? (int)item.minimum : INT_MIN;
        maximum_ = IsNumber(item.maximum) ? (int)item.maximum : INT_MAX;
        step_ = IsNumber(item.step) ? max(1, (int)item.step) : 1;
        bounded_ = IsNumber(item.minimum) && IsNumber(item.maximum) && maximum_ > minimum_;
        edit_.Min(minimum_);
        edit_.Max(maximum_);
        edit_.Step(step_);
        slider_.SetRange(minimum_, maximum_);
        slider_.SetStep(step_);
        edit_.Enable(enabled_);
        slider_.Enable(enabled_);
        toggle_.Show(item.show_slider_toggle && bounded_);
        toggle_.Enable(enabled_);
        slider_mode_ = slider_mode_ && bounded_;
        UpdateVisible();
    }

    virtual void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        if(mixed || IsNull(value)) {
            edit_.SetData(String());
            slider_.SetValue(bounded_ ? minimum_ : 0);
        }
        else {
            edit_.SetValue((int)value);
            slider_.SetValue((int)value);
        }
        edit_.SetCommitBaseline(edit_.GetData());
        syncing_ = false;
    }

    virtual Value GetEditorValue() const override
    {
        return edit_.GetData();
    }

    virtual void FocusEditor() override
    {
        if(slider_mode_)
            slider_.SetFocus();
        else {
            edit_.SetFocus();
            edit_.SetSelection();
        }
    }

    virtual void Layout() override
    {
        const int toggle_width = toggle_.IsShown() ? DPI(30) : 0;
        toggle_.SetRect(max(0, GetSize().cx - toggle_width), 0, toggle_width, GetSize().cy);
        const int width = max(0, GetSize().cx - toggle_width - DPI(4));
        edit_.SetRect(0, 0, width, GetSize().cy);
        slider_.SetRect(0, 0, width, GetSize().cy);
    }

private:
    void ActionIconsChanged() override
    {
        if(!action_icons_.numeric_slider.IsEmpty())
            toggle_.SetIcon(action_icons_.numeric_slider);
        toggle_.SetIconSize(action_icons_.size, action_icons_.size);
    }

    void UpdateVisible()
    {
        edit_.Show(!slider_mode_);
        slider_.Show(slider_mode_ && bounded_);
        Layout();
    }

    PropertyCommitIntEdit edit_;
    UiSlider slider_;
    UiButton toggle_;
    bool syncing_ = false;
    bool enabled_ = true;
    bool bounded_ = false;
    bool slider_mode_ = false;
    int minimum_ = INT_MIN;
    int maximum_ = INT_MAX;
    int step_ = 1;
};

class PropertyDoubleValueEditor : public PropertyValueEditor {
public:
    PropertyDoubleValueEditor()
    {
        Add(edit_.SizePos());
        edit_.SetTextAlign(UiAlign::RIGHT);
        edit_.WhenChange = [=] {
            if(!syncing_ && edit_.IsInputComplete())
                WhenPreview(edit_.GetData());
        };
        edit_.WhenCommit = [=] {
            if(!syncing_ && edit_.IsInputComplete())
                WhenCommit(edit_.GetData());
        };
    }

    virtual void Configure(const PropertyEditorItem& item) override
    {
        decimals_ = max(0, item.decimals);
        edit_.Enable(item.enabled && !item.read_only);
        edit_.SetPlaceholder(item.mixed ? "<mixed>" :
                             item.inherited ? "<inherited>" : "");
        edit_.Min(IsNumber(item.minimum) ? (double)item.minimum : -DBL_MAX);
        edit_.Max(IsNumber(item.maximum) ? (double)item.maximum : DBL_MAX);
        if(IsNumber(item.step))
            edit_.Step(max(0.0, (double)item.step));
        edit_.Precision(decimals_);
        Enable(item.enabled && !item.read_only);
    }

    virtual void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        if(mixed || IsNull(value))
            edit_.SetData(String());
        else
            edit_.SetValue((double)value);
        edit_.SetCommitBaseline(edit_.GetData());
        syncing_ = false;
    }

    virtual Value GetEditorValue() const override
    {
        return edit_.GetData();
    }

    virtual void FocusEditor() override
    {
        edit_.SetFocus();
        edit_.SetSelection();
    }

private:
    PropertyCommitFloatEdit edit_;
    bool syncing_ = false;
    int decimals_ = 3;
};

class PropertyNumericDoubleValueEditor : public PropertyValueEditor {
public:
    PropertyNumericDoubleValueEditor()
    {
        Add(edit_);
        Add(slider_);
        Add(toggle_);
        slider_.SetCustomStyle(UiTheme::ResolveSlider());
        slider_.ExpandTrack();
        toggle_.SetText("")
               .SetIcon(ICON_DESIGN_SLIDERS_48())
               .SetIconSize(DPI(16), DPI(16))
               .SetIconRenderMode(UiIconRenderMode::MonoTint)
               .SetContentInset(DPI(1));
        toggle_.Tip("Switch between numeric entry and slider");
        toggle_.SetCustomStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));
        toggle_.WhenAction = [=] {
            slider_mode_ = !slider_mode_;
            UpdateVisible();
            FocusEditor();
        };
        slider_.WhenChanging = [=] {
            if(syncing_) return;
            syncing_ = true;
            edit_.SetValue(slider_.GetValue());
            syncing_ = false;
            WhenPreview(edit_.GetData());
        };
        slider_.WhenAction = [=] {
            if(!syncing_)
                WhenCommit(edit_.GetData());
        };
        edit_.WhenChange = [=] {
            if(!syncing_ && edit_.IsInputComplete())
                WhenPreview(edit_.GetData());
        };
        edit_.WhenCommit = [=] {
            if(!syncing_ && edit_.IsInputComplete())
                WhenCommit(edit_.GetData());
        };
    }

    virtual void Configure(const PropertyEditorItem& item) override
    {
        enabled_ = item.enabled && !item.read_only;
        decimals_ = max(0, item.decimals);
        minimum_ = IsNumber(item.minimum) ? (double)item.minimum : 0.0;
        maximum_ = IsNumber(item.maximum) ? (double)item.maximum : 100.0;
        step_ = IsNumber(item.step) ? max(0.0, (double)item.step) : 0.0;
        bounded_ = IsNumber(item.minimum) && IsNumber(item.maximum) && maximum_ > minimum_;
        slider_mode_ = slider_mode_ && bounded_;
        if(bounded_) {
            slider_.SetRange(minimum_, maximum_);
            slider_.SetStep(step_);
        }
        edit_.Min(minimum_);
        edit_.Max(maximum_);
        if(step_ > 0) edit_.Step(step_);
        edit_.Precision(decimals_);
        edit_.SetPlaceholder(item.mixed ? "<mixed>" :
                             item.inherited ? "<inherited>" : "");
        edit_.Enable(enabled_);
        slider_.Enable(enabled_);
        toggle_.Show(item.show_slider_toggle && bounded_);
        toggle_.Enable(enabled_);
        UpdateVisible();
    }

    virtual void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        if(mixed || IsNull(value)) {
            edit_.SetData(String());
            slider_.SetValue(bounded_ ? minimum_ : 0.0);
        }
        else {
            const double v = (double)value;
            edit_.SetValue(v);
            slider_.SetValue(v);
        }
        edit_.SetCommitBaseline(edit_.GetData());
        syncing_ = false;
    }

    virtual Value GetEditorValue() const override { return edit_.GetData(); }
    virtual void FocusEditor() override
    {
        if(slider_mode_)
            slider_.SetFocus();
        else {
            edit_.SetFocus();
            edit_.SetSelection();
        }
    }

    virtual void Layout() override
    {
        const int toggle_width = toggle_.IsShown() ? DPI(30) : 0;
        toggle_.SetRect(max(0, GetSize().cx - toggle_width), 0, toggle_width, GetSize().cy);
        const int width = max(0, GetSize().cx - toggle_width - DPI(4));
        edit_.SetRect(0, 0, width, GetSize().cy);
        slider_.SetRect(0, 0, width, GetSize().cy);
    }

private:
    void ActionIconsChanged() override
    {
        if(!action_icons_.numeric_slider.IsEmpty())
            toggle_.SetIcon(action_icons_.numeric_slider);
        toggle_.SetIconSize(action_icons_.size, action_icons_.size);
    }

    void UpdateVisible()
    {
        edit_.Show(!slider_mode_);
        slider_.Show(slider_mode_ && bounded_);
        Layout();
    }

    PropertyCommitFloatEdit edit_;
    UiSlider slider_;
    UiButton toggle_;
    bool syncing_ = false;
    bool enabled_ = true;
    bool bounded_ = false;
    bool slider_mode_ = false;
    double minimum_ = 0.0;
    double maximum_ = 100.0;
    double step_ = 0.0;
    int decimals_ = 3;
};

class PropertyBooleanValueEditor : public PropertyValueEditor {
public:
    PropertyBooleanValueEditor()
    {
        Add(option_.SizePos());
        option_.SetText(String());
        option_.SetCustomStyle(UiTheme::ResolveCheckBox(UICHECKVIS_CLASSIC));
        option_.WhenAction = [=] {
            if(syncing_)
                return;
            Value v = option_.GetData();
            // Preview callbacks may rebuild the owning PropertyEditor and
            // tear down this inline editor. Snapshot both callbacks before
            // dispatch so that teardown cannot clear the commit callback
            // between the preview and commit notifications.
            Event<Value> preview = WhenPreview;
            Event<Value> commit = WhenCommit;
            if(preview)
                preview(v);
            if(commit)
                commit(v);
        };
    }

    virtual void Configure(const PropertyEditorItem& item) override
    {
        mixed_ = item.mixed;
        option_.SetTriState(item.mixed);
        option_.Enable(item.enabled && !item.read_only);
    }

    virtual void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        mixed_ = mixed;
        option_.SetTriState(mixed);
        if(mixed)
            option_.SetState(UICHECK_INDETERMINATE);
        else
            option_.SetData(value);
        syncing_ = false;
    }

    virtual Value GetEditorValue() const override
    {
        int state = (int)option_.GetData();
        if(mixed_ && state == UICHECK_INDETERMINATE)
            return Value(Null);
        return state == UICHECK_CHECKED;
    }

    virtual void FocusEditor() override
    {
        option_.SetFocus();
    }

private:
    UiCheckBox option_;
    bool syncing_ = false;
    bool mixed_ = false;
};

class PropertyChoiceValueEditor : public PropertyValueEditor {
public:
    PropertyChoiceValueEditor()
    {
        Add(drop_.SizePos());
        drop_.SetCustomStyle(UiTheme::ResolveDropdown());
        drop_.WhenSelectData = [=](const Value& v) {
            if(syncing_)
                return;
            WhenPreview(v);
            WhenCommit(v);
        };
    }

    virtual void Configure(const PropertyEditorItem& item) override
    {
        syncing_ = true;
        drop_.Clear();
        for(const PropertyEditorChoice& choice : item.choices) {
            UiDropdown::Item it(choice.label, choice.value, true);
            it.icon = choice.icon;
            drop_.Add(it);
        }
        drop_.SetPlaceholderText(item.mixed ? "<multiple values>" :
                                 item.inherited ? "<inherited>" : "");
        drop_.Enable(item.enabled && !item.read_only);
        syncing_ = false;
    }

    virtual void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        if(mixed)
            drop_.ClearSelection();
        else
            drop_.SetDataSilently(value);
        syncing_ = false;
    }

    virtual Value GetEditorValue() const override
    {
        return drop_.GetSelectedData();
    }

    virtual void FocusEditor() override
    {
        drop_.SetFocus();
    }

private:
    UiDropdown drop_;
    bool syncing_ = false;
};

class PropertyColorSwatchCtrl : public Ctrl {
public:
    typedef PropertyColorSwatchCtrl CLASSNAME;

    PropertyColorSwatchCtrl()
    {
        Transparent();
        WantFocus();
    }

    PropertyColorSwatchCtrl& SetColor(Color color, bool mixed = false)
    {
        color_ = color;
        mixed_ = mixed;
        Tip(mixed ? "Multiple colours" :
            Format("#%02X%02X%02X", color.GetR(), color.GetG(), color.GetB()));
        Refresh();
        return *this;
    }

    Event<> WhenAction;

    void Paint(Draw& w) override
    {
        const Size size = GetSize();
        const int side = min(DPI(19), max(0, min(size.cx, size.cy) - DPI(2)));
        if(side <= 0)
            return;
        const Rect swatch = RectC((size.cx - side) / 2,
                                  (size.cy - side) / 2,
                                  side, side);
        Color fill = mixed_ ? SColorDisabled() : color_;
        if(!IsEnabled())
            fill = Blend(fill, SColorFace(), 110);
        w.DrawRect(swatch, fill);
        DrawFrame(w, swatch, HasFocus() ? SColorHighlight() : SColorShadow());
    }

    void LeftDown(Point, dword) override
    {
        if(!IsEnabled())
            return;
        SetFocus();
        WhenAction();
    }

    bool Key(dword key, int count) override
    {
        if(IsEnabled() && (key == K_ENTER || key == K_SPACE)) {
            WhenAction();
            return true;
        }
        return Ctrl::Key(key, count);
    }

private:
    Color color_ = Color(128, 128, 128);
    bool mixed_ = false;
};

class PropertyColorValueEditor : public PropertyValueEditor {
public:
    PropertyColorValueEditor()
    {
        Add(swatch_);
        Add(hex_);
        hex_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        hex_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
        swatch_.WhenAction = [=] { OpenColorDialog(); };
        hex_.WhenAction = [=] { OpenColorDialog(); };
    }

    void Configure(const PropertyEditorItem& item) override
    {
        swatch_.Enable(item.enabled && !item.read_only);
    }

    void SetEditorValue(const Value& value, bool mixed) override
    {
        mixed_ = mixed;
        value_ = mixed || IsNull(value) ? Value(Color(128, 128, 128)) : value;
        SetDisplay(Color(value_), mixed_);
    }

    Value GetEditorValue() const override
    {
        return mixed_ ? Value(Null) : value_;
    }

    void FocusEditor() override
    {
        swatch_.SetFocus();
    }

    void Layout() override
    {
        const int swatch_width = min(DPI(28), GetSize().cx);
        swatch_.SetRect(0, 0, swatch_width, GetSize().cy);
        hex_.SetRect(min(GetSize().cx, swatch_width + DPI(7)), 0,
                     max(0, GetSize().cx - swatch_width - DPI(7)), GetSize().cy);
    }

private:
    void SetDisplay(Color color, bool mixed = false)
    {
        swatch_.SetColor(color, mixed);
        hex_.SetText(mixed ? "<multiple values>"
                           : Format("#%02X%02X%02X", color.GetR(), color.GetG(), color.GetB()));
    }

    void OpenColorDialog()
    {
        const Color original = Color(value_);
        Color chosen = original;
        bool accepted = false;

        class ColorDialog : public TopWindow {
        public:
            UiColorPicker picker;
            ColorDialog()
            {
                Title("Color");
                Sizeable().Zoomable();
                SetRect(0, 0, DPI(720), DPI(520));
                Add(picker.SizePos());
            }
        } dlg;

        dlg.picker.SetSlotCount(1);
        dlg.picker.SetSlotColor(0, original, false);
        dlg.picker.SetActiveSlot(0);
        const auto preview_color = [&] {
            chosen = dlg.picker.GetColor();
            SetDisplay(chosen);
            WhenPreview(chosen);
        };
        dlg.picker.WhenChanging = preview_color;
        dlg.picker.WhenAction = preview_color;
        dlg.picker.WhenAccept = [&] {
            chosen = dlg.picker.GetColor();
            accepted = true;
            dlg.AcceptBreak(IDOK);
        };
        dlg.picker.WhenCancel = [&] {
            accepted = false;
            dlg.RejectBreak(IDCANCEL);
        };

        dlg.CenterOwner();
        if(dlg.Run() == IDOK && accepted) {
            value_ = chosen;
            mixed_ = false;
            SetDisplay(chosen);
            WhenCommit(value_);
        }
        else {
            SetDisplay(original, mixed_);
            WhenPreview(original);
        }
    }

    PropertyColorSwatchCtrl swatch_;
    UiLabel hex_;
    Value value_ = Color(128, 128, 128);
    bool mixed_ = false;
};

class PropertyFilePathValueEditor : public PropertyValueEditor {
public:
    PropertyFilePathValueEditor()
    {
        Add(edit_);
        Add(detail_);
        Add(expand_);
        Add(browse_);
        ConfigurePropertyAction(expand_, ICON_DESIGN_UNFOLD_MORE_48(),
                                "Expand or collapse the complete path");
        ConfigurePropertyAction(browse_, ICON_DESIGN_PENDING_48(), "Choose file");
        detail_.SetReadOnly();
        detail_.Hide();
        expand_.WhenAction = [=] { WhenToggleExpanded(); };
        browse_.WhenAction = [=] { Browse(); };
        edit_.WhenChange = [=] {
            if(!syncing_)
                WhenPreview(edit_.GetData());
        };
        edit_.WhenCommit = [=] {
            if(!syncing_)
                WhenCommit(edit_.GetData());
        };
    }

    virtual void Configure(const PropertyEditorItem& item) override
    {
        const bool enabled = item.enabled && !item.read_only;
        edit_.Enable(enabled);
        browse_.Enable(enabled);
        expand_.Show(item.expanded_row_span > 1);
        expand_.Enable(item.expanded_row_span > 1 && enabled);
        edit_.SetPlaceholder(item.mixed ? "<multiple values>" :
                             item.inherited ? "<inherited>" : "");
    }

    virtual void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        edit_.SetData(mixed ? Value(String()) : Value(AsString(value)));
        edit_.SetCommitBaseline(edit_.GetData());
        detail_.SetData(edit_.GetData());
        syncing_ = false;
    }

    virtual Value GetEditorValue() const override
    {
        return edit_.GetData();
    }

    virtual void FocusEditor() override
    {
        edit_.SetFocus();
        edit_.SetSelection();
    }

    virtual void Layout() override
    {
        const int gap = DPI(4);
        const int action = min(DPI(28), GetSize().cy);
        const int browse_x = max(0, GetSize().cx - action);
        const int expand_x = max(0, browse_x - (expand_.IsShown() ? action + gap : 0));
        if(expanded_) {
            edit_.Hide();
            detail_.Show();
            detail_.SetRect(0, 0, max(0, expand_x - gap), GetSize().cy);
            expand_.SetRect(expand_x, 0, action, min(action, GetSize().cy));
            browse_.SetRect(browse_x, 0, action, min(action, GetSize().cy));
        }
        else {
            detail_.Hide();
            edit_.Show();
            edit_.SetRect(0, 0, max(0, expand_x - gap), GetSize().cy);
            expand_.SetRect(expand_x, 0, expand_.IsShown() ? action : 0, min(action, GetSize().cy));
            browse_.SetRect(browse_x, 0, action, min(action, GetSize().cy));
        }
    }

    void SetExpanded(bool expanded) override
    {
        expanded_ = expanded;
        ActionIconsChanged();
        Layout();
    }

private:
    void ActionIconsChanged() override
    {
        if(action_icons_.browse.IsEmpty()) {
            browse_.ClearIcon();
            browse_.SetText("...");
        }
        else {
            browse_.SetText(String());
            browse_.SetIcon(action_icons_.browse);
            browse_.SetIconSize(action_icons_.size, action_icons_.size);
            browse_.SetIconRenderMode(UiIconRenderMode::MonoTint);
        }
        const Image expand_icon = expanded_ ? action_icons_.collapse : action_icons_.expand;
        if(!expand_icon.IsEmpty())
            expand_.SetIcon(expand_icon);
        expand_.SetIconSize(action_icons_.size, action_icons_.size);
    }

    void Browse()
    {
        const String selected = UiOsFileDialog::SelectOpenFile("Select file", String(), this);
        if(selected.IsEmpty())
            return;
        syncing_ = true;
        edit_.SetData(selected);
        detail_.SetData(selected);
        syncing_ = false;
        WhenPreview(selected);
        WhenCommit(selected);
    }

    PropertyCommitEdit edit_;
    UiMultiEdit detail_;
    UiToolButton expand_, browse_;
    bool syncing_ = false;
    bool expanded_ = false;
};

class PropertyColorPaletteValueEditor : public PropertyValueEditor {
public:
    PropertyColorPaletteValueEditor()
    {
        for(int i = 0; i < 8; i++) {
            Add(swatches_[i]);
            const int index = i;
            swatches_[i].WhenAction = [=] { OpenColorDialog(index); };
        }
    }

    void Configure(const PropertyEditorItem& item) override
    {
        count_ = clamp(item.color_count, 1, 8);
        enabled_ = item.enabled && !item.read_only;
        for(int i = 0; i < 8; i++) {
            swatches_[i].Show(i < count_);
            swatches_[i].Enable(enabled_ && i < count_);
        }
        Layout();
    }

    void SetEditorValue(const Value& value, bool mixed) override
    {
        mixed_ = mixed;
        value_.Clear();
        if(!mixed && value.Is<ValueArray>()) {
            ValueArray colors = value;
            for(int i = 0; i < count_; i++)
                value_.Add(i < colors.GetCount() && colors[i].Is<Color>()
                               ? Value(colors[i]) : Value(Color(128, 128, 128)));
        }
        else {
            for(int i = 0; i < count_; i++)
                value_.Add(Color(128, 128, 128));
        }
        UpdateSwatches();
    }

    Value GetEditorValue() const override
    {
        return mixed_ ? Value(Null) : Value(value_);
    }

    void FocusEditor() override
    {
        if(count_ > 0)
            swatches_[0].SetFocus();
    }

    void Layout() override
    {
        const int diameter = min(DPI(21), max(DPI(15), GetSize().cy));
        const int gap = DPI(3);
        for(int i = 0; i < count_; i++)
            swatches_[i].SetRect(i * (diameter + gap), 0,
                                diameter, GetSize().cy);
    }

private:
    void UpdateSwatches()
    {
        for(int i = 0; i < count_; i++)
            swatches_[i].SetColor(Color(value_[i]), mixed_);
    }

    void OpenColorDialog(int index)
    {
        if(index < 0 || index >= count_ || mixed_)
            return;
        ValueArray original = value_;
        bool accepted = false;

        class ColorDialog : public TopWindow {
        public:
            UiColorPicker picker;
            ColorDialog()
            {
                Title("Color");
                Sizeable().Zoomable();
                SetRect(0, 0, DPI(720), DPI(520));
                Add(picker.SizePos());
            }
        } dlg;

        dlg.picker.SetSlotCount(count_);
        for(int i = 0; i < count_; i++)
            dlg.picker.SetSlotColor(i, Color(value_[i]), false);
        dlg.picker.SetActiveSlot(index);
        const auto sync_all_slots = [&] {
            for(int i = 0; i < count_; i++)
                value_.At(i) = dlg.picker.GetSlotColor(i);
        };
        const auto preview_palette = [&] {
            sync_all_slots();
            UpdateSwatches();
            WhenPreview(value_);
        };
        dlg.picker.WhenChanging = preview_palette;
        dlg.picker.WhenAction = preview_palette;
        dlg.picker.WhenAccept = [&] {
            sync_all_slots();
            accepted = true;
            dlg.AcceptBreak(IDOK);
        };
        dlg.picker.WhenCancel = [&] {
            accepted = false;
            dlg.RejectBreak(IDCANCEL);
        };

        dlg.CenterOwner();
        if(dlg.Run() == IDOK && accepted) {
            UpdateSwatches();
            WhenCommit(value_);
        }
        else {
            value_ = original;
            UpdateSwatches();
            WhenPreview(value_);
        }
    }

    PropertyColorSwatchCtrl swatches_[8];
    ValueArray value_;
    int count_ = 1;
    bool enabled_ = true;
    bool mixed_ = false;
};

class PropertyFillRecipeValueEditor : public PropertyValueEditor {
public:
    PropertyFillRecipeValueEditor()
    {
        Add(mode_);
        Add(hex_);
        hex_.SetReadOnly();
        hex_.SetTextAlign(UiAlign::LEFT);
        hex_.SetCustomStyle(UiTheme::ResolveEdit(UiRole::Subtle));
        mode_.SetCustomStyle(UiTheme::ResolveDropdown());
        mode_.Add(UiDropdown::Item("None", "None", true));
        mode_.Add(UiDropdown::Item("Solid", "Solid", true));
        mode_.Add(UiDropdown::Item("Gradient", "QuadGradient", true));
        mode_.Add(UiDropdown::Item("Image unavailable", "Image", false));
        mode_.WhenSelectData = [=](const Value& value) {
            if(syncing_)
                return;
            const String next = AsString(value);
            if(next == "Image")
                return;
            const String previous = RecipeText("mode", "None");
            const Color seed = previous == "Solid"
                ? RecipeColor("solid", Color(128, 128, 128))
                : RecipeColor("top_left", Color(128, 128, 128));
            if(next == "Solid" && recipe_.Find("solid") < 0)
                recipe_.Set("solid", seed);
            if(next == "QuadGradient")
                for(int i = 0; i < 4; i++)
                    if(recipe_.Find(Key(i)) < 0)
                        recipe_.Set(Key(i), seed);
            recipe_.Set("mode", next);
            UpdateVisible();
            WhenPreview(recipe_);
            WhenCommit(recipe_);
        };
        for(int i = 0; i < 4; i++) {
            Add(swatches_[i]);
            const int index = i;
            swatches_[i].WhenAction = [=] { OpenColorDialog(index); };
        }
    }

    void Configure(const PropertyEditorItem& item) override
    {
        enabled_ = item.value_editable && item.enabled && !item.read_only;
        mode_.Enable(enabled_);
        for(int i = 0; i < 4; i++)
            swatches_[i].Enable(enabled_);
        UpdateVisible();
    }

    void SetEditorValue(const Value& value, bool mixed) override
    {
        mixed_ = mixed;
        recipe_ = value.Is<ValueMap>() ? (ValueMap)value : ValueMap();
        if(recipe_.Find("mode") < 0)
            recipe_.Set("mode", "None");
        if(recipe_.Find("solid") < 0)
            recipe_.Set("solid", Color(128, 128, 128));
        for(int i = 0; i < 4; i++)
            if(recipe_.Find(Key(i)) < 0)
                recipe_.Set(Key(i), RecipeColor("solid", Color(128, 128, 128)));
        syncing_ = true;
        mode_.SetDataSilently(RecipeText("mode", "None"));
        syncing_ = false;
        UpdateVisible();
    }

    Value GetEditorValue() const override { return recipe_; }

    void FocusEditor() override { mode_.SetFocus(); }

    void Layout() override
    {
        const String mode = RecipeText("mode", "None");
        const int count = mode == "QuadGradient" ? 4 : mode == "Solid" ? 1 : 0;
        const int gap = DPI(3);
        const int swatch_width = min(DPI(21), max(DPI(17), GetSize().cy - DPI(2)));
        const int swatch_total = count > 0
            ? count * swatch_width + (count - 1) * gap : 0;
        const int inter_gap = count > 0 ? DPI(4) : 0;
        const int hex_width = mode == "Solid" ? min(DPI(82), max(0, GetSize().cx / 3)) : 0;
        const int hex_gap = hex_width ? DPI(4) : 0;
        const int mode_width = min(DPI(88), max(DPI(64),
            GetSize().cx - swatch_total - inter_gap - hex_width - hex_gap));
        mode_.SetRect(0, 0, max(0, min(mode_width, GetSize().cx)), GetSize().cy);
        int x = min(GetSize().cx, mode_width + inter_gap);
        for(int i = 0; i < 4; i++) {
            if(i < count)
                swatches_[i].SetRect(x + i * (swatch_width + gap), 0,
                                    swatch_width, GetSize().cy);
            else
                swatches_[i].SetRect(0, 0, 0, 0);
        }
        const int hex_x = x + swatch_total + hex_gap;
        hex_.SetRect(hex_x, 0, max(0, min(hex_width, GetSize().cx - hex_x)), GetSize().cy);
    }

private:
    static String Key(int index)
    {
        static const char *keys[] = {
            "top_left", "top_right", "bottom_left", "bottom_right"
        };
        return keys[index];
    }

    String RecipeText(const String& key, const String& fallback) const
    {
        const int q = recipe_.Find(key);
        return q >= 0 ? AsString(recipe_.GetValue(q)) : fallback;
    }

    Color RecipeColor(const String& key, Color fallback) const
    {
        const int q = recipe_.Find(key);
        return q >= 0 && recipe_.GetValue(q).Is<Color>()
            ? Color(recipe_.GetValue(q)) : fallback;
    }

    void UpdateVisible()
    {
        const String mode = RecipeText("mode", "None");
        const int count = mode == "QuadGradient" ? 4 : mode == "Solid" ? 1 : 0;
        for(int i = 0; i < 4; i++) {
            swatches_[i].Show(i < count);
            const String key = mode == "Solid" ? "solid" : Key(i);
            swatches_[i].SetColor(RecipeColor(key, Color(128, 128, 128)), mixed_);
        }
        const Color solid = RecipeColor("solid", Color(128, 128, 128));
        hex_.SetTextUtf8(Format("#%02X%02X%02X", solid.GetR(), solid.GetG(), solid.GetB()));
        hex_.Show(mode == "Solid");
        Layout();
    }

    void OpenColorDialog(int index)
    {
        const String mode = RecipeText("mode", "None");
        const int count = mode == "QuadGradient" ? 4 : mode == "Solid" ? 1 : 0;
        if(!enabled_ || mixed_ || index < 0 || index >= count)
            return;
        ValueMap original = recipe_;
        bool accepted = false;

        class ColorDialog : public TopWindow {
        public:
            UiColorPicker picker;
            ColorDialog()
            {
                Title("Fill colour");
                Sizeable().Zoomable();
                SetRect(0, 0, DPI(720), DPI(520));
                Add(picker.SizePos());
            }
        } dlg;

        dlg.picker.SetSlotCount(count);
        for(int i = 0; i < count; i++) {
            const String slot_key = mode == "Solid" ? "solid" : Key(i);
            dlg.picker.SetSlotColor(i,
                RecipeColor(slot_key, Color(128, 128, 128)), false);
        }
        dlg.picker.SetActiveSlot(index);
        const auto sync_all_slots = [&] {
            for(int i = 0; i < count; i++) {
                const String slot_key = mode == "Solid" ? "solid" : Key(i);
                recipe_.Set(slot_key, dlg.picker.GetSlotColor(i));
            }
        };
        const auto preview_recipe = [&] {
            sync_all_slots();
            UpdateVisible();
            WhenPreview(recipe_);
        };
        dlg.picker.WhenChanging = preview_recipe;
        dlg.picker.WhenAction = preview_recipe;
        dlg.picker.WhenAccept = [&] {
            sync_all_slots();
            accepted = true;
            dlg.AcceptBreak(IDOK);
        };
        dlg.picker.WhenCancel = [&] { dlg.RejectBreak(IDCANCEL); };
        dlg.CenterOwner();
        if(dlg.Run() == IDOK && accepted)
            WhenCommit(recipe_);
        else {
            recipe_ = original;
            UpdateVisible();
            WhenPreview(recipe_);
        }
    }

    UiDropdown mode_;
    UiLineEdit hex_;
    PropertyColorSwatchCtrl swatches_[4];
    ValueMap recipe_;
    bool enabled_ = true;
    bool mixed_ = false;
    bool syncing_ = false;
};

class PropertySliderIntValueEditor : public PropertyValueEditor {
public:
    PropertySliderIntValueEditor()
    {
        Add(slider_);
        Add(edit_);
        slider_.SetCustomStyle(UiTheme::ResolveSlider());
        slider_.ExpandTrack();
        slider_.WhenChanging = [=] {
            if(syncing_)
                return;
            syncing_ = true;
            int value = (int)slider_.GetValue();
            edit_.SetValue(value);
            syncing_ = false;
            WhenPreview(Value(value));
        };
        slider_.WhenAction = [=] {
            if(syncing_)
                return;
            int value = (int)slider_.GetValue();
            edit_.SetValue(value);
            WhenCommit(Value(value));
        };
        edit_.WhenChange = [=] {
            if(!syncing_)
                WhenPreview(edit_.GetData());
        };
        edit_.WhenCommit = [=] {
            if(!syncing_)
                WhenCommit(edit_.GetData());
        };
    }

    virtual void Configure(const PropertyEditorItem& item) override
    {
        minimum_ = IsNumber(item.minimum) ? (int)item.minimum : 0;
        maximum_ = IsNumber(item.maximum) ? (int)item.maximum : 100;
        step_ = IsNumber(item.step) ? max(1, (int)item.step) : 1;
        if(maximum_ <= minimum_)
            maximum_ = minimum_ + 1;
        slider_.SetRange(minimum_, maximum_);
        slider_.SetStep(step_);
        slider_.Enable(item.enabled && !item.read_only);
        edit_.Enable(item.enabled && !item.read_only);
        edit_.MinMax(minimum_, maximum_);
        edit_.Step(step_);
        edit_.SetPlaceholder(item.mixed ? "<mixed>" :
                             item.inherited ? "<inherited>" : "");
    }

    virtual void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        if(mixed || IsNull(value)) {
            edit_.SetData(String());
            slider_.SetValue(minimum_);
        }
        else {
            int v = (int)value;
            edit_.SetValue(v);
            slider_.SetValue(v);
        }
        edit_.SetCommitBaseline(edit_.GetData());
        syncing_ = false;
    }

    virtual Value GetEditorValue() const override
    {
        return edit_.GetData();
    }

    virtual void Layout() override
    {
        Size sz = GetSize();
        int edit_cx = min(max(DPI(58), sz.cx / 4), DPI(96));
        slider_.SetRect(0, 0, max(0, sz.cx - edit_cx - DPI(6)), sz.cy);
        edit_.SetRect(max(0, sz.cx - edit_cx), 0, edit_cx, sz.cy);
    }

    virtual void FocusEditor() override
    {
        edit_.SetFocus();
        edit_.SetSelection();
    }

    UiSlider slider_;
    PropertyCommitIntEdit edit_;
    bool syncing_ = false;
    int minimum_ = 0;
    int maximum_ = 100;
    int step_ = 1;
};

class PropertySliderDoubleValueEditor : public PropertyValueEditor {
public:
    PropertySliderDoubleValueEditor()
    {
        Add(edit_.SizePos());
        edit_.Slider().SetCustomStyle(UiTheme::ResolveSlider());
        edit_.Slider().ExpandTrack();
        edit_.SetFieldWidth(DPI(62)).SetGap(DPI(4));
        edit_.WhenChanging = [=] {
            if(!syncing_)
                WhenPreview(edit_.GetData());
        };
        edit_.WhenAction = [=] {
            if(!syncing_)
                WhenCommit(edit_.GetData());
        };
    }

    virtual void Configure(const PropertyEditorItem& item) override
    {
        minimum_ = IsNumber(item.minimum) ? (double)item.minimum : 0.0;
        maximum_ = IsNumber(item.maximum) ? (double)item.maximum : 100.0;
        step_ = IsNumber(item.step) ? max(0.0, (double)item.step) : 0.0;
        decimals_ = max(0, item.decimals);
        if(maximum_ <= minimum_)
            maximum_ = minimum_ + 1.0;
        edit_.SetRange(minimum_, maximum_);
        if(step_ > 0)
            edit_.SetStep(step_);
        edit_.Field().Precision(decimals_);
        edit_.Enable(item.enabled && !item.read_only);
    }

    virtual void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        if(mixed || IsNull(value))
            edit_.SetData(String());
        else
            edit_.SetValue((double)value);
        syncing_ = false;
    }

    virtual Value GetEditorValue() const override
    {
        return edit_.GetData();
    }

    virtual void FocusEditor() override
    {
        edit_.Field().SetFocus();
        edit_.Field().SetSelection();
    }

private:
    UiSliderEdit edit_;
    bool syncing_ = false;
    double minimum_ = 0.0;
    double maximum_ = 100.0;
    double step_ = 0.0;
    int decimals_ = 3;
};

class PropertyVectorValueEditor : public PropertyValueEditor {
public:
    explicit PropertyVectorValueEditor(int count)
        : count_(count)
    {
        Add(expand_);
        ConfigurePropertyAction(expand_, ICON_DESIGN_UNFOLD_MORE_48(),
                                "Expand or collapse vector components");
        expand_.WhenAction = [=] { WhenToggleExpanded(); };
        for(int i = 0; i < count_; i++) {
            PropertyCommitFloatEdit& edit = edits_.Add();
            UiLabel& label = labels_.Add();
            Add(edit);
            Add(label);
            label.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
            label.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
            label.SetText(i == 0 ? "X" : i == 1 ? "Y" : "Z");
            edit.SetTextAlign(UiAlign::RIGHT);
            edit.WhenChange = [=] {
                if(!syncing_ && AllInputsComplete())
                    WhenPreview(GetEditorValue());
            };
            edit.WhenCommit = [=] {
                if(!syncing_ && AllInputsComplete())
                    WhenCommit(GetEditorValue());
            };
        }
    }

    virtual void Configure(const PropertyEditorItem& item) override
    {
        decimals_ = max(0, item.decimals);
        expand_.Show(item.expanded_row_span > 1);
        expand_.Enable(item.expanded_row_span > 1 && item.enabled && !item.read_only);
        for(UiLabel& label : labels_)
            label.Show();
        for(PropertyCommitFloatEdit& edit : edits_) {
            edit.Enable(item.enabled && !item.read_only);
            edit.SetPlaceholder(item.mixed ? "<mixed>" : item.inherited ? "<inherited>" : "");
            edit.Precision(decimals_);
        }
    }

    virtual void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        Vector<double> v = PropertyEditorReadVector(value, count_);
        for(int i = 0; i < edits_.GetCount(); i++)
            edits_[i].SetData(mixed ? Value(String()) : Value(v[i]));
        for(PropertyCommitFloatEdit& edit : edits_)
            edit.SetCommitBaseline(edit.GetData());
        syncing_ = false;
    }

    virtual Value GetEditorValue() const override
    {
        ValueArray value;
        for(const PropertyCommitFloatEdit& edit : edits_)
            value.Add(edit.GetData());
        return value;
    }

    virtual void Layout() override
    {
        Size sz = GetSize();
        const int gap = DPI(4);
        const int label_cx = DPI(14);
        const int action = expand_.IsShown() ? min(DPI(28), sz.cy) : 0;
        const int content_cx = max(0, sz.cx - action - (action ? DPI(3) : 0));
        if(expanded_) {
            const int row = max(1, sz.cy / count_);
            for(int i = 0; i < edits_.GetCount(); i++) {
                const int y = i * row;
                const int cy = i + 1 == edits_.GetCount() ? sz.cy - y : row;
                labels_[i].SetRect(0, y, min(label_cx, content_cx), cy);
                edits_[i].SetRect(min(label_cx, content_cx), y,
                                  max(0, content_cx - label_cx), cy);
            }
            expand_.SetRect(content_cx + DPI(3), 0, action, min(action, sz.cy));
            return;
        }
        const int columns = 4;
        int cell = max(1, (content_cx - gap * (columns - 1)) / columns);
        int x = 0;
        for(int i = 0; i < edits_.GetCount(); i++) {
            int cx = min(cell, max(0, content_cx - x));
            labels_[i].SetRect(x, 0, min(label_cx, cx), sz.cy);
            edits_[i].SetRect(x + min(label_cx, cx), 0,
                              max(0, cx - label_cx), sz.cy);
            x += cell + gap;
        }
        expand_.SetRect(content_cx + DPI(3), 0, action, min(action, sz.cy));
    }

    void SetExpanded(bool expanded) override
    {
        expanded_ = expanded;
        ActionIconsChanged();
        Layout();
    }

    virtual void FocusEditor() override
    {
        if(!edits_.IsEmpty()) {
            edits_[0].SetFocus();
            edits_[0].SetSelection();
        }
    }

private:
    void ActionIconsChanged() override
    {
        const Image icon = expanded_ ? action_icons_.collapse : action_icons_.expand;
        if(!icon.IsEmpty())
            expand_.SetIcon(icon);
        expand_.SetIconSize(action_icons_.size, action_icons_.size);
    }

    bool AllInputsComplete() const
    {
        for(const PropertyCommitFloatEdit& edit : edits_)
            if(!edit.IsInputComplete())
                return false;
        return true;
    }

    Array<PropertyCommitFloatEdit> edits_;
    Array<UiLabel> labels_;
    UiToolButton expand_;
    int count_ = 2;
    int decimals_ = 3;
    bool syncing_ = false;
    bool expanded_ = false;
};

class PropertyReadOnlyValueEditor : public PropertyValueEditor {
public:
    PropertyReadOnlyValueEditor()
    {
        Add(label_.SizePos());
        label_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        label_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
    }

    virtual void Configure(const PropertyEditorItem&) override
    {
    }

    virtual void SetEditorValue(const Value& value, bool mixed) override
    {
        value_ = value;
        label_.SetText(mixed ? "<multiple values>" : AsString(value));
    }

    virtual Value GetEditorValue() const override
    {
        return value_;
    }

private:
    UiLabel label_;
    Value value_;
};

class PropertyCurveDialog : public TopWindow {
public:
    typedef PropertyCurveDialog CLASSNAME;

    PropertyCurveDialog()
    {
        Title("Curve editor");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(560), DPI(410));

        Add(canvas_);
        Add(help_);
        Add(reset_);
        Add(remove_);
        Add(ok_);
        Add(cancel_);

        help_.SetText("Click empty space to add a point. Drag points to move them. Delete removes the selected point.");
        help_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
        reset_.SetText("Linear");
        remove_.SetText("Remove");
        ok_.SetText("OK");
        cancel_.SetText("Cancel");
        reset_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
        remove_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
        ok_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
        cancel_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));

        reset_.WhenAction = [=] { canvas_.ResetLinear(); };
        remove_.WhenAction = [=] { canvas_.DeleteSelected(); };
        ok_.WhenAction = [=] { AcceptBreak(IDOK); };
        cancel_.WhenAction = [=] { RejectBreak(IDCANCEL); };
    }

    void SetCurve(const Value& value)
    {
        canvas_.SetCurve(value);
    }

    Value GetCurve() const
    {
        return canvas_.GetCurve();
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        int pad = DPI(10);
        int button_h = DPI(28);
        int help_h = DPI(34);
        int button_w = DPI(82);
        int gap = DPI(6);

        help_.SetRect(pad, pad, max(0, r.GetWidth() - 2 * pad), help_h);
        int bottom = r.bottom - pad - button_h;
        canvas_.SetRect(pad, pad + help_h + gap,
                        max(0, r.GetWidth() - 2 * pad),
                        max(0, bottom - (pad + help_h + 2 * gap)));

        int x = pad;
        reset_.SetRect(x, bottom, button_w, button_h);
        x += button_w + gap;
        remove_.SetRect(x, bottom, button_w, button_h);

        cancel_.SetRect(r.right - pad - button_w, bottom, button_w, button_h);
        ok_.SetRect(r.right - pad - 2 * button_w - gap, bottom, button_w, button_h);
    }

private:
    PropertyCurveCanvas canvas_;
    UiLabel help_;
    UiButton reset_;
    UiButton remove_;
    UiButton ok_;
    UiButton cancel_;
};

class PropertyCurveValueEditor : public PropertyValueEditor {
public:
    PropertyCurveValueEditor()
    {
        Add(summary_);
        Add(expand_);
        Add(dialog_);
        Add(canvas_);
        Add(bezier_canvas_);
        summary_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        summary_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
        ConfigurePropertyAction(expand_, ICON_DESIGN_UNFOLD_MORE_48(),
                                "Expand or collapse inline curve");
        ConfigurePropertyAction(dialog_, ICON_DESIGN_BOTTOM_PANEL_OPEN_48(),
                                "Open curve editor dialog");
        expand_.WhenAction = [=] { WhenToggleExpanded(); };
        summary_.WhenAction = [=] {
            if(!expanded_)
                WhenToggleExpanded();
        };
        dialog_.WhenAction = [=] {
            Value edited = value_;
            const bool accepted = bezier_mode_
                ? EditPropertyBezierCurve(edited, this, y_minimum_, y_maximum_)
                : EditPropertyCurve(edited, this);
            if(accepted) {
                value_ = edited;
                SetCanvases();
                SetSummary();
                WhenPreview(value_);
                WhenCommit(value_);
            }
        };
        canvas_.WhenCurvePreview = [=](Value value) {
            value_ = value;
            summary_.SetText(PropertyEditorFormatCurve(value_));
            WhenPreview(value_);
        };
        canvas_.WhenCurveCommit = [=](Value value) {
            value_ = value;
            summary_.SetText(PropertyEditorFormatCurve(value_));
            WhenCommit(value_);
        };
        bezier_canvas_.WhenChanging = [=] {
            value_ = bezier_canvas_.GetData();
            SetSummary();
            WhenPreview(value_);
        };
        bezier_canvas_.WhenAction = [=] {
            value_ = bezier_canvas_.GetData();
            SetSummary();
            WhenCommit(value_);
        };
        canvas_.Hide();
        bezier_canvas_.Hide();
    }

    virtual void Configure(const PropertyEditorItem& item) override
    {
        bezier_mode_ = item.editor_variant == "bezier";
        y_minimum_ = !IsNull(item.minimum) ? (double)item.minimum : -1.0;
        y_maximum_ = !IsNull(item.maximum) ? (double)item.maximum : 2.0;
        bezier_canvas_.SetYRange(y_minimum_, y_maximum_);
        enabled_ = item.enabled && !item.read_only;
        expand_.Enable(item.expanded_row_span > 1);
        dialog_.Enable(enabled_);
        canvas_.Enable(enabled_);
        bezier_canvas_.Enable(enabled_);
    }

    virtual void SetEditorValue(const Value& value, bool mixed) override
    {
        value_ = bezier_mode_ ? PropertyEditorNormalizeBezierCurve(value)
                              : PropertyEditorNormalizeCurve(value);
        SetCanvases();
        summary_.SetText(mixed ? "<multiple curves>" : FormatSummary());
    }

    virtual Value GetEditorValue() const override
    {
        return value_;
    }

    virtual void Layout() override
    {
        Size sz = GetSize();
        const int row = min(DPI(28), sz.cy);
        const int action = min(DPI(28), row);
        if(expanded_) {
            const int rail_x = max(0, sz.cx - action);
            summary_.Hide();
            Ctrl& active = bezier_mode_ ? static_cast<Ctrl&>(bezier_canvas_)
                                        : static_cast<Ctrl&>(canvas_);
            active.SetRect(0, 0, max(0, rail_x - DPI(3)), sz.cy);
            expand_.SetRect(rail_x, 0, action, min(action, sz.cy));
            dialog_.SetRect(rail_x, min(action + DPI(2), sz.cy), action,
                            min(action, max(0, sz.cy - action - DPI(2))));
        }
        else {
            const int dialog_x = max(0, sz.cx - action);
            const int expand_x = max(0, dialog_x - action - DPI(2));
            summary_.Show();
            summary_.SetRect(0, 0, max(0, expand_x - DPI(4)), row);
            expand_.SetRect(expand_x, 0, action, row);
            dialog_.SetRect(dialog_x, 0, action, row);
            canvas_.SetRect(0, 0, 0, 0);
            bezier_canvas_.SetRect(0, 0, 0, 0);
        }
    }

    void SetExpanded(bool expanded) override
    {
        expanded_ = expanded;
        ActionIconsChanged();
        canvas_.Show(expanded_ && !bezier_mode_);
        bezier_canvas_.Show(expanded_ && bezier_mode_);
        Layout();
    }

    virtual void FocusEditor() override
    {
        if(!expanded_)
            expand_.SetFocus();
        else if(bezier_mode_)
            bezier_canvas_.SetFocus();
        else
            canvas_.SetFocus();
    }

private:
    String FormatSummary() const
    {
        return bezier_mode_ ? PropertyEditorFormatBezierCurve(value_)
                            : PropertyEditorFormatCurve(value_);
    }

    void SetSummary() { summary_.SetText(FormatSummary()); }

    void SetCanvases()
    {
        if(bezier_mode_)
            bezier_canvas_.SetData(value_);
        else
            canvas_.SetCurve(value_);
    }

    void ActionIconsChanged() override
    {
        const Image icon = expanded_ ? action_icons_.collapse : action_icons_.expand;
        if(!icon.IsEmpty())
            expand_.SetIcon(icon);
        expand_.SetIconSize(action_icons_.size, action_icons_.size);
        if(!action_icons_.dialog.IsEmpty())
            dialog_.SetIcon(action_icons_.dialog);
        dialog_.SetIconSize(action_icons_.size, action_icons_.size);
    }

    PropertyActionLabel summary_;
    UiToolButton expand_, dialog_;
    PropertyCurveCanvas canvas_;
    UiBezierCurveEditor bezier_canvas_;
    Value value_;
    bool expanded_ = false;
    bool enabled_ = true;
    bool bezier_mode_ = false;
    double y_minimum_ = -1.0;
    double y_maximum_ = 2.0;
};

PropertyEditorFactory& PropertyEditorFactory::Global()
{
    static PropertyEditorFactory factory;
    return factory;
}

One<PropertyValueEditor> PropertyEditorFactory::Create(const PropertyEditorItem& item) const
{
    if(item.kind == PropertyEditorKind::Custom) {
        int q = custom_.Find(item.custom_editor);
        if(q >= 0 && custom_[q])
            return custom_[q]();
        return One<PropertyValueEditor>();
    }

    switch(item.kind) {
    case PropertyEditorKind::Text:
        return One<PropertyValueEditor>(new PropertyTextValueEditor);
    case PropertyEditorKind::Multiline:
        return One<PropertyValueEditor>(new PropertyMultilineValueEditor);
    case PropertyEditorKind::Integer:
        return One<PropertyValueEditor>(new PropertyIntegerValueEditor);
    case PropertyEditorKind::Double:
        return One<PropertyValueEditor>(new PropertyDoubleValueEditor);
    case PropertyEditorKind::NumericInt:
        return One<PropertyValueEditor>(new PropertyNumericIntValueEditor);
    case PropertyEditorKind::NumericDouble:
        return One<PropertyValueEditor>(new PropertyNumericDoubleValueEditor);
    case PropertyEditorKind::Boolean:
        return One<PropertyValueEditor>(new PropertyBooleanValueEditor);
    case PropertyEditorKind::Choice:
        return One<PropertyValueEditor>(new PropertyChoiceValueEditor);
    case PropertyEditorKind::Color:
        return One<PropertyValueEditor>(new PropertyColorValueEditor);
    case PropertyEditorKind::ColorPalette:
        return One<PropertyValueEditor>(new PropertyColorPaletteValueEditor);
    case PropertyEditorKind::FillRecipe:
        return One<PropertyValueEditor>(new PropertyFillRecipeValueEditor);
    case PropertyEditorKind::FilePath:
        return One<PropertyValueEditor>(new PropertyFilePathValueEditor);
    case PropertyEditorKind::SliderInt:
        return One<PropertyValueEditor>(new PropertySliderIntValueEditor);
    case PropertyEditorKind::SliderDouble:
        return One<PropertyValueEditor>(new PropertySliderDoubleValueEditor);
    case PropertyEditorKind::Vector2:
        return One<PropertyValueEditor>(new PropertyVectorValueEditor(2));
    case PropertyEditorKind::Vector3:
        return One<PropertyValueEditor>(new PropertyVectorValueEditor(3));
    case PropertyEditorKind::Curve:
        return One<PropertyValueEditor>(new PropertyCurveValueEditor);
    case PropertyEditorKind::ReadOnly:
        return One<PropertyValueEditor>(new PropertyReadOnlyValueEditor);
    case PropertyEditorKind::Custom:
        break;
    }
    return One<PropertyValueEditor>();
}

void PropertyEditorFactory::RegisterCustom(const String& id,
                                           PropertyValueEditorCreator creator)
{
    int q = custom_.Find(id);
    if(q < 0)
        custom_.Add(id, pick(creator));
    else
        custom_[q] = pick(creator);
}

bool PropertyEditorFactory::HasCustom(const String& id) const
{
    return custom_.Find(id) >= 0;
}

Vector<String> PropertyEditorFactory::GetCustomIds() const
{
    Vector<String> out;
    for(int i = 0; i < custom_.GetCount(); i++)
        out.Add(custom_.GetKey(i));
    return out;
}

PropertyCurveCanvas::PropertyCurveCanvas()
{
    WantFocus();
    ResetLinear();
}

void PropertyCurveCanvas::SetCurve(const Value& value)
{
    const int previous = selected_;
    points_ = PropertyEditorReadCurve(PropertyEditorNormalizeCurve(value));
    selected_ = points_.IsEmpty() ? -1 : minmax(previous, 0, points_.GetCount() - 1);
    dragging_ = -1;
    Refresh();
}

Value PropertyCurveCanvas::GetCurve() const
{
    return PropertyEditorMakeCurve(points_);
}

void PropertyCurveCanvas::ResetLinear()
{
    points_.Clear();
    points_.Add(Pointf(0.0, 0.0));
    points_.Add(Pointf(1.0, 1.0));
    selected_ = 0;
    dragging_ = -1;
    Refresh();
    EmitPreview();
    EmitCommit();
}

void PropertyCurveCanvas::DeleteSelected()
{
    if(selected_ < 0 || selected_ >= points_.GetCount() || points_.GetCount() <= 2)
        return;
    points_.Remove(selected_);
    selected_ = min(selected_, points_.GetCount() - 1);
    Normalize();
    Refresh();
    EmitPreview();
    EmitCommit();
}

Rect PropertyCurveCanvas::GetGraphRect() const
{
    Rect r = GetSize();
    r.Deflate(DPI(16));
    return r;
}

Point PropertyCurveCanvas::CurveToClient(const Pointf& p) const
{
    Rect r = GetGraphRect();
    int x = r.left + (int)floor(p.x * r.GetWidth() + 0.5);
    int y = r.bottom - (int)floor(p.y * r.GetHeight() + 0.5);
    return Point(x, y);
}

Pointf PropertyCurveCanvas::ClientToCurve(Point p) const
{
    Rect r = GetGraphRect();
    if(r.GetWidth() <= 0 || r.GetHeight() <= 0)
        return Pointf(0, 0);
    double x = (double)(p.x - r.left) / (double)r.GetWidth();
    double y = (double)(r.bottom - p.y) / (double)r.GetHeight();
    return Pointf(minmax(x, 0.0, 1.0), minmax(y, 0.0, 1.0));
}

int PropertyCurveCanvas::HitPoint(Point p) const
{
    int radius = DPI(7);
    int best = -1;
    int best_d2 = radius * radius;
    for(int i = 0; i < points_.GetCount(); i++) {
        Point q = CurveToClient(points_[i]);
        int dx = p.x - q.x;
        int dy = p.y - q.y;
        int d2 = dx * dx + dy * dy;
        if(d2 <= best_d2) {
            best_d2 = d2;
            best = i;
        }
    }
    return best;
}

static double PropertyCurveSegmentDistance_(Point p, Point a, Point b)
{
    const Pointf ab(b.x - a.x, b.y - a.y);
    const Pointf ap(p.x - a.x, p.y - a.y);
    const double length2 = ab.x * ab.x + ab.y * ab.y;
    const double t = length2 > 0.0
        ? minmax((ap.x * ab.x + ap.y * ab.y) / length2, 0.0, 1.0) : 0.0;
    const double dx = p.x - (a.x + t * ab.x);
    const double dy = p.y - (a.y + t * ab.y);
    return sqrt(dx * dx + dy * dy);
}

void PropertyCurveCanvas::Normalize()
{
    Value normalized = PropertyEditorNormalizeCurve(PropertyEditorMakeCurve(points_));
    points_ = PropertyEditorReadCurve(normalized);
    if(selected_ >= points_.GetCount())
        selected_ = points_.GetCount() - 1;
}

void PropertyCurveCanvas::EmitPreview()
{
    WhenCurvePreview(GetCurve());
}

void PropertyCurveCanvas::EmitCommit()
{
    WhenCurveCommit(GetCurve());
}

void PropertyCurveCanvas::Paint(Draw& w)
{
    Size sz = GetSize();
    Color paper = SColorPaper();
    Color frame = SColorShadow();
    Color grid = Blend(SColorPaper(), SColorText(), 28);
    Color line = SColorHighlight();

    w.DrawRect(sz, paper);
    Rect r = GetGraphRect();
    w.DrawRect(r, paper);
    DrawFrame(w, r, frame);

    for(int i = 1; i < 4; i++) {
        int x = r.left + r.GetWidth() * i / 4;
        int y = r.top + r.GetHeight() * i / 4;
        w.DrawLine(x, r.top, x, r.bottom, 1, grid);
        w.DrawLine(r.left, y, r.right, y, 1, grid);
    }

    for(int i = 1; i < points_.GetCount(); i++) {
        Point a = CurveToClient(points_[i - 1]);
        Point b = CurveToClient(points_[i]);
        w.DrawLine(a.x, a.y, b.x, b.y, DPI(2), line);
    }

    int radius = DPI(5);
    for(int i = 0; i < points_.GetCount(); i++) {
        Point p = CurveToClient(points_[i]);
        Color fill = i == selected_ ? SColorHighlight() : SColorFace();
        Color ink = i == selected_ ? SColorHighlightText() : SColorText();
        w.DrawEllipse(RectC(p.x - radius, p.y - radius, 2 * radius + 1, 2 * radius + 1),
                      fill, 1, ink);
    }
    if(dragging_ >= 0 && dragging_ < points_.GetCount()) {
        const Point p = CurveToClient(points_[dragging_]);
        const String text = Format("x %.4f  y %.4f", points_[dragging_].x, points_[dragging_].y);
        const Font font = SansSerif().Height(DPI(10));
        const Size ts = GetTextSize(text, font);
        Rect badge = RectC(minmax(p.x + DPI(8), r.left, max(r.left, r.right - ts.cx - DPI(10))),
                           max(r.top, p.y - ts.cy - DPI(10)), ts.cx + DPI(8), ts.cy + DPI(4));
        w.DrawRect(badge, SColorPaper());
        DrawFrame(w, badge, SColorShadow());
        w.DrawText(badge.left + DPI(4), badge.top + DPI(2), text, font, SColorText());
    }
}

void PropertyCurveCanvas::LeftDown(Point p, dword)
{
    SetFocus();
    int hit = HitPoint(p);
    if(hit < 0) {
        double nearest = 1e9;
        for(int i = 1; i < points_.GetCount(); i++)
            nearest = min(nearest, PropertyCurveSegmentDistance_(p,
                          CurveToClient(points_[i - 1]), CurveToClient(points_[i])));
        if(nearest > DPI(9))
            return;
        points_.Add(ClientToCurve(p));
        Normalize();
        hit = HitPoint(p);
    }
    selected_ = hit;
    dragging_ = hit;
    SetCapture();
    Refresh();
    EmitPreview();
}

void PropertyCurveCanvas::MouseMove(Point p, dword keyflags)
{
    if(dragging_ < 0 || !(keyflags & K_MOUSELEFT))
        return;

    Pointf value = ClientToCurve(p);
    if(dragging_ == 0)
        value.x = 0.0;
    if(dragging_ == points_.GetCount() - 1)
        value.x = 1.0;

    points_[dragging_] = value;
    Normalize();
    selected_ = HitPoint(CurveToClient(value));
    dragging_ = selected_;
    Refresh();
    EmitPreview();
}

void PropertyCurveCanvas::LeftUp(Point, dword)
{
    if(dragging_ < 0)
        return;
    dragging_ = -1;
    ReleaseCapture();
    Normalize();
    Refresh();
    EmitCommit();
}

bool PropertyCurveCanvas::Key(dword key, int count)
{
    if(key == K_DELETE || key == K_BACKSPACE) {
        DeleteSelected();
        return true;
    }
    if(selected_ >= 0 && selected_ < points_.GetCount() &&
       (key == K_LEFT || key == K_RIGHT || key == K_UP || key == K_DOWN)) {
        Pointf next = points_[selected_];
        const double step = 0.01 * max(1, count);
        if(key == K_LEFT) next.x -= step;
        if(key == K_RIGHT) next.x += step;
        if(key == K_UP) next.y += step;
        if(key == K_DOWN) next.y -= step;
        if(selected_ == 0) next.x = 0.0;
        if(selected_ == points_.GetCount() - 1) next.x = 1.0;
        next.x = minmax(next.x, 0.0, 1.0);
        next.y = minmax(next.y, 0.0, 1.0);
        points_[selected_] = next;
        Normalize();
        Refresh();
        EmitPreview();
        EmitCommit();
        return true;
    }
    return Ctrl::Key(key, count);
}

Size PropertyCurveCanvas::GetMinSize() const
{
    return Size(DPI(280), DPI(220));
}

bool EditPropertyCurve(Value& value, Ctrl *owner)
{
        PropertyCurveDialog dlg;
        dlg.SetCurve(value);
        if(owner)
            dlg.CenterOwner();
        if(dlg.Run() != IDOK)
            return false;
    value = PropertyEditorNormalizeCurve(dlg.GetCurve());
    return true;
}

bool EditPropertyBezierCurve(Value& value, Ctrl *owner,
                             double y_minimum, double y_maximum)
{
    struct Dialog : TopWindow {
        UiBezierCurveEditor editor;
        UiButton ok, cancel;

        Dialog()
        {
            Title("Bezier curve editor").Sizeable().Zoomable();
            SetRect(0, 0, DPI(520), DPI(380));
            Add(editor.HSizePos(DPI(12), DPI(12)).VSizePos(DPI(12), DPI(52)));
            Add(ok.RightPos(DPI(112), DPI(96)).BottomPos(DPI(12), DPI(30)));
            Add(cancel.RightPos(DPI(12), DPI(92)).BottomPos(DPI(12), DPI(30)));
            ok.SetText("OK");
            cancel.SetText("Cancel");
            ok.WhenAction = [=] { AcceptBreak(IDOK); };
            cancel.WhenAction = [=] { RejectBreak(IDCANCEL); };
        }
    } dlg;

    dlg.editor.SetYRange(y_minimum, y_maximum)
              .SetData(PropertyEditorNormalizeBezierCurve(value));
    if(owner)
        dlg.CenterOwner();
    if(dlg.Run() != IDOK)
        return false;
    value = PropertyEditorNormalizeBezierCurve(dlg.editor.GetData());
    return true;
}

}
