#ifndef _Ui_UiLayoutCursor_h_
#define _Ui_UiLayoutCursor_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.
    UiLayoutCursor
    ==============

    Purpose
    - Tiny cursor helper for explicit manual layout in demo shells and other
      lightweight placement code.

    Intent
    - Reduce scattered x/y arithmetic without replacing real layout classes.
    - Keep manual placement readable for humans by pairing "take" operations
      with automatic cursor advancement.
    - Make it obvious when a line, indent, gap, or width is being consumed.

    Scope
    - Use this where layout is intentionally manual and simple.
    - Do not use it to replace UiBoxLayout / UiGridLayout inside reusable
      controls that already benefit from a structural layout container.

    Model
    - The cursor owns one area rectangle.
    - It tracks a left-to-right cursor (`x_`) and a right-to-left cursor
      (`rx_`) for the current line.
    - "Take" methods return the current coordinate and also advance the
      matching cursor.
    - `NextLine()` moves to the next row and restores both horizontal cursors
      to the active indented bounds.

    Typical usage
    - SetArea(body);
    - SetLineHeight(...);
    - SetGapX(...), SetGapY(...);
    - SetIndent(...);
    - Use `TakeIncrX(...)` for left-to-right rows.
    - Use `TakeDecrX(...)` for right-aligned shell clusters.

    Changelog
    - 2026-04: introduced as a lightweight manual-layout cursor for demo
      shell cleanup after repeated hard-coded placement regressions.
*/

#include <CtrlCore/CtrlCore.h>

namespace Upp {

class UiLayoutCursor {
public:
    UiLayoutCursor() = default;
    explicit UiLayoutCursor(const Rect& area) { SetArea(area); }

    UiLayoutCursor& SetArea(const Rect& area)
    {
        area_ = area;
        Reset();
        return *this;
    }

    UiLayoutCursor& Reset()
    {
        y_ = area_.top;
        ResetX();
        return *this;
    }

    UiLayoutCursor& ResetX()
    {
        x_ = Left();
        rx_ = Right();
        return *this;
    }

    UiLayoutCursor& ResetY()
    {
        y_ = area_.top;
        return *this;
    }

    UiLayoutCursor& ResetLine()
    {
        return Reset();
    }

    UiLayoutCursor& SetIndent(int left, int right = 0)
    {
        indent_left_ = max(0, left);
        indent_right_ = max(0, right);
        ResetX();
        return *this;
    }

    UiLayoutCursor& SetGapX(int gap)
    {
        gap_x_ = max(0, gap);
        return *this;
    }

    UiLayoutCursor& SetGapY(int gap)
    {
        gap_y_ = max(0, gap);
        return *this;
    }

    UiLayoutCursor& SetLineHeight(int h)
    {
        line_h_ = max(0, h);
        return *this;
    }

    UiLayoutCursor& SetRowMetrics(int h, int gap_y)
    {
        line_h_ = max(0, h);
        gap_y_ = max(0, gap_y);
        return *this;
    }

    UiLayoutCursor& IncrX(int dx)
    {
        x_ += dx;
        return *this;
    }

    UiLayoutCursor& DecrX(int dx)
    {
        rx_ -= dx;
        return *this;
    }

    UiLayoutCursor& IncrY(int dy)
    {
        y_ += dy;
        return *this;
    }

    UiLayoutCursor& NextLine(int h = -1, int gap = -1)
    {
        y_ += RowH(h) + GapY(gap);
        ResetX();
        return *this;
    }

    int TakeIncrX(int width, int gap = -1)
    {
        int out = x_;
        x_ += max(0, width) + GapX(gap);
        return out;
    }

    int TakeDecrX(int width, int gap = -1)
    {
        int w = max(0, width);
        rx_ -= w;
        int out = rx_;
        rx_ -= GapX(gap);
        return out;
    }

    int X() const        { return x_; }
    int Y() const        { return y_; }
    int Left() const     { return area_.left + indent_left_; }
    int Right() const    { return max(Left(), area_.right - indent_right_); }
    int FullW() const    { return max(0, Right() - Left()); }
    int AvailW() const   { return max(0, rx_ - x_); }
    int LineH() const    { return line_h_; }
    int RowH(int h) const { return h >= 0 ? h : line_h_; }

    int GapX(int gap = -1) const
    {
        return gap >= 0 ? max(0, gap) : gap_x_;
    }

    int GapY(int gap = -1) const
    {
        return gap >= 0 ? max(0, gap) : gap_y_;
    }

private:
    Rect area_;
    int indent_left_ = 0;
    int indent_right_ = 0;
    int x_ = 0;
    int rx_ = 0;
    int y_ = 0;
    int line_h_ = 0;
    int gap_x_ = 0;
    int gap_y_ = 0;
};

}

#endif

