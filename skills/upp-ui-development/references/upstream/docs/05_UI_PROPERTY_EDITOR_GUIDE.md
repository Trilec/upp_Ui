# 05 — PropertyEditor Guide

The reusable property system has a headless model (`Utilities/PropertyEditorCore`)
and a Ui-backed visual editor (`Utilities/PropertyEditor`). Neither depends on
UiDesigner, SymbolPicker or a demo executable. The host owns application semantics,
commands/undo, resource providers and persistence.

## Boundaries and lifetime

PropertyEditorModel describes typed values, defaults, ranges, validation, groups,
help/units, mixed/inherited state, visibility, indentation/row spans and refresh
impact. Core may store opaque custom-editor/provider identifiers without depending
on their concrete GUI implementation. It is useful to GUI, CLI, agent and tests.

PropertyEditor owns row/filter rendering, selection, inline/popup editor lifetime,
viewport-limited creation and preview/commit/cancel interaction. PropertyEditorFactory
is the kind/adapter factory; do not introduce a second competing factory or duplicate
advanced-editor implementations. Bind a model with SetModel(&model); detach with
SetModel(nullptr) before destroying/replacing the borrowed model as required by
its lifetime contract. Replacing the model intentionally resets active editing.

Production callers normally use RegisterPropertyEditorEditors to install the
complete standard adapters. The existing V1 registration entry remains compatible;
do not remove it without sweeping its actual callers.

## Schema and values

Choose a property type/adapter for what a value means, not just its storage type.
Ordinary schema supports text/multiline, integer/double, Boolean, choice, color,
palettes, fill recipes, paths, sliders, vectors, curves, read-only and custom values.
Semantic adapters cover date/time/date-time, duration, point/size/rect, linked
four-sided insets/radii, flags, ordered strings, gradients, key chords, optional
nullable values and application-owned references/resources.

Default/range/unit/help and visibility/read-only/enablement belong to the property
model. Unsupported metadata must not pretend to change the preview. Mixed means
multiple differing authored values; it is not a corrupt sentinel value. Optional
Null, inherited/theme value, resettable default and explicit None are separate
states. Never display U++'s null double sentinel as a real large negative number.

Vector2/Vector3 are numeric ValueArrays. Generic curves are ValueArrays of two-
number points; the Bezier adapter uses four scalars [x1,y1,x2,y2]. Use the existing
AddBezierCurve helpers: x is normalized time, while the declared y range can permit
overshoot. Duration stores canonical seconds even when displaying ms/s/min/h;
changing the display unit alone must not emit a false value commit.

Gradient recipes preserve ordered stops, alpha, angle and interpolation. Date/time
uses the production local-value UiDateTime policy; do not invent a time-zone contract
inside the editor. Incomplete floating mantissa/exponent input stays editor-local
until valid; signed scientific notation remains accepted.

## Preview, commit, reset and cancellation

WhenPreview is temporary editing; WhenCommit is a durable authored change.
WhenCancel restores the edit origin, including mixed/inherited state where needed.
WhenReset expresses the reset operation. WhenUndoRequest delegates to host history;
the generic editor does not own a second application undo stack.

Callbacks may synchronously rebuild the inspector. Snapshot in-flight preview/
commit callbacks where a preview rebuild could clear a later commit; guard lifetime
before subsequent access. Prevent redundant same-property reconstruction while an
inline editor is committing, particularly when a modal picker is involved. A
successful picker opening is not permission to lose the committed callback.

Ordinary edits should refresh dependent values/summaries without destroying the
entire model. Preserve selection, filter, scroll and expanded rows. A genuine
schema/type change may reconstruct under a guarded selection/page/revision and
restore relevant state. Delayed work must not overwrite a newer selection.

## Inherited and local overrides

An inactive override shows inherited state. Editing its value first requests local
override activation through WhenOverride(true), then commits the authored value;
the host owns that transition. Reset returns to inheritance where the host schema
says so. Passive inherited markers are not the same action as ordinary Reset.
A fallback commit path must preserve activation even when a value editor's usual
mouse path is bypassed.

A control's SetCustomStyle often owns a full snapshot. A per-field Overrides page
therefore projects active authored fields onto a newly resolved theme base. Do not
freeze resolved values in inactive rows or conflate explicit None with Use theme.
Generated C++ and the preview must read the same authored recipe.

