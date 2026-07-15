#include <Utilities/UiDesigner/Services/UiDesignerServices.h>

using namespace Upp;

static void Usage()
{
    Cout() <<
        "UiDesigner CLI\n"
        "  list-controls\n"
        "  validate <project-or-design.json>\n"
        "  schema <control-type>\n"
        "  set <project> <node-id> <property> <json-value>\n"
        "  theme-set <project> <property> <json-value>\n"
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

static int PrintResult(const Value& value)
{
    Cout() << AsJSON(value, true) << '\n';
    if(value.Is<ValueMap>() && !(bool)UiDesignerMapValue(ValueMap(value), "ok", false))
        return 1;
    return 0;
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
        SetExitCode(PrintResult(automation.ListControls()));
        return;
    }

    if(command == "schema") {
        if(args.GetCount() != 2) { Usage(); SetExitCode(2); return; }
        const UiDesignerControlSpec* spec = session.Catalog().Find(args[1]);
        if(!spec) {
            Cerr() << "Unknown control type: " << args[1] << '\n';
            SetExitCode(1);
            return;
        }
        ValueArray properties;
        for(const UiDesignerPropertySpec& property : spec->properties) {
            ValueMap item;
            item.Set("id", property.id);
            item.Set("label", property.label);
            item.Set("group", property.group);
            item.Set("kind", PropertyEditorKindName(property.kind));
            item.Set("domain", PropertyEditorDomainName(property.domain));
            item.Set("impact", PropertyEditorImpactName(property.impact));
            item.Set("default", property.default_value);
            item.Set("minimum", property.minimum);
            item.Set("maximum", property.maximum);
            item.Set("step", property.step);
            item.Set("help", property.help);
            properties.Add(item);
        }
        ValueMap result;
        result.Set("type", spec->type_id);
        result.Set("cpp_type", spec->runtime_cpp_type);
        result.Set("category", spec->category);
        result.Set("properties", properties);
        Cout() << AsJSON(result, true) << '\n';
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
        if((bool)UiDesignerMapValue(ValueMap(result), "ok", false)) {
            String error;
            if(!session.Save(args[1], error)) {
                Cerr() << error << '\n'; SetExitCode(1); return;
            }
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
        if((bool)UiDesignerMapValue(ValueMap(result), "ok", false)) {
            String error;
            if(!session.Save(args[1], error)) {
                Cerr() << error << '\n'; SetExitCode(1); return;
            }
        }
        SetExitCode(PrintResult(result));
        return;
    }

    if(command == "generate") {
        if(args.GetCount() < 3 || args.GetCount() > 4 ||
           !LoadProject(session, args[1])) {
            Usage(); SetExitCode(2); return;
        }
        String error;
        const String class_name = args.GetCount() == 4
                                      ? args[3] : "GeneratedUiWindow";
        if(!session.Export(args[2], class_name, error)) {
            Cerr() << error << '\n'; SetExitCode(1); return;
        }
        Cout() << "Generated " << class_name << " in " << args[2] << '\n';
        return;
    }

    if(command == "migrate") {
        if(args.GetCount() != 3 || !LoadProject(session, args[1])) {
            Usage(); SetExitCode(2); return;
        }
        String error;
        if(!session.Save(args[2], error)) {
            Cerr() << error << '\n'; SetExitCode(1); return;
        }
        Cout() << "Migrated to " << args[2] << '\n';
        return;
    }

    Usage();
    SetExitCode(2);
}
