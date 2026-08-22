# ACTIVE WORK

Remote GitHub is authoritative. Never force-update `main`. Refresh current `main` before work/publication and preserve unrelated concurrent advances.

Detailed older work remains in:
- `docs/ACTIVE_WORK_ARCHIVE_PRE_FOUR_CONTROL_2026-08-17.md`
- `docs/ACTIVE_WORK_UI_OVERRIDE_ROLLOUT.md`
- `docs/12_UI_DEMO_MODERNIZATION_PLAN.md`
- `docs/13_UI_MODEL_MUTATION_SCALE_CONTRACT.md`

## CURRENT SUPERVISORY STATE — 2026-08-22

STATUS: **PROPERTYEDITOR DEMO ALIGNMENT SLICE SOURCE COMPLETE; WINDOWS BUILD + VISUAL ACCEPTANCE PENDING.**

Authoritative branch: `main`.

Current substantive source checkpoint before this documentation commit:

- `9eed37895c6967b830d210e207e590b79e2752a9` — explicit UiButton icon-size/render-mode contract coverage;
- `9f08dd9d2ff7f3806eb45a84c7fc66521d8d03f3` — new combined selectable `UiEditDemo`;
- `bb0b8774bae13dfb9484412ee05768a87592b7de` — `UiSliderDemo` rebuilt on production PropertyEditor;
- `5bf1ef1424def906b926f84fc27faf58f0b22ae3` — `UiCheckBoxDemo` rebuilt on production PropertyEditor.

The PropertyEditor interaction baseline immediately below this slice remains:

- `00502ca0b779187314d72d51fc11fbb16f5ebad2` — Cardinal4 editors preserve canonical caller values;
- `9d89914ef3068bd88b53697a3552051d174efe0d` — focused numeric PropertyEditor editing owns the mouse wheel rather than scrolling the outer PropertyEditor.

No Windows PASS is claimed for the new demo slice yet.

---

## SUPERVISORY OWNERSHIP

The supervisor owns architecture, diagnosis, substantive source changes, source review and publication.

Gary is the Windows validator/helper. He fetches exact requested `main`, builds/runs the focused gates, performs the requested visual smoke and reports exact diagnostics. Mechanical build corrections are acceptable; substantive source/design failures return to the supervisor.

---

## PROPERTYEDITOR DEMO ALIGNMENT CONTRACT

`UiLabelDemo` remains the interaction reference for PropertyEditor behavior. The current demo sweep deliberately converges on the same reusable interaction vocabulary rather than keeping bespoke demo-only property rows.

Shared expectations:

- bounded numeric values use `NumericInt` / `NumericDouble` with the compact value-to-slider affordance;
- a focused numeric editor responds to the mouse wheel without moving the whole PropertyEditor viewport;
- four-way direction choices use the shared Cardinal4 matrix where the domain really is Left / Right / Top / Bottom;
- icon properties use the shared icon chooser;
- color values use the production PropertyEditor color editor;
- demos use an explicit 38% PropertyEditor label/value divider;
- Light/Dark changes update both live preview and PropertyEditor palette;
- the demonstrated production control remains directly interactive.

Do not reintroduce `DemoSliderRow`, `DemoToggleRow` or similar hand-built rows as the authoritative property UI for migrated full demos. Those helpers remain transitional for demos not yet migrated.

---

## UIBUTTON DEMO / CLASS AUDIT

`examples/UiButtonDemo` was re-read against the current `UiButton` implementation and current Label/PropertyEditor interaction contract.

Result: **no substantive Button demo or Button production change was required.**

The existing Button demo already has:

- production PropertyEditor Inspector/Overrides;
- `SetLabelRatio(38)`;
- bounded `AddNumericInt` fields with the shared slider-toggle affordance;
- shared icon chooser;
- Cardinal4 icon-side editing;
- Auto / PreserveColor / MonoTint icon rendering;
- live preview and generated-code page.

The production `UiButton` class already matches the corrected explicit icon sizing contract used by `UiLabel`:

- `SetIconSize(cx, cy)` retains independent explicit dimensions;
- with scale-to-content off, explicit dimensions are treated as the rendered size rather than an aspect-fit box;
- `SetIconScaleToContent()` remains an explicit independent mode;
- render mode remains independent through `SetIconRenderMode()`.

`Utilities/UiButtonInteractionContractTest` now includes explicit non-square icon size, render-mode and scale-to-content assertions in addition to the existing interaction-geometry coverage.

---

## UICHECKBOX DEMO — PROPERTYEDITOR MIGRATED

`examples/UiCheckBoxDemo` has been replaced with a simpler executable-documentation demo using production PropertyEditor rather than the previous bespoke accordion / DemoSliderRow / DemoToggleRow property system.

Current live groups:

- Content — text, checked icon, tri-state icon, marker rendering;
- State — Classic/Chip/List visual, check state, tri-state, enabled;
- Layout — indicator side, size, gap, mark thickness;
- Body — face/frame enablement, radius/frame width and colors;
- Indicator — face/frame enablement, radius/frame width and face/frame/mark colors.

