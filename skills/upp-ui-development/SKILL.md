---
name: upp-ui-development
description: Build, integrate or repair native U++ C++ applications and reusable Ui controls, including ownership, layouts, themes, models, PropertyEditor and UMK packages. Use for native implementation; not for Designer JSON authoring or browser-only mockups.
---

# U++ and Ui development

Use the target checkout as API authority. Bundled references are portable snapshots,
not a substitute for the headers of the version being compiled. Do not assume the
machine paths or API summaries from an old session still apply.

## Start with the relevant contracts

- Read [coding and ownership](references/upstream/docs/00_UPP_CODING_GUIDE.md)
  and [build discovery](references/build.md) before implementation.
- For controls, use the [catalogue](references/upstream/docs/01_UI_CONTROLS_GUIDE.md)
  to locate the actual public header and maintained example. Verify setter names,
  return types, model binding and child-host APIs before writing calls.
- Read [layout and interaction](references/composition.md) for application shells,
  scrolling, focus and control integration.
- For appearance, read [themes](references/upstream/docs/02_UI_THEME_GUIDE.md);
  for a custom renderer, read [drawing](references/upstream/docs/07_UI_DRAWING_GUIDE.md).
- For data-backed views, read [models](references/upstream/docs/03_UI_MODEL_GUIDE.md).
  For inspectors, also read [PropertyEditor](references/upstream/docs/05_UI_PROPERTY_EDITOR_GUIDE.md).
- For control demos, read the [demo contract](references/upstream/docs/04_UI_DEMO_GUIDE.md).
  For graphs, start with [graph usage](references/upstream/docs/08_UIGRAPH_GUIDE.md);
  read [graph internals](references/upstream/docs/09_UIGRAPH_DEVELOPMENT.md) only when changing that engine.

## Preserve U++ semantics

Parenting is not deletion ownership. Prefer member controls, One or owning Array;
borrowed models outlive views. Ptr is an observer, not shared ownership. Moveable
is a relocation promise, not a generic optimization. Treat pick/clone deliberately.
Keep callbacks, timers, GUI capture and transient editor lifetimes explicit.
Callbacks can synchronously rebuild or destroy the originating control.

Use one authoritative semantic model and the current notification/request APIs.
Validate imported values before replacing live state. Null, inherited, mixed,
explicit None and an incomplete edit are distinct states. Keep preview, commit,
cancel and undo responsibilities separate.

Compose with native controls and Ui layouts; fix reusable defects at their owning
layer. Do not conceal a bad measurement, focus path, clipped paint or missing
notification with application-specific padding or repaint overrides. An actual
application spacing preference belongs in its layout.

Follow existing naming, header guards, package boundaries and direct dependencies.
Do not introduce an ops-table architecture merely because an old prompt recommends
it. Ordinary virtual interfaces or value models are appropriate when they match
the current code. Do not import old Chameleon recipes into Ui's role resolver.

## Verify what will ship

Build the runnable caller with the actual assembly and method. For shared changes,
run focused regression tests and repository-required checks, including relevant
Debug/Release and header/BLITZ coverage. Inspect changed controls in native UI:
small bounds, resize, keyboard/focus, Light/Dark, disabled and selected states.
Compile generated C++ when its contract changes. Successful compilation is not
proof of visual correctness or a working interaction.

Report exact artifact paths, checks performed and remaining limits. A historical
guide's publishing section does not authorize committing, pushing or releasing.
