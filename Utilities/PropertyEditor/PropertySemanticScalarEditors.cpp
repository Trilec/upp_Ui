#include "PropertySemanticEditorsInternal.h"

#include <cmath>
#include <cfloat>
#include <Ui/UiDateTime.h>

namespace Upp {

static String PeJoinStrings(const Vector<String>& values, const char *separator)
{
    String out;
    for(int i = 0; i < values.GetCount(); i++) {
        if(i)
            out << separator;
        out << values[i];
    }
    return out;
}

static String PeCanonicalKeyChord(const String& source)
{
    Vector<String> parts = Split(source, '+');
    bool ctrl = false;
    bool alt = false;
    bool shift = false;
    bool meta = false;
    String key;

    for(String part : parts) {
        part = TrimBoth(part);
        String lower = ToLower(part);
        if(lower == "ctrl" || lower == "control")
            ctrl = true;
        else if(lower == "alt" || lower == "option")
            alt = true;
        else if(lower == "shift")
            shift = true;
        else if(lower == "meta" || lower == "cmd" || lower == "command" || lower == "win")
            meta = true;
        else if(!part.IsEmpty())
            key = part;
    }

    if(key.GetCount() == 1)
        key = ToUpper(key);
    else if(!key.IsEmpty()) {
        String lower = ToLower(key);
        if(lower == "space") key = "Space";
        else if(lower == "enter" || lower == "return") key = "Enter";
        else if(lower == "escape" || lower == "esc") key = "Escape";
        else if(lower == "tab") key = "Tab";
        else if(lower == "backspace") key = "Backspace";
        else if(lower == "delete" || lower == "del") key = "Delete";
        else if(lower == "left") key = "Left";
        else if(lower == "right") key = "Right";
        else if(lower == "up") key = "Up";
        else if(lower == "down") key = "Down";
        else if(lower[0] == 'f') {
            int n = StrInt(lower.Mid(1));
            if(n >= 1 && n <= 24)
                key = "F" + AsString(n);
        }
    }

    Vector<String> output;
    if(ctrl) output.Add("Ctrl");
    if(alt) output.Add("Alt");
    if(shift) output.Add("Shift");
    if(meta) output.Add("Meta");
    if(!key.IsEmpty()) output.Add(key);
    return PeJoinStrings(output, "+");
}

class PropertyDateTimeValueEditor : public PropertyValueEditor {
public:
    PropertyDateTimeValueEditor()
    {
        Add(edit_.SizePos());
        edit_.WhenChanging = [=] {
            if(!syncing_)
                WhenPreview(GetEditorValue());
        };
        edit_.WhenAction = [=] {
            if(!syncing_)
                WhenCommit(GetEditorValue());
        };
    }

    void Configure(const PropertyEditorItem& item) override
    {
        variant_ = item.editor_variant;
        const bool seconds = variant_.Find(".seconds") >= 0;
        if(variant_.StartsWith("date-only"))
            edit_.DateMode();
        else if(variant_.StartsWith("time-only"))
            edit_.TimeMode();
        else
            edit_.DateTimeMode();
        edit_.ShowSeconds(seconds);
        edit_.AllowNull(item.allow_null);
        edit_.SetEditable(item.enabled && item.value_editable && !item.read_only);
    }

    void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        if(mixed || IsNull(value))
            edit_.ClearValue(false);
        else if(variant_.StartsWith("date-only")) {
            if(value.Is<Date>())
                edit_.SetDate((Date)value, false);
            else if(value.Is<Time>()) {
                Time t = value;
                edit_.SetDate(Date(t.year, t.month, t.day), false);
            }
        }
        else if(value.Is<Time>())
            edit_.SetValue((Time)value, false);
        else if(value.Is<Date>())
            edit_.SetDate((Date)value, false);
        syncing_ = false;
    }

    Value GetEditorValue() const override
    {
        if(edit_.IsNullValue())
            return Value();
        return variant_.StartsWith("date-only") ? Value(edit_.GetDate())
                                                 : Value(edit_.GetValue());
    }

