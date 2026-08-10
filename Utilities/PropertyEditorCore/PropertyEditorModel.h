#ifndef _Utilities_PropertyEditor_PropertyEditorModel_h_
#define _Utilities_PropertyEditor_PropertyEditorModel_h_

/*
    Generic Property Editor utility
    --------------------------------
    Headless property schema and value model.

    License: Apache License 2.0, matching the repository LICENSE.
    Thread context: model mutation is expected on the GUI thread unless the
    owning application provides its own synchronization.
*/

#include <Draw/Draw.h>

namespace Upp {

enum class PropertyEditorKind : byte {
    Text = 0,
    Multiline,
    Integer,
    Double,
    NumericInt,
    NumericDouble,
    Boolean,
    Choice,
    Color,
    ColorPalette,
    FillRecipe,
    FilePath,
    SliderInt,
    SliderDouble,
    Vector2,
    Vector3,
    Curve,
    ReadOnly,
    Custom,
};

enum class PropertyBooleanPresentation : byte {
    Check = 0,
    OnOff,
    TrueFalse,
};

enum class PropertyEditorDomain : byte {
    General = 0,
    Content,
    Behaviour,
    Layout,
    Appearance,
    Theme,
    Runtime,
    DesignerOnly,
};

enum PropertyEditorImpact : dword {
    PropertyImpactNone             = 0,
    PropertyImpactPaint            = 1 << 0,
    PropertyImpactControlState     = 1 << 1,
    PropertyImpactLocalLayout      = 1 << 2,
    PropertyImpactAncestorLayout   = 1 << 3,
    PropertyImpactSubtree          = 1 << 4,
    PropertyImpactStructure        = 1 << 5,
    PropertyImpactSelection        = 1 << 6,
    PropertyImpactInspectorSchema  = 1 << 7,
    PropertyImpactCode             = 1 << 8,
    PropertyImpactThemeGlobal      = 1 << 9,
    PropertyImpactFullPreview      = 1 << 10,
};

inline PropertyEditorImpact operator|(PropertyEditorImpact a, PropertyEditorImpact b)
{
    return (PropertyEditorImpact)((dword)a | (dword)b);
}

inline PropertyEditorImpact& operator|=(PropertyEditorImpact& a, PropertyEditorImpact b)
{
    a = a | b;
    return a;
}

inline bool HasPropertyImpact(PropertyEditorImpact value, PropertyEditorImpact flag)
{
    return (((dword)value) & ((dword)flag)) != 0;
}

struct PropertyEditorChoice : Moveable<PropertyEditorChoice> {
    Value  value;
    String label;
    Image  icon;

    PropertyEditorChoice() {}
    PropertyEditorChoice(const Value& v, const String& text, const Image& image = Image())
        : value(v), label(text), icon(image) {}
};

struct PropertyEditorItem {
    String id;
    String label;
    String group;
    String help;
    String unit;
    String custom_editor;

    // Optional semantic information consumed by visual editor adapters. Core
    // deliberately treats these as opaque strings and never depends on Ui.
    String editor_variant;
    String picker_provider;

    PropertyEditorKind   kind = PropertyEditorKind::Text;
    PropertyEditorDomain domain = PropertyEditorDomain::General;
    PropertyEditorImpact impact = PropertyImpactNone;
    PropertyBooleanPresentation boolean_presentation = PropertyBooleanPresentation::Check;

    Value value;
    Value default_value;
    Value minimum;
    Value maximum;
    Value step;

    Array<PropertyEditorChoice> choices;

    bool visible = true;
    bool enabled = true;
    bool value_editable = true;
    bool read_only = false;
    bool resettable = false;
    bool mixed = false;
    bool inherited = false;
    bool allow_null = false;
    bool show_slider_toggle = false;
    bool inline_editor = false;
    bool overrideable = false;
    bool override_active = false;

    int indent = 0;
    int row_span = 0; // 0 = visual editor default, otherwise line-count multiple.
    int color_count = 1;
    int decimals = 3;
    int sort_order = 0;

    String validation_error;

    Function<Value(const Value&)> normalize;
    Function<String(const Value&)> validate;

    PropertyEditorItem& SetRange(const Value& min_value, const Value& max_value,
                                 const Value& step_value = Value());
    PropertyEditorItem& SetDefault(const Value& v, bool can_reset = true);
    PropertyEditorItem& AddChoice(const Value& v, const String& text,
                                  const Image& icon = Image());
    PropertyEditorItem& SetMixed(bool on = true);
    PropertyEditorItem& SetInherited(bool on = true);
    PropertyEditorItem& SetEnabled(bool on = true);
    PropertyEditorItem& SetVisible(bool on = true);
    PropertyEditorItem& SetReadOnly(bool on = true);
    PropertyEditorItem& SetHelp(const String& text);
    PropertyEditorItem& SetUnit(const String& text);
    PropertyEditorItem& SetImpact(PropertyEditorImpact value);
    PropertyEditorItem& SetDomain(PropertyEditorDomain value);
    PropertyEditorItem& SetColorCount(int count);
    PropertyEditorItem& SetSliderToggle(bool on = true);
    PropertyEditorItem& SetInlineEditor(bool on = true)
    {
        inline_editor = on;
        return *this;
    }
    PropertyEditorItem& SetRowSpan(int lines)
    {
        row_span = max(0, lines);
        return *this;
    }
    PropertyEditorItem& SetBooleanPresentation(PropertyBooleanPresentation presentation)
    {
        boolean_presentation = presentation;
        return *this;
    }
    PropertyEditorItem& SetEditorVariant(const String& variant)
    {
        editor_variant = variant;
        return *this;
    }
    PropertyEditorItem& SetPickerProvider(const String& provider)
    {
        picker_provider = provider;
        return *this;
    }
};

class PropertyEditorModel {
public:
    typedef PropertyEditorModel CLASSNAME;

