# ACTIVE WORK

Remote GitHub is authoritative. Never force-update `main`. Work directly from refreshed `main`; do not create temporary feature branches for ordinary control normalization.

## CURRENT SUPERVISORY STATE — 2026-08-17

STATUS: **UIGRAPH CORRECTIVE ACTIVE; UI LABEL REFERENCE ACCEPTED; FOUR-CONTROL SOURCE WORK COMPLETE; WINDOWS VALIDATION PENDING.**

The previous long-form convergence/Label/UiGraph record is preserved verbatim in `docs/ACTIVE_WORK_ARCHIVE_PRE_FOUR_CONTROL_2026-08-17.md`. Task-specific four-control detail is retained in `docs/ACTIVE_WORK_UI_OVERRIDE_ROLLOUT.md`.

### Authoritative published checkpoints before this commit

- UiLabel reference and PropertyEditor ordering are accepted; latest published Label demo styling checkpoint recorded in the archived log is `8a016656651fb929c08ac1c2f801a6b8c2f2ab77`.
- UiGraph scale/source-review work is already promoted into `main`; its source checkpoint is recorded in the archived log. Windows focused scale validation remains pending.
- UiList runtime style-authority + renderer contract: `d0579b8753748ca765710f6c29805d2859ddf6aa`.
- UiList striped-row style persistence: `97d1531192f712365cddea1f9390a1a031e01836`.
- cross-repository `Trilec/upp_uidesigner` List + Edit adapter checkpoint: `c27f499c8d51ad73037d9a60481bb73d870d38a7`.
- cross-repository `Trilec/upp_uidesigner` Dropdown + Accordion checkpoint/current accepted source head: `ec02f1cbcc040f70ad55e656b98ec64640142cec`.
- this commit completes the four Ui demo ownership references and package routing.

## ACTIVE TASK — FOUR-CONTROL OVERRIDE NORMALIZATION

TASK: `UI-LIST-EDIT-DROPDOWN-ACCORDION-OVERRIDE-NORMALIZATION`.

SOURCE RESULT:

- `UiList` built-in rows now project the owning List style instead of independently resolving presentation from global theme state; explicit custom item renderers retain renderer ownership.
- `UiList::Style::striped_rows` is serialized and covered by the focused style contract test.
- Designer has dedicated normalized adapters for UiList, UiBaseEdit, UiDropdown and UiAccordion; existing authored field ids are preserved where they existed.
- Face fields promoted from plain Color to FillRecipe accept legacy Color values as Solid recipes rather than losing authored data.
- Dropdown preserves collapsed-control ownership separately from nested `Popup/*` domains.
- Accordion preserves outer chrome separately from composed `Header/*`, `Body/*`, Section, Behaviour and Animation domains.
- resource-backed Skin/custom-image editing remains intentionally deferred until Designer theme adapters receive document-resource resolution; do not add raw path workarounds.
- four Ui demos use one local config/model authority each. The large legacy List/Dropdown builders remain in their old `main.cpp` files for reference but are no longer package entrypoints; `NormalizedDemo.cpp` is compiled instead.
- demo ownership sections contain live runtime-backed fields; no placeholder/empty style section is intentionally exposed.

TOUCHED BY FINAL DEMO CHECKPOINT:

- `examples/UiListDemo/NormalizedDemo.cpp`
- `examples/UiListDemo/UiListDemo.upp`
- `examples/UiLineEditDemo/main.cpp`
- `examples/UiDropdownDemo/NormalizedDemo.cpp`
- `examples/UiDropdownDemo/UiDropdownDemo.upp`
- `examples/UiAccordionDemo/main.cpp`
- `docs/ACTIVE_WORK.md`
- `docs/ACTIVE_WORK_UI_OVERRIDE_ROLLOUT.md`
- `docs/ACTIVE_WORK_ARCHIVE_PRE_FOUR_CONTROL_2026-08-17.md` (preserved prior ACTIVE_WORK blob)

## WINDOWS VALIDATION REQUIRED

No Windows/U++ PASS is claimed for the new four-control work yet.

`Trilec/upp_Ui` on the exact final `main` HEAD:

1. Build/run `Utilities/UiListStyleContractTest` Debug + Release; require `UILIST_STYLE_CONTRACT_SUMMARY checks=13 failed=0`.
2. Build/run `Utilities/UiModelViewPerformanceTest` Debug + Release; require 52/0 and unchanged second List `Layout()` reporting zero renderer relayouts.
3. Build `examples/UiListDemo`, `examples/UiLineEditDemo`, `examples/UiDropdownDemo`, and `examples/UiAccordionDemo` Debug + Release with zero compile errors; report warnings.
4. Launch all four demos. Confirm every visible ownership section contains working controls and changes affect the live preview without crashes/corruption.
5. UiList smoke: viewport chrome remains distinct from `Rows/*`; model rows, badges, metadata/checks and drag settings remain functional.
6. UiLineEdit smoke: Editing, Underline, Whitespace, Focus, Shadow and Highlight settings affect the live edit as applicable.
7. UiDropdown smoke: collapsed control remains usable; open popup after changing Popup/Items/Marker/Badge settings and verify the nested style domains visibly remain popup-owned.
8. UiAccordion smoke: headers/bodies remain composed separately; section open policy, chevron, line, drag and animation remain functional.
9. Run `git diff --check` and confirm clean `git status --short`.

`Trilec/upp_uidesigner` at `ec02f1cbcc040f70ad55e656b98ec64640142cec` unless remote `main` has legitimately advanced:

1. Build/run `tests/ListEditThemeAdapterTest` Debug + Release; require emitted zero-failure summary.
2. Build/run `tests/DropdownAccordionThemeAdapterTest` Debug + Release; require emitted zero-failure summary.
3. Build UiDesigner Debug + Release and launch it.
4. Select UiList, UiBaseEdit-family control, UiDropdown and UiAccordion; verify normalized group ownership/order and that Face rows use FillRecipe where specified.
5. Verify authored QuadGradient recipes survive Inspector refresh/reselection and generated C++ for the covered Face fields.
6. Confirm legacy Color-backed Face values resolve as Solid FillRecipe instead of None.
7. Confirm no fake resource-path Skin editor appears.

If a substantive ownership, model, rendering, persistence or adapter-contract failure appears, return it to implementation. Do not restore retired accessors, duplicate state, weaken tests or broaden unrelated architecture during Windows validation.

## UIGRAPH CORRECTIVE — ACTIVE

BASE: `67bb841c267b983fcec470eeef9a33f5349c185c` (`main` at corrective refresh).

TASK: `UI-NODEGRAPH-CORRECTIVE-R1` — close Windows acceptance findings for live style editing, selection presentation, double-click semantics, marquee performance and UiLabel-consistent PropertyEditor structure.

TOUCHED SO FAR:

- `Ui/UiGraph/UiNodeGraphInteraction.cpp`
- `docs/ACTIVE_WORK.md`

PUBLISHED:

- `426004a1e6dc7838e5ddfed9b4da6a1d9564022b` — damage-bounded marquee movement and double-click sole-selection semantics.
- `0eebe8f4ad466749899bacbfd220aac834642770` — marquee release selects through the retained world-space spatial index instead of walking prepared/all model geometry.

SOURCE RESULT SO FAR:

- marquee drag is overlay-only: it performs no selection query and no prepared-geometry rebuild on pointer movement;
- mouse-up converts the marquee to world space and queries the same retained spatial cells used by viewport culling, so selection resolution scales with covered cells rather than total graph size;
- double-click clears prior node/edge selection, selects only the clicked node, then fires `WhenNodeAction`;
- old/new marquee damage is refreshed locally rather than invalidating the whole control;
- existing capture-safe left interaction and capture-free middle pan contracts are preserved.

VALIDATION: source-reviewed only; Windows/U++ build/runtime pending. The prior acceptance remains STOPPED until the remaining corrective items are published and retested.

NEXT ACTION:

1. Finish production paint correction: one antialiased 2px selection overlay for every standard node shape, translucent modern marquee and dirty-paint bounded vector work.
2. Normalize UiGraphDemo style editing to the accepted UiLabel PropertyEditor schema and shared FillRecipe values; fix live colour/fill preview and cancel/commit behavior without parallel style state.
3. Apply a softer deterministic demo palette without changing unrelated global theme semantics.
4. Extend focused graph scale/interaction assertions, review the complete corrective diff, then hand Windows acceptance back to Gary.

## NEXT

1. Complete the active UiGraph corrective above before treating the earlier UiGraph validation gate as resumable.
2. Complete the four-control Windows gates above and source-review any genuinely mechanical platform correction before publishing it.
3. If UiGraph validation passes after the corrective, remove its obsolete remote work branch after confirming ancestry in current `main`.
4. Remove stale temporary/Label branches only after comparing each tip to current `main`; the accidental `DO_NOT_USE` branch contains no useful work and is safe to delete.
5. Continue remaining control normalization from the same documented convention rather than introducing a new parallel schema framework.
