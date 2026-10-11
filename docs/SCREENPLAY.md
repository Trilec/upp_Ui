# UiScreenplay and the UiDoc foundation

`UiScreenplay` is a native screenplay editor derived from `UiDoc`. It uses the
same `UiDocCore` for text, styles, semantic blocks, resources, comments, metadata,
anchors, revisioned transactions and undo. No alternate screenplay document
store or per-paragraph child controls are involved.

Include `<Ui/UiScreenplay.h>` or `<Ui/Ui.h>` and depend on package `Ui`.
The maintained example is [UiScreenplayDemo](../examples/UiScreenplayDemo).
The [document audit](UIDOC_AUDIT.md) records the validation boundary.

## A small usable editor

```cpp
#include <Ui/UiScreenplay.h>
using namespace Upp;

GUI_APP_MAIN
{
    UiScreenplay script;
    String error;
    script.ImportFountain("INT. ARCHIVE - NIGHT\n\nA lamp comes on.\n\nMARA\nWe still have time.\n", &error);
    UiScreenplayTitlePage cover;
    cover.enabled = true;
    cover.title = "THE LAST LIGHT";
    cover.author = "Example Writer";
    script.SetTitlePage(cover);
    TopWindow window;
    window.Title("Screenplay").Sizeable().Zoomable();
    window.SetRect(0, 0, DPI(1100), DPI(850));
    window.Add(script.SizePos());
    window.Run();
}
```

For structured input, pass `Vector<UiScreenplayParagraph>` to `SetParagraphs`.
Each paragraph has an optional stable `id`, an `element`, `WString text` and a
`ValueMap meta`. Text must contain no paragraph separators; use separate items.
Elements are Scene, Action, Character, Dialogue, Parenthetical, Transition, Shot,
Centered, Lyrics and PageBreak. They become `screenplay.*` blocks. `GetParagraphs`
returns their text, IDs and metadata. `SetElement` changes the current paragraph
in one undoable transaction; `GetElement` and static `Role`, `Name`, `Element`
and `NextElement` support host toolbars.

`SetParagraphs` replaces the body, including its annotations, styles and anchors;
it preserves document metadata and resources. It is a bulk loading operation,
not the way to patch a live scene with comments. Live edits use core transactions.

Enter creates the next semantic paragraph: Character/Parenthetical advance to
Dialogue, other elements to Action. Tab/Shift+Tab cycle the nine visible element
types. Ctrl+Enter inserts an invisible page-break paragraph while retaining text
on either side. Backspace/Delete at a paragraph boundary join semantic blocks.
Element text retains the author's casing; role changes do not rewrite content.
Standard scene headings, character cues and transitions should be uppercase.

## Formatting and pagination

`UiScreenplayFormat` uses physical inches and points, converted once to native
DPI-scaled coordinates. Defaults are US Letter, Courier New 12 point, left margin
1.5 inches and other page margins 1 inch. Dialogue has 2.5-inch left/right margins,
character cues start at 3.7 inches, and parentheticals use a narrower column.
The line-spacing target is 54 lines per Letter page. Font metrics, paragraph
spacing and keep rules determine actual occupancy; 54 is not a forced line count.
Page numbers start on the second body page at the upper right. The optional
cover is unnumbered. Scene numbering is off for a submission draft.

`SetFormat` validates paper, column and font bounds before committing metadata;
it returns false and an optional error without changing the document on failure.
`GetFormat`, `ToValue` and `FromValue` expose the persistent configuration.
Font face/size, paper size, all margins, dialogue/character/parenthetical indents,
line target, page numbering, scene numbering and continuation labels are editable
in the demo. Invalid stored format metadata displays with safe defaults.

Word wrapping and hit testing use the same measured lines as painting. Headings
stay with following content; character/parenthetical cues stay with dialogue.
Long dialogue reserves `(MORE)` and repeated `CHARACTER (CONT'D)` labels, with
at least two lines per split where space permits. Explicit page breaks are hidden.
Full layout exposes `GetLayoutLines`, `GetPageCount`, `GetPageRect` and
`GetPageAtPos`. Ordinary UiDoc remains viewport-driven; paginated output measures
all paragraphs and reuses unchanged measurements when text edits shift positions.
Repagination is linear in document lines; it is not a constant-time operation.

## Cover design without an HTML runtime

`UiScreenplayTitlePage` provides enabled, title, credit, author, source, contact,
draft and copyright fields. Optional `qtf` replaces the central title/credit/
author/source composition with native U++ QTF, supporting rich text, emphasis and
alignment. It uses U++ RichText only for this optional cover composition; the
editable screenplay body remains UiDoc. There is no HTML, JavaScript or browser
engine in the control.

