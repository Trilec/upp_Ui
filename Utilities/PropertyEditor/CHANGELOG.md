# PropertyEditor Visual Changelog

Version: **1.1.0**

## 2026-08 - v1.1 semantic value adapters

- added complete standard registration through `RegisterPropertyEditorEditors()` while retaining `RegisterPropertyEditorV1Editors()` for compatibility;
- added Date, Time and DateTime adapters backed by production `UiDateTime`;
- added canonical-seconds Duration editing with ms/s/min/h presentation and no false commit when only the display unit changes;
- added semantic Point, Size and Rect compound editors;
- added Insets/Padding/Margins-style four-sided editing and four-corner radii with link/unlink presentation state;
- added Flags / multi-choice editing and bounded ordered string collections;
- added normalized Linear/Radial gradient recipes with arbitrary ordered stops, alpha, angle and interpolation;
- added canonical keyboard Key Chord editing;
- added application-provider Resource / Reference browsing without moving domain ownership into PropertyEditor;
- added explicit nullable Optional text/int/double values, separate from inherited/theme override state;
- split semantic implementation into scalar, collection/reference and gradient source slices while keeping `PropertyEditorCore` headless;
- added `PropertyEditorSemanticRunTests` deterministic contract coverage and `PropertyEditorSemanticDemo` as the visual capability matrix;
- documented semantic durable-value conventions and integration boundaries in the package README.

## 2026-08 - v1 hardening and acceptance

- snapshot value-editor preview/commit callbacks before dispatch so a host rebuild triggered by Preview cannot clear the subsequent Commit callback;
- make resettable inherited projections show a passive inherited-state marker, while authored rows retain the Reset action and value-editor affordances remain independent;
- make mouse override regression coverage derive its target from live PropertyEditor geometry rather than stale absolute coordinates;
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
- fixed the PropertyEditorDemo toolbar for the coordinate-based `UiLayoutCursor` API;
- expanded PropertyEditorDemo into a complete editor-kind and adapter matrix, including nested headings, property indentation, state examples, and a live PropertyEditor-based style laboratory;
- identified the expanded demo as version 1.1.0 in both its title bar and visible toolbar banner;
- advanced the capability demo to 1.2.0 with compact/expand/dialog rich rows;
- advanced the capability demo to 1.3.0 with full-area expanded editors, optional stacked vectors, and an eight-colour palette example;
- advanced the capability demo to 1.4.0 with explicit mixed-value guidance, configurable action imagery and derived nested-heading colour;
- added `SetExpandedRowSpan` and PropertyEditor-owned expansion state for Matrix, Curve, Image and Multiline adapters;
- made PropertyEditor sliders request expanding tracks, kept vectors on one row, made nested-group indentation visible in property labels, and exposed the label divider with resize feedback;
- made the demo Image provider open a real file chooser and render the selected image, and synchronized all Quad Gradient slots through UiColorPicker;
- made the filter taller and retained live filtering on every text change;
- changed colour chips to crisp square swatches and fixed mixed floating-point editors so U++'s null sentinel is never rendered or committed as `-DBL_MAX`;
- made compact rich rows expand on activation, moved Matrix, Image, Curve and Multiline actions into compact icon rails, and removed repeated summaries from expanded layouts;
- made UiColorPicker slot count authoritative for one through eight colours and synchronized complete ordered palettes instead of only the active swatch;
- kept partial floating-point input editor-local until the mantissa and exponent are complete, while accepting signed scientific notation such as `+2.5e-2`;
- standardized editor action glyphs at 16px and exposed reset, expand, collapse, dialog and browse imagery through `PropertyEditorStyle`;
- fixed first-open UiColorPicker slot realization and retained exact 1, 4 or 8 slot geometry requested by the calling property;
- confirmed the visual package has no Designer, SymbolPicker, UiComposite, or duplicate advanced-editor dependency.

## 2026-07

- split the reusable model into `PropertyEditorCore`;
- finished the Ui-backed visual editor migration for the reusable PropertyEditor package;
- moved the browser palette toward the Ui theme system instead of the old stock widgets;
- restored a dedicated visual package changelog so the shell stops hiding the story.
