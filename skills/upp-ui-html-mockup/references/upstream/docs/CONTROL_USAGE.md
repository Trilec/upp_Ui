# Control usage recipes

Short public API examples for every concrete entry in the
[Controls Guide](01_UI_CONTROLS_GUIDE.md). That catalogue links each public header
and maintained demo; headers remain the authority for overloads, callbacks and
binding formats. These are setup fragments for a GUI-thread host, not complete
applications. Include `<Ui/Ui.h>` and use `namespace Upp`. Keep controls as host
members when they must survive setup, then add them to a parent/layout. Adding a
Ctrl does not transfer C++ ownership. Optional packages are identified below.

## Text and prepared tags

UiLabel is ordinary styled text with optional image/icon, wrapping and selection.
UiTag is prepared presentation data, not a child Ctrl: its host owns placement,
interaction state and actions. Prepare tags when data/style/bounds change, then
call `UiPaintTag(w, prepared, state)` from the host's Paint.

```cpp
UiLabel caption;
caption.SetText("Ready to review");

UiTagData status("READY", UiRole::Accent);
UiTagStyle tag_style = UiResolveTagStyle(status.role);
UiTagPresentation prepared = UiPrepareTag(status, tag_style, RectC(0, 0, 80, 24));
```

## Actions and boolean choices

UiButton supplies the primary action. UiToolButton adds compact toolbar-oriented
defaults and can be checkable. UiSplitButton keeps the primary action separate
from its popup choices, which report through `WhenSelect`. Use UiCheckBox for an
independent state, UiRadioButton for mutually exclusive sibling choices with the
same group, and UiToggle for an on/off switch. Connect `WhenAction` to host work;
programmatic setup is separate from user activation.

```cpp
UiButton save;
save.SetText("Save");
UiToolButton pin;
pin.SetText("Pin").SetCheckable(true).SetChecked(true);
UiSplitButton export_action;
export_action.SetText("Export");
export_action.Add("Image", "image").Add("Document", "document");

UiCheckBox include_audio;
include_audio.SetText("Include audio").SetChecked(true);
UiRadioButton draft, final_output;
draft.SetText("Draft").SetGroup(1).SetChecked(true);
final_output.SetText("Final").SetGroup(1);
UiToggle enabled;
enabled.SetOn(true);
```

UiBreadcrumbs represents a path, with stable payloads on individual crumbs when
navigation needs more than display text. The current index is separate from the
host's navigation operation.

```cpp
UiBreadcrumbs path;
path.SetPath("Projects/Aurora/Shots");
path.SetCurrentIndex(2);
```

## Text and numeric entry

UiLineEdit edits one line. UiPasswordEdit masks it; masking is presentation and
does not make its stored value a secure secret container. UiMultiEdit permits
multiple lines. UiMaskEdit constrains character slots; semantic validation still
belongs to its validator/host. All share UiBaseEdit's text, selection and focus
behavior. Use UTF-8 helpers when the host data is a `String`.

```cpp
UiLineEdit name;
name.SetTextUtf8("Aurora");
UiPasswordEdit password;
password.SetTextUtf8("example only");
UiMultiEdit notes;
notes.SetTextUtf8("First line\nSecond line");
UiMaskEdit date_code;
date_code.SetMask("##/##/####");
date_code.SetTextUtf8("09/10/2026");
```

UiIntEdit owns integer entry; UiFloatEdit supports finite decimal/scientific
values and incomplete text during typing. Set meaningful bounds and step units.
Read `TryGetValue` / `IsInputComplete` for incomplete floating-point edits rather
than rewriting the text on every keystroke.

```cpp
UiIntEdit count;
count.MinMax(1, 10).Step(1);
count.SetValue(3);
UiFloatEdit exposure;
exposure.MinMax(-10.0, 10.0).Step(0.25).Precision(2);
exposure.SetValue(1.25);
```

## Scalar, interval and segment values

UiSlider chooses one value. UiRangeSlider chooses an ordered lower/upper interval.
UiSliderEdit and UiRangeSliderEdit add numeric fields around those same semantic
values. Set the hard domain before the current value/interval and choose a step
in that domain's units. `WhenChanging` is live user feedback; `WhenAction` is the
commit boundary. See the catalogue for adjustable bounds and cancellation.

