# 00 — U++ Coding Guide

Reusable engineering rules for the Ui library family. Read this before changing a
control, demo or package. The Controls Guide describes the public surface; the
Theme, Models, Demo, PropertyEditor and Drawing guides own their respective
contracts. Graph usage and Graph development are separate guides.

## Packages and dependencies

A `.upp` file defines a package. A library has no application entry point and no
`mainconfig`. A runnable test/demo supplies `GUI_APP_MAIN` or `CONSOLE_APP_MAIN`
and an appropriate configuration. Do not add `main`/`WinMain` to a library merely
to make an executable build command link. Compile libraries through a real caller.

Declare direct package dependencies in `uses` and direct header dependencies in
source. Keep `.upp` file membership complete, including internal `.inc` parts
that TheIDE should expose. Production never depends on demos/tests. Headless
packages may use Core/Draw where required but never acquire CtrlCore/CtrlLib/Ui
merely to reuse a GUI helper. Image import/export callers declare the relevant
`plugin/png`, `plugin/jpg` or `plugin/bmp` package.

Use the actual assembly `.var` and installed build method. The repository's
`GitHubOut.var` is the maintainer's Windows configuration, not a portable path
promise. Record the U++ build and external dependency revisions when validating.
Build output belongs in the assembly output directory or a temporary evidence
directory, never among source files. See [Getting Started](../GETTING_STARTED.md).

## Source organization and comments

Headers have an include guard, necessary includes, `namespace Upp`, public API,
then private state. Use `_Package_Header_h_` guards. A Ctrl type using THISBACK
has `typedef T CLASSNAME;`. Keep substantial Paint/Layout/event implementations
in `.cpp`; tiny getters can remain inline. `.inc` is appropriate for deliberate
single-translation-unit implementation parts, not a second production copy.
Do not mechanically convert `.inc` to `.h` or fragment an already small control.

Each public control header explains author/license, purpose, intended usage,
GUI-thread context, non-obvious ownership and a short example. Document range,
null, normalization and callback semantics beside the relevant public methods.
Implementation comments explain invariants and reasons, not obvious assignments.
Remove stale alternative implementations and misleading comments, not useful
rationale. Header comments refer to the library release identity in `UiVersion.h`;
individual controls do not maintain independent release counters.

Public API follows U++ naming: SetX/GetX, WhenX events, SetData/GetData for binding.
Keep family vocabulary stable: SetText for primary text, SetTitle/SetSubTitle for
container identity, UiDirection/UiAlign for direction/alignment. Convenience
fluent APIs are allowed where the family already uses them. Do not add aliases
or meaningless getters simply to force spelling symmetry. Changing a public
contract requires its callers, tests, generated C++ and documentation to change
in the same coherent slice; discuss disruptive changes before publishing them.

New demo/application names should expose responsibility: `tc_header`,
`box_header_actions`, `pnl_preview`, `stk_pages`, `btn_copy`, `lbl_caption`,
`pe_inspector`, `pe_model_inspector`, `edit_code`, `str_code`, `img_preview`,
`val_icon`, `ctx_theme`. Do not churn established member names for cosmetics.

## Parenting is not C++ ownership

Adding a Ctrl to a parent establishes its GUI relationship; it does **not**
transfer responsibility for deleting the C++ object. Removing it detaches it.
Prefer member controls, `One<T>` or an explicitly owning `Array<T>`. Never infer
heap ownership merely from Add/SetContent/SetModel. A borrowed child/model must
have an explicit lifetime contract; use `Ptr<Ctrl>` for potentially destroyed
controls and confirm the current parent before resizing/detaching borrowed
content. Reject self/ancestor parenting before changing existing content.

Do not attach one control to two parents or introduce parent cycles. Composite
construction/destruction, child removal/reparenting and model replacement are
regression-test cases. External models must outlive their active binding unless
the API explicitly supplies a lifetime-safe detach mechanism.

## Callbacks, capture and timers

A committed notification observes the new public state. A request event reports
intent before an application-owned model mutation. These are different contracts;
see [Models](03_UI_MODEL_GUIDE.md). State whether programmatic setters emit user
events. Keep preview, commit and cancellation distinct; do not manufacture a
second commit on capture loss. Document whether a cancelled live edit rolls back
or retains its last live value.

A user callback may synchronously rebuild or destroy the originating control.
Copy an in-flight callback before dispatch where reconfiguration can clear it;
guard subsequent member access with a lifetime-aware pointer. Capture by value
where appropriate and never leave an unguarded delayed raw `this`. Rebinding
must ignore inactive/old model notifications, including address reuse.