    void FocusEditor() override
    {
        edit_.SetFocus();
    }

private:
    UiDateTime edit_;
    String variant_;
    bool syncing_ = false;
};

class PropertyDurationValueEditor : public PropertyValueEditor {
public:
    PropertyDurationValueEditor()
    {
        Add(amount_);
        Add(unit_);
        unit_.UseInternalModel().Clear()
             .Add("ms", 0.001)
             .Add("s", 1.0)
             .Add("min", 60.0)
             .Add("h", 3600.0);
        unit_.SelectByData(1.0);
        amount_.SetTextAlign(UiAlign::RIGHT);

        amount_.WhenChange = [=] {
            if(!syncing_)
                WhenPreview(GetEditorValue());
        };
        amount_.WhenAction = [=] {
            if(!syncing_)
                WhenCommit(GetEditorValue());
        };
        unit_.WhenAction = [=] {
            if(syncing_)
                return;
            const double seconds = value_seconds_;
            syncing_ = true;
            SyncAmount(seconds);
            syncing_ = false;
            WhenPreview(seconds);
            WhenCommit(seconds);
        };
    }

    void Configure(const PropertyEditorItem& item) override
    {
        minimum_ = IsNumber(item.minimum) ? (double)item.minimum : -DBL_MAX;
        maximum_ = IsNumber(item.maximum) ? (double)item.maximum : DBL_MAX;
        if(maximum_ < minimum_)
            Swap(maximum_, minimum_);
        step_ = IsNumber(item.step) ? max(0.0, (double)item.step) : 0.0;
        const bool enabled = item.enabled && item.value_editable && !item.read_only;
        amount_.Enable(enabled);
        unit_.Enable(enabled);
    }

    void SetEditorValue(const Value& value, bool mixed) override
    {
        if(mixed || !IsNumber(value))
            return;

        value_seconds_ = minmax((double)value, minimum_, maximum_);
        syncing_ = true;
        const double absolute = fabs(value_seconds_);
        if(absolute >= 3600.0 && fmod(absolute, 3600.0) < 1e-9)
            unit_.SelectByData(3600.0);
        else if(absolute >= 60.0 && fmod(absolute, 60.0) < 1e-9)
            unit_.SelectByData(60.0);
        else if(absolute > 0.0 && absolute < 1.0)
            unit_.SelectByData(0.001);
        else
            unit_.SelectByData(1.0);
        SyncAmount(value_seconds_);
        syncing_ = false;
    }

    Value GetEditorValue() const override
    {
        double unit = (double)unit_.GetSelectedData();
        if(unit <= 0.0)
            unit = 1.0;
        double seconds = amount_.GetValue() * unit;
        if(step_ > 0.0)
            seconds = minimum_ + floor((seconds - minimum_) / step_ + 0.5) * step_;
        value_seconds_ = minmax(seconds, minimum_, maximum_);
        return value_seconds_;
    }

    void FocusEditor() override
    {
        amount_.SetFocus();
        amount_.SetSelection();
    }

    void Layout() override
    {
        const int gap = DPI(4);
        const int unit_width = min(DPI(68), GetSize().cx / 3);
        amount_.SetRect(0, 0, max(0, GetSize().cx - unit_width - gap), GetSize().cy);
        unit_.SetRect(max(0, GetSize().cx - unit_width), 0, unit_width, GetSize().cy);
    }

private:
    void SyncAmount(double seconds)
    {
        double unit = (double)unit_.GetSelectedData();
        if(unit <= 0.0)
            unit = 1.0;
        amount_.SetValue(seconds / unit);
        value_seconds_ = seconds;
    }

    UiFloatEdit amount_;
    UiDropdown unit_;
    mutable double value_seconds_ = 0.0;
    double minimum_ = 0.0;
    double maximum_ = 86400.0;
    double step_ = 0.001;
    bool syncing_ = false;
};

