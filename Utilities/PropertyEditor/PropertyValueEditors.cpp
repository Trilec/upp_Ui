#include "PropertyValueEditors.h"

#include <cmath>

namespace Upp {

void PropertyValueEditor::FocusEditor()
{
    SetFocus();
}

class PropertyCommitEdit : public EditString {
public:
    typedef PropertyCommitEdit CLASSNAME;

    PropertyCommitEdit()
    {
        WhenEnter = [=] { EmitCommit(); };
    }

    virtual void LostFocus() override
    {
        EditString::LostFocus();
        EmitCommit();
    }

    Event<> WhenCommit;

private:
    void EmitCommit()
    {
        Value v = GetData();
        if(!has_last_commit_ || v != last_commit_) {
            last_commit_ = v;
            has_last_commit_ = true;
            WhenCommit();
        }
    }

    Value last_commit_;
    bool has_last_commit_ = false;
};

class PropertyTextValueEditor : public PropertyValueEditor {
public:
    PropertyTextValueEditor()
    {
        Add(edit_.SizePos());
        edit_.WhenAction = [=] {
            if(!syncing_)
                WhenPreview(edit_.GetData());
        };
        edit_.WhenCommit = [=] {
            if(!syncing_)
                WhenCommit(edit_.GetData());
        };
    }

    virtual void Configure(const PropertyEditorItem& item) override
    {
        if(item.read_only || !item.enabled)
            edit_.SetReadOnly();
        else
            edit_.SetEditable(true);
        edit_.NullText(item.mixed ? "<multiple values>" :
                       item.inherited ? "<inherited>" : "");
        Enable(item.enabled && !item.read_only);
    }

    virtual void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        edit_.SetData(mixed ? Value(String()) : Value(AsString(value)));
        syncing_ = false;
    }

    virtual Value GetEditorValue() const override
    {
        return edit_.GetData();
    }

    virtual void FocusEditor() override
    {
        edit_.SetFocus();
        edit_.SetSelection();
    }

private:
    PropertyCommitEdit edit_;
    bool syncing_ = false;
};

class PropertyMultilineEdit : public TextCtrl {
public:
    typedef PropertyMultilineEdit CLASSNAME;

    PropertyMultilineEdit()
    {
        WhenState = [=] {
            EmitPreview();
        };
    }

    Event<> WhenFinalCommit;

    virtual void LostFocus() override
    {
        TextCtrl::LostFocus();
        EmitCommit();
    }

private:
    void EmitPreview()
    {
        WhenPreview(GetData());
    }

    void EmitCommit()
    {
        Value v = GetData();
        if(!has_last_commit_ || v != last_commit_) {
            last_commit_ = v;
            has_last_commit_ = true;
            WhenFinalCommit();
        }
    }

    Value last_commit_;
    bool has_last_commit_ = false;

public:
    Event<Value> WhenPreview;
};

class PropertyMultilineValueEditor : public PropertyValueEditor {
public:
    PropertyMultilineValueEditor()
    {
        Add(edit_.SizePos());
        edit_.NoProcessTab();
        edit_.NoProcessEnter();
        edit_.WhenPreview = [=](Value v) {
            if(!syncing_)
                WhenPreview(v);
        };
        edit_.WhenFinalCommit = [=] {
            if(!syncing_)
                WhenCommit(edit_.GetData());
        };
    }

    virtual void Configure(const PropertyEditorItem& item) override
    {
        if(item.read_only || !item.enabled)
            edit_.SetReadOnly();
        else
            edit_.SetEditable(true);
        Enable(item.enabled && !item.read_only);
        edit_.SetFrame(EditFieldFrame());
    }

    virtual void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        edit_.SetData(mixed ? Value(String()) : Value(AsString(value)));
        syncing_ = false;
    }

    virtual Value GetEditorValue() const override
    {
        return edit_.GetData();
    }

    virtual void FocusEditor() override
    {
        edit_.SetFocus();
    }

private:
    PropertyMultilineEdit edit_;
    bool syncing_ = false;
};

