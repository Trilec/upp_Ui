# PropertyEditor reliability audit — 2026-09-27

Scope: model notifications/reset, override input, model rebinding, deferred
colour edits, row sorting, inline virtualization, and editor registrations.
This is a focused source and regression audit, not certification of every
popup/editor or a measured application-wide performance benchmark.

## Repaired

- **Reset callback safety.** Reset previously called Apply (which emits host
  callbacks), then wrote through the original item pointer. A host that rebuilt
  its schema could invalidate that pointer. Reset now publishes the final
  inherited value in one value notification, followed by Commit and Reset.
  Property IDs and defaults are copied before callbacks. Tests rebuild the
  schema in WhenValueChanged and pass an ID owned by the original item.
- **Read-only overrides.** Mouse body/circle and Enter/Space could request
  activation on a read-only override. Those paths now respect read_only.
  Disabled inherited rows retain the existing override-activation contract;
  enabled and value_editable are distinct capabilities.
- **Rebinding.** Returning to a previously attached model could execute multiple
  subscribed handlers for one notification. Binding generations ensure only the
  current handler set can refresh the editor. The test switches models twenty
  times and compares the resulting layout count with a fresh binding.
- **Deferred work.** A delayed colour edit cannot cross into another model
  binding with a matching property ID. Switching models clears stale drop
  highlighting and pending structural refresh requests. This guard is inspected
  and compiled; native asynchronous switching during a drop was not simulated.
- **Ordering.** Replaced quadratic insertion sorting with Sort and an explicit
  original-index tie breaker. A 512-property reverse-ordered schema checks
  keyboard order and equal-key stability. No timing improvement is claimed
  without a benchmark.

## Existing strengths retained

- Semantic editor factories and normalization are independent of Designer.
- Hosts own application commands, persistence and Undo.
- Inline editor allocation is limited to the viewport plus overscan.
- Validation and read-only checks remain in the headless model as well as UI.
- PropertyV1Editors.cpp is actively registered by PropertySemanticEditors.cpp.
  Its name alone is not grounds for removal.

## Further lifecycle work

1. Event chains retain inert callbacks after model switches. Generation checks
   eliminate duplicate refreshes but do not remove these entries. A detachable
   subscription API should be designed in PropertyEditorCore, with ownership
   and destruction-order tests, rather than clearing host-owned Events.
2. Active/inline editors still have paths holding item or editor references
   across host callbacks, particularly BeginEdit, Preview, selection and modal
   picker completion. They need a dedicated destructive-callback test harness
   before claiming arbitrary synchronous schema replacement is safe everywhere.
3. A shared model pointer does not identify which application object its schema
   represents. Hosts that replace the inspected object in place may need an
   explicit edit-context token to cancel delayed edits, distinct from ordinary
   structure revisions caused by activating an override.
4. Scrolling currently resizes partially visible editors to their visible row
   intersection. Visually exercise compound editors at viewport boundaries
   before changing that to a dedicated clipping host.

## Validation

- PropertyEditorTests: 143 checks, zero failures (10 new checks).
- PropertyEditorSemanticRunTests: 44 checks, zero failures.
- PropertyEditorV1RunTests: 156 checks, zero failures.
- Designer AssistantDesignerTests: 200 checks, zero failures.

The preceding palette repair was tested natively for Inspector colours and
fills, direct sample selection and Theme Undo. This audit adds source-level
lifecycle repairs and deterministic regression coverage; it does not repeat
all native interactions or the seven-preset visual audit.
