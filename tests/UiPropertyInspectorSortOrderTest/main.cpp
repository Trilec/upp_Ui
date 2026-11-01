#include <Ui/UiPropertyInspector/UiPropertyInspector.h>

using namespace Upp;

namespace {
int checks = 0;
int failed = 0;

void Check(bool ok, const char *message)
{
    checks++;
    if(ok)
        Cout() << "PASS: " << message << '\n';
    else {
        failed++;
        Cout() << "FAIL: " << message << '\n';
    }
}
}

CONSOLE_APP_MAIN
{
    UiPropertyInspectorModel model;
    UiPropertyInspectorItem& third = model.AddText("third", "Third", "3", "Group");
    UiPropertyInspectorItem& first = model.AddText("first", "First", "1", "Group");
    UiPropertyInspectorItem& second = model.AddText("second", "Second", "2", "Group");
    third.sort_order = 30;
    first.sort_order = 10;
    second.sort_order = 20;
    model.StructureChanged();

    UiPropertyInspector editor;
    editor.SetRect(0, 0, 320, 240);
    editor.SetModel(&model);

    Check(editor.GetSelectedPropertyId() == "first",
          "lowest sort_order is the first displayed property");
    editor.Key(K_DOWN, 1);
    Check(editor.GetSelectedPropertyId() == "second",
          "keyboard navigation follows sort_order");
    editor.Key(K_DOWN, 1);
    Check(editor.GetSelectedPropertyId() == "third",
          "all display rows follow sort_order");

    UiPropertyInspectorModel stable;
    UiPropertyInspectorItem& alpha = stable.AddText("alpha", "Alpha", "a", "Group");
    UiPropertyInspectorItem& beta = stable.AddText("beta", "Beta", "b", "Group");
    alpha.sort_order = 10;
    beta.sort_order = 10;
    stable.StructureChanged();

    UiPropertyInspector stable_editor;
    stable_editor.SetRect(0, 0, 320, 240);
    stable_editor.SetModel(&stable);
    Check(stable_editor.GetSelectedPropertyId() == "alpha",
          "equal sort_order preserves model insertion order");

    UiPropertyInspectorModel defaults;
    defaults.AddText("one", "One", "1", "Group");
    defaults.AddText("two", "Two", "2", "Group");
    defaults.StructureChanged();

    UiPropertyInspector default_editor;
    default_editor.SetRect(0, 0, 320, 240);
    default_editor.SetModel(&defaults);
    Check(default_editor.GetSelectedPropertyId() == "one",
          "default Add() sort_order preserves existing insertion order");

    Cout() << "PROPERTY_EDITOR_SORT_ORDER_SUMMARY checks=" << checks
           << " failed=" << failed << '\n';
    SetExitCode(failed ? 1 : 0);
}
