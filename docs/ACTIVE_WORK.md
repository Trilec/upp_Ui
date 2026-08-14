# ACTIVE WORK

BASE: `a0b07fad20c034dd5096335af591a971f9857f0a` — verified live `main` at R2D start; complete published R2C Tree/Table implementation checkpoint.

TASK: `UI-MODEL-RENDERING-R2D` — converge Dropdown and Menu on the shared model/render architecture.

AUTHORITATIVE PLAN: `docs/07_UI_MODEL_RENDERING_PLAN.md`. Remote GitHub is authoritative. No compatibility/shim requirement; preserve one model authority and one presentation authority. Never force-update `main`.

PUBLISHED FOUNDATION:
- R2A: `UiItemRenderData`, non-Ctrl `UiItemRender`, Basic/Image, dirty-gated prepared layout, theme/style integration.
- R2B: List + Gallery bounded renderer pools, shared 10k demo, Gallery zoom/marquee, 100k deterministic scale acceptance.
- R2C substantive checkpoint: `0c6b63287e4a76750cbaf6afda4c7673cde91b77`; recovery log: `a0b07fad20c034dd5096335af591a971f9857f0a`. Table has direct 100k-row/variable-column geometry plus cell/header renderer pools. Tree has direct visible-row lookup, bounded primary/column renderer pools and 100k-node acceptance.

WINDOWS EVIDENCE RECEIVED:
- Gary tested R2A/R2B at `63f4a6498fca55b47489871e60ab90a2cda4865b`: Ui Debug source compile PASS, 0 warnings/errors; `UiModelViewPerformanceTest` Debug/Release **41/41 PASS**; `UiGalleryDemo` Release builds/launches.
- Therefore the combined R2C `UiModelViewPerformanceTest` target is **52 checks**, not the stale 50 previously recorded: 41 accepted pre-Table checks + 11 Table checks. `UiTreeScaleTest` target remains **11 checks**.
- Gallery acceptance also found a real R2B defect outside R2D scope: `UiGallery::CancelMode()` recursively calls `ReleaseCapture()` during capture teardown, producing Win32 stack overflow. Gallery dark-theme/marquee visibility/zoom-text issues are separately recorded in chat and must be corrected before final renderer-family closure.

R2D AUDIT AT BASE:
- `UiDropdown` binds `UiListModel` but also mirrors every row into `Vector<UiDropdown::Item> items_`, with `ToModelItem`, `FromModelItem`, `SyncItemsFromModel`, mutable `GetItem()` and a competing `WhenPaintItem` popup paint authority. This violates one-authoritative-state and shared-renderer goals.
- Dropdown popup already paints only visible rows, but row content layout/paint is one-off and `PopupWindow::Paint()` calls scrollbar-state synchronization. R2D should make popup geometry/render preparation happen outside Paint.
- `UiMenuModel` is already authoritative and domain-specific. `UiMenu` still manually lays out icon/text/description/right content for each row. Check/radio/shortcut/submenu/command semantics correctly remain Menu-owned.
- Existing external `UiDropdown::Item` callers are limited to `examples/UiDropdownDemo` and PropertyEditor sources and can be migrated directly to `UiModelItem`; no compatibility alias is required.

R2D TARGET — DROPDOWN:
- remove `UiDropdown::Item` and `items_` mirror entirely;
- make `UiListModel` the sole row authority for both internal and external models;
- expose/add/mutate `UiModelItem` directly through explicit model-backed APIs; remove mutable direct item references and `RefreshFromModel` mirror semantics;
- replace popup `WhenPaintItem` authority with `SetItemRender(const UiItemRender&)` and a bounded visible popup renderer pool;
- default popup renderer is theme-aware `UiItemRenderBasic` configured for Dropdown row content; Dropdown retains selection/check markers, group/separator, drag handle/reorder, popup chrome and scroll semantics;
- collapsed face may reuse prepared renderer content where it simplifies icon/text presentation, while Dropdown keeps indicator and multi-select badge chrome;
- renderer layout/scrollbar geometry must be prepared outside Paint.

R2D TARGET — MENU:
- add `UiMakeItemRenderData(const UiMenuItem&, ...)` shared adapter;
- add default/custom Menu content renderer slot(s), using bounded popup renderer instances and prepared geometry outside Paint;
- renderer handles ordinary icon/title/description/right-content composition;
- Menu keeps check/radio glyphs, submenu arrow, separator, shortcut/command interaction, popup stack, hot/pressed backgrounds and bar/session semantics;
- model remains `UiMenuModel`; do not force Menu into `UiListModel` or `UiModelItem` inheritance.

IMPLEMENTATION RHYTHM:
1. Dropdown model-authority/public API migration + caller migration; publish and verify.
2. Dropdown popup renderer pool/lifecycle + deterministic tests; publish and verify.
3. Menu shared render adapter/pool + tests; publish and verify.
4. Update model-render docs and this recovery log; static diff/package review; Windows validation handoff.

STATUS: **R2D IN PROGRESS — DROPDOWN FIRST.**

NEXT: rewrite `UiDropdown` around direct `UiListModel`, migrate current `UiDropdown::Item` callers to `UiModelItem`, then publish the first recoverable R2D checkpoint before Menu changes.
