// UiBoxLayout logical visibility: windowless public-API geometry/state regressions.
// No normal window, focus activation, input, app store or theme recipe is changed.
#include <Ui/UiBoxLayout.h>
#include <Ui/UiMeasure.h>
#include <Ui/UiTheme.h>
#include <Ui/UiLineEdit.h>
#include <Ui/UiMultiEdit.h>
#include <Ui/UiButton.h>

using namespace Upp;

namespace {
int checks = 0, failures = 0;
String current_case;
ValueArray failed_checks, case_records;

void Check(bool ok, const String& message)
{
    ++checks;
    if(!ok) {
        ++failures;
        String detail = current_case + ": " + message;
        failed_checks.Add(detail);
        Cout() << "FAIL " << detail << '\n';
    }
}

template<class F> void RunCase(const String& name, F run)
{
    current_case = name;
    int before_checks = checks, before_failures = failures;
    run();
    ValueMap row;
    row.Add("case", name);
    row.Add("checks", checks - before_checks);
    row.Add("failures", failures - before_failures);
    case_records.Add(row);
}

class Probe : public Ctrl {
public:
    Size minimum;
    mutable int measurements = 0;
    int* destroyed = nullptr;
    Probe(int width, int height, int* deaths = nullptr)
        : minimum(width, height), destroyed(deaths) {}
    ~Probe() override { if(destroyed) ++*destroyed; }
    Size GetMinSize() const override { ++measurements; return minimum; }
};

bool Within(const Ctrl& child, const Ctrl& parent)
{
    Rect r = child.GetRect();
    return r.left >= 0 && r.top >= 0 && r.right <= parent.GetSize().cx
           && r.bottom <= parent.GetSize().cy;
}

void DefaultGeometry(UiDirection direction)
{
    UiBoxLayout box(direction);
    Probe a(20, 10), b(30, 12);
    box.SetGap(5).SetInset(2);
    box.Add(a).Fixed(20);
    box.Add(b).Fixed(30);
    box.SetRect(0, 0, 100, 100);
    box.Layout();
    Check(box.IsItemLogicallyVisible(0) && box.IsItemLogicallyVisible(1),
          "existing Add defaults to logical participation");
    if(direction == UiDirection::H) {
        Check(a.GetRect() == RectC(2, 2, 20, 96), "legacy horizontal first rectangle");
        Check(b.GetRect() == RectC(27, 2, 30, 96), "legacy horizontal second rectangle");
        Check(box.GetPreferredSize() == Size(59, 16), "legacy horizontal preferred extent");
        Check(box.GetUsedSize() == Size(55, 96), "legacy horizontal used extent");
    }
    else {
        Check(a.GetRect() == RectC(2, 2, 96, 20), "legacy vertical first rectangle");
        Check(b.GetRect() == RectC(2, 27, 96, 30), "legacy vertical second rectangle");
        Check(box.GetPreferredSize() == Size(34, 59), "legacy vertical preferred extent");
        Check(box.GetUsedSize() == Size(96, 55), "legacy vertical used extent");
    }
    int count = box.GetLayoutCallCount();
    a.Hide();
    box.Layout();
    Check(a.IsShown(), "raw Ctrl.Hide retains its existing temporary semantics");
    Check(box.IsItemLogicallyVisible(0), "raw Hide does not alter requested participation");
    Check(box.SetItemVisible(0, true), "unchanged valid setter reports success");
    Check(box.GetLayoutCallCount() == count + 1, "unchanged setter does not lay out");
}

void HiddenSizing(UiDirection direction, int sizing)
{
    UiBoxLayout box(direction);
    Probe a(20, 10), b(30, 12), c(40, 14);
    box.SetGap(5).SetInset(2);
    auto first = box.Add(a);
    auto middle = box.Add(b);
    auto last = box.Add(c);
    if(sizing == 0) {
        first.Fit(); middle.Fit(); last.Fit();
    }
    else if(sizing == 1) {
        first.Fixed(20); middle.Fixed(30); last.Fixed(40);
    }
    else {
        first.Expand(1); middle.Expand(7); last.Expand(1);
    }
    box.SetRect(0, 0, 100, 100);
    box.Layout();
    middle.Visible(false);
    Check(!b.IsShown() && !box.IsItemVisible(1) && box.GetItemRect(1).IsEmpty(),
          "hidden middle has no rendered allocation");
    Check(b.GetParent() == &box && box.FindItem(b) == 1 && box.GetItemCount() == 3,
          "hidden control remains attached at the same logical index");
    Rect ra = a.GetRect(), rc = c.GetRect();
    Check(direction == UiDirection::H ? rc.left == ra.right + 5 : rc.top == ra.bottom + 5,
          "hidden middle leaves exactly one gap");
    if(sizing == 2) {
        Check(direction == UiDirection::H ? ra.GetWidth() + rc.GetWidth() == 91
                                          : ra.GetHeight() + rc.GetHeight() == 91,
              "hidden expand weight receives none of the available extent");
    }
    else {
        int main_a = direction == UiDirection::H ? 20 : sizing == 1 ? 20 : 10;
        int main_c = direction == UiDirection::H ? 40 : sizing == 1 ? 40 : 14;
        Size expected = direction == UiDirection::H ? Size(main_a + 5 + main_c + 4, 18)
                                                     : Size(44, main_a + 5 + main_c + 4);
        Check(box.GetPreferredSize() == expected, "hidden middle excluded from preferred size");
    }
    first.Visible(false);
    Check(direction == UiDirection::H ? c.GetRect().left == 2 : c.GetRect().top == 2,
          "hidden first leaves no leading gap");
    last.Visible(false);
    Check(box.GetPreferredSize() == Size(4, 4) && box.GetUsedSize() == Size(0, 0),
          "all hidden leaves only authored inset and no used extent");
    Check(!a.IsShown() && !b.IsShown() && !c.IsShown(), "all children hidden without detachment");
    first.Visible();
    Check(box.IsItemLogicallyVisible(0) && a.IsShown(), "fluent default argument restores first");
    Check(direction == UiDirection::H ? a.GetRect().left == 2 : a.GetRect().top == 2,
          "hidden last adds no trailing or leading allocation");
    middle.Visible();
    last.Visible();
    Check(a.IsShown() && b.IsShown() && c.IsShown(), "all three restore after hide/show");
}

void InvalidAndBatch()
{
    UiBoxLayout box(UiDirection::V);
    Probe a(20, 10), b(30, 12);
    box.SetGap(5);
    box.Add(a).Fit();
    box.Add(b).Fit();
    box.SetRect(0, 0, 100, 100);
    box.Layout();
    int before = box.GetLayoutCallCount();
    Check(!box.SetItemVisible(-1, false) && !box.SetItemVisible(2, false),
          "invalid indices reject without mutation");
    Check(!box.IsItemLogicallyVisible(-1) && !box.IsItemLogicallyVisible(2),
          "invalid requested-state queries are false");
    UiBoxLayout::ItemRef invalid;
    invalid.Visible(false);
    box.ItemAt(200).Visible(false);
    Check(box.GetLayoutCallCount() == before, "invalid/default handles cause no layout");
    Check(box.SetItemVisible(0, true) && box.GetLayoutCallCount() == before,
          "valid unchanged request is a successful no-op");
    box.PauseLayout().PauseLayout();
    box.SetItemVisible(0, false);
    box.SetItemVisible(1, false);
    Check(!box.IsItemLogicallyVisible(0) && a.IsShown() && box.IsItemVisible(0),
          "requested state changes while rendered cache remains deferred");
    Check(box.GetPreferredSize() == Size(0, 0), "paused preferred measurement sees new request");
    Check(box.GetLayoutCallCount() == before, "nested batch emits no intermediate layout");
    box.ResumeLayout();
    Check(box.GetLayoutCallCount() == before, "inner resume remains paused");
    box.ResumeLayout();
    Check(box.GetLayoutCallCount() == before + 1 && !a.IsShown() && !b.IsShown(),
          "outer resume performs one layout and hides both");
    box.PauseLayout();
    box.SetItemVisible(0, true);
    box.ResumeLayout(false);
    Check(!a.IsShown() && box.IsItemLogicallyVisible(0), "deferred resume retains pending request");
    box.Layout();
    Check(a.IsShown() && box.IsItemVisible(0), "manual layout flushes a deferred request");
}

void WrappingAndCache(UiBoxWrap wrap)
{
    UiBoxLayout box(UiDirection::H);
    Probe a(40, 10), b(40, 10), c(40, 10);
    box.SetGap(5).SetWrap(wrap).SetWrapAutoResize(true);
    if(wrap == UiBoxWrap::Snap) {
        Vector<int> slots; slots.Add(40); slots.Add(40);
        box.SetWrapSnapSizes(slots).SetWrapSnapCount(2);
    }
    box.Add(a).Fit(); box.Add(b).Fit(); box.Add(c).Fit();
    box.SetRect(0, 0, 90, 80);
    box.Layout();
    Check(box.MeasureHeightForWidth(90) == 25, "three items wrap to two rows");
    Check(box.MeasureHeightForWidth(90) == 25, "warm measurement cache agrees");
    box.SetItemVisible(1, false);
    Rect before_a = a.GetRect(), before_c = c.GetRect();
    Check(box.MeasureHeightForWidth(90) == 10, "hide invalidates cached wrapping height");
    Check(a.GetRect() == before_a && c.GetRect() == before_c && !b.IsShown(),
          "measurement preserves rendered rectangles and hidden child state");
    Check(c.GetRect().top == 0 && c.GetRect().left == 45,
          "hidden item consumes no wrap/snap slot");
    Check(box.GetMinWrapWidth() == 40 && box.GetPreferredSize() == Size(85, 10),
          "minimum/preferred wrapping excludes hidden item");
    b.measurements = 0;
    box.GetMinSize(); box.GetPreferredSize(); box.GetMinWrapWidth();
    box.MeasureHeightForWidth(90); box.MeasureHeightForWidth(50);
    Check(b.measurements == 0, "hidden child is not queried by minimum or cache signature");
    box.SetItemVisible(1, true);
    Check(box.MeasureHeightForWidth(90) == 25 && b.IsShown(), "show restores wrapping and cache");
    box.PauseLayout();
    box.SetItemVisible(0, false); box.SetItemVisible(1, false); box.SetItemVisible(2, false);
    box.ResumeLayout();
    Check(box.MeasureHeightForWidth(90) == 0 && box.GetMinWrapWidth() == 0,
          "all hidden has no phantom row or minimum wrap width");
    UiBoxLayout snap(UiDirection::H);
    Probe hidden(1, 10), only(1, 10);
    Vector<int> unequal; unequal.Add(40); unequal.Add(90);
    snap.SetWrap(UiBoxWrap::Snap).SetWrapSnapSizes(unequal).SetWrapAutoResize(true);
    snap.Add(hidden).Fit().Visible(false); snap.Add(only).Fit();
    snap.SetRect(0, 0, 100, 30); snap.Layout();
    Check(snap.GetMinWrapWidth() == 40 && only.GetRect().GetWidth() == 40,
          "hidden leading item does not retain the second snap-size slot");
}

void BreakAndSpacer(UiDirection direction)
{
    UiBoxLayout box(direction);
    Probe a(20, 10), b(20, 10);
    box.SetGap(5).SetWrap(direction == UiDirection::H);
    box.Add(a).Fit();
    auto split = box.AddBreak();
    box.Add(b).Fit();
    box.SetRect(0, 0, 100, 80); box.Layout();
    Check(b.GetRect().top > a.GetRect().top, "visible break retains existing boundary behavior");
    split.Visible(false);
    Check(direction == UiDirection::H ? b.GetRect().top == 0 && b.GetRect().left == 25
                                      : b.GetRect().top == 15,
          "hidden break contributes no wrap or extra gap");
    split.Visible();
    Check(b.GetRect().top > a.GetRect().top, "break can be restored");
    UiBoxLayout spaced(direction);
    Probe first(20, 10), last(20, 10);
    spaced.SetGap(5);
    spaced.Add(first).Fit();
    auto spacer = spaced.AddSpacer().Fixed(12);
    spaced.Add(last).Fit();
    spaced.SetRect(0, 0, 100, 80); spaced.Layout();
    spacer.Visible(false);
    Check(direction == UiDirection::H ? last.GetRect().left == 25 : last.GetRect().top == 15,
          "hidden spacer contributes neither extent nor ghost gap");
    Check(spaced.GetPreferredSize() == (direction == UiDirection::H ? Size(45, 10) : Size(20, 25)),
          "preferred size excludes hidden spacer");
    spacer.Visible();
    Check(direction == UiDirection::H ? last.GetRect().left == 42 : last.GetRect().top == 32,
          "fixed spacer extent and adjacent gaps restore");
}

int NonWhitePixels(const Image& image)
{
    int count = 0;
    const RGBA* pixels = image.Begin();
    for(int i = 0; i < image.GetWidth() * image.GetHeight(); ++i)
        if(pixels[i].r != 255 || pixels[i].g != 255 || pixels[i].b != 255)
            ++count;
    return count;
}

void SeparatorPaint()
{
    UiBoxLayout box(UiDirection::H);
    auto line = box.AddSpacer().Fixed(20).LineEnabled().LineColor(Red());
    box.SetRect(0, 0, 100, 40); box.Layout();
    ImageDraw visible(Size(100, 40));
    visible.DrawRect(0, 0, 100, 40, White()); box.Paint(visible);
    Check(NonWhitePixels(Image(visible)) > 0, "visible separator paints a native line");
    line.Visible(false);
    ImageDraw hidden(Size(100, 40));
    hidden.DrawRect(0, 0, 100, 40, White()); box.Paint(hidden);
    Check(NonWhitePixels(Image(hidden)) == 0, "hidden separator emits no pixels");
}

void EmptyAllocationRestores(UiDirection direction)
{
    UiBoxLayout box(direction);
    Probe child(20, 10);
    auto item = box.Add(child);
    if(direction == UiDirection::H) item.Fit(); else item.Expand();
    box.SetRect(0, 0, 40, 0); box.Layout();
    Check(box.IsItemLogicallyVisible(0) && !box.IsItemVisible(0) && !child.IsShown(),
          "zero allocation auto-hides while requested state remains true");
    Check(box.GetMinSize().cx >= 20, "auto-hidden child still contributes its minimum");
    box.SetRect(0, 0, 40, 20); box.Layout();
    Check(child.IsShown() && box.IsItemVisible(0), "growing restores an auto-hidden child");
    item.Visible(false);
    box.SetRect(0, 0, 100, 100); box.Layout();
    Check(!child.IsShown() && !box.IsItemLogicallyVisible(0),
          "growing never restores an explicitly hidden item");
    child.Show(); box.Layout();
    Check(!child.IsShown(), "raw Show cannot override logical exclusion on next layout");
}

class ParentProbe : public Ctrl {
public:
    UiBoxLayout box;
    Probe child{80, 25};
    Size observed_minimum;
    int layouts = 0;
    ParentProbe() { Add(box.SizePos()); box.Add(child).Fit(); }
    void Layout() override {
        ++layouts;
        box.SetRect(0, 0, GetSize().cx, GetSize().cy);
        observed_minimum = box.GetMinSize();
    }
};

void ParentInvalidation()
{
    ParentProbe host;
    host.SetRect(0, 0, 120, 80); host.Layout();
    int before = host.layouts;
    host.box.SetItemVisible(0, false);
    Check(host.layouts > before && host.observed_minimum == Size(0, 0),
          "visibility change notifies parent measurement/layout");
    before = host.layouts;
    host.box.SetItemVisible(0, false);
    Check(host.layouts == before, "unchanged request does not notify parent");
    host.box.PauseLayout();
    host.box.SetItemVisible(0, true);
    Check(host.layouts == before, "parent notification waits for batch completion");
    host.box.ResumeLayout();
    Check(host.layouts > before && host.observed_minimum == Size(80, 25),
          "batch completion restores parent minimum");
}

void Ownership()
{
    int destroyed = 0;
    {
        UiBoxLayout box;
        Probe child(20, 10, &destroyed);
        box.Add(child).Fit();
        box.SetRect(0, 0, 100, 40);
        box.SetItemVisible(0, false); box.SetItemVisible(0, true);
        Check(destroyed == 0 && child.GetParent() == &box,
              "visibility does not delete or detach a borrowed child");
    }
    Check(destroyed == 1, "caller-owned object is destroyed exactly once");
}

// Ui-only fixtures of the two real Flow caller contracts. They do not instantiate
// CineFlow pages/services or qualify the approved app composition.
void FlowSettingsContract()
{
    UiBoxLayout content(UiDirection::V), flow_row(UiDirection::H);
    UiLineEdit title, flow_name;
    UiMultiEdit writer;
    UiButton add, rename;
    int actions = 0;
    title.SetTextUtf8("Unsaved project title");
    flow_name.SetTextUtf8("Unsaved episode name");
    writer.SetTextUtf8("Writer draft retained across settings visibility.");
    title.WhenAction = [&] { ++actions; };
    flow_name.WhenAction = [&] { ++actions; };
    add.SetText("Add flow"); rename.SetText("Rename");
    flow_row.SetGap(7).SetWrap(true).SetWrapAutoResize(true);
    flow_row.Add(flow_name).Expand().MinWidth(150);
    flow_row.Add(add).Fixed(max(90, add.GetMinSize().cx));
    flow_row.Add(rename).Fixed(max(85, rename.GetMinSize().cx));
    content.SetGap(8);
    content.Add(title).Fixed(max(44, title.GetMinSize().cy));
    auto settings = content.Add(flow_row).Fixed(74).MinMaxCross(780);
    content.Add(writer).Expand();
    Size expanded = content.GetPreferredSize();
    settings.Visible(false);
    Size collapsed = content.GetPreferredSize();
    Check(expanded.cx >= 780 && collapsed.cx < expanded.cx,
          "collapsed settings removes its observed wide minimum contribution");
    Check(expanded.cy - collapsed.cy == 82,
          "collapsed fixed74 settings removes its height and adjacent gap");
    for(Size viewport : {Size(1360, 900), Size(880, 640)}) {
        content.SetRect(0, 0, viewport.cx - 182, viewport.cy - 180); content.Layout();
        Rect writer_before = writer.GetRect();
        Check(!flow_row.IsShown() && Within(writer, content) && Within(title, content),
              "collapsed settings leaves visible title/draft contained");
        settings.Visible();
        Check(flow_row.IsShown() && flow_name.GetParent() == &flow_row,
              "expanding retains controls and ownership");
        settings.Visible(false);
        Check(writer.GetRect() == writer_before, "collapse restores the same draft allocation");
        Check(title.GetTextUtf8() == "Unsaved project title"
              && flow_name.GetTextUtf8() == "Unsaved episode name"
              && writer.GetTextUtf8() == "Writer draft retained across settings visibility.",
              "settings toggles preserve actual native editor buffers");
    }
    Check(actions == 0, "settings visibility emits no native authoring action");
}

void FlowTreatmentContract()
{
    UiBoxLayout root(UiDirection::V), actions(UiDirection::H);
    UiMultiEdit raw, short_draft, full_draft;
    UiButton save, discard, secondary;
    int authored = 0;
    raw.SetTextUtf8("Raw structure\nCanonical source remains unchanged.");
    raw.SetEditable(false);
    short_draft.SetTextUtf8("Unsaved short narrative\nSecond line.");
    full_draft.SetTextUtf8("Unsaved full narrative\nIndependent version.");
    short_draft.WhenAction = [&] { ++authored; };
    full_draft.WhenAction = [&] { ++authored; };
    save.SetText("Save working text"); discard.SetText("Discard"); secondary.SetText("");
    save.WhenAction = [&] { ++authored; };
    actions.SetGap(8);
    auto save_item = actions.Add(save).Fixed(max(145, save.GetMinSize().cx));
    auto discard_item = actions.Add(discard).Fixed(max(110, discard.GetMinSize().cx));
    auto empty_item = actions.Add(secondary).Fixed(175).Visible(false);
    root.SetGap(8);
    auto raw_item = root.Add(raw).Expand();
    auto short_item = root.Add(short_draft).Expand();
    auto full_item = root.Add(full_draft).Expand();
    auto action_item = root.Add(actions).Fixed(max(36, actions.GetMinSize().cy));
    auto select = [&](int mode) {
        root.PauseLayout(); actions.PauseLayout();
        raw_item.Visible(mode == 0); short_item.Visible(mode == 1); full_item.Visible(mode == 2);
        save_item.Visible(mode != 0); discard_item.Visible(mode != 0);
        empty_item.Visible(false); action_item.Visible(mode != 0);
        actions.ResumeLayout(); root.ResumeLayout();
    };
    for(Size viewport : {Size(1360, 900), Size(880, 640)}) {
        root.SetRect(0, 0, viewport.cx - 182, viewport.cy - 250);
        for(int mode : {0, 1, 2, 1, 0}) {
            select(mode); root.Layout();
            Check(raw.IsShown() == (mode == 0) && short_draft.IsShown() == (mode == 1)
                  && full_draft.IsShown() == (mode == 2), "exactly selected content remains shown");
            Check(!secondary.IsShown() && actions.GetItemRect(2).IsEmpty(),
                  "unavailable secondary action leaves no blank button allocation");
            Ctrl& active = mode == 0 ? static_cast<Ctrl&>(raw)
                                   : mode == 1 ? static_cast<Ctrl&>(short_draft)
                                               : static_cast<Ctrl&>(full_draft);
            Check(Within(active, root) && active.GetRect().GetHeight() > 0,
                  "selected content keeps a nonempty contained viewport");
            Check(mode == 0 || discard.GetRect().left == save.GetRect().right + 8,
                  "hidden secondary does not disturb remaining action spacing");
            Check(short_draft.GetTextUtf8() == "Unsaved short narrative\nSecond line."
                  && full_draft.GetTextUtf8() == "Unsaved full narrative\nIndependent version."
                  && raw.GetTextUtf8() == "Raw structure\nCanonical source remains unchanged.",
                  "mode switching preserves independent native text buffers");
        }
    }
    Check(raw.GetParent() == &root && short_draft.GetParent() == &root
          && full_draft.GetParent() == &root && secondary.GetParent() == &actions,
          "mode switching never reparents content or actions");
    Check(authored == 0, "mode switching emits no authoring or save event");
}
}

