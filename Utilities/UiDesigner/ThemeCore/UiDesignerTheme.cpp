#include "UiDesignerTheme.h"

namespace Upp {

static bool IsThemePresetName(const String& value)
{
    static const char *names[] = {
        "Minimal", "Pill", "Linear", "Solid", "Outline", "Compact", "Layered"
    };
    for(int i = 0; i < __countof(names); i++)
        if(value == names[i])
            return true;
    return false;
}

static bool IsThemeModeName(const String& value)
{
    return value == "Light" || value == "Dark" || value == "System";
}

static String ThemeColorText(Color color)
{
    return Format("#%02X%02X%02X", color.GetR(), color.GetG(), color.GetB());
}

static bool ParseThemeColor(const Value& value, Color& color)
{
    if(value.Is<Color>()) {
        color = value;
        return true;
    }
    String text = value;
    if(text.GetCount() != 7 || text[0] != '#')
        return false;
    int digit[6];
    for(int i = 0; i < 6; i++) {
        int c = text[i + 1];
        if(c >= '0' && c <= '9') digit[i] = c - '0';
        else if(c >= 'a' && c <= 'f') digit[i] = c - 'a' + 10;
        else if(c >= 'A' && c <= 'F') digit[i] = c - 'A' + 10;
        else return false;
    }
    color = Color(digit[0] * 16 + digit[1], digit[2] * 16 + digit[3],
                  digit[4] * 16 + digit[5]);
    return true;
}

ValueMap UiDesignerThemeSnapshot::ToValue() const
{
    ValueMap out;
    out.Set("preset", preset);
    out.Set("mode", mode);
    out.Set("accent", ThemeColorText(accent));
    out.Set("spacing", spacing);
    out.Set("radius", radius);
    out.Set("pill_radius", pill_radius);
    out.Set("border_width", border_width);
    out.Set("shadows", shadows);
    out.Set("shadow_distance", shadow_distance);
    out.Set("shadow_offset_y", shadow_offset_y);
    out.Set("shadow_alpha", shadow_alpha);
    return out;
}

bool UiDesignerThemeSnapshot::FromValue(const Value& value, String& error)
{
    if(!value.Is<ValueMap>()) {
        error = "Theme root must be an object";
        return false;
    }
    ValueMap in = value;
    preset = UiDesignerMapValue(in, "preset", "Minimal");
    mode = UiDesignerMapValue(in, "mode", "Light");
    if(!IsThemePresetName(preset))
        preset = "Minimal";
    if(!IsThemeModeName(mode))
        mode = "Light";
    Color parsed_accent;
    if(ParseThemeColor(UiDesignerMapValue(in, "accent", "#3A84FF"), parsed_accent))
        accent = parsed_accent;
    else
        accent = Color(58, 132, 255);
    spacing = UiDesignerMapValue(in, "spacing", 8);
    radius = UiDesignerMapValue(in, "radius", 8);
    pill_radius = UiDesignerMapValue(in, "pill_radius", 25);
    border_width = UiDesignerMapValue(in, "border_width", 1);
    shadows = UiDesignerMapValue(in, "shadows", true);
    shadow_distance = UiDesignerMapValue(in, "shadow_distance", 6);
    shadow_offset_y = UiDesignerMapValue(in, "shadow_offset_y", 2);
    shadow_alpha = UiDesignerMapValue(in, "shadow_alpha", 24);
    error.Clear();
    return true;
}

UiDesignerThemeDocument::UiDesignerThemeDocument()
{
    preview_ = value_;
}

static void AddThemeChoice(PropertyEditorModel& model, const String& id,
                           const String& label, const Value& value,
                           const char *const *choices, int count)
{
    PropertyEditorItem& item = model.AddChoice(id, label, value, "Theme");
    for(int i = 0; i < count; i++)
        item.AddChoice(choices[i], choices[i]);
    item.SetDomain(PropertyEditorDomain::Theme)
        .SetImpact(PropertyImpactThemeGlobal |
                   PropertyImpactPaint |
                   PropertyImpactFullPreview);
}

void UiDesignerThemeDocument::BuildPropertyModel(
    PropertyEditorModel& model) const
{
    const UiDesignerThemeSnapshot& t = GetEffective();
    model.Clear(false);

    static const char *preset_choices[] = {
        "Minimal", "Pill", "Linear", "Solid", "Outline", "Compact", "Layered"
    };
    static const char *mode_choices[] = {"Light", "Dark", "System"};
    AddThemeChoice(model, "preset", "Preset", t.preset,
                   preset_choices, __countof(preset_choices));
    AddThemeChoice(model, "mode", "Mode", t.mode,
                   mode_choices, __countof(mode_choices));

    model.AddColor("accent", "Accent", t.accent, "Palette")
        .SetDefault(Color(58, 132, 255))
        .SetDomain(PropertyEditorDomain::Theme)
        .SetImpact(PropertyImpactThemeGlobal | PropertyImpactPaint);

    model.AddSliderInt("spacing", "Spacing", t.spacing, 0, 32, 1,
                       "Metrics")
        .SetDefault(8)
        .SetDomain(PropertyEditorDomain::Theme)
        .SetImpact(PropertyImpactThemeGlobal |
                   PropertyImpactLocalLayout);

    model.AddSliderInt("radius", "Surface radius", t.radius, 0, 32, 1,
                       "Metrics")
        .SetDefault(8)
        .SetDomain(PropertyEditorDomain::Theme)
        .SetImpact(PropertyImpactThemeGlobal | PropertyImpactPaint);

    model.AddSliderInt("pill_radius", "Pill radius", t.pill_radius, 0, 40, 1,
                       "Metrics")
        .SetDefault(25)
        .SetHelp("The authored icon-holder capsule radius.")
        .SetDomain(PropertyEditorDomain::Theme)
        .SetImpact(PropertyImpactThemeGlobal | PropertyImpactPaint);

    model.AddSliderInt("border_width", "Border width", t.border_width, 0, 6, 1,
                       "Metrics")
        .SetDefault(1)
        .SetDomain(PropertyEditorDomain::Theme)
        .SetImpact(PropertyImpactThemeGlobal | PropertyImpactPaint);

    model.AddBoolean("shadows", "Shadows", t.shadows, "Shadow")
        .SetDefault(true)
        .SetDomain(PropertyEditorDomain::Theme)
        .SetImpact(PropertyImpactThemeGlobal | PropertyImpactPaint);

    model.AddSliderInt("shadow_distance", "Distance", t.shadow_distance, 0, 24, 1,
                       "Shadow")
        .SetDefault(6)
        .SetDomain(PropertyEditorDomain::Theme)
        .SetImpact(PropertyImpactThemeGlobal | PropertyImpactPaint);

    model.AddSliderInt("shadow_offset_y", "Y offset", t.shadow_offset_y, -12, 12, 1,
                       "Shadow")
        .SetDefault(2)
        .SetDomain(PropertyEditorDomain::Theme)
        .SetImpact(PropertyImpactThemeGlobal | PropertyImpactPaint);

    model.AddSliderInt("shadow_alpha", "Alpha", t.shadow_alpha, 0, 255, 1,
                       "Shadow")
        .SetDefault(24)
        .SetDomain(PropertyEditorDomain::Theme)
        .SetImpact(PropertyImpactThemeGlobal | PropertyImpactPaint);

    model.StructureChanged();
}

Value UiDesignerThemeDocument::GetProperty(
    const UiDesignerThemeSnapshot& source, const String& property) const
{
    if(property == "preset") return source.preset;
    if(property == "mode") return source.mode;
    if(property == "accent") return source.accent;
    if(property == "spacing") return source.spacing;
    if(property == "radius") return source.radius;
    if(property == "pill_radius") return source.pill_radius;
    if(property == "border_width") return source.border_width;
    if(property == "shadows") return source.shadows;
    if(property == "shadow_distance") return source.shadow_distance;
    if(property == "shadow_offset_y") return source.shadow_offset_y;
    if(property == "shadow_alpha") return source.shadow_alpha;
    return Value();
}

bool UiDesignerThemeDocument::SetProperty(
    UiDesignerThemeSnapshot& target, const String& property,
    const Value& value, String& error) const
{
    if(property == "preset") {
        const String preset = value;
        if(!IsThemePresetName(preset)) {
            error = "Unknown theme preset: " + preset;
            return false;
        }
        target.preset = preset;
    }
    else if(property == "mode") {
        const String mode = value;
        if(!IsThemeModeName(mode)) {
            error = "Unknown theme mode: " + mode;
            return false;
        }
        target.mode = mode;
    }
    else if(property == "accent") target.accent = (Color)value;
    else if(property == "spacing") target.spacing = minmax((int)value, 0, 32);
    else if(property == "radius") target.radius = minmax((int)value, 0, 32);
    else if(property == "pill_radius") target.pill_radius = minmax((int)value, 0, 40);
    else if(property == "border_width") target.border_width = minmax((int)value, 0, 6);
    else if(property == "shadows") target.shadows = (bool)value;
    else if(property == "shadow_distance") target.shadow_distance = minmax((int)value, 0, 24);
    else if(property == "shadow_offset_y") target.shadow_offset_y = minmax((int)value, -12, 12);
    else if(property == "shadow_alpha") target.shadow_alpha = minmax((int)value, 0, 255);
    else {
        error = "Unknown theme property: " + property;
        return false;
    }
    error.Clear();
    return true;
}

bool UiDesignerThemeDocument::Preview(
    const String& property, const Value& value, String& error)
{
    preview_ = preview_active_ ? preview_ : value_;
    if(!SetProperty(preview_, property, value, error))
        return false;
    preview_active_ = true;
    WhenPreview();
    return true;
}

void UiDesignerThemeDocument::TruncateRedo()
{
    while(history_.GetCount() > position_)
        history_.Remove(history_.GetCount() - 1);
    if(saved_position_ > position_)
        saved_position_ = -1;
}

bool UiDesignerThemeDocument::Commit(
    const String& property, const Value& value,
    const String& label, String& error)
{
    UiDesignerThemeSnapshot after = value_;
    if(!SetProperty(after, property, value, error))
        return false;

    if(after.ToValue() == value_.ToValue()) {
        preview_ = value_;
        preview_active_ = false;
        error.Clear();
        return true;
    }

    TruncateRedo();
    UiDesignerThemeHistoryEntry& entry = history_.Add();
    entry.label = label.IsEmpty() ? "Set " + property : label;
    entry.before = value_;
    entry.after = after;

    value_ = after;
    preview_ = value_;
    preview_active_ = false;
    position_ = history_.GetCount();
    WhenChanged();
    WhenHistoryChanged();
    return true;
}

void UiDesignerThemeDocument::CancelPreview()
{
    if(!preview_active_)
        return;
    preview_ = value_;
    preview_active_ = false;
    WhenPreview();
}

bool UiDesignerThemeDocument::Reset(
    const String& property, String& error)
{
    UiDesignerThemeSnapshot defaults;
    return Commit(property, GetProperty(defaults, property),
                  "Reset " + property, error);
}

bool UiDesignerThemeDocument::Undo()
{
    if(!CanUndo())
        return false;
    value_ = history_[position_ - 1].before;
    preview_ = value_;
    preview_active_ = false;
    position_--;
    WhenChanged();
    WhenHistoryChanged();
    return true;
}

bool UiDesignerThemeDocument::Redo()
{
    if(!CanRedo())
        return false;
    value_ = history_[position_].after;
    preview_ = value_;
    preview_active_ = false;
    position_++;
    WhenChanged();
    WhenHistoryChanged();
    return true;
}

bool UiDesignerThemeDocument::Replace(
    const UiDesignerThemeSnapshot& value, bool mark_saved)
{
    value_ = value;
    preview_ = value;
    preview_active_ = false;
    history_.Clear();
    position_ = 0;
    saved_position_ = mark_saved ? 0 : -1;
    WhenChanged();
    WhenHistoryChanged();
    return true;
}

String UiDesignerThemeDocument::Serialize(bool pretty) const
{
    ValueMap root;
    root.Set("format", "upp-ui-theme-designer");
    root.Set("schema", 1);
    root.Set("theme", value_.ToValue());
    return AsJSON(root, pretty);
}

bool UiDesignerThemeDocument::Deserialize(
    const String& json, String& error)
{
    Value parsed = ParseJSON(json);
    if(IsError(parsed)) {
        error = GetErrorText(parsed);
        return false;
    }
    if(!parsed.Is<ValueMap>()) {
        error = "Theme root must be an object";
        return false;
    }
    ValueMap root = parsed;
    if((String)UiDesignerMapValue(root, "format", "") != "upp-ui-theme-designer") {
        error = "Unsupported theme document";
        return false;
    }
    UiDesignerThemeSnapshot loaded;
    if(!loaded.FromValue(UiDesignerMapValue(root, "theme", ValueMap()), error))
        return false;
    value_ = loaded;
    preview_ = loaded;
    preview_active_ = false;
    history_.Clear();
    position_ = saved_position_ = 0;
    WhenChanged();
    WhenHistoryChanged();
    return true;
}

}
