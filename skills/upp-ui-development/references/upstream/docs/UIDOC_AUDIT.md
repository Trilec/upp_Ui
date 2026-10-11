# UiDoc audit and screenplay acceptance

Audit date: 2026-10-11. Source baseline: `b09b06f` on main. This is a focused
Windows document/screenplay audit, not whole-library or cross-platform certification.
Implementation: [UiDocCore](../Ui/UiDoc/UiDocCore.h),
[UiDoc](../Ui/UiDoc/UiDoc.h), [UiScreenplay](../Ui/UiScreenplay.h).
Public contracts: [Models](03_UI_MODEL_GUIDE.md), [Screenplay](SCREENPLAY.md).

## Findings and implemented repairs

| Finding | Repair and regression |
| --- | --- |
| View callbacks accumulated on previously bound cores | Removable core observer tokens; disconnect on rebind/destruction; shared-model and destruction/rebinding tests |
| Reentrant transactions could deliver position maps out of order | Queue notifications; bound observers precede the public event; ordered shared-view tests |
| Editor methods continued after callbacks destroyed/rebound them | Lifetime/binding guards and callback copies; typing, navigation, selection deletion and image lifecycle regressions |
| Undo/redo restored text without remapping independent views | Inverse/forward position maps; shared-view caret/selection regressions |
| Non-history structural edits retained incompatible history | Clear that history; rebase presentation flags in retained snapshots |
| Failed compound edits could leave dangling resource references | Validate final state and atomically restore text/collections on refusal; stale AI revision rejection |
| Collection snapshots repeated for every operation | One before/after snapshot per transaction; no after snapshot when history is disabled |
| Valid metadata title/body/merge values appeared empty | Replace numeric ValueMap key lookups with GetValue(index); geometry and payload regressions |
| Whole-document clear retained obsolete cached paragraph bounds | Full deletion map and layout reset; Debug assertion and cleared-search geometry regression |
| Imports could throw on malformed typed fields or silently clip style ranges | Parse into a candidate inside an exception boundary, validate before publishing; malformed import regressions |
| Saves wrote over the destination directly | Unique adjacent temporary file and native replacement; overwrite/failure-preserves-old-file tests |
| Explicit/imported numeric IDs could collide or overflow later allocation | Bounded suffix parsing, UUID fallback at counter exhaustion, collision checks; allocation regressions |
| Typed image dimensions could fail inside view layout | Validate numeric dimension envelopes before publishing; malformed embed rollback test |
| All paragraph measurements were discarded after ordinary text edits | Map and reuse unaffected caches, binary style lookup and bounded glyph cache; one rebuild for an edit in 1,000 paragraphs |
| Screenplay Enter across mixed elements depended on selection direction | Use the first selected paragraph's element, identity and metadata; forward/reverse selection and exact native undo regressions |
| Refused bulk screenplay loads returned no explanation | Propagate the candidate transaction error; duplicate-ID refusal preserves the live document |

No parallel document model, permanent timer, character Ctrl tree or full-control
raster cache was added. The existing positional/sparse model remains authoritative.
History separates compact text-edit records from one transaction snapshot pair.
History still clones collections at transaction boundaries, WString text editing
and range remapping are linear, and paginated layout walks document lines.
These are explicit costs, not claims of constant-time editing at arbitrary scale.

## Automated validation

The retained seven suites pass in Release BLITZ and Debug without BLITZ:

| Suite | Checks per configuration | Failures |
| --- | ---: | ---: |
| UiDocCoreTest | 84 | 0 |
| UiDocModelTest | 151 across 17 cases | 0 |
| UiDocModelBindingTest | 22 | 0 |
| UiDocGeometryTest | 34 | 0 |
| UiDocMetadataTest | 47 | 0 |
| UiDocInteractionTest | 41 | 0 |
| UiDocImageTest | 75 | 0 |
| UiDocHygieneTest | 50 | 0 |
| UiScreenplayTest, including three page renders | 1489 | 0 |
| UiScreenplayDemo editor checks | 20 | 0 |

