# Designer V1 shell gap list

## Current architecture status
- Core Designer architecture is stable after the structural/layout codegen split.
- Shell work is still layered on top of the existing model/command/inspector flow.
- Theme Studio is intentionally not being built yet.

## Recent-history behaviour
- Save As, save existing, and load/open should all feed one shared recent-document list.
- The list should be ordered newest-first, deduped, persisted, and capped at 10 entries.
- Missing recent files can be shown as unavailable instead of disappearing silently.

## Shell gaps
- Top bar: save/load history menus still need a final V1 polish pass.
- Left panel: tool/category rail remains mostly visual scaffolding.
- Right panel: mode rail and content area still need a tighter V1 wiring pass.
- Preview/zoom: aspect helper exists, but the surrounding chrome still needs consistency checks.
- Theme selector: still a placeholder, not Theme Studio.
- Diagnostics area: present as shell space, not a finished workflow.

## Reliability notes
- Failed load must keep the current model intact and report the actual error.
- Successful save/load should update the shared recent-document model.
- Recent items must not duplicate; missing items should be marked unavailable.

## Remaining V1 blockers
- Final Save/Load menu behavior and wording.
- Final shell polish around the top bar and side rails.
- Full Theme Studio implementation.

## Post-V1 items
- Theme Studio.
- Broader theme export workflow.
- Further shell cleanup once the V1 paths stop shifting underfoot.
