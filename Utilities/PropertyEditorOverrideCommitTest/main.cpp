#include <Utilities/PropertyEditor/PropertyEditor.h>

using namespace Upp;

static int checks = 0;
static int fails = 0;

static void Check(bool condition, const char *message)
{
    checks++;
    if(!condition) {
        fails++;
        Cout() << "FAIL: " << message << "\n";
    }
}

static UiDropdown* FindDropdown(Ctrl& root)
{
    for(Ctrl *q = root.GetFirstChild(); q; q = q->GetNext()) {
        if(UiDropdown *drop = dynamic_cast<UiDropdown *>(q))
            return drop;
        if(UiDropdown *drop = FindDropdown(*q))
            return drop;
    }
    return nullptr;
}

CONSOLE_APP_MAIN
{
    PropertyEditorModel model;
    PropertyEditorItem& mode = model.AddChoice("mode", "Mode", "Theme", "Appearance");
    mode.AddChoice("Theme", "Use theme")
        .AddChoice("Solid", "Solid");
    mode.overrideable = true;
    mode.override_active = false;
    mode.inherited = true;
    mode.enabled = true;
    mode.value_editable = true;
    model.StructureChanged();

    PropertyEditor editor;
    editor.SetRect(0, 0, 360, 180);
    editor.SetModel(&model);

    int override_requests = 0;
    int commits = 0;
    editor.WhenOverride = [&](String id, bool active) {
        override_requests++;
        Check(id == "mode" && active,
              "commit requests activation for the edited override");
        PropertyEditorItem *item = model.Find(id);
        if(item) {
            item->override_active = active;
            item->inherited = !active;
        }
    };
    editor.WhenCommit = [&](String id, Value value) {
        commits++;
        Check(id == "mode" && AsString(value) == "Solid",
              "commit reports the normalized authored value");
    };

    Check(editor.SelectProperty("mode", true),
          "overrideable Choice opens its value editor programmatically");
    editor.Layout();
    UiDropdown *drop = FindDropdown(editor);
    Check(drop != nullptr,
          "active Choice uses the real UiDropdown editor");
    if(drop)
        drop->SelectByData("Solid");

    Check(override_requests == 1 && model.Find("mode")->override_active,
          "successful dropdown commit auto-activates override exactly once");
    Check(commits == 1 && AsString(model.Find("mode")->value) == "Solid",
          "authored dropdown value commits after override activation");

    Cout() << "PROPERTYEDITOR_OVERRIDE_COMMIT_SUMMARY checks=" << checks
           << " failed=" << fails << "\n";
    SetExitCode(fails ? 1 : 0);
}
