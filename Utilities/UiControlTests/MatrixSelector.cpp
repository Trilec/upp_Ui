#include <Core/Core.h>
#include <Ui/Ui.h>

using namespace Upp;

struct TestCtx {
    int checks = 0;
    int fails = 0;
    void Expect(bool cond, const String& msg) {
        checks++;
        if(!cond) { fails++; Cout() << "[FAIL] " << msg << "\n"; }
    }
    void Section(const char* title) { Cout() << "\n=== " << title << " ===\n"; }
};

static void TestPresets(TestCtx& t)
{
    t.Section("Preset contract");
    UiMatrixSelector s;
    t.Expect(s.GetPreset() == UiMatrixPreset::Position9, "default preset is Position9");
    t.Expect(s.GetRows() == 3 && s.GetColumns() == 3 && s.GetCellCount() == 9, "Position9 is 3x3");
    t.Expect(s.GetSelectedIndex() == 4 && s.GetData() == Value("center"), "Position9 defaults center");
    t.Expect(s.GetCell(0).short_label == "TL" && s.GetCell(8).short_label == "BR", "Position9 labels");

    s.SetPreset(UiMatrixPreset::Compass8);
    t.Expect(s.GetCellCount() == 9 && !s.GetCell(4).enabled && s.GetCell(4).visible, "Compass8 center disabled");
    t.Expect(s.GetCell(0).glyph == UiMatrixGlyph::ArrowNW && s.GetCell(8).glyph == UiMatrixGlyph::ArrowSE, "Compass arrows");
    t.Expect(s.GetData() == Value("east"), "Compass default selection valid");

    s.SetPreset(UiMatrixPreset::Region5);
    t.Expect(!s.GetCell(0).visible && !s.GetCell(2).visible && !s.GetCell(6).visible && !s.GetCell(8).visible, "Region5 corners masked");
    t.Expect(s.GetCell(1).short_label == "TOP" && s.GetCell(7).short_label == "BTM", "Region5 labels");

    s.SetPreset(UiMatrixPreset::QuadPair);
    t.Expect(s.GetRows() == 2 && s.GetColumns() == 2 && s.GetCellCount() == 4, "QuadPair is 2x2");
    t.Expect(s.IsPairSelection(), "QuadPair defaults to pair selection mode");
    t.Expect(!s.HasPairStart() && !s.HasCompletePair(), "QuadPair starts without pair endpoints");

    s.SetPreset(UiMatrixPreset::Cardinal4);
    t.Expect(s.GetRows() == 3 && s.GetColumns() == 3 && s.GetCellCount() == 9,
             "Cardinal4 retains directional 3x3 geometry");
    t.Expect(s.GetData() == Value("top") && s.GetSelectedIndex() == 1,
             "Cardinal4 defaults to top");
    t.Expect(!s.GetCell(0).visible && !s.GetCell(4).visible &&
             !s.GetCell(8).visible,
             "Cardinal4 hides diagonals and centre");
    t.Expect(s.GetCell(1).glyph == UiMatrixGlyph::ArrowN &&
             s.GetCell(3).glyph == UiMatrixGlyph::ArrowW &&
             s.GetCell(5).glyph == UiMatrixGlyph::ArrowE &&
             s.GetCell(7).glyph == UiMatrixGlyph::ArrowS,
             "Cardinal4 exposes four directional glyphs");
}

static void TestSelectionAndData(TestCtx& t)
{
    t.Section("Selection and data");
    UiMatrixSelector s;
    int changing = 0, actions = 0;
    s.WhenChanging = [&] { changing++; };
    s.WhenAction = [&] { actions++; };

    s.SelectIndex(0);
    t.Expect(s.GetSelectedIndex() == 0 && s.GetData() == Value("top_left"), "programmatic single selection");
    t.Expect(changing == 0 && actions == 0, "programmatic selection silent");
    s.SelectIndex(8, true);
    t.Expect(s.GetData() == Value("bottom_right") && changing == 1 && actions == 1, "explicit action events");
    s.SetData(Value("left"));
    t.Expect(s.GetSelectedIndex() == 3 && s.GetSelectedLabel() == "Left", "SetData semantic lookup");
    s.SetCell(3, "WEST", "West side", 42);
    s.SetData(42);
    t.Expect(s.GetSelectedLabel() == "West side" && s.GetData() == Value(42), "labels and values independent");
}

