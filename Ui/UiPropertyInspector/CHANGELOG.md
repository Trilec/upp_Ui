# UiPropertyInspector Visual Changelog

Version: **1.1.0**

## 2026-10 - package cleanup

- moved the visual package to `Ui/UiPropertyInspector` and its headless model to
  `Ui/UiPropertyInspector/Core`, with no duplicate utility implementation;
- renamed public `PropertyEditor` prefixes to `UiPropertyInspector` and updated
  consumers; value-editor family names remain unchanged;
- moved demonstrations to `examples/` and regression packages to `tests/`;
- removed the obsolete V1 subset registration alias; use the complete
  `RegisterUiPropertyInspectorEditors(factory)` entry point.

## 2026-10 - adapter naming

- renamed the built-in adapter implementation and regression package to remove
  the historical V1 label; there remains one UiPropertyInspector control and factory;
- maintained demos use complete registration. The subset compatibility wrapper
  introduced at this checkpoint was removed by the package cleanup above.

## 2026-10 - editor callback lifecycle and integer presentation

- protect active and inline editor Preview/Commit across synchronous model structure notifications; defer row rebuilding until the editor callback stack unwinds, including nested event processing;
- snapshot Choice selection payloads and callbacks, re-find replaced model items, and prevent old editor callbacks from writing into a newly bound model;
- use the existing numeric/slider editor for bounded Integer properties while preserving their schema kind, range and step; SliderInt starts in slider mode and offers numeric entry;
- retain unrestricted integer entry for unbounded properties;
- add actual Choice lifecycle, schema replacement/rebinding and integer presentation regressions in UiPropertyInspectorCallbackLifecycleTest.

## 2026-08 - v1.1 semantic value adapters

- added complete standard registration; the former subset entry point was
  retained at this checkpoint and has since been removed;
- added Date, Time and DateTime adapters backed by production `UiDateTime`;
- added canonical-seconds Duration editing with ms/s/min/h presentation and no false commit when only the display unit changes;
- added semantic Point, Size and Rect compound editors;
- added Insets/Padding/Margins-style four-sided editing and four-corner radii with link/unlink presentation state;
- added Flags / multi-choice editing and bounded ordered string collections;
- added normalized Linear/Radial gradient recipes with arbitrary ordered stops, alpha, angle and interpolation;
- added canonical keyboard Key Chord editing;
- added application-provider Resource / Reference browsing without moving domain ownership into UiPropertyInspector;
- added explicit nullable Optional text/int/double values, separate from inherited/theme override state;
- split semantic implementation into scalar, collection/reference and gradient source slices while keeping `UiPropertyInspectorCore` headless;
- added `UiPropertyInspectorSemanticRunTests` deterministic contract coverage and `UiPropertyInspectorSemanticDemo` as the visual capability matrix;
- documented semantic durable-value conventions and integration boundaries in the package README.

## 2026-08 - v1 hardening and acceptance

- snapshot value-editor preview/commit callbacks before dispatch so a host rebuild triggered by Preview cannot clear the subsequent Commit callback;
- make resettable inherited projections show a passive inherited-state marker, while authored rows retain the Reset action and value-editor affordances remain independent;
- make mouse override regression coverage derive its target from live UiPropertyInspector geometry rather than stale absolute coordinates;
- restored clean Debug and Release compilation after the const-Font paint regression;
- made single Color rows stable inline swatch + `#RRGGBB` editors;
- replaced the ambiguous numeric slider glyph with a clear `12` mode toggle;
- retained direct `Check`, `OnOff`, and `TrueFalse` Boolean presentations;
- retained compact first-class `UiRangeSliderEdit` and `UiMatrixSelector` adapters with row-span geometry;
- made Icon and Font catalog enumeration lazy and shared across editor instances;
- added provider-driven compact Image thumbnails that preserve aspect ratio;
- kept rich inline editors viewport/overscan bounded under 1,000-row stress;
- moved group-summary aggregation out of Paint and into row rebuilding;
- preserved active-editor replacement, inherited override activation, Escape rollback, and host `Ctrl+Z` delegation coverage;
- fixed the UiPropertyInspectorDemo toolbar for the coordinate-based `UiLayoutCursor` API;
- expanded UiPropertyInspectorDemo into a complete editor-kind and adapter matrix, including nested headings, property indentation, state examples, and a live UiPropertyInspector-based style laboratory;
- identified the expanded demo as version 1.1.0 in both its title bar and visible toolbar banner;
- advanced the capability demo to 1.2.0 with compact/expand/dialog rich rows;
- advanced the capability demo to 1.3.0 with full-area expanded editors, optional stacked vectors, and an eight-colour palette example;
- advanced the capability demo to 1.4.0 with explicit mixed-value guidance, configurable action imagery and derived nested-heading colour;
- added `SetExpandedRowSpan` and UiPropertyInspector-owned expansion state for Matrix, Curve, Image and Multiline adapters;
- made UiPropertyInspector sliders request expanding tracks, kept vectors on one row, made nested-group indentation visible in property labels, and exposed the label divider with resize feedback;
- made the demo Image provider open a real file chooser and render the selected image, and synchronized all Quad Gradient slots through UiColorPicker;
- made the filter taller and retained live filtering on every text change;
- changed colour chips to crisp square swatches and fixed mixed floating-point editors so U++'s null sentinel is never rendered or committed as `-DBL_MAX`;
- made compact rich rows expand on activation, moved Matrix, Image, Curve and Multiline actions into compact icon rails, and removed repeated summaries from expanded layouts;
- made UiColorPicker slot count authoritative for one through eight colours and synchronized complete ordered palettes instead of only the active swatch;
- kept partial floating-point input editor-local until the mantissa and exponent are complete, while accepting signed scientific notation such as `+2.5e-2`;
- standardized editor action glyphs at 16px and exposed reset, expand, collapse, dialog and browse imagery through `UiPropertyInspectorStyle`;
- fixed first-open UiColorPicker slot realization and retained exact 1, 4 or 8 slot geometry requested by the calling property;
- confirmed the visual package has no Designer, SymbolPicker, UiComposite, or duplicate advanced-editor dependency.

## 2026-07

- split the reusable model into `UiPropertyInspectorCore`;
- finished the Ui-backed visual editor migration for the reusable UiPropertyInspector package;
- moved the browser palette toward the Ui theme system instead of the old stock widgets;
- restored a dedicated visual package changelog so the shell stops hiding the story.
