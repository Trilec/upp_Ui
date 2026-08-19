# 13 — Ui Model Mutation and Scale Contract

This note records the cross-control rules confirmed by the 2026-08 model audit.
The audit was driven by real high-scale consumers (`UiNodeGraph` and the
5,000+ item Symbol Picker Gallery), not by API symmetry alone.

Use this together with:

- `03_UI_MODEL_GUIDE.md` — ownership and model/view responsibilities;
- `06_UI_MODEL_VIEW_SCALE_GUIDE.md` — renderer/view scaling;
- `09_UI_MODEL_API_AUDIT.md` — public API convergence.

## 1. One semantic authority

The active model owns semantic record state. A view owns interaction state and
prepared presentation only.

Do not add a second item store merely to make a control faster. Scale the
projection, renderer pool, spatial lookup or notification scope instead.

The canonical ownership API remains:

```cpp
control.Model();
control.SetModel(external);
control.UseInternalModel();
control.IsUsingInternalModel();
control.ClearModel();
```

External models are non-owning. Ordinary model-backed controls use lifetime-safe
weak observer identity so a destroyed inactive model cannot suppress a later
model allocated at the same address.

## 2. Mutable access requires explicit publication

If a model exposes mutable record access, changing the returned record silently
is incomplete. Publish the mutation once after the edit.

Current explicit publication APIs are:

```cpp
UiListModel::Touch(first, count);
UiTreeModel::Touch(node);
UiTableModel::TouchCell(row, col);
UiTableModel::TouchHeader(axis, index);
UiMenuModel::Touch(node);
UiGraphModel::TouchNode(id);
UiGraphModel::TouchEdge(id);
```

For a contiguous prepared list/gallery range, mutate through `Get()` and call
one ranged `Touch()` after the batch. Do not call `Set()` once per row when the
semantic operation is one prepared-range update.

Example:

```cpp
int first_changed = -1;
int last_changed = -1;

for(int i = first; i <= last; ++i) {
    UiModelItem& item = model.Get(i);
    if(/* item needs prepared image */) {
        item.image = PrepareImage(i);
        if(first_changed < 0)
            first_changed = i;
        last_changed = i;
    }
}

if(first_changed >= 0)
    model.Touch(first_changed, last_changed - first_changed + 1);
```

The changed range may include unchanged rows between the first and last edited
row. One bounded update event is preferable to many fine-grained events when the
view already invalidates only visible prepared renderers inside that range.

## 3. Sequential identity: List, Gallery and Dropdown

`UiListModel` is an ordered sequential model. List, Gallery and Dropdown keep
index-based interaction state attached to the same semantic row across ordinary
structural mutations.

All three use the shared helpers:

```cpp
UiIsSequentialStructuralChange(change);
UiRemapSequentialIndex(index, change);
UiRemapSequentialSelection(selection, change);
```

List-model payload convention:

- INSERT: `(start, count)`;
- ERASE: `(start, count)`;
- MOVE: `(old_index, new_index, c=1)`;
- `SwapItems`: MOVE with `c=0`;
- CLEAR / RESET: sequential view indexes are invalidated.

Do not reimplement these rules inside individual sequential controls.

For application identity across a complete filtered/reordered projection,
`UiModelItem::data` should carry the stable application key. A view may clear
index state on RESET and the application may restore selection by that stable
key where appropriate.

List and Gallery restore multi-selection tokens with
`UiResolveSequentialSelectionTokens()`. The helper resolves all stable
`UiModelItem::data` tokens in one model scan, then applies numeric row-index
fallback only to unresolved numeric tokens. This preserves the single-token
identity contract while avoiding O(selection x model) work when a large
projection restores thousands of selected records.

Do not restore a large stable selection by calling a linear `FindByData` /
`ResolveSelectionIndex` once for every token.

## 4. Stable-ID identity: Tree, Menu and Graph

Tree, Menu and Graph have real stable node/edge identities. They must not be
forced through the sequential-index remapper.

- Tree uses `UiTreeNodeRef`.
- Menu uses `UiMenuNodeRef`.
- Graph uses stable `UiGraphId` (`int64`).

Local item changes retain those IDs. Structural derived state is rebuilt only
when the model operation can actually change that structure.

Important Tree example:

- ordinary node text/image/metadata update -> rebind visible prepared renderer;
- lazy/disclosure-changing update -> rebuild the flattened visible projection.

`UiTreeModel::ImportList()` is a bulk semantic operation and emits one bulk
insert notification rather than one event per imported row.

## 5. Coordinate identity: Table

`UiTable` is intentionally different. Its public active-cell and selection
contract is coordinate/range based; rows currently do not carry stable row IDs.

After row/column structure changes, Table clamps active cell and selection to the
new valid coordinate space.

Do not silently retrofit List-style semantic-row remapping into Table. If stable
row identity becomes a future Table requirement, that is a model design change
and should be introduced explicitly.

## 6. Bulk operations should look bulk to observers

A single semantic operation should normally produce one model notification when
its payload can describe the affected range/structure.

Current examples:

- `UiListModel::AddRange()` -> one INSERT;
- `UiListModel::Touch(first,count)` -> one UPDATE;
- `UiTreeModel::ImportList()` -> one INSERT;
- Dropdown multi-check clear/set operations mutate rows in place and publish one
  ranged `Touch()`.

Do not create public batching APIs merely to reduce two already-bounded events.
Add a wider API only when a real consumer demonstrates that the existing event
contract itself is the bottleneck or cannot express the semantic operation.

## 7. Derived view work must match the mutation

A model revision does not automatically justify rebuilding every derived view
structure.

Use the narrowest correct reaction:

- presentation-only UPDATE -> invalidate affected prepared renderers;
- uniform List/Gallery item update -> no grid/linear geometry rebuild;
- Tree ordinary item update -> no flattened projection rebuild;
- Graph local style/node/edge update -> local prepared geometry and affected
  incident edges only;
- structural INSERT/ERASE/MOVE/RESET -> rebuild the derived structure that the
  mutation actually changes.

This is the central scale rule: semantic model size and live prepared-view work
must remain independent where the control geometry permits it.

## 8. Uniform Gallery does not need a spatial tree

Gallery uses regular uniform cells. Visible range, hit location and marquee
candidate rows/columns are derived arithmetically from viewport, scroll, item
size, gap and column count.

Do not add Graph's spatial hash, an R-tree, quadtree or BVH to Gallery while the
layout remains uniform.

The 100,000-item model/view performance fixture exists specifically to prove
that Gallery/List renderer population and Paint work remain viewport bounded.

## 9. Lazy assets belong at the visible-range seam

`UiGallery::WhenVisibleRange` exists so applications can prepare expensive
thumbnail/image content only for visible + overscan rows.

The recommended flow is:

```text
domain/catalog
    -> cheap UiListModel projection (text + stable data key)
    -> UiGallery visible/overscan range
    -> bounded image preparation/cache
    -> mutable row image updates
    -> one ranged UiListModel::Touch()
```

Do not eagerly decode thousands of assets and do not create one `Ctrl` per
logical record.

## 10. Audit rule for future controls

When a high-scale consumer exposes a problem, classify it before fixing it:

1. Is this domain-specific app behavior?
2. Is a reusable control missing an interaction/view capability?
3. Is the shared model missing a mutation/identity notification primitive?
4. Is work being done at full-model scale when viewport/local work is enough?

Fix the lowest reusable layer that actually owns the defect. Do not move domain
semantics into a generic model, and do not leave a generic model defect patched
only in an application.
