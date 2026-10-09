# Generic controls: first implementation

The shared controls live in `E:/apps/github/upp_Ui`, inside its existing `Ui`
package. Include `<Ui/Ui.h>` or the individual public header. They do not depend
on CineView, OpenImageIO, FFmpeg, UiTimeline, a GPU renderer or a network service.

| Name | Generic responsibility | CineView supplies |
| --- | --- | --- |
| `UiColorProbe` | Display swatch, copyable RGBA readout, Live/Point/Area and Source/Display choices; raw HDR values and explicit quality label | Image coordinate/rectangle selection, pixel sampling, averaging rules, colour transforms, actual proxy resolution |
| `UiPlaybackBar` | Transport requests, integer frame scrubber, editable range, marker ticks/navigation and supplied cache coverage | Media decoding, audio clock, playback scheduling, frame rate/timecode formatting, remote synchronisation |
| `UiRangeSlider` | Scalar interval, optional outer bounds, whole-interval dragging, preview/commit/cancel | HDR meaning, tone mapping, exposure/black/white transform, command history |

`UiSlider` also gained drag cancellation for scrubbing. `UiSliderEdit` and
`UiRangeSliderEdit` forward drag begin/cancel notifications and resynchronise
their numeric fields after cancellation. Whole-range dragging and rollback are
opt-in, preserving existing callers' range-click behaviour and live-value policy.

## Boundary and naming decisions

These two new composites are worth sharing now: colour inspection applies to
image editors, render tools and scientific viewers; playback navigation applies
to animation, video, audio and simulation tools. Extend the existing range
control instead of making a second interval implementation. Keep the generic
range usable horizontally and vertically.

Do not add `CineRGB`, `CineTransport`, a separate `HDRSlider`, or another timeline
engine. CineView's HDR panel can compose the range slider, existing numeric edits
and histogram. Annotation, metadata overlay zones, media snapshots, review
sessions and application commands remain subsequent work.

## Contract

### Compact layout revision

`UiColorProbe` supports RGB or RGBA, with a 196 × 22 logical-pixel minimum for the default inline RGB layout. The compact RGBA header demo uses 180 × 36 pixels.
There are no built-in channel-name labels. Red/green/blue (and optional neutral
alpha) background tints identify each value. Raw float, 5/8/10/12/16-bit integer
and hexadecimal formats are available through a small arrow menu. Integer/hex
presentation maps 0–1 into the selected scale and clamps only that presentation;
the original `UiColorSample` remains unchanged. Space, quality and full channel
values move to tooltips instead of consuming toolbar space.

Point and rectangular sampling have separate visible icons. Right-click or Shift+F10 on either opens the sampling-space menu for Source/Display choices. The
optional copy icon offers displayed values or the independently transformed
display swatch as RGB hex. Alpha, swatch and copy are independently optional.
`SetControlsSide` puts sampling/copy icons left, right, top or bottom; the format
arrow stays beside the values. `SetRowHeight` allows a 16-pixel row for dense
headers. Four-pixel gaps are configurable through `SetGap`; the swatch fills the value-row height and the format arrow uses an eight-pixel glyph. The control divides the allocated width between its visible values.

For the reference layout, supply the caption as borrowed host content:

```cpp
UiLabel caption; // Owned by the host; outlives its attachment.
caption.SetText("RGBA");
probe.ShowAlpha().SetControlsSide(UiAlign::TOP).SetRowHeight(DPI(16));
probe.Accessory().SetContent(caption);
```

`Accessory()` is an existing `UiDirectContentHost`, not a new label system.
It occupies the space opposite the icons in a top/bottom row. Supply one label,
button or a layout containing several controls; the host retains ownership.
It is hidden for inline Left/Right layouts. Clear/reassign its content through
the standard content-host API.

Playback uses cached monochrome factories registered in `UiIconCatalog`.
`UiPlaybackBar::Style` contains per-command images, tint, icon size and layout
extents. `SetIcon`, `SetIconColor`, `SetControlsSide`, `SetRangeSide` and
`SetDirection` support custom artwork and horizontal/vertical embedding.
The default puts transport, seek and time readout on one row, with the optional
range on a separate edge. No text glyphs substitute for transport icons.

The revised native demo passed 48 checks, including number-format mapping,
unchanged HDR values, copy text, borrowed-header placement, catalogue lookup,
per-command icon replacement and all four playback sides in both directions.
An early demo layout lookup caused a startup access violation; the lookup now
uses a safe default until the PropertyEditor model exists. The corrected native
startup smoke run, interaction checks and render export all exit successfully.

All controls operate on the GUI thread. Programmatic setters are silent.
Playback transport requests leave playback state unchanged until the host calls
`SetPlayback`. Scrub/range edits update local values and then notify; the `preview`
argument separates live changes from a durable commit. A cancelled gesture emits
its cancellation event without a commit. CineView must restore any corresponding
temporary engine/display state in that callback and create one undo command at
the commit boundary.

`UiColorSample` preserves floating-point channels, including negative, above-one,
NaN and infinity values. Its `swatch` is a separate host-supplied display `Color`:
the control does not silently clamp raw channels into the swatch. `space` and
`quality` describe the sampled values. Changing Source/Display requests a new
sample through `WhenOptions`; it does not relabel or transform the old sample.

`UiPlaybackBar` keeps absolute frame indices as `int64` and projects only their
relative offset into sliders. Domains exceeding `INT_MAX` steps, or reversed
domains, are rejected without replacing state. Negative frame indices, a large
absolute origin and a single-frame domain are supported. This is an integer frame
control; timecode, drop-frame and fractional rates belong in the host formatter.
Markers can arrive unsorted; navigation selects the nearest valid marker.
Coverage is passive host-supplied information, not a cache implementation.

