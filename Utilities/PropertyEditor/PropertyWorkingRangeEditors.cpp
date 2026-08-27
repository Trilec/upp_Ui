#include "PropertyWorkingRangeEditors.h"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <Ui/UiIcons.h>
#include <Ui/UiTheme.h>

namespace Upp {

static bool ParseWorkingRangeInteger(const String& text, int& out)
{
    const String value = TrimBoth(text);
    if(value.IsEmpty())
        return false;
    errno = 0;
    char *end = nullptr;
    const long parsed = std::strtol(~value, &end, 10);
    if(errno != 0 || end == ~value)
        return false;
    while(end && *end && (byte)*end <= ' ')
        ++end;
    if(end && *end)
        return false;
    out = (int)minmax(parsed, (long)INT_MIN, (long)INT_MAX);
    return true;
}

const char *PropertyEditorWorkingRangeIntId()
{
    return "property.numeric-int-working-range";
}

String PropertyEditorWorkingRangeVariant(int minimum, int maximum)
{
    return Format("%d:%d", minimum, maximum);
}

bool PropertyEditorParseWorkingRangeVariant(const String& variant,
                                            int& minimum, int& maximum)
{
    const Vector<String> parts = Split(variant, ':');
    int lo = 0;
    int hi = 0;
    if(parts.GetCount() != 2 ||
       !ParseWorkingRangeInteger(parts[0], lo) ||
       !ParseWorkingRangeInteger(parts[1], hi) || hi <= lo)
        return false;
    minimum = lo;
    maximum = hi;
    return true;
}

class PropertyWorkingRangeIntEditor : public PropertyValueEditor {
public:
    PropertyWorkingRangeIntEditor()
    {
        Add(edit_);
        Add(slider_);
        Add(toggle_);

        edit_.SetTextAlign(UiAlign::RIGHT);
        slider_.SetCustomStyle(UiTheme::ResolveSlider());
        slider_.ExpandTrack();
        toggle_.SetText("")
               .SetIcon(ICON_DESIGN_SLIDERS_48())
               .SetIconSize(DPI(16), DPI(16))
               .SetIconRenderMode(UiIconRenderMode::MonoTint)
               .SetContentInset(DPI(1))
               .SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
        toggle_.Tip("Switch between numeric entry and the working-range slider");

        toggle_.WhenAction = [=] {
            slider_mode_ = !slider_mode_;
            if(slider_mode_)
                SyncSliderFromEdit();
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
            if(syncing_)
                return;
            edit_.SetCommitBaseline(edit_.GetData());
            WhenCommit(edit_.GetData());
        };
        edit_.WhenChange = [=] {
            if(syncing_)
                return;
            SyncSliderFromEdit();
            WhenPreview(edit_.GetData());
        };
        edit_.WhenCommit = [=] {
            if(syncing_)
                return;
            SyncSliderFromEdit();
            WhenCommit(edit_.GetData());
        };
    }

    void Configure(const PropertyEditorItem& item) override
    {
        enabled_ = item.enabled && !item.read_only;
        legal_minimum_ = IsNumber(item.minimum) ? (int)item.minimum : INT_MIN;
        legal_maximum_ = IsNumber(item.maximum) ? (int)item.maximum : INT_MAX;
        step_ = IsNumber(item.step) ? max(1, (int)item.step) : 1;

        int working_minimum = 0;
        int working_maximum = 0;
        working_valid_ = PropertyEditorParseWorkingRangeVariant(
            item.editor_variant, working_minimum, working_maximum);
        if(working_valid_) {
            working_minimum_ = max(working_minimum, legal_minimum_);
            working_maximum_ = min(working_maximum, legal_maximum_);
            working_valid_ = working_maximum_ > working_minimum_;
        }
        if(!working_valid_ && legal_maximum_ > legal_minimum_ &&
           legal_minimum_ != INT_MIN && legal_maximum_ != INT_MAX) {
            working_minimum_ = legal_minimum_;
            working_maximum_ = legal_maximum_;
            working_valid_ = true;
        }

        edit_.SetPlaceholder(item.mixed ? "<mixed>" :
                             item.inherited ? "<inherited>" : "");
        edit_.Min(legal_minimum_).Max(legal_maximum_).Step(step_);
        edit_.Enable(enabled_);

        if(working_valid_) {
            slider_.SetRange(working_minimum_, working_maximum_);
            slider_.SetStep(step_);
            slider_.Tip(Format("Working range %d–%d. Switch to numeric entry for values outside this range.",
                               working_minimum_, working_maximum_));
        }
        slider_.Enable(enabled_ && working_valid_);
        toggle_.Show(working_valid_);
        toggle_.Enable(enabled_);
        slider_mode_ = slider_mode_ && working_valid_;
        UpdateVisible();
    }

