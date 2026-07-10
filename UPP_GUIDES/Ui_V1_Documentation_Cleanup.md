# Ui Version 1 Documentation Cleanup Inventory

This document identifies what should be retained, corrected, consolidated, archived, or removed before the Version 1 release.

It is deliberately conservative: do not delete useful historical or safety material merely because it is long. The goal is to remove ambiguity and superfluous active documents while preserving information that future developers and coding agents may need.

## Status categories

- **KEEP** — active authoritative or useful reference document.
- **CLEAN** — retain, but correct stale paths, conflicting rules, encoding damage, duplication, or obsolete status language.
- **CONSOLIDATE** — merge useful material into an authoritative guide, then archive the source.
- **ARCHIVE** — useful historical/design record that should not remain in the active reading path.
- **REMOVE** — generated, duplicate, empty, superseded, or misleading material with no remaining reference value.

No document should be deleted until references to it have been searched and the useful content has been preserved.

## Active guide set

### `00_Ui_V1_Engineering_Contract.md`

**Status: KEEP — project authority**

Purpose:

- canonical Version 1 public API and style vocabulary;
- style ownership and theme rules;
- geometry, container, alignment, lifetime, animation, serialization, and Designer parity contracts.

Keep concise. It should not absorb every implementation detail from the larger guides.

### `01_Upp_General_Guide.md`

**Status: CLEAN and KEEP**

The underlying U++ onboarding material remains useful, but the current document contains imported-source residue and repository assumptions that no longer match this project.

Required cleanup:

- remove the `Imported Source` heading from the active introduction, or label it clearly as provenance rather than current authority;
- repair mojibake such as broken arrows, quotation marks, and hyphens;
- replace the generic `/lib`, `/demo`, `/docs`, and `/res` project layout with the actual repository layout:
  - `Ui/`
  - `examples/`
  - `Utilities/`
  - `UPP_GUIDES/`
- retain generic U++ package-layout guidance in a clearly labelled general-reference section;
- update the U++ version/build baseline to the release baseline actually used for Version 1;
- replace stale links to `.txt` files that do not exist in the active guide set;
- replace the stale instruction to read `u_new_controls_checklist.md` with links to:
  - `00_Ui_V1_Engineering_Contract.md`
  - `02_Ui_Controls_Guide.md`
  - `Ui_V1_Control_Audit.md`
- correct the statement that accordions inside scroll panels always need explicit body sizing; current guidance should prefer measured content and use explicit height only for deliberately bounded viewports;
- retain ownership, GUI-thread, callback, package, resource, and build workflow fundamentals.

### `02_Upp_Coding_Standards.md`

**Status: CLEAN and KEEP — safety reference**

This is likely the document referred to as the agent/U++ code and safety guide. It contains valuable detail and should not be discarded.

Keep:

- value semantics and RAII;
- `One<>`, `Ptr<>`, `Pte<>`, parent-owned `Ctrl` lifetime guidance;
- GUI-thread boundary and `PostCallback` guidance;
- timer/callback teardown rules;
- error, assert, logging, drawing, clipping, alpha, parsing, serialization, networking, and build caveats;
- anti-patterns and crash-avoidance examples;
- agent review checklists.

Required cleanup:

- add a prominent opening banner stating that `00_Ui_V1_Engineering_Contract.md` is authoritative for `Ui*` public API and styling;
- repair mojibake throughout;
- change the generic claim that methods/functions use `camelCase`; this project and normal U++ public APIs use PascalCase methods such as `SetText`, `GetData`, and `WhenAction`;
- keep lower camel case for local variables where appropriate;
- demote the old Chameleon section from `Ui*` policy to generic/legacy U++ reference;
- remove the duplicated minimal themable-control skeleton;
- mark borrowed `const Style*` examples as inappropriate for new `Ui*` controls;
- point new `Ui*` controls to control-owned `SetCustomStyle` / `ClearCustomStyle` and `UiTheme` role resolution;
- remove or consolidate repeated sections imported from multiple source notes;
- verify every absolute path and example package name;
- keep generic networking/database/material only when it is genuinely useful as an agent safety reference; otherwise move broad cookbook material to an appendix or archive file so the safety rules remain findable;
- add a short release-review section covering:
  - parent-cycle checks;
  - timer/animation teardown;
  - generated-code injection and escaping;
  - allocation/count bounds;
  - invalid enum and persistence input;
  - hidden file/process/network behavior.

The correct result is a retained, cleaner safety guide — not a shortened document that loses important traps.

### `02_Ui_Controls_Guide.md`

**Status: CLEAN and KEEP — detailed control guide**

This is generally strong and already contains many current control contracts.

Required cleanup:

- add a clear reference to `00_Ui_V1_Engineering_Contract.md` at the top;
- remove the active `Imported Source` presentation while retaining provenance in a short note;
- repair absolute Windows Markdown links such as `E:\apps\github\...`; use repository-relative links;
- confirm the control inventory against `Ui/Ui.upp` and `Ui/Ui.h`;
- add `UiQuadSplitter`, `UiSplitButton`, and `UiProgressBar` when their Version 1 implementations are accepted;
- distinguish runtime inventory from current Designer coverage;
- replace `frame -> inset -> padding` terminology with the canonical frame / skin inset / content margin / container inset vocabulary;
- keep the strong selection-state-before-callback guidance;
- keep the cache, paint, style, sizing, model, and demo review gates;
- remove duplicated imported sections where the same checklist appears more than once;
- label historical proposals and completed refactor plans so they are not mistaken for current tasks.

