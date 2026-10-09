# Shared project fonts and typography

Implemented on 2026-10-07 in Ui and UiDesigner. This is the public handoff for the
Font Picker: the picker should reuse this loader and the existing theme/property
editor infrastructure.

## Public contract

- `Ui/UiFonts.h`: `UiFontCatalog`, `UiFontAsset`, `UiFontResolution`,
  `UiTypography`, `UiFonts`, `UiApplyTypography`.
- `Ui/UiTheme.h`: `UiTheme::SetTypography` / `GetTypography`; existing role
  resolvers and `GetRevision` include typography.
- Designer `UiDesigner/Fonts/UiDesignerFonts.h`: `UiDesignerImportFont`,
  `UiDesignerLoadFonts`, `UiDesignerActivateFonts`, `UiDesignerTypography`.
- Designer `UiDesignerSession::ImportProjectFont`: authoring import through the
  existing document history, with Undo/Redo and unsaved-project tracking.

The minimal snippet uses BRC entries `BINARY(pt_regular,
"fonts/PT_Sans-Web-Regular.ttf")` and `BINARY(serif_regular,
"fonts/PT_Serif-Web-Regular.ttf")`.

Catalogue and typography mutations belong on the GUI thread. Import/validate
before controls or in an explicit authoring operation, never in Paint.

```cpp
#include <Ui/Ui.h>
#include "Fonts.brc" // BINARY entries pt_regular and serif_regular, as below.

UiFontCatalog fonts;
auto status = fonts.Import("pt-regular", "pt-sans",
    String((const char*)pt_regular, pt_regular_length),
    "PT Sans Regular", "SIL OFL 1.1",
    "9cc831490532009bae2b3ce0d39c62adfc889060beb421593bfd9d2396d0f10a");
fonts.Import("serif-regular", "pt-serif",
    String((const char*)serif_regular, serif_regular_length), "PT Serif Regular", "SIL OFL 1.1");
// Import the real Bold, Italic and BoldItalic files under this same family ID.
UiFonts::SetCatalog(fonts);
UiTypography typography;
typography.body = "project:pt-sans";
typography.heading = "project:pt-serif";
typography.code = "system:monospace";
typography.fallback = "STDFONT"; // A project family is also allowed.
UiTheme::SetTypography(typography); // Before constructing the application UI.

UiButton button; // Its ordinary theme style inherits Body without a color snapshot.
auto resolved = UiFonts::Resolve("project:pt-sans", SansSerifZ(11).Bold());
// Inspect requested/status/diagnostic/style_fallback, not just the returned Font.
auto local = UiTheme::ResolveButton();
String requested = "project:pt-serif"; // Persist this selection, including when missing.
UiFonts::ApplySelection(local.font, requested);
button.SetCustomStyle(local); // Explicit local style, including its existing colors.
```

`project:<family-id>` selects the asset family; each face has its own stable asset
ID and SHA-256 content identity. `system:<name>` is an explicitly optional system
source. Plain family names remain legacy system selections, including U++'s
case/punctuation lookup. They are not silently migrated to project assets.
Selecting a Project entry makes that migration explicit. Runtime face indices and
private `UiP...` aliases must never become authored project identity; use the
requested string and `UiFonts::Selection` for editor round trips.

`Resolve` preserves prototype pixel height, traits and decoration. It selects
real regular/bold/italic faces where present. Missing styles report `Fallback`
and `style_fallback`, rather than claiming a synthetic face is genuine. Missing,
unsupported, invalid and resource-limit assets retain their identity and chosen
fallback. `glyph_policy=NativeFallback` is separate: an unavailable glyph does
not make its otherwise loaded family a missing asset.

`UiFonts::Normalize` resolves genuine variants after changing Bold/Italic on a
registered Font. `UiApplyTypography(style, false)` normalizes explicit/nested
fonts; its default argument inherits existing builtin family fields centrally.
Existing `style.font`, `metrics.text_font`, sizes, local full-style snapshots and
intentional system/monospace overrides retain their contracts. Heading defaults
inherit Body when Heading is empty; an empty Code selection preserves monospace.
Font height is already pixels after U++ DPI conversion; resolving a family does
not scale it again.

## Ownership and supported adapter

Windows GDI private memory registration is implemented and tested. Original
redistributed files remain intact. The runtime prepares a checksum-correct SFNT
with a content-derived private family alias, registers it with
`AddFontMemResourceEx`, and appends an immutable U++ face. It never installs fonts
on the machine, reuses an index for different bytes, or downloads at runtime.

