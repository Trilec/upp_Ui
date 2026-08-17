# UI OVERRIDE ROLLOUT — RECOVERY LOG

BASE: `9619f93fd633b0e974c2c270f2b0f3437bc3b2ef` on `upp_Ui/main` before this rollout began.

TASK: normalize UiList, UiBaseEdit, UiDropdown and UiAccordion against the accepted UiLabel PropertyEditor/Designer override convention.

STATUS: **UILIST RUNTIME SOURCE COMPLETE — WINDOWS VALIDATION PENDING.**

PUBLISHED:
- `a53154e19afc2ee654821775373d6a51435e7cc2` — owning List style becomes authoritative for built-in row rendering; stray connector-probe files removed in the same tree checkpoint.

CURRENT CHECKPOINT:
- the built-in UiList row renderer now projects the owning List style instead of independently resolving row presentation from global theme state;
- viewport Skin/shadow/focus remain viewport-owned and are not copied into rows;
- hot/selected face/frame/ink, row radius/padding, stripes and visibility flags are authoritative for the built-in renderer;
- explicit custom-styled item renderers retain renderer ownership;
- List style changes and theme revision changes reset only the bounded renderer pool so unchanged layouts still reuse prepared geometry;
- right-text badge chrome/ink and state underline modes are painted by UiList itself;
- focused `Utilities/UiListStyleContractTest` records the pure style/data projection contract.

TOUCHED:
- `Ui/UiList.cpp`
- `Ui/UiListPaint.cpp`
- `Ui/UiListRender.cpp`
- `Ui/UiListRenderStyle.h`
- `Ui/Ui.upp`
- `Utilities/UiListStyleContractTest/`
- `docs/ACTIVE_WORK_UI_OVERRIDE_ROLLOUT.md`

VALIDATION:
- source/static review complete for style authority and bounded-renderer invalidation path;
- Windows U++ Debug/Release test/build pending;
- existing `UiModelViewPerformanceTest` remains a required regression gate because unchanged List Layout must still report zero renderer relayouts.

NEXT:
1. normalize UiList + UiBaseEdit Designer adapters and add focused adapter tests;
2. normalize UiDropdown + UiAccordion typed Designer adapters;
3. normalize the four demos;
4. hand the accumulated meaningful checkpoints to Gary for Windows validation.

Repository-wide `docs/ACTIVE_WORK.md` remains intact so concurrent UiGraph recovery evidence is not overwritten; this task-specific recovery log is the authority for the four-control override rollout until the work is folded back into the main checkpoint.