### `03_Ui_Demo_Guide.md`

**Status: CLEAN and KEEP**

The demo guide should remain because demos are part of the manual regression surface.

Audit points:

- confirm the current shared demo shell and inspector sections;
- remove references to retired prototypes except in an archive note;
- ensure executable/output paths match `GitHubOut.var` and the repository `out/` policy;
- distinguish mandatory Version 1 demo behavior from optional builder-demo polish;
- ensure every stable runtime control has either a focused demo or a documented reason for being covered through another demo;
- add progress-bar demo requirements when the runtime control is accepted;
- remove stale version-pill text and screenshots during release closure.

### `05_Ui_Feature_Reference.md`

**Status: CLEAN and KEEP as a reference compendium**

This document is valuable, but large feature references can become misleading when completed plans remain mixed with active plans.

Required cleanup:

- add a status marker to each major feature section:
  - Implemented
  - Partially implemented
  - Planned
  - Historical
- move completed implementation plans that no longer explain current behavior into `archive/` after their final contracts are represented in the control guide;
- keep durable architecture explanations, public behavior, persistence formats, and known limitations;
- remove duplicate task lists that are already tracked in `Ui_V1_Control_Audit.md`;
- verify UiDoc, menu, table, dropdown, tree/list/model, shadow, and PatchTrack descriptions against current code;
- avoid presenting speculative APIs as shipped APIs.

## New Version 1 documents

### `Ui_V1_Control_Audit.md`

**Status: KEEP through Version 1; archive after release closure if fully resolved**

This is the master remediation plan. Individual work assignments should be extracted from it rather than editing the document into a daily task log.

After Version 1:

- retain a final resolved snapshot under `archive/releases/v1/`, or
- convert durable review gates into `CHECKLIST.md` and archive the planning sections.

### `Ui_V1_Documentation_Cleanup.md`

**Status: KEEP during documentation cleanup; archive after release**

This file records cleanup decisions and prevents accidental deletion of valuable safety material.

## Design and roadmap documents

The following policy should be applied to all focused design notes, including layout, Group Panel, model-driven controls, UiDoc lanes, Designer migration notes, and feature-specific roadmaps.

### Keep active when

- the feature is still being implemented;
- the document contains the only accurate architecture contract;
- contributors still need it to make current decisions.

### Consolidate and archive when

- implementation is complete;
- durable behavior has been copied into `02_Ui_Controls_Guide.md`, `05_Ui_Feature_Reference.md`, or a control header;
- the remainder is implementation history or task sequencing.

### Remove only when

- the document is an exact duplicate;
- it contains no unique rationale, contract, or historical value;
- all inbound references have been updated;
- deletion is explicitly reviewed.

Suggested archive structure:

```text
UPP_GUIDES/archive/
    imported/
    completed_designs/
    superseded/
    releases/v1/
```

Do not leave completed task documents in the active top-level guide directory indefinitely.

## Root documents

### `README.md`

**Status: CLEAN and KEEP**

Required cleanup:

- link the Version 1 engineering contract and audit;
- keep the runtime control inventory current;
- distinguish stable controls, in-progress controls, and Designer coverage;
- use canonical `SetCustomStyle(...)` examples rather than stale `SetStyle(...)` calls;
- remove fixed local machine paths from public instructions where a relative or assembly-based instruction is sufficient;
- retain one verified local build example, clearly labelled as a project-machine example rather than a portable command.

### `GETTING_STARTED.md`

**Status: CLEAN and KEEP**

Check against the current assembly, `Animation` dependency, output directory, package names, and canonical style API.

### `CHANGELOG.md`

**Status: KEEP**

Before Version 1:

- separate unreleased work from released history;
- list intentional breaking API cleanup;
- avoid turning the changelog into a task tracker.

### `CHECKLIST.md`

**Status: CLEAN and KEEP if it remains a release gate**

Consolidate duplicate checklists from other documents into this file only after the Version 1 audit is resolved. Avoid maintaining several conflicting release checklists.

## Superfluous-document detection procedure

Gary or another contributor can perform the cleanup as a documentation-only task:

1. Inventory every `.md`, `.txt`, `.tpp`, and design-note file.
2. Record title, purpose, last meaningful update, current references, and status category.
3. Search all repository references before renaming, moving, or deleting.
4. Compare each active guide against current public headers and demos.
5. Move completed or superseded notes into the archive structure.
6. Replace absolute local links with repository-relative links.
7. Repair encoding damage without rewriting technical meaning.
8. Remove exact duplication only after preserving unique safety rationale.
9. Run a Markdown link check and search for stale names/paths.
10. Report every moved or deleted document and where its useful content now lives.

## Documentation cleanup acceptance gate

- `UPP_GUIDES/README.md` provides one unambiguous reading order.
- Guides 1–5 remain present and accurately labelled.
- The U++ coding/safety guide remains detailed and useful.
- Historical Chameleon examples are not presented as current `Ui*` architecture.
- No active guide links to nonexistent `.txt` or old checklist names.
- No public guide uses broken absolute Markdown links.
- Completed design/task notes are archived rather than mixed with active guidance.
- No useful ownership, callback, threading, drawing, serialization, or security warning is lost.
- Root README and Getting Started examples use current APIs.
- A final inventory lists all retained, archived, consolidated, and removed files.
