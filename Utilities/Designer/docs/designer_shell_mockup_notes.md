# Designer Shell Mockup Notes

Reference:
- `E:\apps\github\upp_Ui\designs\designer_new_mockup.cpp`

This note captures the shell direction from the generated mockup so the live
Designer can be updated without copying the generated source directly.

## 1. Top Bar

Observed top bar controls in the mockup:
- Designer title card with brand/logo media
- Save split button
- Load split button
- Version badge
- Theme selector area
- Dark theme tool button
- Help tool button
- Exit tool button

Observed styling direction:
- Compact shell chrome
- Accent role for the header/card emphasis
- Small icon buttons with mono-tint rendering
- Subtle panel surfaces rather than saturated demo blocks

## 2. Left Tool Strip

Mockup left-side controls:
- Layouts
- Containers
- Controls
- Composites
- Presets
- Left panel open/close
- Left expand/contract affordance if implemented later

Observed layout direction:
- A short top strip for category switching
- A scrollable information/tool area beneath the strip
- Lightweight vertical spacing and narrow icon-first controls

## 3. Center Strip

Mockup center helper controls:
- Portrait aspect button
- Landscape aspect button
- Square aspect button
- Aspect preset dropdown / split button

Observed layout direction:
- Helper strip sits above the preview area
- The preview area is the main central surface
- Aspect helpers are visually lightweight and should not dominate the shell

## 4. Right Tool Strip

Mockup right-side controls:
- Right panel open/close first
- Expand / contract affordances
- Hierarchy
- Inspector
- Theme overrides
- Code

Observed layout direction:
- Top strip for shell navigation and pane affordances
- Scrollable information area below the strip
- Wider panel than the left tool strip because the hierarchy needs more room

## 5. Lower Status / Help Area

Mockup lower area direction:
- Quiet status/help text row
- Space reserved for future status controls
- Should stay visually subordinate to the header and preview

## 6. Icon Check

I checked the icons used by the mockup against the current catalog in
`Ui/UiIcons.h`.

No missing icon symbols were found for the mockup references:
- `ICON_ACTION_DARK_MODE_48`
- `ICON_BRAND_NEWLOGO_V5_48`
- `ICON_DESIGN_ACCOUNT_TREE_48`
- `ICON_DESIGN_ADJUST_48`
- `ICON_DESIGN_CODE_BLOCKS_48`
- `ICON_DESIGN_DASHBOARD_EDIT_48`
- `ICON_DESIGN_DYNAMIC_FORM_48`
- `ICON_DESIGN_FORMAT_PAINT_48`
- `ICON_DESIGN_HELP_48`
- `ICON_DESIGN_LAYOUTS_CATEGORY_48`
- `ICON_DESIGN_LEFT_PANEL_CLOSE_48`
- `ICON_DESIGN_MODE_OFF_ON_48`
- `ICON_DESIGN_RIGHT_PANEL_CLOSE_48`
- `ICON_DESIGN_SPLITSCREEN_LANDSCAPE_48`
- `ICON_DESIGN_SPLITSCREEN_PORTRAIT_48`
- `ICON_DESIGN_TAB_GROUP_48`
- `ICON_DESIGN_TUNE_48`
- `ICON_DESIGN_WIDGETS_48`
- `ICON_EDITOR_FORMAT_INDENT_DECREASE_48`
- `ICON_EDITOR_FORMAT_INDENT_INCREASE_48`
- `ICON_TOGGLE_CHECK_BOX_OUTLINE_BLANK_48`

## 7. Cleanup Notes Before Shell Implementation

Suggested naming cleanup for the live Designer shell:
- `open_close_lift_panel` -> `left_panel_toggle`
- `open_close_right_panel` -> `right_panel_toggle`
- `preset_saspect` -> `aspect_preset`
- `left_tool_button_panel` / `right_tool_button_panel` should read as shell strips
  rather than document-content panels

Suggested implementation rule:
- Keep the Designer model and inspector flow unchanged.
- Only reshape the shell chrome and the container hierarchy.
- Use the mockup as the visual guide, not as generated source.
