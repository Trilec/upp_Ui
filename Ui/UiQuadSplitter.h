#ifndef _Ui_UiQuadSplitter_h_
#define _Ui_UiQuadSplitter_h_

/*
    UiQuadSplitter
    ==============

    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    Purpose
    - Four-pane splitter container built from the UiSplitter primitive.

    Intent
    - Provide the common IDE/designer four-pane layout without making
      UiSplitter itself carry a special quad mode.
    - Keep pane ownership with caller-supplied child controls. The quad
      splitter only reserves and resizes regions.

    Thread context
    - GUI thread only.

    Usage
    - Call Set(top_left, top_right, bottom_left, bottom_right), then adjust
      SetColumnSplitPercent() and SetRowSplitPercent().

    Changelog
    - 2026-05: initial four-pane splitter composed from UiSplitter.
*/

#include <Ui/UiSplitter.h>

namespace Upp {

class UiQuadSplitter : public Ctrl {
public:
    typedef UiQuadSplitter CLASSNAME;

    UiQuadSplitter();
    virtual ~UiQuadSplitter() {}

    UiQuadSplitter& Set(Ctrl& top_left, Ctrl& top_right, Ctrl& bottom_left, Ctrl& bottom_right);
    void Add(Ctrl& pane);
    void Remove(Ctrl& pane);
    UiQuadSplitter& operator<<(Ctrl& pane) { Add(pane); return *this; }
    void Clear();
    UiQuadSplitter& SetColumnSplitPercent(double percent);
    UiQuadSplitter& SetRowSplitPercent(double percent);
    UiQuadSplitter& SetSplitPercent(double column_percent, double row_percent)
    {
        return SetColumnSplitPercent(column_percent).SetRowSplitPercent(row_percent);
    }

    double GetColumnSplitPercent() const { return column_percent_; }
    double GetRowSplitPercent() const { return row_percent_; }

    UiQuadSplitter& SetMinPixels(int pane, int px);
    UiQuadSplitter& SetSplitterStyle(const UiSplitter::Style& style);
    UiQuadSplitter& ClearSplitterStyle();

    UiSplitter& RootSplitter() { return root_; }
    UiSplitter& TopSplitter() { return top_; }
    UiSplitter& BottomSplitter() { return bottom_; }

    const UiSplitter& RootSplitter() const { return root_; }
    const UiSplitter& TopSplitter() const { return top_; }
    const UiSplitter& BottomSplitter() const { return bottom_; }

    virtual void Layout() override;
    virtual Size GetMinSize() const override;
    Size GetContentSize() const;

    Event<> WhenSplitFinish;

private:
    void Wire();
    void SyncColumnFromTop();
    void SyncColumnFromBottom();
    void SyncRowFromRoot();

    UiSplitter root_;
    UiSplitter top_;
    UiSplitter bottom_;
    Ctrl* panes_[4] = {};
    double column_percent_ = 50.0;
    double row_percent_ = 50.0;
};

} // namespace Upp

#endif
