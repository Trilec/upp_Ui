#include <Core/Core.h>
#include <Utilities/UiDesigner/Services/UiDesignerServices.h>

using namespace Upp;

struct FoundationTester {
    int checks = 0;
    int failures = 0;

    void Check(bool condition, const String& label)
    {
        checks++;
        if(condition)
            Cout() << "PASS  " << label << '\n';
        else {
            failures++;
            Cout() << "FAIL  " << label << '\n';
        }
    }
};

static UiDesignerNodeId AddThroughDrop(UiDesignerSession& session,
                                       const String& type,
                                       UiDesignerNodeId parent,
                                       Point position = Point(0, 0),
                                       bool positioned = false)
{
    UiDesignerDropPlan plan = session.PlanAddControl(
        type, parent, position, positioned);
    if(!plan.valid)
        return 0;
    String error;
    UiDesignerNodeId created = 0;
    return session.ExecuteDrop(plan, &created, error) ? created : 0;
}

static bool BuildFixture(UiDesignerSession& session, String& error)
{
    session.NewDocument("blank");
    const UiDesignerNodeId root = session.Document().GetRootId();
    UiDesignerNodeId box = AddThroughDrop(session, "UiBoxLayout", root,
                                          Point(20, 20), true);
    if(!box) { error = "Unable to add BoxLayout"; return false; }
    session.Commands().SetProperty(box, "direction", "V",
        UiDesignerImpactStructure | UiDesignerImpactCode);

    UiDesignerNodeId label = AddThroughDrop(session, "UiLabel", box);
    UiDesignerNodeId spacer = AddThroughDrop(session, "Spacer", box);
    UiDesignerNodeId button = AddThroughDrop(session, "UiButton", box);
    UiDesignerNodeId grid = AddThroughDrop(session, "UiGridLayout", box);
    UiDesignerNodeId stock = AddThroughDrop(session, "UppLabel", box);
    if(!label || !spacer || !button || !grid || !stock) {
        error = "Unable to build fixture nodes";
        return false;
    }

    session.Commands().SetProperty(label, "text",
        "Quoted \"text\"\\path\nsecond line",
        UiDesignerImpactControlState | UiDesignerImpactCode);
    session.Commands().SetProperty(spacer, "weight", 2.0,
        UiDesignerImpactAncestorLayout | UiDesignerImpactCode);
    session.Commands().SetProperty(spacer, "line_enabled", true,
        UiDesignerImpactPaint | UiDesignerImpactCode);
    session.Commands().SetProperty(spacer, "line_dash", "Dash",
        UiDesignerImpactPaint | UiDesignerImpactCode);
    session.Commands().SetProperty(button, "text", "Close",
        UiDesignerImpactControlState | UiDesignerImpactCode);
    session.Commands().SetProperty(stock, "text", "Stock U++ label",
        UiDesignerImpactControlState | UiDesignerImpactCode);

    UiDesignerNodeId grid_spacer = AddThroughDrop(session, "Spacer", grid,
                                                  Point(40, 40), true);
    UiDesignerNodeId edit = AddThroughDrop(session, "UiLineEdit", grid,
                                           Point(200, 40), true);
    if(!grid_spacer || !edit) {
        error = "Unable to build Grid fixture";
        return false;
    }
    session.Commands().SetProperty(grid_spacer, "line_enabled", true,
        UiDesignerImpactPaint | UiDesignerImpactCode);
    session.Commands().SetProperty(grid_spacer, "line_orientation", "Vertical",
        UiDesignerImpactPaint | UiDesignerImpactCode);

    UiDesignerActionBinding binding;
    binding.event_id = "WhenAction";
    binding.action = UiDesignerActionType::CloseWindow;
    binding.enabled = true;
    if(!session.Commands().SetActionBinding(button, binding, "Bind close")) {
        error = session.Commands().GetLastError();
        return false;
    }
    error.Clear();
    return true;
}

static bool EmitFixture(const String& folder, String& error)
{
    UiDesignerSession session;
    if(!BuildFixture(session, error))
        return false;
    UiDesignerExportRequest request;
    request.profile = UiDesignerExportProfile::CompleteCppPackage;
    request.destination = folder;
    request.generation.package_name = "GeneratedUi";
    request.generation.class_name = "GeneratedUiWindow";
    request.generation.namespace_name = "Upp";
    request.write.overwrite = UiDesignerOverwritePolicy::ReplaceAll;
    request.write.preserve_user_files = false;
    UiDesignerExportService service(session.Catalog());
    UiDesignerExportResult result = service.Execute(
        session.Document(), session.Theme(), request);
    if(!result.success) {
        error = result.diagnostic;
        return false;
    }
    Cout() << "EXPORTED " << folder << '\n';
    for(const String& file : result.written_files)
        Cout() << "FILE " << file << '\n';
    return true;
}