class PropertyNumberValueEditor : public PropertyValueEditor {
public:
    explicit PropertyNumberValueEditor(bool integer)
        : integer_(integer)
    {
        Add(edit_.SizePos());
        edit_.AlignRight();
        edit_.WhenAction = [=] {
            if(!syncing_)
                WhenPreview(GetEditorValue());
        };
        edit_.WhenCommit = [=] {
            if(!syncing_)
                WhenCommit(GetEditorValue());
        };
    }

    virtual void Configure(const PropertyEditorItem& item) override
    {
        decimals_ = max(0, item.decimals);
        if(item.read_only || !item.enabled)
            edit_.SetReadOnly();
        else
            edit_.SetEditable(true);
        edit_.NullText(item.mixed ? "<mixed>" :
                       item.inherited ? "<inherited>" : "");
        Enable(item.enabled && !item.read_only);
    }

    virtual void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        if(mixed || IsNull(value))
            edit_.SetData(String());
        else if(integer_)
            edit_.SetData(AsString((int)value));
        else
            edit_.SetData(Format("%.*f", decimals_, (double)value));
        syncing_ = false;
    }

    virtual Value GetEditorValue() const override
    {
        return edit_.GetData();
    }

    virtual void FocusEditor() override
    {
        edit_.SetFocus();
        edit_.SetSelection();
    }

private:
    PropertyCommitEdit edit_;
    bool integer_ = false;
    bool syncing_ = false;
    int decimals_ = 3;
};

class PropertyBooleanValueEditor : public PropertyValueEditor {
public:
    PropertyBooleanValueEditor()
    {
        Add(option_.SizePos());
        option_.ShowLabel(false);
        option_.WhenAction = [=] {
            if(syncing_)
                return;
            Value v = option_.GetData();
            WhenPreview(v);
            WhenCommit(v);
        };
    }

    virtual void Configure(const PropertyEditorItem& item) override
    {
        option_.ThreeState(item.mixed);
        option_.Enable(item.enabled && !item.read_only);
    }

    virtual void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        option_.ThreeState(mixed);
        option_.SetData(mixed ? Value(Null) : value);
        syncing_ = false;
    }

    virtual Value GetEditorValue() const override
    {
        return option_.GetData();
    }

    virtual void FocusEditor() override
    {
        option_.SetFocus();
    }

private:
    Option option_;
    bool syncing_ = false;
};

class PropertyChoiceValueEditor : public PropertyValueEditor {
public:
    PropertyChoiceValueEditor()
    {
        Add(drop_.SizePos());
        drop_.WhenAction = [=] {
            if(syncing_)
                return;
            Value v = drop_.GetData();
            WhenPreview(v);
            WhenCommit(v);
        };
    }

    virtual void Configure(const PropertyEditorItem& item) override
    {
        syncing_ = true;
        drop_.ClearList();
        for(const PropertyEditorChoice& choice : item.choices)
            drop_.Add(choice.value, choice.label);
        drop_.Enable(item.enabled && !item.read_only);
        syncing_ = false;
    }

    virtual void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        drop_.SetData(mixed ? Value(Null) : value);
        syncing_ = false;
    }

    virtual Value GetEditorValue() const override
    {
        return drop_.GetData();
    }

    virtual void FocusEditor() override
    {
        drop_.SetFocus();
    }

private:
    DropList drop_;
    bool syncing_ = false;
};

class PropertyColorValueEditor : public PropertyValueEditor {
public:
    PropertyColorValueEditor()
    {
        Add(color_.SizePos());
        color_.WithText().WithHex().Track();
        color_.WhenAction = [=] {
            if(syncing_)
                return;
            Value v = color_.GetData();
            WhenPreview(v);
            WhenCommit(v);
        };
    }

    virtual void Configure(const PropertyEditorItem& item) override
    {
        color_.Enable(item.enabled && !item.read_only);
    }

    virtual void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        color_.SetData(mixed ? Value(Null) : value);
        syncing_ = false;
    }

    virtual Value GetEditorValue() const override
    {
        return color_.GetData();
    }

    virtual void FocusEditor() override
    {
        color_.SetFocus();
    }

