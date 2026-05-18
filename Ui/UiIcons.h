#ifndef _Ui_UiIcons_h_
#define _Ui_UiIcons_h_

/*
    UiIcons
    =======

    Purpose
    - Public icon catalog and thin ICON_* wrappers over the shared IML-backed
      Ui icon image class.
*/

#include <CtrlCore/CtrlCore.h>
#include <Ui/UiDataModels.h>
namespace Upp {

#define IMAGECLASS UiIconsImg
#define IMAGEFILE <Ui/UiIcons.iml>
#include <Draw/iml_header.h>

using UiIconFactoryFn = Image (*)();

struct UiIconCatalogEntry : Moveable<UiIconCatalogEntry> {
    String          name;
    UiIconFactoryFn factory = nullptr;

    UiIconCatalogEntry() {}
    UiIconCatalogEntry(const String& n, UiIconFactoryFn f) : name(n), factory(f) {}
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

inline Image ICON_DESIGN_TRIP_ORIGIN_48()
{
    return UiIconsImg::ICON_DESIGN_TRIP_ORIGIN_48();
}

inline Image ICON_BRAND_NEWLOGO_V5_48()
{
    return UiIconsImg::ICON_BRAND_NEWLOGO_V5_48();
}

inline const Vector<UiIconCatalogEntry>& UiIconCatalog()
{
    static const Vector<UiIconCatalogEntry> catalog = [] {
        Vector<UiIconCatalogEntry> out;
        out.Reserve(79);
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
        out.Add(UiIconCatalogEntry("ICON_DESIGN_TRIP_ORIGIN_48", &ICON_DESIGN_TRIP_ORIGIN_48));
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

inline UiListModel UiIconListModel(bool use_icon_name_as_data = true)
{
    UiListModel model;
    const Vector<UiIconCatalogEntry>& catalog = UiIconCatalog();
    model.Reserve(catalog.GetCount());
    for(int i = 0; i < catalog.GetCount(); i++) {
        UiModelItem it;
        it.text = catalog[i].name;
        if(use_icon_name_as_data)
            it.data = catalog[i].name;
        it.icon = catalog[i].factory ? catalog[i].factory() : Image();
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
