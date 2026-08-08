#include <Core/Core.h>
#include <Ui/Ui.h>

using namespace Upp;

struct TestCtx {
    int checks = 0;
    int fails = 0;

    void Expect(bool cond, const String& msg)
    {
        checks++;
        if(!cond) {
            fails++;
            Cout() << "[FAIL] " << msg << "\n";
        }
    }

    void Section(const char* title)
    {
        Cout() << "\n=== " << title << " ===\n";
    }
};

static void TestPresets(TestCtx& t)
{
    t.Section("Preset contract");
    UiMatrixSelector s;

    t.Expect(s.GetPreset() == UiMatrixPreset::Position9, "default preset is Position9");
    t.Expect(s.GetRows() == 3 && s.GetColumns() == 3 && s.GetCellCount() == 9,
             "Position9 is a 3x3 matrix");
    t.Expect(s.GetSelectedIndex() == 4 && s.GetData() == Value("center"),
             "Position9 defaults to center");
    t.Expect(s.GetCell(0).short_label == "TL" && s.GetCell(8).short_label == "BR",
             "Position9 carries compact corner labels");

    s.SetPreset(UiMatrixPreset::Compass8);
    t.Expect(s.GetCellCount() == 9 && !s.GetCell(4).enabled && s.GetCell(4).visible,
             "Compass8 keeps a visible non-selectable center");
    t.Expect(s.GetCell(0).glyph == UiMatrixGlyph::ArrowNW && s.GetCell(8).glyph == UiMatrixGlyph::ArrowSE,
             "Compass8 uses procedural direction glyphs");
    t.Expect(s.GetData() == Value("east"), "Compass8 has a valid default selection");

    s.SetPreset(UiMatrixPreset::Region5);
    t.Expect(!s.GetCell(0).visible && !s.GetCell(2).visible && !s.GetCell(6).visible && !s.GetCell(8).visible,
             "Region5 masks all four corners");
    t.Expect(s.GetCell(1).short_label == "TOP" && s.GetCell(7).short_label == "BTM",
             "Region5 exposes top and bottom compact labels");

    s.SetPreset(UiMatrixPreset::DramaticaQuad);
    t.Expect(s.GetRows() == 2 && s.GetColumns() == 2 && s.GetCellCount() == 4,
             "DramaticaQuad is a 2x2 matrix");
    t.Expect(s.GetOverlay() == UiMatrixOverlay::DynamicPairs,
             "DramaticaQuad retains the original dynamic-pair presentation overlay");
}

static void TestSelectionAndData(TestCtx& t)
{
    t.Section("Selection and data");
    UiMatrixSelector s;
    s.SetPreset(UiMatrixPreset::Position9);

    int changing = 0;
    int actions = 0;
    s.WhenChanging = [&] { changing++; };
    s.WhenAction = [&] { actions++; };

    s.SelectIndex(0);
    t.Expect(s.GetSelectedIndex() == 0 && s.GetData() == Value("top_left"),
             "SelectIndex changes the selected semantic value");
    t.Expect(changing == 0 && actions == 0, "programmatic selection is silent by default");

    s.SelectIndex(8, true);
    t.Expect(s.GetData() == Value("bottom_right") && changing == 1 && actions == 1,
             "fire_action emits changing and committed action once");

    s.SetData(Value("left"));
    t.Expect(s.GetSelectedIndex() == 3 && s.GetSelectedLabel() == "Left",
             "SetData resolves preset semantic values");

    s.SetCell(3, "WEST", "West side", 42);
    s.SetData(42);
    t.Expect(s.GetSelectedIndex() == 3 && s.GetSelectedLabel() == "West side" && s.GetData() == Value(42),
             "cell display text, readout label, and semantic value are independent");
}

