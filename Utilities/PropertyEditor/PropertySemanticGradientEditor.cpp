#include "PropertySemanticEditorsInternal.h"

#include <Ui/UiColorPicker/UiColorPicker.h>

namespace Upp {

struct PropertyGradientStop : Moveable<PropertyGradientStop> {
    double position = 0.0;
    Color color = Black();
    int alpha = 255;
};

static Value PeMapValue(const ValueMap& map,
                        const char *key,
                        const Value& fallback = Value())
{
    int q = map.Find(key);
    return q >= 0 ? map.GetValue(q) : fallback;
}

static Value PeGradientStopValue(const PropertyGradientStop& stop)
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
    if(mode != "Radial")
        mode = "Linear";

    String interpolation = AsString(
        PeMapValue(source, "interpolation", "Linear"));
    if(interpolation != "Smooth")
        interpolation = "Linear";

    double angle = 0.0;
    Value angle_value = PeMapValue(source, "angle", 0.0);
    if(IsNumber(angle_value))
        angle = (double)angle_value;
    while(angle < 0.0)
        angle += 360.0;
    while(angle >= 360.0)
        angle -= 360.0;

    Vector<PropertyGradientStop> stops;
    Value stop_value = PeMapValue(source, "stops");
    if(stop_value.Is<ValueArray>()) {
        ValueArray input = stop_value;
        for(int i = 0; i < input.GetCount(); i++) {
            if(!input[i].Is<ValueMap>())
                continue;
            ValueMap map = input[i];
            PropertyGradientStop& stop = stops.Add();

            Value position = PeMapValue(map, "position", 0.0);
            stop.position = IsNumber(position)
                ? minmax((double)position, 0.0, 1.0) : 0.0;

            Value color = PeMapValue(map, "color", Black());
            stop.color = color.GetType() == COLOR_V ? Color(color) : Black();

            Value alpha = PeMapValue(map, "alpha", 255);
            stop.alpha = IsNumber(alpha)
                ? minmax((int)alpha, 0, 255) : 255;
        }
    }

    if(stops.GetCount() < 2) {
        stops.Clear();
        PropertyGradientStop& first = stops.Add();
        first.position = 0.0;
        first.color = Black();
        PropertyGradientStop& last = stops.Add();
        last.position = 1.0;
        last.color = White();
    }

    Sort(stops, [](const PropertyGradientStop& a,
                   const PropertyGradientStop& b) {
        return a.position < b.position;
    });

    ValueArray normalized_stops;
    for(const PropertyGradientStop& stop : stops)
        normalized_stops.Add(PeGradientStopValue(stop));

    ValueMap out;
    out.Set("mode", mode);
    out.Set("angle", angle);
    out.Set("interpolation", interpolation);
    out.Set("stops", normalized_stops);
    return out;
}

static bool PePickGradientColor(Color& color, Ctrl *owner)
{
    struct Dialog : TopWindow {
        UiColorPicker picker;

        Dialog(Color initial)
        {
            Title("Choose gradient stop colour");
            Sizeable().Zoomable();
            SetRect(0, 0, DPI(720), DPI(560));
            Add(picker.SizePos());
            picker.EnableSessionPersistence(false)
                  .SetSlotCount(1)
                  .SetGeneratorCount(1)
                  .SetSlotLabel(0, "Stop")
                  .SetSlotColor(0, initial, false)
                  .SetActiveSlot(0);
            picker.WhenAccept = [=] { AcceptBreak(IDOK); };
            picker.WhenCancel = [=] { RejectBreak(IDCANCEL); };
        }
    } dialog(color);

    if(owner)
        dialog.CenterOwner();
    if(dialog.Run() != IDOK)
        return false;
    color = dialog.picker.GetSlotColor(0);
    return true;
}

