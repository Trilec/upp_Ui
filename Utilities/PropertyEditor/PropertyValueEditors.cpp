#include "PropertyValueEditors.h"

#include <cmath>
#include <Ui/UiColorPicker.h>
#include <Ui/UiIcons.h>
#include <Ui/UiOsFileDialog/UiOsFileDialog.h>
#include <Ui/UiSliderEdit.h>
#include <Ui/UiTheme.h>

namespace Upp {

void PropertyValueEditor::FocusEditor()
{
    SetFocus();
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

private:
    void EmitCommit()
    {
        Value v = T::GetData();
        if(!has_last_commit_ || v != last_commit_) {
            last_commit_ = v;
            has_last_commit_ = true;
            WhenCommit();
        }
    }

    Value last_commit_;
    bool has_last_commit_ = false;
};

using PropertyCommitEdit = PropertyCommitEditCtrl<UiLineEdit>;
using PropertyCommitMultiEdit = PropertyCommitEditCtrl<UiMultiEdit>;
using PropertyCommitIntEdit = PropertyCommitEditCtrl<UiIntEdit>;
using PropertyCommitFloatEdit = PropertyCommitEditCtrl<UiFloatEdit>;

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
        Enable(item.enabled && !item.read_only);
    }

    virtual void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        edit_.SetData(mixed ? Value(String()) : Value(AsString(value)));
        syncing_ = false;
    }

    virtual Value GetEditorValue() const override
    {
        return edit_.GetData();
    }

    virtual void FocusEditor() override
    {
        edit_.SetFocus();
    }

private:
    PropertyCommitMultiEdit edit_;
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
        toggle_.SetIcon(ICON_DESIGN_SLIDERS_48())
               .SetIconSize(DPI(20), DPI(20))
               .SetContentInset(DPI(1));
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
        toggle_.SetIcon(ICON_DESIGN_SLIDERS_48())
               .SetIconSize(DPI(20), DPI(20))
               .SetContentInset(DPI(1));
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
            WhenPreview(v);
            WhenCommit(v);
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
        const int diameter = min(DPI(16), max(0, min(size.cx, size.cy) - DPI(4)));
        if(diameter <= 0)
            return;
        const Rect dot = RectC((size.cx - diameter) / 2,
                               (size.cy - diameter) / 2,
                               diameter, diameter);
        Color fill = mixed_ ? SColorDisabled() : color_;
        if(!IsEnabled())
            fill = Blend(fill, SColorFace(), 110);
        w.DrawEllipse(dot, fill, 1,
                      HasFocus() ? SColorHighlight() : SColorShadow());
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
        swatch_.WhenAction = [=] { OpenColorDialog(); };
    }

    void Configure(const PropertyEditorItem& item) override
    {
        swatch_.Enable(item.enabled && !item.read_only);
    }

    void SetEditorValue(const Value& value, bool mixed) override
    {
        mixed_ = mixed;
        value_ = mixed || IsNull(value) ? Value(Color(128, 128, 128)) : value;
        swatch_.SetColor(Color(value_), mixed_);
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
        swatch_.SetRect(0, 0, min(DPI(28), GetSize().cx), GetSize().cy);
    }

private:
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

        dlg.picker.SetColor(original);
        dlg.picker.WhenChanging = [&] {
            chosen = dlg.picker.GetColor();
            swatch_.SetColor(chosen);
            WhenPreview(chosen);
        };
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
            swatch_.SetColor(chosen);
            WhenCommit(value_);
        }
        else {
            swatch_.SetColor(original, mixed_);
            WhenPreview(original);
        }
    }

    PropertyColorSwatchCtrl swatch_;
    Value value_ = Color(128, 128, 128);
    bool mixed_ = false;
};

class PropertyFilePathValueEditor : public PropertyValueEditor {
public:
    PropertyFilePathValueEditor()
    {
        Add(edit_);
        Add(browse_);
        browse_.SetText("...");
        browse_.SetCustomStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));
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
        edit_.SetPlaceholder(item.mixed ? "<multiple values>" :
                             item.inherited ? "<inherited>" : "");
    }

    virtual void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        edit_.SetData(mixed ? Value(String()) : Value(AsString(value)));
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
        const int button_width = DPI(30);
        edit_.SetRect(0, 0, max(0, GetSize().cx - button_width - gap), GetSize().cy);
        browse_.SetRect(max(0, GetSize().cx - button_width), 0, button_width, GetSize().cy);
    }

private:
    void Browse()
    {
        const String selected = UiOsFileDialog::SelectOpenFile("Select file", String(), this);
        if(selected.IsEmpty())
            return;
        syncing_ = true;
        edit_.SetData(selected);
        syncing_ = false;
        WhenPreview(selected);
        WhenCommit(selected);
    }

    PropertyCommitEdit edit_;
    UiButton browse_;
    bool syncing_ = false;
};

class PropertyColorPaletteValueEditor : public PropertyValueEditor {
public:
    PropertyColorPaletteValueEditor()
    {
        for(int i = 0; i < 4; i++) {
            Add(swatches_[i]);
            const int index = i;
            swatches_[i].WhenAction = [=] { OpenColorDialog(index); };
        }
    }

