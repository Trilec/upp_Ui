# Designer Shell Mockup Notes

Reference:
- `E:\apps\github\upp_Ui\designs\designer_new_mockup.cpp`

This note captures the shell direction from the generated mockup so the live
Designer can be updated without copying the generated source directly.

## 0. Adopted Into The Live Shell

The following mockup elements were carried into `Utilities/Designer/main.cpp`:
- cleaner top header row with title card, save/load split buttons, version badge,
  theme dropdown, dark/help tools, and exit action
- left tool strip with category buttons, a scroll-hosted inner info area, and the
  panel toggle at the end of the strip
- center aspect helper strip above the preview surface
- right tool strip with pane controls, expand/contract affordances, and a
  scroll-hosted inner info area
- wider right panel treatment for hierarchy/inspector/code content
- lower status/help row kept as the quiet bottom feedback area
- role-first panel styling with compact mono-tint tool buttons

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

Live shell mapping:
- `header_` is the title card shell
- `save_button_` and `load_button_` are the split buttons
- `version_badge_` uses `ICON_DESIGN_ADJUST_48`
- `theme_preset_row_` is the live theme selector control
- `dark_theme_tool_`, `help_tool_`, and `exit_button_` implement the right-side actions

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
- A dedicated panel toggle affordance belongs at the far left when implemented

Live shell mapping:
- `toolbox_layouts_button_`
- `toolbox_containers_button_`
- `toolbox_controls_button_`
- `toolbox_composites_button_`
- `toolbox_presets_button_`
- `left_panel_toggle_`
- `toolbox_scroll_` and `left_info_box_` form the scroll-hosted info area

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
- The live shell uses a compact fit/portrait/landscape/square helper strip instead
  of copying the mockup verbatim

Live shell mapping:
- `aspect_panel_`
- `aspect_layout_`
- `portrait_aspect_`
- `landscape_aspect_`
- `square_aspect_`
- `aspect_preset_`
- `preview_`

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
- The live shell keeps the right panel wider than the left, but does not copy the
  mockup's generated helper blocks directly

Live shell mapping:
- `collapse_button_`
- `right_panel_expand_`
- `right_panel_contract_`
- `hierarchy_mode_button_`
- `inspector_mode_button_`
- `overrides_mode_button_`
- `code_mode_button_`
- `diagnostics_mode_button_`
- `side_` and `right_info_box_` form the scroll-hosted info area

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

Icons used in the live shell implementation:
- `ICON_ACTION_LIGHT_MODE_48`
- `ICON_ACTION_OUTLINED_VISIBILITY_48`
- `ICON_DESIGN_ASPECT_RATIO_48`
- `ICON_DESIGN_DASHBOARD_EDIT_48`
- `ICON_DESIGN_ADJUST_48`
- `ICON_DESIGN_HELP_48`
- `ICON_DESIGN_LAYOUTS_CATEGORY_48`
- `ICON_DESIGN_LEFT_PANEL_CLOSE_48`
- `ICON_DESIGN_LEFT_PANEL_OPEN_48`
- `ICON_DESIGN_RIGHT_PANEL_CLOSE_48`
- `ICON_DESIGN_RIGHT_PANEL_OPEN_48`
- `ICON_DESIGN_SPLITSCREEN_LANDSCAPE_48`
- `ICON_DESIGN_SPLITSCREEN_PORTRAIT_48`
- `ICON_DESIGN_TAB_GROUP_48`
- `ICON_DESIGN_WIDGETS_48`
- `ICON_NAVIGATION_EXIT_TO_APP_48`
- `ICON_TOGGLE_CHECK_BOX_OUTLINE_BLANK_48`

Missing or renamed icons:
- none missing from the current catalog for the adopted shell set
- the mockup's left/right panel toggle labels were kept as shell affordances,
  not hard-coded generated names

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

## 8. Intentionally Not Copied

The following mockup-generated helper/styling blocks were intentionally not copied
into the live shell:
- generated style helper functions or large style factory blocks
- generated mockup layout scaffolding that would bypass the live Designer model
- any direct generated source structure that would mix shell chrome and model logic
- mockup-only placeholder controls that do not exist in the current Designer shell

## 9. Placeholders Left For Later

The live shell still leaves these for a later pass:
- explicit left panel collapse/expand behavior if it is added as a real feature
- any deeper code-view/status-bar expansion beyond the current warning/status row
- further spacing polish once the right-side content pages are tuned in practice
- any icon catalog cleanup that turns up during the next control-module audit

## 10. Current Status

The live shell is now close to the mockup, but it is still a hand-authored shell
rather than generated source:
- the theme selector is implemented with the existing composite dropdown path
- the shell uses live Designer widgets and callbacks instead of mockup-only helpers
- the layout matches the mockup structure closely enough for this pass, but the
  generated reference still remains the source of truth for spacing polish
