# 04 — Demo Guide

A demo is executable documentation for a control, not another application
framework. `examples/UiMediaCardDemo` and `examples/UiCollectionDemo` are the
accepted shell references. Full demos
use real Ui controls and the production PropertyEditor, remain self-contained,
and generate useful C++ that uses the public control API.

## Keep the construction visible

No mandatory DemoBase or shared DemoFramework. A small amount of repeated header,
exit and layout setup is preferable to hiding the example behind another layer.
Share production controls, styles, drawing and PropertyEditor adapters; do not
invent a convenience abstraction merely to remove harmless shell duplication.
Demos never depend on UiDesigner or another demo executable.

Every control has one designated maintained demo, or shares a documented family
demo that actually covers its concrete type. The release inventory is the
authority for that mapping. A toolbar using a control is not automatically a
feature demonstration of that control. Specialized authoring, benchmarks and
platform integration examples are explicit exceptions, not competing canonical
builders. Each demo keeps an explanatory source header and declares its direct
U++/Ui dependencies; production PropertyEditor and codecs are valid dependencies.
Generated usage C++ depends only on the public control/library APIs and declared
host resources, never on another demo or shell helper.

A substantial demo normally has a class header, implementation, tiny main.cpp and
.upp. Separate Inspector/Overrides/Code implementation only when that improves
navigation; do not split a small example into dozens of fragments. Suitable method
names are BuildHeader, BuildPreview, BuildRightRail, BuildInspectorModel,
BuildOverrideModel, ConfigureEditors, ConnectEvents, ApplyProjection, ApplyTheme
and UpdateGeneratedCode. Source comments explain those responsibilities and state
ownership, not every assignment. New names follow the coding-guide prefixes.

## Shell and page contract

Use a UiTitleCard with the control name and a useful one-line purpose. Keep
Light/Dark, Help and Exit actions compact, correctly labeled/tipped and consistent
with UiLabelDemo. Each action has its own appropriate icon; verify actual tint,
contrast, sizing and selection rather than merely checking that SetIcon is called.

Keep the title/purpose at the left and exactly three adjacent actions at the top
right, in Theme / Help / Exit order. Use 16 DPI-scaled pixel action icons in compact
34-pixel buttons. Help explains the demo and Exit closes it normally.

Provide a generous live preview of the actual control. Size/layout, content,
interaction and state must be inspectable; do not shrink it to fit more properties.
An ordinary demo is not a selectable mini-Designer canvas.

The right rail contains Inspector / Theme Overrides / optional Data / Code.
One page occupies the available rail at a time. Selection is persistent page
state, not a hover/focus trick. Hidden pages must not be re-shown by a flow-layout
pass or reserve blank space. Resizing, page changes and theme changes keep the
chosen page and editor state usable. Ordinary controls do not show an empty Data
page. A Data page is appropriate for real model/domain collections.

Page actions form a compact, left-aligned icon strip above the PropertyEditor:
Inspector / Theme Overrides / optional Data / Generated Code, in that order.
Use 17 DPI-scaled pixel icons in 38-pixel buttons, meaningful tooltips and persistent
checked feedback. Put flexible space after the strip, rather than stretching each
button across the rail. UiMediaCardDemo demonstrates this layout alongside the
UiLabelDemo reference; the same shell conventions apply to all canonical demos.

Preview and right rail are matching opaque rounded panels (8 DPI-scaled pixels),
slightly darker than the window: RGB 245/245/245 in Light, 18/18/18 in Dark.
The rail contains both its action strip and editor. Inner pages are transparent;
the PropertyEditor has no separate square outer frame. Page and header actions
have no pill face, frame or underline: grey at rest, white on Dark hover, blue
when selected. Exit stays red. Resolve these styles again on each theme change.