class PropertyGeometryValueEditor : public PropertyValueEditor {
public:
    PropertyGeometryValueEditor()
    {
        for(int i = 0; i < 4; i++) {
            Add(field_[i]);
            const int index = i;
            field_[i].SetTextAlign(UiAlign::RIGHT);
            field_[i].WhenChange = [=] { FieldChanged(index, false); };
            field_[i].WhenAction = [=] { FieldChanged(index, true); };
        }
        Add(link_);
        link_.SetText("Link").SetCheckable();
        link_.WhenAction = [=] {
            linked_ = link_.IsChecked();
            if(linked_)
                Propagate(0);
            Value value = GetEditorValue();
            WhenPreview(value);
            WhenCommit(value);
        };
    }

    void Configure(const PropertyEditorItem& item) override
    {
        variant_ = item.editor_variant;
        linked_ = variant_.EndsWith(".linked");
        link_.SetChecked(linked_);
        count_ = variant_.StartsWith("point") || variant_.StartsWith("size") ? 2 : 4;
        linkable_ = variant_.StartsWith("insets") || variant_.StartsWith("corners");

        static const char *point_tips[] = {"X", "Y", "", ""};
        static const char *size_tips[] = {"Width", "Height", "", ""};
        static const char *rect_tips[] = {"X", "Y", "Width", "Height"};
        static const char *inset_tips[] = {"Left", "Top", "Right", "Bottom"};
        static const char *corner_tips[] = {"Top left", "Top right", "Bottom right", "Bottom left"};
        const char **tips = variant_.StartsWith("point") ? point_tips :
                            variant_.StartsWith("size") ? size_tips :
                            variant_.StartsWith("rect") ? rect_tips :
                            variant_.StartsWith("corners") ? corner_tips : inset_tips;

        const bool enabled = item.enabled && item.value_editable && !item.read_only;
        for(int i = 0; i < 4; i++) {
            field_[i].Show(i < count_);
            field_[i].Enable(enabled);
            if(i < count_)
                field_[i].Tip(tips[i]);
        }
        link_.Show(linkable_);
        link_.Enable(enabled);
    }

    void SetEditorValue(const Value& value, bool mixed) override
    {
        if(mixed || !value.Is<ValueArray>())
            return;
        ValueArray values = value;
        if(values.GetCount() != count_)
            return;
        syncing_ = true;
        for(int i = 0; i < count_; i++)
            if(IsNumber(values[i]))
                field_[i].SetValue((double)values[i]);
        syncing_ = false;
    }

    Value GetEditorValue() const override
    {
        ValueArray out;
        for(int i = 0; i < count_; i++)
            out.Add(field_[i].GetValue());
        return out;
    }

    void FocusEditor() override
    {
        field_[0].SetFocus();
        field_[0].SetSelection();
    }

    void Layout() override
    {
        const int gap = DPI(3);
        const int link_width = linkable_ ? DPI(42) : 0;
        const int usable = max(0, GetSize().cx - link_width - (linkable_ ? gap : 0));
        const int width = count_ ? max(1, (usable - gap * (count_ - 1)) / count_) : usable;
        int x = 0;
        for(int i = 0; i < count_; i++) {
            int width_i = i + 1 == count_ ? max(0, usable - x) : width;
            field_[i].SetRect(x, 0, width_i, GetSize().cy);
            x += width_i + gap;
        }
        if(linkable_)
            link_.SetRect(max(0, GetSize().cx - link_width), 0, link_width, GetSize().cy);
    }

private:
    void Propagate(int source)
    {
        if(!linked_ || source < 0 || source >= count_)
            return;
        syncing_ = true;
        const double value = field_[source].GetValue();
        for(int i = 0; i < count_; i++)
            if(i != source)
                field_[i].SetValue(value);
        syncing_ = false;
    }

