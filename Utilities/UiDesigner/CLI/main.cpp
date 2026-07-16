#include <Utilities/UiDesigner/Services/UiDesignerServices.h>

using namespace Upp;

static void Usage()
{
    Cout() <<
        "UiDesigner CLI\n"
        "  list-controls [query] [category]\n"
        "  schema <control-type>\n"
        "  validate <project-or-design.json>\n"
        "  set <project> <node-id> <property> <json-value>\n"
        "  theme-set <project> <property> <json-value>\n"
        "  plan-add <project> <type> <target-id> [x y] [index]\n"
        "  add <project> <type> <target-id> [x y] [index]\n"
        "  plan-move <project> <target-id> <node-id[,node-id...]> [index]\n"
        "  move <project> <target-id> <node-id[,node-id...]> [index]\n"
        "  behavior-get <project> <node-id>\n"
        "  behavior-set <project> <node-id> <event> <action>"
        " [target-id] [property] [json-value] [delta] [handler]\n"
        "  behavior-remove <project> <node-id> <event>\n"
        "  export <project> <complete|component|project_json|document_json|theme_json>"
        " <destination> [package] [class] [namespace]\n"
        "  generate <project> <output-folder> [class-name]\n"
        "  migrate <legacy-design.json> <project.uidesign.json>\n";
}

static Value ParseArgumentValue(const String& text)
{
    Value value = ParseJSON(text);
    return IsError(value) ? Value(text) : value;
}

static bool LoadProject(UiDesignerSession& session, const String& path)
{
    String error;
    if(session.Load(path, error))
        return true;
    Cerr() << error << '\n';
    return false;
}

static bool ResultOk(const Value& value)
{
    return value.Is<ValueMap>() &&
           (bool)UiDesignerMapValue(ValueMap(value), "ok", false);
}

static int PrintResult(const Value& value)
{
    Cout() << AsJSON(value, true) << '\n';
    return ResultOk(value) ? 0 : 1;
}

static bool SaveProject(UiDesignerSession& session, const String& path)
{
    String error;
    if(session.Save(path, error))
        return true;
    Cerr() << error << '\n';
    return false;
}

static ValueArray ParseNodeList(const String& text)
{
    ValueArray result;
    for(const String& token : Split(text, ',')) {
        const String trimmed = TrimBoth(token);
        if(!trimmed.IsEmpty())
            result.Add(ScanInt64(trimmed));
    }
    return result;
}

static ValueMap DropParams(const Vector<String>& args, bool move)
{
    ValueMap request;
    request.Set("operation", move ? "move" : "add");
    request.Set("target", ScanInt64(args[2]));
    if(move)
        request.Set("nodes", ParseNodeList(args[3]));
    else
        request.Set("type", args[1]);

    const int base = move ? 4 : 3;
    if(args.GetCount() >= base + 2) {
        request.Set("x", ScanInt(args[base]));
        request.Set("y", ScanInt(args[base + 1]));
    }
    if(args.GetCount() >= base + 3)
        request.Set("index", ScanInt(args[base + 2]));
    return request;
}