private:
    ColorPusher color_;
    bool syncing_ = false;
};

class PropertySliderValueEditor : public PropertyValueEditor {
public:
    explicit PropertySliderValueEditor(bool integer)
        : integer_(integer)
    {
        Add(slider_);
        Add(edit_);
        slider_.WhenAction = [=] {
            if(syncing_)
                return;
            syncing_ = true;
            double value = SliderToValue((int)slider_.GetData());
            edit_.SetData(integer_ ? Value(AsString((int)floor(value + 0.5))) :
                                     Value(Format("%.*f", decimals_, value)));
            syncing_ = false;
            WhenPreview(integer_ ? Value((int)floor(value + 0.5)) : Value(value));
        };
        slider_.WhenSlideFinish = [=] {
            if(syncing_)
                return;
            double value = SliderToValue((int)slider_.GetData());
            WhenCommit(integer_ ? Value((int)floor(value + 0.5)) : Value(value));
        };
        edit_.WhenAction = [=] {
            if(syncing_)
                return;
            Value v = edit_.GetData();
            WhenPreview(v);
        };
        edit_.WhenCommit = [=] {
            if(syncing_)
                return;
            Value v = edit_.GetData();
            WhenCommit(v);
        };
    }

    virtual void Configure(const PropertyEditorItem& item) override
    {
        decimals_ = max(0, item.decimals);
        step_ = IsNumber(item.step) ? max(0.0, (double)item.step) : 0.0;
        minimum_ = IsNumber(item.minimum) ? (double)item.minimum : 0.0;
        maximum_ = IsNumber(item.maximum) ? (double)item.maximum : 100.0;
        if(maximum_ <= minimum_)
            maximum_ = minimum_ + 1.0;

        slider_.MinMax(0, slider_resolution_);
        slider_.Enable(item.enabled && !item.read_only);
        edit_.Enable(item.enabled && !item.read_only);
        if(item.read_only || !item.enabled)
            edit_.SetReadOnly();
        else
            edit_.SetEditable(true);
        edit_.AlignRight();
        edit_.NullText(item.mixed ? "<mixed>" : "");
    }

    virtual void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        if(mixed || IsNull(value)) {
            edit_.SetData(String());
            slider_.SetData(0);
        }
        else {
            double v = (double)value;
            slider_.SetData(ValueToSlider(v));
            edit_.SetData(integer_ ? Value(AsString((int)floor(v + 0.5))) :
                                     Value(Format("%.*f", decimals_, v)));
        }
        syncing_ = false;
    }

    virtual Value GetEditorValue() const override
    {
        return edit_.GetData();
    }

    virtual void Layout() override
    {
        Size sz = GetSize();
        int edit_cx = min(max(DPI(58), sz.cx / 4), DPI(96));
        slider_.SetRect(0, 0, max(0, sz.cx - edit_cx - DPI(6)), sz.cy);
        edit_.SetRect(max(0, sz.cx - edit_cx), 0, edit_cx, sz.cy);
    }

    virtual void FocusEditor() override
    {
        slider_.SetFocus();
    }

private:
    int ValueToSlider(double value) const
    {
        double t = (value - minimum_) / (maximum_ - minimum_);
        t = minmax(t, 0.0, 1.0);
        return (int)floor(t * slider_resolution_ + 0.5);
    }

    double SliderToValue(int slider_value) const
    {
        double t = (double)slider_value / (double)slider_resolution_;
        double v = minimum_ + (maximum_ - minimum_) * t;
        if(step_ > 0)
            v = minimum_ + floor((v - minimum_) / step_ + 0.5) * step_;
        return minmax(v, minimum_, maximum_);
    }

    SliderCtrl slider_;
    PropertyCommitEdit edit_;
    bool integer_ = false;
    bool syncing_ = false;
    double minimum_ = 0;
    double maximum_ = 100;
    double step_ = 0;
    int decimals_ = 3;
    int slider_resolution_ = 10000;
};

