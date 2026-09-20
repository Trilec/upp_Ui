# Workspace 03B — selection, readable text, and curved-shape layout direction

Base inspected: `8f9a49a8367f0dd9d95d1085582e9348c5aa91db`.
The latest user input is Curt's oval-band sketch, native screenshot and Gary's
03A report. This page separates shipped source changes from proposed geometry.
Read ACTIVE_WORK for current publication and validation; earlier limitations in
V8 audit are not silently declared fixed.

## Delete and preview selection — implemented source

The workspace had a Remove button/transaction, but NodeWorkspace::Key only handled
Save and Undo. Structure/region click handlers also did not take focus, so merely
adding a global Delete handler could delete a component while its text editor was
active. Both omissions are fixed together.

Only a focused structure table, region diagram, overlay diagram or node preview
routes Delete to the existing Remove command. Property editors, filters and other
controls keep their own key semantics. PreviewGraph lets Delete bubble to the
workspace rather than invoke UiNodeGraph's graph selection deletion. The same
transaction preserves Undo and rejects inherited layouts; no topology changes.

The preview first tries exact visible component footprints, then a small three-
logical-pixel tolerance clipped to slot and safe area. Hidden components are not
invented as preview hits; they remain selectable through the structure table.
Selection callbacks receive copied IDs, not references invalidated by a rebuild.
A painted surface takes focus after its selection callback finishes.

## Asset name at Normal — implemented source correction

Normal is the node's inclusion level, not a guarantee that every requested font
fits. The Media header is fixed-height. A larger Subtitle plus its gap can leave
less height for Title Fill than the title font's actual GetCy(). Existing text
preparation immediately chose a proxy on that condition, without trying a smaller
readable line. Horizontal ellipsis cannot solve a vertical deficit by itself.

For single-line Ellipsis text, preparation now preserves the projected font when
it fits; otherwise it searches a bounded set of smaller heights (at most sixteen
metric probes), never below readable_min_px. It does not enlarge text when zoom
already projects it below that floor. At the chosen readable font, width still
uses the existing ellipsis path. If even that fails, the approved proxy remains.

This policy does not alter authored font/style, template slots, node size or LOD
masks. Clip and Wrap retain their existing sizing semantics. The Micro early return
still precedes font metric/fitting work; Paint continues to consume prepared output.
A taller header remains the correct authoring choice when both lines must retain
their exact preferred fonts. A wider node cannot cure an insufficient line height.

Text proxies now report NoSpace for a capacity failure versus TooSmall when the
projected font is below its readable floor. The table prefixes placement/features
with current representation and named cause. Its four LOD cells remain inclusion
policy; they are not false promises that four fonts are readable.

Tests exercise direct production preparation and real Media templates, including
Rectangle/Ellipse, two icons, an enlarged subtitle, exact font preservation,
ellipsis, unreadable-floor rejection and unchanged Micro behaviour. Native and
Windows font-backend validation remains necessary; the exact user-edited JSON
was not supplied, so exact reproduction of all screenshot properties is not claimed.

## Oval / circle: independently fitted bands — PROPOSED, not implemented

Curt's sketch preserves Header / Body / Footer but moves the top and bottom bands
into more useful positions within the curved silhouette. That is compatible with
the structural vocabulary. It is NOT compatible with simply moving the preview's
red/green rectangles while production still clips everything to one inscribed
safe rectangle.

Proposed bounded layout policy:

1. Keep the conservative shared safe rectangle as the existing/default policy.
2. An explicit shape-aware band policy may allocate Header, central Body and Footer
   independently inside the silhouette. Header/Footer have preferred height, a
   minimum usable width and a bounded vertical bias/inset. Their result remains
   ordinary prepared rectangles, not curved text or per-node widget trees.
3. Validate the WHOLE rectangle of each band against the silhouette, including
   its narrowest edge, not just its centre. For an ellipse, moving towards the top
   or bottom trades available width for vertical separation. A taller or wider
   band must move inward, become narrower within its declared limits, or report
   insufficient capacity; it cannot be guaranteed to fit merely because its centre
   is inside the ellipse.
4. Resolve disjoint bands, gaps and port reservations before the independent
   Content/Overlay columns. Both layers share the same post-port central capacity.
   Never overlay header/footer, labelled lanes or their reserved intervals.
5. Port anchors stay on the silhouette. Label lanes and their usable side interval
   may be shorter; an optional Body-interval distribution is a separate explicit
   port policy, not an automatic change to port identities/connections. Many labels
   require more node space, summaries or an explicit disclosure mode, not overlap.
6. Store the evaluated result in NodeGeometry.presentation and project it during
   compatible pan/zoom. Do not retain a second shape-layout cache. Shape, authored
   dimensions, font/reservation or policy changes invalidate preparation normally.
7. Audit consumers of presentation.safe, clipping, projection, hit testing, ports,
   copy/export and tests before any band can extend beyond the old safe rectangle.
   Do NOT redefine safe as a larger non-contained bounding rectangle to silence
   tests. Concave/custom shapes keep a conservative fallback until proven safe.

The user benefits are a less cramped oval layout and the same semantic template
across shapes, with optional shape overrides. The cost is a richer capacity
contract, so this must be implemented with the pending Body-only/Full-edge work,
not as an unrelated demo adjustment. No shape-aware-band API or serialization
field is introduced by 03B.

## Gary — focused Debug gate

Repo `E:\apps\github\upp_Ui`, branch main. Fetch/fast-forward clean current main.
Require the supervisor's published 03B SHA as an ancestor (not exact equality).

```powershell
Set-Location E:\apps\github\upp_Ui
powershell -NoProfile -ExecutionPolicy Bypass `
    -File .\scripts\ValidateUiGraphWorkspace.ps1 `
    -RequiredAncestor '<published 03B SHA>' -Launch
if ($LASTEXITCODE) { throw 'Workspace Debug gate failed; inspect evidence logs' }
```

Use the established U++/CLANGx64 setup. Debug only. Existing RenderTests,
WorkspaceTests, unchanged Ui/CtrlLib-only generated C++ and native workspace
view/startup tests must all pass. Native focus regressions briefly open and close
a test workspace; they do not save files or present discard prompts. Do not build
retired DesignMatrix. A positive passing summary is required; no fixed snapshot
of the test count is imposed.

Manual acceptance: select one icon in the structure table, press Delete, verify
only that component disappears and Ctrl+Z restores it. Repeat from diagram and
preview, including a thin title proxy. Edit text in the property inspector and
press Delete there: the component must NOT be removed. Inherited Ellipse must
reject deletion until Base editing or explicit layout detachment is chosen.

Use Media, two Left icons and Subtitle at 18-height/Bold. On Rectangle and Ellipse,
check Title at Normal, narrowed width with ellipsis, and a deliberately too-short
header. Verify the table distinguishes Text from Bar/no room; increasing Header
height should restore the preferred font where feasible. Zoom below the readable
floor must still simplify. Check existing port-label inspector scroll stability.

No geometry redesign, mask changes to hide capacity bugs, weakened tests, native
Micro rich fallback or broad benchmarking. Minor mechanical CLANG fixes are
allowed with full diff review, ACTIVE_WORK update, remote refresh/publication and
retest. Report exact HEAD, ancestry, all summaries, first issue, native/manual
findings, evidence directory, PID, clean worktree and any fix commit. Held-button
Escape DND remains a separate previously unverified manual gesture.