An optional `resource_key` refers to an image in the same core resource table.
Add a decoded-size `UiDocResource` through `AddResource` before `SetTitlePage`.
The demo's artwork button imports PNG; hosts declare whichever raster codecs
they need. The image is aspect-fitted above the title composition. QTF parsing and image
decoding are reused while their source content remains unchanged. Missing
resource keys are refused atomically. Native saving retains artwork bytes and QTF.

## Comments, treatments and AI-assisted development

Use inherited `AddComment`, `ResolveComment`, `RemoveComment` and `GetComments`
for review. Use `AddMetadata`, `UpdateMetadata`, `SetMetadataExpanded` and
`GetMetadata` for treatment/scene guidance. Metadata types are application-defined,
for example `dramatica.scene`, `story.beat` or `ai.suggestion`; the control does
not assert that these implement a story theory or generate text itself.

```cpp
ValueMap payload;
payload.Add("objective", "Find the owner of the reel");
payload.Add("turn", "The name changes the investigation");
ValueMap provenance;
provenance.Add("origin", "Writer");
provenance.Add("base_revision", (int64)script.Model().GetRevision());
script.AddMetadata(UiDocRange(0, 0), "dramatica.scene", "Scene purpose",
                   "The clue must cause a decision.", payload, provenance);
```

Annotations retain stable IDs, remapped text ranges, payload and provenance.
The demo's Data page edits the selected reference and its JSON maps. Gutter
markers reveal references without changing screenplay text. Inline reference
cards are hidden by default to preserve pagination; authors can enable them
while drafting. Metadata payloads are not automatically included in printed pages.
`ShowMetadata`, `ShowMetadataCards` and `ShowLineNumbers` are independent view
settings, as are `ReadOnly` and appearance overrides.

AI providers belong to the host. Prepare a bounded `UiDocCoreTransaction` with
`base_revision = model.GetRevision()`, send that revision with the source text,
then apply returned changes on the GUI thread. A stale base revision is refused;
the host must present/reconcile the suggestion rather than retry against an
unknown document. Put proposed text in a comment/metadata payload until the
writer accepts it. The core validates references and applies or rolls back the
whole transaction. See [model contracts](03_UI_MODEL_GUIDE.md).

## Files and output

Inherited `Save`/`Load` use native `.uidoc` version 2. Native data retains typed
Value payloads, resources, sparse marks, annotations, anchors and cover/format
metadata. Undo history is session state and is not persisted. Save writes a unique
sibling temporary file before replacing the destination; failed writes preserve
its previous contents. Successful load replaces live state and clears history.

`ImportFountain` / `ExportFountain` cover common elements, forced prefixes,
character extensions as cue text, emphasis, page breaks, title fields, scene-number
metadata, notes, sections, synopses and boneyards. Notes become metadata.
Export omits notes by default; `ExportFountain(true)` explicitly includes them.
Fountain is not lossless native storage: exact whitespace, arbitrary typed
metadata, resource bytes, fonts/colours and custom roles do not round-trip.
Shot exports as a forced scene heading. Unsupported dual dialogue (`^`) is
explicitly refused without modifying the live document.

For paper output create a separate `UiScreenplay print` and call
`script.ConfigurePrintView(print)`. It borrows the same model, hides editorial
cards/markers, uses white paper and black default ink, and is read-only. Call
`PaintPage(draw, page)` for each `GetPageCount()` page. The demo uses PdfDraw at
600 dots per inch, with exactly the physical paper size, and never changes the
writer's selection or theme to export. Authored text colours/emphasis and cover
QTF remain authored content. The model must outlive the print view.

## Scope and reference material

This is an initial reusable screenplay-writing control, not a Final Draft clone.
Dual-dialogue columns, FDX import/export, production revision locks/coloured pages,
MORE/CONT'D handling across separate alternating speeches, automatic character
completion and advanced multilingual shaping are not implemented. Oversized
embedded body tables/images do not acquire screenplay-specific page-splitting
rules. Validate a production template's font/paper/DPI before final delivery.
Native interactive acceptance and cross-platform font/print acceptance remain
separate from the automated Windows checks.

Formatting sources reviewed 2026-10-11:

- [Academy screenwriting resources](https://www.oscars.org/nicholl/screenwriting-resources)
  and [sample formatting guide](https://www.oscars.org/sites/oscars/files/scriptsample.pdf).
- [Final Draft line spacing](https://kb.finaldraft.com/hc/en-us/articles/15575339757716-How-many-lines-per-page-does-Final-Draft-write-and-what-are-my-line-spacing-options).
- [Fountain syntax](https://fountain.io/syntax/).
- [StudioBinder screenplay elements](https://www.studiobinder.com/blog/brilliant-script-screenplay-format/).

Professional templates vary slightly. Indents are configurable rather than
presented as one universally mandatory template. The demo's short story is original.