class PropertyGradientValueEditor : public PropertyValueEditor {
public:
    PropertyGradientValueEditor()
    {
        Add(summary_);
        Add(edit_);
        summary_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        summary_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
        edit_.SetText("Edit...");
        summary_.WhenAction = [=] { OpenDialog(); };
        edit_.WhenAction = [=] { OpenDialog(); };
    }

    void Configure(const PropertyEditorItem& item) override
    {
        enabled_ = item.enabled && item.value_editable && !item.read_only;
        summary_.Enable(enabled_);
        edit_.Enable(enabled_);
        SyncSummary();
    }

    void SetEditorValue(const Value& value, bool mixed) override
    {
        mixed_ = mixed;
        value_ = PeNormalizeGradient(value);
        SyncSummary();
    }

    Value GetEditorValue() const override
    {
        return value_;
    }

    void FocusEditor() override
    {
        edit_.SetFocus();
    }

    void Layout() override
    {
        const int button_width = min(DPI(64), GetSize().cx / 3);
        summary_.SetRect(0, 0,
                         max(0, GetSize().cx - button_width - DPI(4)),
                         GetSize().cy);
        edit_.SetRect(max(0, GetSize().cx - button_width), 0,
                      button_width, GetSize().cy);
    }

private:
    void SyncSummary()
    {
        if(mixed_) {
            summary_.SetText("<multiple gradients>");
            return;
        }
        ValueMap map = value_.Is<ValueMap>() ? ValueMap(value_) : ValueMap();
        Value stops = PeMapValue(map, "stops");
        int count = stops.Is<ValueArray>() ? ValueArray(stops).GetCount() : 0;
        summary_.SetText(Format("%s · %d stops · %.0f deg",
                                AsString(PeMapValue(map, "mode", "Linear")),
                                count,
                                (double)PeMapValue(map, "angle", 0.0)));
    }

