#ifndef _Ui_UiIcons_h_
#define _Ui_UiIcons_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    UiIcons
    =======

    Purpose
    - Public header for the UiIcons component.

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
    UiIcons
    =======

    Purpose
    - Public icon catalog and thin ICON_* wrappers over the shared IML-backed
      Ui icon image class.

    V1 contract
    - Icons are source-neutral at the control boundary.
    - Designer stores stable icon ids/names and codegen resolves through this
      catalog.
    - Future SVG/vector sources must plug in behind the same cache-aware image
      path; controls should not depend on icon source format.
*/

#include <CtrlCore/CtrlCore.h>
#include <Ui/UiDataModels.h>
namespace Upp {

#define IMAGECLASS UiIconsImg
#define IMAGEFILE <Ui/UiIcons.iml>
#include <Draw/iml_header.h>

using UiIconFactoryFn = Image (*)();

inline String UiIconDisplayName(const String& raw_name)
{
    String s = raw_name;
    if(s.StartsWith("ICON_"))
        s = s.Mid(5);
    if(s.EndsWith("_48"))
        s = s.Left(s.GetCount() - 3);
    s.Replace("_", " ");
    s = ToLower(s);

    String out;
    bool word_start = true;
    for(int i = 0; i < s.GetCount(); i++) {
        int c = s[i];
        if(IsSpace(c)) {
            out.Cat(' ');
            word_start = true;
        }
        else {
            out.Cat(word_start ? ToUpper(c) : c);
            word_start = false;
        }
    }
    return out;
}

struct UiIconCatalogEntry : Moveable<UiIconCatalogEntry> {
    String          name;
    String          display_name;
    UiIconFactoryFn factory = nullptr;

