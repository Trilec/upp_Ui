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

static bool IsSuccessfulJsonRpcResponse(const String& response)
{
    Value parsed = ParseJSON(response);
    if(IsError(parsed) || !parsed.Is<ValueMap>())
        return false;
    ValueMap object = parsed;
    return object.Find("result") >= 0 && object.Find("error") < 0;
}

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

static bool BindAction(UiDesignerSession& session, UiDesignerNodeId node,
                       UiDesignerActionType action,
                       UiDesignerNodeId target = 0,
                       Value value = Value(),
                       const String& property = String(),
                       const String& handler = String())
{
    UiDesignerActionBinding binding;
    binding.event_id = "WhenAction";
    binding.action = action;
    binding.target = target;
    binding.value = value;
    binding.target_property = property;
    binding.handler_name = handler;
    binding.enabled = true;
    return session.Commands().SetActionBinding(node, binding, "Bind fixture action");
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
    UiDesignerNodeId close_button = AddThroughDrop(session, "UiButton", box);
    UiDesignerNodeId accept_button = AddThroughDrop(session, "UiButton", box);
    UiDesignerNodeId grid = AddThroughDrop(session, "UiGridLayout", box);
    UiDesignerNodeId absolute = AddThroughDrop(session, "UiAbsoluteLayout", box);
    UiDesignerNodeId stack = AddThroughDrop(session, "UiStack", box);
    UiDesignerNodeId tab = AddThroughDrop(session, "UiTab", box);
    UiDesignerNodeId splitter = AddThroughDrop(session, "UiSplitter", box);
    UiDesignerNodeId quad = AddThroughDrop(session, "UiQuadSplitter", box);
    UiDesignerNodeId stock = AddThroughDrop(session, "UppLabel", box);
    if(!label || !spacer || !close_button || !accept_button || !grid || !absolute ||
       !stack || !tab || !splitter || !quad || !stock) {
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
    session.Commands().SetProperty(close_button, "text", "Close",
        UiDesignerImpactControlState | UiDesignerImpactCode);
    session.Commands().SetProperty(accept_button, "text", "Accept",
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
    UiDesignerNodeId absolute_button = AddThroughDrop(
        session, "UiButton", absolute, Point(37, 29), true);
    if(!absolute_button) {
        error = "Unable to build AbsoluteLayout fixture";
        return false;
    }
    session.Commands().SetProperty(grid_spacer, "line_enabled", true,
        UiDesignerImpactPaint | UiDesignerImpactCode);
    session.Commands().SetProperty(grid_spacer, "line_orientation", "Vertical",
        UiDesignerImpactPaint | UiDesignerImpactCode);
    session.Commands().SetProperty(grid_spacer, "line_align", "End",
        UiDesignerImpactPaint | UiDesignerImpactCode);

    UiDesignerNodeId stack_a = AddThroughDrop(session, "UiPanel", stack);
    UiDesignerNodeId stack_b = AddThroughDrop(session, "UiPanel", stack);
    UiDesignerNodeId tab_a = AddThroughDrop(session, "UiPanel", tab);
    UiDesignerNodeId tab_b = AddThroughDrop(session, "UiPanel", tab);
    UiDesignerNodeId split_a = AddThroughDrop(session, "UiPanel", splitter);
    UiDesignerNodeId split_b = AddThroughDrop(session, "UiPanel", splitter);
    if(!stack_a || !stack_b || !tab_a || !tab_b || !split_a || !split_b) {
        error = "Unable to build page/splitter fixture";
        return false;
    }
    for(int i = 0; i < 4; i++)
        if(!AddThroughDrop(session, "UiPanel", quad)) {
            error = "Unable to build QuadSplitter fixture";
            return false;
        }

    if(!BindAction(session, close_button, UiDesignerActionType::CloseWindow) ||
       !BindAction(session, accept_button, UiDesignerActionType::AcceptDialog)) {
        error = session.Commands().GetLastError();
        return false;
    }
    UiDesignerNodeId page_button = AddThroughDrop(session, "UiButton", box);
    if(!page_button ||
       !BindAction(session, page_button, UiDesignerActionType::ActivatePage,
                   stack, 1)) {
        error = "Unable to bind page action";
        return false;
    }
    error.Clear();
    return true;
}

static UiDesignerCodeGenerationOptions FixtureOptions()
{
    UiDesignerCodeGenerationOptions options;
    options.package_name = "GeneratedPackage";
    options.class_name = "GeneratedUiWindow";
    options.namespace_name = "Upp";
    options.include_source_design = true;
    options.include_theme = true;
    return options;
}

static bool EmitFixture(const String& folder, String& error)
{
    UiDesignerSession session;
    if(!BuildFixture(session, error))
        return false;
    UiDesignerExportRequest request;
    request.profile = UiDesignerExportProfile::CompleteCppPackage;
    request.destination = folder;
    request.generation = FixtureOptions();
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
    bool default_has_splitter = false;
    for(const UiDesignerNode& node : session.Document().GetNodes())
        default_has_splitter = default_has_splitter || node.type == "UiSplitter";
    t.Check(!default_has_splitter,
            "new Designer document starts without a splitter preset");
    t.Check(session.Catalog().Validate(error),
            "catalog validates with adapter/event contracts");
    const UiDesignerControlSpec* spacer_spec = session.Catalog().Find("Spacer");
    t.Check(spacer_spec != nullptr, "Spacer is registered");
    t.Check(spacer_spec && spacer_spec->IsSemanticItem(),
            "Spacer is semantic, not a Ctrl");
    t.Check(spacer_spec && spacer_spec->FindProperty("line_enabled"),
            "Spacer exposes separator properties");
    const UiDesignerControlSpec* button_spec = session.Catalog().Find("UiButton");
    t.Check(button_spec && button_spec->FindEvent("WhenAction"),
            "Button exposes typed action event");
    const UiDesignerControlSpec* absolute_spec =
        session.Catalog().Find("UiAbsoluteLayout");
    t.Check(absolute_spec && absolute_spec->child_adapter_id == "absolute",
            "AbsoluteLayout has a registered child adapter");
    t.Check(absolute_spec && HasUiDesignerCapability(
                absolute_spec->capabilities, UiDesignerCapabilityFreeform),
            "AbsoluteLayout accepts local freeform placement");
    t.Check(absolute_spec && absolute_spec->FindProperty("x") &&
                absolute_spec->FindProperty("y") &&
                absolute_spec->FindProperty("width") &&
                absolute_spec->FindProperty("height"),
            "AbsoluteLayout geometry is Inspector-bindable");
    t.Check(absolute_spec && !absolute_spec->FindProperty("inset"),
            "AbsoluteLayout omits inherited layout inset");

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
    t.Check(session.Document().GetVirtualSize() == Size(512, 250),
            "blank preset starts at 512x250");
    session.NewDocument("dialog");
    t.Check(session.Document().GetVirtualSize() == Size(512, 250),
            "dialog preset starts at 512x250");
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
    const int history_after_box = session.Commands().GetHistoryPosition();
    UiDesignerDropPlan spacer_plan = session.PlanAddControl("Spacer", box);
    UiDesignerNodeId spacer = 0;
    t.Check(spacer_plan.valid && session.ExecuteDrop(spacer_plan, &spacer, error),
            "Box Spacer terminal drop executes");
    t.Check(session.Commands().GetHistoryPosition() == history_after_box + 1,
            "Spacer drop creates one history entry");
    t.Check(session.Undo() && !session.Document().Find(spacer),
            "Spacer drop undo removes semantic node");
    t.Check(session.Redo(), "Spacer drop redo succeeds");

    UiDesignerNodeId absolute = AddThroughDrop(
        session, "UiAbsoluteLayout", box, Point(24, 24), true);
    UiDesignerDropPlan absolute_child = session.PlanAddControl(
        "UiButton", absolute, Point(37, 29), true);
    t.Check(absolute && absolute_child.valid,
            "AbsoluteLayout accepts a positioned child drop");
    t.Check((int)UiDesignerMapValue(absolute_child.add_defaults, "x", -1) == 40 &&
                (int)UiDesignerMapValue(absolute_child.add_defaults, "y", -1) == 32,
            "AbsoluteLayout drop stores snapped local X/Y");
    t.Check((int)UiDesignerMapValue(absolute_child.add_defaults, "width", -1) == 190 &&
                (int)UiDesignerMapValue(absolute_child.add_defaults, "height", -1) == 34,
            "AbsoluteLayout drop preserves the child size");
    UiDesignerNodeId absolute_button = 0;
    t.Check(session.ExecuteDrop(absolute_child, &absolute_button, error),
            "AbsoluteLayout positioned child drop executes");
    Vector<UiDesignerNodeId> absolute_move_nodes;
    absolute_move_nodes.Add(absolute_button);
    UiDesignerDropPlan absolute_move = session.Drops().PlanMove(
        absolute_move_nodes, absolute, Point(73, 55), true, -1);
    const ValueMap* absolute_move_properties =
        absolute_move.property_updates.GetCount()
            ? &absolute_move.property_updates[0] : nullptr;
    t.Check(absolute_move.valid && absolute_move_properties &&
                (int)UiDesignerMapValue(*absolute_move_properties, "x", -1) == 72 &&
                (int)UiDesignerMapValue(*absolute_move_properties, "y", -1) == 56,
            "AbsoluteLayout move plans an exact local rectangle origin");
    t.Check(absolute_move_properties &&
                absolute_move_properties->Find("width") < 0 &&
                absolute_move_properties->Find("height") < 0,
            "AbsoluteLayout move preserves the existing child size");

    UiDesignerNodeId button = AddThroughDrop(session, "UiButton", box);
    UiDesignerActionBinding binding;
    binding.event_id = "WhenAction";
    binding.action = UiDesignerActionType::CloseWindow;
    t.Check(button && session.Commands().SetActionBinding(button, binding),
            "typed behavior command succeeds");
    const String serialized = UiDesignerSerialize(session.Document(), true);
    UiDesignerDocument roundtrip;
    t.Check(UiDesignerDeserialize(serialized, roundtrip, error),
            "behavior document round trip succeeds");
    t.Check(roundtrip.Find(button) != nullptr,
            "stable node identity survives round trip");
    t.Check(roundtrip.GetActionBinding(button, "WhenAction") != nullptr,
            "behavior survives round trip");
    const UiDesignerNode* roundtrip_absolute_button =
        roundtrip.Find(absolute_button);
    t.Check(roundtrip_absolute_button &&
                (int)roundtrip_absolute_button->GetProperty("x", -1) == 40 &&
                (int)roundtrip_absolute_button->GetProperty("y", -1) == 32 &&
                (int)roundtrip_absolute_button->GetProperty("width", -1) == 190 &&
                (int)roundtrip_absolute_button->GetProperty("height", -1) == 34,
            "AbsoluteLayout child rectangle survives serialization");

    UiDesignerNodeId child_panel = AddThroughDrop(session, "UiPanel", box);
    UiDesignerNodeId nested = AddThroughDrop(session, "UiPanel", child_panel);
    Vector<UiDesignerNodeId> move_nodes;
    move_nodes.Add(child_panel);
    UiDesignerDropPlan cyclic = session.Drops().PlanMove(
        move_nodes, nested, Point(), false, -1);
    t.Check(!cyclic.valid, "drop planner rejects descendant target");

    UiDesignerSession fixture;
    t.Check(BuildFixture(fixture, error), "complete codegen fixture builds in model");
    UiDesignerCodeGenerator generator(fixture.Catalog());
    const UiDesignerCodeGenerationOptions generation = FixtureOptions();
    UiDesignerGeneratedProject project = generator.Generate(
        fixture.Document(), generation);
    t.Check(project.IsValid(), "generated project validates");
    t.Check(project.generated_header.Find("Spacer") < 0,
            "Spacer does not emit a runtime member");
    t.Check(project.generated_header.Find("UiAbsoluteLayout") >= 0,
            "AbsoluteLayout emits a runtime member");
    t.Check(project.generated_source.Find(".AddSpacer(") >= 0,
            "Box Spacer emits semantic layout API");
    t.Check(project.generated_source.Find(".AddBlank(") >= 0,
            "Grid Spacer emits semantic layout API");
    t.Check(project.generated_source.Find("UiGridLayout::Align::End") >= 0,
            "Grid separator uses Grid alignment type");
    t.Check(project.generated_source.Find(
                ".Add(button_n", 0) >= 0 &&
            project.generated_source.Find(
                "DPI(40), DPI(32), DPI(190), DPI(34)") >= 0,
            "AbsoluteLayout emits exact DPI-scaled child placement");
    t.Check(project.generated_source.Find("Break(IDOK)") >= 0,
            "Accept action emits compile-safe dialog break");
    t.Check(project.generated_source.Find(".SetData(1)") >= 0,
            "Activate-page action uses common page data API");
    t.Check(project.generated_source.Find(" << ") >= 0,
            "splitter families use registered attachment adapter");
    t.Check(project.FindFile("GeneratedPackage.upp") != nullptr,
            "package filename is independent from class name");
    t.Check(project.package.Find("GeneratedUiWindow.generated.cpp") >= 0,
            "package manifest lists class-owned source files");
    t.Check(project.package.Find("GeneratedPackage.generated.cpp") < 0,
            "package manifest does not invent package-named class files");
    const UiDesignerGeneratedFile* user_source =
        project.FindFile("GeneratedUiWindow.cpp");
    t.Check(user_source && !user_source->generator_owned,
            "user implementation is marked preserved");

    const String temp = AppendFileName(GetTempPath(),
        "uidesigner-foundation-" + AsString(Uuid::Create()));
    DeleteFolderDeep(temp);
    UiDesignerExportService export_service(fixture.Catalog());

    UiDesignerExportRequest complete;
    complete.profile = UiDesignerExportProfile::CompleteCppPackage;
    complete.destination = AppendFileName(temp, "complete");
    complete.generation = generation;
    complete.write.overwrite = UiDesignerOverwritePolicy::ReplaceGenerated;
    UiDesignerExportResult first = export_service.Execute(
        fixture.Document(), fixture.Theme(), complete);
    t.Check(first.success && first.written_files.GetCount() >= 7,
            "complete package export succeeds atomically");
    const String user_path = AppendFileName(
        complete.destination, "GeneratedUiWindow.cpp");
    SaveFile(user_path, "// preserved user code\n");
    UiDesignerExportResult second = export_service.Execute(
        fixture.Document(), fixture.Theme(), complete);
    t.Check(second.success && LoadFile(user_path) == "// preserved user code\n",
            "repeat package export preserves user code");

    UiDesignerExportRequest component = complete;
    component.profile = UiDesignerExportProfile::ComponentOnly;
    component.destination = AppendFileName(temp, "component");
    UiDesignerExportResult component_result = export_service.Execute(
        fixture.Document(), fixture.Theme(), component);
    t.Check(component_result.success &&
            !FileExists(AppendFileName(component.destination, "main.cpp")) &&
            !FileExists(AppendFileName(component.destination, "GeneratedPackage.upp")),
            "component export omits package entry point");

    const struct JsonCase {
        UiDesignerExportProfile profile;
        const char *name;
    } json_cases[] = {
        {UiDesignerExportProfile::ProjectJson, "project.uidesign.json"},
        {UiDesignerExportProfile::DocumentJson, "document.uidesign.json"},
        {UiDesignerExportProfile::ThemeJson, "theme.json"},
    };
    for(const auto& item : json_cases) {
        UiDesignerExportRequest request;
        request.profile = item.profile;
        request.destination = AppendFileName(temp, item.name);
        request.generation = generation;
        UiDesignerExportResult result = export_service.Execute(
            fixture.Document(), fixture.Theme(), request);
        t.Check(result.success && result.written_files.GetCount() == 1 &&
                FileExists(request.destination),
                String(item.name) + " export is distinct");
    }

    const String refuse_folder = AppendFileName(temp, "refuse");
    RealizeDirectory(refuse_folder);
    const String sentinel = AppendFileName(
        refuse_folder, "GeneratedUiWindow.generated.h");
    SaveFile(sentinel, "sentinel");
    UiDesignerExportRequest refuse = complete;
    refuse.destination = refuse_folder;
    refuse.write.overwrite = UiDesignerOverwritePolicy::RefuseExisting;
    UiDesignerExportResult refused = export_service.Execute(
        fixture.Document(), fixture.Theme(), refuse);
    t.Check(!refused.success && LoadFile(sentinel) == "sentinel" &&
            !FileExists(AppendFileName(refuse_folder,
                                       "GeneratedUiWindow.generated.cpp")),
            "preflight refusal leaves no partial export");

    UiDesignerAutomationService automation(fixture);
    ValueMap search_params;
    search_params.Set("query", "spacer");
    t.Check(UiDesignerMapValue(
                ValueMap(automation.ListControls(search_params)), "ok", false),
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
            "MCP exposes drop, behavior and export tools");

    UiDesignerMcpEndpoint endpoint(automation);
    const String resource_response = endpoint.HandleJsonLine(
        R"JSON({"jsonrpc":"2.0","id":1,"method":"resources/read","params":{"uri":"uidesigner://document"}})JSON");
    t.Check(IsSuccessfulJsonRpcResponse(resource_response),
            "MCP endpoint reads the document resource through its service");

    const String tools_response = endpoint.HandleJsonLine(
        R"JSON({"jsonrpc":"2.0","id":2,"method":"tools/list","params":{}})JSON");
    t.Check(IsSuccessfulJsonRpcResponse(tools_response),
            "MCP endpoint lists tools through its service");

    const String call_response = endpoint.HandleJsonLine(
        R"JSON({"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"uidesigner_get_document","arguments":{}}})JSON");
    t.Check(IsSuccessfulJsonRpcResponse(call_response),
            "MCP endpoint dispatches tool calls through its service");

    ValueMap stale;
    stale.Set("operation", "add");
    stale.Set("type", "UiLabel");
    stale.Set("target", fixture.Document().GetRootId());
    stale.Set("expected_revision", (int64)fixture.Document().GetRevision() - 1);
    ValueMap stale_result = automation.ApplyDrop(stale);
    t.Check(!UiDesignerMapValue(stale_result, "ok", true),
            "automation rejects stale drop revision");

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
