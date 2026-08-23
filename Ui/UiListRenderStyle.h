#ifndef _Ui_UiListRenderStyle_h_
#define _Ui_UiListRenderStyle_h_

#include <Ui/UiList.h>

namespace Upp {

// Canonical projection from the owning List style into the built-in row renderer.
// The List owns viewport chrome; rows intentionally do not inherit the viewport Skin,
// shadow or focus ring. Advanced callers that supply a custom-styled UiItemRender keep
// full renderer ownership and bypass this projection.
inline UiItemRenderStyle UiListOwnedItemRenderStyle(const UiList::Style& list, int row_index)
{
    UiItemRenderStyle out;
    out.palette = list.palette;

    UiFill normal_face = list.palette.face[ST_NORMAL];
    if(list.striped_rows) {
        Color stripe = (row_index & 1) ? list.row_odd_face : list.row_even_face;
        if(!IsNull(stripe))
            normal_face = UiFill::Solid(stripe);
    }

    out.palette.face[ST_NORMAL] = normal_face;
    out.palette.face[ST_HOT] = list.hot_as_underline
        ? normal_face : UiFill::Solid(list.hot_face);
    out.palette.face[ST_PRESSED] = list.selected_as_underline
        ? normal_face : UiFill::Solid(list.selected_face);

    out.palette.frame[ST_NORMAL] = Null;
    out.palette.frame[ST_DISABLED] = Null;
    out.palette.frame[ST_HOT] = list.row_state_frame_enabled ? list.hot_frame : Null;
    out.palette.frame[ST_PRESSED] = list.row_state_frame_enabled ? list.selected_frame : Null;

    out.palette.ink[ST_NORMAL] = list.ink;
    out.palette.ink[ST_HOT] = list.hot_ink;
    out.palette.ink[ST_PRESSED] = list.selected_ink;
    out.palette.ink[ST_DISABLED] = list.disabled_ink;
    out.palette.icon[ST_HOT] = list.hot_ink;
    out.palette.icon[ST_PRESSED] = list.selected_ink;
    out.palette.icon[ST_DISABLED] = list.disabled_ink;

    out.metrics = list.metrics;
    out.metrics.face_enabled = true;
    out.metrics.frame_enabled = list.row_state_frame_enabled;
    out.metrics.frame_width = max(DPI(1), list.metrics.frame_width);
    out.metrics.radius = max(0, list.row_radius);
    out.metrics.content_margin = Rect(max(0, list.h_padding), max(0, list.v_padding),
                                      max(0, list.h_padding), max(0, list.v_padding));
    out.metrics.shadow.enabled = false;
    out.metrics.focus_enabled = false;
    out.skin = StyledSkin();

    out.title_font = list.font;
    out.subtitle_font = list.font;
    out.subtitle_font.Height(max(DPI(9), list.font.GetHeight() - DPI(1)));
    out.description_font = out.subtitle_font;
    out.right_font = list.font;
    out.icon_size = max(1, list.icon_size);
    out.check_size = max(1, list.check_size);
    out.content_gap = max(0, list.content_gap);
    out.metadata_size = max(1, list.metadata_size);
    out.metadata_gap = max(0, list.metadata_gap);
    out.muted_ink = list.muted_ink;
    out.metadata_default = list.metadata_default;
    out.check_frame = list.check_frame;
    out.check_fill = list.check_fill;
    out.show_face = true;
    out.show_icon = list.show_icons;
    out.show_metadata = list.show_metadata_marker;
    return out;
}

inline UiItemRenderData UiListOwnedItemRenderData(const UiList::Style& list,
                                                   const UiModelItem& item)
{
    UiItemRenderData data = UiMakeItemRenderData(item);
    if(!list.show_checks) {
        data.has_check = false;
        data.checked = false;
    }
    if(!list.show_icons)
        data.icon = Image();
    if(!list.show_metadata_marker)
        data.has_metadata = false;
    // Badge mode is List-owned chrome. Suppress renderer right text so the same
    // semantic value is not laid out and painted twice.
    if(list.right_text_as_badge)
        data.right_text.Clear();
    return data;
}

} // namespace Upp

#endif
