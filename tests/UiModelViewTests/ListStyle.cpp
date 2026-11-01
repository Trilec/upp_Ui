#include <Ui/UiListRenderStyle.h>
#include <Ui/UiIcons.h>

using namespace Upp;

namespace {
int checks = 0;
int failed = 0;

void Check(bool ok, const char *message)
{
    checks++;
    if(!ok) {
        failed++;
        Cout() << "FAIL: " << message << '\n';
    }
}
}

int RunListStyleSuite()
{
    UiList::Style style = UiList::StyleDefault();
    style.hot_face = Color(10, 20, 30);
    style.hot_frame = Color(40, 50, 60);
    style.hot_ink = Color(70, 80, 90);
    style.selected_face = Color(11, 21, 31);
    style.selected_frame = Color(41, 51, 61);
    style.selected_ink = Color(71, 81, 91);
    style.row_state_frame_enabled = true;
    style.row_radius = 7;
    style.h_padding = 9;
    style.v_padding = 5;
    style.striped_rows = true;
    style.row_even_face = Color(101, 102, 103);
    style.row_odd_face = Color(111, 112, 113);
    style.show_checks = false;
    style.show_icons = false;
    style.show_metadata_marker = false;

    UiItemRenderStyle even = UiListOwnedItemRenderStyle(style, 0);
    UiItemRenderStyle odd = UiListOwnedItemRenderStyle(style, 1);
    Check(even.palette.face[ST_NORMAL].IsSolid() &&
              even.palette.face[ST_NORMAL].color == style.row_even_face,
          "even striped row uses authored even face");
    Check(odd.palette.face[ST_NORMAL].IsSolid() &&
              odd.palette.face[ST_NORMAL].color == style.row_odd_face,
          "odd striped row uses authored odd face");
    Check(even.palette.face[ST_HOT].IsSolid() &&
              even.palette.face[ST_HOT].color == style.hot_face &&
              even.palette.ink[ST_HOT] == style.hot_ink,
          "hot row face and ink come from owning List style");
    Check(even.palette.face[ST_PRESSED].IsSolid() &&
              even.palette.face[ST_PRESSED].color == style.selected_face &&
              even.palette.ink[ST_PRESSED] == style.selected_ink,
          "selected row face and ink come from owning List style");
    Check(even.palette.frame[ST_HOT] == style.hot_frame &&
              even.palette.frame[ST_PRESSED] == style.selected_frame,
          "row state frames honor List row-state frame enablement");
    Check(even.metrics.radius == style.row_radius &&
              even.metrics.content_margin == Rect(style.h_padding, style.v_padding,
                                                   style.h_padding, style.v_padding),
          "row radius and padding project into the built-in renderer");
    Check(!even.metrics.shadow.enabled && !even.metrics.focus_enabled &&
              !even.skin.enabled,
          "row renderer does not inherit viewport Skin shadow or focus chrome");

    UiModelItem item("Row", 1);
    item.has_check = true;
    item.checked = true;
    item.icon = ICON_DESIGN_DRAG_INDICATOR_48();
    item.has_metadata = true;
    UiItemRenderData data = UiListOwnedItemRenderData(style, item);
    Check(!data.has_check && !data.checked,
          "List show_checks=false suppresses default-renderer check data");
    Check(IsNull(data.icon),
          "List show_icons=false suppresses default-renderer icon data");
    Check(!data.has_metadata,
          "List show_metadata_marker=false suppresses default-renderer metadata data");

    UiItemRenderStyle reserved = UiListOwnedItemRenderStyle(style, 0);
    int base_left = reserved.metrics.content_margin.left;
    int base_right = reserved.metrics.content_margin.right;
    UiReserveItemRenderDecoration(reserved, UiAlign::LEFT, 20, 6);
    UiReserveItemRenderDecoration(reserved, UiAlign::LEFT, 12, 4);
    UiReserveItemRenderDecoration(reserved, UiAlign::RIGHT, 30, 8);
    Check(reserved.metrics.content_margin.left == base_left + 42 &&
              reserved.metrics.content_margin.right == base_right + 38,
          "multiple external decoration lanes accumulate actual extent plus gap on each side");

    style.right_text_as_badge = true;
    item.right_text = "Exact";
    data = UiListOwnedItemRenderData(style, item);
    Check(data.right_text.IsEmpty(),
          "List-owned badge mode removes duplicate renderer right text");
    style.right_text_as_badge = false;

    style.hot_as_underline = true;
    style.selected_as_underline = true;
    UiItemRenderStyle underline = UiListOwnedItemRenderStyle(style, 0);
    Check(underline.palette.face[ST_HOT].IsSolid() &&
              underline.palette.face[ST_HOT].color == style.row_even_face &&
              underline.palette.face[ST_PRESSED].IsSolid() &&
              underline.palette.face[ST_PRESSED].color == style.row_even_face,
          "underline state mode keeps the row base face instead of applying state fills");

    style.row_state_frame_enabled = false;
    UiItemRenderStyle frameless = UiListOwnedItemRenderStyle(style, 0);
    Check(IsNull(frameless.palette.frame[ST_HOT]) &&
              IsNull(frameless.palette.frame[ST_PRESSED]) &&
              !frameless.metrics.frame_enabled,
          "disabled row-state frames remain genuinely frameless");

    StringStream stored;
    style.Serialize(stored);
    UiList::Style restored = UiList::StyleDefault();
    StringStream loading(stored.GetResult());
    restored.Serialize(loading);
    Check(restored.striped_rows == style.striped_rows &&
              restored.row_even_face == style.row_even_face &&
              restored.row_odd_face == style.row_odd_face,
          "List striped-row style survives serialization round trip");

    Cout() << "UILIST_STYLE_CONTRACT_SUMMARY checks=" << checks
           << " failed=" << failed << '\n';
    return failed ? 1 : 0;
}