static void TestGeometry(TestCtx& t)
{
    t.Section("Geometry and hit testing");
    UiMatrixSelector s;
    s.SetRect(0, 0, 420, 250);
    Rect matrix = s.GetMatrixRect();
    Rect readout = s.GetReadoutRect();
    t.Expect(!matrix.IsEmpty() && matrix.GetWidth() == matrix.GetHeight(), "matrix remains square");
    t.Expect(!readout.IsEmpty() && readout.left > matrix.right, "readout remains right of matrix");
    for(int i = 0; i < 9; i++)
        t.Expect(s.HitTest(s.GetCellRect(i).CenterPoint()) == i, Format("cell %d hit-test", i));
    s.SetPreset(UiMatrixPreset::Region5);
    t.Expect(s.HitTest(s.GetCellRect(0).CenterPoint()) == -1, "hidden corner is not hit-testable");
    t.Expect(s.HitTest(s.GetCellRect(4).CenterPoint()) == 4, "center hit-testable");
    s.SetCellGap(DPI(6));
    t.Expect(s.GetCellRect(4).top - s.GetCellRect(1).bottom >= DPI(6), "gap separates cells");
}

static void TestPair(TestCtx& t)
{
    t.Section("Ordered pair contract");
    UiMatrixSelector s;
    s.SetPreset(UiMatrixPreset::QuadPair);
    t.Expect(s.GetReadoutText() == "Choose first point", "pair readout starts with first prompt");

    s.SetPair(0, 3);
    t.Expect(s.HasCompletePair() && s.GetPairStartIndex() == 0 && s.GetPairEndIndex() == 3, "ordered pair stored");
    t.Expect(s.GetPairOrientation() == UiMatrixPairOrientation::Diagonal, "opposite corners diagonal");
    t.Expect(s.GetPairOrientationName() == "Diagonal", "diagonal name exposed");
    t.Expect(s.GetPairDirectionLabel() == "Upper left -> Lower right", "pair direction preserved");
    t.Expect(s.GetReadoutText().Find("Diagonal") >= 0 && s.GetReadoutText().Find("Upper left -> Lower right") >= 0, "readout contains geometry and direction");

    Value v = s.GetData();
    t.Expect(v.Is<ValueArray>(), "pair data is ValueArray");
    ValueArray pair = v;
    t.Expect(pair.GetCount() == 2 && pair[0] == Value("a") && pair[1] == Value("d"), "pair data ordered");

    s.SetPair(3, 0);
    t.Expect(s.GetPairDirectionLabel() == "Lower right -> Upper left", "reverse direction retained");
    ValueArray rev = s.GetData();
    t.Expect(rev[0] == Value("d") && rev[1] == Value("a"), "reverse data retained");

    s.SetPair(0, 1);
    t.Expect(s.GetPairOrientation() == UiMatrixPairOrientation::Horizontal, "same row horizontal");
    s.SetPair(1, 3);
    t.Expect(s.GetPairOrientation() == UiMatrixPairOrientation::Vertical, "same column vertical");

    ValueArray input;
    input.Add(String("c")); input.Add(String("d"));
    s.SetData(input);
    t.Expect(s.GetPairStartIndex() == 2 && s.GetPairEndIndex() == 3, "pair restored from ValueArray");
    t.Expect(s.GetPairOrientation() == UiMatrixPairOrientation::Horizontal, "restored pair geometry");

    s.SetCell(2, "FROM", "Source concept", "source");
    s.SetCell(0, "TO", "Target concept", "target");
    s.SetPair(2, 0);
    t.Expect(s.GetPairDirectionLabel() == "Source concept -> Target concept", "injected labels used by direction");
    ValueArray injected = s.GetData();
    t.Expect(injected[0] == Value("source") && injected[1] == Value("target"), "injected values used by pair data");

    s.ClearPair();
    t.Expect(!s.HasPairStart() && !s.HasCompletePair(), "ClearPair clears pair");
    ValueArray start;
    start.Add(String("b"));
    s.SetData(start);
    t.Expect(s.GetPairStartIndex() == 1 && s.GetPairEndIndex() == -1, "one-value data restores pending start");
    t.Expect(s.GetReadoutText().Find("choose second") >= 0, "pending readout requests second point");

    s.SetPreset(UiMatrixPreset::Position9).SetSelectionMode(UiMatrixSelectionMode::Pair).SetPair(0, 8);
    t.Expect(s.GetPairOrientation() == UiMatrixPairOrientation::Diagonal, "generic matrix supports diagonal pair");
}

