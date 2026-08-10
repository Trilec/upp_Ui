#ifndef _Utilities_PropertyEditor_PropertyValueEditors_h_
#define _Utilities_PropertyEditor_PropertyValueEditors_h_

#include <Ui/Ui.h>
#include <Utilities/PropertyEditorCore/PropertyEditorCore.h>

namespace Upp {

struct PropertyEditorActionIcons {
    Image expand;
    Image collapse;
    Image dialog;
    Image browse;
    int size = DPI(16);
};

class PropertyValueEditor : public ParentCtrl {
public:
    typedef PropertyValueEditor CLASSNAME;

    virtual ~PropertyValueEditor() {}

    virtual void Configure(const PropertyEditorItem& item) = 0;
    virtual void SetEditorValue(const Value& value, bool mixed) = 0;
    virtual Value GetEditorValue() const = 0;
    virtual void FocusEditor();
    virtual void SetExpanded(bool) {}
    virtual void SetActionIcons(const PropertyEditorActionIcons& icons)
    {
        action_icons_ = icons;
        ActionIconsChanged();
    }

    Event<Value> WhenPreview;
    Event<Value> WhenCommit;
    Event<> WhenToggleExpanded;

protected:
    virtual void ActionIconsChanged() {}
    PropertyEditorActionIcons action_icons_;
};

typedef Function<One<PropertyValueEditor>()> PropertyValueEditorCreator;
typedef Function<bool(Value& value, Ctrl *owner)> PropertyValuePicker;
typedef Function<Image(const Value& value)> PropertyValueThumbnailProvider;

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
// PropertyEditorCore while providing first-class reusable PropertyEditor APIs.
const char *PropertyEditorRangeDoubleId();
const char *PropertyEditorAdjustableRangeId();
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
PropertyEditorItem& AddPropertyAdjustableRange(PropertyEditorModel& model,
                                               const String& id, const String& label,
                                               double hard_minimum, double bound_lower,
                                               double lower, double upper,
                                               double bound_upper, double hard_maximum,
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
