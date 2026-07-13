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

class PropertyEditorFactory {
public:
    static PropertyEditorFactory& Global();

    One<PropertyValueEditor> Create(const PropertyEditorItem& item) const;

    void RegisterCustom(const String& id, PropertyValueEditorCreator creator);
    bool HasCustom(const String& id) const;
    Vector<String> GetCustomIds() const;

private:
    VectorMap<String, PropertyValueEditorCreator> custom_;
};

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