class PropertyVectorValueEditor : public PropertyValueEditor {
public:
    explicit PropertyVectorValueEditor(int count)
        : count_(count)
    {
        for(int i = 0; i < count_; i++) {
            PropertyCommitEdit& edit = edits_.Add();
            Label& label = labels_.Add();
            Add(edit);
            Add(label);
            label.SetAlign(ALIGN_CENTER);
            label.SetLabel(i == 0 ? "X" : i == 1 ? "Y" : "Z");
            edit.AlignRight();
            edit.WhenAction = [=] {
                if(!syncing_)
                    WhenPreview(GetEditorValue());
            };
            edit.WhenCommit = [=] {
                if(!syncing_)
                    WhenCommit(GetEditorValue());
            };
        }
    }

    virtual void Configure(const PropertyEditorItem& item) override
    {
        decimals_ = max(0, item.decimals);
        for(Label& label : labels_)
            label.Show();
        for(PropertyCommitEdit& edit : edits_) {
            edit.Enable(item.enabled && !item.read_only);
            if(item.read_only || !item.enabled)
                edit.SetReadOnly();
            else
                edit.SetEditable(true);
            edit.NullText(item.mixed ? "<mixed>" : "");
        }
    }

    virtual void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        Vector<double> v = PropertyEditorReadVector(value, count_);
        for(int i = 0; i < edits_.GetCount(); i++)
            edits_[i].SetData(mixed ? Value(String()) :
                              Value(Format("%.*f", decimals_, v[i])));
        syncing_ = false;
    }

    virtual Value GetEditorValue() const override
    {
        ValueArray value;
        for(const PropertyCommitEdit& edit : edits_)
            value.Add(edit.GetData());
        return value;
    }

    virtual void Layout() override
    {
        Size sz = GetSize();
        int gap = DPI(4);
        int label_h = DPI(14);
        int cell = max(1, (sz.cx - gap * (count_ - 1)) / count_);
        int x = 0;
        for(int i = 0; i < edits_.GetCount(); i++) {
            int cx = i + 1 == edits_.GetCount() ? sz.cx - x : cell;
            labels_[i].SetRect(x, 0, max(0, cx), label_h);
            edits_[i].SetRect(x, label_h + DPI(2), max(0, cx), max(0, sz.cy - label_h - DPI(2)));
            x += cell + gap;
        }
    }

    virtual void FocusEditor() override
    {
        if(!edits_.IsEmpty()) {
            edits_[0].SetFocus();
            edits_[0].SetSelection();
        }
    }

private:
    Array<PropertyCommitEdit> edits_;
    Array<Label> labels_;
    int count_ = 2;
    int decimals_ = 3;
    bool syncing_ = false;
};

class PropertyReadOnlyValueEditor : public PropertyValueEditor {
public:
    PropertyReadOnlyValueEditor()
    {
        Add(label_.SizePos());
        label_.SetAlign(ALIGN_LEFT);
    }

    virtual void Configure(const PropertyEditorItem&) override
    {
    }

    virtual void SetEditorValue(const Value& value, bool mixed) override
    {
        value_ = value;
        label_.SetLabel(mixed ? "<multiple values>" : AsString(value));
    }

    virtual Value GetEditorValue() const override
    {
        return value_;
    }

private:
    Label label_;
    Value value_;
};

class PropertyCurveDialog : public TopWindow {
public:
    typedef PropertyCurveDialog CLASSNAME;

    PropertyCurveDialog()
    {
        Title("Curve editor");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(560), DPI(410));

        Add(canvas_);
        Add(help_);
        Add(reset_);
        Add(remove_);
        Add(ok_);
        Add(cancel_);

        help_.SetLabel("Click empty space to add a point. Drag points to move them. Delete removes the selected point.");
        reset_.SetLabel("Linear");
        remove_.SetLabel("Remove");
        ok_.SetLabel("OK");
        ok_.Ok();
        cancel_.SetLabel("Cancel");
        cancel_.Cancel();