    void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        if(mixed || IsNull(value))
            edit_.SetData(String());
        else
            edit_.SetValue((int)value);
        SyncSliderFromEdit();
        edit_.SetCommitBaseline(edit_.GetData());
        syncing_ = false;
    }

    Value GetEditorValue() const override
    {
        return edit_.GetData();
    }

    void FocusEditor() override
    {
        if(slider_mode_ && working_valid_)
            slider_.SetFocus();
        else {
            edit_.SetFocus();
            edit_.SetSelection();
        }
    }

    void Layout() override
    {
        const int toggle_width = toggle_.IsShown() ? DPI(30) : 0;
        toggle_.SetRect(max(0, GetSize().cx - toggle_width), 0,
                        toggle_width, GetSize().cy);
        const int width = max(0, GetSize().cx - toggle_width - DPI(4));
        edit_.SetRect(0, 0, width, GetSize().cy);
        slider_.SetRect(0, 0, width, GetSize().cy);
    }

private:
    class CommitIntEdit : public UiIntEdit {
    public:
        typedef CommitIntEdit CLASSNAME;

        CommitIntEdit()
        {
            UiIntEdit::WhenAction = [=] { EmitCommit(); };
        }

        void LostFocus() override
        {
            UiIntEdit::LostFocus();
            EmitCommit();
        }

        Event<> WhenCommit;

        void SetCommitBaseline(const Value& value)
        {
            baseline_ = value;
            has_baseline_ = true;
        }

    private:
        void EmitCommit()
        {
            const Value value = GetData();
            if(!has_baseline_ || value != baseline_) {
                baseline_ = value;
                has_baseline_ = true;
                WhenCommit();
            }
        }

        Value baseline_;
        bool has_baseline_ = false;
    };

    void SyncSliderFromEdit()
    {
        if(!working_valid_)
            return;
        const Value value = edit_.GetData();
        if(!IsNumber(value))
            return;
        const int authored = (int)value;
        slider_.SetValue(minmax(authored, working_minimum_, working_maximum_));
    }

    void UpdateVisible()
    {
        edit_.Show(!slider_mode_ || !working_valid_);
        slider_.Show(slider_mode_ && working_valid_);
        Layout();
    }

    void ActionIconsChanged() override
    {
        if(!action_icons_.numeric_slider.IsEmpty())
            toggle_.SetIcon(action_icons_.numeric_slider);
        toggle_.SetIconSize(action_icons_.size, action_icons_.size);
    }

    CommitIntEdit edit_;
    UiSlider slider_;
    UiButton toggle_;
    bool syncing_ = false;
    bool enabled_ = true;
    bool slider_mode_ = false;
    bool working_valid_ = false;
    int legal_minimum_ = INT_MIN;
    int legal_maximum_ = INT_MAX;
    int working_minimum_ = 0;
    int working_maximum_ = 100;
    int step_ = 1;
};

void RegisterPropertyEditorWorkingRangeEditors(PropertyEditorFactory& factory)
{
    factory.RegisterCustom(PropertyEditorWorkingRangeIntId(), [] {
        return One<PropertyValueEditor>(new PropertyWorkingRangeIntEditor);
    });
}

}
