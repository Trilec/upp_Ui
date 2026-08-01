# UiDesigner Container Contract

The designer treats controls as either leaves or containers. New container
nodes default to `Expand` on both axes with `Stretch` cell alignment. New
leaf nodes retain natural-size `Fit` placement. These are authored placement
properties, not Preview-only overrides, so Grid, Box and generated C++ use the
same contract. Existing documents keep their authored values.

Every container is a hard geometry boundary. Runtime layout, the geometry
snapshot, selection decoration, drag hit testing and generated code must use
the same published rectangles. A container must not escape its assigned cell;
when an inset cannot fit in a very small cell, its drop body is bounded to the
outer rectangle rather than becoming an invalid rectangle.

## Conformance Matrix

| Control | Sizing class | Direct content contract | Published drop region |
| --- | --- | --- | --- |
| `UiWindow` | Container | ordered content | Window content |
| `UiGridLayout` | Container/layout | ordered grid cells | Grid cell |
| `UiBoxLayout` | Container/layout | ordered items | Box body/frame/gap |
| `UiAbsoluteLayout` | Container/layout | positioned children | Layout bounds |
| `UiPanel` | Container | ordinary direct children | Panel body |
| `UiGroupPanel` | Container | zero or one direct child | Group body |
| `UiDirectContentHost` | Container | one direct child | Host body |
| `UiScrollPanel` | Container | one direct child | Scroll body |
| `UiTitleCard` | Container | zero or one content child | Title Card content |
| `UiTab` | Container | semantic pages | Tab page |
| `UiTabPage` | Semantic page | zero or one direct child | Page host |
| `UiStack` | Container | ordered stacked children | Stack bounds |
| `UiAccordion` | Container | semantic sections | Section content |
| `UiSplitter` | Container | bounded panes | Splitter pane |
| `UiQuadSplitter` | Container | bounded panes | Splitter pane |

Title Card content is attached with `UiTitleCard::SetContentCell`. Accordion
section content is attached through the section's runtime host. Both are
one-child relationships in the document model and are rejected with guidance
when a second direct child is attempted. They are not encoded as ordinary
properties or JSON strings.

`UiGroupPanel` content is attached with `UiGroupPanel::SetContent` and cleared
with `ClearContent`; its prospective body is `GetBodyRect()`. `UiTabPage`
content is attached to the page's `ParentCtrl` host, never to `UiTab` as a
generic sibling. Catalog metadata supplies the host kind, direct-child limit,
semantic allowance, preview/code-generation adapter and drop-region kind. Drop
planning and hierarchy inside-drops consume this metadata rather than node
flags or control-name branches.

Accordion section body height is zero for an empty section, and populated
open sections are allocated deterministically within the Accordion rectangle.
Closed sections reserve header space only. The same runtime geometry drives
the section drop region and selection decoration.

The orange dashed region is shown for the selected container and is reused as
the drag target. Empty regions are available; occupied one-child regions are
shown as occupied and reject a second direct child. Leaf selection does not
publish a container content region.

## Managed Rebuilds

Preview subtree replacement detaches through the owning adapter before the
old `Ctrl` is destroyed: `UiGridLayout::RemoveItem`,
`UiBoxLayout::RemoveItem`, `UiAbsoluteLayout::Remove`,
`UiTitleCard::ClearContentCell`, `UiAccordion::GetSectionContent` removal,
`UiTab::Remove`, and the splitter removal APIs. Grid validation checks that
every registered control is non-null, parented by the Grid and unique before
measurement. Accordion text, open and lock edits project directly to the
current runtime section index; structural section changes rebuild only the
Accordion subtree.