GUI_APP_MAIN
{
    try {
        const auto& args = CommandLine();
        if(args.GetCount() != 1)
            throw Exc("Usage: UiBoxVisibilityTests <owned-evidence-directory>");
        String output = args[0];
        if(!RealizeDirectory(output))
            throw Exc("Cannot create evidence directory");
        for(UiDirection direction : {UiDirection::H, UiDirection::V}) {
            String axis = direction == UiDirection::H ? "H" : "V";
            RunCase(axis + "-default", [&] { DefaultGeometry(direction); });
            for(int sizing = 0; sizing < 3; ++sizing)
                RunCase(axis + "-sizing-" + AsString(sizing), [&] { HiddenSizing(direction, sizing); });
            RunCase(axis + "-break-spacer", [&] { BreakAndSpacer(direction); });
            RunCase(axis + "-empty-restore", [&] { EmptyAllocationRestores(direction); });
        }
        RunCase("invalid-noop-batching", InvalidAndBatch);
        RunCase("flow-wrap-cache", [&] { WrappingAndCache(UiBoxWrap::Flow); });
        RunCase("snap-wrap-cache", [&] { WrappingAndCache(UiBoxWrap::Snap); });
        RunCase("separator-paint", SeparatorPaint);
        RunCase("parent-invalidation", ParentInvalidation);
        RunCase("borrowed-ownership", Ownership);
        for(UiThemeMode mode : {UiThemeMode::Light, UiThemeMode::Dark}) {
            UiTheme::Set(UiThemePreset::Minimal, mode);
            String name = mode == UiThemeMode::Light ? "light" : "dark";
            RunCase(name + "-flow-settings-contract", FlowSettingsContract);
            RunCase(name + "-flow-treatment-contract", FlowTreatmentContract);
        }
        ValueMap report;
        report.Add("schema", "ui.box-visibility-tests.r1");
        report.Add("checks", checks); report.Add("failures", failures);
        report.Add("dpi100", DPI(100)); report.Add("cases", case_records);
        report.Add("failed_checks", failed_checks);
        report.Add("normalWindowJourney", "NOT RUN");
        report.Add("nativeInputAndFocus", "NOT RUN");
        report.Add("actualCineFlowPageQualification", "NOT RUN");
        report.Add("scope", "Actual public UiBoxLayout, memory Draw separator, and Ui-only Flow caller contract fixtures");
        if(!SaveFile(AppendFileName(output, "RESULT.json"), AsJSON(report, true)))
            throw Exc("Cannot write result");
        Cout() << "UiBoxVisibilityTests: " << checks << " checks, " << failures << " failures\n";
        if(checks == 0 || failures)
            SetExitCode(1);
    }
    catch(const Exc& error) {
        Cerr() << "UiBoxVisibilityTests ERROR: " << error << '\n';
        SetExitCode(2);
    }
}