static void TestGeometryAndHitTest(TestCtx& t)
{
    t.Section("Geometry and hit testing");
    UiMatrixSelector s;
    s.SetRect(0, 0, 420, 250);
    s.SetPreset(UiMatrixPreset::Position9);

    Rect matrix = s.GetMatrixRect();
    Rect readout = s.GetReadoutRect();
    t.Expect(!matrix.IsEmpty() && matrix.GetWidth() == matrix.GetHeight(),
             "matrix remains square inside a wider control");
    t.Expect(!readout.IsEmpty() && readout.left > matrix.right,
             "readout is centered in a separate region to the right");

    for(int i = 0; i < 9; i++)
        t.Expect(s.HitTest(s.GetCellRect(i).CenterPoint()) == i,
                 Format("Position9 cell %d hit-tests to itself", i));

    s.SetPreset(UiMatrixPreset::Region5);
    t.Expect(s.HitTest(s.GetCellRect(0).CenterPoint()) == -1,
             "masked Region5 corner is not selectable");
    t.Expect(s.HitTest(s.GetCellRect(4).CenterPoint()) == 4,
             "Region5 center remains selectable");

    s.SetCellGap(DPI(6));
    Rect a = s.GetCellRect(1);
    Rect b = s.GetCellRect(4);
    t.Expect(b.top - a.bottom >= DPI(6), "cell gap creates visibly separated button geometry");
}

static void TestKeyboard(TestCtx& t)
{
    t.Section("Keyboard navigation");
    UiMatrixSelector s;
    s.SetPreset(UiMatrixPreset::Region5);
    s.SelectIndex(4);

    t.Expect(s.Key(K_UP, 1) && s.GetSelectedIndex() == 1, "up moves center to top");
    s.SelectIndex(4);
    t.Expect(s.Key(K_DOWN, 1) && s.GetSelectedIndex() == 7, "down moves center to bottom");
    s.SelectIndex(4);
    t.Expect(s.Key(K_LEFT, 1) && s.GetSelectedIndex() == 3, "left moves center to left");
    s.SelectIndex(4);
    t.Expect(s.Key(K_RIGHT, 1) && s.GetSelectedIndex() == 5, "right moves center to right");

    s.SetPreset(UiMatrixPreset::Compass8);
    s.SelectIndex(5);
    s.Key(K_LEFT, 1);
    t.Expect(s.GetSelectedIndex() == 3, "Compass8 skips disabled center during horizontal navigation");
}

static void TestDramaticaOverlays(TestCtx& t)
{
    t.Section("Dramatica overlays");
    UiMatrixSelector s;
    s.SetPreset(UiMatrixPreset::DramaticaQuad);

    s.SetOverlay(UiMatrixOverlay::DynamicPairs);
    t.Expect(s.GetOverlay() == UiMatrixOverlay::DynamicPairs, "dynamic-pair overlay is selectable");
    s.SetOverlay(UiMatrixOverlay::CompanionPairs);
    t.Expect(s.GetOverlay() == UiMatrixOverlay::CompanionPairs, "companion-pair overlay is selectable");
    s.SetOverlay(UiMatrixOverlay::DependentPairs);
    t.Expect(s.GetOverlay() == UiMatrixOverlay::DependentPairs, "dependent-pair overlay is selectable");
    s.SetOverlay(UiMatrixOverlay::PathU);
    t.Expect(s.GetOverlay() == UiMatrixOverlay::PathU, "U path overlay is selectable");
    s.SetOverlay(UiMatrixOverlay::PathZ);
    t.Expect(s.GetOverlay() == UiMatrixOverlay::PathZ, "Z path overlay is selectable");
    s.SetOverlay(UiMatrixOverlay::PathButterfly);
    t.Expect(s.GetOverlay() == UiMatrixOverlay::PathButterfly, "Butterfly path overlay is selectable");

    Vector<int> path;
    path << 0 << 2 << 1 << 3;
    s.SetCustomPath(path);
    bool same_path = s.GetCustomPath().GetCount() == path.GetCount();
    for(int i = 0; same_path && i < path.GetCount(); i++)
        same_path = s.GetCustomPath()[i] == path[i];
    t.Expect(s.GetOverlay() == UiMatrixOverlay::CustomPath && same_path,
             "custom ordered quad path is retained");
}