Keep one current executable per maintained demo in `bin/windows-x64`. Intermediate objects,
build logs, tests and generated-code acceptance output belong in `build`.
Run `python examples/build_demos.py --umk <path-to-umk.exe>` to build the current
inventory and promote successful executables. `--list` shows targets;
`--package UiGraphDemo` selects a package. The assembly includes this checkout's
examples/library, the sibling animation/state-machine libraries and U++ uppsrc.
Use `--upp-root` when the installed tool is not beside uppsrc. The generated build
report in `build/demos` records each package's outcome; compilation remains
separate from native visual and generated-code acceptance.

## Inspector and PropertyEditor

Inspector rows map to meaningful supported public APIs: content/value, roles,
fonts, enabled/read-only states, selection, orientation, geometry, ranges/steps,
icons/media and control-specific behavior. Use the production factory/adapters
rather than hand-built property rows. Prefer spatial matrices to text dropdowns
for direction/side/position; use suitable range, color/palette, font, icon, image,
curve and compound editors. Expose only values the preview can actually apply.

Property models own authored configuration. The preview and code generator read
that same configuration; a short local projection is fine, a second competing
mutable configuration store is not. On an ordinary property edit, update values
and the affected preview rather than clearing/rebuilding the entire inspector.
Preserve selection/filter/scroll/expanded state. Rebuild schema only when the set
of properties genuinely changes, and guard queued work against stale selection.

A model-backed Data page operates on the **same active production model** as the
preview: List/Gallery/Dropdown -> UiListModel, Tree -> UiTreeModel, Table ->
UiTableModel, Menu -> UiMenuModel. Graph/Doc may retain specialized authoring tools;
do not flatten their data into generic row mirrors.

Providers, domain interpretation, persistence and application commands stay in
the demo/host. Do not add demo-only meaning to PropertyEditorCore or the factory.
See [PropertyEditor](05_UI_PROPERTY_EDITOR_GUIDE.md) for adapter and transaction details.

## Theme and authored overrides

Minimal Standard/Subtle/Accent/Alert in Light/Dark is the common visual baseline.
Preserve family-specific roles and data colors. Apply the complete shell/panel/
PropertyEditor palette when switching modes, following the reference's U++ theme
bridge; changing only the preview is not a complete demo theme change.

Synchronize the U++ native palette as well as UiTheme (the reference uses
Ctrl::SwapDarkLight on an actual mode transition). Explicitly apply the matching
PropertyEditorPaletteMode to every editor, including hidden pages. Resolve any
custom header/panel/label/button styles again; stale custom snapshots do not
inherit a newly selected mode. Check the exposed window background, header icon
contrast and filter/editor surfaces in Light / Dark / Light.

Paint the exposed window area with the resolved shell surface, including behind
the title and header actions. A dark preview inside a native grey window leaves
the header disconnected; switching the native palette alone does not establish
the intended shell background. Keep nested inspector surfaces deliberate.

Inactive style rows inherit the current theme; active rows apply only the local
recipe the control supports. Explicit None is not the same as inheritance. Reset
restores current theme behavior, not a stale resolved snapshot. Some control APIs
own a complete custom-style snapshot: project active authored fields onto a newly
resolved base when the demo intentionally offers per-field inheritance.

Expose real palette states and supported metrics/skins/focus/shadows only. Match
actual runtime nouns and nesting: Face/Skin, Frame, Ink, Icon, Typography, Content
Margin, Focus, Shadow, Highlight and real subparts such as Track/Thumb or Popup.
Do not present an unused style field just because it is in a struct. Preview and
generated C++ must express the same authored behavior.

Use the name **Frame Accent** for the shared edge decoration. Where a preview
surface consumes it, expose independent Top/Bottom/Left/Right choices, thickness,
colour (including inherited/state frame colour) and alpha. Keep its ordinary Frame
settings separate. A bounded thickness slider should use a useful small domain
(normally 0–20 DPI-scaled pixels), not a generic 0–1000 range. Verify curved corners,
adjacent/opposite/all edges, hidden ordinary frame, disabled state and Light/Dark.
For Panel/GroupPanel/ScrollPanel also check header gaps and fixed scroll chrome.
Generated C++ emits the actual `metrics.frame_accent` fields or public setter;
reset restores no authored edges. Never offer nonfunctional accent fields for a
custom-painted subpart simply because its Style contains StyledMetrics.

