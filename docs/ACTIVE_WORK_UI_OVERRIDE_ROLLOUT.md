# UI OVERRIDE ROLLOUT — RECOVERY LOG

BASE: `9619f93fd633b0e974c2c270f2b0f3437bc3b2ef` on `upp_Ui/main`.

TASK: normalize UiList, UiBaseEdit, UiDropdown and UiAccordion against the accepted UiLabel PropertyEditor/Designer override convention.

STATUS: LIST RUNTIME AUTHORITY FIX IN PROGRESS — PLATFORM VALIDATION PENDING.

CURRENT FINDING:
- the built-in UiList row renderer was resolving row presentation independently from the owning List style;
- this made custom List row fields such as hot/selected face/frame/ink, row radius/padding, stripes and visibility flags non-authoritative;
- fix direction is to project the owning List style into the built-in renderer while preserving explicitly custom-styled renderers and viewport/row style separation.

TOUCHED SO FAR:
- `Ui/UiListRenderStyle.h`
- `Utilities/UiListStyleContractTest/`

NEXT:
1. complete UiList render/paint integration and verify bounded renderer reuse;
2. publish/review the complete List runtime checkpoint;
3. implement List + UiBaseEdit Designer normalization;
4. implement UiDropdown + UiAccordion typed Designer normalization;
5. normalize the four demos and hand Windows validation to Gary.

Note: `docs/ACTIVE_WORK.md` remains the repository-wide recovery record; this task-specific log exists so the active four-control rollout can checkpoint without overwriting concurrent UiGraph recovery evidence.
