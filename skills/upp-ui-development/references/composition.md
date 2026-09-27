# Native composition and control repair

Choose structure from sizing behavior. UiGridLayout places stable rows/columns;
UiBoxLayout handles ordered rows/columns, spacers and supported wrapping.
SetGridSize takes columns then rows; placement APIs take row then column—check
the overload. Fit measures preferred content subject to constraints, Fixed asks
for an extent, and Expand shares allocated space. Minimums do not create space
in an undersized parent. Avoid nested boxes with no meaningful sizing purpose.

Build a shell with header, expanding body and footer; put sidebars and workspace
inside the body. Use a wrapping Box where flow is intended, rather than assuming
CSS grid/flex rules are identical. Keep footer and assistant areas in the layout,
not painted over the workspace. A plain label is often sufficient for a title.

UiPanel is a styled surface, not automatic flow. Single-root hosts need one
layout root for multiple children. Read each host's attachment API: UiScrollPanel
exposes Content(), and its scrolling children belong there, not beside it.
UiGroupPanel has separate header/body slots. UiTab, UiStack and UiAccordion own
page/section visibility; don't let a layout accidentally show inactive pages.
Preserve logical SizePos/HSizePos/VSizePos when code measures and repositions
children; assigning a raw rectangle can erase the anchoring contract.

Frames, shadows, focus rings, skin insets and content margins participate in
measurement as documented. Apply DPI once. Use the shared geometry for Layout,
Paint, hit tests and min/preferred sizes. Don't fix a clipped icon by changing
its drawing rectangle while leaving hit and sizing rectangles unchanged.
Painter::Ellipse uses centre/radii; Draw::DrawEllipse takes a rectangle.
Keep aspect ratio for pictographic carets; explicit rectangular icon sizes may
intentionally stretch in controls that support that contract.

Prefer UiList/Tree/Table/Gallery models for data, not a Ctrl per record. A sparse
active cell editor is different from thousands of permanent editors. Keep stable
keys where needed and emit the narrow model notification after committed changes.
No speculative preallocation based on total logical item count during Paint.

Keyboard navigation follows the visible tree projection across expanded siblings
and groups and stops at ends. Handled keys must not leak into unrelated selectors.
Scroll interaction must route to the actual viewport/bar; overlays consume the
same clipped scrolled geometry. Capture loss cancels/completes per the control's
contract exactly once. Preserve page, selection and scroll through theme changes.

Ui themes distinguish preset, Light/Dark/System mode and semantic role. Keep
authoring/review mode separate from an exported app's startup mode. A custom
style is an owned snapshot; ClearCustomStyle resumes inheritance. Transparent
None is not a request for an OS-colored fallback. Use theme-aware icon/ink/focus
colors, and account for shadows in layout instead of painting beyond allocation.

For legacy CtrlLib controls, inspect the actual typed Style and ChPaint/ChMargins
contracts. SetStyle may borrow a style whose lifetime must cover use. Ui's
SetCustomStyle owns a snapshot. There is no universal interchangeable
Chameleon_Style matrix for both systems.