static void TestPairSelection(TestCtx& t)
{
    t.Section("Ordered pair selection");
    UiMatrixSelector s;
    s.SetPreset(UiMatrixPreset::DramaticaQuad)
     .SetOverlay(UiMatrixOverlay::None)
     .SetSelectionMode(UiMatrixSelectionMode::Pair);

    t.Expect(s.IsPairSelection() && !s.HasPairStart() && !s.HasCompletePair(),
             "pair mode starts with no relationship selected");
    t.Expect(s.GetOverlay() == UiMatrixOverlay::None,
             "pair mode clears the preset reference overlay so the chosen pair is authoritative");
    t.Expect(s.GetReadoutText() == "Choose first point", "pair readout prompts for the first endpoint");

    s.SetPair(0, 3);
    t.Expect(s.HasCompletePair() && s.GetPairStartIndex() == 0 && s.GetPairEndIndex() == 3,
             "SetPair stores an ordered pair");
    t.Expect(s.GetPairOrientation() == UiMatrixPairOrientation::Diagonal,
             "opposite corners classify as diagonal");
    t.Expect(s.GetDramaticaRelationship() == UiMatrixRelationship::Dynamic,
             "Dramatica diagonal classifies as Dynamic");
    t.Expect(s.GetRelationshipName() == "Dynamic" && s.GetPairOrientationName() == "Diagonal",
             "relationship and geometric names are exposed");
    t.Expect(s.GetPairDirectionLabel() == "Upper left -> Lower right",
             "pair direction preserves first-to-second order");
    t.Expect(s.GetReadoutText().Find("Dynamic") >= 0 && s.GetReadoutText().Find("Upper left -> Lower right") >= 0,
             "readout combines theory, geometry, and direction");

    Value pair_value = s.GetData();
    t.Expect(pair_value.Is<ValueArray>(), "pair mode GetData returns an ordered ValueArray");
    ValueArray pair = pair_value;
    t.Expect(pair.GetCount() == 2 && pair[0] == Value("a") && pair[1] == Value("d"),
             "pair data preserves semantic endpoint order");

    s.SetPair(3, 0);
    t.Expect(s.GetPairDirectionLabel() == "Lower right -> Upper left",
             "reverse selection preserves reverse direction");
    ValueArray reverse = s.GetData();
    t.Expect(reverse[0] == Value("d") && reverse[1] == Value("a"),
             "reverse pair data is not normalized away");

    s.SetPair(0, 1);
    t.Expect(s.GetPairOrientation() == UiMatrixPairOrientation::Horizontal
             && s.GetDramaticaRelationship() == UiMatrixRelationship::Companion,
             "same-row pair classifies as horizontal Companion");

    s.SetPair(1, 3);
    t.Expect(s.GetPairOrientation() == UiMatrixPairOrientation::Vertical
             && s.GetDramaticaRelationship() == UiMatrixRelationship::Dependent,
             "same-column pair classifies as vertical Dependent");

    ValueArray input;
    input.Add(String("c"));
    input.Add(String("d"));
    s.SetData(input);
    t.Expect(s.GetPairStartIndex() == 2 && s.GetPairEndIndex() == 3,
             "SetData restores an ordered semantic pair");
    t.Expect(s.GetDramaticaRelationship() == UiMatrixRelationship::Companion,
             "restored lower-row pair is Companion");

    s.SetCell(2, "IC", "Impact concept", "impact");
    s.SetPair(2, 0);
    t.Expect(s.GetPairDirectionLabel() == "Impact concept -> Upper left",
             "injected cell labels flow through pair direction text");
    ValueArray injected = s.GetData();
    t.Expect(injected[0] == Value("impact") && injected[1] == Value("a"),
             "injected semantic values flow through pair data");

    s.ClearPair();
    t.Expect(!s.HasPairStart() && !s.HasCompletePair(), "ClearPair clears both endpoints");

    ValueArray start_only;
    start_only.Add(String("b"));
    s.SetData(start_only);
    t.Expect(s.GetPairStartIndex() == 1 && s.GetPairEndIndex() == -1,
             "one-value pair data restores a pending first endpoint");
    t.Expect(s.GetReadoutText().Find("choose second") >= 0,
             "pending pair readout asks for the second endpoint");

    s.SetPreset(UiMatrixPreset::Position9);
    s.SetSelectionMode(UiMatrixSelectionMode::Pair);
    s.SetPair(0, 8);
    t.Expect(s.GetPairOrientation() == UiMatrixPairOrientation::Diagonal
             && s.GetDramaticaRelationship() == UiMatrixRelationship::None,
             "generic matrices classify geometry without inventing Dramatica semantics");
}

