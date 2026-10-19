# UiBoxVisibilityTests

Focused native public-API regression caller for UiBoxLayout logical visibility.
Source preparation is not a compiled or passing result. Root separately allocates
the serialized Debug/Release build and windowless run gates.

## Public contract

GUI thread only. Add/Spacer/Break items default to participating in layout.
Use ItemRef::Visible(bool=true), or checked SetItemVisible(index,bool), to exclude
an item from measurement, minimum/preferred size, gaps, weights, wrapping and
separator paint. The child stays parented; the caller retains C++ ownership.

SetItemVisible returns false for an invalid index and true for a valid request.
An unchanged request performs no layout/paint/parent notification. A default or
out-of-range ItemRef is harmless. ItemRef indices must be reacquired after removal,
as with existing item setters.

IsItemLogicallyVisible reports requested participation. Existing IsItemVisible
reports the last nonempty rendered allocation. PauseLayout defers child Show/Hide
and parent notification; minimum/preferred queries already see the new request.
ResumeLayout(false) defers that application until a later Layout. Nested batches
coalesce visibility invalidation.

Raw Ctrl::Show/Hide remains temporary in a managed item. A logically visible
child with an empty allocated rectangle is hidden automatically, then reappears
when the layout grows. Explicit logical exclusion survives resize and theme
changes. Normal Ctrl hiding/capture/focus teardown applies; the new API neither
selects focus nor reparents, clears text or emits an authoring callback.

~~~cpp
UiBoxLayout content(UiDirection::V);
UiBoxLayout advanced_settings(UiDirection::H);
UiMultiEdit writer;
auto optional = content.Add(advanced_settings).Fit().Visible(false);
content.Add(writer).Expand();
optional.Visible(show_advanced_settings);
~~~

## Meaningful coverage

- Literal pre-existing H/V Fixed geometry and default participation; raw Hide
  compatibility; Fit/Fixed/Expand exclusion, first/middle/last/all-hidden gaps.
- Invalid/default handles, unchanged no-op, nested batches, deferred/manual layout
  and parent minimum/layout notification.
- Flow and Snap wrapping, slot numbering, warm-cache invalidation, hidden-child
  measurement exclusion, visible/hidden breaks and fixed spacers.
- Memory-backed native Draw verifies separator paint versus a white backdrop.
- Zero-height allocation hides a requested-visible item; growing restores it.
  Explicit exclusion remains hidden after growth or a raw Show call.
- Borrowed ownership and actual native text-buffer/action preservation.

The two Flow contract fixtures use actual Ui controls and reproduce the settings
minimum74/780 and Treatment content/action visibility seams found in CineFlow.
They do not depend on or instantiate CineFlow pages/services. Expanded legacy
settings containment, full Synopsis/Treatment composition, project revisions,
queued semantic intent and accepted native focus/keyboard behavior still require
the actual app caller and normal UX journey. These tests do not claim those gates.

## Prepared qualification requests

Use installed CLANGx64 with the Ui assembly nests from GitHubOut.var:

Nests:
E:/apps/github/upp_Ui/examples,E:/apps/github/upp_Ui,E:/apps/github/upp_statemachine,E:/apps/github/upp_animation,E:/upp-18468/uppsrc

Main package: Ui/tests/UiBoxVisibilityTests
Root-selected warm GUI cache:
E:/apps/github/upp_cinespace/build/cinewrite-r41-cache
Use this existing cache only after matching its actual GUI configurations; no -a
or copied objects. A changed public header must invalidate all affected consumers.

1. Debug, no BLITZ: -H2 +GUI, output build/klick/UiBoxVisibility-Debug.exe
2. Run that freshly hashed Debug output with one absolute, fresh owned evidence
   directory: E:/apps/github/upp_Ui/build/klick/box-visibility-20261015-debug.
3. Release BLITZ: -brH2 +GUI, output build/klick/UiBoxVisibility-Release.exe
4. Run that freshly hashed Release output with a distinct fresh evidence directory:
   E:/apps/github/upp_Ui/build/klick/box-visibility-20261015-release.

Klick project: project-B3BB7BB32A38; builder: upp-builder, pinned actual identity.
Do not send expected_sha256 on the pinned compiler request. Runtime:
ui-example-output, approved build/klick/*.exe, with actual fresh raw executable
SHA256. Execution is NOT authorized by this README; root must activate the bounded
four-gate lease after source review. Assert the exact changed-source hashes and
test payloads in each producer/consumer request.

Success requires nonzero executed checks, failures=0, exit0 and a complete
RESULT.json matching stdout. Preserve exact failed diagnostics before any repair.
Reports explicitly leave normalWindowJourney, nativeInputAndFocus and
actualCineFlowPageQualification NOT RUN. No normal app windows, input, live stores,
bin staging, theme suite repetitions, commits or publishing are part of this caller.