    void FieldChanged(int source, bool commit)
    {
        if(syncing_)
            return;
        Propagate(source);
        Value value = GetEditorValue();
        WhenPreview(value);
        if(commit)
            WhenCommit(value);
    }

    UiFloatEdit field_[4];
    UiButton link_;
    String variant_;
    int count_ = 2;
    bool linkable_ = false;
    bool linked_ = false;
    bool syncing_ = false;
};

class PropertyKeyChordValueEditor : public PropertyValueEditor {
public:
    PropertyKeyChordValueEditor()
    {
        Add(edit_.SizePos());
        edit_.WhenChange = [=] {
            if(!syncing_)
                WhenPreview(PeCanonicalKeyChord(edit_.GetTextUtf8()));
        };
        edit_.WhenAction = [=] {
            if(syncing_)
                return;
            String value = PeCanonicalKeyChord(edit_.GetTextUtf8());
            syncing_ = true;
            edit_.SetTextUtf8(value);
            syncing_ = false;
            WhenCommit(value);
        };
    }

    void Configure(const PropertyEditorItem& item) override
    {
        edit_.Enable(item.enabled && item.value_editable && !item.read_only);
        edit_.SetPlaceholder("Ctrl+Shift+S");
    }

    void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        edit_.SetTextUtf8(mixed ? String() : PeCanonicalKeyChord(AsString(value)));
        syncing_ = false;
    }

    Value GetEditorValue() const override
    {
        return PeCanonicalKeyChord(edit_.GetTextUtf8());
    }

    void FocusEditor() override
    {
        edit_.SetFocus();
        edit_.SetSelection();
    }

private:
    UiLineEdit edit_;
    bool syncing_ = false;
};

class PropertyOptionalValueEditor : public PropertyValueEditor {
public:
    PropertyOptionalValueEditor()
    {
        Add(set_);
        Add(text_);
        Add(integer_);
        Add(number_);
        set_.SetText("Set");
        set_.WhenAction = [=] { ToggleSet(); };
        text_.WhenChange = [=] { Changed(false); };
        text_.WhenAction = [=] { Changed(true); };
        integer_.WhenChange = [=] { Changed(false); };
        integer_.WhenAction = [=] { Changed(true); };
        number_.WhenChange = [=] { Changed(false); };
        number_.WhenAction = [=] { Changed(true); };
    }

    void Configure(const PropertyEditorItem& item) override
    {
        variant_ = item.editor_variant;
        fallback_ = item.default_value;
        enabled_ = item.enabled && item.value_editable && !item.read_only;
        set_.Enable(enabled_);
        UpdateVisible();
    }

    void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        is_set_ = !mixed && !IsNull(value);
        set_.SetData(is_set_);
        Value shown = is_set_ ? value : fallback_;
        if(variant_ == "int")
            integer_.SetData(shown);
        else if(variant_ == "double")
            number_.SetData(shown);
        else
            text_.SetTextUtf8(AsString(shown));
        syncing_ = false;
        UpdateVisible();
    }

    Value GetEditorValue() const override
    {
        if(!is_set_)
            return Value();
        if(variant_ == "int")
            return integer_.GetData();
        if(variant_ == "double")
            return number_.GetData();
        return text_.GetData();
    }

    void FocusEditor() override
    {
        if(!is_set_)
            set_.SetFocus();
        else if(variant_ == "int")
            integer_.SetFocus();
        else if(variant_ == "double")
            number_.SetFocus();
        else
            text_.SetFocus();
    }

    void Layout() override
    {
        const int check = min(DPI(48), GetSize().cx / 3);
        set_.SetRect(0, 0, check, GetSize().cy);
        Rect value = RectC(check + DPI(4), 0,
                           max(0, GetSize().cx - check - DPI(4)), GetSize().cy);
        text_.SetRect(value);
        integer_.SetRect(value);
        number_.SetRect(value);
    }