## First-class adapter choices

Use the existing helpers/factory for Matrix, Range, Adjustable Range, Color,
Color Palette, Fill Recipe, Icon, Font, Image, Curve/Bezier and numeric slider/text
editing. Spatial side/position choices should use UiMatrixSelector when clearer
than a textual dropdown. Cardinal4 excludes illegal center/diagonal choices.

Range and adjustable-range use the real slider/edit controls. Adjustable values
remain ordered lower-bound/lower-selection/upper-selection/upper-bound. Numeric
slider/text presentation is editing UI, not another semantic value. Boolean Check,
OnOff and TrueFalse are supported alternatives, not custom handwritten rows.

Icon/font catalog enumeration is lazy and shared across editor instances. Image
thumbnails are provider-driven, compact and aspect-preserving. Color palettes use
UiColorPicker's complete one-to-eight-slot contract; all slots synchronize, not
only the active chip. One-color callers explicitly request one slot on first open.

Core stores opaque custom_editor/editor_variant/picker_provider identifiers.
The visual factory supplies concrete implementations. Resource/reference providers
remain host-owned; the editor cannot infer application domain identity from a label.
RegisterPicker callbacks receive the current Value and owner Ctrl and return an
accepted result; RegisterThumbnailProvider supplies display resources. Do not add
hard dependencies on a particular asset browser, Designer or application dialog.

## Override page grammar

Use the control's actual runtime nouns and supported nesting:

- General and semantic role;
- Face/Skin, Frame, Ink, Icon, Typography, Content Margin;
- Focus, Shadow and Highlight;
- real subparts such as Track/Thumb/Popup/Selected only where supported.

Frame width/color/visibility are one coherent group; do not duplicate them under
several names. General styling changes do not silently become semantic data edits.
Only expose fields the control's real render/layout path consumes and generated
code can express. The presence of a member in an old Style struct is not proof
that every custom paint path honors it.

PropertyEditor should demonstrate useful standard controls/adapters, not a generic
bag of all properties. Domain-backed Data pages edit the same production model as
the preview. Graph and document authoring may use dedicated transactions instead
of pretending to be ordinary scalar rows. See the Models and Demo guides.

## Geometry, appearance and scale

SetRowSpan and SetExpandedRowSpan describe compact/expanded capacity. Expandable
Matrix, Curve, Image and Multiline editors use PropertyEditor-owned expansion
state and compact action rails. Rich editors may expand inline or use a provider
picker; the host chooses meaningful presentation rather than creating another
layout implementation per property.

Inline editor creation is viewport/overscan bounded, not one Ctrl per property.
GetInlineEditorCount and stress fixtures make this observable. Layout computes
row/column/action/editor rectangles and reusable summaries; Paint does not create
all editors or aggregate entire groups. Guard model/active-editor replacement and
preserve the actual selected/expanded property through ordinary value changes.

SetPaletteMode can follow UiTheme or select Light/Dark. Complete mode switching
refreshes editor surfaces, filter, rows and glyphs, not just the previewed control.
PropertyEditorStyle owns action imagery (reset/expand/collapse/dialog/browse),
filter spacing, nesting/indentation and label divider geometry. The divider gives
resize feedback; rich inline controls receive usable space. Color chips remain
crisp and actual colors, not a role-tinted substitute.

## Validation and documentation

Retain the existing PropertyEditorTests, V1/Semantic suites, OverrideCommit,
SortOrder, WorkingRange and CoreProbe coverage. PropertyEditorDemo is the capability
reference; SemanticDemo is a focused semantic matrix, not automatically obsolete.
Core must still compile without CtrlCore/CtrlLib/Ui. Visual changes need native
edit/cancel/rebuild, picker-slot, inherited activation, mixed/partial numeric,
filter/scroll and viewport-pool checks. Exercise repeated Light/Dark transitions.

The release inventory is a coverage register, not automatic acceptance. Compile
actual generated demo output unchanged in a minimal dependency package, including
active overrides/resources. Record source review separately from native execution
and visual usability. A build-only PASS does not prove commit ordering, glyph
readability, picker negotiation or a 1,000-row editor population bound.