Screenplay coverage includes semantic Enter/join/page-break undo, stable paragraph
IDs/metadata, common Fountain roles/emphasis/notes, atomic refused imports, native
cover QTF/artwork, missing-resource refusal, columns/hit testing, a 14-page dialogue,
minimum split fragments, metadata-independent pagination and a shared-model print
view. The large fixture reports `rebuilt=1 paragraphs=1000` after a text insertion.
Without the optional render directory, UiScreenplayTest executes 1486 checks.

Four actual demo exports (default, dark overrides, rich metadata/QTF/artwork and
native extensions/anchors) compile unchanged in Debug and Release. A separate
harness includes the untouched recipe, calls its ConfigureScreenplay function,
and compares native JSON plus the live view/style signature with demo fixtures.
Generated code contains no test harness, PropertyEditor or demo-shell dependency.
The isolated public header and the complete application recipe in SCREENPLAY.md
also compile in both configurations. Test artifacts/logs stay under build.

## Reproduction on the audited Windows toolchain

The audited setup is U++ `E:/upp-18468`, method `CLANGx64`, jobs 2. Resolve the
local U++ installation/nests before adapting these commands on another machine.
Use latest main containing this change; preserve local work before updating.

```powershell
$umk = 'E:/upp-18468/umk.exe'
$assembly = 'E:/apps/github/upp_Ui,E:/apps/github/upp_Ui/examples,E:/apps/github/upp_animation,E:/apps/github/upp_statemachine,E:/upp-18468/uppsrc'
$packages = @('Core','Model','ModelBinding','Geometry','Metadata','Interaction','Image','Hygiene') | ForEach-Object { "Utilities/UiDoc$($_)Test" }
$packages += 'Utilities/UiScreenplayTest'
foreach ($flags in @('-brH2','-H2')) {
    foreach ($package in $packages) {
        $exe = Join-Path $PWD "build/doc-validation/$($package.Split('/')[-1]).exe"
        New-Item -ItemType Directory -Force (Split-Path $exe) | Out-Null
        & $umk $assembly $package CLANGx64 --out-dir "$PWD/build/doc-validation/cache" $flags $exe
        if ($LASTEXITCODE) { throw "Build failed: $package" }
        $result = (& $exe) -join "`n"
        if ($LASTEXITCODE -or $result -notmatch '(checks|total)=[1-9][0-9]*' -or $result -notmatch 'failed=0') {
            throw "Failed, missing or empty summary: $package`n$result"
        }
        $result
    }
}
```

Build UiScreenplayDemo with the same assembly/flags; run `--self-test` and
`--export-generated <evidence-directory>`. Compile the four exported .cpp files
as ordinary Ui applications. `UiDocHygieneTest` and `UiScreenplayTest` are also
registered focused targets in the maintained release inventory. Their execution
must return zero with a positive mandatory summary. Stop on failed builds,
assertions, absent/empty summaries or any changed native round-trip content.
Never remove a regression to make the gate pass.

## Remaining acceptance

Native off-screen page PNGs were inspected for cover emphasis, body columns,
page numbers and MORE/CONT'D continuation placement. The demo exports a PDF
through its exact print-view/PdfDraw path; physical page dimensions and rendered
PDF pages are checked separately from PNG output. Poppler renders the two-page
Letter fixture and pdfplumber extracts its text. The bundled pypdf parser raises
a font-map extraction error on this PdfDraw output; acceptance in other PDF
readers remains an explicit output-compatibility gate.

Opening Final Draft 12 and the demo through the native UI automation tool timed
out awaiting app approval. No successful interactive reference inspection is
claimed. The executable is available for the writer's acceptance: edit in each
element, drag-select and cancel capture, switch light/dark and inspector pages,
resize, review/resolve guidance, save/reopen and compare exported PDF pages.
These hands-on gates remain open, as do other platforms/fonts/printers/DPI
transitions and the advanced features listed in SCREENPLAY.md. The control is a
reviewable initial implementation; this audit cannot provide a 100% defect-free
or Final Draft feature-parity guarantee.
