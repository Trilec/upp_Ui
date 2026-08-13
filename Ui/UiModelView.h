#ifndef _Ui_UiModelView_h_
#define _Ui_UiModelView_h_

/*
    UiModelView
    ===========

    Purpose
    - Shared arithmetic helpers for high-scale model-backed Ui controls.

    Intent
    - Keep viewport work proportional to visible content rather than total model size.
    - Use uniform extents for the fast path used by UiList and UiGallery.
    - Keep geometry calculations overflow-safe for very large logical item counts.

    Thread context
    - Pure helpers; no GUI state is mutated.
*/

#include <Core/Core.h>

namespace Upp {

struct UiVisibleRange {
    int first = -1;
    int last = -1;

    bool IsEmpty() const { return first < 0 || last < first; }
    int GetCount() const { return IsEmpty() ? 0 : last - first + 1; }
    bool Contains(int index) const { return !IsEmpty() && index >= first && index <= last; }
};

inline int UiUniformContentExtent(int item_count, int item_size, int gap)
{
    if(item_count <= 0)
        return 0;
    int64 count = item_count;
    int64 size = max(1, item_size);
    int64 spacing = max(0, gap);
    int64 gaps = count > 1 ? (count - 1) * spacing : 0;
    int64 extent = count * size + gaps;
    return extent >= INT_MAX ? INT_MAX : (int)extent;
}

inline UiVisibleRange UiComputeLinearVisibleRange(int item_count,
                                                   int scroll_pos,
                                                   int viewport_extent,
                                                   int item_size,
                                                   int gap = 0,
                                                   int overscan_items = 0)
{
    UiVisibleRange out;
    if(item_count <= 0 || viewport_extent <= 0)
        return out;

    int size = max(1, item_size);
    int spacing = max(0, gap);
    int extent = max(1, size + spacing);
    int scroll = max(0, scroll_pos);
    int overscan = max(0, overscan_items);

    int first = scroll / extent;
    int64 last_pixel = (int64)scroll + max(0, viewport_extent - 1);
    int64 last64 = last_pixel / extent;
    int last = (int)(last64 >= item_count ? item_count - 1 : last64);

    first = max(0, first - overscan);
    last = min(item_count - 1, last + overscan);

    if(first >= item_count || last < 0 || last < first)
        return UiVisibleRange();

    out.first = first;
    out.last = last;
    return out;
}

inline int UiComputeUniformInsertBefore(int item_count,
                                        int64 logical_pos,
                                        int item_size,
                                        int gap = 0)
{
    if(item_count <= 0)
        return 0;
    int size = max(1, item_size);
    int extent = max(1, size + max(0, gap));
    int64 first_mid = size / 2;
    if(logical_pos < first_mid)
        return 0;
    int64 before = 1 + (logical_pos - first_mid) / extent;
    return before >= item_count ? item_count : (int)before;
}

inline UiVisibleRange UiComputeGridVisibleRange(int item_count,
                                                 int columns,
                                                 int scroll_pos,
                                                 int viewport_extent,
                                                 int item_height,
                                                 int row_gap = 0,
                                                 int overscan_rows = 0)
{
    UiVisibleRange out;
    if(item_count <= 0 || columns <= 0 || viewport_extent <= 0)
        return out;

    int rows = (int)(((int64)item_count + columns - 1) / columns);
    UiVisibleRange row_range = UiComputeLinearVisibleRange(rows,
                                                           scroll_pos,
                                                           viewport_extent,
                                                           item_height,
                                                           row_gap,
                                                           overscan_rows);
    if(row_range.IsEmpty())
        return out;

    int64 first = (int64)row_range.first * columns;
    int64 last = ((int64)row_range.last + 1) * columns - 1;
    out.first = (int)(first >= item_count ? item_count - 1 : first);
    out.last = (int)(last >= item_count ? item_count - 1 : last);
    return out;
}

} // namespace Upp

#endif
