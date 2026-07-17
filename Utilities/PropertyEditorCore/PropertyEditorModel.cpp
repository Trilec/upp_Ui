#include "PropertyEditorModel.h"

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

static Value PeClampNumber(const PropertyEditorItem& item, double value, bool integer)
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

PropertyEditorItem& PropertyEditorItem::SetRange(const Value& min_value,
                                                  const Value& max_value,
                                                  const Value& step_value)
{
    minimum = min_value;
    maximum = max_value;
    step = step_value;
    return *this;
}

PropertyEditorItem& PropertyEditorItem::SetDefault(const Value& v, bool can_reset)
{
    default_value = v;
    resettable = can_reset;
    return *this;
}

PropertyEditorItem& PropertyEditorItem::AddChoice(const Value& v,
                                                   const String& text,
                                                   const Image& icon)
{
    choices.Add(PropertyEditorChoice(v, text, icon));
    return *this;
}

PropertyEditorItem& PropertyEditorItem::SetMixed(bool on)
{
    mixed = on;
    return *this;
}

PropertyEditorItem& PropertyEditorItem::SetInherited(bool on)
{
    inherited = on;
    return *this;
}

PropertyEditorItem& PropertyEditorItem::SetEnabled(bool on)
{
    enabled = on;
    return *this;
}

PropertyEditorItem& PropertyEditorItem::SetVisible(bool on)
{
    visible = on;
    return *this;
}

PropertyEditorItem& PropertyEditorItem::SetReadOnly(bool on)
{
    read_only = on;
    return *this;
}

PropertyEditorItem& PropertyEditorItem::SetHelp(const String& text)
{
    help = text;
    return *this;
}

PropertyEditorItem& PropertyEditorItem::SetUnit(const String& text)
{
    unit = text;
    return *this;
}

PropertyEditorItem& PropertyEditorItem::SetImpact(PropertyEditorImpact value)
{
    impact = value;
    return *this;
}

PropertyEditorItem& PropertyEditorItem::SetDomain(PropertyEditorDomain value)
{
    domain = value;
    return *this;
}

void PropertyEditorModel::Clear(bool notify)
{
    items_.Clear();
    if(notify)
        StructureChanged();
}

