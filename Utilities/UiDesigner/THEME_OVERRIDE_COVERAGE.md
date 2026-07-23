# UiDesigner Theme Override Coverage

This document records the current typed Theme Adapter coverage used by the
Designer preview, validation, and code generation paths.

## Completed adapters

- `UiButton`
- `UiToolButton`
- `UiTree`
- `UiList`
- `UiMenu`

## Effective style resolver

Each adapter resolves from the control's current inherited runtime theme and
applies only authored override fields. Absent overrides remain inherited; they
do not fall back to schema defaults.

## Supported field groups

- Button / tool-button paint, frame, shadow, underline, icon and text fields
- Tree layout, glyph, visibility and palette fields
- List layout, visibility, badge and row styling fields
- Menu layout, visibility, popup and palette fields

## Deferred controls

- `UiDropdown`
- `UiBreadcrumbs`
- `UiTitleCard`
- `UiTab`
- `UiStack`
- `UiAccordion`
- other controls without a local typed theme surface

## Controls without local theme surface

Stock U++ controls and structural layout items that do not own a dedicated
runtime style surface remain outside the typed Theme Adapter system for now.

