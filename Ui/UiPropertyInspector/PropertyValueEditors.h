#ifndef _Ui_UiPropertyInspector_PropertyValueEditors_h_
#define _Ui_UiPropertyInspector_PropertyValueEditors_h_

#include <Ui/Ui.h>
#include <Ui/UiPropertyInspector/Core/UiPropertyInspectorCore.h>

namespace Upp {

class PropertyActionLabel : public UiLabel {
public:
    typedef PropertyActionLabel CLASSNAME;

    void LeftDown(Point p, dword keyflags) override;
    bool Key(dword key, int count) override;
};

struct UiPropertyInspectorActionIcons {
    Image expand;
    Image collapse;
    Image dialog;
    Image browse;
    Image numeric_slider;
    int size = DPI(16);
};

class PropertyValueEditor : public ParentCtrl {
public:
    typedef PropertyValueEditor CLASSNAME;

    virtual ~PropertyValueEditor() {}

    virtual void Configure(const UiPropertyInspectorItem& item) = 0;
    virtual void SetEditorValue(const Value& value, bool mixed) = 0;
    virtual Value GetEditorValue() const = 0;
    virtual void FocusEditor();
    // Return true when this concrete value editor owns and handled a wheel
    // gesture directed at its active value surface.
    virtual bool HandleMouseWheel(int zdelta, dword keyflags) { return false; }
    virtual void SetExpanded(bool) {}
    virtual void SetActionIcons(const UiPropertyInspectorActionIcons& icons)
    {
        action_icons_ = icons;
        ActionIconsChanged();
    }

    Event<Value> WhenPreview;
    Event<Value> WhenCommit;
    Event<> WhenToggleExpanded;

protected:
    virtual void ActionIconsChanged() {}
    UiPropertyInspectorActionIcons action_icons_;
};

typedef Function<One<PropertyValueEditor>()> PropertyValueEditorCreator;
typedef Function<bool(Value& value, Ctrl *owner)> PropertyValuePicker;
typedef Function<Image(const Value& value)> PropertyValueThumbnailProvider;

class UiPropertyInspectorFactory {
public:
    static UiPropertyInspectorFactory& Global();

    One<PropertyValueEditor> Create(const UiPropertyInspectorItem& item) const;

    void RegisterCustom(const String& id, PropertyValueEditorCreator creator);
    bool HasCustom(const String& id) const;
    Vector<String> GetCustomIds() const;

