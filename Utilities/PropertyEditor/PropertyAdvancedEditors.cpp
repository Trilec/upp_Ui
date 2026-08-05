#include "PropertyValueEditors.h"

#include <Ui/UiColorPicker.h>
#include <Ui/UiIcons.h>
#include <Ui/UiTheme.h>

namespace Upp {

template <class T>
class PropertyLiveCommitEdit : public T {
public:
    typedef PropertyLiveCommitEdit CLASSNAME;

    PropertyLiveCommitEdit()
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
        Value value = T::GetData();
        if(!has_last_commit_ || value != last_commit_) {
            last_commit_ = value;
            has_last_commit_ = true;
            WhenCommit();
        }
    }

    Value last_commit_;
    bool has_last_commit_ = false;
};

class PropertyLiveNumericIntEditor : public PropertyValueEditor {
public:
    PropertyLiveNumericIntEditor()
    {
        Add(edit_);
        Add(slider_);
        Add(toggle_);

        edit_.SetTextAlign(UiAlign::RIGHT);
        slider_.SetCustomStyle(UiTheme::ResolveSlider());
        toggle_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
        toggle_.SetText("");
        toggle_.SetIcon(ICON_DESIGN_SLIDERS_48())
               .SetIconSize(DPI(18), DPI(18))
               .SetContentInset(DPI(3))
               .SetContentGap(DPI(0))
               .SetAlign(UiAlign::CENTER, UiAlign::CENTER)
               .SetIconRenderMode(UiIconRenderMode::MonoTint)
               .SetIconScaleToContent(false);
        toggle_.NoWantFocus();
        toggle_.Tip("Switch between numeric entry and slider");

        toggle_.WhenAction = [=] {
            if(!bounded_)
                return;
            slider_mode_ = !slider_mode_;
            UpdateVisible();
            FocusEditor();
        };
        slider_.WhenChanging = [=] {
            if(syncing_)
                return;
            syncing_ = true;
            edit_.SetValue((int)slider_.GetValue());
            syncing_ = false;
            WhenPreview(edit_.GetData());
        };
        slider_.WhenAction = [=] {
            if(!syncing_)
                WhenCommit(edit_.GetData());
        };
        edit_.WhenChange = [=] {
            if(syncing_)
                return;
            syncing_ = true;
            if(!IsNull(edit_.GetData()))
                slider_.SetValue((int)edit_.GetData());
            syncing_ = false;
            WhenPreview(edit_.GetData());
        };
        edit_.WhenCommit = [=] {
            if(!syncing_)
                WhenCommit(edit_.GetData());
        };
    }

    void Configure(const PropertyEditorItem& item) override
    {
        enabled_ = item.enabled && item.value_editable && !item.read_only;
        minimum_ = IsNumber(item.minimum) ? (int)item.minimum : INT_MIN;
        maximum_ = IsNumber(item.maximum) ? (int)item.maximum : INT_MAX;
        step_ = IsNumber(item.step) ? max(1, (int)item.step) : 1;
        bounded_ = IsNumber(item.minimum) && IsNumber(item.maximum) &&
                   maximum_ > minimum_;

        edit_.SetPlaceholder(item.mixed ? "<mixed>" :
                             item.inherited ? "<inherited>" : "");
        edit_.Min(minimum_);
        edit_.Max(maximum_);
        edit_.Step(step_);
        slider_.SetRange(minimum_, maximum_);
        slider_.SetStep(step_);

        edit_.Enable(enabled_);
        slider_.Enable(enabled_);
        toggle_.Show(bounded_);
        toggle_.Enable(enabled_ && bounded_);
        slider_mode_ = slider_mode_ && bounded_;
        UpdateVisible();
    }

    void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        if(mixed || IsNull(value)) {
            edit_.SetData(String());
            slider_.SetValue(bounded_ ? minimum_ : 0);
        }
        else {
            int v = (int)value;
            edit_.SetValue(v);
            slider_.SetValue(v);
        }
        syncing_ = false;
    }

    Value GetEditorValue() const override
    {
        return edit_.GetData();
    }

    void FocusEditor() override
    {
        if(slider_mode_)
            slider_.SetFocus();
        else {
            edit_.SetFocus();
            edit_.SetSelection();
        }
    }

    void Layout() override
    {
        const int toggle_width = toggle_.IsShown() ? DPI(34) : 0;
        toggle_.SetRect(max(0, GetSize().cx - toggle_width),
                        0, toggle_width, GetSize().cy);
        const int gap = toggle_width ? DPI(4) : 0;
        const int width = max(0, GetSize().cx - toggle_width - gap);
        edit_.SetRect(0, 0, width, GetSize().cy);
        slider_.SetRect(0, 0, width, GetSize().cy);
    }

private:
    void UpdateVisible()
    {
        edit_.Show(!slider_mode_ || !bounded_);
        slider_.Show(slider_mode_ && bounded_);
        toggle_.SetIcon(slider_mode_
            ? ICON_EDITOR_FORMAT_SIZE_48()
            : ICON_DESIGN_SLIDERS_48());
        toggle_.Tip(slider_mode_
            ? "Switch to numeric entry"
            : "Switch to slider");
        Layout();
    }

    PropertyLiveCommitEdit<UiIntEdit> edit_;
    UiSlider slider_ { UiDirection::H };
    UiToolButton toggle_;
    bool syncing_ = false;
    bool enabled_ = true;
    bool bounded_ = false;
    bool slider_mode_ = false;
    int minimum_ = INT_MIN;
    int maximum_ = INT_MAX;
    int step_ = 1;
};