    int GetCount() const { return items_.GetCount(); }
    bool IsEmpty() const { return items_.IsEmpty(); }

    void Clear(bool notify = true);

    PropertyEditorItem& Add(const String& id, const String& label,
                            PropertyEditorKind kind, const Value& value = Value(),
                            const String& group = String());

    PropertyEditorItem& AddText(const String& id, const String& label,
                                const String& value, const String& group = String());
    PropertyEditorItem& AddMultiline(const String& id, const String& label,
                                     const String& value, const String& group = String());
    PropertyEditorItem& AddInteger(const String& id, const String& label,
                                   int value, const String& group = String());
    PropertyEditorItem& AddDouble(const String& id, const String& label,
                                  double value, const String& group = String());
    PropertyEditorItem& AddNumericInt(const String& id, const String& label,
                                      int value, int minimum, int maximum,
                                      int step = 1, const String& group = String());
    PropertyEditorItem& AddNumericDouble(const String& id, const String& label,
                                         double value, double minimum, double maximum,
                                         double step = 0.0, const String& group = String());
    PropertyEditorItem& AddBoolean(const String& id, const String& label,
                                   bool value, const String& group = String());
    PropertyEditorItem& AddChoice(const String& id, const String& label,
                                  const Value& value, const String& group = String());
    PropertyEditorItem& AddColor(const String& id, const String& label,
                                 Color value, const String& group = String());
    PropertyEditorItem& AddSlider(const String& id, const String& label,
                                  double value, double minimum, double maximum,
                                  double step = 0.01, const String& group = String());
    PropertyEditorItem& AddSliderInt(const String& id, const String& label,
                                    int value, int minimum, int maximum,
                                    int step = 1, const String& group = String());
    PropertyEditorItem& AddVector2(const String& id, const String& label,
                                   double x, double y, const String& group = String());
    PropertyEditorItem& AddVector3(const String& id, const String& label,
                                   double x, double y, double z,
                                   const String& group = String());
    PropertyEditorItem& AddCurve(const String& id, const String& label,
                                 const Value& curve, const String& group = String());
    PropertyEditorItem& AddReadOnly(const String& id, const String& label,
                                    const Value& value, const String& group = String());

    PropertyEditorItem* Find(const String& id);
    const PropertyEditorItem* Find(const String& id) const;
    int FindIndex(const String& id) const;

    PropertyEditorItem& operator[](int i) { return items_[i]; }
    const PropertyEditorItem& operator[](int i) const { return items_[i]; }

    const Array<PropertyEditorItem>& GetItems() const { return items_; }

    bool SetValue(const String& id, const Value& value, bool notify = true);
    bool SetMixed(const String& id, bool mixed, bool notify = true);
    bool SetEnabled(const String& id, bool enabled, bool notify = true);
    bool SetVisible(const String& id, bool visible, bool notify = true);
    bool SetValidationError(const String& id, const String& error, bool notify = true);

    void SetGroupSubtitle(const String& group, const String& subtitle);
    String GetGroupSubtitle(const String& group) const;
    void ClearGroupSubtitle(const String& group);
    void ClearGroupSubtitles();

    bool Preview(const String& id, const Value& candidate, String *error = nullptr);
    bool Commit(const String& id, const Value& candidate, String *error = nullptr);
    bool Reset(const String& id, String *error = nullptr);

    int GetStructureRevision() const { return structure_revision_; }
    int GetValueRevision() const { return value_revision_; }

    void StructureChanged();
    void ValueChanged(const String& id);

    Event<> WhenStructureChanged;
    Event<String> WhenValueChanged;
    Event<String, Value> WhenPreview;
    Event<String, Value> WhenCommit;
    Event<String> WhenReset;
    Event<String> WhenGroupMetadataChanged;

private:
    bool Apply(const String& id, const Value& candidate, bool final_commit, String *error);

    Array<PropertyEditorItem> items_;
    int structure_revision_ = 0;
    int value_revision_ = 0;
    VectorMap<String, String> group_subtitles_;
};

Value PropertyEditorMakeVector(double x, double y);
Value PropertyEditorMakeVector(double x, double y, double z);
Vector<double> PropertyEditorReadVector(const Value& value, int expected_count,
                                        double fallback = 0.0);
String PropertyEditorFormatVector(const Value& value, int expected_count,
                                  int decimals = 3);

Value PropertyEditorMakeCurve(const Vector<Pointf>& points);
Vector<Pointf> PropertyEditorReadCurve(const Value& value);
Value PropertyEditorNormalizeCurve(const Value& value);
String PropertyEditorFormatCurve(const Value& value);

bool PropertyEditorNormalizeValue(const PropertyEditorItem& item,
                                  const Value& candidate,
                                  Value& normalized,
                                  String& error);

String PropertyEditorKindName(PropertyEditorKind kind);
String PropertyEditorDomainName(PropertyEditorDomain domain);
String PropertyEditorImpactName(PropertyEditorImpact impact);

}

#endif
