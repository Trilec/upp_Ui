# UPP Guides

This folder is the working documentation hub for engineers and coding agents.
The guide set is intentionally small at the top level. Older notes and source
documents are preserved in `archive/` so detail is not lost.

## Read These First

If you are a new developer or AI agent, read these in order:

1. `01_Upp_General_Guide.md`
   - U++ onboarding, package structure, local workflow, repo conventions, and
     practical project context.
2. `02_Upp_Coding_Standards.md`
   - U++ coding standards, memory/lifetime caveats, anti-patterns, package
     rules, review safety checks, and agent guardrails.
3. `02_Ui_Controls_Guide.md`
   - The `Ui*` control architecture, theme system, sizing/content contract,
     panels vs layouts, composites, paint hooks, and control review gates.
4. `03_Ui_Demo_Guide.md`
   - Demo shell rules, builder-demo structure, inspector composition, copyable
     usage output, and demo migration gates.

Optional reference:

5. `05_Ui_Feature_Reference.md`
   - Larger feature-specific designs and roadmaps for UiDoc, UiMenu, UiTable,
     dropdown, tree/list/model work, shadow API, and PatchTrack.

Active layout/container notes:

- `UiBoxLayout_Flow_Groups_Design.md`
  - Box wrapping, flow, and snap behavior; confirms grouping chrome belongs in
    `UiGroupPanel`, not in the layout engine.
- `UiGroupPanel_Design.md`
  - Titled group-panel container contract, header modes, designer support, and
    theme-role styling expectations.

## Archive Policy

`archive/` contains the original segmented notes used to build the consolidated
guides. Do not delete archive files unless the same detail has been deliberately
merged elsewhere and reviewed.

When a new guide grows out of a design note:

- move the old note to `archive/`
- import or preserve the useful detail in one of the primary guides
- update this README if the reading order changes
- avoid compressing U++ caveats and safety notes so far that future agents miss
  the reason behind the rule

## Root Docs

Keep public/project-entry docs in the repository root:

- `README.md`
- `GETTING_STARTED.md`
- `CHANGELOG.md`
- `CHECKLIST.md`

Keep implementation guidance, standards, and feature roadmaps here.