static void TestDefault(TestCtx& t)
{
    t.Section("Default indicator contract");
    UiMatrixSelector s;
    t.Expect(!s.HasDefault() && s.GetDefaultIndex() == -1, "no default initially");
    s.SetDefault(4);
    t.Expect(s.HasDefault() && s.GetDefaultIndex() == 4 && s.IsDefaultShown(), "SetDefault stores and shows default");
    t.Expect(s.IsDefaultSelected(), "selected center equals default");
    s.SelectIndex(0);
    t.Expect(!s.IsDefaultSelected(), "different selection exposes default reminder state");
    s.ShowDefault(false);
    t.Expect(!s.IsDefaultShown() && s.HasDefault(), "ShowDefault hides without clearing");
    s.ShowDefault();
    t.Expect(s.IsDefaultShown(), "ShowDefault restores marker visibility");
    s.SetDefault(s.GetSelectedIndex());
    t.Expect(s.GetDefaultIndex() == 0 && s.IsDefaultSelected(), "current selection can be baked as new default");
    s.ClearDefault();
    t.Expect(!s.HasDefault() && !s.IsDefaultSelected(), "ClearDefault removes marker");

    s.SetPreset(UiMatrixPreset::QuadPair).SetDefault(0).SetPair(0, 3);
    t.Expect(s.IsDefaultSelected(), "pair containing default suppresses reminder");
    s.SetPair(1, 3);
    t.Expect(!s.IsDefaultSelected(), "pair excluding default exposes reminder");
    s.EnableCell(0, false);
    t.Expect(!s.HasDefault(), "disabling default cell clears default");

    s.SetPreset(UiMatrixPreset::Region5);
    s.SetDefault(0);
    t.Expect(!s.HasDefault(), "cannot set hidden disabled cell as default");
}

static void TestKeyboard(TestCtx& t)
{
    t.Section("Keyboard interaction");
    UiMatrixSelector s;
    s.SetPreset(UiMatrixPreset::Region5).SelectIndex(4);
    t.Expect(s.Key(K_UP, 1) && s.GetSelectedIndex() == 1, "up moves center to top");
    s.SelectIndex(4); t.Expect(s.Key(K_DOWN, 1) && s.GetSelectedIndex() == 7, "down moves center to bottom");
    s.SelectIndex(4); t.Expect(s.Key(K_LEFT, 1) && s.GetSelectedIndex() == 3, "left moves center to left");
    s.SelectIndex(4); t.Expect(s.Key(K_RIGHT, 1) && s.GetSelectedIndex() == 5, "right moves center to right");

    s.SetPreset(UiMatrixPreset::Compass8).SelectIndex(5);
    s.Key(K_LEFT, 1);
    t.Expect(s.GetSelectedIndex() == 3, "Compass skips disabled center");

    s.SetPreset(UiMatrixPreset::QuadPair).SelectIndex(0);
    int changing = 0, actions = 0;
    s.WhenChanging = [&] { changing++; };
    s.WhenAction = [&] { actions++; };
    t.Expect(s.Key(K_SPACE, 1) && s.GetPairStartIndex() == 0 && s.GetPairEndIndex() == -1, "Space commits first pair endpoint");
    t.Expect(changing == 1 && actions == 0, "first endpoint is live change only");
    t.Expect(s.Key(K_RIGHT, 1) && s.GetSelectedIndex() == 1, "pair cursor moves independently");
    t.Expect(changing == 1 && actions == 0, "pair cursor emits no data event");
    t.Expect(s.Key(K_ENTER, 1) && s.GetPairEndIndex() == 1, "Enter commits second endpoint");
    t.Expect(changing == 2 && actions == 1, "complete pair commits once");
    t.Expect(s.GetPairOrientation() == UiMatrixPairOrientation::Horizontal, "keyboard pair geometry correct");
}