static void TestPairKeyboard(TestCtx& t)
{
    t.Section("Pair keyboard interaction");
    UiMatrixSelector s;
    s.SetPreset(UiMatrixPreset::DramaticaQuad)
     .SetOverlay(UiMatrixOverlay::None)
     .SetSelectionMode(UiMatrixSelectionMode::Pair)
     .SelectIndex(0);

    int changing = 0;
    int actions = 0;
    s.WhenChanging = [&] { changing++; };
    s.WhenAction = [&] { actions++; };

    t.Expect(s.Key(K_SPACE, 1) && s.GetPairStartIndex() == 0 && s.GetPairEndIndex() == -1,
             "Space commits the first keyboard endpoint");
    t.Expect(changing == 1 && actions == 0, "first endpoint changes data without committing a complete action");

    t.Expect(s.Key(K_RIGHT, 1) && s.GetSelectedIndex() == 1,
             "arrow keys move the pair cursor without changing the chosen first endpoint");
    t.Expect(changing == 1 && actions == 0,
             "pair cursor navigation emits no data event");

    t.Expect(s.Key(K_ENTER, 1) && s.GetPairEndIndex() == 1,
             "Enter commits the second keyboard endpoint");
    t.Expect(changing == 2 && actions == 1,
             "complete keyboard pair emits changing then one committed action");
    t.Expect(s.GetDramaticaRelationship() == UiMatrixRelationship::Companion,
             "keyboard-created upper-row pair is Companion");

    s.Key(K_DOWN, 1);
    s.Key(K_SPACE, 1);
    t.Expect(s.GetPairStartIndex() == 3 && s.GetPairEndIndex() == -1,
             "after a complete pair, the next activation begins a new ordered pair");
}

static void TestStyle(TestCtx& t)
{
    t.Section("Style contract");
    UiMatrixSelector s;
    t.Expect(!s.HasCustomStyle(), "selector starts theme-driven");

    s.SetCellGap(DPI(5)).SetCellRadius(DPI(8)).SetOuterRadius(DPI(10))
     .SetGlyphInset(DPI(6)).SetIconInset(DPI(5)).SetOverlayWidth(DPI(4))
     .SetPairArrowSize(DPI(9))
     .SetCellFont(SansSerifZ(12)).SetReadoutFont(SansSerifZ(13))
     .SetReadoutRadius(DPI(7)).SetReadoutGap(DPI(12)).SetReadoutWidth(DPI(120))
     .ShowCellFace(false).ShowCellFrame(false)
     .ShowReadoutFace(true).ShowReadoutFrame(false)
     .ShowSurface(true).ShowSurfaceFrame(true).SetSurfaceShadow(true);

    t.Expect(s.HasCustomStyle(), "local visual adjustments promote a custom style");
    const UiMatrixSelector::Style& st = s.GetStyle();
    t.Expect(st.cell_gap == DPI(5) && st.cell_metrics.radius == DPI(8),
             "cell gap and cell radius are styleable");
    t.Expect(st.surface_metrics.radius == DPI(10) && st.surface_metrics.shadow.enabled,
             "outer radius and shadow are styleable");
    t.Expect(!st.cell_metrics.face_enabled && !st.cell_metrics.frame_enabled,
             "cell face and frame can be independently disabled");
    t.Expect(st.readout_metrics.face_enabled && !st.readout_metrics.frame_enabled,
             "readout face and frame can be styled independently");
    t.Expect(st.glyph_inset == DPI(6) && st.icon_inset == DPI(5)
             && st.overlay_width == DPI(4) && st.pair_arrow_size == DPI(9),
             "glyph, icon, line, and pair-arrow geometry are styleable");
    t.Expect(st.readout_gap == DPI(12) && st.readout_width == DPI(120),
             "readout geometry is styleable");

    s.ClearCustomStyle();
    t.Expect(!s.HasCustomStyle(), "ClearCustomStyle restores theme-driven style");
}

CONSOLE_APP_MAIN
{
    TestCtx t;
    TestPresets(t);
    TestSelectionAndData(t);
    TestGeometryAndHitTest(t);
    TestKeyboard(t);
    TestDramaticaOverlays(t);
    TestPairSelection(t);
    TestPairKeyboard(t);
    TestStyle(t);

    Cout() << "\nChecks: " << t.checks << ", Fails: " << t.fails << "\n";
    SetExitCode(t.fails ? 1 : 0);
}
