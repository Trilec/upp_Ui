# UiDoc foundation audit

Audit date: 2026-10-11. Repairs were published in `ecadb12` from baseline `b09b06f`
on main. This is focused Windows document validation, not whole-library or
cross-platform certification. Application-specific authoring controls are not
part of the Ui library. Implementation: [core](../Ui/UiDoc/UiDocCore.h),
[view](../Ui/UiDoc/UiDoc.h). Public contracts: [Models](03_UI_MODEL_GUIDE.md).

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

No parallel document store, permanent timer, character Ctrl tree or full-control
raster cache was added. History separates compact text-edit records from one
collection snapshot pair per transaction. Collection cloning at transaction
boundaries, WString editing and positional remapping remain linear costs.
Paginated layout walks lines while retaining unaffected paragraph measurements.

## Automated validation

All eight suites pass in Release BLITZ and Debug without BLITZ:

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

The hygiene suite retains rollback, stale-revision, shared-view history,
callback destruction/rebinding/reentrancy, read-only, metadata, clear/search
geometry, imported IDs, image-dimension and safe-save regressions. Existing
UiDocDemo/UiDocEditorDemo builds and the 73-check release smoke pass in both
configurations. Build artifacts remain under build; binaries under bin.

## Reproduction on the audited Windows toolchain

Use the actual local U++ nests and method when adapting these commands. This
machine uses U++ E:/upp-18468 and CLANGx64. Stop on failed builds, assertions,
missing/empty summaries or any regression; never remove a check to pass a gate.

```powershell
$umk = 'E:/upp-18468/umk.exe'
$assembly = 'E:/apps/github/upp_Ui,E:/apps/github/upp_Ui/examples,E:/apps/github/upp_animation,E:/apps/github/upp_statemachine,E:/upp-18468/uppsrc'
$packages = @('Core','Model','ModelBinding','Geometry','Metadata','Interaction','Image','Hygiene') | ForEach-Object { "Utilities/UiDoc$($_)Test" }
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


UiDocHygieneTest is registered as a focused target in the maintained release
inventory. All model/view access runs on the GUI thread. External model owners
must keep models alive longer than their views; callbacks must not throw.

## Remaining acceptance

Native typing/selection, focus and capture cancellation, light/dark, resizing,
save/reopen and platform/font/DPI/printer acceptance remain hands-on gates.
Native automation launch approval timed out during the source audit; no
successful interactive inspection is claimed. Generic print/paging hooks are
available for host policy, not a guarantee of every application's output format.
This audit cannot provide a 100% defect-free guarantee.