The adapter supports static TrueType outlines (SFNT 0x00010000), including an OTF
file with those outlines. CFF OTF, TTC collections, variable/color fonts and
name-table formats other than zero report Unsupported. Malformed/bounds-invalid
containers report Invalid; OS/2 embedding restrictions are checked. Importers
still supply and own the actual redistribution licence.

Registration bytes and private handles live for the process lifetime. Content
hashes deduplicate registrations across catalogues/project switches. Limits are
128 unique faces, 64 MiB retained registration data and 16 MiB per input face;
limits produce diagnostics instead of evicting a face still in use. Old controls
and queued Font values remain valid after a project switch. This deliberately
trades reclamation of unused faces for stable identity within a bounded process.
`RegisteredFaceCount` and `RetainedBytes` expose that budget.

Cocoa/Core Text, Fontconfig/FreeType, custom font backends and browser hosts have
no loader adapter here yet. They report Unsupported. This Windows acceptance is
not platform parity, a shaping implementation, or a GPU text qualification.

## Designer authoring, persistence and export

Load menu > **Import Project fonts...** selects one or more font files plus a
licence text to copy into the project. Theme Studio's existing inspector exposes
Body / Heading / Code. Local `property.font` overrides use the same catalogue.
Project entries are the initial source when available; **Show system fonts...**
adds clearly labelled System entries. Existing selectors follow catalogue revision
without reopening the editor. Missing choices remain visible with their fallback.

The existing project resource array stores `resource_type="font"`, original
bytes using the existing resource encoding, original filename and metadata:
`font_asset_id`, `font_family_id`, `font_sha256`, `font_family`, `font_face`,
`font_bold`, `font_italic`, `font_license`, `font_source`. A root `font_manifest`
array can retain unavailable asset records (`asset_id`, `family_id`, `source`).
The theme's additive `typography` map persists `body_font`, `heading_font`,
`code_font`, and optional `fallback_font`. Legacy projects lacking that map load
unchanged. Existing theme recipes and local font fields store stable selections.

A C++ export packages only selected families, including all of their genuine
style files. Actual selected theme recipes/local document font fields participate;
ordinary text is not interpreted as a font selection. Generated files include:

- `<Class>.fonts.brc`, `fonts/<sha256>.ttf` and copied licence text files;
- `fonts/manifest.json` with schema 1, assets and requested typography;
- a first generated data member `FontBootstrap font_bootstrap_`, whose constructor
  loads BRC bytes and sets the catalogue/roles before the member controls are
  constructed or laid out;
- `UiFonts::ApplySelection` for generated local/nested font assignments.

The bootstrap also works for component exports, without modifying a user-owned
main.cpp or depending on the current directory. Exported original font bytes and
hashes are checked; a missing selection emits `DeclareMissing` and retains its
fallback/requested identity. Project fonts come from copied resources, not source
paths, on reload and startup. Assistant `list_fonts` returns objects with
`selection`, `label`, and `source`; its validation and theme authoring accept the
same project/system selections.

## Explicit-family audit

All 19 files in the supplied source inventory were reviewed:

| Files | Treatment |
| --- | --- |
| UiTheme.h, UiThemeResolverImpl.h | Central Body/Heading/Code inheritance; fresh resolvers retain palette inheritance. |
| UiAccordion.cpp, UiBreadcrumbs.h, UiCheckBox.cpp, UiRadioButton.cpp, UiSplitter.cpp, UiMediaCard.h, UiGroupPanel.h, UiTitleCard.h | Existing builtin family/size defaults preserved; resolved/nested theme fields receive roles. Explicit custom styles remain explicit. |
| UiBaseEdit.cpp, UiButton.cpp, UiGroupPanel.cpp | Genuine face normalization for custom styles; edit caret uses the same effective text font as painting and warms measurement before Paint. |
| UiDoc.cpp, UiDocPaint.cpp, UiDocPaintOverlay.cpp, UiDocMetadataPrivate.h | Body/Heading/Code and stable local selection resolution; typography revision invalidates glyph/paragraph/caret caches. Removed double DPI conversion at reviewed annotation/code sites. |
| UiColorPicker/UiColorPicker.cpp | Direct small labels inherit Body; the hexadecimal footer keeps the Code role. Only additive font-expression changes, preserving concurrent color-picker edits. |
| UiFileBrowser/UiFileBrowserStyle.cpp | Concurrent untracked browser work belongs to CineView. Its inherited GetStdFont path and monospace preview integration were identified and explicitly handed to that owner; caller SetFont overrides must stay explicit. That owner has applied the inherited Body/Code resolver in the current browser work; it is not included in this font commit or its isolated acceptance. |