private:
    void ToggleSet()
    {
        if(syncing_)
            return;
        is_set_ = (bool)set_.GetData();
        UpdateVisible();
        Value value = GetEditorValue();
        WhenPreview(value);
        WhenCommit(value);
    }

    void Changed(bool commit)
    {
        if(syncing_ || !is_set_)
            return;
        Value value = GetEditorValue();
        WhenPreview(value);
        if(commit)
            WhenCommit(value);
    }

    void UpdateVisible()
    {
        const bool show_text = is_set_ && variant_ != "int" && variant_ != "double";
        text_.Show(show_text);
        integer_.Show(is_set_ && variant_ == "int");
        number_.Show(is_set_ && variant_ == "double");
        text_.Enable(enabled_);
        integer_.Enable(enabled_);
        number_.Enable(enabled_);
        Layout();
    }

    UiCheckBox set_;
    UiLineEdit text_;
    UiIntEdit integer_;
    UiFloatEdit number_;
    String variant_ = "text";
    Value fallback_;
    bool is_set_ = false;
    bool enabled_ = true;
    bool syncing_ = false;
};

void RegisterPropertyEditorSemanticScalarEditors(PropertyEditorFactory& factory)
{
    if(!factory.HasCustom(PropertyEditorDateTimeId()))
        factory.RegisterCustom(PropertyEditorDateTimeId(), [] {
            return One<PropertyValueEditor>(new PropertyDateTimeValueEditor);
        });
    if(!factory.HasCustom(PropertyEditorDurationId()))
        factory.RegisterCustom(PropertyEditorDurationId(), [] {
            return One<PropertyValueEditor>(new PropertyDurationValueEditor);
        });
    if(!factory.HasCustom(PropertyEditorGeometryId()))
        factory.RegisterCustom(PropertyEditorGeometryId(), [] {
            return One<PropertyValueEditor>(new PropertyGeometryValueEditor);
        });
    if(!factory.HasCustom(PropertyEditorKeyChordId()))
        factory.RegisterCustom(PropertyEditorKeyChordId(), [] {
            return One<PropertyValueEditor>(new PropertyKeyChordValueEditor);
        });
    if(!factory.HasCustom(PropertyEditorOptionalId()))
        factory.RegisterCustom(PropertyEditorOptionalId(), [] {
            return One<PropertyValueEditor>(new PropertyOptionalValueEditor);
        });
}

static PropertyEditorItem& PeAddDateTime(PropertyEditorModel& model,
                                         const String& id, const String& label,
                                         const Value& value,
                                         const String& variant,
                                         const String& group)
{
    PropertyEditorItem& item = model.Add(id, label, PropertyEditorKind::Custom, value, group);
    item.custom_editor = PropertyEditorDateTimeId();
    item.editor_variant = variant;
    item.inline_editor = true;
    item.row_span = 1;
    item.allow_null = true;
    return item;
}

PropertyEditorItem& AddPropertyDate(PropertyEditorModel& model,
                                    const String& id, const String& label,
                                    Date value, const String& group)
{
    PropertyEditorItem& item = PeAddDateTime(model, id, label, value, "date-only", group);
    item.normalize = [](const Value& candidate) {
        if(IsNull(candidate) || candidate.Is<Date>())
            return candidate;
        if(candidate.Is<Time>()) {
            Time time = candidate;
            return Value(Date(time.year, time.month, time.day));
        }
        return candidate;
    };
    item.validate = [](const Value& candidate) {
        return IsNull(candidate) || candidate.Is<Date>()
             ? String() : String("Expected a Date value");
    };
    return item;
}

