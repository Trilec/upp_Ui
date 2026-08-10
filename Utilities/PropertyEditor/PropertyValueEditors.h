#ifndef _Utilities_PropertyEditor_PropertyValueEditors_h_
#define _Utilities_PropertyEditor_PropertyValueEditors_h_

#include <Ui/Ui.h>
#include <Utilities/PropertyEditorCore/PropertyEditorCore.h>

namespace Upp {

class PropertyValueEditor : public ParentCtrl {
public:
    typedef PropertyValueEditor CLASSNAME;

    virtual ~PropertyValueEditor() {}

    virtual void Configure(const PropertyEditorItem& item) = 0;
    virtual void SetEditorValue(const Value& value, bool mixed) = 0;
    virtual Value GetEditorValue() const = 0;
    virtual void FocusEditor();

    Event<Value> WhenPreview;
    Event<Value> WhenCommit;
};

typedef Function<One<PropertyValueEditor>()> PropertyValueEditorCreator;
typedef Function<bool(Value& value, Ctrl *owner)> PropertyValuePicker;

class PropertyEditorFactory {
public:
    static PropertyEditorFactory& Global();

    One<PropertyValueEditor> Create(const PropertyEditorItem& item) const;

    void RegisterCustom(const String& id, PropertyValueEditorCreator creator);
    bool HasCustom(const String& id) const;
    Vector<String> GetCustomIds() const;

    // Picker providers keep application-specific browsers (for example a
    // project image browser) outside the PropertyEditor package. A provider
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

private:
    VectorMap<String, PropertyValueEditorCreator> custom_;
    VectorMap<String, PropertyValuePicker> pickers_;
};

// Stable visual-package adapter ids. They intentionally use Custom in the
// headless Core model, keeping Ui controls and picker implementations out of
// PropertyEditorCore while providing first-class reusable PropertyEditor APIs.
const char *PropertyEditorRangeDoubleId();
const char *PropertyEditorMatrixId();
const char *PropertyEditorIconId();
const char *PropertyEditorFontId();
const char *PropertyEditorImageId();
void RegisterPropertyEditorV1Editors(PropertyEditorFactory& factory);

PropertyEditorItem& AddPropertyRange(PropertyEditorModel& model,
                                     const String& id, const String& label,
                                     double lower, double upper,
                                     double minimum, double maximum,
                                     double step = 0.0,
                                     const String& group = String());
PropertyEditorItem& AddPropertyMatrix(PropertyEditorModel& model,
                                      const String& id, const String& label,
                                      const Value& value,
                                      const String& preset = "Position9",
                                      const String& group = String());
PropertyEditorItem& AddPropertyIcon(PropertyEditorModel& model,
                                    const String& id, const String& label,
                                    const String& icon_name,
                                    const String& group = String());
PropertyEditorItem& AddPropertyFont(PropertyEditorModel& model,
                                    const String& id, const String& label,
                                    const String& face_name,
                                    const String& group = String());
PropertyEditorItem& AddPropertyImage(PropertyEditorModel& model,
                                     const String& id, const String& label,
                                     const Value& value,
                                     const String& picker_provider,
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

}

#endif
