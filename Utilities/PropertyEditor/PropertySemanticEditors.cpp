#include "PropertyValueEditors.h"

#include <Ui/UiColorPicker/UiColorPicker.h>
#include <Ui/UiDateTime.h>
#include <Ui/UiDataModels.h>
#include <Ui/UiList.h>

namespace Upp {

const char *PropertyEditorDateTimeId()   { return "property.datetime"; }
const char *PropertyEditorDurationId()   { return "property.duration"; }
const char *PropertyEditorGeometryId()   { return "property.geometry"; }
const char *PropertyEditorFlagsId()      { return "property.flags"; }
const char *PropertyEditorStringListId() { return "property.string-list"; }
const char *PropertyEditorGradientId()   { return "property.gradient"; }
const char *PropertyEditorKeyChordId()   { return "property.key-chord"; }
const char *PropertyEditorReferenceId()  { return "property.reference"; }
const char *PropertyEditorOptionalId()   { return "property.optional"; }

static Value PeMapValue(const ValueMap& map, const char *key, const Value& fallback = Value())
{
    int q = map.Find(key);
    return q >= 0 ? map.GetValue(q) : fallback;
}

static bool PeContainsValue(const ValueArray& values, const Value& value)
{
    for(int i = 0; i < values.GetCount(); i++)
        if(values[i] == value)
            return true;
    return false;
}

static ValueArray PeNumericArray(const Value& value, int count, const ValueArray& fallback)
{
    if(!value.Is<ValueArray>())
        return fallback;
    ValueArray source = value;
    if(source.GetCount() != count)
        return fallback;
    for(int i = 0; i < count; i++)
        if(!IsNumber(source[i]))
            return fallback;
    return source;
}

static String PeJoin(const Vector<String>& values, const char *separator)
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
    bool ctrl = false, alt = false, shift = false, meta = false;
    String key;
    for(String part : parts) {
        part = TrimBoth(part);
        String lower = ToLower(part);
        if(lower == "ctrl" || lower == "control") ctrl = true;
        else if(lower == "alt" || lower == "option") alt = true;
        else if(lower == "shift") shift = true;
        else if(lower == "meta" || lower == "cmd" || lower == "command" || lower == "win") meta = true;
        else if(!part.IsEmpty()) key = part;
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
    Vector<String> out;
    if(ctrl) out.Add("Ctrl");
    if(alt) out.Add("Alt");
    if(shift) out.Add("Shift");
    if(meta) out.Add("Meta");
    if(!key.IsEmpty()) out.Add(key);
    return PeJoin(out, "+");
}

// -----------------------------------------------------------------------------
// Date / Time / DateTime

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

    void FocusEditor() override { edit_.SetFocus(); }

private:
    UiDateTime edit_;
    String variant_;
    bool syncing_ = false;
};

// -----------------------------------------------------------------------------
// Duration. Durable unit is seconds; the unit dropdown is presentation only.

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
            SyncAmount(seconds);
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
        double abs_seconds = fabs(value_seconds_);
        if(abs_seconds >= 3600.0 && fmod(abs_seconds, 3600.0) < 1e-9)
            unit_.SelectByData(3600.0);
        else if(abs_seconds >= 60.0 && fmod(abs_seconds, 60.0) < 1e-9)
            unit_.SelectByData(60.0);
        else if(abs_seconds > 0.0 && abs_seconds < 1.0)
            unit_.SelectByData(0.001);
        else
            unit_.SelectByData(1.0);
        SyncAmount(value_seconds_);
        syncing_ = false;
    }

    Value GetEditorValue() const override
    {
        double unit = (double)unit_.GetSelectedData();
        double seconds = amount_.GetValue() * unit;
        if(step_ > 0.0)
            seconds = minimum_ + floor((seconds - minimum_) / step_ + 0.5) * step_;
        value_seconds_ = minmax(seconds, minimum_, maximum_);
        return value_seconds_;
    }

    void FocusEditor() override { amount_.SetFocus(); amount_.SetSelection(); }

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
        if(unit <= 0.0) unit = 1.0;
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