## Review and validation

The native `examples/UiMediaControlsDemo` package contains a PropertyEditor,
theme toggle, usage-code export and `--self-test <result-file>` interaction
checks. The checks cover HDR preservation, silent setters, extreme frame indices,
rejected domains, disabled requests, marker navigation, range width/clamping,
Escape, capture loss, disabled-during-drag, vertical bounds and callback deletion.
The generated standalone C++ example is compiled separately.

Validation on 2026-10-04: 25 focused interaction checks passed against the
shared-source demo; the RangeSlider suite passed 50 checks (now retained in `Utilities/UiControlTests/RangeSlider.cpp`).
Debug and Release BLITZ builds succeeded, as did a Release build without BLITZ.
The exported usage example compiled directly against the shared Ui package.
`git diff --check` passed. Native Light/Dark, disabled and narrow-size renders
were inspected; initial light-mode desktop inspection was also performed.
The desktop input/capture helper subsequently reported an active request and
prevented completing the broader manual interaction sweep. Simulated gesture
tests ran on opened native windows. Multiple-display/high-DPI acceptance is
still pending; these checks do not constitute a full Ui release certification.

The implementation is a first iteration. No media throughput, audio synchronisation, progressive transport or
GPU/HDR-output performance is implied by these control tests. High-DPI acceptance
on multiple displays and the broader Ui release matrix remain separate work.

Integer/hex readouts mark clipped channels with `*`; `IsChannelClipped(channel)`
exposes this presentation state. Canonical numeric/hex copy text remains valid
without the marker. Float supports raw HDR values above 1 and negative values.
Mode defaults to Live; existing Point/Area enum values are preserved. The themed
buttons expose the selected mode and request host handling through `WhenOptions`.
Setters remain silent. S/D, right-click and Shift+F10 open the sampling menu.
The control owns no image, mouse-selection, averaging or held-sample logic.
The Workbench implements live hover, click-to-hold Point and drag-to-hold Area,
with source-coordinate overlays and bounded row buffers for the exact region.
It captures Source and Display separately and supplies the requested capture.

Focused validation on 2026-10-05: the Debug media-control self-test passed 51
checks. The host Workbench passed 226 Debug checks, including native mode/menu,
held-sample and rectangle-overlay checks; dark/light and narrow renders were
inspected. Its actual Release executable compiled and passed startup/normal close.


## Single-track review timeline (2026-10-06)

`UiRangeSlider::EnablePosition()` adds a separate scalar playhead while preserving
the existing interval and optional outer bounds. `SetPosition` is silent;
`WhenPositionChanging`, `WhenPositionAction` and `WhenPositionCancel` distinguish
preview, commit and rollback. Plain track clicks seek; inner/outer thumb hits edit
the corresponding handle. Shift-drag translates the interval when range dragging
is enabled. Escape, capture loss and disabling use the existing cancellation policy.
The default remains the original two/four-handle range interaction.

`SetSelectedTrackThickness(px)` makes the selected interval thicker than the base
track. Zero inherits the base thickness. `SetThumbSize` controls inner handles;
`SetBoundThumbSize` optionally controls the smaller outer handles. Public
`GetThumbRect(handle)` reports the same geometry used for paint and hit testing.

`UiPlaybackBar::SetCombinedTimeline()` uses that playhead and interval in one
track with markers and cache coverage. The horizontal toolbar is centred above
the track, with outer domain and trimmed In/Out readouts. `ShowPauseButton(false)`
omits the separate Pause command; both forward and reverse play buttons request
Pause when already playing in that direction. `ShowTime(false)` lets the host put
its current-frame information elsewhere. These features are opt-in; legacy
placement and the int64 frame-domain contract remain supported.


## Reviewer typography and hover (2026-10-06)

`UiColorProbe::SetFont(Font)` sets the numeric channel font independently of row
height; `SetFont(Null)` restores the host font. Channel colours are refreshed from
the current theme, including after a font override. Optional caption content stays
host-owned and can use the same font through its own label style.

Neutral `UiPlaybackBar` transport icons intensify on hover and re-resolve the theme
when styles/icons are refreshed. Reapplying the same playback state does not rebuild
all seven buttons. `UiSlider` now paints its existing hot-state palettes on mouse
enter and returns to normal on mouse leave; disabled and captured/pressed states
retain priority. Hover does not change a value, emit an edit, or acquire capture.


## Review-tool emphasis and cache line (2026-10-07)

`UiPlaybackBar::Style::icon_sizes[7]` optionally overrides the common
`icon_size` per command, in the documented command order. A zero size keeps
the common fallback. Forward/reverse play retain their slot's size when showing
Pause. `transport_offset` adds a bounded cross-axis inset in combined layout;
it leaves the transport's major-axis centre and the legacy layout unchanged.
`coverage_thickness` controls the passive cache line (minimum one pixel,
default `DPI(2)`), independently of the timeline's track thickness.

`UiColorProbe` defaults to a restrained, slightly square selection outline
with transparent fill; inactive icons are dimmed and hover intensifies the
icon. The outline follows the host's light/dark theme. Sampling/copy contracts,
silent setters and the existing default geometry are unchanged.


Focused Windows validation for this revision: Debug and Release each pass 53
maintained media-demo self-tests and 576 native selector checks. The two affected
public headers compile separately; all six exported C++ examples compile and
instantiate in both configurations. CineView's 98 native checks also cover the
new per-command emphasis, transport inset, sampler/mode selection outlines and
Skip-field geometry. Evidence: `upp_cineview/build/skip-correction-shared` and
`upp_cineview/build/skip-correction-acceptance.txt`; this is focused validation,
not the broader Ui release gate or multi-display/high-DPI acceptance.