    void OpenDialog()
    {
        if(!enabled_)
            return;

        struct Dialog : TopWindow {
            UiDropdown mode;
            UiDropdown interpolation;
            UiFloatEdit angle;
            UiFloatEdit position;
            UiIntEdit alpha;
            UiLabel mode_label;
            UiLabel interpolation_label;
            UiLabel angle_label;
            UiLabel stop_label;
            UiLabel position_label;
            UiLabel alpha_label;
            UiButton color;
            UiButton previous;
            UiButton next;
            UiButton add;
            UiButton remove;
            UiButton ok;
            UiButton cancel;
            Vector<PropertyGradientStop> stops;
            int selected = 0;

            Dialog(const Value& recipe)
            {
                Title("Gradient editor");
                Sizeable().Zoomable();
                SetRect(0, 0, DPI(560), DPI(360));

                ValueMap map = PeNormalizeGradient(recipe);
                mode.UseInternalModel().Clear()
                    .Add("Linear", "Linear")
                    .Add("Radial", "Radial");
                mode.SelectByData(AsString(PeMapValue(map, "mode", "Linear")));
                interpolation.UseInternalModel().Clear()
                    .Add("Linear", "Linear")
                    .Add("Smooth", "Smooth");
                interpolation.SelectByData(
                    AsString(PeMapValue(map, "interpolation", "Linear")));
                angle.SetValue((double)PeMapValue(map, "angle", 0.0));
                angle.MinMax(0.0, 359.999).Step(1.0);
                position.MinMax(0.0, 1.0).Step(0.01).Precision(3);
                alpha.MinMax(0, 255).Step(1);

                ValueArray input = PeMapValue(map, "stops");
                for(const Value& value : input) {
                    ValueMap item = value;
                    PropertyGradientStop& stop = stops.Add();
                    stop.position = (double)PeMapValue(item, "position", 0.0);
                    stop.color = Color(PeMapValue(item, "color", Black()));
                    stop.alpha = (int)PeMapValue(item, "alpha", 255);
                }

                mode_label.SetText("Mode");
                interpolation_label.SetText("Interpolation");
                angle_label.SetText("Angle");
                position_label.SetText("Position");
                alpha_label.SetText("Alpha");
                previous.SetText("Previous");
                next.SetText("Next");
                add.SetText("Add stop");
                remove.SetText("Remove");
                ok.SetText("OK");
                cancel.SetText("Cancel");

                Add(mode_label); Add(mode);
                Add(interpolation_label); Add(interpolation);
                Add(angle_label); Add(angle);
                Add(stop_label);
                Add(position_label); Add(position);
                Add(alpha_label); Add(alpha);
                Add(color);
                Add(previous); Add(next); Add(add); Add(remove);
                Add(ok); Add(cancel);

                previous.WhenAction = [=] {
                    SaveStop();
                    if(selected > 0)
                        selected--;
                    LoadStop();
                };
                next.WhenAction = [=] {
                    SaveStop();
                    if(selected + 1 < stops.GetCount())
                        selected++;
                    LoadStop();
                };
                add.WhenAction = [=] { AddStop(); };
                remove.WhenAction = [=] { RemoveStop(); };
                color.WhenAction = [=] { PickStopColor(); };
                ok.WhenAction = [=] {
                    SaveStop();
                    AcceptBreak(IDOK);
                };
                cancel.WhenAction = [=] { RejectBreak(IDCANCEL); };
                LoadStop();
            }

            void Layout() override
            {
                Rect r = GetSize();
                const int pad = DPI(12);
                const int label_width = DPI(104);
                const int height = DPI(30);
                const int gap = DPI(8);
                int y = pad;

                mode_label.SetRect(pad, y, label_width, height);
                mode.SetRect(pad + label_width, y, DPI(150), height);
                y += height + gap;
                interpolation_label.SetRect(pad, y, label_width, height);
                interpolation.SetRect(pad + label_width, y, DPI(150), height);
                y += height + gap;
                angle_label.SetRect(pad, y, label_width, height);
                angle.SetRect(pad + label_width, y, DPI(110), height);
                y += height + DPI(14);

                stop_label.SetRect(pad, y,
                                   max(0, r.GetWidth() - 2 * pad), height);
                y += height + gap;
                position_label.SetRect(pad, y, label_width, height);
                position.SetRect(pad + label_width, y, DPI(110), height);
                alpha_label.SetRect(pad + label_width + DPI(126), y,
                                    DPI(54), height);
                alpha.SetRect(pad + label_width + DPI(184), y,
                              DPI(80), height);
                color.SetRect(r.right - pad - DPI(100), y,
                              DPI(100), height);
                y += height + gap;

                previous.SetRect(pad, y, DPI(82), height);
                next.SetRect(pad + DPI(88), y, DPI(72), height);
                add.SetRect(pad + DPI(170), y, DPI(88), height);
                remove.SetRect(pad + DPI(264), y, DPI(76), height);

                cancel.SetRect(r.right - pad - DPI(88),
                               r.bottom - pad - height, DPI(88), height);
                ok.SetRect(r.right - pad - DPI(182),
                           r.bottom - pad - height, DPI(88), height);
            }

            void SaveStop()
            {
                if(selected < 0 || selected >= stops.GetCount())
                    return;
                PropertyGradientStop current = stops[selected];
                current.position = minmax(position.GetValue(), 0.0, 1.0);
                current.alpha = minmax(alpha.GetValue(), 0, 255);
                stops[selected] = current;
                Sort(stops, [](const PropertyGradientStop& a,
                               const PropertyGradientStop& b) {
                    return a.position < b.position;
                });
                for(int i = 0; i < stops.GetCount(); i++)
                    if(stops[i].position == current.position &&
                       stops[i].color == current.color &&
                       stops[i].alpha == current.alpha) {
                        selected = i;
                        break;
                    }
            }

            void LoadStop()
            {
                if(stops.IsEmpty())
                    return;
                selected = minmax(selected, 0, stops.GetCount() - 1);
                const PropertyGradientStop& stop = stops[selected];
                position.SetValue(stop.position);
                alpha.SetValue(stop.alpha);
                color.SetText(Format("#%02X%02X%02X",
                                     stop.color.GetR(),
                                     stop.color.GetG(),
                                     stop.color.GetB()));
                stop_label.SetText(
                    Format("Stop %d of %d", selected + 1, stops.GetCount()));
                previous.Enable(selected > 0);
                next.Enable(selected + 1 < stops.GetCount());
                remove.Enable(stops.GetCount() > 2);
            }

            void AddStop()
            {
                SaveStop();
                PropertyGradientStop stop;
                stop.position = stops.IsEmpty()
                    ? 0.5 : min(1.0, stops[selected].position + 0.1);
                stop.color = stops.IsEmpty() ? White() : stops[selected].color;
                stop.alpha = stops.IsEmpty() ? 255 : stops[selected].alpha;
                stops.Insert(selected + 1, stop);
                selected++;
                LoadStop();
            }

            void RemoveStop()
            {
                if(stops.GetCount() <= 2)
                    return;
                stops.Remove(selected);
                selected = min(selected, stops.GetCount() - 1);
                LoadStop();
            }

            void PickStopColor()
            {
                if(selected < 0 || selected >= stops.GetCount())
                    return;
                Color next_color = stops[selected].color;
                if(PePickGradientColor(next_color, this)) {
                    stops[selected].color = next_color;
                    LoadStop();
                }
            }

            Value GetRecipe() const
            {
                ValueArray values;
                for(const PropertyGradientStop& stop : stops)
                    values.Add(PeGradientStopValue(stop));
                ValueMap out;
                out.Set("mode", AsString(mode.GetSelectedData()));
                out.Set("angle", angle.GetValue());
                out.Set("interpolation",
                        AsString(interpolation.GetSelectedData()));
                out.Set("stops", values);
                return PeNormalizeGradient(out);
            }
        } dialog(value_);

        dialog.CenterOwner();
        if(dialog.Run() != IDOK)
            return;
        value_ = dialog.GetRecipe();
        mixed_ = false;
        SyncSummary();
        WhenPreview(value_);
        WhenCommit(value_);
    }