        reset_.WhenAction = [=] { canvas_.ResetLinear(); };
        remove_.WhenAction = [=] { canvas_.DeleteSelected(); };
        ok_.WhenAction = [=] { AcceptBreak(IDOK); };
        cancel_.WhenAction = [=] { RejectBreak(IDCANCEL); };
    }

    void SetCurve(const Value& value)
    {
        canvas_.SetCurve(value);
    }

    Value GetCurve() const
    {
        return canvas_.GetCurve();
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        int pad = DPI(10);
        int button_h = DPI(28);
        int help_h = DPI(34);
        int button_w = DPI(82);
        int gap = DPI(6);

        help_.SetRect(pad, pad, max(0, r.GetWidth() - 2 * pad), help_h);
        int bottom = r.bottom - pad - button_h;
        canvas_.SetRect(pad, pad + help_h + gap,
                        max(0, r.GetWidth() - 2 * pad),
                        max(0, bottom - (pad + help_h + 2 * gap)));

        int x = pad;
        reset_.SetRect(x, bottom, button_w, button_h);
        x += button_w + gap;
        remove_.SetRect(x, bottom, button_w, button_h);

        cancel_.SetRect(r.right - pad - button_w, bottom, button_w, button_h);
        ok_.SetRect(r.right - pad - 2 * button_w - gap, bottom, button_w, button_h);
    }

private:
    PropertyCurveCanvas canvas_;
    Label help_;
    Button reset_;
    Button remove_;
    Button ok_;
    Button cancel_;
};

class PropertyCurveValueEditor : public PropertyValueEditor {
public:
    PropertyCurveValueEditor()
    {
        Add(summary_);
        Add(button_);
        summary_.SetAlign(ALIGN_LEFT);
        button_.SetLabel("Edit...");
        button_.WhenAction = [=] {
            Value edited = value_;
            if(EditPropertyCurve(edited, this)) {
                value_ = edited;
                summary_.SetLabel(PropertyEditorFormatCurve(value_));
                WhenPreview(value_);
                WhenCommit(value_);
            }
        };
    }

    virtual void Configure(const PropertyEditorItem& item) override
    {
        button_.Enable(item.enabled && !item.read_only);
    }

    virtual void SetEditorValue(const Value& value, bool mixed) override
    {
        value_ = PropertyEditorNormalizeCurve(value);
        summary_.SetLabel(mixed ? "<multiple curves>" : PropertyEditorFormatCurve(value_));
    }

    virtual Value GetEditorValue() const override
    {
        return value_;
    }

    virtual void Layout() override
    {
        Size sz = GetSize();
        int button_cx = min(DPI(76), max(DPI(54), sz.cx / 3));
        summary_.SetRect(0, 0, max(0, sz.cx - button_cx - DPI(4)), sz.cy);
        button_.SetRect(max(0, sz.cx - button_cx), 0, button_cx, sz.cy);
    }

    virtual void FocusEditor() override
    {
        button_.SetFocus();
    }

private:
    Label summary_;
    Button button_;
    Value value_;
};

PropertyEditorFactory& PropertyEditorFactory::Global()
{
    static PropertyEditorFactory factory;
    return factory;
}

One<PropertyValueEditor> PropertyEditorFactory::Create(const PropertyEditorItem& item) const
{
    if(item.kind == PropertyEditorKind::Custom) {
        int q = custom_.Find(item.custom_editor);
        if(q >= 0 && custom_[q])
            return custom_[q]();
        return One<PropertyValueEditor>();
    }

    switch(item.kind) {
    case PropertyEditorKind::Text:
        return One<PropertyValueEditor>(new PropertyTextValueEditor);
    case PropertyEditorKind::Multiline:
        return One<PropertyValueEditor>(new PropertyMultilineValueEditor);
    case PropertyEditorKind::Integer:
        return One<PropertyValueEditor>(new PropertyNumberValueEditor(true));
    case PropertyEditorKind::Double:
        return One<PropertyValueEditor>(new PropertyNumberValueEditor(false));
    case PropertyEditorKind::Boolean:
        return One<PropertyValueEditor>(new PropertyBooleanValueEditor);
    case PropertyEditorKind::Choice:
        return One<PropertyValueEditor>(new PropertyChoiceValueEditor);
    case PropertyEditorKind::Color:
        return One<PropertyValueEditor>(new PropertyColorValueEditor);
    case PropertyEditorKind::SliderInt:
        return One<PropertyValueEditor>(new PropertySliderValueEditor(true));
    case PropertyEditorKind::SliderDouble:
        return One<PropertyValueEditor>(new PropertySliderValueEditor(false));
    case PropertyEditorKind::Vector2:
        return One<PropertyValueEditor>(new PropertyVectorValueEditor(2));
    case PropertyEditorKind::Vector3:
        return One<PropertyValueEditor>(new PropertyVectorValueEditor(3));
    case PropertyEditorKind::Curve:
        return One<PropertyValueEditor>(new PropertyCurveValueEditor);
    case PropertyEditorKind::ReadOnly:
        return One<PropertyValueEditor>(new PropertyReadOnlyValueEditor);
    case PropertyEditorKind::Custom:
        break;
    }
    return One<PropertyValueEditor>();
}

