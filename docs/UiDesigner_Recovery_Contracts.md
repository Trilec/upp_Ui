# UiDesigner Recovery Contracts

## Bounded Grid geometry

`UiGridLayout` resolves columns before rows. Width-dependent children are
measured using the final resolved column width, then row preferences are
bounded against the available client extent. Track allocation keeps authored
minimums as model constraints, uses fixed requests and natural fit sizes as
preferences, and gives extra space only to eligible Expand tracks. When the
preferred or minimum totals cannot fit, the rendered tracks are compressed
deterministically so gaps, cells and children remain inside the Grid client
rectangle. The authored values are not rewritten to hide a small preview area.

This is a runtime geometry invariant, not a special case for Tabs, Title Cards,
or any other control:

```text
sum(column widths) + gaps <= Grid client width
sum(row heights) + gaps <= Grid client height
cell rectangle contains its assigned item rectangle
```

## Canonical and preview-only data

Canonical Data is serialized document state and is available for editing only
where a control has a real data contract. UiTab pages are canonical. The
Dropdown First/Second/Third entries and the Tree Workspace entry are Preview-
only samples; they are not saved or generated. Menu and Accordion currently
have no sample items or sections. Unsupported Data surfaces state that fact
explicitly instead of presenting an unexplained blank panel.

## Detail-surface coverage

| Control | Data | Theme Overrides | Events & Actions | Code | Diagnostics |
| --- | --- | --- | --- | --- | --- |
| UiTab | canonical pages | explicit unsupported status | page changed | generated or diagnostic | live preview |
| UiAccordion | explicit unsupported status | explicit unsupported status | page changed | generated or diagnostic | live preview |
| UiTitleCard | explicit unsupported status | explicit unsupported status | explicit unsupported status | generated or diagnostic | live preview |
| UiTree | explicit unsupported status | registered Tree fields | explicit unsupported status | generated or diagnostic | live preview |
| UiList | explicit unsupported status | registered List fields | explicit unsupported status | generated or diagnostic | live preview |
| UiDropdown | explicit unsupported status | explicit unsupported status | selection changed | generated or diagnostic | live preview |
| UiMenu | explicit unsupported status | registered Menu fields | explicit unsupported status | generated or diagnostic | live preview |

Preview-only samples must remain visibly identified as such until their
focused canonical-data tasks land.

## Locked follow-up sequence

1. `UID-DATA-001B1`: give UiAccordion three canonical editable sections.
2. `UID-DATA-001C1`: give Tree, List, Dropdown and Menu canonical editable data.
3. `UID-THEME-002`: add Tab, Accordion, Title Card and Dropdown theme adapters
   and verify Menu projection.
4. `UID-BEHAVIOR-CODE-002`: complete events, actions and generated-code parity.