```cpp
UiSlider amount;
amount.SetRange(0, 100).SetStep(1).SetValue(40);
UiRangeSlider interval;
interval.SetRange(0, 100).SetStep(1).SetValues(20, 80);
UiSliderEdit amount_field;
amount_field.SetRange(0, 100).SetStep(1).SetValue(40);
UiRangeSliderEdit interval_fields;
interval_fields.SetRange(0, 100).SetStep(1).SetValues(20, 80);
```

UiRangeSegments partitions a fixed domain into contiguous spans. Interior
boundaries edit neighboring spans; endpoints remain fixed. Payloads and labels
remain semantic data when direction or presentation changes.

```cpp
UiRangeSegments phases;
phases.SetRange(0, 100).SetStep(1).SetSegmentCount(3);
Vector<double> boundaries;
boundaries << 25 << 70;
phases.SetBoundaryValues(boundaries);
```

UiScrollBar exposes a position, total range and visible page extent. It does not
scroll a child automatically; connect its changes to the host, or use UiScrollPanel.

```cpp
UiScrollBar scroll;
scroll.SetRange(0, 1000, 200).SetPos(150);
```

## Progress and proportional data

UiProgressBar and UiProgressRing show one amount against a total. UiChartRing
shows several proportional segment values; its labels are data, not a built-in
legend or selection model. Keep a playback/worker's scheduling in the host.

```cpp
UiProgressBar progress;
progress.Set(35, 100);
UiProgressRing ring;
ring.Set(35, 100);
UiChartRing distribution;
distribution.AddSegment(60, "Complete").AddSegment(40, "Remaining");
```

## Spatial, date and colour choices

UiMatrixSelector selects a spatial cell (or a supported ordered pair), with
default and selected markers kept separate. UiDateTime edits local date/time;
it does not convert time zones. Pick its mode explicitly.

```cpp
UiMatrixSelector anchor;
anchor.SelectIndex(4);
UiDateTime timestamp;
timestamp.DateTimeMode().SetNow();
```

UiColorMatrix is a compact set of one to eight colours with a shared picker.
UiColorPickerMicro provides a small embeddable palette and optional editor.
UiColorPicker provides the full multi-slot editing surface. Colours are host
data and must not be recoloured by semantic theme roles. The full picker's colour
setter can notify; use `false` for silent setup.

```cpp
UiColorMatrix swatches;
swatches.SetColorCount(2).SetColor(0, Color(70, 110, 170));
swatches.SetColor(1, Color(200, 140, 70));
UiColorPickerMicro compact_picker;
compact_picker.SetColor(Color(70, 110, 170)).ShowHex(true);
UiColorPicker picker;
picker.SetSlotCount(2).SetSlotColor(0, Color(70, 110, 170), false);
```

## Choices and command menus

UiDropdown presents list-model choices. Give entries stable payloads when labels
can change; `GetData()` returns the selected payload according to its binding
contract. UiMenu presents commands, checks, radio items and nested submenus from
UiMenuModel. Its `WhenAction` includes the node and command item; the host executes
the command. A menu's command model is distinct from dropdown selection.

```cpp
UiDropdown quality;
quality.Add("Draft", 0).Add("Final", 1).Select(1);
UiMenu commands;
commands.Model().AddChild(commands.Model().Root(), UiMenuItem("Export", "export"));
```

## Surfaces and content hosts

UiPanel paints a styled surface and parents ordinary children; use a layout
child to arrange several controls. UiDirectContentHost borrows one child and adds
Fit/Fixed/Expand sizing without painted styling. UiGroupPanel borrows independent
body and header-content roots around its title/subtitle. These examples use
separate children because a Ctrl can have only one parent.

```cpp
UiPanel panel;
UiLabel panel_text;
panel_text.SetText("Preview");
panel.Add(panel_text.SizePos());
UiDirectContentHost slot;
UiLabel slot_text;
slot_text.SetText("Content");
slot.SetContent(slot_text);
UiGroupPanel group;
UiLabel body;
UiToolButton options;
group.SetTitle("Settings").SetContent(body).SetHeaderContent(options);
```

UiScrollPanel owns a fixed viewport and a `Content()` parent for scrolling
children; put logical content there, not on the outer chrome. The content extent
must exceed the bounded viewport for scrolling to be needed. For several children
use a layout inside Content().