// -----------------------------------------------------------------------------
// Point / Size / Rect / Insets / Corners

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
            WhenPreview(GetEditorValue());
            WhenCommit(GetEditorValue());
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

    void FocusEditor() override { field_[0].SetFocus(); field_[0].SetSelection(); }

    void Layout() override
    {
        const int gap = DPI(3);
        const int link_width = linkable_ ? DPI(42) : 0;
        const int usable = max(0, GetSize().cx - link_width - (linkable_ ? gap : 0));
        const int width = count_ ? max(1, (usable - gap * (count_ - 1)) / count_) : usable;
        int x = 0;
        for(int i = 0; i < count_; i++) {
            int w = i + 1 == count_ ? max(0, usable - x) : width;
            field_[i].SetRect(x, 0, w, GetSize().cy);
            x += w + gap;
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
        double value = field_[source].GetValue();
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

// -----------------------------------------------------------------------------
// Flags / multi-choice

class PropertyFlagsValueEditor : public PropertyValueEditor {
public:
    PropertyFlagsValueEditor()
    {
        Add(summary_);
        Add(edit_);
        summary_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        summary_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
        edit_.SetText("Edit...");
        edit_.WhenAction = [=] { OpenDialog(); };
        summary_.WhenAction = [=] { OpenDialog(); };
    }

    void Configure(const PropertyEditorItem& item) override
    {
        choices_.Clear();
        for(const PropertyEditorChoice& choice : item.choices)
            choices_.Add(choice);
        enabled_ = item.enabled && item.value_editable && !item.read_only;
        edit_.Enable(enabled_);
        summary_.Enable(enabled_);
        SyncSummary();
    }

    void SetEditorValue(const Value& value, bool mixed) override
    {
        mixed_ = mixed;
        selected_ = value.Is<ValueArray>() ? ValueArray(value) : ValueArray();
        SyncSummary();
    }

    Value GetEditorValue() const override { return selected_; }
    void FocusEditor() override { edit_.SetFocus(); }

    void Layout() override
    {
        const int button = min(DPI(64), GetSize().cx / 3);
        summary_.SetRect(0, 0, max(0, GetSize().cx - button - DPI(4)), GetSize().cy);
        edit_.SetRect(max(0, GetSize().cx - button), 0, button, GetSize().cy);
    }

private:
    void SyncSummary()
    {
        if(mixed_) {
            summary_.SetText("<multiple selections>");
            return;
        }
        Vector<String> labels;
        for(const PropertyEditorChoice& choice : choices_)
            if(PeContainsValue(selected_, choice.value))
                labels.Add(choice.label);
        summary_.SetText(labels.IsEmpty() ? "<none>" : PeJoin(labels, ", "));
    }

    void OpenDialog()
    {
        if(!enabled_)
            return;
        struct Dialog : TopWindow {
            Array<UiCheckBox> boxes;
            UiButton ok, cancel;
            Dialog(const Array<PropertyEditorChoice>& choices, const ValueArray& selected)
            {
                Title("Select flags");
                Sizeable();
                SetRect(0, 0, DPI(420), DPI(max(190, 92 + choices.GetCount() * 32)));
                for(const PropertyEditorChoice& choice : choices) {
                    UiCheckBox& box = boxes.Add();
                    Add(box);
                    box.SetText(choice.label);
                    box.SetData(PeContainsValue(selected, choice.value));
                }
                Add(ok); Add(cancel);
                ok.SetText("OK"); cancel.SetText("Cancel");
                ok.WhenAction = [=] { AcceptBreak(IDOK); };
                cancel.WhenAction = [=] { RejectBreak(IDCANCEL); };
            }
            void Layout() override
            {
                Rect r = GetSize();
                const int pad = DPI(12), row = DPI(28);
                int y = pad;
                for(UiCheckBox& box : boxes) {
                    box.SetRect(pad, y, max(0, r.GetWidth() - 2 * pad), row);
                    y += row + DPI(3);
                }
                cancel.SetRect(r.right - pad - DPI(88), r.bottom - pad - DPI(30), DPI(88), DPI(30));
                ok.SetRect(r.right - pad - DPI(182), r.bottom - pad - DPI(30), DPI(88), DPI(30));
            }
        } dlg(choices_, selected_);
        dlg.CenterOwner();
        if(dlg.Run() != IDOK)
            return;
        ValueArray next;
        for(int i = 0; i < choices_.GetCount(); i++)
            if((bool)dlg.boxes[i].GetData())
                next.Add(choices_[i].value);
        selected_ = next;
        mixed_ = false;
        SyncSummary();
        WhenPreview(selected_);
        WhenCommit(selected_);
    }

    PropertyActionLabel summary_;
    UiButton edit_;
    Array<PropertyEditorChoice> choices_;
    ValueArray selected_;
    bool mixed_ = false;
    bool enabled_ = true;
};

// -----------------------------------------------------------------------------
// Small ordered string collection

class PropertyStringListValueEditor : public PropertyValueEditor {
public:
    PropertyStringListValueEditor()
    {
        Add(summary_);
        Add(edit_);
        summary_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        summary_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
        edit_.SetText("Edit...");
        edit_.WhenAction = [=] { OpenDialog(); };
        summary_.WhenAction = [=] { OpenDialog(); };
    }

    void Configure(const PropertyEditorItem& item) override
    {
        maximum_ = IsNumber(item.maximum) ? max(1, (int)item.maximum) : 32;
        enabled_ = item.enabled && item.value_editable && !item.read_only;
        edit_.Enable(enabled_);
        summary_.Enable(enabled_);
        SyncSummary();
    }

    void SetEditorValue(const Value& value, bool mixed) override
    {
        mixed_ = mixed;
        values_ = value.Is<ValueArray>() ? ValueArray(value) : ValueArray();
        SyncSummary();
    }

    Value GetEditorValue() const override { return values_; }
    void FocusEditor() override { edit_.SetFocus(); }

    void Layout() override
    {
        const int button = min(DPI(64), GetSize().cx / 3);
        summary_.SetRect(0, 0, max(0, GetSize().cx - button - DPI(4)), GetSize().cy);
        edit_.SetRect(max(0, GetSize().cx - button), 0, button, GetSize().cy);
    }

private:
    void SyncSummary()
    {
        summary_.SetText(mixed_ ? "<multiple lists>" : Format("%d item%s", values_.GetCount(), values_.GetCount() == 1 ? "" : "s"));
    }

    void OpenDialog()
    {
        if(!enabled_)
            return;
        struct Dialog : TopWindow {
            UiList list;
            UiListModel model;
            UiLineEdit text;
            UiButton add, remove, up, down, ok, cancel;
            int maximum = 32;

            Dialog(const ValueArray& values, int max_items)
            {
                maximum = max_items;
                Title("Edit ordered values"); Sizeable().Zoomable();
                SetRect(0, 0, DPI(520), DPI(430));
                Add(list); Add(text); Add(add); Add(remove); Add(up); Add(down); Add(ok); Add(cancel);
                for(int i = 0; i < values.GetCount(); i++)
                    model.Add(AsString(values[i]), i);
                list.SetModel(model).SetSelectionMode(UILISTSEL_SINGLE);
                if(model.GetCount()) list.Select(0);
                add.SetText("Add"); remove.SetText("Remove"); up.SetText("Up"); down.SetText("Down");
                ok.SetText("OK"); cancel.SetText("Cancel");
                list.WhenSelection = [=] { SyncText(); };
                text.WhenAction = [=] { Rename(); };
                add.WhenAction = [=] { AddValue(); };
                remove.WhenAction = [=] { RemoveValue(); };
                up.WhenAction = [=] { Move(-1); };
                down.WhenAction = [=] { Move(1); };
                ok.WhenAction = [=] { Rename(); AcceptBreak(IDOK); };
                cancel.WhenAction = [=] { RejectBreak(IDCANCEL); };
                SyncText();
            }

            void Layout() override
            {
                Rect r = GetSize();
                const int pad = DPI(10), gap = DPI(6), h = DPI(30);
                list.SetRect(pad, pad, max(0, r.GetWidth() - 2 * pad), max(0, r.GetHeight() - DPI(116)));
                int y = max(pad, r.bottom - DPI(96));
                text.SetRect(pad, y, max(0, r.GetWidth() - 2 * pad), h); y += h + gap;
                add.SetRect(pad, y, DPI(64), h);
                remove.SetRect(pad + DPI(70), y, DPI(72), h);
                up.SetRect(pad + DPI(148), y, DPI(54), h);
                down.SetRect(pad + DPI(208), y, DPI(58), h);
                cancel.SetRect(r.right - pad - DPI(88), y, DPI(88), h);
                ok.SetRect(r.right - pad - DPI(182), y, DPI(88), h);
            }

            void SyncText()
            {
                int q = list.GetCursor();
                text.SetTextUtf8(q >= 0 && q < model.GetCount() ? model.Get(q).text : String());
            }

            void Rename()
            {
                int q = list.GetCursor();
                if(q < 0 || q >= model.GetCount()) return;
                UiModelItem item = model.Get(q);
                item.text = text.GetTextUtf8();
                model.Set(q, item);
            }

            void AddValue()
            {
                Rename();
                if(model.GetCount() >= maximum) return;
                int q = model.Add(Format("Item %d", model.GetCount() + 1), model.GetCount());
                list.Select(q);
                SyncText();
            }

            void RemoveValue()
            {
                int q = list.GetCursor();
                if(q < 0 || q >= model.GetCount()) return;
                model.Remove(q);
                if(model.GetCount()) list.Select(min(q, model.GetCount() - 1));
                SyncText();
            }

            void Move(int delta)
            {
                Rename();
                int q = list.GetCursor();
                int to = q + delta;
                if(q < 0 || to < 0 || to >= model.GetCount()) return;
                model.SwapItems(q, to);
                list.Select(to);
                SyncText();
            }

            ValueArray GetValues() const
            {
                ValueArray out;
                for(int i = 0; i < model.GetCount(); i++)
                    out.Add(model.Get(i).text);
                return out;
            }
        } dlg(values_, maximum_);
        dlg.CenterOwner();
        if(dlg.Run() != IDOK)
            return;
        values_ = dlg.GetValues();
        mixed_ = false;
        SyncSummary();
        WhenPreview(values_);
        WhenCommit(values_);
    }

    PropertyActionLabel summary_;
    UiButton edit_;
    ValueArray values_;
    int maximum_ = 32;
    bool mixed_ = false;
    bool enabled_ = true;
};

// -----------------------------------------------------------------------------
// Gradient recipe

struct PeGradientStop : Moveable<PeGradientStop> {
    double position = 0.0;
    Color color = Black();
    int alpha = 255;
};

static Value PeGradientStopValue(const PeGradientStop& stop)
{
    ValueMap out;
    out.Set("position", stop.position);
    out.Set("color", stop.color);
    out.Set("alpha", stop.alpha);
    return out;
}

static Value PeNormalizeGradient(const Value& value)
{
    ValueMap source = value.Is<ValueMap>() ? ValueMap(value) : ValueMap();
    String mode = AsString(PeMapValue(source, "mode", "Linear"));
    if(mode != "Radial") mode = "Linear";
    String interpolation = AsString(PeMapValue(source, "interpolation", "Linear"));
    if(interpolation != "Smooth") interpolation = "Linear";
    double angle = IsNumber(PeMapValue(source, "angle")) ? (double)PeMapValue(source, "angle") : 0.0;
    while(angle < 0.0) angle += 360.0;
    while(angle >= 360.0) angle -= 360.0;

    Vector<PeGradientStop> stops;
    Value raw = PeMapValue(source, "stops");
    if(raw.Is<ValueArray>()) {
        ValueArray input = raw;
        for(int i = 0; i < input.GetCount(); i++) {
            if(!input[i].Is<ValueMap>()) continue;
            ValueMap map = input[i];
            PeGradientStop& stop = stops.Add();
            Value p = PeMapValue(map, "position", 0.0);
            stop.position = IsNumber(p) ? minmax((double)p, 0.0, 1.0) : 0.0;
            Value c = PeMapValue(map, "color", Black());
            stop.color = c.Is<Color>() ? Color(c) : Black();
            Value a = PeMapValue(map, "alpha", 255);
            stop.alpha = IsNumber(a) ? minmax((int)a, 0, 255) : 255;
        }
    }
    if(stops.GetCount() < 2) {
        stops.Clear();
        PeGradientStop& a = stops.Add(); a.position = 0.0; a.color = Black();
        PeGradientStop& b = stops.Add(); b.position = 1.0; b.color = White();
    }
    Sort(stops, [](const PeGradientStop& a, const PeGradientStop& b) {
        return a.position < b.position;
    });
    ValueArray stop_values;
    for(const PeGradientStop& stop : stops)
        stop_values.Add(PeGradientStopValue(stop));
    ValueMap out;
    out.Set("mode", mode);
    out.Set("angle", angle);
    out.Set("interpolation", interpolation);
    out.Set("stops", stop_values);
    return out;
}

static bool PePickColor(Color& color, Ctrl *owner)
{
    struct Dialog : TopWindow {
        UiColorPicker picker;
        Dialog(Color color)
        {
            Title("Choose gradient stop colour"); Sizeable().Zoomable();
            SetRect(0, 0, DPI(720), DPI(560));
            Add(picker.SizePos());
            picker.EnableSessionPersistence(false)
                  .SetSlotCount(1)
                  .SetGeneratorCount(1)
                  .SetSlotLabel(0, "Stop")
                  .SetSlotColor(0, color, false)
                  .SetActiveSlot(0);
            picker.WhenAccept = [=] { AcceptBreak(IDOK); };
            picker.WhenCancel = [=] { RejectBreak(IDCANCEL); };
        }
    } dlg(color);
    if(owner) dlg.CenterOwner();
    if(dlg.Run() != IDOK) return false;
    color = dlg.picker.GetSlotColor(0);
    return true;
}

class PropertyGradientValueEditor : public PropertyValueEditor {
public:
    PropertyGradientValueEditor()
    {
        Add(summary_); Add(edit_);
        summary_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        summary_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
        edit_.SetText("Edit...");
        edit_.WhenAction = [=] { OpenDialog(); };
        summary_.WhenAction = [=] { OpenDialog(); };
    }

    void Configure(const PropertyEditorItem& item) override
    {
        enabled_ = item.enabled && item.value_editable && !item.read_only;
        summary_.Enable(enabled_); edit_.Enable(enabled_);
        SyncSummary();
    }

    void SetEditorValue(const Value& value, bool mixed) override
    {
        mixed_ = mixed;
        value_ = PeNormalizeGradient(value);
        SyncSummary();
    }

    Value GetEditorValue() const override { return value_; }
    void FocusEditor() override { edit_.SetFocus(); }

    void Layout() override
    {
        const int button = min(DPI(64), GetSize().cx / 3);
        summary_.SetRect(0, 0, max(0, GetSize().cx - button - DPI(4)), GetSize().cy);
        edit_.SetRect(max(0, GetSize().cx - button), 0, button, GetSize().cy);
    }

private:
    void SyncSummary()
    {
        if(mixed_) { summary_.SetText("<multiple gradients>"); return; }
        ValueMap map = value_.Is<ValueMap>() ? ValueMap(value_) : ValueMap();
        Value stops = PeMapValue(map, "stops");
        int count = stops.Is<ValueArray>() ? ValueArray(stops).GetCount() : 0;
        summary_.SetText(Format("%s · %d stops · %.0f deg", AsString(PeMapValue(map, "mode", "Linear")), count,
                                (double)PeMapValue(map, "angle", 0.0)));
    }

    void OpenDialog()
    {
        if(!enabled_) return;
        struct Dialog : TopWindow {
            UiDropdown mode, interpolation;
            UiFloatEdit angle, position;
            UiIntEdit alpha;
            UiLabel mode_label, interpolation_label, angle_label, stop_label, position_label, alpha_label;
            UiButton color, previous, next, add, remove, ok, cancel;
            Vector<PeGradientStop> stops;
            int selected = 0;

            Dialog(const Value& recipe)
            {
                Title("Gradient editor"); Sizeable().Zoomable();
                SetRect(0, 0, DPI(560), DPI(360));
                ValueMap map = PeNormalizeGradient(recipe);
                mode.UseInternalModel().Clear().Add("Linear", "Linear").Add("Radial", "Radial");
                mode.SelectByData(AsString(PeMapValue(map, "mode", "Linear")));
                interpolation.UseInternalModel().Clear().Add("Linear", "Linear").Add("Smooth", "Smooth");
                interpolation.SelectByData(AsString(PeMapValue(map, "interpolation", "Linear")));
                angle.SetValue((double)PeMapValue(map, "angle", 0.0)); angle.MinMax(0, 359.999).Step(1);
                ValueArray input = PeMapValue(map, "stops");
                for(const Value& value : input) {
                    ValueMap item = value;
                    PeGradientStop& stop = stops.Add();
                    stop.position = (double)PeMapValue(item, "position", 0.0);
                    stop.color = Color(PeMapValue(item, "color", Black()));
                    stop.alpha = (int)PeMapValue(item, "alpha", 255);
                }
                mode_label.SetText("Mode"); interpolation_label.SetText("Interpolation"); angle_label.SetText("Angle");
                position_label.SetText("Position"); alpha_label.SetText("Alpha");
                color.SetText("Colour..."); previous.SetText("Previous"); next.SetText("Next");
                add.SetText("Add stop"); remove.SetText("Remove"); ok.SetText("OK"); cancel.SetText("Cancel");
                Add(mode_label); Add(mode); Add(interpolation_label); Add(interpolation); Add(angle_label); Add(angle);
                Add(stop_label); Add(position_label); Add(position); Add(alpha_label); Add(alpha); Add(color);
                Add(previous); Add(next); Add(add); Add(remove); Add(ok); Add(cancel);
                previous.WhenAction = [=] { SaveStop(); if(selected > 0) selected--; LoadStop(); };
                next.WhenAction = [=] { SaveStop(); if(selected + 1 < stops.GetCount()) selected++; LoadStop(); };
                add.WhenAction = [=] { AddStop(); };
                remove.WhenAction = [=] { RemoveStop(); };
                color.WhenAction = [=] { PickStopColor(); };
                ok.WhenAction = [=] { SaveStop(); AcceptBreak(IDOK); };
                cancel.WhenAction = [=] { RejectBreak(IDCANCEL); };
                LoadStop();
            }

            void Layout() override
            {
                Rect r = GetSize();
                const int pad = DPI(12), label = DPI(104), h = DPI(30), gap = DPI(8);
                int y = pad;
                mode_label.SetRect(pad, y, label, h); mode.SetRect(pad + label, y, DPI(150), h); y += h + gap;
                interpolation_label.SetRect(pad, y, label, h); interpolation.SetRect(pad + label, y, DPI(150), h); y += h + gap;
                angle_label.SetRect(pad, y, label, h); angle.SetRect(pad + label, y, DPI(110), h); y += h + DPI(14);
                stop_label.SetRect(pad, y, max(0, r.GetWidth() - 2 * pad), h); y += h + gap;
                position_label.SetRect(pad, y, label, h); position.SetRect(pad + label, y, DPI(110), h);
                alpha_label.SetRect(pad + label + DPI(126), y, DPI(54), h); alpha.SetRect(pad + label + DPI(184), y, DPI(80), h);
                color.SetRect(r.right - pad - DPI(100), y, DPI(100), h); y += h + gap;
                previous.SetRect(pad, y, DPI(82), h); next.SetRect(pad + DPI(88), y, DPI(72), h);
                add.SetRect(pad + DPI(170), y, DPI(88), h); remove.SetRect(pad + DPI(264), y, DPI(76), h);
                cancel.SetRect(r.right - pad - DPI(88), r.bottom - pad - h, DPI(88), h);
                ok.SetRect(r.right - pad - DPI(182), r.bottom - pad - h, DPI(88), h);
            }

            void SaveStop()
            {
                if(selected < 0 || selected >= stops.GetCount()) return;
                stops[selected].position = minmax(position.GetValue(), 0.0, 1.0);
                stops[selected].alpha = minmax(alpha.GetValue(), 0, 255);
                Sort(stops, [](const PeGradientStop& a, const PeGradientStop& b) { return a.position < b.position; });
                selected = min(selected, stops.GetCount() - 1);
            }

            void LoadStop()
            {
                if(stops.IsEmpty()) return;
                selected = minmax(selected, 0, stops.GetCount() - 1);
                const PeGradientStop& stop = stops[selected];
                position.SetValue(stop.position); alpha.SetValue(stop.alpha);
                color.SetText(Format("#%02X%02X%02X", stop.color.GetR(), stop.color.GetG(), stop.color.GetB()));
                stop_label.SetText(Format("Stop %d of %d", selected + 1, stops.GetCount()));
                previous.Enable(selected > 0); next.Enable(selected + 1 < stops.GetCount());
                remove.Enable(stops.GetCount() > 2);
            }

            void AddStop()
            {
                SaveStop();
                PeGradientStop stop;
                stop.position = stops.IsEmpty() ? 0.5 : min(1.0, stops[selected].position + 0.1);
                stop.color = stops.IsEmpty() ? White() : stops[selected].color;
                stop.alpha = stops.IsEmpty() ? 255 : stops[selected].alpha;
                stops.Insert(selected + 1, stop); selected++;
                LoadStop();
            }

            void RemoveStop()
            {
                if(stops.GetCount() <= 2) return;
                stops.Remove(selected); selected = min(selected, stops.GetCount() - 1); LoadStop();
            }

            void PickStopColor()
            {
                if(selected < 0 || selected >= stops.GetCount()) return;
                Color next_color = stops[selected].color;
                if(PePickColor(next_color, this)) { stops[selected].color = next_color; LoadStop(); }
            }

            Value GetRecipe() const
            {
                ValueArray values;
                for(const PeGradientStop& stop : stops) values.Add(PeGradientStopValue(stop));
                ValueMap out;
                out.Set("mode", AsString(mode.GetSelectedData()));
                out.Set("angle", angle.GetValue());
                out.Set("interpolation", AsString(interpolation.GetSelectedData()));
                out.Set("stops", values);
                return PeNormalizeGradient(out);
            }
        } dlg(value_);
        dlg.CenterOwner();
        if(dlg.Run() != IDOK) return;
        value_ = dlg.GetRecipe(); mixed_ = false; SyncSummary();
        WhenPreview(value_); WhenCommit(value_);
    }

    PropertyActionLabel summary_;
    UiButton edit_;
    Value value_;
    bool mixed_ = false;
    bool enabled_ = true;
};

// -----------------------------------------------------------------------------
// Key chord

class PropertyKeyChordValueEditor : public PropertyValueEditor {
public:
    PropertyKeyChordValueEditor()
    {
        Add(edit_.SizePos());
        edit_.WhenChange = [=] { if(!syncing_) WhenPreview(PeCanonicalKeyChord(edit_.GetTextUtf8())); };
        edit_.WhenAction = [=] {
            if(syncing_) return;
            String value = PeCanonicalKeyChord(edit_.GetTextUtf8());
            syncing_ = true; edit_.SetTextUtf8(value); syncing_ = false;
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
    Value GetEditorValue() const override { return PeCanonicalKeyChord(edit_.GetTextUtf8()); }
    void FocusEditor() override { edit_.SetFocus(); edit_.SetSelection(); }
private:
    UiLineEdit edit_;
    bool syncing_ = false;
};

// -----------------------------------------------------------------------------
// Generic resource/reference provider

class PropertyReferenceValueEditor : public PropertyValueEditor {
public:
    PropertyReferenceValueEditor()
    {
        Add(summary_); Add(browse_);
        summary_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        summary_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
        browse_.SetText("Browse...");
        summary_.WhenAction = [=] { Pick(); };
        browse_.WhenAction = [=] { Pick(); };
    }
    void Configure(const PropertyEditorItem& item) override
    {
        provider_ = item.picker_provider;
        enabled_ = item.enabled && item.value_editable && !item.read_only &&
                   !provider_.IsEmpty() && PropertyEditorFactory::Global().HasPicker(provider_);
        summary_.Enable(enabled_); browse_.Enable(enabled_); SyncSummary();
    }
    void SetEditorValue(const Value& value, bool mixed) override
    {
        value_ = value; mixed_ = mixed; SyncSummary();
    }
    Value GetEditorValue() const override { return value_; }
    void FocusEditor() override { browse_.SetFocus(); }
    void Layout() override
    {
        const int button = min(DPI(76), GetSize().cx / 3);
        summary_.SetRect(0, 0, max(0, GetSize().cx - button - DPI(4)), GetSize().cy);
        browse_.SetRect(max(0, GetSize().cx - button), 0, button, GetSize().cy);
    }
private:
    void SyncSummary() { summary_.SetText(mixed_ ? "<multiple references>" : IsNull(value_) ? "<none>" : AsString(value_)); }
    void Pick()
    {
        if(!enabled_) return;
        Value next = value_;
        if(!PropertyEditorFactory::Global().PickValue(provider_, next, this)) return;
        value_ = next; mixed_ = false; SyncSummary(); WhenPreview(value_); WhenCommit(value_);
    }
    PropertyActionLabel summary_;
    UiButton browse_;
    Value value_;
    String provider_;
    bool mixed_ = false;
    bool enabled_ = false;
};

// -----------------------------------------------------------------------------
// Optional / nullable value

class PropertyOptionalValueEditor : public PropertyValueEditor {
public:
    PropertyOptionalValueEditor()
    {
        Add(set_); Add(text_); Add(integer_); Add(number_);
        set_.SetText("Set");
        set_.WhenAction = [=] { ToggleSet(); };
        text_.WhenChange = [=] { Changed(false); }; text_.WhenAction = [=] { Changed(true); };
        integer_.WhenChange = [=] { Changed(false); }; integer_.WhenAction = [=] { Changed(true); };
        number_.WhenChange = [=] { Changed(false); }; number_.WhenAction = [=] { Changed(true); };
    }
    void Configure(const PropertyEditorItem& item) override
    {
        variant_ = item.editor_variant;
        fallback_ = item.default_value;
        enabled_ = item.enabled && item.value_editable && !item.read_only;
        set_.Enable(enabled_); UpdateVisible();
    }
    void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        is_set_ = !mixed && !IsNull(value);
        set_.SetData(is_set_);
        Value shown = is_set_ ? value : fallback_;
        if(variant_ == "int") integer_.SetData(shown);
        else if(variant_ == "double") number_.SetData(shown);
        else text_.SetTextUtf8(AsString(shown));
        syncing_ = false; UpdateVisible();
    }
    Value GetEditorValue() const override
    {
        if(!is_set_) return Value();
        if(variant_ == "int") return integer_.GetData();
        if(variant_ == "double") return number_.GetData();
        return text_.GetData();
    }
    void FocusEditor() override
    {
        if(!is_set_) set_.SetFocus();
        else if(variant_ == "int") integer_.SetFocus();
        else if(variant_ == "double") number_.SetFocus();
        else text_.SetFocus();
    }
    void Layout() override
    {
        const int check = min(DPI(48), GetSize().cx / 3);
        set_.SetRect(0, 0, check, GetSize().cy);
        Rect value(check + DPI(4), 0, max(0, GetSize().cx - check - DPI(4)), GetSize().cy);
        text_.SetRect(value); integer_.SetRect(value); number_.SetRect(value);
    }
private:
    void ToggleSet()
    {
        if(syncing_) return;
        is_set_ = (bool)set_.GetData(); UpdateVisible();
        Value value = GetEditorValue(); WhenPreview(value); WhenCommit(value);
    }
    void Changed(bool commit)
    {
        if(syncing_ || !is_set_) return;
        Value value = GetEditorValue(); WhenPreview(value); if(commit) WhenCommit(value);
    }
    void UpdateVisible()
    {
        const bool show_text = is_set_ && variant_ != "int" && variant_ != "double";
        text_.Show(show_text); integer_.Show(is_set_ && variant_ == "int"); number_.Show(is_set_ && variant_ == "double");
        text_.Enable(enabled_); integer_.Enable(enabled_); number_.Enable(enabled_);
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

// -----------------------------------------------------------------------------
// Registration

void RegisterPropertyEditorSemanticEditors(PropertyEditorFactory& factory)
{
    if(!factory.HasCustom(PropertyEditorDateTimeId()))
        factory.RegisterCustom(PropertyEditorDateTimeId(), [] { return One<PropertyValueEditor>(new PropertyDateTimeValueEditor); });
    if(!factory.HasCustom(PropertyEditorDurationId()))
        factory.RegisterCustom(PropertyEditorDurationId(), [] { return One<PropertyValueEditor>(new PropertyDurationValueEditor); });
    if(!factory.HasCustom(PropertyEditorGeometryId()))
        factory.RegisterCustom(PropertyEditorGeometryId(), [] { return One<PropertyValueEditor>(new PropertyGeometryValueEditor); });
    if(!factory.HasCustom(PropertyEditorFlagsId()))
        factory.RegisterCustom(PropertyEditorFlagsId(), [] { return One<PropertyValueEditor>(new PropertyFlagsValueEditor); });
    if(!factory.HasCustom(PropertyEditorStringListId()))
        factory.RegisterCustom(PropertyEditorStringListId(), [] { return One<PropertyValueEditor>(new PropertyStringListValueEditor); });
    if(!factory.HasCustom(PropertyEditorGradientId()))
        factory.RegisterCustom(PropertyEditorGradientId(), [] { return One<PropertyValueEditor>(new PropertyGradientValueEditor); });
    if(!factory.HasCustom(PropertyEditorKeyChordId()))
        factory.RegisterCustom(PropertyEditorKeyChordId(), [] { return One<PropertyValueEditor>(new PropertyKeyChordValueEditor); });
    if(!factory.HasCustom(PropertyEditorReferenceId()))
        factory.RegisterCustom(PropertyEditorReferenceId(), [] { return One<PropertyValueEditor>(new PropertyReferenceValueEditor); });
    if(!factory.HasCustom(PropertyEditorOptionalId()))
        factory.RegisterCustom(PropertyEditorOptionalId(), [] { return One<PropertyValueEditor>(new PropertyOptionalValueEditor); });
}

void RegisterPropertyEditorEditors(PropertyEditorFactory& factory)
{
    RegisterPropertyEditorV1Editors(factory);
    RegisterPropertyEditorSemanticEditors(factory);
}

// -----------------------------------------------------------------------------
// Schema helpers and normalization

static PropertyEditorItem& PeAddDateTime(PropertyEditorModel& model,
                                         const String& id, const String& label,
                                         const Value& value, const String& variant,
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
        if(IsNull(candidate)) return Value();
        if(candidate.Is<Date>()) return candidate;
        if(candidate.Is<Time>()) { Time t = candidate; return Value(Date(t.year, t.month, t.day)); }
        return Value();
    };
    item.validate = [](const Value& candidate) {
        if(IsNull(candidate)) return String();
        return candidate.Is<Date>() || candidate.Is<Time>() ? String() : String("Expected a Date value");
    };
    return item;
}

PropertyEditorItem& AddPropertyTime(PropertyEditorModel& model,
                                    const String& id, const String& label,
                                    Time value, bool show_seconds,
                                    const String& group)
{
    if(!IsNull(value)) value = Time(1970, 1, 1, value.hour, value.minute, show_seconds ? value.second : 0);
    PropertyEditorItem& item = PeAddDateTime(model, id, label, value,
        show_seconds ? "time-only.seconds" : "time-only", group);
    item.normalize = [=](const Value& candidate) {
        if(IsNull(candidate)) return Value();
        if(!candidate.Is<Time>()) return Value();
        Time t = candidate;
        return Value(Time(1970, 1, 1, t.hour, t.minute, show_seconds ? t.second : 0));
    };
    item.validate = [](const Value& candidate) {
        return IsNull(candidate) || candidate.Is<Time>() ? String() : String("Expected a Time value");
    };
    return item;
}

PropertyEditorItem& AddPropertyDateTime(PropertyEditorModel& model,
                                        const String& id, const String& label,
                                        Time value, bool show_seconds,
                                        const String& group)
{
    if(!IsNull(value) && !show_seconds) value.second = 0;
    PropertyEditorItem& item = PeAddDateTime(model, id, label, value,
        show_seconds ? "date-time.seconds" : "date-time", group);
    item.normalize = [=](const Value& candidate) {
        if(IsNull(candidate)) return Value();
        if(!candidate.Is<Time>()) return Value();
        Time t = candidate; if(!show_seconds) t.second = 0; return Value(t);
    };
    item.validate = [](const Value& candidate) {
        return IsNull(candidate) || candidate.Is<Time>() ? String() : String("Expected a Time value");
    };
    return item;
}

PropertyEditorItem& AddPropertyDuration(PropertyEditorModel& model,
                                        const String& id, const String& label,
                                        double seconds, double minimum_seconds,
                                        double maximum_seconds, double step_seconds,
                                        const String& group)
{
    if(maximum_seconds < minimum_seconds) Swap(maximum_seconds, minimum_seconds);
    seconds = minmax(seconds, minimum_seconds, maximum_seconds);
    PropertyEditorItem& item = model.Add(id, label, PropertyEditorKind::Custom, seconds, group);
    item.custom_editor = PropertyEditorDurationId(); item.minimum = minimum_seconds; item.maximum = maximum_seconds;
    item.step = max(0.0, step_seconds); item.inline_editor = true; item.row_span = 1; item.unit = "s";
    item.normalize = [=](const Value& candidate) {
        if(!IsNumber(candidate)) return Value(seconds);
        double value = minmax((double)candidate, minimum_seconds, maximum_seconds);
        if(step_seconds > 0.0)
            value = minimum_seconds + floor((value - minimum_seconds) / step_seconds + 0.5) * step_seconds;
        return Value(minmax(value, minimum_seconds, maximum_seconds));
    };
    item.validate = [](const Value& candidate) { return IsNumber(candidate) ? String() : String("Expected duration seconds"); };
    return item;
}

static PropertyEditorItem& PeAddGeometry(PropertyEditorModel& model,
                                         const String& id, const String& label,
                                         const ValueArray& values, const String& variant,
                                         const String& group)
{
    PropertyEditorItem& item = model.Add(id, label, PropertyEditorKind::Custom, values, group);
    item.custom_editor = PropertyEditorGeometryId(); item.editor_variant = variant; item.inline_editor = true; item.row_span = 1;
    const int count = values.GetCount();
    item.normalize = [=](const Value& candidate) { return Value(PeNumericArray(candidate, count, values)); };
    item.validate = [=](const Value& candidate) {
        if(!candidate.Is<ValueArray>()) return String("Expected a geometry value array");
        ValueArray value = candidate;
        if(value.GetCount() != count) return Format("Expected %d geometry components", count);
        for(const Value& part : value) if(!IsNumber(part)) return String("Geometry components must be numeric");
        return String();
    };
    return item;
}

PropertyEditorItem& AddPropertyPoint(PropertyEditorModel& model, const String& id, const String& label,
                                     double x, double y, const String& group)
{ ValueArray v; v.Add(x); v.Add(y); return PeAddGeometry(model, id, label, v, "point", group); }
PropertyEditorItem& AddPropertySize(PropertyEditorModel& model, const String& id, const String& label,
                                    double cx, double cy, const String& group)
{ ValueArray v; v.Add(cx); v.Add(cy); return PeAddGeometry(model, id, label, v, "size", group); }
PropertyEditorItem& AddPropertyRect(PropertyEditorModel& model, const String& id, const String& label,
                                    double x, double y, double cx, double cy, const String& group)
{ ValueArray v; v.Add(x); v.Add(y); v.Add(cx); v.Add(cy); return PeAddGeometry(model, id, label, v, "rect", group); }
PropertyEditorItem& AddPropertyInsets(PropertyEditorModel& model, const String& id, const String& label,
                                      double left, double top, double right, double bottom,
                                      bool linked, const String& group)
{ ValueArray v; v.Add(left); v.Add(top); v.Add(right); v.Add(bottom); return PeAddGeometry(model, id, label, v, linked ? "insets.linked" : "insets", group); }
PropertyEditorItem& AddPropertyCorners(PropertyEditorModel& model, const String& id, const String& label,
                                       double tl, double tr, double br, double bl,
                                       bool linked, const String& group)
{ ValueArray v; v.Add(tl); v.Add(tr); v.Add(br); v.Add(bl); return PeAddGeometry(model, id, label, v, linked ? "corners.linked" : "corners", group); }

PropertyEditorItem& AddPropertyFlags(PropertyEditorModel& model,
                                     const String& id, const String& label,
                                     const ValueArray& selected,
                                     const String& group)
{
    PropertyEditorItem& item = model.Add(id, label, PropertyEditorKind::Custom, selected, group);
    item.custom_editor = PropertyEditorFlagsId(); item.inline_editor = true; item.row_span = 1;
    item.normalize = [&item](const Value& candidate) {
        ValueArray out;
        if(!candidate.Is<ValueArray>()) return Value(out);
        ValueArray source = candidate;
        for(const Value& value : source) {
            bool allowed = item.choices.IsEmpty();
            for(const PropertyEditorChoice& choice : item.choices)
                if(choice.value == value) { allowed = true; break; }
            if(allowed && !PeContainsValue(out, value)) out.Add(value);
        }
        return Value(out);
    };
    item.validate = [](const Value& candidate) { return candidate.Is<ValueArray>() ? String() : String("Expected selected flag values"); };
    return item;
}

PropertyEditorItem& AddPropertyStringList(PropertyEditorModel& model,
                                          const String& id, const String& label,
                                          const ValueArray& values,
                                          int maximum_items,
                                          const String& group)
{
    maximum_items = max(1, maximum_items);
    PropertyEditorItem& item = model.Add(id, label, PropertyEditorKind::Custom, values, group);
    item.custom_editor = PropertyEditorStringListId(); item.maximum = maximum_items; item.inline_editor = true; item.row_span = 1;
    item.normalize = [=](const Value& candidate) {
        ValueArray out;
        if(!candidate.Is<ValueArray>()) return Value(out);
        ValueArray source = candidate;
        for(int i = 0; i < source.GetCount() && i < maximum_items; i++) out.Add(AsString(source[i]));
        return Value(out);
    };
    item.validate = [](const Value& candidate) { return candidate.Is<ValueArray>() ? String() : String("Expected an ordered value array"); };
    return item;
}

Value PropertyEditorMakeGradientStop(double position, Color color, int alpha)
{
    PeGradientStop stop; stop.position = minmax(position, 0.0, 1.0); stop.color = color; stop.alpha = minmax(alpha, 0, 255);
    return PeGradientStopValue(stop);
}

Value PropertyEditorMakeGradient(const ValueArray& stops, const String& mode,
                                 double angle, const String& interpolation)
{
    ValueMap out; out.Set("mode", mode); out.Set("angle", angle); out.Set("interpolation", interpolation); out.Set("stops", stops);
    return PeNormalizeGradient(out);
}

PropertyEditorItem& AddPropertyGradient(PropertyEditorModel& model,
                                        const String& id, const String& label,
                                        const Value& recipe,
                                        const String& group)
{
    PropertyEditorItem& item = model.Add(id, label, PropertyEditorKind::Custom, PeNormalizeGradient(recipe), group);
    item.custom_editor = PropertyEditorGradientId(); item.inline_editor = true; item.row_span = 1;
    item.normalize = [](const Value& candidate) { return PeNormalizeGradient(candidate); };
    item.validate = [](const Value& candidate) { return candidate.Is<ValueMap>() ? String() : String("Expected a gradient recipe"); };
    return item;
}

PropertyEditorItem& AddPropertyKeyChord(PropertyEditorModel& model,
                                        const String& id, const String& label,
                                        const String& chord,
                                        const String& group)
{
    PropertyEditorItem& item = model.Add(id, label, PropertyEditorKind::Custom, PeCanonicalKeyChord(chord), group);
    item.custom_editor = PropertyEditorKeyChordId(); item.inline_editor = true; item.row_span = 1;
    item.normalize = [](const Value& candidate) { return Value(PeCanonicalKeyChord(AsString(candidate))); };
    item.validate = [](const Value& candidate) {
        String value = PeCanonicalKeyChord(AsString(candidate));
        return value.IsEmpty() ? String("Enter a key chord such as Ctrl+S") : String();
    };
    return item;
}

PropertyEditorItem& AddPropertyReference(PropertyEditorModel& model,
                                         const String& id, const String& label,
                                         const Value& value,
                                         const String& picker_provider,
                                         const String& group)
{
    PropertyEditorItem& item = model.Add(id, label, PropertyEditorKind::Custom, value, group);
    item.custom_editor = PropertyEditorReferenceId(); item.picker_provider = picker_provider; item.inline_editor = true; item.row_span = 1;
    return item;
}

PropertyEditorItem& AddPropertyOptional(PropertyEditorModel& model,
                                        const String& id, const String& label,
                                        const Value& value,
                                        const Value& fallback,
                                        const String& variant,
                                        const String& group)
{
    String use_variant = variant == "int" || variant == "double" ? variant : "text";
    PropertyEditorItem& item = model.Add(id, label, PropertyEditorKind::Custom, value, group);
    item.custom_editor = PropertyEditorOptionalId(); item.editor_variant = use_variant; item.allow_null = true; item.inline_editor = true; item.row_span = 1;
    item.SetDefault(fallback);
    item.normalize = [=](const Value& candidate) {
        if(IsNull(candidate)) return Value();
        if(use_variant == "int") return Value((int)candidate);
        if(use_variant == "double") return Value((double)candidate);
        return Value(AsString(candidate));
    };
    return item;
}

} // namespace Upp