PropertyEditorItem& AddPropertyTime(PropertyEditorModel& model,
                                    const String& id, const String& label,
                                    Time value, bool show_seconds,
                                    const String& group)
{
    if(!IsNull(value))
        value = Time(1970, 1, 1, value.hour, value.minute,
                     show_seconds ? value.second : 0);
    PropertyEditorItem& item = PeAddDateTime(
        model, id, label, value,
        show_seconds ? "time-only.seconds" : "time-only", group);
    item.normalize = [=](const Value& candidate) {
        if(IsNull(candidate) || !candidate.Is<Time>())
            return candidate;
        Time time = candidate;
        return Value(Time(1970, 1, 1, time.hour, time.minute,
                          show_seconds ? time.second : 0));
    };
    item.validate = [](const Value& candidate) {
        return IsNull(candidate) || candidate.Is<Time>()
             ? String() : String("Expected a Time value");
    };
    return item;
}

PropertyEditorItem& AddPropertyDateTime(PropertyEditorModel& model,
                                        const String& id, const String& label,
                                        Time value, bool show_seconds,
                                        const String& group)
{
    if(!IsNull(value) && !show_seconds)
        value.second = 0;
    PropertyEditorItem& item = PeAddDateTime(
        model, id, label, value,
        show_seconds ? "date-time.seconds" : "date-time", group);
    item.normalize = [=](const Value& candidate) {
        if(IsNull(candidate) || !candidate.Is<Time>())
            return candidate;
        Time time = candidate;
        if(!show_seconds)
            time.second = 0;
        return Value(time);
    };
    item.validate = [](const Value& candidate) {
        return IsNull(candidate) || candidate.Is<Time>()
             ? String() : String("Expected a Time value");
    };
    return item;
}

PropertyEditorItem& AddPropertyDuration(PropertyEditorModel& model,
                                        const String& id, const String& label,
                                        double seconds,
                                        double minimum_seconds,
                                        double maximum_seconds,
                                        double step_seconds,
                                        const String& group)
{
    if(maximum_seconds < minimum_seconds)
        Swap(maximum_seconds, minimum_seconds);
    seconds = minmax(seconds, minimum_seconds, maximum_seconds);

    PropertyEditorItem& item = model.Add(
        id, label, PropertyEditorKind::Custom, seconds, group);
    item.custom_editor = PropertyEditorDurationId();
    item.minimum = minimum_seconds;
    item.maximum = maximum_seconds;
    item.step = max(0.0, step_seconds);
    item.inline_editor = true;
    item.row_span = 1;
    item.unit = "s";
    item.normalize = [=](const Value& candidate) {
        if(!IsNumber(candidate))
            return candidate;
        double value = minmax((double)candidate,
                              minimum_seconds, maximum_seconds);
        if(step_seconds > 0.0)
            value = minimum_seconds +
                    floor((value - minimum_seconds) / step_seconds + 0.5) * step_seconds;
        return Value(minmax(value, minimum_seconds, maximum_seconds));
    };
    item.validate = [](const Value& candidate) {
        return IsNumber(candidate)
             ? String() : String("Expected duration seconds");
    };
    return item;
}

static PropertyEditorItem& PeAddGeometry(PropertyEditorModel& model,
                                         const String& id, const String& label,
                                         const ValueArray& values,
                                         const String& variant,
                                         const String& group)
{
    PropertyEditorItem& item = model.Add(
        id, label, PropertyEditorKind::Custom, values, group);
    item.custom_editor = PropertyEditorGeometryId();
    item.editor_variant = variant;
    item.inline_editor = true;
    item.row_span = 1;
    const int count = values.GetCount();
    item.normalize = [=](const Value& candidate) {
        if(!candidate.Is<ValueArray>())
            return candidate;
        ValueArray array = candidate;
        if(array.GetCount() != count)
            return candidate;
        for(int i = 0; i < count; i++)
            if(!IsNumber(array[i]))
                return candidate;
        return Value(array);
    };
    item.validate = [=](const Value& candidate) {
        if(!candidate.Is<ValueArray>())
            return String("Expected a geometry value array");
        ValueArray array = candidate;
        if(array.GetCount() != count)
            return Format("Expected %d geometry components", count);
        for(const Value& part : array)
            if(!IsNumber(part))
                return String("Geometry components must be numeric");
        return String();
    };
    return item;
}