CONSOLE_APP_MAIN
{
    const Vector<String>& args = CommandLine();
    if(args.IsEmpty()) {
        Usage();
        SetExitCode(2);
        return;
    }

    UiDesignerSession session;
    UiDesignerAutomationService automation(session);
    const String command = args[0];

    if(command == "list-controls") {
        ValueMap params;
        if(args.GetCount() >= 2) params.Set("query", args[1]);
        if(args.GetCount() >= 3) params.Set("category", args[2]);
        SetExitCode(PrintResult(automation.ListControls(params)));
        return;
    }

    if(command == "schema") {
        if(args.GetCount() != 2) { Usage(); SetExitCode(2); return; }
        ValueMap params;
        params.Set("type", args[1]);
        SetExitCode(PrintResult(automation.GetControlSpec(params)));
        return;
    }

    if(command == "validate") {
        if(args.GetCount() != 2 || !LoadProject(session, args[1])) {
            SetExitCode(1);
            return;
        }
        SetExitCode(PrintResult(automation.ValidateDocument()));
        return;
    }

    if(command == "set") {
        if(args.GetCount() != 5 || !LoadProject(session, args[1])) {
            Usage(); SetExitCode(2); return;
        }
        session.Select(ScanInt64(args[2]), false);
        ValueMap request;
        request.Set("property", args[3]);
        request.Set("value", ParseArgumentValue(args[4]));
        request.Set("expected_revision", (int64)session.Document().GetRevision());
        Value result = automation.CommitProperty(request);
        if(ResultOk(result) && !SaveProject(session, args[1])) {
            SetExitCode(1); return;
        }
        SetExitCode(PrintResult(result));
        return;
    }

    if(command == "theme-set") {
        if(args.GetCount() != 4 || !LoadProject(session, args[1])) {
            Usage(); SetExitCode(2); return;
        }
        ValueMap request;
        request.Set("property", args[2]);
        request.Set("value", ParseArgumentValue(args[3]));
        Value result = automation.CommitThemeProperty(request);
        if(ResultOk(result) && !SaveProject(session, args[1])) {
            SetExitCode(1); return;
        }
        SetExitCode(PrintResult(result));
        return;
    }

    if(command == "plan-add" || command == "add") {
        if(args.GetCount() < 4 || args.GetCount() > 7 ||
           !LoadProject(session, args[1])) {
            Usage(); SetExitCode(2); return;
        }
        Vector<String> drop_args;
        drop_args.Add(args[2]);
        drop_args.Add(args[2]);
        drop_args.Add(args[3]);
        for(int i = 4; i < args.GetCount(); i++) drop_args.Add(args[i]);
        ValueMap request = DropParams(drop_args, false);
        Value result;
        if(command == "plan-add")
            result = automation.PlanAdd(request);
        else {
            request.Set("expected_revision", (int64)session.Document().GetRevision());
            result = automation.ApplyDrop(request);
            if(ResultOk(result) && !SaveProject(session, args[1])) {
                SetExitCode(1); return;
            }
        }
        SetExitCode(PrintResult(result));
        return;
    }

    if(command == "plan-move" || command == "move") {
        if(args.GetCount() < 4 || args.GetCount() > 5 ||
           !LoadProject(session, args[1])) {
            Usage(); SetExitCode(2); return;
        }
        ValueMap request;
        request.Set("operation", "move");
        request.Set("target", ScanInt64(args[2]));
        request.Set("nodes", ParseNodeList(args[3]));
        if(args.GetCount() == 5) request.Set("index", ScanInt(args[4]));
        Value result;
        if(command == "plan-move")
            result = automation.PlanMove(request);
        else {
            request.Set("expected_revision", (int64)session.Document().GetRevision());
            result = automation.ApplyDrop(request);
            if(ResultOk(result) && !SaveProject(session, args[1])) {
                SetExitCode(1); return;
            }
        }
        SetExitCode(PrintResult(result));
        return;
    }

    if(command == "behavior-get") {
        if(args.GetCount() != 3 || !LoadProject(session, args[1])) {
            Usage(); SetExitCode(2); return;
        }
        ValueMap request;
        request.Set("node", ScanInt64(args[2]));
        SetExitCode(PrintResult(automation.GetBehaviors(request)));
        return;
    }

    if(command == "behavior-set") {
        if(args.GetCount() < 6 || args.GetCount() > 11 ||
           !LoadProject(session, args[1])) {
            Usage(); SetExitCode(2); return;
        }
        ValueMap request;
        request.Set("node", ScanInt64(args[2]));
        request.Set("event", args[3]);
        request.Set("action", args[4]);
        request.Set("expected_revision", (int64)session.Document().GetRevision());
        if(args.GetCount() >= 6) request.Set("target", ScanInt64(args[5]));
        if(args.GetCount() >= 7) request.Set("property", args[6]);
        if(args.GetCount() >= 8) request.Set("value", ParseArgumentValue(args[7]));
        if(args.GetCount() >= 9) request.Set("delta", ScanDouble(args[8]));
        if(args.GetCount() >= 10) request.Set("handler", args[9]);
        Value result = automation.SetBehavior(request);
        if(ResultOk(result) && !SaveProject(session, args[1])) {
            SetExitCode(1); return;
        }
        SetExitCode(PrintResult(result));
        return;
    }

    if(command == "behavior-remove") {
        if(args.GetCount() != 4 || !LoadProject(session, args[1])) {
            Usage(); SetExitCode(2); return;
        }
        ValueMap request;
        request.Set("node", ScanInt64(args[2]));
        request.Set("event", args[3]);
        request.Set("expected_revision", (int64)session.Document().GetRevision());
        Value result = automation.RemoveBehavior(request);
        if(ResultOk(result) && !SaveProject(session, args[1])) {
            SetExitCode(1); return;
        }
        SetExitCode(PrintResult(result));
        return;
    }

    if(command == "export") {
        if(args.GetCount() < 4 || args.GetCount() > 7 ||
           !LoadProject(session, args[1])) {
            Usage(); SetExitCode(2); return;
        }
        ValueMap request;
        request.Set("profile", args[2]);
        request.Set("destination", args[3]);
        if(args.GetCount() >= 5) request.Set("package_name", args[4]);
        if(args.GetCount() >= 6) request.Set("class_name", args[5]);
        if(args.GetCount() >= 7) request.Set("namespace", args[6]);
        SetExitCode(PrintResult(automation.Export(request)));
        return;
    }

    if(command == "generate") {
        if(args.GetCount() < 3 || args.GetCount() > 4 ||
           !LoadProject(session, args[1])) {
            Usage(); SetExitCode(2); return;
        }
        ValueMap request;
        request.Set("profile", "complete");
        request.Set("destination", args[2]);
        const String class_name = args.GetCount() == 4
            ? args[3] : "GeneratedUiWindow";
        request.Set("package_name", class_name);
        request.Set("class_name", class_name);
        SetExitCode(PrintResult(automation.Export(request)));
        return;
    }

    if(command == "migrate") {
        if(args.GetCount() != 3 || !LoadProject(session, args[1])) {
            Usage(); SetExitCode(2); return;
        }
        if(!SaveProject(session, args[2])) {
            SetExitCode(1); return;
        }
        Cout() << "Migrated to " << args[2] << '\n';
        return;
    }

    Usage();
    SetExitCode(2);
}