    // Picker providers keep application-specific browsers (for example a
    // project image browser) outside the UiPropertyInspector package. A provider
    // edits the supplied Value and returns true only when the user accepts.
    void RegisterPicker(const String& id, PropertyValuePicker picker)
    {
        int q = pickers_.Find(id);
        if(q < 0)
            pickers_.Add(id, pick(picker));
        else
            pickers_[q] = pick(picker);
    }
    bool HasPicker(const String& id) const { return pickers_.Find(id) >= 0; }
    bool PickValue(const String& id, Value& value, Ctrl *owner) const
    {
        int q = pickers_.Find(id);
        return q >= 0 && pickers_[q] ? pickers_[q](value, owner) : false;
    }
    void RegisterThumbnailProvider(const String& id,
                                   PropertyValueThumbnailProvider provider)
    {
        int q = thumbnails_.Find(id);
        if(q < 0)
            thumbnails_.Add(id, pick(provider));
        else
            thumbnails_[q] = pick(provider);
    }
    bool HasThumbnailProvider(const String& id) const
    {
        return thumbnails_.Find(id) >= 0;
    }
    Image ResolveThumbnail(const String& id, const Value& value) const
    {
        int q = thumbnails_.Find(id);
        return q >= 0 && thumbnails_[q] ? thumbnails_[q](value) : Image();
    }

private:
    VectorMap<String, PropertyValueEditorCreator> custom_;
    VectorMap<String, PropertyValuePicker> pickers_;
    VectorMap<String, PropertyValueThumbnailProvider> thumbnails_;
};

// Stable visual-package adapter ids. They intentionally use Custom in the
// headless Core model, keeping Ui controls and picker implementations out of
// UiPropertyInspectorCore while providing first-class reusable UiPropertyInspector APIs.
const char *UiPropertyInspectorRangeDoubleId();
const char *UiPropertyInspectorAdjustableRangeId();
const char *UiPropertyInspectorMatrixId();
const char *UiPropertyInspectorIconId();
const char *UiPropertyInspectorFontId();
const char *UiPropertyInspectorImageId();

// Semantic adapters. These remain visual-package custom editors so the
// UiPropertyInspectorCore schema stays headless and applications can still use the
// generic Custom escape hatch for their own domain types.
const char *UiPropertyInspectorDateTimeId();
const char *UiPropertyInspectorDurationId();
const char *UiPropertyInspectorGeometryId();
const char *UiPropertyInspectorFlagsId();
const char *UiPropertyInspectorStringListId();
const char *UiPropertyInspectorGradientId();
const char *UiPropertyInspectorKeyChordId();
const char *UiPropertyInspectorReferenceId();
const char *UiPropertyInspectorOptionalId();

void RegisterUiPropertyInspectorBuiltinEditors(UiPropertyInspectorFactory& factory);
void RegisterUiPropertyInspectorSemanticEditors(UiPropertyInspectorFactory& factory);
// Preferred complete registration point for new applications/demos.
void RegisterUiPropertyInspectorEditors(UiPropertyInspectorFactory& factory);

UiPropertyInspectorItem& AddPropertyRange(UiPropertyInspectorModel& model,
                                     const String& id, const String& label,
                                     double lower, double upper,
                                     double minimum, double maximum,
                                     double step = 0.0,
                                     const String& group = String());
UiPropertyInspectorItem& AddPropertyAdjustableRange(UiPropertyInspectorModel& model,
                                               const String& id, const String& label,
                                               double hard_minimum, double bound_lower,
                                               double lower, double upper,
                                               double bound_upper, double hard_maximum,
                                               double step = 0.0,
                                               const String& group = String());
UiPropertyInspectorItem& AddPropertyMatrix(UiPropertyInspectorModel& model,
                                      const String& id, const String& label,
                                      const Value& value,
                                      const String& preset = "Position9",
                                      const String& group = String());
UiPropertyInspectorItem& AddPropertyIcon(UiPropertyInspectorModel& model,
                                    const String& id, const String& label,
                                    const String& icon_name,
                                    const String& group = String());
UiPropertyInspectorItem& AddPropertyFont(UiPropertyInspectorModel& model,
                                    const String& id, const String& label,
                                    const String& face_name,
                                    const String& group = String());
UiPropertyInspectorItem& AddPropertyImage(UiPropertyInspectorModel& model,
                                     const String& id, const String& label,
                                     const Value& value,
                                     const String& picker_provider,
                                     const String& group = String());

// Date / time values use the production UiDateTime picker. Date stores Date;
// Time and DateTime store Time, with Time normalized to the 1970-01-01 anchor.
UiPropertyInspectorItem& AddPropertyDate(UiPropertyInspectorModel& model,
                                    const String& id, const String& label,
                                    Date value, const String& group = String());
UiPropertyInspectorItem& AddPropertyTime(UiPropertyInspectorModel& model,
                                    const String& id, const String& label,
                                    Time value, bool show_seconds = false,
                                    const String& group = String());
UiPropertyInspectorItem& AddPropertyDateTime(UiPropertyInspectorModel& model,
                                        const String& id, const String& label,
                                        Time value, bool show_seconds = false,
                                        const String& group = String());

// Duration is stored in seconds. The editor chooses a convenient display unit
// (ms / s / min / h) without changing the durable value unit.
UiPropertyInspectorItem& AddPropertyDuration(UiPropertyInspectorModel& model,
                                        const String& id, const String& label,
                                        double seconds,
                                        double minimum_seconds = 0.0,
                                        double maximum_seconds = 86400.0,
                                        double step_seconds = 0.001,
                                        const String& group = String());

// Semantic geometry values are ValueArray-based to keep Core independent of
// Ui/geometry controls. Point/Size contain 2 numbers; Rect/Insets/Corners 4.
UiPropertyInspectorItem& AddPropertyPoint(UiPropertyInspectorModel& model,
                                     const String& id, const String& label,
                                     double x, double y,
                                     const String& group = String());
UiPropertyInspectorItem& AddPropertySize(UiPropertyInspectorModel& model,
                                    const String& id, const String& label,
                                    double cx, double cy,
                                    const String& group = String());
UiPropertyInspectorItem& AddPropertyRect(UiPropertyInspectorModel& model,
                                    const String& id, const String& label,
                                    double x, double y, double cx, double cy,
                                    const String& group = String());
UiPropertyInspectorItem& AddPropertyInsets(UiPropertyInspectorModel& model,
                                      const String& id, const String& label,
                                      double left, double top,
                                      double right, double bottom,
                                      bool linked = false,
                                      const String& group = String());
UiPropertyInspectorItem& AddPropertyCorners(UiPropertyInspectorModel& model,
                                       const String& id, const String& label,
                                       double top_left, double top_right,
                                       double bottom_right, double bottom_left,
                                       bool linked = false,
                                       const String& group = String());

// Flags store a ValueArray of selected choice values. Add choices to the
// returned UiPropertyInspectorItem with AddChoice(value,label[,icon]).
UiPropertyInspectorItem& AddPropertyFlags(UiPropertyInspectorModel& model,
                                     const String& id, const String& label,
                                     const ValueArray& selected,
                                     const String& group = String());

// A bounded small ordered string collection. This is for property-sized arrays,
// not a replacement for model-authoritative Data pages such as List/Tree/Table.
UiPropertyInspectorItem& AddPropertyStringList(UiPropertyInspectorModel& model,
                                          const String& id, const String& label,
                                          const ValueArray& values,
                                          int maximum_items = 32,
                                          const String& group = String());

// Gradient recipe schema:
// { mode: "Linear"|"Radial", angle: double, interpolation: "Linear"|"Smooth",
//   stops: [ { position:0..1, color:Color, alpha:0..255 }, ... ] }
Value UiPropertyInspectorMakeGradientStop(double position, Color color, int alpha = 255);
Value UiPropertyInspectorMakeGradient(const ValueArray& stops,
                                 const String& mode = "Linear",
                                 double angle = 0.0,
                                 const String& interpolation = "Linear");
UiPropertyInspectorItem& AddPropertyGradient(UiPropertyInspectorModel& model,
                                        const String& id, const String& label,
                                        const Value& recipe,
                                        const String& group = String());

// Key chords store a canonical string such as "Ctrl+Shift+S".
UiPropertyInspectorItem& AddPropertyKeyChord(UiPropertyInspectorModel& model,
                                        const String& id, const String& label,
                                        const String& chord,
                                        const String& group = String());

// Generic resource/reference browser. The application owns the picker provider;
// the UiPropertyInspector only stores/displays the returned Value.
UiPropertyInspectorItem& AddPropertyReference(UiPropertyInspectorModel& model,
                                         const String& id, const String& label,
                                         const Value& value,
                                         const String& picker_provider,
                                         const String& group = String());

// First-class nullable value presentation. Variant can be "text", "int" or
// "double". Null is a normal durable value, not an inherited-state sentinel.
UiPropertyInspectorItem& AddPropertyOptional(UiPropertyInspectorModel& model,
                                        const String& id, const String& label,
                                        const Value& value,
                                        const Value& fallback,
                                        const String& variant = "text",
                                        const String& group = String());

class PropertyCurveCanvas : public Ctrl {
public:
    typedef PropertyCurveCanvas CLASSNAME;

    PropertyCurveCanvas();

    void SetCurve(const Value& value);
    Value GetCurve() const;

    void ResetLinear();
    void DeleteSelected();

    virtual void Paint(Draw& w) override;
    virtual void LeftDown(Point p, dword keyflags) override;
    virtual void MouseMove(Point p, dword keyflags) override;
    virtual void LeftUp(Point p, dword keyflags) override;
    virtual bool Key(dword key, int count) override;
    virtual Size GetMinSize() const override;

    Event<Value> WhenCurvePreview;
    Event<Value> WhenCurveCommit;

private:
    Rect GetGraphRect() const;
    Point CurveToClient(const Pointf& p) const;
    Pointf ClientToCurve(Point p) const;
    int HitPoint(Point p) const;
    void Normalize();
    void EmitPreview();
    void EmitCommit();

    Vector<Pointf> points_;
    int selected_ = -1;
    int dragging_ = -1;
};

bool EditPropertyCurve(Value& value, Ctrl *owner = nullptr);
bool EditPropertyBezierCurve(Value& value, Ctrl *owner = nullptr,
                             double y_minimum = -1.0,
                             double y_maximum = 2.0);

}

#endif