static void TestStyle(TestCtx& t)
{
    t.Section("Style contract");
    UiMatrixSelector s;
    t.Expect(!s.HasCustomStyle(), "starts theme-driven");
    s.SetCellGap(DPI(5)).SetCellRadius(DPI(8)).SetOuterRadius(DPI(10))
     .SetGlyphInset(DPI(6)).SetIconInset(DPI(5)).SetPairLineWidth(DPI(4))
     .SetPairArrowSize(DPI(9)).SetDefaultDash(DPI(5), DPI(2)).SetDefaultFrameWidth(DPI(2))
     .SetSelectedFrameExtra(DPI(3))
     .SetCellFont(SansSerifZ(12)).SetReadoutFont(SansSerifZ(13))
     .SetReadoutRadius(DPI(7)).SetReadoutGap(DPI(12)).SetReadoutWidth(DPI(120))
     .ShowCellFace(false).ShowCellFrame(false)
     .ShowReadoutFace(true).ShowReadoutFrame(false)
     .ShowSurface(true).ShowSurfaceFrame(true).SetSurfaceShadow(true);
    t.Expect(s.HasCustomStyle(), "local adjustments create custom style");
    const UiMatrixSelector::Style& st = s.GetStyle();
    t.Expect(st.cell_gap == DPI(5) && st.cell_metrics.radius == DPI(8), "cell geometry styleable");
    t.Expect(st.surface_metrics.radius == DPI(10) && st.surface_metrics.shadow.enabled, "outer styleable");
    t.Expect(!st.cell_metrics.face_enabled && !st.cell_metrics.frame_enabled, "cell face/frame independent");
    t.Expect(st.readout_metrics.face_enabled && !st.readout_metrics.frame_enabled, "readout face/frame independent");
    t.Expect(st.glyph_inset == DPI(6) && st.icon_inset == DPI(5), "glyph/icon inset styleable");
    t.Expect(st.pair_line_width == DPI(4) && st.pair_arrow_size == DPI(9), "pair line and arrow styleable");
    t.Expect(st.default_dash == DPI(5) && st.default_dash_gap == DPI(2) && st.default_frame_width == DPI(2), "default dash styleable");
    t.Expect(st.selected_frame_extra == DPI(3), "selected frame emphasis styleable");
    t.Expect(st.readout_gap == DPI(12) && st.readout_width == DPI(120), "readout geometry styleable");
    s.ClearCustomStyle();
    t.Expect(!s.HasCustomStyle(), "ClearCustomStyle restores theme style");
}

int RunMatrixSelectorSuite()
{
    TestCtx t;
    TestPresets(t);
    TestSelectionAndData(t);
    TestGeometry(t);
    TestPair(t);
    TestDefault(t);
    TestKeyboard(t);
    TestStyle(t);
    Cout() << "\nChecks: " << t.checks << ", Fails: " << t.fails << "\n";
    return t.fails ? 1 : 0;
}
