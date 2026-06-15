#ifndef _Ui_UiBezierCurveField_h_
#define _Ui_UiBezierCurveField_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    UiBezierCurveField
    ==================

    Purpose
    - Public header for the UiBezierCurveField component.

    Intent
    - Define the runtime API, style contract, and integration points used by the rest of the Ui package.

    Thread context
    - GUI thread only.

    Usage
    - Include this header where the component is used or extended. Keep implementation details in the matching .cpp when present.

    Changelog
    - 2026-06: normalized the top-level header documentation.
*/

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.
    UiBezierCurveField.h
    ====================

    Purpose
    - Reusable Bezier curve field combining an editor surface with an optional
      selectable formula row and copy action.

    Intent
    - Package the common "edit curve + inspect/copy values" workflow into a
      small first-class Ui control, while keeping UiBezierCurveEditor usable as
      the low-level primitive.

    Changelog
    - 2026-04: extracted from shadow-curve work in UiPanelDemo.
*/

#include <Ui/UiBezierCurveEditor.h>
#include <Ui/UiTheme.h>
#include <Ui/UiLabel.h>
#include <Ui/UiButton.h>
#include <Ui/UiIcons.h>

namespace Upp {

class UiBezierCurveField : public ParentCtrl {
public:
    typedef UiBezierCurveField CLASSNAME;

    UiBezierCurveField();

    UiBezierCurveField& SetCurve(const ShadowCurve& c);
    const ShadowCurve&  GetCurve() const { return editor_.GetCurve(); }

    UiBezierCurveField& SetShowFormula(bool on = true);
    UiBezierCurveField& SetShowCopy(bool on = true);
    UiBezierCurveField& SetEditable(bool on = true);
    UiBezierCurveField& SetFlipHorizontal(bool on = true);
    UiBezierCurveField& SetFlipVertical(bool on = true);
    UiBezierCurveField& SetFormulaSelectable(bool on = true);
    UiBezierCurveField& SetCurveStyle(const UiBezierCurveEditor::Style& s);
    UiBezierCurveField& SetFormulaStyle(const UiLabel::Style& s);
    UiBezierCurveField& SetCopyStyle(const UiButton::Style& s);

    bool IsShowFormula() const { return show_formula_; }
    bool IsShowCopy() const    { return show_copy_; }
    bool IsEditable() const    { return editor_.IsEditable(); }

    void  SetData(const Value& v) override;
    Value GetData() const override;
    Size  GetMinSize() const override;
    void  Layout() override;

    UiBezierCurveEditor& Editor()       { return editor_; }
    const UiBezierCurveEditor& Editor() const { return editor_; }

    Event<> WhenChanging;
    Event<> WhenAction;

private:
    void   SyncFormula();
    String BuildFormulaText() const;

    UiBezierCurveEditor editor_;
    UiLabel             formula_;
    UiButton            copy_;
    bool                show_formula_ = true;
    bool                show_copy_ = true;
    int                 row_gap_ = DPI(4);
    int                 row_height_ = DPI(20);
};

}

#endif

