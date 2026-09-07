# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; never force-update `main`.
Recovery state only; Git history is implementation history.

BASE: `e0d27500a7273e062bc97029f7b9088efaf17255`
TASK: **Close PropertyEditor first-wheel override regression, then finish final Windows acceptance**
TOUCHED:
- `Utilities/PropertyEditor/PropertyValueEditors.h`
- `Utilities/PropertyEditor/PropertyValueEditors.cpp`
- `Utilities/PropertyEditor/PropertyEditorInteraction.cpp`
- `Utilities/PropertyEditorTests/main.cpp`
- `docs/ACTIVE_WORK.md`
STATUS: **SOURCE REPAIR PUBLISHED — WINDOWS VALIDATION PENDING**
PUBLISHED: this commit (PropertyEditor active-value wheel ownership repair)
VALIDATION: source/diff review complete; Windows CLANGx64 focused/full gate pending.
NEXT ACTION: run PropertyEditor focused gates first; if green, resume the complete final Windows gate.

## CURRENT REPAIR

Inactive override body/value clicks still request `WhenOverride(id, true)` before editor activation.
The host remains authoritative for override/inheritance state, and the explicit override action
remains the independent on/off toggle.

Wheel ownership is now local and deterministic:
- only the active value rectangle can hand a wheel gesture to its active value editor;
- the concrete value editor owns wheel semantics;
- Integer/Double/NumericInt/NumericDouble forward to their real `UiIntEdit`,
  `UiFloatEdit` or active `UiSlider`;
- PropertyEditor scrolling remains the fallback outside the active value surface;
- global focus state is no longer used to decide ownership across synchronous model refresh.

Regression coverage verifies both sides: first wheel after inactive numeric override activation
edits the value, while a wheel outside the active numeric value surface does not.

Gary's prior mechanical UiGraph compile repair at `e0d27500...` is preserved:
`Ui/UiGraph/UiNodeGraphPerformance.inc` uses `Array<StyledMetrics>` for stable storage.

## REQUIRED WINDOWS GATE

1. `Utilities/PropertyEditorTests`
2. `Utilities/PropertyEditorOverrideCommitTest`
3. If both pass, UiDesigner focused RC sequence:
   `Tests`, `RegressionTests`, `FoundationTests`, `ExportedThemeContractTest`.
4. If all focused tests pass, run UiDesigner `RunSupervisorValidation.ps1` completely.
5. Then complete the existing manual/generated Theme fidelity checks before RC closure.

Also retain the final upp_Ui acceptance items already established:
- UiGraphScaleTests and UiNodeGraphPerformanceTest;
- UiGraphRenderTests, UiGraphViewTests and UiGraphTest;
- UiChartRingDemo/UiProgressRingDemo override click -> immediate tick -> first wheel edits value;
- UiGraphDemo Reference + 10k visuals/interactions/LOD;
- 10k `micro_rasters > 0 && <= 32`;
- same-machine 10k/Reference phase timings and idle checks;
- Reference -> 10k -> Reference -> 10k remains clean;
- `git diff --check` PASS; final tree clean.

## CONTRACTS TO PRESERVE

- generated explicit curves target 0.35 final-device-pixel positional error inside the supported
  `TessellationStatus` envelope;
- direct Draw/native Painter first; shared exact raster cache for stable repeated AA;
- Graph may use `UiGeometry` directly for dense final-pixel geometry;
- semantic positions never depend on tessellation vertex index;
- one retained world broad phase remains authoritative;
- static views eventually become idle;
- reusable PropertyEditor defects are fixed in `upp_Ui`, never through Designer workarounds.

## BRANCH STATE

Single authoritative branch: `main`.

## CANONICAL DOCS

`00` Coding · `01` Controls · `02` Theme · `03` Model · `04` Demo ·
`05` PropertyEditor · `06` Large-scale Views & LOD · `07` Drawing & Geometry ·
`08` UiGraph · `09` UiDoc