void PropertyEditorFactory::RegisterCustom(const String& id,
                                           PropertyValueEditorCreator creator)
{
    int q = custom_.Find(id);
    if(q < 0)
        custom_.Add(id, pick(creator));
    else
        custom_[q] = pick(creator);
}

bool PropertyEditorFactory::HasCustom(const String& id) const
{
    return custom_.Find(id) >= 0;
}

Vector<String> PropertyEditorFactory::GetCustomIds() const
{
    Vector<String> out;
    for(int i = 0; i < custom_.GetCount(); i++)
        out.Add(custom_.GetKey(i));
    return out;
}

PropertyCurveCanvas::PropertyCurveCanvas()
{
    WantFocus();
    ResetLinear();
}

void PropertyCurveCanvas::SetCurve(const Value& value)
{
    points_ = PropertyEditorReadCurve(PropertyEditorNormalizeCurve(value));
    selected_ = points_.IsEmpty() ? -1 : 0;
    dragging_ = -1;
    Refresh();
}

Value PropertyCurveCanvas::GetCurve() const
{
    return PropertyEditorMakeCurve(points_);
}

void PropertyCurveCanvas::ResetLinear()
{
    points_.Clear();
    points_.Add(Pointf(0.0, 0.0));
    points_.Add(Pointf(1.0, 1.0));
    selected_ = 0;
    dragging_ = -1;
    Refresh();
    EmitPreview();
    EmitCommit();
}

void PropertyCurveCanvas::DeleteSelected()
{
    if(selected_ < 0 || selected_ >= points_.GetCount() || points_.GetCount() <= 2)
        return;
    points_.Remove(selected_);
    selected_ = min(selected_, points_.GetCount() - 1);
    Normalize();
    Refresh();
    EmitPreview();
    EmitCommit();
}

Rect PropertyCurveCanvas::GetGraphRect() const
{
    Rect r = GetSize();
    r.Deflate(DPI(16));
    return r;
}

Point PropertyCurveCanvas::CurveToClient(const Pointf& p) const
{
    Rect r = GetGraphRect();
    int x = r.left + (int)floor(p.x * r.GetWidth() + 0.5);
    int y = r.bottom - (int)floor(p.y * r.GetHeight() + 0.5);
    return Point(x, y);
}

Pointf PropertyCurveCanvas::ClientToCurve(Point p) const
{
    Rect r = GetGraphRect();
    if(r.GetWidth() <= 0 || r.GetHeight() <= 0)
        return Pointf(0, 0);
    double x = (double)(p.x - r.left) / (double)r.GetWidth();
    double y = (double)(r.bottom - p.y) / (double)r.GetHeight();
    return Pointf(minmax(x, 0.0, 1.0), minmax(y, 0.0, 1.0));
}

int PropertyCurveCanvas::HitPoint(Point p) const
{
    int radius = DPI(7);
    int best = -1;
    int best_d2 = radius * radius;
    for(int i = 0; i < points_.GetCount(); i++) {
        Point q = CurveToClient(points_[i]);
        int dx = p.x - q.x;
        int dy = p.y - q.y;
        int d2 = dx * dx + dy * dy;
        if(d2 <= best_d2) {
            best_d2 = d2;
            best = i;
        }
    }
    return best;
}