```cpp
UiScrollPanel viewport;
UiLabel long_content;
long_content.SetText("Content larger than the viewport");
viewport.Content().Add(long_content.LeftPos(0, DPI(700)).TopPos(0, DPI(900)));
viewport.SetScrollMode(UIPANELSCROLL_AUTO);
```

### Frame Accent

Panel, GroupPanel and ScrollPanel support the shared Frame Accent decoration.
Choose any combination of Top, Bottom, Left and Right. Thickness and colour are
independent of the ordinary frame; it follows the same corner curvature and
paints just inside that frame. GroupPanel preserves the centered header gaps;
ScrollPanel's accent belongs to its fixed surface and does not scroll with content.

```cpp
UiPanel panel;
UiGroupPanel group;
UiScrollPanel viewport;
panel.SetFrameAccent(StyledFrameAccent::Top, DPI(2), Color(70, 110, 170));
group.SetFrameAccent(StyledFrameAccent::Top | StyledFrameAccent::Bottom,
                     DPI(3), Color(70, 110, 170), 180);
viewport.SetFrameAccent(StyledFrameAccent::Left, DPI(2));
// Null colour follows the resolved state frame colour. No edges means off.
panel.ClearFrameAccent();
```

The theme/style form is `style.metrics.frame_accent`, containing `edges`,
`thickness`, `color` and `alpha`. It defaults to no edges and does not change
measurement, padding or hit testing. See [Theme](02_UI_THEME_GUIDE.md) for
snapshot/inheritance semantics and [Drawing](07_UI_DRAWING_GUIDE.md) for custom
renderers. Specialized painters must consume the shared helper; merely exposing
a metrics field does not imply every subpart paints it.

## Cards and media

UiTitleCard composes a title, subtitle, optional media and adjacent content cell.
UiMediaCard presents media with optional prepared header/footer, tags and overlay.
Neither control loads files or owns project/media semantics. The host supplies an
Image and handles choose/drop actions. For dense Gallery/List tiles, use the
shared UiMediaCardRender presentation rather than allocating a Ctrl per record.

```cpp
UiTitleCard heading;
heading.SetTitle("Project Aurora").SetSubTitle("Review the latest shots");
UiMediaCard card;
card.SetTitle("SH030 take 03").SetSubTitle("1536 x 864");
// Once the host has an Image preview: card.SetImage(preview);
```

UiColorProbe displays host-supplied raw channel values and a display swatch;
sampling and colour conversion remain with the host. UiPlaybackBar emits transport,
seek and range requests; it does not decode media or run a playback clock.

```cpp
UiColorProbe probe;
UiColorSample sample;
sample.r = 0.2; sample.g = 0.4; sample.b = 0.8;
sample.swatch = Color(51, 102, 204);
sample.valid = true;
probe.SetSample(sample).SetMode(UiColorProbe::Mode::Point);
UiPlaybackBar transport;
if(transport.SetFrames(1001, 1100))
    transport.SetPosition(1025).SetSelection(1010, 1090);
```

## Pages, disclosure and split panes

UiStack owns exclusive page visibility; keys are useful for host navigation.
UiTab adds a visible tab strip around that page-selection behavior. UiAccordion
owns collapsible sections; populate each section's content parent. Parent children
must outlive the host binding, and hidden pages must not be shown by another layout.

```cpp
UiStack pages;
UiPanel overview, details;
pages.AddPage(overview, "overview");
pages.AddPage(details, "details");
pages.SetActiveKey("details");
UiTab tabs;
UiPanel first_tab, second_tab;
tabs.Add(first_tab, "Overview");
tabs.Add(second_tab, "Details");
tabs.SetActiveTab(0);
UiAccordion sections;
UiLabel section_text;
int section = sections.AddSection("Advanced", false);
sections.GetSectionContent(section).Add(section_text.SizePos());
sections.Open(section, true);
```

UiSplitter sizes panes around interactive handles. UiQuadSplitter composes four
panes using the same splitter behavior. Pane contents remain ordinary borrowed
controls; sizes/proportions and persistence policy belong to the host.

```cpp
UiSplitter split;
UiPanel left, right;
split.Set(left, right);
UiQuadSplitter quad;
UiPanel top_left, top_right, bottom_left, bottom_right;
quad.Set(top_left, top_right, bottom_left, bottom_right);
```

