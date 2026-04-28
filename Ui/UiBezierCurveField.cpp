#include <Ui/UiBezierCurveField.h>

namespace Upp {

UiBezierCurveField::UiBezierCurveField()
{
    Add(editor_);
    Add(formula_);
    Add(copy_);

    formula_.SetSelectable(true).NoWantFocus();
    copy_.SetText("").SetIcon(ICON_CONTENT_CONTENT_COPY_48()).NoWantFocus();

    formula_.SetStyle(UiTheme::ResolveLabel(UiLabelRole::Body));
    copy_.SetStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));

    copy_.WhenAction = [=] { WriteClipboardText(BuildFormulaText()); };
    editor_.WhenChanging = [=] {
        SyncFormula();
        WhenChanging();
    };
    editor_.WhenAction = [=] {
        SyncFormula();
        WhenAction();
    };

    SyncFormula();
}

UiBezierCurveField& UiBezierCurveField::SetCurve(const ShadowCurve& c)
{
    editor_.SetCurve(c);
    SyncFormula();
    return *this;
}

UiBezierCurveField& UiBezierCurveField::SetShowFormula(bool on)
{
    if(show_formula_ == on)
        return *this;
    show_formula_ = on;
    formula_.Show(on);
    RefreshLayout();
    return *this;
}

UiBezierCurveField& UiBezierCurveField::SetShowCopy(bool on)
{
    if(show_copy_ == on)
        return *this;
    show_copy_ = on;
    copy_.Show(on);
    RefreshLayout();
    return *this;
}

UiBezierCurveField& UiBezierCurveField::SetEditable(bool on)
{
    editor_.SetEditable(on);
    return *this;
}

UiBezierCurveField& UiBezierCurveField::SetFlipHorizontal(bool on)
{
    editor_.SetFlipHorizontal(on);
    return *this;
}

UiBezierCurveField& UiBezierCurveField::SetFlipVertical(bool on)
{
    editor_.SetFlipVertical(on);
    return *this;
}

UiBezierCurveField& UiBezierCurveField::SetFormulaSelectable(bool on)
{
    formula_.SetSelectable(on);
    return *this;
}

UiBezierCurveField& UiBezierCurveField::SetCurveStyle(const UiBezierCurveEditor::Style& s)
{
    editor_.SetStyle(s);
    return *this;
}

UiBezierCurveField& UiBezierCurveField::SetFormulaStyle(const UiLabel::Style& s)
{
    formula_.SetStyle(s);
    return *this;
}

UiBezierCurveField& UiBezierCurveField::SetCopyStyle(const UiButton::Style& s)
{
    copy_.SetStyle(s);
    return *this;
}

void UiBezierCurveField::SetData(const Value& v)
{
    editor_.SetData(v);
    SyncFormula();
}

Value UiBezierCurveField::GetData() const
{
    return editor_.GetData();
}

Size UiBezierCurveField::GetMinSize() const
{
    Size sz = editor_.GetMinSize();
    if(show_formula_ || show_copy_)
        sz.cy += row_gap_ + row_height_;
    return sz;
}

void UiBezierCurveField::Layout()
{
    Rect r = GetSize();
    if(r.IsEmpty())
        return;

    bool show_row = show_formula_ || show_copy_;
    if(!show_row) {
        editor_.SetRect(r);
        formula_.SetRect(0, 0, 0, 0);
        copy_.SetRect(0, 0, 0, 0);
        return;
    }

    int row_h = row_height_;
    editor_.SetRect(r.left, r.top, r.GetWidth(), max(0, r.GetHeight() - row_gap_ - row_h));
    int y = editor_.GetRect().bottom + row_gap_;
    int copy_w = show_copy_ ? DPI(22) : 0;
    int gap = show_formula_ && show_copy_ ? DPI(4) : 0;
    if(show_formula_)
        formula_.SetRect(r.left, y, max(0, r.GetWidth() - copy_w - gap), row_h);
    else
        formula_.SetRect(0, 0, 0, 0);
    if(show_copy_)
        copy_.SetRect(r.right - copy_w, y - DPI(1), copy_w, row_h + DPI(2));
    else
        copy_.SetRect(0, 0, 0, 0);
}

String UiBezierCurveField::BuildFormulaText() const
{
    const ShadowCurve& c = editor_.GetCurve();
    return Format("[ Bezier(%.3f, %.3f, %.3f, %.3f) ]", c.x1, c.y1, c.x2, c.y2);
}

void UiBezierCurveField::SyncFormula()
{
    formula_.SetText(BuildFormulaText());
}

}
