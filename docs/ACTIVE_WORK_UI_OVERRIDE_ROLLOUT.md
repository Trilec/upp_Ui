# UI OVERRIDE ROLLOUT — RECOVERY LOG

BASE: `9619f93fd633b0e974c2c270f2b0f3437bc3b2ef` on `upp_Ui/main` before this rollout began.

TASK: normalize UiList, UiBaseEdit, UiDropdown and UiAccordion against the accepted UiLabel PropertyEditor/Designer override convention.

STATUS: **FOUR CONTROL RUNTIME/ADAPTER SOURCE COMPLETE — DEMO NORMALIZATION + WINDOWS VALIDATION PENDING.**

PUBLISHED UI CHECKPOINTS:
- `a53154e19afc2ee654821775373d6a51435e7cc2` — owning List style becomes authoritative for built-in row rendering; stray connector-probe files removed in the same tree checkpoint.
- `d0579b8753748ca765710f6c29805d2859ddf6aa` — List renderer package membership and focused style contract integration complete.
- current follow-on checkpoint adds `striped_rows` to `UiList::Style::Serialize()` and adds a serialization round-trip assertion; record its exact SHA after publish.

PUBLISHED DESIGNER CHECKPOINTS:
- `Trilec/upp_uidesigner` `c27f499c8d51ad73037d9a60481bb73d870d38a7` — normalized dedicated UiList + UiBaseEdit adapters and focused contract test.
- `Trilec/upp_uidesigner` `ec02f1cbcc040f70ad55e656b98ec64640142cec` — normalized dedicated UiDropdown + UiAccordion adapters, shared legacy Color-to-FillRecipe bridge, focused composite test and coverage docs.

CURRENT UI CONTRACT:
- the built-in UiList row renderer projects the owning List style instead of independently resolving row presentation from global theme state;
- viewport Skin/shadow/focus remain viewport-owned and are not copied into rows;
- hot/selected face/frame/ink, row radius/padding, stripes and visibility flags are authoritative for the built-in renderer;
- explicit custom-styled item renderers retain renderer ownership;
- List style changes and theme revision changes reset only the bounded renderer pool so unchanged layouts still reuse prepared geometry;
- right-text badge chrome/ink and state underline modes are painted by UiList itself;
- `striped_rows` is serialized with the rest of the live List style state;
- focused `Utilities/UiListStyleContractTest` covers style/data projection plus striped-row serialization persistence.

DESIGNER CONTRACT:
- UiList: General, Face, Frame, Ink, Icon, Typography, Content Margin, Focus, Shadow, Highlight, Rows/Layout, Rows/State, Content, Badge, Drag;
- UiBaseEdit: General, Face, Frame, Ink, Typography, Content Margin, Editing, Underline, Whitespace, Focus, Shadow, Highlight;
- UiDropdown: common collapsed-control domains plus nested Popup/Layout, Popup/Face, Popup/Frame, Popup/Items/*, Popup/Marker, Popup/Badge and Drag;
- UiAccordion: outer chrome plus Section, Header/*, Body/*, Behaviour and Animation domains;
- existing serialized field ids are retained where they already existed;
- fields promoted from Color to FillRecipe accept legacy Color values and normalize them to Solid rather than losing authored data;
- image-backed Skin/custom glyph resources remain intentionally deferred until the Designer theme adapter receives document-resource resolution.

TOUCHED UI:
- `Ui/UiList.h`
- `Ui/UiList.cpp`
- `Ui/UiListPaint.cpp`
- `Ui/UiListRender.cpp`
- `Ui/UiListRenderStyle.h`
- `Ui/Ui.upp`
- `Utilities/UiListStyleContractTest/`
- `docs/ACTIVE_WORK_UI_OVERRIDE_ROLLOUT.md`

VALIDATION:
- source/static review complete for List style authority, bounded-renderer invalidation and four Designer adapter contracts;
- Windows U++ Debug/Release test/build pending;
- `Utilities/UiListStyleContractTest`: require `UILIST_STYLE_CONTRACT_SUMMARY checks=13 failed=0` after the persistence checkpoint;
- existing `UiModelViewPerformanceTest`: require 52/0; unchanged second List `Layout()` must still report zero renderer relayouts;
- Designer `ListEditThemeAdapterTest`: require emitted zero-failure summary Debug + Release;
- Designer `DropdownAccordionThemeAdapterTest`: require emitted zero-failure summary Debug + Release.

NEXT:
1. publish and verify the List persistence checkpoint;
2. normalize the UiList, UiBaseEdit, UiDropdown and UiAccordion demos using existing config/state authority rather than adding a parallel demo model;
3. fold this rollout state into repository-wide `docs/ACTIVE_WORK.md` without overwriting the concurrent UiGraph validation record;
4. hand the accumulated meaningful checkpoints to Gary for Windows tests/builds and four GUI smokes.

Repository-wide `docs/ACTIVE_WORK.md` remains intact so concurrent UiGraph recovery evidence is not overwritten; this task-specific recovery log is authoritative for this four-control rollout until the final demo/validation checkpoint is folded back into the main record.