Also fixed label minimum-size cache synchronization, open-popup renderer revision synchronization, item emphasis normalization,
and Dark document page/caret/highlight colors paired with its resolved text.

## Renderer boundary and Font Picker obligations

`upp_render/render/RenderGpu2D/RenderGpu2DText.cpp` was reviewed, unchanged. Its
current glyph key is `Font::AsInt64` plus character. Appended immutable faces give
new assets new keys; old registered faces stay alive for queued frames. Typography
revision invalidates Ui measurement/layout, not the identity of old GPU atlas
entries. Existing renderer atlas budgets/reuse still belong to the renderer. The registration budget excludes OS font/cache allocations and Designer project/Undo storage; it is not a whole-process memory bound.

Native per-character glyph fallback is the only glyph policy supplied here.
HarfBuzz/FreeType shaping, kerning/ligatures, contextual scripts, bidi and cluster
caret/selection semantics require a later common text authority and separate
acceptance; this task does not implement them. No renderer/backend sources changed.

The Font Picker must keep requested IDs when assets are missing, show diagnostic
and resolved fallback, refresh lists on `UiFonts::GetRevision`, and distinguish
Project/System. Use the catalogue's actual metadata/style availability; never
infer deployment from an installed family name. Route Designer imports through
its session history; retain licences/resources in the authoring model. Do not
load per tile/Paint, register a font every preview frame, reuse raw face indices,
or add an independent theme/layout authority.

## Validation and runnable artifacts

See `examples/UiTypographyDemo/README.md` for reproducible build/run commands.
The example bundles PT Sans and PT Serif, each Regular/Bold/Italic/BoldItalic,
from the [Google Fonts PT Sans source](https://github.com/google/fonts/tree/main/ofl/ptsans)
and [PT Serif source](https://github.com/google/fonts/tree/main/ofl/ptserif), with
original SIL OFL licence texts and file hashes in its manifest.

Current evidence lives in `build/font-work` (local build artifacts, not source):

- `UiTypographyDemo.exe`, `ui-tests.txt`: 83 checks, zero failures. Native name/OS/2
  tables prove private/genuine faces; late import/project switch/missing/restored,
  local/nested fonts, an open popup, label measurement, caret, document wrapping, colors and warm
  resolves are exercised. Eight immutable registrations retain 3,137,980 bytes;
  repeated imports and 1,000 warm resolves do not increase that budget.
- `rendered/typography-{light,dark}-{sans,serif}-{850,1200}.png`: eight native
  control renders at the current display DPI and two window widths, inspected for
  family/contrast/layout. This is not a real multi-monitor DPI transition test.
- `DesignerAcceptance/project.uidesign.json` and `DesignerAcceptance/PackagedFonts`:
  actual imported-resource save/load and generated packaged-export fixture.
- `ExportFontRuntime.exe`, `export-runtime.txt`: fresh-process generated-source
  compile/run checks, with zero private registrations before construction.
- `ProjectFontTest.exe`, `DesignerAcceptance/results.txt`: Designer import/history,
  roles, actual preview, local override, roundtrip, selected-only export, missing
  export and legacy compatibility checks.

Final focused acceptance: UiTypographyDemo Release/BLITZ and Debug/no-BLITZ each
pass 83 checks; ProjectFontTest passes 43; AssistantDesignerTests passes 318;
PropertyEditorTests passes 143; UiThemeTests passes 1,092 structure checks and
13 surface checks; fresh-process generated export passes 9. Ui Release
and Designer font tests/export were rebuilt against the staged Ui snapshot, without
concurrent/unrelated Ui changes. The full Designer Release application builds.

Local distributables: `bin/windows-x64/UiTypographyDemo.exe` in Ui and
`bin/UiDesigner.exe` in Designer. Logs, tests, caches and export fixtures stay in
`build/font-work`; they are not added as duplicate demos or committed build outputs.
Real monitor-scale transitions, OS adapter parity and shaped/complex-script text
remain separate acceptance gates. Compilation alone is not their acceptance.