void PropertyCurveCanvas::Normalize()
{
    Value normalized = PropertyEditorNormalizeCurve(PropertyEditorMakeCurve(points_));
    points_ = PropertyEditorReadCurve(normalized);
    if(selected_ >= points_.GetCount())
        selected_ = points_.GetCount() - 1;
}

void PropertyCurveCanvas::EmitPreview()
{
    WhenCurvePreview(GetCurve());
}

void PropertyCurveCanvas::EmitCommit()
{
    WhenCurveCommit(GetCurve());
}

void PropertyCurveCanvas::Paint(Draw& w)
{
    Size sz = GetSize();
    Color paper = SColorPaper();
    Color frame = SColorShadow();
    Color grid = Blend(SColorPaper(), SColorText(), 28);
    Color line = SColorHighlight();

    w.DrawRect(sz, paper);
    Rect r = GetGraphRect();
    w.DrawRect(r, paper);
    DrawFrame(w, r, frame);

    for(int i = 1; i < 4; i++) {
        int x = r.left + r.GetWidth() * i / 4;
        int y = r.top + r.GetHeight() * i / 4;
        w.DrawLine(x, r.top, x, r.bottom, 1, grid);
        w.DrawLine(r.left, y, r.right, y, 1, grid);
    }

    for(int i = 1; i < points_.GetCount(); i++) {
        Point a = CurveToClient(points_[i - 1]);
        Point b = CurveToClient(points_[i]);
        w.DrawLine(a.x, a.y, b.x, b.y, DPI(2), line);
    }

    int radius = DPI(5);
    for(int i = 0; i < points_.GetCount(); i++) {
        Point p = CurveToClient(points_[i]);
        Color fill = i == selected_ ? SColorHighlight() : SColorFace();
        Color ink = i == selected_ ? SColorHighlightText() : SColorText();
        w.DrawEllipse(RectC(p.x - radius, p.y - radius, 2 * radius + 1, 2 * radius + 1),
                      fill, 1, ink);
    }
}

void PropertyCurveCanvas::LeftDown(Point p, dword)
{
    SetFocus();
    int hit = HitPoint(p);
    if(hit < 0) {
        points_.Add(ClientToCurve(p));
        Normalize();
        hit = HitPoint(p);
    }
    selected_ = hit;
    dragging_ = hit;
    SetCapture();
    Refresh();
    EmitPreview();
}

void PropertyCurveCanvas::MouseMove(Point p, dword keyflags)
{
    if(dragging_ < 0 || !(keyflags & K_MOUSELEFT))
        return;

    Pointf value = ClientToCurve(p);
    if(dragging_ == 0)
        value.x = 0.0;
    if(dragging_ == points_.GetCount() - 1)
        value.x = 1.0;

    points_[dragging_] = value;
    Normalize();
    selected_ = HitPoint(CurveToClient(value));
    dragging_ = selected_;
    Refresh();
    EmitPreview();
}

void PropertyCurveCanvas::LeftUp(Point, dword)
{
    if(dragging_ < 0)
        return;
    dragging_ = -1;
    ReleaseCapture();
    Normalize();
    Refresh();
    EmitCommit();
}

bool PropertyCurveCanvas::Key(dword key, int count)
{
    if(key == K_DELETE || key == K_BACKSPACE) {
        DeleteSelected();
        return true;
    }
    return Ctrl::Key(key, count);
}

Size PropertyCurveCanvas::GetMinSize() const
{
    return Size(DPI(280), DPI(220));
}

bool EditPropertyCurve(Value& value, Ctrl *owner)
{
    PropertyCurveDialog dlg;
    dlg.SetCurve(value);
    if(owner)
        dlg.CenterOwner();
    if(dlg.Run() != IDOK)
        return false;
    value = PropertyEditorNormalizeCurve(dlg.GetCurve());
    return true;
}

}
