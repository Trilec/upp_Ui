# ACTIVE WORK

BASE: `a0b07fad20c034dd5096335af591a971f9857f0a` — verified live `main` at R2D start; complete published R2C Tree/Table implementation checkpoint.

TASK: `UI-MODEL-RENDERING-R2D` — converge Dropdown and Menu on the shared model/render architecture.

AUTHORITATIVE ARCHITECTURE: `docs/07_UI_MODEL_RENDERING_PLAN.md`. R2D implementation contract: `docs/08_UI_MODEL_RENDERING_R2D.md`. Remote GitHub is authoritative. Never force-update `main`.

PRIOR WINDOWS EVIDENCE:
- Gary tested R2A/R2B at `63f4a6498fca55b47489871e60ab90a2cda4865b`: Ui Debug source compile PASS, 0 warnings/errors; `UiModelViewPerformanceTest` Debug/Release **41/41 PASS**; Gallery demo Release builds/launches.
- R2C adds 11 Table checks, so current combined `UiModelViewPerformanceTest` target is **52 checks**, not the stale 50 previously recorded. `UiTreeScaleTest` remains **11 checks**.
- Gary's R2A/R2B Gallery acceptance found a real separate defect: `UiGallery::CancelMode()` calls `ReleaseCapture()` during capture teardown, causing recursive `CancelMode -> ReleaseCapture -> CancelMode` and Win32 stack overflow. Gallery dark-surface, marquee visibility/selection-frame and zoom-text issues are also still open. R2D does not claim those fixed.

R2D DROPDOWN — PUBLISHED:
- `7bdd240a678778f8f4170e84de14e1b4dfa3e9c9` — R2D recovery/audit start.
- `8a3198c21f4bdccd4c091d29168e7a66867f1118` — new direct-model/render public contract.
- `caa283ef2bf2125465bbcda68ca38e24640f1f11` — removes parallel `Vector<Item>` mirror and model conversion/sync authority; direct `UiListModel` operations plus collapsed shared renderer.
- `96cc9414d4e868a26c8563135c41371e6d92e83c` — bounded visible popup renderer pool, popup geometry/renderer preparation outside Paint, direct-model popup selection/check/drag behavior.
- `d9b38fc61c7511e00e6e3f1de74ff20e2c155223` — package includes `UiDropdownPopup.cpp`.
- `ed5feccd6bcac39ef21979ce2a7dab4277b4737c` — `UiDropdown::Item` retained only as a direct `using Item = UiModelItem` spelling so existing demo/PropertyEditor source remains source-compatible without a second object type or state. Primary API/documentation uses `UiModelItem`.

R2D DROPDOWN — RESULT:
- `UiListModel` is the sole item-state authority for internal and external models.
- No `items_`, `SyncItemsFromModel`, `ToModelItem`, `FromModelItem` or `RefreshFromModel` mirror path remains.
- No competing Dropdown item-paint callback remains; `SetItemRender(const UiItemRender&)` is the item-presentation extension point.
- collapsed face uses one prepared renderer instance; popup keeps only visible-row renderer instances.
- default renderer is content-only `UiItemRenderBasic` configured from Dropdown style/theme.
- Dropdown retains popup surface, group/separator, selection/check marker, badge, indicator, typeahead, drag handle/reorder and popup lifetime semantics.
- popup scrollbar/visible geometry and renderer layout are prepared from Layout/scroll/model/style paths, not from popup Paint.

R2D MENU — PUBLISHED:
- `f0c2bc683cbaa3a8dadeef32eba073c44d84c7a3` + `58df0ac8ffa23c98cdf0e178feec97ddcd85ea4a` — shared `UiMenuItem` -> `UiItemRenderData` adapter contract/implementation.
- `972cbf349feb0fd550448460c1df8f1334f3bd29` — Menu popup shared-render public/pool contract.
- `03bfb16474ba2be459857e69e550091f96ceba1f` — popup ordinary content uses bounded prepared shared renderers while Menu keeps domain chrome and command/session behavior.

R2D MENU — RESULT:
- `UiMenuModel` remains the authoritative domain model; Menu is not forced into `UiListModel` or `UiModelItem` inheritance.
- renderer handles popup icon/title/optional description/shortcut-or-right text/default-item emphasis.
- Menu retains check/radio glyphs, submenu arrow, separator, hot/pressed popup chrome, top menu bar, command activation/request-first mutation, popup stack/session/focus semantics.
- each open popup level owns only a visible-row renderer pool; a closed menu allocates no per-model renderer objects.
- popup Paint consumes prepared renderer geometry and does not prepare renderer layout.

R2D TEST/DOC CHECKPOINTS:
- `fe9ff5771da5ee4b061952dd70105ccd8ded56b5` + `e01d14be4c1149b208ab032ff2783af882c06e61` — focused `Utilities/UiDropdownMenuRenderTest` package and initial 11-check contract test.
- `b67553ebd2d7bf30a48931effab2eac965fdc95d` — test made RTTI-independent while preserving 11 checks.
- `01d3dc82c5a86fcf4a251fc6bcdd8e9dd0ee3e4a` — public R2D implementation documentation.

DETERMINISTIC TARGETS AFTER R2D:
- `Utilities/UiModelViewPerformanceTest`: **52 checks**, expected `Checks: 52, Fails: 0` Debug/Release.
- `Utilities/UiTreeScaleTest`: **11 checks**, expected `Checks: 11, Fails: 0` Debug/Release.
- `Utilities/UiDropdownMenuRenderTest`: **11 checks**, expected `Checks: 11, Fails: 0` Debug/Release.

R2D STATIC REVIEW:
- compare R2C base `a0b07fad...` -> substantive R2D doc checkpoint `01d3dc82...`: R2D is ahead-only; touched slice is limited to Dropdown/Menu, shared item-render adapter, `Ui.upp`, focused tests and R2D docs/recovery state.
- `Ui.upp` includes `UiDropdownPopup.cpp`.
- Dropdown mirror/helper searches return no `items_`, `SyncItemsFromModel`, `ToModelItem`, `FromModelItem`, `RefreshFromModel` or Dropdown `WhenPaintItem` authority.
- renderer pool ownership uses `Array` + `One<UiItemRender>`; no renderer per logical item is stored.
- no Windows/U++ compile/runtime result is claimed for R2C/R2D from this implementation environment.

TOUCHED R2D SLICE: `Ui/UiDropdown.h`, `Ui/UiDropdown.cpp`, `Ui/UiDropdownPopup.cpp`, `Ui/UiMenu.h`, `Ui/UiMenu.cpp`, `Ui/UiItemRender.h`, `Ui/UiItemRenderData.cpp`, `Ui/Ui.upp`, `Utilities/UiDropdownMenuRenderTest/*`, `docs/08_UI_MODEL_RENDERING_R2D.md`, `docs/ACTIVE_WORK.md`.

STATUS: **R2D DROPDOWN + MENU IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING.**

NEXT ACTION: verify final remote `main`, then Windows-validate Ui Debug source compile, the three deterministic suites (52/0, 11/0, 11/0), `UiDropdownDemo` and `UiMenuDemo` build/runtime behavior, including Dropdown popup scrolling/selection/multi-check/drag reorder and Menu check/radio/submenu/keyboard/theme behavior. Tiny mechanical compile fixes are Gary scope; substantive model/render/view issues return to implementation. Separately, fix the known Gallery capture recursion and visual/theme/zoom issues before renderer-family closure.
