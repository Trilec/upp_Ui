// Size presets remeasure the same owned controls. Always scale from the Medium
// baseline; never accumulate mutations from a previously selected preset.
#include "UiFileBrowser.h"

namespace Upp
{

int UiFileBrowser::SizePx(int logical_pixels) const
{
    const int scale = view_size_ == ViewSize::Small ? 11 : view_size_ == ViewSize::Large ? 15 : 13;
    // Host font size is the baseline; density is a separate relative choice.
    return max(1, int((int64(logical_pixels) * scale * max(1, base_font_.GetHeight()) + 84) / 169));
}

void UiFileBrowser::ApplyViewSizeLayout()
{
    // Derive every preset from the Medium dimensions, so switching never drifts.
    navigation_layout_.SetInset(Rect(SizePx(6), SizePx(3), SizePx(6), SizePx(3))).SetGap(SizePx(2));
    RebuildNavigation();
    footer_layout_.SetInset(Rect(SizePx(9), SizePx(7), SizePx(9), SizePx(7))).SetGap(SizePx(6));
    footer_layout_.ItemAt(3).Fixed(SizePx(220));
    footer_layout_.ItemAt(4).Fixed(SizePx(78));
    footer_layout_.ItemAt(5).Fixed(SizePx(132));
    places_layout_.SetInset(SizePx(7)).SetGap(SizePx(4));
    places_layout_.ItemAt(0).Fixed(SizePx(26));
    commands_layout_.SetInset(Rect(SizePx(8), SizePx(2), SizePx(8), SizePx(2))).SetGap(SizePx(10));
    for(UiBoxLayout *layout : {&filter_layout_, &sequence_layout_, &view_layout_, &sort_layout_})
        layout->SetInset(SizePx(6)).SetGap(SizePx(4));
    filter_layout_.ItemAt(0).Fixed(SizePx(22));
    filter_layout_.ItemAt(1).Fixed(SizePx(24));
    filter_layout_.ItemAt(3).Fixed(SizePx(26));
    filter_layout_.ItemAt(4).Fixed(SizePx(16));
    filter_editor_.SetGap(SizePx(4));
    filter_editor_.ItemAt(0).Fixed(SizePx(78));
    filter_editor_.ItemAt(1).Fixed(SizePx(76));
    filter_editor_.ItemAt(3).Fixed(SizePx(46));
    filter_editor_.ItemAt(4).Fixed(SizePx(52));
    filter_enable_row_.ItemAt(1).Fixed(SizePx(30));
    sequence_layout_.ItemAt(0).Fixed(SizePx(18));
    for(int i = 1; i <= 3; ++i)
        sequence_layout_.ItemAt(i).Fixed(SizePx(24));
    sequence_layout_.ItemAt(4).Fixed(SizePx(22));
    sequence_layout_.ItemAt(5).Fixed(SizePx(16));
    sequence_layout_.ItemAt(6).Fixed(SizePx(8));
    view_layout_.ItemAt(0).Fixed(SizePx(18));
    view_layout_.ItemAt(1).Fixed(SizePx(28));
    for(int i = 2; i <= 4; ++i)
        view_layout_.ItemAt(i).Fixed(SizePx(22));
    for(UiBoxLayout *row : {&size_row_, &frames_row_, &inspector_row_})
        row->ItemAt(1).Fixed(SizePx(30));
    sort_layout_.ItemAt(0).Fixed(SizePx(18));
    sort_layout_.ItemAt(1).Fixed(SizePx(24));
    sort_layout_.ItemAt(2).Fixed(SizePx(24));
    inspector_layout_.SetInset(SizePx(9)).SetGap(SizePx(6));
    preview_layout_.SetInset(SizePx(10)).SetGap(SizePx(4));
    preview_panel_.SetSizeMin(SizePx(180), SizePx(150));
    inspector_meta_.SetGap(SizePx(4)).SetMinCellSize(Size(SizePx(64), SizePx(20)));
    RebuildInspectorDetails();
    gallery_.SetItemSize(Size(SizePx(158), SizePx(126))).SetGap(SizePx(8)).SetInset(SizePx(8));
    RebuildShell();
    RebuildWorkspace();
    if(options_open_)
        PositionOptions();
}

} // namespace Upp