class PropertyLiveColorSwatch : public Ctrl {
public:
    typedef PropertyLiveColorSwatch CLASSNAME;

    PropertyLiveColorSwatch()
    {
        Transparent();
        WantFocus();
    }

    PropertyLiveColorSwatch& SetColor(Color color, bool mixed = false)
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
        Size size = GetSize();
        int diameter = min(DPI(16),
                           max(0, min(size.cx, size.cy) - DPI(4)));
        if(diameter <= 0)
            return;
        Rect dot = RectC((size.cx - diameter) / 2,
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

class PropertyLiveFillRecipeEditor : public PropertyValueEditor {
public:
    PropertyLiveFillRecipeEditor()
    {
        Add(mode_);
        mode_.SetCustomStyle(UiTheme::ResolveDropdown());
        mode_.Add(UiDropdown::Item("None", "None", true));
        mode_.Add(UiDropdown::Item("Solid", "Solid", true));
        mode_.Add(UiDropdown::Item("Gradient", "QuadGradient", true));
        mode_.WhenSelectData = [=](const Value& value) {
            if(syncing_)
                return;
            String next = AsString(value);
            String previous = RecipeText("mode", "None");
            Color seed = previous == "Solid"
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
        enabled_ = item.enabled && item.value_editable && !item.read_only;
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

    Value GetEditorValue() const override
    {
        return recipe_;
    }

    void FocusEditor() override
    {
        mode_.SetFocus();
    }

    void Layout() override
    {
        String mode = RecipeText("mode", "None");
        int count = mode == "QuadGradient" ? 4 : mode == "Solid" ? 1 : 0;
        int gap = DPI(3);
        int swatch_width = min(DPI(18), max(DPI(14), GetSize().cy - DPI(4)));
        int swatch_total = count > 0
            ? count * swatch_width + (count - 1) * gap : 0;
        int inter_gap = count > 0 ? DPI(4) : 0;
        int mode_width = min(DPI(88),
            max(DPI(64), GetSize().cx - swatch_total - inter_gap));
        mode_.SetRect(0, 0, max(0, min(mode_width, GetSize().cx)), GetSize().cy);

        int x = min(GetSize().cx, mode_width + inter_gap);
        for(int i = 0; i < 4; i++) {
            if(i < count)
                swatches_[i].SetRect(x + i * (swatch_width + gap),
                                    0, swatch_width, GetSize().cy);
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
        int q = recipe_.Find(key);
        return q >= 0 ? AsString(recipe_.GetValue(q)) : fallback;
    }

    Color RecipeColor(const String& key, Color fallback) const
    {
        int q = recipe_.Find(key);
        return q >= 0 && recipe_.GetValue(q).Is<Color>()
            ? Color(recipe_.GetValue(q)) : fallback;
    }

    void UpdateVisible()
    {
        String mode = RecipeText("mode", "None");
        int count = mode == "QuadGradient" ? 4 : mode == "Solid" ? 1 : 0;
        for(int i = 0; i < 4; i++) {
            swatches_[i].Show(i < count);
            String key = mode == "Solid" ? "solid" : Key(i);
            swatches_[i].SetColor(
                RecipeColor(key, Color(128, 128, 128)), mixed_);
        }
        Layout();
    }

    void ReadPicker(UiColorPicker& picker, int count)
    {
        for(int i = 0; i < count; i++) {
            String key = count == 1 ? "solid" : Key(i);
            recipe_.Set(key, picker.GetSlotColor(i));
        }
        UpdateVisible();
    }

    void OpenColorDialog(int index)
    {
        String mode = RecipeText("mode", "None");
        int count = mode == "QuadGradient" ? 4 : mode == "Solid" ? 1 : 0;
        if(!enabled_ || mixed_ || index < 0 || index >= count)
            return;

        ValueMap original = recipe_;
        bool accepted = false;

        class ColorDialog : public TopWindow {
        public:
            UiColorPicker picker;

            explicit ColorDialog(int count)
            {
                Title(count == 1 ? "Fill colour" : "Gradient colours");
                Sizeable().Zoomable();
                SetRect(0, 0, DPI(720), DPI(520));
                Add(picker.SizePos());
                picker.SetSlotCount(count);
                picker.SetAlphaEnabled(false);
            }
        } dlg(count);

        for(int i = 0; i < count; i++) {
            String key = count == 1 ? "solid" : Key(i);
            dlg.picker.SetSlotColor(
                i, RecipeColor(key, Color(128, 128, 128)), false);
        }
        dlg.picker.SetActiveSlot(index);
        dlg.picker.WhenChanging = [&] {
            ReadPicker(dlg.picker, count);
            WhenPreview(recipe_);
        };
        dlg.picker.WhenAccept = [&] {
            ReadPicker(dlg.picker, count);
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
    PropertyLiveColorSwatch swatches_[4];
    ValueMap recipe_;
    bool enabled_ = true;
    bool mixed_ = false;
    bool syncing_ = false;
};

One<PropertyValueEditor> CreatePropertyLiveNumericIntEditor()
{
    return MakeOne<PropertyLiveNumericIntEditor>();
}

One<PropertyValueEditor> CreatePropertyLiveFillRecipeEditor()
{
    return MakeOne<PropertyLiveFillRecipeEditor>();
}

}