PropertyEditorItem& PropertyEditorModel::Add(const String& id,
                                             const String& label,
                                             PropertyEditorKind kind,
                                             const Value& value,
                                             const String& group)
{
    PropertyEditorItem& item = items_.Add();
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

PropertyEditorItem& PropertyEditorModel::AddText(const String& id,
                                                 const String& label,
                                                 const String& value,
                                                 const String& group)
{
    return Add(id, label, PropertyEditorKind::Text, value, group);
}

PropertyEditorItem& PropertyEditorModel::AddMultiline(const String& id,
                                                      const String& label,
                                                      const String& value,
                                                      const String& group)
{
    return Add(id, label, PropertyEditorKind::Multiline, value, group);
}

PropertyEditorItem& PropertyEditorModel::AddInteger(const String& id,
                                                    const String& label,
                                                    int value,
                                                    const String& group)
{
    return Add(id, label, PropertyEditorKind::Integer, value, group);
}

PropertyEditorItem& PropertyEditorModel::AddDouble(const String& id,
                                                   const String& label,
                                                   double value,
                                                   const String& group)
{
    return Add(id, label, PropertyEditorKind::Double, value, group);
}

PropertyEditorItem& PropertyEditorModel::AddBoolean(const String& id,
                                                    const String& label,
                                                    bool value,
                                                    const String& group)
{
    return Add(id, label, PropertyEditorKind::Boolean, value, group);
}

PropertyEditorItem& PropertyEditorModel::AddChoice(const String& id,
                                                   const String& label,
                                                   const Value& value,
                                                   const String& group)
{
    return Add(id, label, PropertyEditorKind::Choice, value, group);
}

PropertyEditorItem& PropertyEditorModel::AddColor(const String& id,
                                                  const String& label,
                                                  Color value,
                                                  const String& group)
{
    return Add(id, label, PropertyEditorKind::Color, value, group);
}

PropertyEditorItem& PropertyEditorModel::AddSlider(const String& id,
                                                   const String& label,
                                                   double value,
                                                   double minimum,
                                                   double maximum,
                                                   double step,
                                                   const String& group)
{
    PropertyEditorItem& item =
        Add(id, label, PropertyEditorKind::SliderDouble, value, group);
    item.SetRange(minimum, maximum, step);
    return item;
}

PropertyEditorItem& PropertyEditorModel::AddSliderInt(const String& id,
                                                      const String& label,
                                                      int value,
                                                      int minimum,
                                                      int maximum,
                                                      int step,
                                                      const String& group)
{
    PropertyEditorItem& item =
        Add(id, label, PropertyEditorKind::SliderInt, value, group);
    item.SetRange(minimum, maximum, step);
    return item;
}

PropertyEditorItem& PropertyEditorModel::AddVector2(const String& id,
                                                    const String& label,
                                                    double x, double y,
                                                    const String& group)
{
    return Add(id, label, PropertyEditorKind::Vector2,
               PropertyEditorMakeVector(x, y), group);
}

PropertyEditorItem& PropertyEditorModel::AddVector3(const String& id,
                                                    const String& label,
                                                    double x, double y, double z,
                                                    const String& group)
{
    return Add(id, label, PropertyEditorKind::Vector3,
               PropertyEditorMakeVector(x, y, z), group);
}

PropertyEditorItem& PropertyEditorModel::AddCurve(const String& id,
                                                  const String& label,
                                                  const Value& curve,
                                                  const String& group)
{
    return Add(id, label, PropertyEditorKind::Curve,
               PropertyEditorNormalizeCurve(curve), group);
}

PropertyEditorItem& PropertyEditorModel::AddReadOnly(const String& id,
                                                     const String& label,
                                                     const Value& value,
                                                     const String& group)
{
    PropertyEditorItem& item =
        Add(id, label, PropertyEditorKind::ReadOnly, value, group);
    item.read_only = true;
    return item;
}

PropertyEditorItem* PropertyEditorModel::Find(const String& id)
{
    int q = FindIndex(id);
    return q >= 0 ? &items_[q] : nullptr;
}

const PropertyEditorItem* PropertyEditorModel::Find(const String& id) const
{
    int q = FindIndex(id);
    return q >= 0 ? &items_[q] : nullptr;
}

int PropertyEditorModel::FindIndex(const String& id) const
{
    for(int i = 0; i < items_.GetCount(); i++)
        if(items_[i].id == id)
            return i;
    return -1;
}

bool PropertyEditorModel::SetValue(const String& id, const Value& value, bool notify)
{
    PropertyEditorItem* item = Find(id);
    if(!item)
        return false;
    item->value = value;
    item->mixed = false;
    if(notify)
        ValueChanged(id);
    return true;
}

bool PropertyEditorModel::SetMixed(const String& id, bool mixed, bool notify)
{
    PropertyEditorItem* item = Find(id);
    if(!item)
        return false;
    item->mixed = mixed;
    if(notify)
        ValueChanged(id);
    return true;
}

bool PropertyEditorModel::SetEnabled(const String& id, bool enabled, bool notify)
{
    PropertyEditorItem* item = Find(id);
    if(!item)
        return false;
    item->enabled = enabled;
    if(notify)
        ValueChanged(id);
    return true;
}

bool PropertyEditorModel::SetVisible(const String& id, bool visible, bool notify)
{
    PropertyEditorItem* item = Find(id);
    if(!item)
        return false;
    if(item->visible == visible)
        return true;
    item->visible = visible;
    if(notify)
        StructureChanged();
    return true;
}

bool PropertyEditorModel::SetValidationError(const String& id,
                                             const String& error,
                                             bool notify)
{
    PropertyEditorItem* item = Find(id);
    if(!item)
        return false;
    item->validation_error = error;
    if(notify)
        ValueChanged(id);
    return true;
}

bool PropertyEditorModel::Preview(const String& id,
                                  const Value& candidate,
                                  String *error)
{
    return Apply(id, candidate, false, error);
}

bool PropertyEditorModel::Commit(const String& id,
                                 const Value& candidate,
                                 String *error)
{
    return Apply(id, candidate, true, error);
}

bool PropertyEditorModel::Reset(const String& id, String *error)
{
    PropertyEditorItem* item = Find(id);
    if(!item) {
        if(error) *error = "Unknown property: " + id;
        return false;
    }
    if(!item->resettable) {
        if(error) *error = "Property is not resettable";
        return false;
    }

    String local_error;
    if(!Apply(id, item->default_value, true, &local_error)) {
        if(error) *error = local_error;
        return false;
    }

    item->inherited = true;
    ValueChanged(id);
    WhenReset(id);
    if(error) error->Clear();
    return true;
}

bool PropertyEditorModel::Apply(const String& id,
                                const Value& candidate,
                                bool final_commit,
                                String *error)
{
    PropertyEditorItem* item = Find(id);
    if(!item) {
        if(error) *error = "Unknown property: " + id;
        return false;
    }
    if(!item->visible) {
        if(error) *error = "Property is hidden";
        return false;
    }
    if(!item->enabled || item->read_only || item->kind == PropertyEditorKind::ReadOnly) {
        if(error) *error = "Property is read only";
        return false;
    }

    Value normalized;
    String local_error;
    if(!PropertyEditorNormalizeValue(*item, candidate, normalized, local_error)) {
        item->validation_error = local_error;
        ValueChanged(id);
        if(error) *error = local_error;
        return false;
    }

    item->validation_error.Clear();
    item->value = normalized;
    item->mixed = false;
    item->inherited = false;
    ValueChanged(id);

    if(final_commit)
        WhenCommit(id, normalized);
    else
        WhenPreview(id, normalized);

    if(error)
        error->Clear();
    return true;
}

void PropertyEditorModel::StructureChanged()
{
    structure_revision_++;
    WhenStructureChanged();
}

void PropertyEditorModel::ValueChanged(const String& id)
{
    value_revision_++;
    WhenValueChanged(id);
}

Value PropertyEditorMakeVector(double x, double y)
{
    ValueArray values;
    values.Add(x);
    values.Add(y);
    return values;
}

Value PropertyEditorMakeVector(double x, double y, double z)
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

Vector<double> PropertyEditorReadVector(const Value& value,
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

String PropertyEditorFormatVector(const Value& value,
                                  int expected_count,
                                  int decimals)
{
    Vector<double> values = PropertyEditorReadVector(value, expected_count);
    String out;
    for(int i = 0; i < values.GetCount(); i++) {
        if(i)
            out << ", ";
        out << Format("%.*f", max(0, decimals), values[i]);
    }
    return out;
}

Value PropertyEditorMakeCurve(const Vector<Pointf>& points)
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

Vector<Pointf> PropertyEditorReadCurve(const Value& value)
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

Value PropertyEditorNormalizeCurve(const Value& value)
{
    Vector<Pointf> points = PropertyEditorReadCurve(value);
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

    return PropertyEditorMakeCurve(unique);
}

String PropertyEditorFormatCurve(const Value& value)
{
    Vector<Pointf> points = PropertyEditorReadCurve(PropertyEditorNormalizeCurve(value));
    return Format("%d point%s", points.GetCount(), points.GetCount() == 1 ? "" : "s");
}

bool PropertyEditorNormalizeValue(const PropertyEditorItem& item,
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
    case PropertyEditorKind::Text:
    case PropertyEditorKind::Multiline:
        normalized = AsString(candidate);
        break;

    case PropertyEditorKind::Integer:
    case PropertyEditorKind::SliderInt: {
        int v = 0;
        if(!PeParseInt(candidate, v)) {
            error = "Expected an integer value";
            return false;
        }
        normalized = PeClampNumber(item, v, true);
        break;
    }

    case PropertyEditorKind::Double:
    case PropertyEditorKind::SliderDouble: {
        double v = 0;
        if(!PeParseDouble(candidate, v)) {
            error = "Expected a numeric value";
            return false;
        }
        normalized = PeClampNumber(item, v, false);
        break;
    }

    case PropertyEditorKind::Boolean: {
        bool v = false;
        if(!PeParseBool(candidate, v)) {
            error = "Expected a Boolean value";
            return false;
        }
        normalized = v;
        break;
    }

    case PropertyEditorKind::Choice: {
        bool found = item.choices.IsEmpty();
        for(const PropertyEditorChoice& choice : item.choices)
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

    case PropertyEditorKind::Color:
        if(candidate.GetType() != COLOR_V && !IsNull(candidate)) {
            error = "Expected a Color value";
            return false;
        }
        normalized = candidate;
        break;

    case PropertyEditorKind::Vector2: {
        Vector<double> v;
        if(!PeTryReadVector(candidate, 2, v)) {
            error = "Expected two numeric components";
            return false;
        }
        normalized = PropertyEditorMakeVector(v[0], v[1]);
        break;
    }

    case PropertyEditorKind::Vector3: {
        Vector<double> v;
        if(!PeTryReadVector(candidate, 3, v)) {
            error = "Expected three numeric components";
            return false;
        }
        normalized = PropertyEditorMakeVector(v[0], v[1], v[2]);
        break;
    }

    case PropertyEditorKind::Curve:
        normalized = PropertyEditorNormalizeCurve(candidate);
        break;

    case PropertyEditorKind::ReadOnly:
        error = "Property is read only";
        return false;

    case PropertyEditorKind::Custom:
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

String PropertyEditorKindName(PropertyEditorKind kind)
{
    switch(kind) {
    case PropertyEditorKind::Text: return "Text";
    case PropertyEditorKind::Multiline: return "Multiline";
    case PropertyEditorKind::Integer: return "Integer";
    case PropertyEditorKind::Double: return "Double";
    case PropertyEditorKind::Boolean: return "Boolean";
    case PropertyEditorKind::Choice: return "Choice";
    case PropertyEditorKind::Color: return "Color";
    case PropertyEditorKind::SliderInt: return "SliderInt";
    case PropertyEditorKind::SliderDouble: return "SliderDouble";
    case PropertyEditorKind::Vector2: return "Vector2";
    case PropertyEditorKind::Vector3: return "Vector3";
    case PropertyEditorKind::Curve: return "Curve";
    case PropertyEditorKind::ReadOnly: return "ReadOnly";
    case PropertyEditorKind::Custom: return "Custom";
    }
    return "Unknown";
}

String PropertyEditorDomainName(PropertyEditorDomain domain)
{
    switch(domain) {
    case PropertyEditorDomain::General: return "General";
    case PropertyEditorDomain::Content: return "Content";
    case PropertyEditorDomain::Behaviour: return "Behaviour";
    case PropertyEditorDomain::Layout: return "Layout";
    case PropertyEditorDomain::Appearance: return "Appearance";
    case PropertyEditorDomain::Theme: return "Theme";
    case PropertyEditorDomain::Runtime: return "Runtime";
    case PropertyEditorDomain::DesignerOnly: return "DesignerOnly";
    }
    return "Unknown";
}

String PropertyEditorImpactName(PropertyEditorImpact impact)
{
    if(impact == PropertyImpactNone)
        return "None";

    struct Name {
        PropertyEditorImpact flag;
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
