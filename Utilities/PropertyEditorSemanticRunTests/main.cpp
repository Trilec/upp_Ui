#include <Core/Core.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>

using namespace Upp;

namespace {

int checks = 0;
int failed = 0;

void Check(bool condition, const char *message)
{
    checks++;
    if(condition)
        return;
    failed++;
    Cout() << "FAIL: " << message << '\n';
}

Value MapValue(const ValueMap& map, const char *key, const Value& fallback = Value())
{
    int q = map.Find(key);
    return q >= 0 ? map.GetValue(q) : fallback;
}

}

CONSOLE_APP_MAIN
{
    PropertyEditorFactory factory;
    RegisterPropertyEditorEditors(factory);

    Check(factory.HasCustom(PropertyEditorRangeDoubleId()), "unified registration preserves range adapter");
    Check(factory.HasCustom(PropertyEditorMatrixId()), "unified registration preserves matrix adapter");
    Check(factory.HasCustom(PropertyEditorDateTimeId()), "date/time adapter registered");
    Check(factory.HasCustom(PropertyEditorDurationId()), "duration adapter registered");
    Check(factory.HasCustom(PropertyEditorGeometryId()), "geometry adapter registered");
    Check(factory.HasCustom(PropertyEditorFlagsId()), "flags adapter registered");
    Check(factory.HasCustom(PropertyEditorStringListId()), "ordered list adapter registered");
    Check(factory.HasCustom(PropertyEditorGradientId()), "gradient adapter registered");
    Check(factory.HasCustom(PropertyEditorKeyChordId()), "key chord adapter registered");
    Check(factory.HasCustom(PropertyEditorReferenceId()), "reference adapter registered");
    Check(factory.HasCustom(PropertyEditorOptionalId()), "optional adapter registered");

    PropertyEditorModel model;
    AddPropertyDate(model, "date", "Date", Date(2026, 8, 23), "Time");
    AddPropertyTime(model, "time", "Time", Time(1970, 1, 1, 14, 25, 42), false, "Time");
    AddPropertyDateTime(model, "datetime", "Date Time", Time(2026, 8, 23, 14, 25, 42), true, "Time");
    AddPropertyDuration(model, "duration", "Duration", 1.5, 0.0, 10.0, 0.25, "Time");

    Check(model.Find("date") && model.Find("date")->custom_editor == PropertyEditorDateTimeId() &&
          model.Find("date")->editor_variant == "date-only", "Date uses semantic date adapter variant");
    Check(model.Find("time") && model.Find("time")->editor_variant == "time-only", "Time uses time-only variant");
    Check(model.Find("datetime") && model.Find("datetime")->editor_variant == "date-time.seconds", "DateTime retains seconds variant");

    String error;
    Check(model.Commit("duration", 11.0, &error), "duration commit normalizes rather than rejects range overflow");
    Check(fabs((double)model.Find("duration")->value - 10.0) < 1e-9, "duration clamps to declared maximum");
    Check(model.Commit("duration", 1.37, &error), "duration accepts numeric candidate");
    Check(fabs((double)model.Find("duration")->value - 1.25) < 1e-9, "duration snaps in durable seconds");

    AddPropertyPoint(model, "point", "Point", 12, 24, "Geometry");
    AddPropertySize(model, "size", "Size", 640, 480, "Geometry");
    AddPropertyRect(model, "rect", "Rect", 10, 20, 300, 180, "Geometry");
    AddPropertyInsets(model, "insets", "Insets", 8, 12, 8, 12, true, "Geometry");
    AddPropertyCorners(model, "corners", "Corners", 8, 8, 8, 8, true, "Geometry");

    Check(model.Find("point") && model.Find("point")->editor_variant == "point", "Point semantic variant declared");
    Check(model.Find("size") && ValueArray(model.Find("size")->value).GetCount() == 2, "Size stores two semantic components");
    Check(model.Find("rect") && ValueArray(model.Find("rect")->value).GetCount() == 4, "Rect stores four semantic components");
    Check(model.Find("insets") && model.Find("insets")->editor_variant == "insets.linked", "Insets exposes linked editing mode");
    Check(model.Find("corners") && model.Find("corners")->editor_variant == "corners.linked", "Corners exposes linked editing mode");

    ValueArray malformed_geometry;
    malformed_geometry.Add(1);
    malformed_geometry.Add(2);
    Check(!model.Commit("rect", malformed_geometry, &error), "geometry rejects a malformed component count");
    Check(ValueArray(model.Find("rect")->value).GetCount() == 4, "rejected geometry leaves the previous valid value intact");

    ValueArray selected;
    selected.Add("icon");
    PropertyEditorItem& flags = AddPropertyFlags(model, "flags", "Features", selected, "Selection");
    flags.AddChoice("icon", "Icon").AddChoice("text", "Text").AddChoice("badge", "Badge");
    Check(flags.choices.GetCount() == 3, "flags retain explicit multi-choice domain metadata");
    ValueArray flag_candidate;
    flag_candidate.Add("text"); flag_candidate.Add("text");
    Check(model.Commit("flags", flag_candidate, &error), "flags candidate normalizes");
    ValueArray normalized_flags = model.Find("flags")->value;
    Check(normalized_flags.GetCount() == 1 && normalized_flags[0] == "text", "flags de-duplicate selected values");

    ValueArray list;
    list.Add("Alpha"); list.Add("Beta");
    AddPropertyStringList(model, "list", "Ordered", list, 3, "Selection");
    ValueArray list_candidate;
    list_candidate.Add("One"); list_candidate.Add(2); list_candidate.Add("Three"); list_candidate.Add("Four");
    Check(model.Commit("list", list_candidate, &error), "ordered list normalizes");
    ValueArray normalized_list = model.Find("list")->value;
    Check(normalized_list.GetCount() == 3 && AsString(normalized_list[1]) == "2", "ordered list stringifies and enforces maximum item count");

    ValueArray stops;
    stops.Add(PropertyEditorMakeGradientStop(1.2, Color(255, 0, 0), 300));
    stops.Add(PropertyEditorMakeGradientStop(-0.2, Color(0, 0, 255), -4));
    Value gradient = PropertyEditorMakeGradient(stops, "Linear", 450.0, "Smooth");
    AddPropertyGradient(model, "gradient", "Gradient", gradient, "Appearance");
    ValueMap gradient_map = model.Find("gradient")->value;
    Check(AsString(MapValue(gradient_map, "mode")) == "Linear", "gradient preserves supported mode");
    Check(fabs((double)MapValue(gradient_map, "angle") - 90.0) < 1e-9, "gradient normalizes angle");
    Check(AsString(MapValue(gradient_map, "interpolation")) == "Smooth", "gradient preserves interpolation mode");
    ValueArray normalized_stops = MapValue(gradient_map, "stops");
    Check(normalized_stops.GetCount() == 2, "gradient retains ordered stop collection");
    ValueMap first_stop = normalized_stops[0];
    ValueMap second_stop = normalized_stops[1];
    Check((double)MapValue(first_stop, "position") == 0.0 && (double)MapValue(second_stop, "position") == 1.0,
          "gradient stops are clamped and sorted");
    Check((int)MapValue(first_stop, "alpha") == 0 && (int)MapValue(second_stop, "alpha") == 255,
          "gradient stop alpha is clamped");

    AddPropertyKeyChord(model, "shortcut", "Shortcut", "shift+ctrl+s", "Input");
    Check(AsString(model.Find("shortcut")->value) == "Ctrl+Shift+S", "key chord constructor canonicalizes modifier order");
    Check(model.Commit("shortcut", "alt+control+f5", &error), "key chord commit accepted");
    Check(AsString(model.Find("shortcut")->value) == "Ctrl+Alt+F5", "key chord commit canonicalized");

    AddPropertyReference(model, "asset", "Asset", "asset:hero", "test-reference", "Resources");
    Check(model.Find("asset") && model.Find("asset")->picker_provider == "test-reference", "reference preserves application provider id");

    AddPropertyOptional(model, "optional_text", "Optional", Null, "fallback", "text", "States");
    Check(model.Find("optional_text") && model.Find("optional_text")->allow_null, "optional adapter explicitly permits null");
    Check(model.Commit("optional_text", Null, &error) && IsNull(model.Find("optional_text")->value), "optional value commits Null as durable state");
    Check(model.Reset("optional_text", &error), "optional property reset succeeds");
    Check(AsString(model.Find("optional_text")->value) == "fallback", "optional reset restores explicit fallback/default");

    Cout() << Format("PROPERTYEDITOR_SEMANTIC_SUMMARY checks=%d failed=%d\n", checks, failed);
    SetExitCode(failed ? 1 : 0);
}