## Generated C++ is part of acceptance

Generate readable usage code, not the whole demo shell. With no overrides, use a
concise role/theme-based example. With active overrides, emit those authored
settings and any necessary public paint-hook wiring. Include required headers,
resources and object lifetimes; no unexplained demo helper dependencies.

A generator must escape arbitrary text, preserve numeric precision and use the
selected concrete control type. It must distinguish authored settings from current
interaction state. Resource/provider and application callback code that cannot be
serialized must be explicitly identified as host-supplied, not silently omitted
under a claim of complete reproduction.

Where useful offer Usage / Current changes / Full explicit recipes. This does not
require destabilizing a clear existing Code page simply to add another selector.
Paint examples use production Draw/Painter/UiShapes APIs, never private demo-only
shape tessellation or a parallel allocator.

The acceptance sequence is: configure preview, generate output through the actual
generator, compile it **unchanged** in a minimal declared-dependency package, and
check equivalent configuration/behavior. A hand-written lookalike is not generated-
code evidence. Cover default, changed values, explicit style overrides and supported
paint hooks. Reset/inheritance, special text and each family type also matter.

## Family demos and retirement

Combine controls only when they share a genuine production/style/teaching foundation.
The existing UiEditDemo covers Line/Password/Mask/Multi-line; UiIntFloatDemo is the
numeric companion. UiSliderDemo covers Slider, RangeSlider and both slider editors;
UiButtonDemo covers Button, SplitButton and ToolButton.
Its adjacent selectors retain separate inspector and override models for each type,
show one live preview, and generate only that concrete control. The standalone
UiSplitButtonDemo and UiToolButtonDemo packages are retired.
Do not create a third edit-family demo or put every widget into an enormous switch.

When changing a family type, show only applicable properties, preserve/restore its
own authored state deliberately and generate that type's actual C++. Common shell
code stays explicit within the package. Grouping should make reading easier, not
hide each control behind a generic adapter class.

Use compact adjacent selector buttons above the preview for a family. Selecting
one shows that concrete control alone and routes its inspector, overrides and code
to the same selected configuration. Collection may offer an explicit Compare
option, but its default is a single selected view. A control-type dropdown in the
inspector is not the default family navigation.

Bounded integer properties offer the number/slider toggle and use their real
minimum, maximum and step. Counts such as 1–10 keep that range; do not substitute
an arbitrary 0–1000. Demo preview widths/heights generally stop at 1000 and corner
radii at 60. Font sizes, frame widths, gaps and durations each need appropriate
units and bounds. Unbounded application data must not acquire a fabricated slider
domain or be silently clamped merely to enable that presentation.

Old demos are removed only after the replacement retains their useful examples,
public behavior, generated-code options and a working compile/run path. File age,
shorter code or a newer toolbar is not proof of replacement. Benchmarks, behavioral
regressions and specialized authoring tools are not redundant merely because a
canonical control demo exists. Track disposition in the release inventory rather
than accumulating separate migration reports.

## Focused manual acceptance

Open each canonical demo, exercise every page and relevant control interaction,
change role and Light/Dark/Light, resize, and close normally. Check icon identity,
readable contrast, all displayed property effects, reset/inheritance, disabled and
focus behavior, AA corners, and at least representative DPI settings. For model
views edit/reorder data without a second collection; for edit families test partial
input and cancellation. Confirm generated output through the compile gate above.

An idle demo has no unintended repeating repaint/diagnostics loop; intentional
caret/animation is bounded and stops correctly. Build success is not visual PASS.
Preserve the exact tested SHA/toolchain and state what remains untested. Gary's
surgical compile smoke is an implementation checkpoint, not the entire release gate.