Use owned TimeCallback/UiFrameTicker or the established Animation facility.
Cancel work on hide/remove/destroy where it no longer has a purpose. Ctrl timer
IDs are internal byte-offset identifiers, not arbitrary application handles;
do not invent large integer IDs. A replaced one-shot uses KillSet, not an
accumulating queue. Idle controls need no repeating clock; caret blinking and
intentional animation have explicit ownership and stop conditions.

## Values, validation and semantic authority

Use typed values where fixed; U++ Value/ValueArray/ValueMap are appropriate for
binding, property models and durable payloads. Validate types, enums, ranges,
finite numbers and Null sentinels before mutation. Validate imports into a
candidate before replacing live state. ASSERT protects programmer invariants;
it is not Release input validation. Preserve incomplete numeric text locally
until a complete value can be committed.

Prefer Vector, Array, VectorMap, Index, One and pick/clone with clear ownership.
One concern has one semantic authority. View projections and raster/geometry
caches are disposable derivatives, not parallel editable models. Do not restore
retired RefreshFromModel-style synchronization when model notifications suffice.
Persistence schema versions are independent from the library release version;
never reset or increment a schema for a cosmetic release-number change.

## Geometry, themes and rendering

Apply DPI exactly once. Fit/Fixed/Expand are sizing modes; alignment positions
within an allocation. GetMinSize/GetContentSize/Layout, hit testing and generated
code must agree on frames, skin insets, content margins, gaps and item spacing.
Geometry-affecting setters invalidate layout and paint; visual-only setters
invalidate paint. An unchanged setter should avoid needless work.

Every themeable visible control has meaningful Minimal Standard/Subtle/Accent/
Alert behavior in Light and Dark. Family typography roles and actual content
colors remain distinct from semantic emphasis. Pure layout/value helpers do not
need invented colored faces. Follow [Theme](02_UI_THEME_GUIDE.md).

Paint must not mutate models, emit semantic callbacks, open resources or start
clocks. Keep clipping balanced and handle empty/tiny rectangles. Direct Draw is
appropriate for simple straight geometry/text; native Painter supplies smooth
curves. Reuse bounded exact rasters when beneficial. Do not impose full-control
buffers on every widget. The shared final-device-pixel geometry budget is 0.35 px
within its documented numeric envelope, not a guarantee of identical backend
stroke/AA pixels. See [Drawing](07_UI_DRAWING_GUIDE.md).

## Review and release acceptance

Audit every concrete public control, not only controls with convenient demos.
Check API/ownership, four-role behavior, input/callback/cancellation, rendering
cost, source docs, canonical demo, generated C++ and relevant tests. Inheritance
can share implementation review but does not waive specialized behavior checks.
The machine-readable [release inventory](../tests/ui_release_inventory.json)
records coverage and unresolved work; an untested row is not a PASS.

Debug and Release must compile. A non-BLITZ/header-isolation build catches hidden
include dependencies; also preserve the supported BLITZ path. Keep diagnostics
and tests deterministic and fail on nonzero exit, missing summary or zero executed
checks. Do not delete a legitimate regression merely to reduce target count.
A replacement demo/test must retain the old useful coverage before removal.

Version changes come from `Ui/UiVersion.h`. The release candidate is not a claim
that all platform/visual gates passed. Public 1.0 compatibility starts only after
the declared release surface and known limitations are accepted. Independently
versioned sibling packages retain their own version histories.

## Publishing and recovery

Refresh remote main, inspect complete touched source/callers/tests, implement a
coherent slice, review the full diff, run git diff --check, publish and verify the
remote commit/diff/ancestry. Rebase only the intended changes onto a newer main;
never overwrite concurrent work or force-push it backwards.

Keep ACTIVE_WORK.md at no more than 100 lines: BASE / TASK / TOUCHED / STATUS /
PUBLISHED / VALIDATION / NEXT ACTION for current work only. Store contracts in
these guides, history in Git and validation logs outside source. PUBLISHED may
identify the containing commit by a stable git-log path to avoid a self-referential
SHA. Preserve concurrent workstreams and their outstanding validation boundaries.

Give the validator one copy-paste task: latest branch, required ancestor, exact
commands, expected summaries, focused manual checks, stop conditions, evidence
and allowed minor fix policy. Source-reviewed, compiled, runtime-tested and
visually accepted are separate evidence. Report only the states actually proved.