    void Configure(const PropertyEditorItem& item) override
    {
        count_ = clamp(item.color_count, 1, 4);
        enabled_ = item.enabled && !item.read_only;
        for(int i = 0; i < 4; i++) {
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
        const int diameter = min(DPI(20), max(DPI(14), GetSize().cy));
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
        Color chosen = Color(value_[index]);
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

        dlg.picker.SetColor(chosen);
        dlg.picker.WhenChanging = [&] {
            chosen = dlg.picker.GetColor();
            value_.At(index) = chosen;
            UpdateSwatches();
            WhenPreview(value_);
        };
        dlg.picker.WhenAccept = [&] {
            chosen = dlg.picker.GetColor();
            value_.At(index) = chosen;
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

    PropertyColorSwatchCtrl swatches_[4];
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
        const int swatch_width = min(DPI(18), max(DPI(14), GetSize().cy - DPI(4)));
        const int swatch_total = count > 0
            ? count * swatch_width + (count - 1) * gap : 0;
        const int inter_gap = count > 0 ? DPI(4) : 0;
        const int mode_width = min(DPI(88),
            max(DPI(64), GetSize().cx - swatch_total - inter_gap));
        mode_.SetRect(0, 0, max(0, min(mode_width, GetSize().cx)), GetSize().cy);
        int x = min(GetSize().cx, mode_width + inter_gap);
        for(int i = 0; i < 4; i++) {
            if(i < count)
                swatches_[i].SetRect(x + i * (swatch_width + gap), 0,
                                    swatch_width, GetSize().cy);
            else
                swatches_[i].SetRect(0, 0, 0, 0);
        }
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
        Layout();
    }

    void OpenColorDialog(int index)
    {
        const String mode = RecipeText("mode", "None");
        const int count = mode == "QuadGradient" ? 4 : mode == "Solid" ? 1 : 0;
        if(!enabled_ || mixed_ || index < 0 || index >= count)
            return;
        const String key = mode == "Solid" ? "solid" : Key(index);
        ValueMap original = recipe_;
        Color chosen = RecipeColor(key, Color(128, 128, 128));
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

        dlg.picker.SetColor(chosen);
        dlg.picker.WhenChanging = [&] {
            recipe_.Set(key, dlg.picker.GetColor());
            UpdateVisible();
            WhenPreview(recipe_);
        };
        dlg.picker.WhenAccept = [&] {
            recipe_.Set(key, dlg.picker.GetColor());
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
                if(!syncing_)
                    WhenPreview(GetEditorValue());
            };
            edit.WhenCommit = [=] {
                if(!syncing_)
                    WhenCommit(GetEditorValue());
            };
        }
    }

    virtual void Configure(const PropertyEditorItem& item) override
    {
        decimals_ = max(0, item.decimals);
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
        int gap = DPI(4);
        int label_h = DPI(14);
        int cell = max(1, (sz.cx - gap * (count_ - 1)) / count_);
        int x = 0;
        for(int i = 0; i < edits_.GetCount(); i++) {
            int cx = i + 1 == edits_.GetCount() ? sz.cx - x : cell;
            labels_[i].SetRect(x, 0, max(0, cx), label_h);
            edits_[i].SetRect(x, label_h + DPI(2), max(0, cx), max(0, sz.cy - label_h - DPI(2)));
            x += cell + gap;
        }
    }

    virtual void FocusEditor() override
    {
        if(!edits_.IsEmpty()) {
            edits_[0].SetFocus();
            edits_[0].SetSelection();
        }
    }

private:
    Array<PropertyCommitFloatEdit> edits_;
    Array<UiLabel> labels_;
    int count_ = 2;
    int decimals_ = 3;
    bool syncing_ = false;
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
        Add(button_);
        summary_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        summary_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
        button_.SetText("Edit...");
        button_.SetCustomStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));
        button_.WhenAction = [=] {
            Value edited = value_;
            if(EditPropertyCurve(edited, this)) {
                value_ = edited;
                summary_.SetText(PropertyEditorFormatCurve(value_));
                WhenPreview(value_);
                WhenCommit(value_);
            }
        };
    }

    virtual void Configure(const PropertyEditorItem& item) override
    {
        button_.Enable(item.enabled && !item.read_only);
    }

    virtual void SetEditorValue(const Value& value, bool mixed) override
    {
        value_ = PropertyEditorNormalizeCurve(value);
        summary_.SetText(mixed ? "<multiple curves>" : PropertyEditorFormatCurve(value_));
    }

    virtual Value GetEditorValue() const override
    {
        return value_;
    }

    virtual void Layout() override
    {
        Size sz = GetSize();
        int button_cx = min(DPI(76), max(DPI(54), sz.cx / 3));
        summary_.SetRect(0, 0, max(0, sz.cx - button_cx - DPI(4)), sz.cy);
        button_.SetRect(max(0, sz.cx - button_cx), 0, button_cx, sz.cy);
    }

    virtual void FocusEditor() override
    {
        button_.SetFocus();
    }

private:
    UiLabel summary_;
    UiButton button_;
    Value value_;
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
    points_ = PropertyEditorReadCurve(PropertyEditorNormalizeCurve(value));
    selected_ = points_.IsEmpty() ? -1 : 0;
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
}

void PropertyCurveCanvas::LeftDown(Point p, dword)
{
    SetFocus();
    int hit = HitPoint(p);
    if(hit < 0) {
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

}