The real `UiCheckBox` is interactive. User toggles are reflected back into the PropertyEditor State value. Icon rows use the shared icon chooser and bounded numerics use the shared slider-toggle interaction.

---

## UISLIDER DEMO — PROPERTYEDITOR MIGRATED

`examples/UiSliderDemo` has been replaced with a production-PropertyEditor reference demo.

Current live groups:

- Value — direction, minimum, maximum, value, step, enabled;
- Ticks — show ticks, major/minor counts, Cardinal4 tick side, lengths/gap/color;
- Geometry — track and thumb dimensions plus track expansion;
- Track — face/frame state, radius/frame width, track and active colors;
- Thumb — face/frame state, radius/frame width and colors.

Dragging or wheeling the real `UiSlider` updates the PropertyEditor Value row. Minimum/maximum/value are normalized coherently in the demo projection rather than allowing an invalid range to create parallel state.

---

## COMBINED UI EDIT FAMILY DEMO

A new canonical package now exists:

```text
examples/UiEditDemo
```

It presents the related edit family together:

- `UiLineEdit`;
- `UiPasswordEdit`;
- `UiMaskEdit`;
- `UiMultiEdit`.

All four are visible in one preview. A compact selector chooses which control the single PropertyEditor is currently describing. This is intentionally similar to Theme Studio selection: shared properties remain shared, while only properties meaningful to the selected subtype are projected.

Common selected-control groups include:

- Content / Behaviour;
- Face / Frame / Ink;
- Typography / Content Margin;
- Editing;
- Underline.

Subtype-only rows:

- Password — mask character, plain-text visibility, eye/visibility control;
- Mask — mask, prompt, validator, formatter, invalid-state display;
- Multi-line — tabs and whitespace visualization.

Each subtype keeps its own configuration when selection changes. The PropertyEditor therefore does not show irrelevant Password/Mask/Multi rows on every edit while still demonstrating the common `UiBaseEdit` contract in one place.

The older specialized edit demo packages remain in the repository for now. `UiEditDemo` is the new combined reference; do not delete the older packages until the combined demo is Windows/visual accepted and any uniquely useful fixture behavior has been checked.

This is an explicit bounded manager-approved exception to the older demo-plan ordering: the edit family was brought forward because its shared `UiBaseEdit` ownership makes a combined selection demo materially cleaner than four separate new property architectures.

---

## CURRENT WINDOWS VALIDATION GATE

Validate the **current final `main`**, not an earlier source checkpoint.

1. Fetch/report exact `upp_Ui/main` SHA.
2. `Utilities/PropertyEditorV1RunTests` Debug + Release — require `Fails: 0`.
3. `Utilities/UiButtonInteractionContractTest` Debug + Release — require `Fails: 0`.
4. Build `UiButtonDemo` Debug + Release.
5. Build `UiCheckBoxDemo` Debug + Release.
6. Build `UiSliderDemo` Debug + Release.
7. Build `UiEditDemo` Debug + Release.
8. Launch the four demos for the focused visual/interaction smoke below.

Smoke:

- Button — numeric value↔slider affordance, focused numeric mouse wheel, Cardinal4 icon side, icon chooser, exact non-square icon size and Auto/PreserveColor/MonoTint.
- CheckBox — PropertyEditor is the authoritative right rail; Classic/Chip/List and two/tri-state behavior work; clicking the checkbox updates State; shared icon/color/numeric editors work.
- Slider — drag/wheel updates Value; horizontal/vertical and tick-side matrix work; bounded numeric slider toggles work; Track/Thumb styling responds live.
- Edit — selector swaps one PropertyEditor among Single-line/Password/Mask/Multi-line while retaining each sample's values; shared style/editing fields affect the selected sample; password eye/plain-text, mask validator/formatter/error state and multi-line tabs/whitespace behave coherently.

Leave `UiEditDemo` running for Curt to inspect/close after the smoke.

If a substantive failure occurs, stop and report exact SHA/configuration/diagnostic/reproduction. Do not redesign the demos during validation.

---

## OTHER PENDING LINES — PRESERVE, DO NOT CONFLATE

The following older published work remains independently pending Windows acceptance and is not superseded by the demo slice:

- PropertyEditor color-transfer / UiDesigner Theme Builder integration;
- shared-model audit / large List-Gallery-Dropdown/Tree scale gates;
- Graph final revalidation;
- SymbolPicker/Gallery convergence.

See the archive/plan documents above for the detailed checkpoints and gates. Do not move those production subsystems underneath this demo validation unless a concrete shared failure requires it.

---

## NEXT AFTER DEMO ACCEPTANCE

Continue the demo modernization in small related groups, using the accepted PropertyEditor interaction contract rather than a new demo framework. Likely next candidates are Toggle/Radio and RangeSlider/ProgressBar, followed by the model-backed demos once their Data-page contract is deliberately handled.
