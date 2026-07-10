# UPP Guides

This folder is the working documentation hub for engineers and coding agents.
The guide set is intentionally small at the top level. Older notes and source
documents are preserved in `archive/` so detail is not lost.

## Documentation authority

For Version 1 work, read the project documents in this order:

1. `00_Ui_V1_Engineering_Contract.md`
   - Current project authority for public API naming, style ownership, geometry,
     container semantics, lifetime, serialization, and Designer parity.
   - This document overrides older imported examples when they conflict with the
     present `Ui*` architecture.
2. `Ui_V1_Control_Audit.md`
   - Master release-readiness audit and remediation workstreams. This is a plan
     to divide into focused tasks, not one implementation assignment.
3. `02_Ui_Controls_Guide.md`
   - Detailed `Ui*` control architecture, sizing/content rules, themes,
     composites, paint hooks, selection ordering, and review gates.
4. `02_Upp_Coding_Standards.md`
   - Broad U++ safety reference: ownership, GUI-thread rules, callbacks,
     packages, serialization, drawing, and review caveats.
   - It contains imported historical material. Generic Chameleon examples,
     borrowed-style-pointer patterns, or naming examples are not authoritative
     for `Ui*` controls when they conflict with the Version 1 engineering
     contract.

Current code and accepted Version 1 decisions take precedence over stale prose.
When a conflict is found, update the documentation rather than preserving two
competing rules.

## Read These Next

If you are a new developer or AI agent:

1. `01_Upp_General_Guide.md`
   - U++ onboarding, package structure, local workflow, repo conventions, and
     practical project context.
2. `03_Ui_Demo_Guide.md`
   - Demo shell rules, builder-demo structure, inspector composition, copyable
     usage output, and demo migration gates.
3. `05_Ui_Feature_Reference.md`
   - Larger feature-specific designs and roadmaps for UiDoc, UiMenu, UiTable,
     dropdown, tree/list/model work, shadow API, and PatchTrack.

## Active layout/container notes

- `UiSizing_Contract.md`
  - Detailed measurement vocabulary and `GetMinSize()` / `GetContentSize()` /
    width-aware sizing contract.
- `UiBoxLayout_Flow_Groups_Design.md`
  - Box wrapping, flow, and snap behavior; confirms grouping chrome belongs in
    `UiGroupPanel`, not in the layout engine.
- `UiGroupPanel_Design.md`
  - Titled group-panel container contract, header modes, Designer support, and
    theme-role styling expectations.
- `UiDoc_Annotation_Lanes.md`
  - Annotation marker lane registry for comments, metadata, script treatments,
    budget notes, and other typed review data.
- `UiModelDrivenControls_Design.md`
  - Request-first mutation contract for list, tree, menu, dropdown, and table.

## Archive policy

`archive/` contains the original segmented notes used to build the consolidated
guides. Do not delete archive files unless the same detail has been deliberately
merged elsewhere and reviewed.

When a new guide grows out of a design note:

- move the old note to `archive/`;
- preserve useful detail in one of the primary guides;
- update this README if the reading order changes;
- do not compress safety caveats so far that future contributors miss the reason
  behind a rule;
- label historical or superseded material clearly.

## Root docs

Keep public/project-entry docs in the repository root:

- `README.md`
- `GETTING_STARTED.md`
- `CHANGELOG.md`
- `CHECKLIST.md`

Keep implementation guidance, standards, audits, and feature roadmaps here.
