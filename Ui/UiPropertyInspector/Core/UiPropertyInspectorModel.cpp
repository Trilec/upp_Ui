#include "UiPropertyInspectorModel.h"

#include <cerrno>
#include <cmath>
#include <cstdlib>

namespace Upp {

static double PeClamp(double v, double lo, double hi)
{
    if(v < lo) return lo;
    if(v > hi) return hi;
    return v;
}

static bool PeParseInt(const Value& value, int& out)
{
    if(IsNumber(value)) {
        out = (int)value;
        return true;
    }

    String s = TrimBoth(AsString(value));
    if(s.IsEmpty())
        return false;

    errno = 0;
    char *end = nullptr;
    long v = std::strtol(s.Begin(), &end, 10);
    if(errno != 0 || end == s.Begin())
        return false;
    while(end && *end && (byte)*end <= ' ')
        ++end;
    if(end && *end)
        return false;

    if(v < INT_MIN) v = INT_MIN;
    if(v > INT_MAX) v = INT_MAX;
    out = (int)v;
    return true;
}

static bool PeParseDouble(const Value& value, double& out)
{
    if(IsNumber(value)) {
        out = (double)value;
        return std::isfinite(out);
    }

    String s = TrimBoth(AsString(value));
    if(s.IsEmpty())
        return false;

    errno = 0;
    char *end = nullptr;
    double v = std::strtod(s.Begin(), &end);
    if(errno != 0 || end == s.Begin() || !std::isfinite(v))
        return false;
    while(end && *end && (byte)*end <= ' ')
        ++end;
    if(end && *end)
        return false;

    out = v;
    return true;
}

static bool PeParseBool(const Value& value, bool& out)
{
    if(value.GetType() == BOOL_V || IsNumber(value)) {
        out = (bool)value;
        return true;
    }

    String s = ToLower(TrimBoth(AsString(value)));
    if(s == "true" || s == "yes" || s == "on" || s == "1") {
        out = true;
        return true;
    }
    if(s == "false" || s == "no" || s == "off" || s == "0") {
        out = false;
        return true;
    }
    return false;
}

static bool PeHasBound(const Value& value)
{
    return !IsNull(value) && IsNumber(value);
}

static Value PeClampNumber(const UiPropertyInspectorItem& item, double value, bool integer)
{
    if(PeHasBound(item.minimum))
        value = max(value, (double)item.minimum);
    if(PeHasBound(item.maximum))
        value = min(value, (double)item.maximum);
    if(PeHasBound(item.step) && (double)item.step > 0) {
        double base = PeHasBound(item.minimum) ? (double)item.minimum : 0.0;
        double step = (double)item.step;
        value = base + floor((value - base) / step + 0.5) * step;
        if(PeHasBound(item.minimum))
            value = max(value, (double)item.minimum);
        if(PeHasBound(item.maximum))
            value = min(value, (double)item.maximum);
    }
    if(integer) {
        double rounded = value >= 0 ? floor(value + 0.5) : ceil(value - 0.5);
        return Value((int)minmax(rounded, (double)INT_MIN, (double)INT_MAX));
    }
    return Value(value);
}

UiPropertyInspectorItem& UiPropertyInspectorItem::SetRange(const Value& min_value,
                                                  const Value& max_value,
                                                  const Value& step_value)
{
    minimum = min_value;
    maximum = max_value;
    step = step_value;
    return *this;
}

UiPropertyInspectorItem& UiPropertyInspectorItem::SetDefault(const Value& v, bool can_reset)
{
    default_value = v;
    resettable = can_reset;
    return *this;
}

UiPropertyInspectorItem& UiPropertyInspectorItem::AddChoice(const Value& v,
                                                   const String& text,
                                                   const Image& icon)
{
    choices.Add(UiPropertyInspectorChoice(v, text, icon));
    return *this;
}

UiPropertyInspectorItem& UiPropertyInspectorItem::SetMixed(bool on)
{
    mixed = on;
    return *this;
}

UiPropertyInspectorItem& UiPropertyInspectorItem::SetInherited(bool on)
{
    inherited = on;
    return *this;
}

UiPropertyInspectorItem& UiPropertyInspectorItem::SetEnabled(bool on)
{
    enabled = on;
    return *this;
}

UiPropertyInspectorItem& UiPropertyInspectorItem::SetVisible(bool on)
{
    visible = on;
    return *this;
}

UiPropertyInspectorItem& UiPropertyInspectorItem::SetReadOnly(bool on)
{
    read_only = on;
    return *this;
}

UiPropertyInspectorItem& UiPropertyInspectorItem::SetHelp(const String& text)
{
    help = text;
    return *this;
}

UiPropertyInspectorItem& UiPropertyInspectorItem::SetUnit(const String& text)
{
    unit = text;
    return *this;
}

UiPropertyInspectorItem& UiPropertyInspectorItem::SetImpact(UiPropertyInspectorImpact value)
{
    impact = value;
    return *this;
}

UiPropertyInspectorItem& UiPropertyInspectorItem::SetDomain(UiPropertyInspectorDomain value)
{
    domain = value;
    return *this;
}

void UiPropertyInspectorModel::Clear(bool notify)
{
    items_.Clear();
    group_subtitles_.Clear();
    if(notify)
        StructureChanged();
}

void UiPropertyInspectorModel::SetGroupSubtitle(const String& group,
                                           const String& subtitle)
{
    const int q = group_subtitles_.Find(group);
    if(subtitle.IsEmpty()) {
        if(q < 0)
            return;
        group_subtitles_.Remove(q);
    }
    else if(q >= 0) {
        if(group_subtitles_[q] == subtitle)
            return;
        group_subtitles_[q] = subtitle;
    }
    else
        group_subtitles_.Add(group, subtitle);
    WhenGroupMetadataChanged(group);
}

String UiPropertyInspectorModel::GetGroupSubtitle(const String& group) const
{
    const int q = group_subtitles_.Find(group);
    return q >= 0 ? group_subtitles_[q] : String();
}

void UiPropertyInspectorModel::ClearGroupSubtitle(const String& group)
{
    const int q = group_subtitles_.Find(group);
    if(q < 0)
        return;
    group_subtitles_.Remove(q);
    WhenGroupMetadataChanged(group);
}

void UiPropertyInspectorModel::ClearGroupSubtitles()
{
    if(group_subtitles_.IsEmpty())
        return;
    group_subtitles_.Clear();
    WhenGroupMetadataChanged(String());
}

UiPropertyInspectorItem& UiPropertyInspectorModel::Add(const String& id,
                                             const String& label,
                                             UiPropertyInspectorKind kind,
                                             const Value& value,
                                             const String& group)
{
    UiPropertyInspectorItem& item = items_.Add();
    item.id = id;
    item.label = label;
    item.kind = kind;
    item.value = value;
    item.default_value = value;
    item.group = group;
    item.sort_order = items_.GetCount() - 1;
    structure_revision_++;
    return item;
}

UiPropertyInspectorItem& UiPropertyInspectorModel::AddText(const String& id,
                                                 const String& label,
                                                 const String& value,
                                                 const String& group)
{
    return Add(id, label, UiPropertyInspectorKind::Text, value, group);
}

UiPropertyInspectorItem& UiPropertyInspectorModel::AddMultiline(const String& id,
                                                      const String& label,
                                                      const String& value,
                                                      const String& group)
{
    return Add(id, label, UiPropertyInspectorKind::Multiline, value, group);
}

UiPropertyInspectorItem& UiPropertyInspectorModel::AddInteger(const String& id,
                                                    const String& label,
                                                    int value,
                                                    const String& group)
{
    return Add(id, label, UiPropertyInspectorKind::Integer, value, group);
}

UiPropertyInspectorItem& UiPropertyInspectorModel::AddDouble(const String& id,
                                                   const String& label,
                                                   double value,
                                                   const String& group)
{
    return Add(id, label, UiPropertyInspectorKind::Double, value, group);
}

UiPropertyInspectorItem& UiPropertyInspectorModel::AddNumericInt(const String& id,
                                                       const String& label,
                                                       int value,
                                                       int minimum,
                                                       int maximum,
                                                       int step,
                                                       const String& group)
{
    UiPropertyInspectorItem& item = Add(id, label, UiPropertyInspectorKind::NumericInt, value, group);
    item.SetRange(minimum, maximum, step).SetSliderToggle();
    return item;
}

UiPropertyInspectorItem& UiPropertyInspectorModel::AddNumericDouble(const String& id,
                                                          const String& label,
                                                          double value,
                                                          double minimum,
                                                          double maximum,
                                                          double step,
                                                          const String& group)
{
    UiPropertyInspectorItem& item = Add(id, label, UiPropertyInspectorKind::NumericDouble, value, group);
    item.SetRange(minimum, maximum, step).SetSliderToggle();
    return item;
}

UiPropertyInspectorItem& UiPropertyInspectorModel::AddBoolean(const String& id,
                                                    const String& label,
                                                    bool value,
                                                    const String& group)
{
    return Add(id, label, UiPropertyInspectorKind::Boolean, value, group);
}

UiPropertyInspectorItem& UiPropertyInspectorModel::AddChoice(const String& id,
                                                   const String& label,
                                                   const Value& value,
                                                   const String& group)
{
    return Add(id, label, UiPropertyInspectorKind::Choice, value, group);
}

UiPropertyInspectorItem& UiPropertyInspectorModel::AddColor(const String& id,
                                                  const String& label,
                                                  Color value,
                                                  const String& group)
{
    return Add(id, label, UiPropertyInspectorKind::Color, value, group);
}

UiPropertyInspectorItem& UiPropertyInspectorItem::SetColorCount(int count)
{
    color_count = clamp(count, 1, 8);
    return *this;
}

UiPropertyInspectorItem& UiPropertyInspectorItem::SetSliderToggle(bool on)
{
    show_slider_toggle = on;
    return *this;
}

UiPropertyInspectorItem& UiPropertyInspectorModel::AddSlider(const String& id,
                                                   const String& label,
                                                   double value,
                                                   double minimum,
                                                   double maximum,
                                                   double step,
                                                   const String& group)
{
    UiPropertyInspectorItem& item =
        Add(id, label, UiPropertyInspectorKind::SliderDouble, value, group);
    item.SetRange(minimum, maximum, step);
    return item;
}

UiPropertyInspectorItem& UiPropertyInspectorModel::AddSliderInt(const String& id,
                                                      const String& label,
                                                      int value,
                                                      int minimum,
                                                      int maximum,
                                                      int step,
                                                      const String& group)
{
    UiPropertyInspectorItem& item =
        Add(id, label, UiPropertyInspectorKind::SliderInt, value, group);
    item.SetRange(minimum, maximum, step);
    return item;
}

UiPropertyInspectorItem& UiPropertyInspectorModel::AddVector2(const String& id,
                                                    const String& label,
                                                    double x, double y,
                                                    const String& group)
{
    return Add(id, label, UiPropertyInspectorKind::Vector2,
               UiPropertyInspectorMakeVector(x, y), group);
}

UiPropertyInspectorItem& UiPropertyInspectorModel::AddVector3(const String& id,
                                                    const String& label,
                                                    double x, double y, double z,
                                                    const String& group)
{
    return Add(id, label, UiPropertyInspectorKind::Vector3,
               UiPropertyInspectorMakeVector(x, y, z), group);
}

UiPropertyInspectorItem& UiPropertyInspectorModel::AddCurve(const String& id,
                                                  const String& label,
                                                  const Value& curve,
                                                  const String& group)
{
    return Add(id, label, UiPropertyInspectorKind::Curve,
               UiPropertyInspectorNormalizeCurve(curve), group);
}

UiPropertyInspectorItem& UiPropertyInspectorModel::AddBezierCurve(
    const String& id, const String& label, const Value& control_points,
    const String& group)
{
    UiPropertyInspectorItem& item = Add(id, label, UiPropertyInspectorKind::Curve,
        UiPropertyInspectorNormalizeBezierCurve(control_points), group);
    item.editor_variant = "bezier";
    item.inline_editor = true;
    item.row_span = 1;
    item.expanded_row_span = 4;
    return item;
}

UiPropertyInspectorItem& UiPropertyInspectorModel::AddReadOnly(const String& id,
                                                     const String& label,
                                                     const Value& value,
                                                     const String& group)
{
    UiPropertyInspectorItem& item =
        Add(id, label, UiPropertyInspectorKind::ReadOnly, value, group);
    item.read_only = true;
    return item;
}

UiPropertyInspectorItem* UiPropertyInspectorModel::Find(const String& id)
{
    int q = FindIndex(id);
    return q >= 0 ? &items_[q] : nullptr;
}

const UiPropertyInspectorItem* UiPropertyInspectorModel::Find(const String& id) const
{
    int q = FindIndex(id);
    return q >= 0 ? &items_[q] : nullptr;
}

int UiPropertyInspectorModel::FindIndex(const String& id) const
{
    for(int i = 0; i < items_.GetCount(); i++)
        if(items_[i].id == id)
            return i;
    return -1;
}

bool UiPropertyInspectorModel::SetValue(const String& id, const Value& value, bool notify)
{
    UiPropertyInspectorItem* item = Find(id);
    if(!item)
        return false;
    item->value = value;
    item->mixed = false;
    if(notify)
        ValueChanged(id);
    return true;
}

bool UiPropertyInspectorModel::SetMixed(const String& id, bool mixed, bool notify)
{
    UiPropertyInspectorItem* item = Find(id);
    if(!item)
        return false;
    item->mixed = mixed;
    if(notify)
        ValueChanged(id);
    return true;
}

bool UiPropertyInspectorModel::SetEnabled(const String& id, bool enabled, bool notify)
{
    UiPropertyInspectorItem* item = Find(id);
    if(!item)
        return false;
    item->enabled = enabled;
    if(notify)
        ValueChanged(id);
    return true;
}

bool UiPropertyInspectorModel::SetVisible(const String& id, bool visible, bool notify)
{
    UiPropertyInspectorItem* item = Find(id);
    if(!item)
        return false;
    if(item->visible == visible)
        return true;
    item->visible = visible;
    if(notify)
        StructureChanged();
    return true;
}

bool UiPropertyInspectorModel::SetValidationError(const String& id,
                                             const String& error,
                                             bool notify)
{
    UiPropertyInspectorItem* item = Find(id);
    if(!item)
        return false;
    item->validation_error = error;
    if(notify)
        ValueChanged(id);
    return true;
}

bool UiPropertyInspectorModel::Preview(const String& id,
                                  const Value& candidate,
                                  String *error)
{
    return Apply(id, candidate, false, error);
}

bool UiPropertyInspectorModel::Commit(const String& id,
                                 const Value& candidate,
                                 String *error)
{
    return Apply(id, candidate, true, error);
}

bool UiPropertyInspectorModel::Reset(const String& id, String *error)
{
    UiPropertyInspectorItem* item = Find(id);
    if(!item) {
        if(error) *error = "Unknown property: " + id;
        return false;
    }
    if(!item->resettable) {
        if(error) *error = "Property is not resettable";
        return false;
    }

    // Notifications can rebuild the model, including the caller's id storage.
    const String property_id = id;
    const Value default_value = item->default_value;
    String local_error;
    if(!Apply(property_id, default_value, true, &local_error, true)) {
        if(error) *error = local_error;
        return false;
    }

    WhenReset(property_id);
    if(error) error->Clear();
    return true;
}

bool UiPropertyInspectorModel::Apply(const String& property_id,
                                const Value& candidate,
                                bool final_commit,
                                String *error, bool inherited)
{
    const String id = property_id;
    UiPropertyInspectorItem* item = Find(id);
    if(!item) {
        if(error) *error = "Unknown property: " + id;
        return false;
    }
    if(!item->visible) {
        if(error) *error = "Property is hidden";
        return false;
    }
    if(!item->enabled || item->read_only || item->kind == UiPropertyInspectorKind::ReadOnly) {
        if(error) *error = "Property is read only";
        return false;
    }

    Value normalized;
    String local_error;
    if(!UiPropertyInspectorNormalizeValue(*item, candidate, normalized, local_error)) {
        item->validation_error = local_error;
        ValueChanged(id);
        if(error) *error = local_error;
        return false;
    }

    item->validation_error.Clear();
    item->value = normalized;
    item->mixed = false;
    item->inherited = inherited;
    ValueChanged(id);

    if(final_commit)
        WhenCommit(id, normalized);
    else
        WhenPreview(id, normalized);

    if(error)
        error->Clear();
    return true;
}

void UiPropertyInspectorModel::StructureChanged()
{
    structure_revision_++;
    WhenStructureChanged();
}

void UiPropertyInspectorModel::ValueChanged(const String& id)
{
    value_revision_++;
    WhenValueChanged(id);
}

Value UiPropertyInspectorMakeVector(double x, double y)
{
    ValueArray values;
    values.Add(x);
    values.Add(y);
    return values;
}

Value UiPropertyInspectorMakeVector(double x, double y, double z)
{
    ValueArray values;
    values.Add(x);
    values.Add(y);
    values.Add(z);
    return values;
}

static Vector<double> PeParseVectorString(const String& text, int expected_count)
{
    Vector<double> out;
    Vector<String> parts = Split(text, ',');
    if(parts.GetCount() == 1)
        parts = Split(text, ' ');
    for(const String& raw : parts) {
        String part = TrimBoth(raw);
        if(part.IsEmpty())
            continue;
        double v = 0;
        if(!PeParseDouble(part, v)) {
            out.Clear();
            return out;
        }
        out.Add(v);
    }
    if(expected_count > 0 && out.GetCount() != expected_count)
        out.Clear();
    return out;
}


static bool PeTryReadVector(const Value& value, int expected_count, Vector<double>& out)
{
    out.Clear();

    if(value.GetType() == VALUEARRAY_V) {
        ValueArray values(value);
        if(expected_count >= 0 && values.GetCount() != expected_count)
            return false;
        for(int i = 0; i < values.GetCount(); i++) {
            double v = 0;
            if(!PeParseDouble(values[i], v)) {
                out.Clear();
                return false;
            }
            out.Add(v);
        }
        return expected_count < 0 || out.GetCount() == expected_count;
    }

    if(value.GetType() == STRING_V || value.GetType() == WSTRING_V) {
        out = PeParseVectorString(AsString(value), expected_count);
        return expected_count < 0 ? !out.IsEmpty() : out.GetCount() == expected_count;
    }

    if(IsNumber(value) && expected_count == 1) {
        out.Add((double)value);
        return true;
    }

    return false;
}

Vector<double> UiPropertyInspectorReadVector(const Value& value,
                                        int expected_count,
                                        double fallback)
{
    Vector<double> out;

    if(value.GetType() == VALUEARRAY_V) {
        ValueArray values(value);
        for(int i = 0; i < values.GetCount(); i++) {
            double v = fallback;
            if(!PeParseDouble(values[i], v))
                v = fallback;
            out.Add(v);
        }
    }
    else if(value.GetType() == STRING_V || value.GetType() == WSTRING_V)
        out = PeParseVectorString(AsString(value), expected_count);
    else if(IsNumber(value))
        out.Add((double)value);

    while(out.GetCount() < expected_count)
        out.Add(fallback);
    if(expected_count >= 0 && out.GetCount() > expected_count)
        out.SetCount(expected_count);

    return out;
}

String UiPropertyInspectorFormatVector(const Value& value,
                                  int expected_count,
                                  int decimals)
{
    Vector<double> values = UiPropertyInspectorReadVector(value, expected_count);
    String out;
    for(int i = 0; i < values.GetCount(); i++) {
        if(i)
            out << ", ";
        out << Format("%.*f", max(0, decimals), values[i]);
    }
    return out;
}

Value UiPropertyInspectorMakeCurve(const Vector<Pointf>& points)
{
    ValueArray curve;
    for(const Pointf& p : points) {
        ValueArray point;
        point.Add(p.x);
        point.Add(p.y);
        curve.Add(point);
    }
    return curve;
}

Vector<Pointf> UiPropertyInspectorReadCurve(const Value& value)
{
    Vector<Pointf> out;
    if(value.GetType() != VALUEARRAY_V)
        return out;

    ValueArray curve(value);
    for(int i = 0; i < curve.GetCount(); i++) {
        if(curve[i].GetType() != VALUEARRAY_V)
            continue;
        ValueArray point(curve[i]);
        if(point.GetCount() < 2)
            continue;
        double x = 0, y = 0;
        if(PeParseDouble(point[0], x) && PeParseDouble(point[1], y))
            out.Add(Pointf(x, y));
    }
    return out;
}

Value UiPropertyInspectorNormalizeCurve(const Value& value)
{
    Vector<Pointf> points = UiPropertyInspectorReadCurve(value);
    if(points.IsEmpty()) {
        points.Add(Pointf(0.0, 0.0));
        points.Add(Pointf(1.0, 1.0));
    }

    for(Pointf& p : points) {
        p.x = PeClamp(p.x, 0.0, 1.0);
        p.y = PeClamp(p.y, 0.0, 1.0);
    }

    Sort(points, [](const Pointf& a, const Pointf& b) {
        if(a.x == b.x)
            return a.y < b.y;
        return a.x < b.x;
    });

    Vector<Pointf> unique;
    for(const Pointf& p : points) {
        if(!unique.IsEmpty() && fabs(unique.Top().x - p.x) < 0.000001)
            unique.Top() = p;
        else
            unique.Add(p);
    }

    if(unique.GetCount() == 1) {
        if(unique[0].x > 0.5)
            unique.Insert(0, Pointf(0.0, unique[0].y));
        else
            unique.Add(Pointf(1.0, unique[0].y));
    }

    return UiPropertyInspectorMakeCurve(unique);
}

String UiPropertyInspectorFormatCurve(const Value& value)
{
    Vector<Pointf> points = UiPropertyInspectorReadCurve(UiPropertyInspectorNormalizeCurve(value));
    String out = Format("%d point%s [", points.GetCount(), points.GetCount() == 1 ? "" : "s");
    const int shown = min(points.GetCount(), 2);
    for(int i = 0; i < shown; i++) {
        if(i)
            out << ", ";
        out << Format("(%.4f, %.4f)", points[i].x, points[i].y);
    }
    if(points.GetCount() > shown)
        out << ", ...";
    return out << ']';
}

Value UiPropertyInspectorMakeBezierCurve(double x1, double y1, double x2, double y2)
{
    ValueArray out;
    out.Add(x1); out.Add(y1); out.Add(x2); out.Add(y2);
    return out;
}

Value UiPropertyInspectorNormalizeBezierCurve(const Value& value)
{
    ValueArray in;
    if(value.Is<ValueArray>())
        in = value;
    if(in.GetCount() < 4 || !IsNumber(in[0]) || !IsNumber(in[1]) ||
       !IsNumber(in[2]) || !IsNumber(in[3]))
        return UiPropertyInspectorMakeBezierCurve(0.0, 0.0, 1.0, 1.0);
    return UiPropertyInspectorMakeBezierCurve(
        PeClamp((double)in[0], 0.0, 1.0),
        (double)in[1],
        PeClamp((double)in[2], 0.0, 1.0),
        (double)in[3]);
}

String UiPropertyInspectorFormatBezierCurve(const Value& value)
{
    ValueArray v = UiPropertyInspectorNormalizeBezierCurve(value);
    return Format("cubic-bezier(%.4f, %.4f, %.4f, %.4f)",
                  (double)v[0], (double)v[1], (double)v[2], (double)v[3]);
}

bool UiPropertyInspectorNormalizeValue(const UiPropertyInspectorItem& item,
                                  const Value& candidate,
                                  Value& normalized,
                                  String& error)
{
    if(IsNull(candidate) && item.allow_null) {
        normalized = Null;
        error.Clear();
        return true;
    }

    switch(item.kind) {
    case UiPropertyInspectorKind::Text:
    case UiPropertyInspectorKind::Multiline:
        normalized = AsString(candidate);
        break;

    case UiPropertyInspectorKind::Integer:
    case UiPropertyInspectorKind::NumericInt:
    case UiPropertyInspectorKind::SliderInt: {
        int v = 0;
        if(!PeParseInt(candidate, v)) {
            error = "Expected an integer value";
            return false;
        }
        normalized = PeClampNumber(item, v, true);
        break;
    }

    case UiPropertyInspectorKind::Double:
    case UiPropertyInspectorKind::NumericDouble:
    case UiPropertyInspectorKind::SliderDouble: {
        double v = 0;
        if(!PeParseDouble(candidate, v)) {
            error = "Expected a numeric value";
            return false;
        }
        normalized = PeClampNumber(item, v, false);
        break;
    }

    case UiPropertyInspectorKind::Boolean: {
        bool v = false;
        if(!PeParseBool(candidate, v)) {
            error = "Expected a Boolean value";
            return false;
        }
        normalized = v;
        break;
    }

    case UiPropertyInspectorKind::Choice: {
        bool found = item.choices.IsEmpty();
        for(const UiPropertyInspectorChoice& choice : item.choices)
            if(choice.value == candidate) {
                found = true;
                break;
            }
        if(!found) {
            error = "Value is not one of the allowed choices";
            return false;
        }
        normalized = candidate;
        break;
    }

    case UiPropertyInspectorKind::Color:
        if(candidate.GetType() != COLOR_V && !IsNull(candidate)) {
            error = "Expected a Color value";
            return false;
        }
        normalized = candidate;
        break;

    case UiPropertyInspectorKind::ColorPalette: {
        if(!candidate.Is<ValueArray>()) {
            error = "Expected a colour palette";
            return false;
        }
        ValueArray palette = candidate;
        if(palette.GetCount() != item.color_count) {
            error = Format("Expected %d colours", item.color_count);
            return false;
        }
        for(const Value& color : palette)
            if(color.GetType() != COLOR_V) {
                error = "Palette entries must be colours";
                return false;
            }
        normalized = palette;
        break;
    }

    case UiPropertyInspectorKind::FillRecipe: {
        if(!candidate.Is<ValueMap>()) {
            error = "Expected a fill recipe";
            return false;
        }
        ValueMap recipe = candidate;
        const String mode = AsString(recipe.GetValue(recipe.Find("mode")));
        if(mode != "None" && mode != "Solid" &&
           mode != "QuadGradient" && mode != "Image") {
            error = "Fill recipe has an invalid mode";
            return false;
        }
        normalized = recipe;
        break;
    }

    case UiPropertyInspectorKind::FilePath:
        normalized = AsString(candidate);
        break;

    case UiPropertyInspectorKind::Vector2: {
        Vector<double> v;
        if(!PeTryReadVector(candidate, 2, v)) {
            error = "Expected two numeric components";
            return false;
        }
        normalized = UiPropertyInspectorMakeVector(v[0], v[1]);
        break;
    }

    case UiPropertyInspectorKind::Vector3: {
        Vector<double> v;
        if(!PeTryReadVector(candidate, 3, v)) {
            error = "Expected three numeric components";
            return false;
        }
        normalized = UiPropertyInspectorMakeVector(v[0], v[1], v[2]);
        break;
    }

    case UiPropertyInspectorKind::Curve:
        normalized = item.editor_variant == "bezier"
            ? UiPropertyInspectorNormalizeBezierCurve(candidate)
            : UiPropertyInspectorNormalizeCurve(candidate);
        if(item.editor_variant == "bezier" &&
           !IsNull(item.minimum) && !IsNull(item.maximum)) {
            ValueArray points = normalized;
            const double minimum = (double)item.minimum;
            const double maximum = (double)item.maximum;
            points.Set(1, PeClamp((double)points[1], minimum, maximum));
            points.Set(3, PeClamp((double)points[3], minimum, maximum));
            normalized = points;
        }
        break;

    case UiPropertyInspectorKind::ReadOnly:
        error = "Property is read only";
        return false;

    case UiPropertyInspectorKind::Custom:
        normalized = candidate;
        break;
    }

    if(item.normalize)
        normalized = item.normalize(normalized);

    if(item.validate) {
        String validation = item.validate(normalized);
        if(!validation.IsEmpty()) {
            error = validation;
            return false;
        }
    }

    error.Clear();
    return true;
}

String UiPropertyInspectorKindName(UiPropertyInspectorKind kind)
{
    switch(kind) {
    case UiPropertyInspectorKind::Text: return "Text";
    case UiPropertyInspectorKind::Multiline: return "Multiline";
    case UiPropertyInspectorKind::Integer: return "Integer";
    case UiPropertyInspectorKind::Double: return "Double";
    case UiPropertyInspectorKind::NumericInt: return "NumericInt";
    case UiPropertyInspectorKind::NumericDouble: return "NumericDouble";
    case UiPropertyInspectorKind::Boolean: return "Boolean";
    case UiPropertyInspectorKind::Choice: return "Choice";
    case UiPropertyInspectorKind::Color: return "Color";
    case UiPropertyInspectorKind::ColorPalette: return "ColorPalette";
    case UiPropertyInspectorKind::FillRecipe: return "FillRecipe";
    case UiPropertyInspectorKind::FilePath: return "FilePath";
    case UiPropertyInspectorKind::SliderInt: return "SliderInt";
    case UiPropertyInspectorKind::SliderDouble: return "SliderDouble";
    case UiPropertyInspectorKind::Vector2: return "Vector2";
    case UiPropertyInspectorKind::Vector3: return "Vector3";
    case UiPropertyInspectorKind::Curve: return "Curve";
    case UiPropertyInspectorKind::ReadOnly: return "ReadOnly";
    case UiPropertyInspectorKind::Custom: return "Custom";
    }
    return "Unknown";
}

String UiPropertyInspectorDomainName(UiPropertyInspectorDomain domain)
{
    switch(domain) {
    case UiPropertyInspectorDomain::General: return "General";
    case UiPropertyInspectorDomain::Content: return "Content";
    case UiPropertyInspectorDomain::Behaviour: return "Behaviour";
    case UiPropertyInspectorDomain::Layout: return "Layout";
    case UiPropertyInspectorDomain::Appearance: return "Appearance";
    case UiPropertyInspectorDomain::Theme: return "Theme";
    case UiPropertyInspectorDomain::Runtime: return "Runtime";
    case UiPropertyInspectorDomain::DesignerOnly: return "DesignerOnly";
    }
    return "Unknown";
}

String UiPropertyInspectorImpactName(UiPropertyInspectorImpact impact)
{
    if(impact == PropertyImpactNone)
        return "None";

    struct Name {
        UiPropertyInspectorImpact flag;
        const char *text;
    };
    static const Name names[] = {
        { PropertyImpactPaint, "Paint" },
        { PropertyImpactControlState, "ControlState" },
        { PropertyImpactLocalLayout, "LocalLayout" },
        { PropertyImpactAncestorLayout, "AncestorLayout" },
        { PropertyImpactSubtree, "Subtree" },
        { PropertyImpactStructure, "Structure" },
        { PropertyImpactSelection, "Selection" },
        { PropertyImpactInspectorSchema, "InspectorSchema" },
        { PropertyImpactCode, "Code" },
        { PropertyImpactThemeGlobal, "ThemeGlobal" },
        { PropertyImpactFullPreview, "FullPreview" },
    };

    String out;
    for(const Name& name : names)
        if(HasPropertyImpact(impact, name.flag)) {
            if(!out.IsEmpty())
                out << "|";
            out << name.text;
        }
    return out;
}

}