PropertyEditorItem& AddPropertyPoint(PropertyEditorModel& model,
                                     const String& id, const String& label,
                                     double x, double y, const String& group)
{
    ValueArray values;
    values.Add(x);
    values.Add(y);
    return PeAddGeometry(model, id, label, values, "point", group);
}

PropertyEditorItem& AddPropertySize(PropertyEditorModel& model,
                                    const String& id, const String& label,
                                    double cx, double cy, const String& group)
{
    ValueArray values;
    values.Add(cx);
    values.Add(cy);
    return PeAddGeometry(model, id, label, values, "size", group);
}

PropertyEditorItem& AddPropertyRect(PropertyEditorModel& model,
                                    const String& id, const String& label,
                                    double x, double y, double cx, double cy,
                                    const String& group)
{
    ValueArray values;
    values.Add(x);
    values.Add(y);
    values.Add(cx);
    values.Add(cy);
    return PeAddGeometry(model, id, label, values, "rect", group);
}

PropertyEditorItem& AddPropertyInsets(PropertyEditorModel& model,
                                      const String& id, const String& label,
                                      double left, double top,
                                      double right, double bottom,
                                      bool linked, const String& group)
{
    ValueArray values;
    values.Add(left);
    values.Add(top);
    values.Add(right);
    values.Add(bottom);
    return PeAddGeometry(model, id, label, values,
                         linked ? "insets.linked" : "insets", group);
}

PropertyEditorItem& AddPropertyCorners(PropertyEditorModel& model,
                                       const String& id, const String& label,
                                       double top_left, double top_right,
                                       double bottom_right, double bottom_left,
                                       bool linked, const String& group)
{
    ValueArray values;
    values.Add(top_left);
    values.Add(top_right);
    values.Add(bottom_right);
    values.Add(bottom_left);
    return PeAddGeometry(model, id, label, values,
                         linked ? "corners.linked" : "corners", group);
}

PropertyEditorItem& AddPropertyKeyChord(PropertyEditorModel& model,
                                        const String& id, const String& label,
                                        const String& chord,
                                        const String& group)
{
    PropertyEditorItem& item = model.Add(
        id, label, PropertyEditorKind::Custom,
        PeCanonicalKeyChord(chord), group);
    item.custom_editor = PropertyEditorKeyChordId();
    item.inline_editor = true;
    item.row_span = 1;
    item.normalize = [](const Value& candidate) {
        return Value(PeCanonicalKeyChord(AsString(candidate)));
    };
    item.validate = [](const Value& candidate) {
        return PeCanonicalKeyChord(AsString(candidate)).IsEmpty()
             ? String("Enter a key chord such as Ctrl+S") : String();
    };
    return item;
}

PropertyEditorItem& AddPropertyOptional(PropertyEditorModel& model,
                                        const String& id, const String& label,
                                        const Value& value,
                                        const Value& fallback,
                                        const String& variant,
                                        const String& group)
{
    String use_variant = variant == "int" || variant == "double"
                       ? variant : "text";
    PropertyEditorItem& item = model.Add(
        id, label, PropertyEditorKind::Custom, value, group);
    item.custom_editor = PropertyEditorOptionalId();
    item.editor_variant = use_variant;
    item.allow_null = true;
    item.inline_editor = true;
    item.row_span = 1;
    item.SetDefault(fallback);
    item.normalize = [=](const Value& candidate) {
        if(IsNull(candidate))
            return Value();
        if(use_variant == "int")
            return IsNumber(candidate) ? Value((int)candidate) : candidate;
        if(use_variant == "double")
            return IsNumber(candidate) ? Value((double)candidate) : candidate;
        return Value(AsString(candidate));
    };
    item.validate = [=](const Value& candidate) {
        if(IsNull(candidate) || use_variant == "text")
            return String();
        return IsNumber(candidate)
             ? String() : String("Expected a numeric optional value");
    };
    return item;
}

} // namespace Upp
