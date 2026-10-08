# UiFileBrowser

Optional native browser package `Ui/UiFileBrowser`, independent of CineView and
Imaging. Add that package to `uses` and include `<Ui/UiFileBrowser/UiFileBrowser.h>`.
The maintained standalone example is `examples/UiFileBrowserDemo`.

`UiFileBrowser` embeds in a host; `UiFileBrowserDialog` wraps the same control for
modal Open/Cancel policy. It supplies breadcrumbs, history, places, session pins,
drives and caller roots; detail/tile views; natural sorting, search/filter rules,
sequence grouping; image/text preview; and small starter-file creation. Fonts and
colours follow the host UiTheme. The inherited font uses the shared UiFonts Body
family and the application StdFont height; compact text previews use the Code
role. SetFont remains an explicit override; UseThemeFont restores inheritance.
Theme buttons request a host change. Semantic
file colours and icons are centralized in `UiFileBrowserAppearance`.

Responsibilities are separated: Model parses/groups filesystem entries; Scanner
owns enumeration generations; Layout arranges native controls; Style and
Appearance resolve presentation; Options supplies short popups; Preview and
Create contain bounded media/file services. Table/gallery models are projections
of the scan model. GUI controls are never touched by worker callbacks.

JPEG/PNG previews preserve aspect ratio; text displays up to ten physical lines
without wrapping. Source fragments and `.url` contents are plain text: links are
not followed or executed. Original image dimensions appear in metadata. A host
`UiFileBrowserPreviewProvider` may add formats (for example EXR via Imaging).
Providers run on the worker, must honour cancellation and budgets, and must own
their services rather than capture a `Ctrl`. Return false for built-in fallback;
return true for handled requests, including a handled error. Sequence preview
uses the selected or representative frame; it does not decode an entire sequence.

Enumeration and preview requests replace older generations. Workers own shared
state independently of the dialog; closing cancels immediately without waiting
for an OS filesystem call. Cancellation becomes effective when that call returns.
Progress is reported during enumeration. Completed catalogue projection still
runs on the GUI thread; starter-file creation is also synchronous. Batched
projection, watchers and measured 100k-directory
acceptance remain future work. This migration does not claim those qualifications.

Audit and input limits:

- Palettes are bounded and invalid hex/dimensions do not replace live state.
- Default preview limits: 64MiB input, 64 megapixels, 512×320 thumbnail; built-in
  image dimensions are capped at 16384 per side. Requested thumbnails above 4096
  per side and non-positive budgets are rejected before allocation.
- Text reads at most 64KiB. An absent cancellation callback is valid.
- New operations accept a single leaf name, reject traversal/reserved names,
  create exclusively without overwriting, and roll back their own partial files.
  Starter images cap each dimension at 8192 and total pixels at 16Mi; encoding
  uses bounded rows rather than a full uncompressed image buffer.
- Private COM apartment/interface guards are shared by decode and encode and are
  noncopyable. Platform handles have explicit ownership.
- Group construction and missing-frame expansion check cancellation periodically;
  POSIX sequence names retain case distinctions. Sparse gap expansion is bounded
  to a span below one million frames.

Windows uses WIC for JPEG/PNG and starter-image encoding. Other platforms use
registered U++ raster decoders for preview; applications must link the codecs
they need. Starter PNG/JPEG creation currently reports unavailable off Windows.
Linux/macOS native validation, network stress, physical drag/drop and high-DPI
manual acceptance are not claimed by the Windows focused tests.

Focused Windows validation (2026-10-07): 25 model and 164 native checks pass
with Debug non-BLITZ, Release non-BLITZ and Release BLITZ builds. Two additional
Release native runs pass after the logical popup-owner correction. CineView's
release integration and EXR preview-provider probe pass. The shared picker family
demo, independent headers and emitted C++ compile/run in Debug and Release.
Detailed evidence is recorded in upp_cineview/build/shared-ui/ACCEPTANCE.txt.

Shared typography follow-up passes five additional native checks for Body/Code
selection, explicit overrides and unchanged-theme polling without table rebuilds.
Evidence: build/browser-typography-{debug,release,blitz}.log and
build/viewer-typography-release.log in upp_cineview.