    UiIconCatalogEntry() {}
    UiIconCatalogEntry(const String& n, UiIconFactoryFn f) : name(n), display_name(UiIconDisplayName(n)), factory(f) {}
    UiIconCatalogEntry(const String& n, const String& d, UiIconFactoryFn f) : name(n), display_name(d), factory(f) {}
};

inline Image ICON_NAVIGATION_OUTLINED_MORE_HORIZ_48()
{
    return UiIconsImg::ICON_NAVIGATION_OUTLINED_MORE_HORIZ_48();
}

inline Image ICON_NAVIGATION_OUTLINED_MENU_48()
{
    return UiIconsImg::ICON_NAVIGATION_OUTLINED_MENU_48();
}

inline Image ICON_NAVIGATION_OUTLINED_MORE_VERT_48()
{
    return UiIconsImg::ICON_NAVIGATION_OUTLINED_MORE_VERT_48();
}

inline Image ICON_NAVIGATION_OUTLINED_ARROW_RIGHT_48()
{
    return UiIconsImg::ICON_NAVIGATION_OUTLINED_ARROW_RIGHT_48();
}

inline Image ICON_NAVIGATION_OUTLINED_ARROW_LEFT_48()
{
    return UiIconsImg::ICON_NAVIGATION_OUTLINED_ARROW_LEFT_48();
}

inline Image ICON_NAVIGATION_OUTLINED_ARROW_DROP_UP_48()
{
    return UiIconsImg::ICON_NAVIGATION_OUTLINED_ARROW_DROP_UP_48();
}

inline Image ICON_NAVIGATION_OUTLINED_ARROW_DROP_DOWN_48()
{
    return UiIconsImg::ICON_NAVIGATION_OUTLINED_ARROW_DROP_DOWN_48();
}

inline Image ICON_NAVIGATION_OUTLINED_APPS_48()
{
    return UiIconsImg::ICON_NAVIGATION_OUTLINED_APPS_48();
}

inline Image ICON_HARDWARE_OUTLINED_KEYBOARD_ARROW_RIGHT_48()
{
    return UiIconsImg::ICON_HARDWARE_OUTLINED_KEYBOARD_ARROW_RIGHT_48();
}

inline Image ICON_HARDWARE_OUTLINED_KEYBOARD_ARROW_LEFT_48()
{
    return UiIconsImg::ICON_HARDWARE_OUTLINED_KEYBOARD_ARROW_LEFT_48();
}

inline Image ICON_CONTENT_OUTLINED_ADD_CIRCLE_OUTLINE_48()
{
    return UiIconsImg::ICON_CONTENT_OUTLINED_ADD_CIRCLE_OUTLINE_48();
}

inline Image ICON_CONTENT_OUTLINED_REMOVE_CIRCLE_OUTLINE_48()
{
    return UiIconsImg::ICON_CONTENT_OUTLINED_REMOVE_CIRCLE_OUTLINE_48();
}

inline Image ICON_CONTENT_OUTLINED_ADD_48()
{
    return UiIconsImg::ICON_CONTENT_OUTLINED_ADD_48();
}

inline Image ICON_CONTENT_OUTLINED_REMOVE_48()
{
    return UiIconsImg::ICON_CONTENT_OUTLINED_REMOVE_48();
}

inline Image ICON_ACTION_OUTLINED_VISIBILITY_48()
{
    return UiIconsImg::ICON_ACTION_OUTLINED_VISIBILITY_48();
}

inline Image ICON_ACTION_OUTLINED_VISIBILITY_OFF_48()
{
    return UiIconsImg::ICON_ACTION_OUTLINED_VISIBILITY_OFF_48();
}

inline Image ICON_NAVIGATION_OUTLINED_DRAG_INDICATOR_48()
{
    return UiIconsImg::ICON_NAVIGATION_OUTLINED_DRAG_INDICATOR_48();
}

inline Image ICON_DESIGN_CHECK_SMALL_48()
{
    return UiIconsImg::ICON_DESIGN_CHECK_SMALL_48();
}

inline Image ICON_DESIGN_DELETE_48()
{
    return UiIconsImg::ICON_DESIGN_DELETE_48();
}

inline Image ICON_DESIGN_DRAG_INDICATOR_48()
{
    return UiIconsImg::ICON_DESIGN_DRAG_INDICATOR_48();
}

inline Image ICON_DESIGN_FOLDER_48()
{
    return UiIconsImg::ICON_DESIGN_FOLDER_48();
}

inline Image ICON_DESIGN_HOME_48()
{
    return UiIconsImg::ICON_DESIGN_HOME_48();
}

inline Image ICON_DESIGN_IMAGE_48()
{
    return UiIconsImg::ICON_DESIGN_IMAGE_48();
}

inline Image ICON_DESIGN_LEFT_PANEL_CLOSE_48()
{
    return UiIconsImg::ICON_DESIGN_LEFT_PANEL_CLOSE_48();
}

inline Image ICON_DESIGN_LEFT_PANEL_OPEN_48()
{
    return UiIconsImg::ICON_DESIGN_LEFT_PANEL_OPEN_48();
}

inline Image ICON_DESIGN_LOWERCASE_48()
{
    return UiIconsImg::ICON_DESIGN_LOWERCASE_48();
}

inline Image ICON_DESIGN_MENU_48()
{
    return UiIconsImg::ICON_DESIGN_MENU_48();
}

inline Image ICON_DESIGN_RIGHT_PANEL_CLOSE_48()
{
    return UiIconsImg::ICON_DESIGN_RIGHT_PANEL_CLOSE_48();
}

inline Image ICON_DESIGN_RIGHT_PANEL_OPEN_48()
{
    return UiIconsImg::ICON_DESIGN_RIGHT_PANEL_OPEN_48();
}

inline Image ICON_DESIGN_SETTINGS_48()
{
    return UiIconsImg::ICON_DESIGN_SETTINGS_48();
}

inline Image ICON_DESIGN_UNFOLD_LESS_48()
{
    return UiIconsImg::ICON_DESIGN_UNFOLD_LESS_48();
}

inline Image ICON_DESIGN_UNFOLD_MORE_48()
{
    return UiIconsImg::ICON_DESIGN_UNFOLD_MORE_48();
}

inline Image ICON_DESIGN_UPPERCASE_48()
{
    return UiIconsImg::ICON_DESIGN_UPPERCASE_48();
}

inline Image ICON_EDITOR_BORDER_LEFT_48()
{
    return UiIconsImg::ICON_EDITOR_BORDER_LEFT_48();
}

inline Image ICON_EDITOR_BORDER_RIGHT_48()
{
    return UiIconsImg::ICON_EDITOR_BORDER_RIGHT_48();
}

inline Image ICON_ACTION_CANCEL_48()
{
    return UiIconsImg::ICON_ACTION_CANCEL_48();
}

inline Image ICON_ACTION_CHECK_CIRCLE_48()
{
    return UiIconsImg::ICON_ACTION_CHECK_CIRCLE_48();
}

inline Image ICON_EDITOR_CLARIFY_48()
{
    return UiIconsImg::ICON_EDITOR_CLARIFY_48();
}

inline Image ICON_NAVIGATION_CLOSE_SMALL_48()
{
    return UiIconsImg::ICON_NAVIGATION_CLOSE_SMALL_48();
}

inline Image ICON_NAVIGATION_EXIT_TO_APP_48()
{
    return UiIconsImg::ICON_NAVIGATION_EXIT_TO_APP_48();
}

inline Image ICON_COMMUNICATION_COMMENT_48()
{
    return UiIconsImg::ICON_COMMUNICATION_COMMENT_48();
}

inline Image ICON_EDITOR_FORMAT_ALIGN_CENTER_48()
{
    return UiIconsImg::ICON_EDITOR_FORMAT_ALIGN_CENTER_48();
}

inline Image ICON_EDITOR_FORMAT_ALIGN_JUSTIFY_48()
{
    return UiIconsImg::ICON_EDITOR_FORMAT_ALIGN_JUSTIFY_48();
}

inline Image ICON_EDITOR_FORMAT_ALIGN_LEFT_48()
{
    return UiIconsImg::ICON_EDITOR_FORMAT_ALIGN_LEFT_48();
}

inline Image ICON_EDITOR_FORMAT_ALIGN_RIGHT_48()
{
    return UiIconsImg::ICON_EDITOR_FORMAT_ALIGN_RIGHT_48();
}

inline Image ICON_EDITOR_FORMAT_BOLD_48()
{
    return UiIconsImg::ICON_EDITOR_FORMAT_BOLD_48();
}

inline Image ICON_EDITOR_FORMAT_H1_48()
{
    return UiIconsImg::ICON_EDITOR_FORMAT_H1_48();
}

inline Image ICON_EDITOR_FORMAT_H2_48()
{
    return UiIconsImg::ICON_EDITOR_FORMAT_H2_48();
}

inline Image ICON_EDITOR_FORMAT_H3_48()
{
    return UiIconsImg::ICON_EDITOR_FORMAT_H3_48();
}

inline Image ICON_EDITOR_FORMAT_INDENT_DECREASE_48()
{
    return UiIconsImg::ICON_EDITOR_FORMAT_INDENT_DECREASE_48();
}

inline Image ICON_EDITOR_FORMAT_INDENT_INCREASE_48()
{
    return UiIconsImg::ICON_EDITOR_FORMAT_INDENT_INCREASE_48();
}

inline Image ICON_EDITOR_FORMAT_ITALIC_48()
{
    return UiIconsImg::ICON_EDITOR_FORMAT_ITALIC_48();
}

inline Image ICON_EDITOR_FORMAT_LETTER_SPACING_STANDARD_48()
{
    return UiIconsImg::ICON_EDITOR_FORMAT_LETTER_SPACING_STANDARD_48();
}

inline Image ICON_EDITOR_FORMAT_LINE_SPACING_48()
{
    return UiIconsImg::ICON_EDITOR_FORMAT_LINE_SPACING_48();
}

inline Image ICON_EDITOR_FORMAT_LIST_BULLETED_48()
{
    return UiIconsImg::ICON_EDITOR_FORMAT_LIST_BULLETED_48();
}

inline Image ICON_EDITOR_FORMAT_LIST_NUMBERED_RTL_48()
{
    return UiIconsImg::ICON_EDITOR_FORMAT_LIST_NUMBERED_RTL_48();
}

inline Image ICON_EDITOR_FORMAT_QUOTE_48()
{
    return UiIconsImg::ICON_EDITOR_FORMAT_QUOTE_48();
}

inline Image ICON_EDITOR_FORMAT_SIZE_48()
{
    return UiIconsImg::ICON_EDITOR_FORMAT_SIZE_48();
}

inline Image ICON_EDITOR_FORMAT_STRIKETHROUGH_48()
{
    return UiIconsImg::ICON_EDITOR_FORMAT_STRIKETHROUGH_48();
}

inline Image ICON_EDITOR_MARGIN_48()
{
    return UiIconsImg::ICON_EDITOR_MARGIN_48();
}

inline Image ICON_EDITOR_MODE_OFF_ON_48()
{
    return UiIconsImg::ICON_EDITOR_MODE_OFF_ON_48();
}

inline Image ICON_EDITOR_NOTES_48()
{
    return UiIconsImg::ICON_EDITOR_NOTES_48();
}

inline Image ICON_ACTION_SEARCH_48()
{
    return UiIconsImg::ICON_ACTION_SEARCH_48();
}

inline Image ICON_EDITOR_SERIF_48()
{
    return UiIconsImg::ICON_EDITOR_SERIF_48();
}

inline Image ICON_EDITOR_TABLE_48()
{
    return UiIconsImg::ICON_EDITOR_TABLE_48();
}

inline Image ICON_EDITOR_TITLECASE_48()
{
    return UiIconsImg::ICON_EDITOR_TITLECASE_48();
}

inline Image ICON_CONTENT_CONTENT_COPY_48()
{
    return UiIconsImg::ICON_CONTENT_CONTENT_COPY_48();
}

inline Image ICON_ACTION_DARK_MODE_48()
{
    return UiIconsImg::ICON_ACTION_DARK_MODE_48();
}

inline Image ICON_ACTION_LIGHT_MODE_48()
{
    return UiIconsImg::ICON_ACTION_LIGHT_MODE_48();
}

inline Image ICON_DESIGN_ADJUST_48()
{
    return UiIconsImg::ICON_DESIGN_ADJUST_48();
}

inline Image ICON_TOGGLE_CHECK_BOX_48()
{
    return UiIconsImg::ICON_TOGGLE_CHECK_BOX_48();
}

inline Image ICON_TOGGLE_CHECK_BOX_OUTLINE_BLANK_48()
{
    return UiIconsImg::ICON_TOGGLE_CHECK_BOX_OUTLINE_BLANK_48();
}

inline Image ICON_DESIGN_CIRCLE_48()
{
    return UiIconsImg::ICON_DESIGN_CIRCLE_48();
}

inline Image ICON_DESIGN_CIRCLE_CIRCLE_48()
{
    return UiIconsImg::ICON_DESIGN_CIRCLE_CIRCLE_48();
}

inline Image ICON_DESIGN_FIBER_MANUAL_RECORD_48()
{
    return UiIconsImg::ICON_DESIGN_FIBER_MANUAL_RECORD_48();
}

inline Image ICON_TOGGLE_RADIO_BUTTON_CHECKED_48()
{
    return UiIconsImg::ICON_TOGGLE_RADIO_BUTTON_CHECKED_48();
}

inline Image ICON_TOGGLE_RADIO_BUTTON_UNCHECKED_48()
{
    return UiIconsImg::ICON_TOGGLE_RADIO_BUTTON_UNCHECKED_48();
}

inline Image ICON_BRAND_NEWLOGO_V5_48()
{
    return UiIconsImg::ICON_BRAND_NEWLOGO_V5_48();
}

inline Image ICON_DESIGN_ACCOUNT_TREE_48()
{
    return UiIconsImg::ICON_DESIGN_ACCOUNT_TREE_48();
}

inline Image ICON_DESIGN_BORDER_HORIZONTAL_48()
{
    return UiIconsImg::ICON_DESIGN_BORDER_HORIZONTAL_48();
}

inline Image ICON_DESIGN_BORDER_INNER_48()
{
    return UiIconsImg::ICON_DESIGN_BORDER_INNER_48();
}

inline Image ICON_DESIGN_BOTTOM_PANEL_CLOSE_48()
{
    return UiIconsImg::ICON_DESIGN_BOTTOM_PANEL_CLOSE_48();
}

inline Image ICON_DESIGN_BOTTOM_PANEL_OPEN_48()
{
    return UiIconsImg::ICON_DESIGN_BOTTOM_PANEL_OPEN_48();
}

inline Image ICON_DESIGN_BOTTOM_SHEETS_48()
{
    return UiIconsImg::ICON_DESIGN_BOTTOM_SHEETS_48();
}

inline Image ICON_DESIGN_ID_CARD_48()
{
    return UiIconsImg::ICON_DESIGN_ID_CARD_48();
}

inline Image ICON_DESIGN_LABEL_48()
{
    return UiIconsImg::ICON_DESIGN_LABEL_48();
}

inline Image ICON_DESIGN_LIST_ALT_48()
{
    return UiIconsImg::ICON_DESIGN_LIST_ALT_48();
}

inline Image ICON_DESIGN_SPACE_DASHBOARD_48()
{
    return UiIconsImg::ICON_DESIGN_SPACE_DASHBOARD_48();
}

inline Image ICON_DESIGN_TOGGLE_ON_48()
{
    return UiIconsImg::ICON_DESIGN_TOGGLE_ON_48();
}

inline Image ICON_DESIGN_TUNE_48()
{
    return UiIconsImg::ICON_DESIGN_TUNE_48();
}

inline Image ICON_DESIGN_BOX_LAYOUT_48()
{
    return UiIconsImg::ICON_DESIGN_BOX_LAYOUT_48();
}

inline Image ICON_DESIGN_GRID_LAYOUT_48()
{
    return UiIconsImg::ICON_DESIGN_GRID_LAYOUT_48();
}

inline Image ICON_DESIGN_PANEL_48()
{
    return UiIconsImg::ICON_DESIGN_PANEL_48();
}

inline Image ICON_DESIGN_SCROLL_PANEL_48()
{
    return UiIconsImg::ICON_DESIGN_SCROLL_PANEL_48();
}

inline Image ICON_DESIGN_TABLE_48()
{
    return UiIconsImg::ICON_DESIGN_TABLE_48();
}

inline Image ICON_DESIGN_TAB_48()
{
    return UiIconsImg::ICON_DESIGN_TAB_48();
}

inline Image ICON_DESIGN_TREE_48()
{
    return UiIconsImg::ICON_DESIGN_TREE_48();
}

inline Image ICON_DESIGN_TOGGLE_COMPOSITE_48()
{
    return UiIconsImg::ICON_DESIGN_TOGGLE_COMPOSITE_48();
}

inline Image ICON_DESIGN_EDIT_TEXT_48()
{
    return UiIconsImg::ICON_DESIGN_EDIT_TEXT_48();
}

inline Image ICON_DESIGN_EDIT_INT_48()
{
    return UiIconsImg::ICON_DESIGN_EDIT_INT_48();
}

inline Image ICON_DESIGN_EDIT_FLOAT_48()
{
    return UiIconsImg::ICON_DESIGN_EDIT_FLOAT_48();
}

inline Image ICON_DESIGN_STACK_48()
{
    return UiIconsImg::ICON_DESIGN_STACK_48();
}

inline Image ICON_DESIGN_BUTTON_48()
{
    return UiIconsImg::ICON_DESIGN_BUTTON_48();
}

inline Image ICON_DESIGN_BREADCRUMBS_48()
{
    return UiIconsImg::ICON_DESIGN_BREADCRUMBS_48();
}

inline Image ICON_DESIGN_ARROWS_OUTPUT_48()
{
    return UiIconsImg::DESIGN_ARROWS_OUTPUT_42_48();
}

inline Image ICON_DESIGN_ASPECT_RATIO_48()
{
    return UiIconsImg::DESIGN_ASPECT_RATIO_42_48();
}

inline Image ICON_DESIGN_DASHBOARD_EDIT_48()
{
    return UiIconsImg::DESIGN_DASHBOARD_2_EDIT_42_48();
}

inline Image ICON_DESIGN_DASHBOARD_CUSTOMIZE_48()
{
    return UiIconsImg::DESIGN_DASHBOARD_CUSTOMIZE_42_48();
}

inline Image ICON_DESIGN_DYNAMIC_FORM_48()
{
    return UiIconsImg::DESIGN_DYNAMIC_FORM_42_48();
}

inline Image ICON_DESIGN_EXPANSION_PANELS_48()
{
    return UiIconsImg::DESIGN_EXPANSION_PANELS_42_48();
}

inline Image ICON_DESIGN_FIT_PAGE_48()
{
    return UiIconsImg::DESIGN_FIT_PAGE_42_48();
}

inline Image ICON_DESIGN_FIT_WIDTH_48()
{
    return UiIconsImg::DESIGN_FIT_WIDTH_42_48();
}

inline Image ICON_DESIGN_GRID_4X4_48()
{
    return UiIconsImg::DESIGN_GRID_4X4_42_48();
}

inline Image ICON_DESIGN_HORIZONTAL_DISTRIBUTE_48()
{
    return UiIconsImg::DESIGN_HORIZONTAL_DISTRIBUTE_42_48();
}

inline Image ICON_DESIGN_LAYOUTS_CATEGORY_48()
{
    return UiIconsImg::DESIGN_LAYOUTS_CATEGORY_48();
}

inline Image ICON_DESIGN_SLIDERS_48()
{
    return UiIconsImg::DESIGN_SLIDERS_42_48();
}

inline Image ICON_DESIGN_TAB_GROUP_48()
{
    return UiIconsImg::DESIGN_TAB_GROUP_42_48();
}

inline Image ICON_DESIGN_VERTICAL_DISTRIBUTE_48()
{
    return UiIconsImg::DESIGN_VERTICAL_DISTRIBUTE_42_48();
}

inline Image ICON_DESIGN_WIDGETS_48()
{
    return UiIconsImg::DESIGN_WIDGETS_42_48();
}

inline Image ICON_DESIGN_BORDER_ALL_48()
{
    return UiIconsImg::DESIGN_BORDER_ALL_32_48();
}

inline Image ICON_DESIGN_BORDER_OUTER_48()
{
    return UiIconsImg::DESIGN_BORDER_OUTER_32_48();
}

inline Image ICON_DESIGN_BORDER_CLEAR_48()
{
    return UiIconsImg::DESIGN_BORDER_CLEAR_32_48();
}

inline Image ICON_DESIGN_SQUARE_DOT_48()
{
    return UiIconsImg::DESIGN_SQUARE_DOT_32_48();
}


// Source: out\icon_import_tmp\arrow_circle_left.svg
// Size: 48x48
inline Image ICON_DESIGN_ARROW_CIRCLE_LEFT_48()
{
    return UiIconsImg::ICON_DESIGN_ARROW_CIRCLE_LEFT_48();
}

// Source: out\icon_import_tmp\comment.svg
// Size: 48x48
inline Image ICON_DESIGN_COMMENT_48()
{
    return UiIconsImg::ICON_DESIGN_COMMENT_48();
}

// Source: out\icon_import_tmp\description.svg
// Size: 48x48
inline Image ICON_DESIGN_DESCRIPTION_48()
{
    return UiIconsImg::ICON_DESIGN_DESCRIPTION_48();
}

// Source: out\icon_import_tmp\find_replace.svg
// Size: 48x48
inline Image ICON_DESIGN_FIND_REPLACE_48()
{
    return UiIconsImg::ICON_DESIGN_FIND_REPLACE_48();
}

// Source: out\icon_import_tmp\full_stacked_bar_chart.svg
// Size: 48x48
inline Image ICON_DESIGN_FULL_STACKED_BAR_CHART_48()
{
    return UiIconsImg::ICON_DESIGN_FULL_STACKED_BAR_CHART_48();
}

// Source: out\icon_import_tmp\remove_selection.svg
// Size: 48x48
inline Image ICON_DESIGN_REMOVE_SELECTION_48()
{
    return UiIconsImg::ICON_DESIGN_REMOVE_SELECTION_48();
}

// Source: out\icon_import_tmp\text_decrease.svg
// Size: 48x48
inline Image ICON_DESIGN_TEXT_DECREASE_48()
{
    return UiIconsImg::ICON_DESIGN_TEXT_DECREASE_48();
}

// Source: out\icon_import_tmp\toolbar.svg
// Size: 48x48
inline Image ICON_DESIGN_TOOLBAR_48()
{
    return UiIconsImg::ICON_DESIGN_TOOLBAR_48();
}

// Source: out\icon_import_tmp\trip_origin.svg
// Size: 48x48
inline Image ICON_DESIGN_TRIP_ORIGIN_48()
{
    return UiIconsImg::ICON_DESIGN_TRIP_ORIGIN_48();
}

// Source: out\icon_import_tmp\youtube_searched_for.svg
// Size: 48x48
inline Image ICON_DESIGN_YOUTUBE_SEARCHED_FOR_48()
{
    return UiIconsImg::ICON_DESIGN_YOUTUBE_SEARCHED_FOR_48();
}

// Source: designs\custom_typography_32.svg
// Size: 48x48
inline Image ICON_DESIGN_CUSTOM_TYPOGRAPHY_48()
{
    return UiIconsImg::ICON_DESIGN_CUSTOM_TYPOGRAPHY_48();
}

// Source: designs\help_48.svg
// Size: 48x48
inline Image ICON_DESIGN_HELP_48()
{
    return UiIconsImg::ICON_DESIGN_HELP_48();
}

// Source: designs\info_48.svg
// Size: 48x48
inline Image ICON_DESIGN_INFO_48()
{
    return UiIconsImg::ICON_DESIGN_INFO_48();
}

// Source: designs\format_paint_48.svg
// Size: 48x48
inline Image ICON_DESIGN_FORMAT_PAINT_48()
{
    return UiIconsImg::ICON_DESIGN_FORMAT_PAINT_48();
}

// Source: designs\code_blocks_48.svg
// Size: 48x48
inline Image ICON_DESIGN_CODE_BLOCKS_48()
{
    return UiIconsImg::ICON_DESIGN_CODE_BLOCKS_48();
}

// Source: designs\desktop_mac_48.svg
// Size: 48x48
inline Image ICON_DESIGN_DESKTOP_MAC_48()
{
    return UiIconsImg::ICON_DESIGN_DESKTOP_MAC_48();
}

// Source: designs\splitscreen_landscape_48.svg
// Size: 48x48
inline Image ICON_DESIGN_SPLITSCREEN_LANDSCAPE_48()
{
    return UiIconsImg::ICON_DESIGN_SPLITSCREEN_LANDSCAPE_48();
}

// Source: designs\splitscreen_portrait_48.svg
// Size: 48x48
inline Image ICON_DESIGN_SPLITSCREEN_PORTRAIT_48()
{
    return UiIconsImg::ICON_DESIGN_SPLITSCREEN_PORTRAIT_48();
}

inline const Vector<UiIconCatalogEntry>& UiIconCatalog()
{
    static const Vector<UiIconCatalogEntry> catalog = [] {
        Vector<UiIconCatalogEntry> out;
        out.Reserve(120);
        out.Add(UiIconCatalogEntry("ICON_NAVIGATION_OUTLINED_MORE_HORIZ_48", &ICON_NAVIGATION_OUTLINED_MORE_HORIZ_48));
        out.Add(UiIconCatalogEntry("ICON_NAVIGATION_OUTLINED_MENU_48", &ICON_NAVIGATION_OUTLINED_MENU_48));
        out.Add(UiIconCatalogEntry("ICON_NAVIGATION_OUTLINED_MORE_VERT_48", &ICON_NAVIGATION_OUTLINED_MORE_VERT_48));
        out.Add(UiIconCatalogEntry("ICON_NAVIGATION_OUTLINED_ARROW_RIGHT_48", &ICON_NAVIGATION_OUTLINED_ARROW_RIGHT_48));
        out.Add(UiIconCatalogEntry("ICON_NAVIGATION_OUTLINED_ARROW_LEFT_48", &ICON_NAVIGATION_OUTLINED_ARROW_LEFT_48));
        out.Add(UiIconCatalogEntry("ICON_NAVIGATION_OUTLINED_ARROW_DROP_UP_48", &ICON_NAVIGATION_OUTLINED_ARROW_DROP_UP_48));
        out.Add(UiIconCatalogEntry("ICON_NAVIGATION_OUTLINED_ARROW_DROP_DOWN_48", &ICON_NAVIGATION_OUTLINED_ARROW_DROP_DOWN_48));
        out.Add(UiIconCatalogEntry("ICON_NAVIGATION_OUTLINED_APPS_48", &ICON_NAVIGATION_OUTLINED_APPS_48));
        out.Add(UiIconCatalogEntry("ICON_HARDWARE_OUTLINED_KEYBOARD_ARROW_RIGHT_48", &ICON_HARDWARE_OUTLINED_KEYBOARD_ARROW_RIGHT_48));
        out.Add(UiIconCatalogEntry("ICON_HARDWARE_OUTLINED_KEYBOARD_ARROW_LEFT_48", &ICON_HARDWARE_OUTLINED_KEYBOARD_ARROW_LEFT_48));
        out.Add(UiIconCatalogEntry("ICON_CONTENT_OUTLINED_ADD_CIRCLE_OUTLINE_48", &ICON_CONTENT_OUTLINED_ADD_CIRCLE_OUTLINE_48));
        out.Add(UiIconCatalogEntry("ICON_CONTENT_OUTLINED_REMOVE_CIRCLE_OUTLINE_48", &ICON_CONTENT_OUTLINED_REMOVE_CIRCLE_OUTLINE_48));
        out.Add(UiIconCatalogEntry("ICON_CONTENT_OUTLINED_ADD_48", &ICON_CONTENT_OUTLINED_ADD_48));
        out.Add(UiIconCatalogEntry("ICON_CONTENT_OUTLINED_REMOVE_48", &ICON_CONTENT_OUTLINED_REMOVE_48));
        out.Add(UiIconCatalogEntry("ICON_ACTION_OUTLINED_VISIBILITY_48", &ICON_ACTION_OUTLINED_VISIBILITY_48));
        out.Add(UiIconCatalogEntry("ICON_ACTION_OUTLINED_VISIBILITY_OFF_48", &ICON_ACTION_OUTLINED_VISIBILITY_OFF_48));
        out.Add(UiIconCatalogEntry("ICON_NAVIGATION_OUTLINED_DRAG_INDICATOR_48", &ICON_NAVIGATION_OUTLINED_DRAG_INDICATOR_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_CHECK_SMALL_48", &ICON_DESIGN_CHECK_SMALL_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_DELETE_48", &ICON_DESIGN_DELETE_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_DRAG_INDICATOR_48", &ICON_DESIGN_DRAG_INDICATOR_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_FOLDER_48", &ICON_DESIGN_FOLDER_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_HOME_48", &ICON_DESIGN_HOME_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_IMAGE_48", &ICON_DESIGN_IMAGE_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_LEFT_PANEL_CLOSE_48", &ICON_DESIGN_LEFT_PANEL_CLOSE_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_LEFT_PANEL_OPEN_48", &ICON_DESIGN_LEFT_PANEL_OPEN_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_LOWERCASE_48", &ICON_DESIGN_LOWERCASE_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_MENU_48", &ICON_DESIGN_MENU_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_RIGHT_PANEL_CLOSE_48", &ICON_DESIGN_RIGHT_PANEL_CLOSE_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_RIGHT_PANEL_OPEN_48", &ICON_DESIGN_RIGHT_PANEL_OPEN_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_SETTINGS_48", &ICON_DESIGN_SETTINGS_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_UNFOLD_LESS_48", &ICON_DESIGN_UNFOLD_LESS_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_UNFOLD_MORE_48", &ICON_DESIGN_UNFOLD_MORE_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_UPPERCASE_48", &ICON_DESIGN_UPPERCASE_48));
        out.Add(UiIconCatalogEntry("ICON_EDITOR_BORDER_LEFT_48", &ICON_EDITOR_BORDER_LEFT_48));
        out.Add(UiIconCatalogEntry("ICON_EDITOR_BORDER_RIGHT_48", &ICON_EDITOR_BORDER_RIGHT_48));
        out.Add(UiIconCatalogEntry("ICON_ACTION_CANCEL_48", &ICON_ACTION_CANCEL_48));
        out.Add(UiIconCatalogEntry("ICON_ACTION_CHECK_CIRCLE_48", &ICON_ACTION_CHECK_CIRCLE_48));
        out.Add(UiIconCatalogEntry("ICON_EDITOR_CLARIFY_48", &ICON_EDITOR_CLARIFY_48));
        out.Add(UiIconCatalogEntry("ICON_NAVIGATION_CLOSE_SMALL_48", &ICON_NAVIGATION_CLOSE_SMALL_48));
        out.Add(UiIconCatalogEntry("ICON_NAVIGATION_EXIT_TO_APP_48", &ICON_NAVIGATION_EXIT_TO_APP_48));
        out.Add(UiIconCatalogEntry("ICON_COMMUNICATION_COMMENT_48", &ICON_COMMUNICATION_COMMENT_48));
        out.Add(UiIconCatalogEntry("ICON_EDITOR_FORMAT_ALIGN_CENTER_48", &ICON_EDITOR_FORMAT_ALIGN_CENTER_48));
        out.Add(UiIconCatalogEntry("ICON_EDITOR_FORMAT_ALIGN_JUSTIFY_48", &ICON_EDITOR_FORMAT_ALIGN_JUSTIFY_48));
        out.Add(UiIconCatalogEntry("ICON_EDITOR_FORMAT_ALIGN_LEFT_48", &ICON_EDITOR_FORMAT_ALIGN_LEFT_48));
        out.Add(UiIconCatalogEntry("ICON_EDITOR_FORMAT_ALIGN_RIGHT_48", &ICON_EDITOR_FORMAT_ALIGN_RIGHT_48));
        out.Add(UiIconCatalogEntry("ICON_EDITOR_FORMAT_BOLD_48", &ICON_EDITOR_FORMAT_BOLD_48));
        out.Add(UiIconCatalogEntry("ICON_EDITOR_FORMAT_H1_48", &ICON_EDITOR_FORMAT_H1_48));
        out.Add(UiIconCatalogEntry("ICON_EDITOR_FORMAT_H2_48", &ICON_EDITOR_FORMAT_H2_48));
        out.Add(UiIconCatalogEntry("ICON_EDITOR_FORMAT_H3_48", &ICON_EDITOR_FORMAT_H3_48));
        out.Add(UiIconCatalogEntry("ICON_EDITOR_FORMAT_INDENT_DECREASE_48", &ICON_EDITOR_FORMAT_INDENT_DECREASE_48));
        out.Add(UiIconCatalogEntry("ICON_EDITOR_FORMAT_INDENT_INCREASE_48", &ICON_EDITOR_FORMAT_INDENT_INCREASE_48));
        out.Add(UiIconCatalogEntry("ICON_EDITOR_FORMAT_ITALIC_48", &ICON_EDITOR_FORMAT_ITALIC_48));
        out.Add(UiIconCatalogEntry("ICON_EDITOR_FORMAT_LETTER_SPACING_STANDARD_48", &ICON_EDITOR_FORMAT_LETTER_SPACING_STANDARD_48));
        out.Add(UiIconCatalogEntry("ICON_EDITOR_FORMAT_LINE_SPACING_48", &ICON_EDITOR_FORMAT_LINE_SPACING_48));
        out.Add(UiIconCatalogEntry("ICON_EDITOR_FORMAT_LIST_BULLETED_48", &ICON_EDITOR_FORMAT_LIST_BULLETED_48));
        out.Add(UiIconCatalogEntry("ICON_EDITOR_FORMAT_LIST_NUMBERED_RTL_48", &ICON_EDITOR_FORMAT_LIST_NUMBERED_RTL_48));
        out.Add(UiIconCatalogEntry("ICON_EDITOR_FORMAT_QUOTE_48", &ICON_EDITOR_FORMAT_QUOTE_48));
        out.Add(UiIconCatalogEntry("ICON_EDITOR_FORMAT_SIZE_48", &ICON_EDITOR_FORMAT_SIZE_48));
        out.Add(UiIconCatalogEntry("ICON_EDITOR_FORMAT_STRIKETHROUGH_48", &ICON_EDITOR_FORMAT_STRIKETHROUGH_48));
        out.Add(UiIconCatalogEntry("ICON_EDITOR_MARGIN_48", &ICON_EDITOR_MARGIN_48));
        out.Add(UiIconCatalogEntry("ICON_EDITOR_MODE_OFF_ON_48", &ICON_EDITOR_MODE_OFF_ON_48));
        out.Add(UiIconCatalogEntry("ICON_EDITOR_NOTES_48", &ICON_EDITOR_NOTES_48));
        out.Add(UiIconCatalogEntry("ICON_ACTION_SEARCH_48", &ICON_ACTION_SEARCH_48));
        out.Add(UiIconCatalogEntry("ICON_EDITOR_SERIF_48", &ICON_EDITOR_SERIF_48));
        out.Add(UiIconCatalogEntry("ICON_EDITOR_TABLE_48", &ICON_EDITOR_TABLE_48));
        out.Add(UiIconCatalogEntry("ICON_EDITOR_TITLECASE_48", &ICON_EDITOR_TITLECASE_48));
        out.Add(UiIconCatalogEntry("ICON_CONTENT_CONTENT_COPY_48", &ICON_CONTENT_CONTENT_COPY_48));
        out.Add(UiIconCatalogEntry("ICON_ACTION_DARK_MODE_48", &ICON_ACTION_DARK_MODE_48));
        out.Add(UiIconCatalogEntry("ICON_ACTION_LIGHT_MODE_48", &ICON_ACTION_LIGHT_MODE_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_ADJUST_48", &ICON_DESIGN_ADJUST_48));
        out.Add(UiIconCatalogEntry("ICON_TOGGLE_CHECK_BOX_48", &ICON_TOGGLE_CHECK_BOX_48));
        out.Add(UiIconCatalogEntry("ICON_TOGGLE_CHECK_BOX_OUTLINE_BLANK_48", &ICON_TOGGLE_CHECK_BOX_OUTLINE_BLANK_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_CIRCLE_48", &ICON_DESIGN_CIRCLE_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_CIRCLE_CIRCLE_48", &ICON_DESIGN_CIRCLE_CIRCLE_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_FIBER_MANUAL_RECORD_48", &ICON_DESIGN_FIBER_MANUAL_RECORD_48));
        out.Add(UiIconCatalogEntry("ICON_TOGGLE_RADIO_BUTTON_CHECKED_48", &ICON_TOGGLE_RADIO_BUTTON_CHECKED_48));
        out.Add(UiIconCatalogEntry("ICON_TOGGLE_RADIO_BUTTON_UNCHECKED_48", &ICON_TOGGLE_RADIO_BUTTON_UNCHECKED_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_ACCOUNT_TREE_48", &ICON_DESIGN_ACCOUNT_TREE_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_BORDER_HORIZONTAL_48", &ICON_DESIGN_BORDER_HORIZONTAL_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_BORDER_INNER_48", &ICON_DESIGN_BORDER_INNER_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_BOTTOM_PANEL_CLOSE_48", &ICON_DESIGN_BOTTOM_PANEL_CLOSE_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_BOTTOM_PANEL_OPEN_48", &ICON_DESIGN_BOTTOM_PANEL_OPEN_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_BOTTOM_SHEETS_48", &ICON_DESIGN_BOTTOM_SHEETS_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_ID_CARD_48", &ICON_DESIGN_ID_CARD_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_LABEL_48", &ICON_DESIGN_LABEL_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_LIST_ALT_48", &ICON_DESIGN_LIST_ALT_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_SPACE_DASHBOARD_48", &ICON_DESIGN_SPACE_DASHBOARD_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_TOGGLE_ON_48", &ICON_DESIGN_TOGGLE_ON_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_TUNE_48", &ICON_DESIGN_TUNE_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_BOX_LAYOUT_48", &ICON_DESIGN_BOX_LAYOUT_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_GRID_LAYOUT_48", &ICON_DESIGN_GRID_LAYOUT_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_PANEL_48", &ICON_DESIGN_PANEL_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_SCROLL_PANEL_48", &ICON_DESIGN_SCROLL_PANEL_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_TABLE_48", &ICON_DESIGN_TABLE_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_TAB_48", &ICON_DESIGN_TAB_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_TREE_48", &ICON_DESIGN_TREE_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_TOGGLE_COMPOSITE_48", &ICON_DESIGN_TOGGLE_COMPOSITE_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_EDIT_TEXT_48", &ICON_DESIGN_EDIT_TEXT_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_EDIT_INT_48", &ICON_DESIGN_EDIT_INT_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_EDIT_FLOAT_48", &ICON_DESIGN_EDIT_FLOAT_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_STACK_48", &ICON_DESIGN_STACK_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_BUTTON_48", &ICON_DESIGN_BUTTON_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_BREADCRUMBS_48", &ICON_DESIGN_BREADCRUMBS_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_ARROWS_OUTPUT_48", &ICON_DESIGN_ARROWS_OUTPUT_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_ASPECT_RATIO_48", &ICON_DESIGN_ASPECT_RATIO_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_DASHBOARD_EDIT_48", &ICON_DESIGN_DASHBOARD_EDIT_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_DASHBOARD_CUSTOMIZE_48", &ICON_DESIGN_DASHBOARD_CUSTOMIZE_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_DYNAMIC_FORM_48", &ICON_DESIGN_DYNAMIC_FORM_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_EXPANSION_PANELS_48", &ICON_DESIGN_EXPANSION_PANELS_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_FIT_PAGE_48", &ICON_DESIGN_FIT_PAGE_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_FIT_WIDTH_48", &ICON_DESIGN_FIT_WIDTH_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_GRID_4X4_48", &ICON_DESIGN_GRID_4X4_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_HORIZONTAL_DISTRIBUTE_48", &ICON_DESIGN_HORIZONTAL_DISTRIBUTE_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_LAYOUTS_CATEGORY_48", &ICON_DESIGN_LAYOUTS_CATEGORY_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_SLIDERS_48", &ICON_DESIGN_SLIDERS_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_TAB_GROUP_48", &ICON_DESIGN_TAB_GROUP_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_VERTICAL_DISTRIBUTE_48", &ICON_DESIGN_VERTICAL_DISTRIBUTE_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_WIDGETS_48", &ICON_DESIGN_WIDGETS_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_BORDER_ALL_48", &ICON_DESIGN_BORDER_ALL_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_BORDER_OUTER_48", &ICON_DESIGN_BORDER_OUTER_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_BORDER_CLEAR_48", &ICON_DESIGN_BORDER_CLEAR_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_SQUARE_DOT_48", &ICON_DESIGN_SQUARE_DOT_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_ARROW_CIRCLE_LEFT_48", &ICON_DESIGN_ARROW_CIRCLE_LEFT_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_COMMENT_48", &ICON_DESIGN_COMMENT_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_DESCRIPTION_48", &ICON_DESIGN_DESCRIPTION_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_FIND_REPLACE_48", &ICON_DESIGN_FIND_REPLACE_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_FULL_STACKED_BAR_CHART_48", &ICON_DESIGN_FULL_STACKED_BAR_CHART_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_REMOVE_SELECTION_48", &ICON_DESIGN_REMOVE_SELECTION_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_TEXT_DECREASE_48", &ICON_DESIGN_TEXT_DECREASE_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_TOOLBAR_48", &ICON_DESIGN_TOOLBAR_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_TRIP_ORIGIN_48", &ICON_DESIGN_TRIP_ORIGIN_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_YOUTUBE_SEARCHED_FOR_48", &ICON_DESIGN_YOUTUBE_SEARCHED_FOR_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_FORMAT_PAINT_48", &ICON_DESIGN_FORMAT_PAINT_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_CODE_BLOCKS_48", &ICON_DESIGN_CODE_BLOCKS_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_DESKTOP_MAC_48", &ICON_DESIGN_DESKTOP_MAC_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_SPLITSCREEN_LANDSCAPE_48", &ICON_DESIGN_SPLITSCREEN_LANDSCAPE_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_SPLITSCREEN_PORTRAIT_48", &ICON_DESIGN_SPLITSCREEN_PORTRAIT_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_CUSTOM_TYPOGRAPHY_48", &ICON_DESIGN_CUSTOM_TYPOGRAPHY_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_HELP_48", &ICON_DESIGN_HELP_48));
        out.Add(UiIconCatalogEntry("ICON_DESIGN_INFO_48", &ICON_DESIGN_INFO_48));

        out.Add(UiIconCatalogEntry("ICON_BRAND_NEWLOGO_V5_48", &ICON_BRAND_NEWLOGO_V5_48));
        return out;
    }();
    return catalog;
}

inline Vector<String> UiIconNameList()
{
    Vector<String> names;
    const Vector<UiIconCatalogEntry>& catalog = UiIconCatalog();
    names.Reserve(catalog.GetCount());
    for(int i = 0; i < catalog.GetCount(); i++)
        names.Add(catalog[i].name);
    return names;
}

inline Vector<String> UiIconDisplayNameList()
{
    Vector<String> names;
    const Vector<UiIconCatalogEntry>& catalog = UiIconCatalog();
    names.Reserve(catalog.GetCount());
    for(int i = 0; i < catalog.GetCount(); i++)
        names.Add(catalog[i].display_name);
    return names;
}

inline UiListModel UiIconListModel(bool use_icon_name_as_data = true)
{
    UiListModel model;
    const Vector<UiIconCatalogEntry>& catalog = UiIconCatalog();
    model.Reserve(catalog.GetCount());
    for(int i = 0; i < catalog.GetCount(); i++) {
        UiModelItem it;
        it.text = catalog[i].display_name;
        if(use_icon_name_as_data)
            it.data = catalog[i].name;
        it.icon = catalog[i].factory ? catalog[i].factory() : Image();
        it.icon_render_mode = UiIconRenderMode::MonoTint;
        model.Add(it);
    }
    return model;
}

inline Image UiIconFromName(const String& icon_name)
{
    const Vector<UiIconCatalogEntry>& catalog = UiIconCatalog();
    for(int i = 0; i < catalog.GetCount(); i++)
        if(icon_name == catalog[i].name)
            return catalog[i].factory ? catalog[i].factory() : Image();
    return Image();
}

}

#endif