    PropertyActionLabel summary_;
    UiButton edit_;
    Value value_;
    bool mixed_ = false;
    bool enabled_ = true;
};

void RegisterPropertyEditorSemanticGradientEditors(PropertyEditorFactory& factory)
{
    if(!factory.HasCustom(PropertyEditorGradientId()))
        factory.RegisterCustom(PropertyEditorGradientId(), [] {
            return One<PropertyValueEditor>(new PropertyGradientValueEditor);
        });
}

Value PropertyEditorMakeGradientStop(double position,
                                     Color color,
                                     int alpha)
{
    PropertyGradientStop stop;
    stop.position = minmax(position, 0.0, 1.0);
    stop.color = color;
    stop.alpha = minmax(alpha, 0, 255);
    return PeGradientStopValue(stop);
}

Value PropertyEditorMakeGradient(const ValueArray& stops,
                                 const String& mode,
                                 double angle,
                                 const String& interpolation)
{
    ValueMap out;
    out.Set("mode", mode);
    out.Set("angle", angle);
    out.Set("interpolation", interpolation);
    out.Set("stops", stops);
    return PeNormalizeGradient(out);
}

PropertyEditorItem& AddPropertyGradient(PropertyEditorModel& model,
                                        const String& id, const String& label,
                                        const Value& recipe,
                                        const String& group)
{
    PropertyEditorItem& item = model.Add(
        id, label, PropertyEditorKind::Custom,
        PeNormalizeGradient(recipe), group);
    item.custom_editor = PropertyEditorGradientId();
    item.inline_editor = true;
    item.row_span = 1;
    item.normalize = [](const Value& candidate) {
        return PeNormalizeGradient(candidate);
    };
    item.validate = [](const Value& candidate) {
        return candidate.Is<ValueMap>()
             ? String() : String("Expected a gradient recipe");
    };
    return item;
}

} // namespace Upp