## Layout controls

UiBoxLayout flows an ordered row/column with Fit/Fixed/Expand item policy.
UiGridLayout assigns logical cells, while UiAbsoluteLayout uses exact local
rectangles. These controls lay out borrowed children; they do not add a semantic
page/selection model. Choose one layout authority for each child.

```cpp
UiBoxLayout column(UiDirection::V);
UiLabel title;
UiButton action;
column.SetGap(DPI(8));
column.Add(title).Fit();
column.Add(action).Fit();
UiGridLayout grid;
UiLabel cell;
grid.SetGridSize(2, 2);
grid.Add(cell, 0, 0, true);
UiAbsoluteLayout exact;
UiLabel positioned;
exact.Add(positioned, RectC(DPI(12), DPI(12), DPI(160), DPI(32)));
```

## Model-backed views

UiList and UiGallery can share one UiListModel while keeping view selection and
layout independent. List is sequential, Gallery is a uniform tile view. Populate
the model before binding, keep it alive longer than both views, and use ranged
notifications for bulk updates. A renderer supplies presentation; no Ctrl per item
is needed. Bounded viewports keep large models from determining window size.

```cpp
UiListModel items; // Declare before the views: it outlives their binding.
items.Add("Shot 010", "sh010");
items.Add("Shot 020", "sh020");
UiList list;
UiGallery gallery;
list.SetModel(items);
gallery.SetModel(items).SetItemSize(Size(DPI(160), DPI(120)));
```

UiTree retains hierarchical identities and expansion/disclosure. UiTable has row/
column coordinates, headers and typed editing; do not flatten either into a list
just to reuse its appearance. Request callbacks allow a host to own mutations.

```cpp
UiTree tree;
tree.Model().AddChild(tree.Model().Root(), UiModelItem("Project", "project"));
UiTable table;
table.Model().SetSize(3, 2);
table.Model().SetCellValue(0, 0, "Shot 010");
table.SetRowHeight(DPI(28)).SetHeaderHeight(DPI(30));
```

## Curves, documents and graphs

UiBezierCurveEditor edits a ShadowCurve. UiBezierCurveField adds formula/copy
composition around the same curve. Read the curve on committed user edits; an
editor is not an animation clock.

```cpp
UiBezierCurveEditor curve;
curve.SetYRange(0, 1).SetEditable(true);
UiBezierCurveField curve_field;
curve_field.SetCurve(curve.GetCurve()).SetShowFormula(true).SetShowCopy(true);
```

UiDoc owns an internal document core by default and offers text editing over that
authority. Structured document edits, rich marks, resources and transactions are
covered in [Models](03_UI_MODEL_GUIDE.md). UiNodeGraph defaults to an internal graph
model; nodes, ports and edges belong there. Its camera and retained presentation
do not change the graph's topology. See [Graph usage](08_UIGRAPH_GUIDE.md).

```cpp
UiDoc document;
document.SetText("Project Aurora\nReview notes");
UiNodeGraph graph;
graph.Model().AddNode("Input", Pointf(0, 0));
graph.Model().AddNode("Output", Pointf(240, 0));
graph.SetEditable(true);
```

## Optional file selection packages

UiFileBrowser is an embeddable browser with asynchronous scanning and host-driven
accept/cancel callbacks. Declare a direct dependency on `Ui/UiFileBrowser` and
include `<Ui/UiFileBrowser/UiFileBrowser.h>`; the base Ui package does not pull it
in. The host supplies a real folder path and decides what acceptance does.

```cpp
UiFileBrowser browser;
browser.SetFolder(GetHomeDirectory());
```

UiOsFileDialog is a platform-native modal wrapper, not a Ctrl or an embeddable
surface. Declare `Ui/UiOsFileDialog` and include
`<Ui/UiOsFileDialog/UiOsFileDialog.h>`. Cancellation returns false; supported OS
backends still need platform-specific acceptance.

```cpp
UiOsFileDialog open;
open.SetMode(UiOsFileDialog::Mode::OpenFile).SetTitle("Choose an image");
open.AddFilter("Images", "*.png;*.jpg;*.jpeg");
if(open.Execute()) {
    String chosen_path = open.GetPath();
    // Load through a host-owned provider with its declared codec dependencies.
}
```