static String LegacySpacerJson()
{
    return R"JSON({
      "format":"upp-ui-designer",
      "virtual_size":{"cx":640,"cy":480},
      "nodes":[
        {"id":1,"parent":0,"type":"Window","name":"Window","properties":{}},
        {"id":2,"parent":1,"type":"Spacer","name":"legacy_spacer",
         "properties":{"weight":{"type":"number","value":2},
                       "line_enabled":{"type":"bool","value":true}}}
      ]
    })JSON";
}

static void RunTests(FoundationTester& t)
{
    UiDesignerSession session;
    String error;
    t.Check(session.Catalog().Validate(error), "catalog validates with adapter/event contracts");
    const UiDesignerControlSpec* spacer_spec = session.Catalog().Find("Spacer");
    t.Check(spacer_spec != nullptr, "Spacer is registered");
    t.Check(spacer_spec && spacer_spec->IsSemanticItem(), "Spacer is semantic, not a Ctrl");
    t.Check(spacer_spec && spacer_spec->FindProperty("line_enabled"), "Spacer exposes separator properties");
    const UiDesignerControlSpec* button_spec = session.Catalog().Find("UiButton");
    t.Check(button_spec && button_spec->FindEvent("WhenAction"), "Button exposes typed action event");

    UiDesignerDocument legacy;
    t.Check(UiDesignerDeserialize(LegacySpacerJson(), legacy, error),
            "legacy root Spacer imports");
    const UiDesignerNode* legacy_root = legacy.Find(legacy.GetRootId());
    t.Check(legacy_root && !legacy_root->children.IsEmpty(),
            "legacy import creates an implicit layout");
    const UiDesignerNode* implicit = legacy_root && !legacy_root->children.IsEmpty()
        ? legacy.Find(legacy_root->children[0]) : nullptr;
    t.Check(implicit && implicit->type == "UiBoxLayout",
            "legacy root Spacer is placed in UiBoxLayout");
    const UiDesignerNode* imported_spacer = implicit && !implicit->children.IsEmpty()
        ? legacy.Find(implicit->children[0]) : nullptr;
    t.Check(imported_spacer && imported_spacer->type == "Spacer",
            "legacy Spacer keeps semantic identity");

    session.NewDocument("blank");
    const UiDesignerNodeId root = session.Document().GetRootId();
    const int before_plan_count = session.Document().GetCount();
    UiDesignerDropPlan invalid_spacer = session.PlanAddControl("Spacer", root);
    t.Check(!invalid_spacer.valid, "Spacer rejects root insertion");
    t.Check(session.Document().GetCount() == before_plan_count,
            "invalid plan does not mutate document");

    UiDesignerDropPlan box_plan = session.PlanAddControl(
        "UiBoxLayout", root, Point(21, 19), true);
    t.Check(box_plan.valid, "BoxLayout add plan is valid");
    t.Check(session.Document().GetCount() == before_plan_count,
            "valid add plan is still pure");
    UiDesignerNodeId box = 0;
    t.Check(session.ExecuteDrop(box_plan, &box, error), "terminal add executes");
    t.Check(box != 0 && session.Document().GetCount() == before_plan_count + 1,
            "terminal add creates one node");
    const int history_after_box = session.Commands().GetHistoryPosition();

    UiDesignerDropPlan spacer_plan = session.PlanAddControl("Spacer", box);
    t.Check(spacer_plan.valid, "Spacer plan is valid inside BoxLayout");
    UiDesignerNodeId spacer = 0;
    t.Check(session.ExecuteDrop(spacer_plan, &spacer, error), "Box Spacer executes");
    t.Check(session.Commands().GetHistoryPosition() == history_after_box + 1,
            "Spacer drop creates one history entry");
    t.Check(session.Undo(), "Spacer drop undo succeeds");
    t.Check(!session.Document().Find(spacer), "Spacer undo removes semantic node");
    t.Check(session.Redo(), "Spacer drop redo succeeds");

    UiDesignerNodeId button = AddThroughDrop(session, "UiButton", box);
    t.Check(button != 0, "button added through shared drop service");
    UiDesignerActionBinding binding;
    binding.event_id = "WhenAction";
    binding.action = UiDesignerActionType::CloseWindow;
    t.Check(session.Commands().SetActionBinding(button, binding),
            "typed behavior command succeeds");
    t.Check(session.Document().GetActionBinding(button, "WhenAction") != nullptr,
            "typed behavior persists in document");
    const String serialized = UiDesignerSerialize(session.Document(), true);
    UiDesignerDocument roundtrip;
    t.Check(UiDesignerDeserialize(serialized, roundtrip, error),
            "behavior document round trip succeeds");
    t.Check(roundtrip.GetActionBinding(button, "WhenAction") != nullptr,
            "behavior survives round trip");
    t.Check(session.Undo(), "behavior undo succeeds");
    t.Check(session.Document().GetActionBinding(button, "WhenAction") == nullptr,
            "behavior undo restores unbound state");
    t.Check(session.Redo(), "behavior redo succeeds");

    UiDesignerNodeId child_panel = AddThroughDrop(session, "UiPanel", box);
    UiDesignerNodeId nested = AddThroughDrop(session, "UiPanel", child_panel);
    Vector<UiDesignerNodeId> move_nodes;
    move_nodes.Add(child_panel);
    UiDesignerDropPlan cyclic = session.Drops().PlanMove(
        move_nodes, nested, Point(), false, -1);
    t.Check(!cyclic.valid, "drop planner rejects descendant target");

    UiDesignerCodeGenerator generator(session.Catalog());
    UiDesignerCodeGenerationOptions generation;
    generation.package_name = "GeneratedUi";
    generation.class_name = "GeneratedUiWindow";
    UiDesignerGeneratedProject project = generator.Generate(
        session.Document(), generation);
    t.Check(project.IsValid(), "generated project validates");
    t.Check(project.generated_header.Find("Spacer") < 0,
            "Spacer does not emit a runtime member");
    t.Check(project.generated_source.Find(".AddSpacer(") >= 0,
            "Box Spacer emits semantic layout API");
    t.Check(project.generated_source.Find("WhenAction =") >= 0,
            "typed behavior emits generated binding");
    t.Check(project.FindFile("GeneratedUiWindow.generated.h") != nullptr,
            "generated base header inventory exists");
    const UiDesignerGeneratedFile* user_source =
        project.FindFile("GeneratedUiWindow.cpp");
    t.Check(user_source && !user_source->generator_owned,
            "user implementation is marked preserved");

    const String temp = AppendFileName(GetTempPath(),
        "uidesigner-foundation-" + AsString(Uuid::Create()));
    DeleteFolderDeep(temp);
    UiDesignerExportRequest complete;
    complete.profile = UiDesignerExportProfile::CompleteCppPackage;
    complete.destination = temp;
    complete.generation = generation;
    complete.write.overwrite = UiDesignerOverwritePolicy::ReplaceGenerated;
    UiDesignerExportService export_service(session.Catalog());
    UiDesignerExportResult preview = export_service.Preview(
        session.Document(), session.Theme(), complete);
    t.Check(preview.success && preview.inventory.GetCount() >= 7,
            "complete export previews full inventory");
    UiDesignerExportResult first = export_service.Execute(
        session.Document(), session.Theme(), complete);
    t.Check(first.success, "complete export succeeds atomically");
    const String user_path = AppendFileName(temp, "GeneratedUiWindow.cpp");
    t.Check(FileExists(user_path), "user implementation is exported");
    SaveFile(user_path, "// preserved user code\n");
    UiDesignerExportResult second = export_service.Execute(
        session.Document(), session.Theme(), complete);
    t.Check(second.success, "repeat export succeeds");
    t.Check(LoadFile(user_path) == "// preserved user code\n",
            "repeat export preserves user code");

    UiDesignerExportRequest document_json;
    document_json.profile = UiDesignerExportProfile::DocumentJson;
    document_json.destination = AppendFileName(temp, "document.json");
    UiDesignerExportResult json_result = export_service.Execute(
        session.Document(), session.Theme(), document_json);
    t.Check(json_result.success && json_result.written_files.GetCount() == 1,
            "document JSON export is distinct and writes one file");

    UiDesignerAutomationService automation(session);
    ValueMap search_params;
    search_params.Set("query", "spacer");
    ValueMap search_result = automation.ListControls(search_params);
    t.Check(UiDesignerMapValue(search_result, "ok", false),
            "automation catalog search succeeds");
    ValueArray tools = automation.ListMcpTools();
    bool has_drop = false, has_behavior = false, has_export = false;
    for(const Value& value : tools) {
        ValueMap tool = value;
        const String name = UiDesignerMapValue(tool, "name", "");
        has_drop |= name == "uidesigner_apply_drop";
        has_behavior |= name == "uidesigner_set_behavior";
        has_export |= name == "uidesigner_export";
    }
    t.Check(has_drop && has_behavior && has_export,
            "MCP tool list exposes drop, behavior and export");

    DeleteFolderDeep(temp);
}

CONSOLE_APP_MAIN
{
    const Vector<String>& args = CommandLine();
    if(args.GetCount() >= 2 && args[0] == "--export-fixture") {
        String error;
        if(!EmitFixture(args[1], error)) {
            Cerr() << error << '\n';
            SetExitCode(1);
        }
        return;
    }

    FoundationTester tester;
    RunTests(tester);
    Cout() << "SUMMARY checks=" << tester.checks
           << " failures=" << tester.failures << '\n';
    SetExitCode(tester.failures ? 1 : 0);
}
