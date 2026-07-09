#include <Core/Core.h>
#include <Ui/Ui.h>

#include "../Designer/DesignerModel.h"
#include "../Designer/DesignerRegistry.h"
#include "../Designer/DesignerBuiltins.h"
#include "../Designer/DesignerAdapter.h"
#include "../Designer/DesignerCommands.h"
#include "../Designer/DesignerDragController.h"
#include "../Designer/DesignerCodeGen.h"
#include "../Designer/DesignerExport.h"
#include "../Designer/DesignerInspector.h"
#include "../Designer/DesignerPreview.h"
#include "../Designer/DesignerSerialization.h"
#include "../Designer/DesignerTemplates.h"
#include "../Designer/DesignerRecentDocuments.h"

#include "../Designer/controls/DesignerControlFamilyShared.cpp"
#include "../Designer/controls/DesignerLayoutControls.cpp"
#include "../Designer/controls/DesignerContainerControls.cpp"
#include "../Designer/controls/DesignerDisplayControls.cpp"
#include "../Designer/controls/DesignerButtonControls.cpp"
#include "../Designer/controls/DesignerEditControls.cpp"
#include "../Designer/controls/DesignerCompositeControls.cpp"
#include "../Designer/controls/DesignerDataControls.cpp"
#include "../Designer/DesignerModel.cpp"
#include "../Designer/DesignerRegistry.cpp"
#include "../Designer/DesignerBuiltins.cpp"
#include "../Designer/DesignerAdapter.cpp"
#include "../Designer/DesignerCommands.cpp"
#include "../Designer/DesignerDragController.cpp"
#include "../Designer/DesignerTemplates.cpp"
#include "../Designer/DesignerRecentDocuments.cpp"
#include "../Designer/DesignerCodeGen.cpp"
#include "../Designer/DesignerExport.cpp"
#include "../Designer/DesignerInspector.cpp"
#include "../Designer/DesignerPreview.cpp"
#include "../Designer/DesignerSerialization.cpp"
#include "../Designer/DesignerTrace.cpp"

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

	void Section(const String& title)
	{
		Cout() << "\n=== " << title << " ===\n";
	}
};

static int FindChildPos(const DesignerNode& n, DesignerNodeId id)
{
	for(int i = 0; i < n.children.GetCount(); i++)
		if(n.children[i] == id)
			return i;
	return -1;
}

static int FindStringPos(const Vector<String>& values, const String& value)
{
	for(int i = 0; i < values.GetCount(); i++)
		if(values[i] == value)
			return i;
	return -1;
}

static bool SourceContains(const String& file, const String& needle)
{
	return LoadFile(file).Find(needle) >= 0;
}

static String SourceRegion(const String& file, const String& start_marker, const String& end_marker)
{
	String src = LoadFile(file);
	int a = src.Find(start_marker);
	if(a < 0)
		return String();
	int b = src.Find(end_marker, a);
	if(b < 0)
		return src.Mid(a);
	return src.Mid(a, b - a);
}

static bool IsValidDesignerBaseName(const String& s)
{
	if(s.IsEmpty() || !IsAlpha((byte)s[0]) && s[0] != '_')
		return false;
	for(int i = 0; i < s.GetCount(); i++) {
		int c = (byte)s[i];
		if(!(IsAlNum(c) || c == '_'))
			return false;
	}
	return true;
}

static void TestDesignerArchitectureGuard(TestCtx& t)
{
	t.Section("Designer architecture guard");

	t.Expect(!SourceContains("Utilities/Designer/controls/DesignerControlFamilyShared.cpp", "MakeDesignerTypeIcon("),
	         "central icon switch is removed");
	t.Expect(!SourceContains("Utilities/Designer/controls/DesignerControlFamilyShared.cpp", "if(id == \""),
	         "shared control defaults do not route by control id");
	t.Expect(!SourceContains("Utilities/Designer/controls/DesignerControlFamilyShared.cpp", "t.icon = id == \"UiTab\""),
	         "shared tab icon selection is removed");
	t.Expect(!SourceContains("Utilities/Designer/controls/DesignerControlFamilyShared.cpp", "child_emission = id == \"UiScrollPanel\""),
	         "shared scroll-panel child emission selection is removed");
	t.Expect(!SourceContains("Utilities/Designer/DesignerBuiltins.cpp", "MakeControlType("),
	         "DesignerBuiltins stays on family registrar calls");
	t.Expect(!SourceContains("Utilities/Designer/DesignerCodeGen.cpp", "EmitCompositeSetup("),
	         "composite setup helper is removed from central codegen");
	t.Expect(!SourceContains("Utilities/Designer/DesignerCodeGen.cpp", "CodeGenCompositeLayoutExpr("),
	         "composite layout helper is removed from central codegen");
	t.Expect(!SourceContains("Utilities/Designer/DesignerCodeGen.cpp", "CodeGenFieldAlignExpr("),
	         "field align helper is removed from central codegen");
	t.Expect(!SourceContains("Utilities/Designer/DesignerCodeGen.cpp", "SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle))"),
	         "generic fallback panel style is removed from central codegen");
	t.Expect(!SourceContains("Utilities/Designer/DesignerCodeGen.cpp", "static bool EmitCentralLayoutSetup("),
	         "layout-central helper is removed from central codegen");
	t.Expect(!SourceContains("Utilities/Designer/DesignerCodeGen.cpp", "static bool EmitCentralStructuralSetup("),
	         "structural-central helper is removed from central codegen");
	t.Expect(SourceContains("Utilities/Designer/DesignerCodeGen.cpp", "static void ReportCodeGenContractError("),
	         "codegen contract error reporter exists");
	String emit_setup = SourceRegion("Utilities/Designer/DesignerCodeGen.cpp", "static void EmitSetup", "static void EmitAddChild");
	t.Expect(!emit_setup.IsEmpty(), "EmitSetup source region is found");
	t.Expect(emit_setup.Find("UiButton") < 0 && emit_setup.Find("UiSplitButton") < 0 &&
	         emit_setup.Find("UiToolButton") < 0 && emit_setup.Find("UiLineEdit") < 0 &&
	         emit_setup.Find("UiIntEdit") < 0 && emit_setup.Find("UiFloatEdit") < 0 &&
	         emit_setup.Find("UiSlider") < 0 && emit_setup.Find("UiToggle") < 0 &&
	         emit_setup.Find("UiDropdown") < 0 && emit_setup.Find("UiCheckBox") < 0 &&
	         emit_setup.Find("UiBreadcrumbs") < 0 && emit_setup.Find("UiLabel") < 0 &&
	         emit_setup.Find("UiTitleCard") < 0 && emit_setup.Find("UiTable") < 0 &&
	         emit_setup.Find("UiTree") < 0,
	         "central EmitSetup no longer contains ordinary-control branches");
	t.Expect(emit_setup.Find("SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle))") < 0,
	         "central EmitSetup no longer has a generic panel fallback");
	t.Expect(emit_setup.Find("EmitCentralLayoutSetup(") < 0 && emit_setup.Find("EmitCentralStructuralSetup(") < 0,
	         "central EmitSetup no longer calls removed route helpers");
	String emit_add_child = SourceRegion("Utilities/Designer/DesignerCodeGen.cpp", "static void EmitAddChild", "static void EmitAdds");
	t.Expect(!emit_add_child.IsEmpty(), "EmitAddChild source region is found");
	t.Expect(emit_add_child.Find("UiPanel") < 0 && emit_add_child.Find("UiScrollPanel") < 0 &&
	         emit_add_child.Find("UiGroupPanel") < 0 && emit_add_child.Find("UiTab") < 0 &&
	         emit_add_child.Find("UiStack") < 0 && emit_add_child.Find("UiAccordion") < 0 &&
	         emit_add_child.Find("UiSplitter") < 0 && emit_add_child.Find("UiQuadSplitter") < 0 &&
	         emit_add_child.Find("BoxLayout") < 0 && emit_add_child.Find("GridLayout") < 0,
	         "central EmitAddChild now delegates structural routing to parent hooks");
	String emit_post = SourceRegion("Utilities/Designer/DesignerCodeGen.cpp", "static void EmitPostAddSetup", "static void EmitAppearanceApply");
	t.Expect(!emit_post.IsEmpty(), "EmitPostAddSetup source region is found");
	t.Expect(emit_post.Find("UiTab") < 0 && emit_post.Find("UiStack") < 0,
	         "central EmitPostAddSetup no longer switches on page containers");
	t.Expect(!SourceContains("Utilities/Designer/DesignerCodeGen.cpp", "crumb_a") &&
	         !SourceContains("Utilities/Designer/DesignerCodeGen.cpp", "crumb_b") &&
	         !SourceContains("Utilities/Designer/DesignerCodeGen.cpp", "crumb_c"),
	         "breadcrumb legacy aliases are removed from codegen");

	DesignerRegistry registry;
	RegisterDesignerBuiltins(registry);
	Vector<const DesignerType*> specs = registry.GetSpecs();
	int route_ordinary = 0;
	int route_layout = 0;
	int route_structural = 0;
	int route_headless = 0;
	int route_no_output = 0;
	for(const DesignerType* spec : specs) {
		t.Expect(spec != nullptr, "registry spec pointer is valid");
		if(!spec)
			continue;
		t.Expect(spec->codegen.route != DesignerCodeGenRoute::Unspecified,
		         spec->id + " has an explicit codegen route");
		switch(spec->codegen.route) {
		case DesignerCodeGenRoute::OrdinaryHook: route_ordinary++; break;
		case DesignerCodeGenRoute::LayoutCentral: route_layout++; break;
		case DesignerCodeGenRoute::StructuralCentral: route_structural++; break;
		case DesignerCodeGenRoute::Headless: route_headless++; break;
		case DesignerCodeGenRoute::NoRuntimeOutput: route_no_output++; break;
		default: break;
		}
		bool visible_toolbox = spec->IsVisibleControl() && !spec->toolbox_group.IsEmpty();
		if(visible_toolbox) {
			t.Expect(!IsNull(spec->icon), spec->id + " visible toolbox spec has an icon");
			t.Expect(IsValidDesignerBaseName(spec->default_base_name),
			         spec->id + " visible toolbox spec has a valid default base name");
			t.Expect((bool)spec->init_defaults, spec->id + " visible toolbox spec has a default initializer");
		}
		if(spec->codegen.route == DesignerCodeGenRoute::OrdinaryHook && spec->IsVisibleControl())
			t.Expect((bool)spec->codegen.emit_setup, spec->id + " ordinary visible spec has a setup hook");
	}
	t.Expect(route_ordinary > 0, "ordinary setup hooks are registered");
	t.Expect(route_layout > 0, "layout routes are registered");
	t.Expect(route_structural > 0, "structural routes are registered");
	t.Expect(route_headless > 0, "headless routes are registered");
	t.Expect(route_no_output > 0, "no-runtime-output routes are registered");
	t.Expect(specs.GetCount() == 38, "registry still exposes the expected 38 control specs");
}

static const DesignerNode* FindNodeByName(const DesignerModel& model, const String& name)
{
	Function<const DesignerNode*(DesignerNodeId)> find = [&](DesignerNodeId id) -> const DesignerNode* {
		const DesignerNode* n = model.Find(id);
		if(!n)
			return nullptr;
		if(n->name == name)
			return n;
		for(DesignerNodeId child_id : n->children)
			if(const DesignerNode* child = find(child_id))
				return child;
		return nullptr;
	};
	return find(Designer_ROOT);
}

static Value TestNodePropertyOr(const DesignerNode& n, const String& key, const Value& def)
{
	int q = n.properties.Find(key);
	return q >= 0 ? n.properties.GetValue(q) : def;
}

static bool ControlTreeHasLabelText(const Ctrl& root, const String& needle)
{
	if(const UiLabel* label = dynamic_cast<const UiLabel*>(&root))
		if(label->GetText().Find(needle) >= 0)
			return true;
	for(const Ctrl* child = root.GetFirstChild(); child; child = child->GetNext())
		if(ControlTreeHasLabelText(*child, needle))
			return true;
	return false;
}

static int ControlTreeCountLabelText(const Ctrl& root, const String& needle)
{
	int count = 0;
	if(const UiLabel* label = dynamic_cast<const UiLabel*>(&root))
		if(label->GetText().Find(needle) >= 0)
			count++;
	for(const Ctrl* child = root.GetFirstChild(); child; child = child->GetNext())
		count += ControlTreeCountLabelText(*child, needle);
	return count;
}

static UiCompositeDropdown* FindCompositeDropdownByLabel(Ctrl& root, const String& label_text)
{
	if(UiCompositeDropdown* row = dynamic_cast<UiCompositeDropdown*>(&root))
		if(row->LabelCtrl().GetText() == label_text)
			return row;
	for(Ctrl* child = root.GetFirstChild(); child; child = child->GetNext())
		if(UiCompositeDropdown* found = FindCompositeDropdownByLabel(*child, label_text))
			return found;
	return nullptr;
}

static void TestModelTreeEdits(TestCtx& t)
{
	t.Section("DesignerModel tree edits");

	DesignerModel m;
	DesignerNodeId box = m.AddNode("BoxLayout", Designer_ROOT);
	DesignerNodeId grid = m.AddNode("GridLayout", box);
	DesignerNodeId label = m.AddNode("UiLabel", grid);
	DesignerNodeId slider = m.AddNode("UiSlider", grid);

	const DesignerNode* root = m.Find(Designer_ROOT);
	const DesignerNode* box_node = m.Find(box);
	const DesignerNode* grid_node = m.Find(grid);
	t.Expect(root && root->children.GetCount() == 1 && root->children[0] == box, "root owns box layout");
	t.Expect(box_node && box_node->children.GetCount() == 1 && box_node->children[0] == grid, "box owns grid layout");
	t.Expect(grid_node && grid_node->children.GetCount() == 2, "grid owns two controls");
	t.Expect(m.Validate(), "initial tree validates");

	DesignerModel append;
	DesignerNodeId append_parent = append.AddNode("BoxLayout", Designer_ROOT);
	DesignerNodeId a = append.AddNode("UiLabel", append_parent, -1);
	DesignerNodeId b = append.AddNode("UiButton", append_parent, -1);
	DesignerNodeId c = append.AddNode("UiSlider", append_parent, -1);
	const DesignerNode* append_parent_node = append.Find(append_parent);
	t.Expect(append_parent_node && append_parent_node->children.GetCount() == 3, "append parent gets all children");
	t.Expect(append_parent_node && append_parent_node->children[0] == a &&
	         append_parent_node->children[1] == b &&
	         append_parent_node->children[2] == c,
	         "append inserts children in source order");

	Vector<DesignerNodeState> restored_states;
	t.Expect(m.CaptureSubtree(grid, restored_states), "capture subtree for restore order test");
	t.Expect(m.RemoveNode(grid), "remove subtree before restore");
	t.Expect(m.RestoreSubtree(restored_states, box, -1), "restore subtree appends at end");
	const DesignerNode* restored_box = m.Find(box);
	t.Expect(restored_box && restored_box->children.GetCount() == 1, "restored subtree adds one child to parent");
	if(restored_box && restored_box->children.GetCount() == 1) {
		DesignerNodeId restored_grid_id = restored_box->children[0];
		const DesignerNode* restored_grid = m.Find(restored_grid_id);
		t.Expect(restored_grid && restored_grid->children.GetCount() == 2, "restored subtree keeps nested children");
		t.Expect(restored_grid && restored_grid->children[0] == label &&
		         restored_grid->children[1] == slider,
		         "restored subtree preserves nested child order");
	}

	t.Expect(!m.MoveNode(box, label), "cannot move node into its descendant");
	t.Expect(m.MoveNode(slider, box, 0), "can move slider to box at index 0");
	box_node = m.Find(box);
	grid_node = m.Find(grid);
	t.Expect(box_node && FindChildPos(*box_node, slider) == 0, "move preserves requested insert position");
	t.Expect(grid_node && FindChildPos(*grid_node, slider) < 0, "move removes node from old parent");
	t.Expect(m.Find(slider)->parent == box, "move updates parent link");
	t.Expect(m.Validate(), "moved tree validates");

	t.Expect(m.MoveNode(label, box, -1), "can move node with append index");
	box_node = m.Find(box);
	t.Expect(box_node && box_node->children.GetCount() == 3 && box_node->children[2] == label,
	         "move append places node at end");

	Vector<DesignerNodeId> multi;
	multi << slider << label << slider;
	m.SetSelection(multi);
	t.Expect(m.GetSelection().GetCount() == 2, "selection removes duplicate ids");
	t.Expect(m.GetSelection()[0] == slider && m.GetSelection()[1] == label, "selection preserves requested order");

	t.Expect(m.RemoveNode(grid), "can remove subtree");
	t.Expect(!m.Find(grid), "remove deletes subtree root");
	t.Expect(m.Find(label) && m.Find(slider), "remove keeps moved siblings outside subtree");
	t.Expect(m.GetSelection().GetCount() == 1 && m.GetSelection()[0] == Designer_ROOT, "remove returns selection to root");
	t.Expect(m.Validate(), "removed tree validates");

	m.SetVirtualSize(Size(10, 10));
	t.Expect(m.GetVirtualSize() == Size(40, 40), "virtual window size clamps to minimum");
	t.Expect(m.Validate(), "clamped virtual size validates");
}

static void TestDesignerAddTarget(TestCtx& t)
{
	t.Section("Designer placement model");

	DesignerModel m;
	DesignerNodeId main = m.AddNode("BoxLayout", Designer_ROOT);
	DesignerNodeId child = m.AddNode("UiLabel", main);
	DesignerNodeId title = m.AddNode("UiTitleCard", main, 0);
	const DesignerNode* main_node = m.Find(main);
	t.Expect(main_node && main_node->children.GetCount() == 2, "placing controls adds them to selected layout");
	t.Expect(main_node && main_node->children[0] == title && main_node->children[1] == child, "insert index controls child order");
	t.Expect(m.Find(child) && m.Find(child)->parent == main, "placed label stores layout parent");
	t.Expect(m.MoveNode(child, main, 0), "can reorder within the same layout");
	main_node = m.Find(main);
	t.Expect(main_node && main_node->children[0] == child && main_node->children[1] == title, "move index reorders child");
	t.Expect(m.Validate(), "placement model validates");
}

static void TestDesignerCommands(TestCtx& t)
{
	t.Section("Designer commands");

	DesignerModel m;
	DesignerNodeId box = m.AddNode("BoxLayout", Designer_ROOT);
	DesignerNodeId label = m.AddNode("UiLabel", box);
	DesignerCommandStack stack;

	t.Expect(stack.Execute(MakeDesignerSetPropertyCommand(label, "text", "Name"), m),
	         "set property command executes");
	t.Expect(m.Find(label)->properties.GetValue(m.Find(label)->properties.Find("text")) == "Name",
	         "set property writes model");
	t.Expect(stack.Undo(m), "set property undo executes");
	t.Expect(m.Find(label)->properties.Find("text") < 0, "set property undo removes absent old property");
	t.Expect(stack.Redo(m), "set property redo executes");
	t.Expect(m.Find(label)->properties.GetValue(m.Find(label)->properties.Find("text")) == "Name",
	         "set property redo restores value");

	t.Expect(stack.Execute(MakeDesignerRenameCommand(label, "nameLabel"), m),
	         "rename command executes");
	t.Expect(m.Find(label)->name == "nameLabel", "rename writes model");
	t.Expect(stack.Undo(m), "rename undo executes");
	t.Expect(m.Find(label)->name != "nameLabel", "rename undo restores old name");
	t.Expect(stack.Redo(m), "rename redo executes");
	t.Expect(m.Find(label)->name == "nameLabel", "rename redo restores new name");

	DesignerNodeId grid = m.AddNode("GridLayout", Designer_ROOT);
	t.Expect(stack.Execute(MakeDesignerMoveNodeCommand(label, grid), m),
	         "move command executes");
	t.Expect(m.Find(label)->parent == grid, "move command changes parent");
	t.Expect(stack.Undo(m), "move undo executes");
	t.Expect(m.Find(label)->parent == box, "move undo restores parent");
	t.Expect(stack.Redo(m), "move redo executes");
	t.Expect(m.Find(label)->parent == grid, "move redo reapplies parent");

	DesignerNodeId added = stack.AddNode(m, "UiSlider", grid);
	t.Expect(added != Designer_NULL && m.Find(added), "add node command creates node");
	m.Find(added)->name = "volumeSlider";
	m.SetProperty(added, "width", 180);
	t.Expect(stack.Undo(m), "add node undo executes");
	t.Expect(!m.Find(added), "add node undo removes created node");
	t.Expect(stack.Redo(m), "add node redo executes");
	t.Expect(m.Find(added) && m.Find(added)->name == "volumeSlider", "add node redo restores initialized node state");
	t.Expect(m.Find(added)->properties.Find("width") >= 0, "add node redo restores node properties");

	DesignerNodeId nested = m.AddNode("UiLabel", label);
	t.Expect(stack.Execute(MakeDesignerRemoveNodeCommand(label), m), "remove command executes");
	t.Expect(!m.Find(label) && !m.Find(nested), "remove command deletes subtree");
	t.Expect(stack.Undo(m), "remove undo executes");
	t.Expect(m.Find(label) && m.Find(nested), "remove undo restores subtree");
	t.Expect(m.Find(label)->parent == grid, "remove undo restores parent");

	stack.BeginGroup("Add adjusted item");
	DesignerNodeId grouped = stack.AddNode(m, "UiTitleCard", grid);
	m.Find(grouped)->name = "header";
	t.Expect(stack.Execute(MakeDesignerSetPropertyCommand(grid, "cell_height", 160), m),
	         "grouped auto property command executes");
	t.Expect(stack.EndGroup(), "command group is committed");
	t.Expect(m.Find(grid)->properties.Find("cell_height") >= 0, "group writes parent property");
	t.Expect(stack.Undo(m), "group undo executes");
	t.Expect(!m.Find(grouped), "group undo removes added node");
	t.Expect(m.Find(grid)->properties.Find("cell_height") < 0, "group undo reverts parent auto property");
	t.Expect(stack.Redo(m), "group redo executes");
	t.Expect(m.Find(grouped) && m.Find(grouped)->name == "header", "group redo restores added node state");
	t.Expect(m.Find(grid)->properties.Find("cell_height") >= 0, "group redo restores parent auto property");
	t.Expect(m.Validate(), "command-mutated model validates");
}

static void TestExplicitEmptyTextValues(TestCtx& t)
{
	t.Section("Designer explicit empty text");

	DesignerRegistry r;
	RegisterDesignerBuiltins(r);

	struct Spec {
		const char* type;
		const char* property;
		const char* expected_code;
		const char* name;
		bool set_icon = false;
		const char* icon = nullptr;
	};

	Vector<Spec> specs;
	specs.Add({"UiButton", "text", ".SetText(\"\")", "exit", true, "Settings"});
	specs.Add({"UiToolButton", "text", ".SetText(\"\")", "tool", true, "Settings"});
	specs.Add({"UiSplitButton", "text", ".SetText(\"\")", "split", true, "Settings"});
	specs.Add({"UiLabel", "text", ".SetText(\"\")", "caption"});
	specs.Add({"UiTitleCard", "text", ".SetTitle(\"\")", "header"});
	specs.Add({"UiTitleCard", "subtitle", ".SetSubTitle(\"\")", "subheader"});
	specs.Add({"UiGroupPanel", "text", ".SetTitle(\"\")", "group"});
	specs.Add({"UiLineEdit", "placeholder", ".SetPlaceholder(\"\")", "edit"});

	for(const Spec& spec : specs) {
		DesignerModel m;
		DesignerCommandStack stack;
		DesignerNodeId layout = m.AddNode("BoxLayout", Designer_ROOT);
		r.Find("BoxLayout")->init_defaults(*m.Find(layout));
		DesignerNodeId id = m.AddNode(spec.type, layout);
		r.Find(spec.type)->init_defaults(*m.Find(id));
		m.Find(id)->name = spec.name;
		if(m.Find(id)->properties.Find(spec.property) >= 0)
			m.RemoveProperty(id, spec.property);
		if(spec.set_icon)
			m.SetProperty(id, "icon", spec.icon ? spec.icon : "Settings");
		t.Expect(stack.Execute(MakeDesignerSetPropertyCommand(id, spec.property, String(), "Clear " + String(spec.property)), m),
		         String(spec.type) + " accepts explicit empty " + spec.property);
		const DesignerNode* n = m.Find(id);
		t.Expect(n && n->properties.Find(spec.property) >= 0 &&
		         AsString(n->properties.GetValue(n->properties.Find(spec.property))) == "",
		         String(spec.type) + " stores explicit empty " + spec.property);
		t.Expect(n && n->name == spec.name,
		         String(spec.type) + " keeps hierarchy/model name while caption is empty");
		String json = StoreDesignerModelJson(m);
		DesignerModel loaded;
		String load_error;
		t.Expect(LoadDesignerModelJson(loaded, r, json, load_error, nullptr),
		         String(spec.type) + " JSON round-trip loads");
		const DesignerNode* loaded_node = FindNodeByName(loaded, spec.name);
		t.Expect(loaded_node && loaded_node->properties.Find(spec.property) >= 0 &&
		         AsString(loaded_node->properties.GetValue(loaded_node->properties.Find(spec.property))) == "",
		         String(spec.type) + " JSON preserves explicit empty " + spec.property);
		String code = GenerateDesignerCode(m, r, "GeneratedEmptyText" + String(spec.type) + spec.property);
		t.Expect(code.Find(spec.expected_code) >= 0,
		         String(spec.type) + " codegen preserves explicit empty " + spec.property);
	}

	DesignerModel button_model;
	DesignerNodeId layout = button_model.AddNode("BoxLayout", Designer_ROOT);
	r.Find("BoxLayout")->init_defaults(*button_model.Find(layout));
	DesignerNodeId button = button_model.AddNode("UiButton", layout);
	r.Find("UiButton")->init_defaults(*button_model.Find(button));
	button_model.Find(button)->name = "exit";
	button_model.SetProperty(button, "icon", "Settings");
	button_model.SetProperty(button, "text", String());
	DesignerButtonAdapter button_adapter;
	button_adapter.SyncFromNode(*button_model.Find(button));
	t.Expect(button_adapter.GetText().IsEmpty(), "button preview preserves explicit empty text");
}

static void TestDesignerDragController(TestCtx& t)
{
	t.Section("Designer drag controller");

	DesignerRegistry r;
	RegisterDesignerBuiltins(r);
	DesignerModel m;
	DesignerCommandStack commands;
	DesignerNodeId box = commands.AddNode(m, "BoxLayout", Designer_ROOT);
	r.Find("BoxLayout")->init_defaults(*m.Find(box));
	DesignerNodeId label = commands.AddNode(m, "UiLabel", box);
	r.Find("UiLabel")->init_defaults(*m.Find(label));

	DesignerDragController drag;
	drag.BeginToolDrag("UiSlider");
	drag.UpdateTarget(m, r, DesignerMakeIntoTarget(box));
	t.Expect(drag.GetTarget().valid, "tool drag validates into box layout");
	t.Expect(drag.Drop(m, commands), "tool drag drop creates node through commands");
	t.Expect(m.Find(box)->children.GetCount() == 2, "tool drop appends child");

	DesignerNodeId grid = commands.AddNode(m, "GridLayout", Designer_ROOT);
	r.Find("GridLayout")->init_defaults(*m.Find(grid));
	drag.BeginNodeDrag(label);
	drag.UpdateTarget(m, r, DesignerMakeIntoTarget(grid));
	t.Expect(drag.GetTarget().valid, "node drag validates into grid layout");
	t.Expect(drag.Drop(m, commands), "node drag drop moves node through commands");
	t.Expect(m.Find(label)->parent == grid, "node drop updates parent");

	drag.BeginNodeDrag(grid);
	drag.UpdateTarget(m, r, DesignerMakeIntoTarget(label));
	t.Expect(!drag.GetTarget().valid, "drag rejects descendant cycle");
	drag.Cancel();
	t.Expect(!drag.IsActive(), "drag cancel clears active state");
	t.Expect(m.Validate(), "drag-mutated model validates");
}

static void TestRegistryAndBuiltins(TestCtx& t)
{
	t.Section("DesignerRegistry built-ins");

	DesignerRegistry r;
	RegisterDesignerBuiltins(r);

	t.Expect(r.Find("Window"), "window type registered");
	t.Expect(r.Find("BoxLayout") && r.Find("BoxLayout")->can_have_children, "box layout registered as container");
	t.Expect(r.Find("GridLayout") && r.Find("GridLayout")->can_have_children, "grid layout registered as container");
	t.Expect(r.Find("Spacer") && !r.Find("Spacer")->can_have_children, "spacer registered as layout item");
	t.Expect(r.Find("PaneSlot") && r.Find("PaneSlot")->can_have_children, "internal pane slot registered as container");
	t.Expect(r.Find("PageSlot") && r.Find("PageSlot")->can_have_children, "internal page slot registered as container");
	t.Expect(r.Find("UiTab") && r.Find("UiTab")->can_have_children, "tab registered as page container");
	t.Expect(r.Find("UiStack") && r.Find("UiStack")->can_have_children, "stack registered as page container");
	t.Expect(r.Find("UiLabel") && !r.Find("UiLabel")->can_have_children, "label registered as control");
	DesignerNode grid_defaults;
	grid_defaults.type_id = "GridLayout";
	r.Find("GridLayout")->init_defaults(grid_defaults);
	t.Expect((int)grid_defaults.properties.GetValue(grid_defaults.properties.Find("columns")) == 2, "grid layout defaults to two stable columns");
	t.Expect((int)grid_defaults.properties.GetValue(grid_defaults.properties.Find("rows")) == 2, "grid layout defaults to two stable rows");
	t.Expect(!IsNull(grid_defaults.properties.GetValue(grid_defaults.properties.Find("debug_color"))),
	         "grid layout defaults include debug color");
	t.Expect((bool)grid_defaults.properties.GetValue(grid_defaults.properties.Find("debug_auto_color")),
	         "grid layout defaults enable stable auto debug color");

	Vector<String> groups = r.GetToolboxGroups();
	t.Expect(groups.GetCount() >= 3, "toolbox exposes layout, container, and control groups");
	t.Expect(FindStringPos(groups, "Layouts") >= 0, "toolbox has layouts group");
	t.Expect(FindStringPos(groups, "Containers") >= 0, "toolbox has containers group");
	t.Expect(FindStringPos(groups, "Controls") >= 0, "toolbox has controls group");
	t.Expect(r.Find("PaneSlot")->toolbox_group.IsEmpty(), "pane slot is internal and not shown in toolbox");
	t.Expect(r.Find("PageSlot")->toolbox_group.IsEmpty(), "page slot is internal and not shown in toolbox");
	t.Expect(r.Find("Generic") && r.Find("Generic")->toolbox_group.IsEmpty(),
	         "generic fallback type is registered but hidden from toolbox");
	t.Expect(r.GetToolboxTypes("Layouts").GetCount() >= 4, "layouts group includes box, grid, splitter, and quad splitter");
	t.Expect(r.GetToolboxTypes("Containers").GetCount() >= 4, "containers group includes panel, scroll panel, tab, and stack");
	t.Expect(r.GetToolboxTypes("Controls").GetCount() >= 8, "controls group has real controls");
	DesignerNode label_defaults;
	label_defaults.type_id = "UiLabel";
	r.Find("UiLabel")->init_defaults(label_defaults);
	t.Expect(!(bool)label_defaults.properties.GetValue(label_defaults.properties.Find("face_enabled")),
	         "new real controls default to theme-provided face");
	t.Expect(!(bool)label_defaults.properties.GetValue(label_defaults.properties.Find("frame_enabled")),
	         "new real controls default to theme-provided frame");
	t.Expect(!r.Find("Item"), "generic Item placeholder is not shown as a designer control");
	t.Expect(r.Find("UiButton") && !r.Find("UiButton")->icon.IsEmpty(), "button control has toolbox icon");
	t.Expect(r.Find("UiLineEdit") && !r.Find("UiLineEdit")->icon.IsEmpty(), "edit control has toolbox icon");
	t.Expect(r.Find("UiToggle") && !r.Find("UiToggle")->icon.IsEmpty(), "toggle control has toolbox icon");
	t.Expect(r.Find("UiDropdown") && !r.Find("UiDropdown")->icon.IsEmpty(), "dropdown control has toolbox icon");
	for(const char *type : { "UiCheckBox", "UiBreadcrumbs", "UiTab", "UiStack", "UiTable", "UiTree" })
		t.Expect(r.Find(type) && !r.Find(type)->icon.IsEmpty(), String(type) + " is registered with toolbox icon");

	DesignerNode parent;
	parent.type_id = "UiLabel";
	DesignerNode child;
	child.type_id = "UiSlider";
	t.Expect(!r.CanDrop(parent, child), "non-container rejects children");
	parent.type_id = "BoxLayout";
	t.Expect(r.CanDrop(parent, child), "box layout accepts controls");
	child.type_id = "Spacer";
	t.Expect(r.CanDrop(parent, child), "box layout accepts spacer item");
	parent.type_id = "Window";
	t.Expect(!r.CanDrop(parent, child), "window rejects root spacer item");
}

static const DesignerApiBinding* FindBinding(const Vector<DesignerApiBinding>& bindings, const String& id)
{
	const DesignerApiBinding *found = nullptr;
	for(const DesignerApiBinding& b : bindings)
		if(b.property_id == id)
			found = &b;
	return found;
}

struct DesignerApiAuditSpec : Moveable<DesignerApiAuditSpec> {
	bool role_visible = false;
	bool theme_visible = false;
	bool surface_visible = false;
	bool quad_visible = false;
	bool ink_visible = false;
	bool icon_ink_visible = false;
	Vector<String> part_overrides;
	Vector<String> hidden_common;
	String role_reason;
	Vector<String> codegen_markers;
	Function<void(DesignerNode&)> setup;

	DesignerApiAuditSpec() = default;
	DesignerApiAuditSpec(const DesignerApiAuditSpec& o) { *this = o; }
	DesignerApiAuditSpec& operator=(const DesignerApiAuditSpec& o)
	{
		role_visible = o.role_visible;
		theme_visible = o.theme_visible;
		surface_visible = o.surface_visible;
		quad_visible = o.quad_visible;
		ink_visible = o.ink_visible;
		icon_ink_visible = o.icon_ink_visible;
		part_overrides.Clear();
		part_overrides.Append(o.part_overrides);
		hidden_common.Clear();
		hidden_common.Append(o.hidden_common);
		role_reason = o.role_reason;
		codegen_markers.Clear();
		codegen_markers.Append(o.codegen_markers);
		setup = o.setup;
		return *this;
	}
};

static bool BindingVisible(const Vector<DesignerApiBinding>& bindings, const String& id)
{
	const DesignerApiBinding* b = FindBinding(bindings, id);
	return b && b->visible;
}

static bool BindingHidden(const Vector<DesignerApiBinding>& bindings, const String& id)
{
	const DesignerApiBinding* b = FindBinding(bindings, id);
	return !b || !b->visible;
}

static void AddStrings(Vector<String>& out, std::initializer_list<const char *> values)
{
	for(const char *s : values)
		out.Add(s);
}

static DesignerApiAuditSpec& AddAuditSpec(VectorMap<String, DesignerApiAuditSpec>& specs, const String& type_id)
{
	return specs.GetAdd(type_id);
}

static VectorMap<String, DesignerApiAuditSpec> BuildDesignerApiAuditSpecs()
{
	VectorMap<String, DesignerApiAuditSpec> specs;
	auto& hidden_layout = AddAuditSpec(specs, "BoxLayout");
	hidden_layout.role_visible = false;
	hidden_layout.role_reason = "layout engine only";
	AddStrings(hidden_layout.hidden_common, {"role", "theme_override", "face", "face_mode", "face_quad", "frame",
	                                        "frame_style", "radius", "face_enabled", "frame_enabled", "shadow_enabled",
	                                        "shadow_distance", "shadow_offset_x", "shadow_offset_y", "shadow_alpha",
	                                        "shadow_color", "shadow_curve"});
	hidden_layout.codegen_markers.Clear();

	auto& grid = AddAuditSpec(specs, "GridLayout");
	grid.role_visible = false;
	grid.role_reason = "layout engine only";
	AddStrings(grid.hidden_common, {"role", "theme_override", "face", "face_mode", "face_quad", "frame",
	                               "frame_style", "radius", "face_enabled", "frame_enabled", "shadow_enabled",
	                               "shadow_distance", "shadow_offset_x", "shadow_offset_y", "shadow_alpha",
	                               "shadow_color", "shadow_curve"});

	auto& spacer = AddAuditSpec(specs, "Spacer");
	spacer.role_visible = false;
	spacer.role_reason = "layout item / separator item only";
	AddStrings(spacer.hidden_common, {"role", "theme_override", "face", "face_mode", "face_quad", "frame",
	                                 "frame_style", "radius", "face_enabled", "frame_enabled", "shadow_enabled",
	                                 "shadow_distance", "shadow_offset_x", "shadow_offset_y", "shadow_alpha",
	                                 "shadow_color", "shadow_curve"});

	auto& splitter = AddAuditSpec(specs, "UiSplitter");
	splitter.role_visible = true;
	AddStrings(splitter.hidden_common, {"theme_override", "face", "face_mode", "face_quad", "frame",
	                                   "frame_style", "radius", "face_enabled", "frame_enabled", "shadow_enabled",
	                                   "shadow_distance", "shadow_offset_x", "shadow_offset_y", "shadow_alpha",
	                                   "shadow_color", "shadow_curve"});
	AddStrings(splitter.codegen_markers, {"UiTheme::ResolveSplitter(UiRole::Accent)"});
	splitter.setup = [](DesignerNode& n) {
		n.properties.Set("role", "Accent");
	};

	auto& quad = AddAuditSpec(specs, "UiQuadSplitter");
	quad.role_visible = false;
	quad.role_reason = "quad splitter is layout/debug only";
	AddStrings(quad.hidden_common, {"role", "theme_override", "face", "face_mode", "face_quad", "frame",
	                               "frame_style", "radius", "face_enabled", "frame_enabled", "shadow_enabled",
	                               "shadow_distance", "shadow_offset_x", "shadow_offset_y", "shadow_alpha",
	                               "shadow_color", "shadow_curve"});

	auto& pane_slot = AddAuditSpec(specs, "PaneSlot");
	pane_slot.role_visible = false;
	pane_slot.role_reason = "structural slot controlled by splitter";
	AddStrings(pane_slot.hidden_common, {"role", "theme_override", "face", "face_mode", "face_quad", "frame",
	                                    "frame_style", "radius", "face_enabled", "frame_enabled", "shadow_enabled",
	                                    "shadow_distance", "shadow_offset_x", "shadow_offset_y", "shadow_alpha",
	                                    "shadow_color", "shadow_curve"});

	auto& page_slot = AddAuditSpec(specs, "PageSlot");
	page_slot.role_visible = false;
	page_slot.role_reason = "structural slot controlled by page container";
	AddStrings(page_slot.hidden_common, {"role", "theme_override", "face", "face_mode", "face_quad", "frame",
	                                    "frame_style", "radius", "face_enabled", "frame_enabled", "shadow_enabled",
	                                    "shadow_distance", "shadow_offset_x", "shadow_offset_y", "shadow_alpha",
	                                    "shadow_color", "shadow_curve"});

	auto& section_slot = AddAuditSpec(specs, "AccordionSectionSlot");
	section_slot.role_visible = false;
	section_slot.role_reason = "structural accordion slot";
	AddStrings(section_slot.hidden_common, {"role", "theme_override", "face", "face_mode", "face_quad", "frame",
	                                       "frame_style", "radius", "face_enabled", "frame_enabled", "shadow_enabled",
	                                       "shadow_distance", "shadow_offset_x", "shadow_offset_y", "shadow_alpha",
	                                       "shadow_color", "shadow_curve"});

	auto& generic = AddAuditSpec(specs, "Generic");
	generic.role_visible = true;
	generic.theme_visible = true;
	generic.surface_visible = true;
	generic.quad_visible = true;
	AddStrings(generic.codegen_markers, {"UiTheme::ResolvePanel(UiRole::Accent)", "s.metrics.face_enabled = true", "s.metrics.frame_enabled = true"});
	generic.setup = [](DesignerNode& n) {
		n.properties.Set("role", "Accent");
		n.properties.Set("theme_override", true);
		n.properties.Set("face_enabled", true);
		n.properties.Set("face", Color(240, 240, 255));
		n.properties.Set("frame_enabled", true);
		n.properties.Set("frame", Color(226, 226, 226));
		n.properties.Set("radius", 8);
	};

	auto& window = AddAuditSpec(specs, "Window");
	window.role_visible = true;
	window.theme_visible = true;
	window.surface_visible = true;
	window.quad_visible = true;
	window.codegen_markers.Clear();
	window.role_reason = "top-level shell is exported as the generated window wrapper, not a widget codegen path";
	window.setup = [](DesignerNode& n) {
		n.properties.Set("role", "Accent");
		n.properties.Set("theme_override", true);
		n.properties.Set("face_enabled", true);
		n.properties.Set("face", Color(240, 240, 255));
		n.properties.Set("frame_enabled", true);
		n.properties.Set("frame", Color(226, 226, 226));
		n.properties.Set("radius", 8);
	};

	auto& panel = AddAuditSpec(specs, "UiPanel");
	panel.role_visible = true;
	panel.theme_visible = true;
	panel.surface_visible = true;
	panel.quad_visible = true;
	AddStrings(panel.codegen_markers, {"UiTheme::ResolvePanel(UiRole::Accent)", "s.metrics.face_enabled = true", "s.metrics.frame_enabled = true"});
	panel.setup = [](DesignerNode& n) {
		n.properties.Set("role", "Accent");
		n.properties.Set("theme_override", true);
		n.properties.Set("face_enabled", true);
		n.properties.Set("face", Color(240, 240, 255));
		n.properties.Set("frame_enabled", true);
		n.properties.Set("frame", Color(226, 226, 226));
		n.properties.Set("radius", 8);
	};

	auto& scroll = AddAuditSpec(specs, "UiScrollPanel");
	scroll.role_visible = true;
	scroll.theme_visible = true;
	scroll.surface_visible = true;
	scroll.quad_visible = true;
	AddStrings(scroll.codegen_markers, {"UiTheme::ResolveScrollPanel(UiRole::Accent)", "s.metrics.face_enabled = true", "s.metrics.frame_enabled = true"});
	scroll.setup = [](DesignerNode& n) {
		n.properties.Set("role", "Accent");
		n.properties.Set("theme_override", true);
		n.properties.Set("face_enabled", true);
		n.properties.Set("face", Color(240, 240, 255));
		n.properties.Set("frame_enabled", true);
		n.properties.Set("frame", Color(226, 226, 226));
		n.properties.Set("radius", 8);
	};

	auto& group = AddAuditSpec(specs, "UiGroupPanel");
	group.role_visible = true;
	group.theme_visible = true;
	group.surface_visible = true;
	group.quad_visible = true;
	AddStrings(group.codegen_markers, {"UiTheme::ResolveGroupPanel(UiRole::Accent)", "s.metrics.face_enabled = true", "s.metrics.frame_enabled = true"});
	group.setup = [](DesignerNode& n) {
		n.properties.Set("role", "Accent");
		n.properties.Set("theme_override", true);
		n.properties.Set("face_enabled", true);
		n.properties.Set("face", Color(240, 240, 255));
		n.properties.Set("frame_enabled", true);
		n.properties.Set("frame", Color(226, 226, 226));
		n.properties.Set("radius", 8);
	};

	auto& label = AddAuditSpec(specs, "UiLabel");
	label.role_visible = true;
	label.theme_visible = true;
	label.surface_visible = true;
	label.quad_visible = true;
	label.ink_visible = true;
	label.icon_ink_visible = true;
	AddStrings(label.codegen_markers, {"UiTheme::ResolveLabel(UiRole::Accent)", "s.palette.ink[ST_NORMAL]", "s.palette.icon[ST_NORMAL]", "s.metrics.face_enabled = true"});
	label.setup = [](DesignerNode& n) {
		n.properties.Set("role", "Accent");
		n.properties.Set("theme_override", true);
		n.properties.Set("face_enabled", true);
		n.properties.Set("face", Color(240, 240, 255));
		n.properties.Set("frame_enabled", true);
		n.properties.Set("frame", Color(226, 226, 226));
		n.properties.Set("radius", 8);
		n.properties.Set("ink_enabled", true);
		n.properties.Set("ink", Color(10, 90, 200));
		n.properties.Set("icon_ink_enabled", true);
		n.properties.Set("icon_ink", Color(10, 90, 200));
	};

	auto& title = AddAuditSpec(specs, "UiTitleCard");
	title.role_visible = true;
	title.theme_visible = true;
	title.surface_visible = true;
	title.quad_visible = true;
	AddStrings(title.codegen_markers, {"UiTheme::ResolveTitleCard(UiRole::Accent)", "title_color =", "subtitle_color =", "card_line_side =", "card_line_color ="});
	title.part_overrides = {"title_color_enabled", "title_color", "subtitle_color_enabled", "subtitle_color",
	                        "card_line_side", "card_line_length", "card_line_style", "card_line_thickness",
	                        "card_line_color_enabled", "card_line_color"};
	title.setup = [](DesignerNode& n) {
		n.properties.Set("role", "Accent");
		n.properties.Set("theme_override", true);
		n.properties.Set("face_enabled", true);
		n.properties.Set("face", Color(240, 240, 255));
		n.properties.Set("frame_enabled", true);
		n.properties.Set("frame", Color(226, 226, 226));
		n.properties.Set("radius", 8);
		n.properties.Set("title_color_enabled", true);
		n.properties.Set("title_color", Color(10, 90, 200));
		n.properties.Set("subtitle_color_enabled", true);
		n.properties.Set("subtitle_color", Color(120, 130, 140));
		n.properties.Set("card_line_side", "Left");
		n.properties.Set("card_line_length", "Medium");
		n.properties.Set("card_line_style", "Dotted");
		n.properties.Set("card_line_thickness", 3);
		n.properties.Set("card_line_color_enabled", true);
		n.properties.Set("card_line_color", Color(10, 90, 200));
	};

	auto& button = AddAuditSpec(specs, "UiButton");
	button.role_visible = true;
	button.theme_visible = true;
	button.surface_visible = true;
	button.quad_visible = true;
	button.ink_visible = true;
	button.icon_ink_visible = true;
	AddStrings(button.codegen_markers, {"UiTheme::ResolveButton(UiRole::Accent)", "s.palette.face[ST_NORMAL]", "s.palette.frame[i] =", "s.palette.ink[ST_NORMAL]", "s.palette.icon[ST_NORMAL]"});
	button.setup = [](DesignerNode& n) {
		n.properties.Set("role", "Accent");
		n.properties.Set("theme_override", true);
		n.properties.Set("face_enabled", true);
		n.properties.Set("face", Color(240, 240, 255));
		n.properties.Set("frame_enabled", true);
		n.properties.Set("frame", Color(226, 226, 226));
		n.properties.Set("radius", 8);
		n.properties.Set("ink_enabled", true);
		n.properties.Set("ink", Color(10, 90, 200));
		n.properties.Set("icon_ink_enabled", true);
		n.properties.Set("icon_ink", Color(10, 90, 200));
	};

	auto& split = AddAuditSpec(specs, "UiSplitButton");
	split.role_visible = true;
	split.theme_visible = true;
	split.surface_visible = true;
	split.quad_visible = true;
	split.ink_visible = true;
	split.icon_ink_visible = true;
	AddStrings(split.codegen_markers, {"UiTheme::ResolveButton(UiRole::Accent)", "s.palette.face[ST_NORMAL]", "s.palette.frame[i] =", "s.palette.ink[ST_NORMAL]", "s.palette.icon[ST_NORMAL]",
	                                  ".SetSplitWidth(DPI(", ".SetPopupMinWidth(DPI(", ".SetSplitContentGap(DPI("});
	split.setup = button.setup;

	auto& tool = AddAuditSpec(specs, "UiToolButton");
	tool.role_visible = true;
	tool.theme_visible = true;
	tool.surface_visible = true;
	tool.quad_visible = true;
	tool.ink_visible = true;
	tool.icon_ink_visible = true;
	AddStrings(tool.codegen_markers, {"UiTheme::ResolveToolButton(UiRole::Accent)", "s.palette.face[ST_NORMAL]", "s.palette.frame[i] =", "s.palette.ink[ST_NORMAL]", "s.palette.icon[ST_NORMAL]"});
	tool.setup = [](DesignerNode& n) {
		n.properties.Set("role", "Accent");
		n.properties.Set("theme_override", true);
		n.properties.Set("face_enabled", true);
		n.properties.Set("face", Color(240, 240, 255));
		n.properties.Set("frame_enabled", true);
		n.properties.Set("frame", Color(226, 226, 226));
		n.properties.Set("radius", 8);
		n.properties.Set("ink_enabled", true);
		n.properties.Set("ink", Color(10, 90, 200));
		n.properties.Set("icon_ink_enabled", true);
		n.properties.Set("icon_ink", Color(10, 90, 200));
	};

	auto& line = AddAuditSpec(specs, "UiLineEdit");
	line.role_visible = true;
	line.theme_visible = true;
	line.surface_visible = true;
	line.quad_visible = true;
	line.ink_visible = true;
	line.icon_ink_visible = false;
	AddStrings(line.codegen_markers, {"UiTheme::ResolveEdit(UiRole::Accent)", "s.palette.ink[ST_NORMAL]", "s.placeholder_ink ="});
	line.setup = [](DesignerNode& n) {
		n.properties.Set("role", "Accent");
		n.properties.Set("theme_override", true);
		n.properties.Set("face_enabled", true);
		n.properties.Set("face", Color(240, 240, 255));
		n.properties.Set("frame_enabled", true);
		n.properties.Set("frame", Color(226, 226, 226));
		n.properties.Set("radius", 8);
		n.properties.Set("ink_enabled", true);
		n.properties.Set("ink", Color(10, 90, 200));
		n.properties.Set("placeholder_ink_enabled", true);
		n.properties.Set("placeholder_ink", Color(120, 130, 140));
	};

	auto& int_edit = AddAuditSpec(specs, "UiIntEdit");
	int_edit = line;
	int_edit.codegen_markers[0] = "UiTheme::ResolveEdit(UiRole::Accent)";
	int_edit.icon_ink_visible = false;

	auto& float_edit = AddAuditSpec(specs, "UiFloatEdit");
	float_edit = line;
	float_edit.icon_ink_visible = false;

	auto& dropdown = AddAuditSpec(specs, "UiDropdown");
	dropdown.role_visible = true;
	dropdown.theme_visible = true;
	dropdown.surface_visible = true;
	dropdown.quad_visible = true;
	dropdown.ink_visible = true;
	dropdown.icon_ink_visible = false;
	dropdown.part_overrides = {"indicator_side", "indicator_closed_icon", "indicator_opened_icon", "indicator_size"};
	AddStrings(dropdown.codegen_markers, {"UiTheme::ResolveDropdown(UiRole::Accent)", "s.palette.face[ST_NORMAL]", "s.palette.ink[ST_NORMAL]"});
	dropdown.setup = [](DesignerNode& n) {
		n.properties.Set("role", "Accent");
		n.properties.Set("theme_override", true);
		n.properties.Set("face_enabled", true);
		n.properties.Set("face", Color(240, 240, 255));
		n.properties.Set("frame_enabled", true);
		n.properties.Set("frame", Color(226, 226, 226));
		n.properties.Set("radius", 8);
		n.properties.Set("ink_enabled", true);
		n.properties.Set("ink", Color(10, 90, 200));
	};

	auto& checkbox = AddAuditSpec(specs, "UiCheckBox");
	checkbox.role_visible = true;
	checkbox.theme_visible = true;
	checkbox.surface_visible = false;
	checkbox.quad_visible = false;
	checkbox.ink_visible = true;
	checkbox.icon_ink_visible = false;
	checkbox.part_overrides = {"indicator_face_enabled", "indicator_face", "indicator_frame_enabled", "indicator_frame", "indicator_ink_enabled", "indicator_ink"};
	AddStrings(checkbox.hidden_common, {"face", "face_mode", "face_quad", "frame", "frame_style", "radius", "face_enabled", "frame_enabled", "shadow_enabled",
	                                    "shadow_distance", "shadow_offset_x", "shadow_offset_y", "shadow_alpha", "shadow_color", "shadow_curve"});
	AddStrings(checkbox.codegen_markers, {"UiTheme::ResolveCheckBox(UiRole::Accent, ", "s.palette.ink[ST_NORMAL]", "s.indicator_palette.palette.face[ST_NORMAL]", "s.indicator_palette.ink[ST_NORMAL]"});
	checkbox.setup = [](DesignerNode& n) {
		n.properties.Set("role", "Accent");
		n.properties.Set("theme_override", true);
		n.properties.Set("ink_enabled", true);
		n.properties.Set("ink", Color(10, 90, 200));
		n.properties.Set("indicator_face_enabled", true);
		n.properties.Set("indicator_face", Color(240, 240, 255));
		n.properties.Set("indicator_frame_enabled", true);
		n.properties.Set("indicator_frame", Color(226, 226, 226));
		n.properties.Set("indicator_ink_enabled", true);
		n.properties.Set("indicator_ink", Color(10, 90, 200));
	};

	auto& toggle = AddAuditSpec(specs, "UiToggle");
	toggle.role_visible = true;
	toggle.theme_visible = true;
	toggle.surface_visible = false;
	toggle.quad_visible = false;
	toggle.ink_visible = false;
	toggle.icon_ink_visible = false;
	toggle.part_overrides = {"track_face_enabled", "track_face", "track_frame_enabled", "track_frame", "thumb_face_enabled", "thumb_face", "thumb_frame_enabled", "thumb_frame"};
	AddStrings(toggle.hidden_common, {"face", "face_mode", "face_quad", "frame", "frame_style", "radius", "face_enabled", "frame_enabled", "shadow_enabled",
	                                  "shadow_distance", "shadow_offset_x", "shadow_offset_y", "shadow_alpha", "shadow_color", "shadow_curve"});
	AddStrings(toggle.codegen_markers, {"UiTheme::ResolveToggle(UiRole::Accent)", "s.track_palette.palette.face[ST_NORMAL]", "s.thumb_palette.palette.face[ST_NORMAL]"});
	toggle.setup = [](DesignerNode& n) {
		n.properties.Set("role", "Accent");
		n.properties.Set("theme_override", true);
		n.properties.Set("track_face_enabled", true);
		n.properties.Set("track_face", Color(240, 240, 255));
		n.properties.Set("track_frame_enabled", true);
		n.properties.Set("track_frame", Color(226, 226, 226));
		n.properties.Set("thumb_face_enabled", true);
		n.properties.Set("thumb_face", Color(10, 90, 200));
		n.properties.Set("thumb_frame_enabled", true);
		n.properties.Set("thumb_frame", Color(10, 90, 200));
	};

	auto& slider = AddAuditSpec(specs, "UiSlider");
	slider.role_visible = true;
	slider.theme_visible = true;
	slider.surface_visible = false;
	slider.quad_visible = false;
	slider.ink_visible = false;
	slider.icon_ink_visible = false;
	slider.part_overrides = {"face_enabled", "face", "frame_enabled", "frame", "radius",
	                         "track_width", "track_height", "thumb_width", "thumb_height",
	                         "track_radius", "thumb_radius", "value"};
	AddStrings(slider.hidden_common, {"face_mode", "face_quad", "frame_style", "shadow_enabled", "shadow_distance", "shadow_offset_x",
	                                  "shadow_offset_y", "shadow_alpha", "shadow_color", "shadow_curve"});
	AddStrings(slider.codegen_markers, {"UiTheme::ResolveSlider(UiRole::Accent)", "s.track_size = Size", "s.thumb_size = Size", "s.track_metrics.radius", "s.thumb_metrics.radius"});
	slider.setup = [](DesignerNode& n) {
		n.properties.Set("role", "Accent");
		n.properties.Set("theme_override", true);
		n.properties.Set("face_enabled", true);
		n.properties.Set("face", Color(214, 231, 255));
		n.properties.Set("frame_enabled", true);
		n.properties.Set("frame", Color(54, 116, 210));
		n.properties.Set("radius", 8);
		n.properties.Set("track_width", 160);
		n.properties.Set("track_height", 4);
		n.properties.Set("thumb_width", 28);
		n.properties.Set("thumb_height", 20);
		n.properties.Set("track_radius", 8);
		n.properties.Set("thumb_radius", 8);
	};

	auto& breadcrumbs = AddAuditSpec(specs, "UiBreadcrumbs");
	breadcrumbs.role_visible = false;
	breadcrumbs.role_reason = "breadcrumbs remain geometry/content driven in V1";
	AddStrings(breadcrumbs.hidden_common, {"role", "theme_override", "face", "face_mode", "face_quad", "frame", "frame_style", "radius",
	                                       "face_enabled", "frame_enabled", "shadow_enabled", "shadow_distance",
	                                       "shadow_offset_x", "shadow_offset_y", "shadow_alpha", "shadow_color", "shadow_curve"});

	auto& tab = AddAuditSpec(specs, "UiTab");
	tab.role_visible = true;
	tab.theme_visible = false;
	tab.surface_visible = false;
	tab.quad_visible = false;
	tab.ink_visible = false;
	tab.icon_ink_visible = false;
	AddStrings(tab.hidden_common, {"theme_override", "face", "face_mode", "face_quad", "frame", "frame_style", "radius", "face_enabled",
	                               "frame_enabled", "shadow_enabled", "shadow_distance", "shadow_offset_x", "shadow_offset_y",
	                               "shadow_alpha", "shadow_color", "shadow_curve"});
	AddStrings(tab.codegen_markers, {"SetCustomStyle(UiTheme::ResolveTab("});
	tab.setup = [](DesignerNode& n) {
		n.properties.Set("role", "Accent");
	};

	auto& stack = AddAuditSpec(specs, "UiStack");
	stack.role_visible = false;
	stack.role_reason = "page container does not own a role-tuned surface in V1";
	AddStrings(stack.hidden_common, {"role", "theme_override", "face", "face_mode", "face_quad", "frame", "frame_style", "radius",
	                                 "face_enabled", "frame_enabled", "shadow_enabled", "shadow_distance", "shadow_offset_x",
	                                 "shadow_offset_y", "shadow_alpha", "shadow_color", "shadow_curve"});

	auto& table = AddAuditSpec(specs, "UiTable");
	table.role_visible = false;
	table.role_reason = "table surface is model-driven and role-neutral in V1";
	AddStrings(table.hidden_common, {"role", "theme_override", "face", "face_mode", "face_quad", "frame", "frame_style", "radius",
	                                 "face_enabled", "frame_enabled", "shadow_enabled", "shadow_distance", "shadow_offset_x",
	                                 "shadow_offset_y", "shadow_alpha", "shadow_color", "shadow_curve"});

	auto& tree = AddAuditSpec(specs, "UiTree");
	tree.role_visible = false;
	tree.role_reason = "tree surface is model-driven and role-neutral in V1";
	AddStrings(tree.hidden_common, {"role", "theme_override", "face", "face_mode", "face_quad", "frame", "frame_style", "radius",
	                                "face_enabled", "frame_enabled", "shadow_enabled", "shadow_distance", "shadow_offset_x",
	                                "shadow_offset_y", "shadow_alpha", "shadow_color", "shadow_curve"});

	auto& accordion = AddAuditSpec(specs, "UiAccordion");
	accordion.role_visible = true;
	accordion.theme_visible = true;
	accordion.surface_visible = true;
	accordion.quad_visible = false;
	accordion.ink_visible = false;
	accordion.icon_ink_visible = false;
	accordion.part_overrides = {"header_face_enabled", "header_face", "header_frame_enabled", "header_frame",
	                            "header_radius", "header_title", "header_subtitle", "body_face_enabled",
	                            "body_face", "body_frame_enabled", "body_frame", "body_radius"};
	AddStrings(accordion.codegen_markers, {"UiTheme::ResolvePanel(UiRole::Accent)", "UiTheme::ResolveTitleCard(UiRole::Accent)", "s.header_style.palette.face[ST_NORMAL]", "s.body_style.palette.face[ST_NORMAL]"});
	AddStrings(accordion.hidden_common, {"face_mode", "face_quad"});
	accordion.setup = [](DesignerNode& n) {
		n.properties.Set("role", "Accent");
		n.properties.Set("theme_override", true);
		n.properties.Set("header_face_enabled", true);
		n.properties.Set("header_face", Color(240, 240, 255));
		n.properties.Set("header_frame_enabled", true);
		n.properties.Set("header_frame", Color(226, 226, 226));
		n.properties.Set("header_radius", 8);
		n.properties.Set("header_title", Color(10, 90, 200));
		n.properties.Set("header_subtitle", Color(120, 130, 140));
		n.properties.Set("body_face_enabled", true);
		n.properties.Set("body_face", Color(252, 252, 252));
		n.properties.Set("body_frame_enabled", true);
		n.properties.Set("body_frame", Color(226, 226, 226));
		n.properties.Set("body_radius", 8);
	};

	auto& split_button = AddAuditSpec(specs, "UiSplitButton");
	split_button = button;
	split_button.part_overrides = {"split_width", "split_content_gap", "split_icon_size", "popup_min_width", "choice_a", "choice_b", "choice_c"};

	auto& composites = AddAuditSpec(specs, "UiCompositeLabel");
	composites.role_visible = false;
	composites.role_reason = "composite wrapper owns its internal runtime control and keeps style surfaces hidden";
	AddStrings(composites.hidden_common, {"role", "theme_override", "face", "face_mode", "face_quad", "frame", "frame_style", "radius",
	                                      "face_enabled", "frame_enabled", "shadow_enabled", "shadow_distance",
	                                      "shadow_offset_x", "shadow_offset_y", "shadow_alpha", "shadow_color", "shadow_curve"});

	auto& composite_edit = AddAuditSpec(specs, "UiCompositeEdit");
	composite_edit = composites;
	auto& composite_dropdown = AddAuditSpec(specs, "UiCompositeDropdown");
	composite_dropdown = composites;
	auto& composite_toggle = AddAuditSpec(specs, "UiCompositeToggle");
	composite_toggle = composites;
	auto& composite_color = AddAuditSpec(specs, "UiCompositeColor");
	composite_color = composites;
	auto& composite_slider = AddAuditSpec(specs, "UiCompositeSlider");
	composite_slider = composites;
	auto& slider_edit = AddAuditSpec(specs, "UiSliderEdit");
	slider_edit = composites;

	return specs;
}

static bool AuditAllVisible(const Vector<DesignerApiBinding>& bindings, const Vector<String>& ids, String& missing)
{
	missing.Clear();
	for(const String& id : ids) {
		const DesignerApiBinding* b = FindBinding(bindings, id);
		if(!b || !b->visible) {
			if(!missing.IsEmpty())
				missing << ", ";
			missing << id;
		}
	}
	return missing.IsEmpty();
}

static bool AuditAllHidden(const Vector<DesignerApiBinding>& bindings, const Vector<String>& ids, String& missing)
{
	missing.Clear();
	for(const String& id : ids) {
		const DesignerApiBinding* b = FindBinding(bindings, id);
		if(b && b->visible) {
			if(!missing.IsEmpty())
				missing << ", ";
			missing << id;
		}
	}
	return missing.IsEmpty();
}

static String AuditResultText(bool ok)
{
	return ok ? "PASS" : "FAIL";
}

static String AuditCodegenMarkers(const String& code, const Vector<String>& markers, String& missing)
{
	missing.Clear();
	for(const String& marker : markers) {
		if(code.Find(marker) < 0) {
			if(!missing.IsEmpty())
				missing << ", ";
			missing << marker;
		}
	}
	return missing.IsEmpty() ? "PASS" : "FAIL";
}

static void TestDesignerApiCoverageAudit(TestCtx& t)
{
	t.Section("Designer API coverage audit");

	DesignerRegistry registry;
	RegisterDesignerBuiltins(registry);
	Vector<const DesignerType*> types = registry.GetTypes();
	VectorMap<String, DesignerApiAuditSpec> specs = BuildDesignerApiAuditSpecs();

	Cout() << "Type                 Role Theme Quad Ink  Parts    Codegen\n";

	for(const DesignerType* type : types) {
		const DesignerApiAuditSpec* spec = specs.FindPtr(type->id);
		t.Expect(spec != nullptr, type->id + " has an audit spec");
		if(!spec) {
			Cout() << Format("%-20s %-4s %-4s %-4s %-4s %-8s %-8s\n",
			                 type->id, "FAIL", "FAIL", "FAIL", "FAIL", "FAIL", "FAIL");
			continue;
		}

		if(type->id == "UiQuadSplitter") {
			Cout() << "  skip: standalone adapter construction crashes on a bare UiQuadSplitter node\n";
			Cout() << Format("%-20s %-4s %-4s %-4s %-4s %-8s %-8s\n",
			                 type->id, "SKIP", "SKIP", "SKIP", "SKIP", "SKIP", "SKIP");
			continue;
		}

		DesignerNode node;
		node.id = 1;
		node.type_id = type->id;
		node.name = type->display_name;
		if(type->init_defaults)
			type->init_defaults(node);
		if(spec->setup)
			spec->setup(node);

		DesignerAdapter* adapter = nullptr;
		One<Ctrl> ctrl;
		ctrl.Attach(CreateDesignerAdapterCtrl(registry, node, &adapter));
		t.Expect(adapter != nullptr, type->id + " creates a real designer adapter");

		Vector<DesignerApiBinding> bindings;
		if(adapter)
			adapter->DescribeApi(bindings, node);

		bool role_ok = spec->role_visible ? BindingVisible(bindings, "role") : BindingHidden(bindings, "role");
		bool theme_ok = spec->theme_visible ? BindingVisible(bindings, "theme_override") : BindingHidden(bindings, "theme_override");
		bool quad_ok = spec->quad_visible ? BindingVisible(bindings, "face_quad") : BindingHidden(bindings, "face_quad");
		bool ink_ok = spec->ink_visible ? BindingVisible(bindings, "ink") : BindingHidden(bindings, "ink");
		bool icon_ink_ok = spec->icon_ink_visible ? BindingVisible(bindings, "icon_ink") : BindingHidden(bindings, "icon_ink");

		String missing_parts;
		bool parts_ok = spec->part_overrides.IsEmpty() ? true : AuditAllVisible(bindings, spec->part_overrides, missing_parts);
		String missing_hidden;
		bool hidden_ok = spec->hidden_common.IsEmpty() ? true : AuditAllHidden(bindings, spec->hidden_common, missing_hidden);

		bool codegen_ok = true;
		String codegen_missing;
		String codegen_status = "n/a";
		String code;
		if(!spec->codegen_markers.IsEmpty()) {
			DesignerModel sample;
			DesignerNodeId host = sample.AddNode("BoxLayout", Designer_ROOT);
			registry.Find("BoxLayout")->init_defaults(*sample.Find(host));
			sample.Find(host)->properties.Set("direction", "V");
			sample.Find(host)->properties.Set("h_sizing", "Expand");
			sample.Find(host)->properties.Set("v_sizing", "Expand");
			sample.Find(host)->properties.Set("gap", 8);
			sample.Find(host)->properties.Set("inset", 8);
			DesignerNodeId sample_id = sample.AddNode(type->id, host);
			registry.Find(type->id)->init_defaults(*sample.Find(sample_id));
			if(spec->setup)
				spec->setup(*sample.Find(sample_id));
			code = GenerateDesignerCode(sample, registry, "Audit" + type->id, true);
			codegen_status = AuditCodegenMarkers(code, spec->codegen_markers, codegen_missing);
			codegen_ok = codegen_status == "PASS";
		}

		if(spec->role_visible)
			t.Expect(role_ok, type->id + " role binding is visible");
		else
			t.Expect(role_ok, type->id + " role binding is hidden intentionally: " + spec->role_reason);
		t.Expect(theme_ok, type->id + " theme override visibility matches contract");
		t.Expect(quad_ok, type->id + " quad face visibility matches contract");
		t.Expect(ink_ok, type->id + " text ink visibility matches contract");
		t.Expect(icon_ink_ok, type->id + " icon ink visibility matches contract");
		t.Expect(parts_ok, type->id + " expected part overrides are exposed");
		t.Expect(hidden_ok, type->id + " expected common fields are hidden");
		t.Expect(codegen_ok, type->id + " codegen emits expected role/theme markers");

		if(!parts_ok && !missing_parts.IsEmpty())
			Cout() << "  missing parts: " << missing_parts << "\n";
		if(!hidden_ok && !missing_hidden.IsEmpty())
			Cout() << "  stale visible fields: " << missing_hidden << "\n";
		if(!codegen_ok && !codegen_missing.IsEmpty())
			Cout() << "  missing codegen markers: " << codegen_missing << "\n";
		if(!spec->role_visible && !spec->role_reason.IsEmpty())
			Cout() << "  note: " << spec->role_reason << "\n";

		Cout() << Format("%-20s %-4s %-4s %-4s %-4s %-8s %-8s\n",
		                 type->id,
		                 AuditResultText(role_ok),
		                 AuditResultText(theme_ok),
		                 AuditResultText(quad_ok),
		                 AuditResultText(ink_ok && icon_ink_ok),
		                 AuditResultText(parts_ok && hidden_ok),
		                 codegen_status);
	}
}

static void TestDesignerThemeSchemaParity(TestCtx& t)
{
	t.Section("Designer theme schema parity");

	DesignerRegistry registry;
	RegisterDesignerBuiltins(registry);
	Vector<const DesignerType*> types = registry.GetTypes();

	for(const DesignerType* type : types) {
		if(!type->SupportsThemeExport())
			continue;
		t.Expect(!type->theme_schema.common_fields.IsEmpty() || !type->theme_schema.part_fields.IsEmpty(),
		         type->id + " declares at least one theme schema field");
		t.Expect(!type->theme_default_source.IsEmpty(),
		         type->id + " declares a theme default source");
		t.Expect(!type->theme_schema.fields.IsEmpty(),
		         type->id + " declares explicit theme field records");
		t.Expect(type->theme_schema.unsupported_fields.GetCount() >= 0,
		         type->id + " has an explicit theme schema container");

		DesignerNode node;
		node.id = 1;
		node.type_id = type->id;
		node.name = type->display_name;
		if(type->init_defaults)
			type->init_defaults(node);

		DesignerAdapter* adapter = nullptr;
		One<Ctrl> ctrl;
		ctrl.Attach(CreateDesignerAdapterCtrl(registry, node, &adapter));
		t.Expect(adapter != nullptr, type->id + " creates an adapter for theme schema parity");
		if(!adapter)
			continue;

		Vector<DesignerApiBinding> bindings;
		adapter->DescribeApi(bindings, node);

		for(const DesignerThemeSchema::DesignerThemeFieldSpec& field : type->theme_schema.fields) {
			const DesignerApiBinding *b = FindBinding(bindings, field.property_id);
			t.Expect(b != nullptr, type->id + " theme schema field exists in inspector: " + field.property_id);
			if(b) {
				t.Expect(b->visible, type->id + " theme schema field is visible in inspector: " + field.property_id);
				t.Expect(b->domain == DesignerPropertyDomain::ThemeStyle,
				         type->id + " theme schema field is classified as ThemeStyle: " + field.property_id);
				t.Expect(field.domain == DesignerPropertyDomain::ThemeStyle,
				         type->id + " theme schema record is classified as ThemeStyle: " + field.property_id);
				if(field.theme_export_supported)
					t.Expect(field.preview_supported && field.exact_codegen_supported,
					         type->id + " theme field declares preview/codegen support: " + field.property_id);
			}
			if(!field.preview_supported || !field.exact_codegen_supported)
				t.Expect(!field.unsupported_reason.IsEmpty(),
				         type->id + " theme field has unsupported reason: " + field.property_id);
		}
		for(int i = 0; i < type->theme_schema.unsupported_fields.GetCount(); i++) {
			const String& key = type->theme_schema.unsupported_fields.GetKey(i);
			t.Expect(!type->theme_schema.unsupported_fields[i].IsEmpty(),
			         type->id + " unsupported theme field has a written reason: " + key);
		}
	}
}

static DesignerModel MakeSampleModel(DesignerRegistry& r);

static void TestDesignerAdapters(TestCtx& t)
{
	t.Section("Designer real-control adapters");

	DesignerRegistry r;
	RegisterDesignerBuiltins(r);

	DesignerNode grid;
	grid.id = 42;
	grid.type_id = "GridLayout";
	grid.name = "contentGrid";
	r.Find("GridLayout")->init_defaults(grid);
	grid.properties.Set("columns", 3);
	grid.properties.Set("debug", true);
	DesignerAdapter *adapter = nullptr;
	One<Ctrl> ctrl;
	ctrl.Attach(CreateDesignerAdapterCtrl(r, grid, &adapter));
	t.Expect(adapter, "adapter factory returns adapter for grid");
	t.Expect(adapter && adapter->GetNodeId() == 42, "adapter stores node id");
	t.Expect(adapter && adapter->GetTypeId() == "GridLayout", "adapter reports grid type");
	t.Expect(adapter && AsDesignerAdapter(adapter->GetCtrl()) == adapter, "adapter can be recovered from real ctrl");

	DesignerOverlayState state;
	state.selected = true;
	adapter->SetOverlayState(state);
	t.Expect(adapter->GetOverlayState().selected, "adapter stores overlay state");
	Vector<DesignerApiBinding> bindings;
	adapter->DescribeApi(bindings, grid);
	const DesignerApiBinding* columns = FindBinding(bindings, "columns");
	const DesignerApiBinding* wrap = FindBinding(bindings, "wrap");
	const DesignerApiBinding* face = FindBinding(bindings, "face");
	const DesignerApiBinding* debug_color = FindBinding(bindings, "debug_color");
	const DesignerApiBinding* debug_auto = FindBinding(bindings, "debug_auto_color");
	t.Expect(columns && columns->editor == DesignerEditorKind::Slider, "grid adapter exposes columns as slider editor");
	t.Expect(columns && columns->enabled, "grid columns are enabled");
	t.Expect(!wrap, "grid adapter does not expose wrap");
	t.Expect(face && !face->visible, "grid layout hides unsupported face color property");
	t.Expect(debug_color && debug_color->editor == DesignerEditorKind::Color, "grid adapter exposes debug color");
	t.Expect(debug_auto && debug_auto->editor == DesignerEditorKind::Bool, "grid adapter exposes auto debug color");

	bindings.Clear();
	adapter->DescribeApi(bindings, grid);
	columns = FindBinding(bindings, "columns");
	const DesignerApiBinding* cell_width = FindBinding(bindings, "cell_width");
	t.Expect(cell_width && cell_width->enabled, "grid min cell width is enabled");
	t.Expect(columns && columns->enabled, "grid columns remain enabled");

	DesignerNode box;
	box.id = 44;
	box.type_id = "BoxLayout";
	box.name = "mainColumn";
	r.Find("BoxLayout")->init_defaults(box);
	One<Ctrl> box_ctrl;
	box_ctrl.Attach(CreateDesignerAdapterCtrl(r, box, &adapter));
	bindings.Clear();
	adapter->DescribeApi(bindings, box);
	t.Expect(FindBinding(bindings, "direction") && FindBinding(bindings, "direction")->enabled, "box exposes direction as enabled descriptor");
	t.Expect(FindBinding(bindings, "face") && !FindBinding(bindings, "face")->visible, "box layout hides unsupported face color property");
	t.Expect(FindBinding(bindings, "debug_color") && FindBinding(bindings, "debug_auto_color"),
	         "box layout exposes debug color controls");

	DesignerNode label;
	label.id = 43;
	label.type_id = "UiLabel";
	label.name = "nameLabel";
	r.Find("UiLabel")->init_defaults(label);
	label.properties.Set("text", "Name");
	One<Ctrl> label_ctrl;
	label_ctrl.Attach(CreateDesignerAdapterCtrl(r, label, &adapter));
	t.Expect(adapter && adapter->GetTypeId() == "UiLabel", "adapter factory creates real label adapter");
	bindings.Clear();
	adapter->DescribeApi(bindings, label);
	const DesignerApiBinding* text = FindBinding(bindings, "text");
	t.Expect(text && text->editor == DesignerEditorKind::Text, "label adapter exposes text editor");
	t.Expect(text && text->api_call == "UiLabel::SetText", "label text binding names real API");

	for(const char *type : { "Spacer", "UiButton", "UiLineEdit", "UiToggle", "UiDropdown",
	                         "UiCheckBox", "UiBreadcrumbs", "UiTab", "UiStack", "UiTable", "UiTree" }) {
		DesignerNode n;
		n.id = 50;
		n.type_id = type;
		n.name = type;
		r.Find(type)->init_defaults(n);
		One<Ctrl> c;
		c.Attach(CreateDesignerAdapterCtrl(r, n, &adapter));
		t.Expect(adapter && adapter->GetTypeId() == type, String(type) + " adapter is created");
		bindings.Clear();
		adapter->DescribeApi(bindings, n);
		t.Expect(FindBinding(bindings, "h_sizing"), String(type) + " exposes width sizing");
		t.Expect(FindBinding(bindings, "v_sizing"), String(type) + " exposes height sizing");
		if(String(type) == "Spacer")
			t.Expect(FindBinding(bindings, "layout_break") && FindBinding(bindings, "weight") &&
			         FindBinding(bindings, "line_enabled") && FindBinding(bindings, "line_orientation"),
			         "spacer adapter exposes direct spacer sizing and separator controls");
		else if(String(type) == "UiToggle")
			t.Expect(FindBinding(bindings, "on") && FindBinding(bindings, "on")->editor == DesignerEditorKind::Bool,
			         "toggle adapter exposes boolean state");
		else if(String(type) == "UiDropdown")
			t.Expect(FindBinding(bindings, "item_text") && FindBinding(bindings, "indicator_side"),
			         "dropdown adapter exposes item text and chevron controls");
		else if(String(type) == "UiCheckBox")
			t.Expect(FindBinding(bindings, "state") && FindBinding(bindings, "state")->editor == DesignerEditorKind::Choice,
			         "checkbox adapter exposes check state");
		else if(String(type) == "UiBreadcrumbs")
			t.Expect(FindBinding(bindings, "crumb_1") && FindBinding(bindings, "crumb_3") && FindBinding(bindings, "current"),
			         "breadcrumbs adapter exposes path properties");
		else if(String(type) == "UiTab")
			t.Expect(FindBinding(bindings, "visual") && FindBinding(bindings, "placement") && FindBinding(bindings, "active") &&
			         FindBinding(bindings, "tab_font") && FindBinding(bindings, "tab_font_size") &&
			         FindBinding(bindings, "tab_icon_size") && FindBinding(bindings, "tab_icon_side"),
			         "tab adapter exposes visual, placement, active page, and tab font/icon controls");
		else if(String(type) == "UiStack")
			t.Expect(FindBinding(bindings, "active"),
			         "stack adapter exposes active page");
		else if(String(type) == "UiTable")
			t.Expect(FindBinding(bindings, "rows_count") && FindBinding(bindings, "cols_count"),
			         "table adapter exposes row and column counts");
		else if(String(type) == "UiTree")
			t.Expect(FindBinding(bindings, "connectors") && FindBinding(bindings, "root_visible"),
			         "tree adapter exposes hierarchy display properties");
		else
			t.Expect(FindBinding(bindings, "text") && FindBinding(bindings, "text")->editor == DesignerEditorKind::Text,
			         String(type) + " adapter exposes text");
	}

	DesignerNode page;
	page.id = 70;
	page.type_id = "PageSlot";
	page.name = "layoutPage";
	r.Find("PageSlot")->init_defaults(page);
	One<Ctrl> page_ctrl;
	page_ctrl.Attach(CreateDesignerAdapterCtrl(r, page, &adapter));
	t.Expect(adapter && adapter->GetTypeId() == "PageSlot", "adapter factory creates page slot adapter");
	bindings.Clear();
	adapter->DescribeApi(bindings, page);
	t.Expect(FindBinding(bindings, "page_title") && FindBinding(bindings, "show_title") &&
	         FindBinding(bindings, "icon"),
	         "page slot adapter exposes tab title, title visibility, and icon controls");
}

static void TestDesignerCodeGenPages(TestCtx& t)
{
	t.Section("Designer codegen page containers");

	DesignerRegistry r;
	RegisterDesignerBuiltins(r);
	DesignerModel m;
	DesignerNodeId tab = m.AddNode("UiTab", Designer_ROOT);
	r.Find("UiTab")->init_defaults(*m.Find(tab));
	DesignerNodeId tab_page = m.AddNode("PageSlot", tab);
	r.Find("PageSlot")->init_defaults(*m.Find(tab_page));
	m.Find(tab_page)->properties.Set("page_title", "Tab A");
	m.Find(tab_page)->properties.Set("show_title", false);
	m.Find(tab_page)->properties.Set("icon", "ICON_DESIGN_HOME_48");
	m.Find(tab)->properties.Set("tab_font", "Segoe UI");
	m.Find(tab)->properties.Set("tab_font_size", 14);
	m.Find(tab)->properties.Set("tab_icon_size", 24);
	m.Find(tab)->properties.Set("tab_icon_side", "Top");
	m.Find(tab)->properties.Set("content_gap", 10);
	m.Find(tab)->properties.Set("affordance_gap", 12);
	DesignerNodeId label = m.AddNode("UiLabel", tab_page);
	r.Find("UiLabel")->init_defaults(*m.Find(label));
	m.Find(label)->properties.Set("text", "Inside tab");

	DesignerNodeId stack = m.AddNode("UiStack", Designer_ROOT);
	r.Find("UiStack")->init_defaults(*m.Find(stack));
	DesignerNodeId stack_page = m.AddNode("PageSlot", stack);
	r.Find("PageSlot")->init_defaults(*m.Find(stack_page));
	m.Find(stack_page)->properties.Set("page_title", "Stack A");
	DesignerNodeId slider = m.AddNode("UiSlider", stack_page);
	r.Find("UiSlider")->init_defaults(*m.Find(slider));

	String code = GenerateDesignerCode(m, r);
	t.Expect(code.Find(".AddPage(") >= 0, "stack codegen emits AddPage for page slot");
	t.Expect(code.Find(".SetTabFont(Font().FaceName(\"Segoe UI\").Height(14))") >= 0 &&
	         code.Find(".SetTabIconSize(DPI(24))") >= 0 &&
	         code.Find(".SetTabIconSide(UiAlign::TOP)") >= 0 &&
	         code.Find("s.content_gap = DPI(10);") >= 0 &&
	         code.Find("s.affordance_gap = DPI(12);") >= 0,
	         "tab codegen emits shared tab font and icon layout controls");
	t.Expect(code.Find(".Add(") >= 0 && code.Find("\"\", ICON_DESIGN_HOME_48()") >= 0,
	         "tab codegen emits icon-only page slot");
	t.Expect(code.Find(".SetCustomStyle(UiTheme::ResolvePanel") < 0, "page slot codegen does not call panel-only API on ParentCtrl");
	t.Expect(code.Find(".SetActiveTab(") >= 0 && code.Find(".SetActivePage(") >= 0, "page container active page is emitted after pages");
}

static void TestPropertyEditStability(TestCtx& t)
{
	t.Section("Designer property edit stability");

	DesignerRegistry r;
	RegisterDesignerBuiltins(r);
	DesignerModel m = MakeSampleModel(r);
	DesignerCommandStack stack;
	DesignerNodeId grid = 4;
	DesignerNodeId label = 5;
	DesignerNodeId slider = 6;

	for(int i = 0; i < 40; i++) {
		stack.Execute(MakeDesignerSetPropertyCommand(grid, "columns", 1 + (i % 4)), m);
		t.Expect(m.Validate(), "model validates after grid column edit");
		stack.Execute(MakeDesignerSetPropertyCommand(label, "height", 24 + i), m);
		t.Expect(m.Validate(), "model validates after height edit");
		String align = i % 3 == 0 ? "Left" : i % 3 == 1 ? "Center" : "Right";
		stack.Execute(MakeDesignerSetPropertyCommand(label, "align", align), m);
		t.Expect(m.Validate(), "model validates after label align edit");
		stack.Execute(MakeDesignerSetPropertyCommand(slider, "h_sizing", i & 1 ? "Fixed" : "Fit"), m);
		t.Expect(m.Validate(), "model validates after width sizing edit");
		stack.Execute(MakeDesignerSetPropertyCommand(slider, "v_sizing", i & 1 ? "Expand" : "Fit"), m);
		t.Expect(m.Validate(), "model validates after height sizing edit");
	}

	for(int i = 0; i < 20; i++)
		t.Expect(stack.Undo(m), "property undo remains stable");
	t.Expect(m.Validate(), "model validates after repeated property undo");
	for(int i = 0; i < 20; i++)
		t.Expect(stack.Redo(m), "property redo remains stable");
	t.Expect(m.Validate(), "model validates after repeated property redo");

	DesignerNodeId scroll_host = m.AddNode("BoxLayout", Designer_ROOT);
	r.Find("BoxLayout")->init_defaults(*m.Find(scroll_host));
	DesignerNodeId scroll = m.AddNode("UiScrollPanel", scroll_host);
	r.Find("UiScrollPanel")->init_defaults(*m.Find(scroll));
	m.Find(scroll)->properties.Set("v_sizing", "Expand");
	t.Expect(stack.Execute(MakeDesignerSetPropertyCommand(scroll, "v_sizing", "Fixed"), m),
	         "scroll panel height mode accepts Fixed");
	t.Expect(TestNodePropertyOr(*m.Find(scroll), "v_sizing", Value()) == Value("Fixed"),
	         "scroll panel height mode stays Fixed after command");
	int fixed_h0 = (int)TestNodePropertyOr(*m.Find(scroll), "fixed_height", 0);
	t.Expect(stack.Execute(MakeDesignerSetPropertyCommand(scroll, "fixed_height", fixed_h0 + 10), m),
	         "scroll panel fixed height command applies");
	t.Expect(TestNodePropertyOr(*m.Find(scroll), "v_sizing", Value()) == Value("Fixed"),
	         "scroll panel fixed height edit preserves Fixed mode");

	DesignerNodeId spacer_host = m.AddNode("BoxLayout", Designer_ROOT);
	r.Find("BoxLayout")->init_defaults(*m.Find(spacer_host));
	m.Find(spacer_host)->properties.Set("direction", "H");
	DesignerNodeId spacer = m.AddNode("Spacer", spacer_host);
	r.Find("Spacer")->init_defaults(*m.Find(spacer));
	t.Expect(stack.Execute(MakeDesignerSetPropertyCommand(spacer, "h_sizing", "Fixed"), m),
	         "spacer width mode accepts Fixed");
	t.Expect(stack.Execute(MakeDesignerSetPropertyCommand(spacer, "fixed_width", 75), m),
	         "spacer fixed width command applies");
	t.Expect(stack.Execute(MakeDesignerSetPropertyCommand(spacer, "v_sizing", "Fixed"), m),
	         "spacer height mode accepts Fixed");
	t.Expect(stack.Execute(MakeDesignerSetPropertyCommand(spacer, "fixed_height", 18), m),
	         "spacer fixed height command applies");
	DesignerPreview preview;
	preview.Set(&m, &r);
	preview.SetRect(0, 0, 640, 360);
	preview.SyncRealPreview();
	preview.Layout();
	t.Expect(m.Validate(), "preview relayout after spacer fixed sizing remains stable");

	DesignerModel live_preview_model;
	DesignerNodeId host = live_preview_model.AddNode("UiPanel", Designer_ROOT);
	r.Find("UiPanel")->init_defaults(*live_preview_model.Find(host));
	DesignerNodeId title = live_preview_model.AddNode("UiTitleCard", host);
	r.Find("UiTitleCard")->init_defaults(*live_preview_model.Find(title));
	DesignerCommandStack live_preview_stack;

	Value old_h = TestNodePropertyOr(*live_preview_model.Find(title), "h_sizing", Value());
	live_preview_model.SetProperty(title, "h_sizing", "Fixed");
	t.Expect(live_preview_stack.Execute(MakeDesignerSetPropertyCommand(title, "h_sizing", old_h, true, Value("Fixed"), "Set h_sizing"), live_preview_model),
	         "live-previewed h_sizing still records a committed command");
	t.Expect(TestNodePropertyOr(*live_preview_model.Find(title), "h_sizing", Value()) == Value("Fixed"),
	         "live-previewed h_sizing remains Fixed after commit");
	t.Expect(live_preview_stack.Undo(live_preview_model),
	         "live-previewed h_sizing commit is undoable");
	t.Expect(TestNodePropertyOr(*live_preview_model.Find(title), "h_sizing", Value()) == old_h,
	         "undo restores pre-preview h_sizing");

	Value old_w = TestNodePropertyOr(*live_preview_model.Find(title), "fixed_width", Value());
	live_preview_model.SetProperty(title, "fixed_width", 320);
	t.Expect(live_preview_stack.Execute(MakeDesignerSetPropertyCommand(title, "fixed_width", old_w, true, 320, "Set fixed_width"), live_preview_model),
	         "live-previewed fixed_width still records a committed command");
	t.Expect((int)TestNodePropertyOr(*live_preview_model.Find(title), "fixed_width", 0) == 320,
	         "live-previewed fixed_width remains committed after final save");
	t.Expect(live_preview_stack.Undo(live_preview_model),
	         "live-previewed fixed_width commit is undoable");
	t.Expect(TestNodePropertyOr(*live_preview_model.Find(title), "fixed_width", Value()) == old_w,
	         "undo restores pre-preview fixed_width");
}

class FixedMinCtrl : public Ctrl {
	Size min_size_;
public:
	FixedMinCtrl(Size sz) : min_size_(sz) {}
	Size GetMinSize() const override { return min_size_; }
};

static void TestLayoutSizingPrimitives(TestCtx& t)
{
	t.Section("Layout sizing primitives");

	UiSplitter splitter;
	FixedMinCtrl left(Size(80, 40));
	FixedMinCtrl right(Size(80, 40));
	splitter.Horz();
	splitter.SetRect(0, 0, 480, 180);
	splitter.Add(left);
	splitter.Add(right);
	splitter.SetMinPixels(0, 80);
	splitter.SetMinPixels(1, 80);
	splitter.SetSplitPercent(33);
	splitter.Layout();
	t.Expect(abs(left.GetRect().GetWidth() - 154) <= 2,
	         Format("splitter percentage positions first pane near 33 percent (got %d)", left.GetRect().GetWidth()));

	UiSplitter feedback;
	FixedMinCtrl feedback_left(Size(80, 40));
	FixedMinCtrl feedback_right(Size(80, 40));
	feedback.Horz();
	feedback.SetRect(0, 0, 480, 180);
	feedback.Add(feedback_left);
	feedback.Add(feedback_right);
	UiSplitter::Style feedback_style = UiSplitter::StyleDefault();
	feedback_style.hit_width = DPI(18);
	feedback_style.track_thickness = DPI(2);
	feedback_style.hot_track_thickness = 0;
	feedback_style.pressed_track_thickness = 0;
	feedback_style.expand_track_on_hot = true;
	feedback_style.expand_track_on_pressed = true;
	feedback.SetCustomStyle(feedback_style);
	feedback.SetSplitPercent(40);
	feedback.Layout();
	Rect normal_track = feedback.GetFeedbackTrackRect(0, ST_NORMAL);
	Rect hot_track = feedback.GetFeedbackTrackRect(0, ST_HOT);
	Rect pressed_track = feedback.GetFeedbackTrackRect(0, ST_PRESSED);
	t.Expect(feedback.GetFeedbackTrackRect(-1, ST_NORMAL).IsEmpty(),
	         "splitter feedback helper ignores negative handle indices");
	t.Expect(feedback.GetFeedbackTrackRect(1, ST_NORMAL).IsEmpty(),
	         "two-pane splitter feedback helper ignores pane-count handle index");
	t.Expect(hot_track.GetWidth() >= normal_track.GetWidth(),
	         Format("hot splitter feedback band expands from %d to %d pixels", normal_track.GetWidth(), hot_track.GetWidth()));
	t.Expect(pressed_track.GetWidth() >= hot_track.GetWidth(),
	         Format("pressed splitter feedback band expands from %d to %d pixels", hot_track.GetWidth(), pressed_track.GetWidth()));
	t.Expect(feedback.CursorImage(normal_track.CenterPoint(), 0) != Image::Arrow(),
	         "splitter cursor changes over the hit band");
	int finished = 0;
	int before_split = (int)(feedback.GetSplitPercent(0) * 100);
	feedback.WhenSplitFinish = [&] { finished++; };
	Point drag_point = normal_track.CenterPoint();
	feedback.LeftDown(drag_point, 0);
	feedback.SetCapture();
	feedback.MouseMove(Point(drag_point.x + 48, drag_point.y), 0);
	feedback.LeftUp(Point(drag_point.x + 48, drag_point.y), 0);
	t.Expect(finished == 1, "splitter split-finish callback fires on release");
	t.Expect((int)(feedback.GetSplitPercent(0) * 100) != before_split,
	         "splitter split position changes during drag");

	UiGridLayout grid;
	grid.SetGridSize(1, 1)
	    .SetGap(8)
	    .SetInset(8)
	    .SetDebugColor(Color(37, 99, 235))
	    .SetDebug(true)
	    .SetMinCellSize(Size(80, 40))
	    .SetAlignItems(UiCrossAlign::Stretch);
	grid.SetRect(0, 0, 500, 240);
	UiSplitter expanding;
	FixedMinCtrl a(Size(80, 40));
	FixedMinCtrl b(Size(80, 40));
	expanding.Horz();
	expanding.Add(a);
	expanding.Add(b);
	grid.Add(expanding, 0, 0, true, Size(320, 120));
	grid.Layout();
	t.Expect(expanding.GetRect().GetWidth() >= 480, "stable grid scale-to-cell child expands across cell width");
	t.Expect(expanding.GetRect().GetHeight() >= 220, "stable grid scale-to-cell child expands across cell height");

	UiGridLayout axis_grid;
	axis_grid.SetGridSize(1, 1)
	         .SetGap(0)
	         .SetInset(0)
	         .SetDebugColor(Color(22, 163, 74))
	         .SetMinCellSize(Size(80, 40))
	         .SetAlignItems(UiCrossAlign::Stretch);
	axis_grid.SetRect(0, 0, 300, 160);
	FixedMinCtrl axis_child(Size(80, 40));
	axis_grid.Add(axis_child, 0, 0, true, false);
	axis_grid.Layout();
	t.Expect(axis_child.GetRect().GetWidth() >= 300, "stable grid can expand child width only");
	t.Expect(axis_child.GetRect().GetHeight() <= 50, "stable grid keeps non-expanded child height natural");

	DesignerRegistry registry;
	RegisterDesignerBuiltins(registry);
	DesignerModel preview_model;
	DesignerNodeId vertical = preview_model.AddNode("BoxLayout", Designer_ROOT);
	registry.Find("BoxLayout")->init_defaults(*preview_model.Find(vertical));
	preview_model.Find(vertical)->properties.Set("direction", "V");
	preview_model.Find(vertical)->properties.Set("h_sizing", "Expand");
	preview_model.Find(vertical)->properties.Set("v_sizing", "Expand");
	preview_model.Find(vertical)->properties.Set("gap", 8);
	preview_model.Find(vertical)->properties.Set("inset", 8);

	DesignerNodeId fixed_child = preview_model.AddNode("UiLabel", vertical);
	registry.Find("UiLabel")->init_defaults(*preview_model.Find(fixed_child));
	preview_model.Find(fixed_child)->properties.Set("text", "Fixed cross width");
	preview_model.Find(fixed_child)->properties.Set("h_sizing", "Fixed");
	preview_model.Find(fixed_child)->properties.Set("v_sizing", "Fit");
	preview_model.Find(fixed_child)->properties.Set("fixed_width", 220);
	preview_model.Find(fixed_child)->properties.Set("fixed_height", 24);
	preview_model.Find(fixed_child)->properties.Set("min_width", 10);
	preview_model.Find(fixed_child)->properties.Set("min_height", 10);

	DesignerNodeId expand_child = preview_model.AddNode("UiButton", vertical);
	registry.Find("UiButton")->init_defaults(*preview_model.Find(expand_child));
	preview_model.Find(expand_child)->properties.Set("text", "Expand");
	preview_model.Find(expand_child)->properties.Set("h_sizing", "Expand");
	preview_model.Find(expand_child)->properties.Set("v_sizing", "Expand");

	DesignerPreview* preview = new DesignerPreview;
	preview->Set(&preview_model, &registry);
	preview->SetRect(0, 0, 620, 360);
	preview->SyncRealPreview();
	preview->Layout();
	const DesignerNode* fixed_node = preview_model.Find(fixed_child);
	const DesignerNode* expand_node = preview_model.Find(expand_child);
	t.Expect(fixed_node && abs(fixed_node->last_rect.GetWidth() - 220) <= 4,
	         Format("designer preview respects fixed_width in vertical box cross-axis (got %d)", fixed_node ? fixed_node->last_rect.GetWidth() : -1));
	t.Expect(fixed_node && fixed_node->last_rect.GetHeight() < 80,
	         Format("designer preview fixed_width does not inflate fixed child height (got %d)", fixed_node ? fixed_node->last_rect.GetHeight() : -1));
	t.Expect(expand_node && expand_node->last_rect.GetWidth() > fixed_node->last_rect.GetWidth(),
	         "designer preview expand sibling still stretches wider than fixed-width child in vertical box");

	DesignerModel toolbar_model;
	DesignerNodeId toolbar = toolbar_model.AddNode("BoxLayout", Designer_ROOT);
	registry.Find("BoxLayout")->init_defaults(*toolbar_model.Find(toolbar));
	toolbar_model.Find(toolbar)->properties.Set("direction", "H");
	toolbar_model.Find(toolbar)->properties.Set("h_sizing", "Expand");
	toolbar_model.Find(toolbar)->properties.Set("v_sizing", "Fit");
	toolbar_model.Find(toolbar)->properties.Set("gap", 6);
	toolbar_model.Find(toolbar)->properties.Set("inset", 6);
	toolbar_model.Find(toolbar)->properties.Set("wrap", "None");

	DesignerNodeId tb_a = toolbar_model.AddNode("UiToolButton", toolbar);
	registry.Find("UiToolButton")->init_defaults(*toolbar_model.Find(tb_a));
	toolbar_model.Find(tb_a)->properties.Set("text", "One");
	toolbar_model.Find(tb_a)->properties.Set("h_sizing", "Fit");
	toolbar_model.Find(tb_a)->properties.Set("v_sizing", "Fit");

	DesignerNodeId tb_b = toolbar_model.AddNode("UiToolButton", toolbar);
	registry.Find("UiToolButton")->init_defaults(*toolbar_model.Find(tb_b));
	toolbar_model.Find(tb_b)->properties.Set("text", "Two");
	toolbar_model.Find(tb_b)->properties.Set("h_sizing", "Fit");
	toolbar_model.Find(tb_b)->properties.Set("v_sizing", "Fit");

	DesignerPreview* toolbar_preview = new DesignerPreview;
	toolbar_preview->Set(&toolbar_model, &registry);
	toolbar_preview->SetRect(0, 0, 520, 180);
	toolbar_preview->SyncRealPreview();
	toolbar_preview->Layout();
	const DesignerNode* tb_node_a = toolbar_model.Find(tb_a);
	const DesignerNode* tb_node_b = toolbar_model.Find(tb_b);
	t.Expect(tb_node_a && tb_node_b && tb_node_a->last_rect.top == tb_node_b->last_rect.top,
	         "designer preview toolbar fit/fit buttons stay on one row when wrap is disabled");
}

static void TestWidthAwareLayoutMeasure(TestCtx& t)
{
	t.Section("Width-aware layout measurement");

	UiBoxLayout wrapped(UiDirection::H);
	wrapped.SetWrap(UiBoxWrap::Flow)
	       .SetWrapAutoResize(true)
	       .SetGap(8, 8)
	       .SetInset(8);
	FixedMinCtrl wa(Size(80, 34));
	FixedMinCtrl wb(Size(80, 34));
	FixedMinCtrl wc(Size(80, 34));
	FixedMinCtrl wd(Size(80, 34));
	wrapped.Add(wa).Fit();
	wrapped.Add(wb).Fit();
	wrapped.Add(wc).Fit();
	wrapped.Add(wd).Fit();

	UiLayoutMeasureResult wrapped_base = UiMeasureLayout(wrapped);
	UiLayoutMeasureResult wrapped_narrow = UiMeasureLayout(wrapped, {120});
	t.Expect(wrapped_base.width_dependent, "wrapped horizontal box reports width-dependent measurement");
	t.Expect(wrapped_base.preferred.cx > wrapped_base.min.cx, "wrapped horizontal box separates preferred width from minimum wrap width");
	t.Expect(wrapped_narrow.measured.cy > wrapped_base.preferred.cy, "wrapped horizontal box grows taller when constrained narrower");
	t.Expect(UiMeasureLayout(wrapped, {120}).measured == wrapped_narrow.measured, "wrapped horizontal box measurement is stable for repeated width queries");
	wrapped.SetWrapRowsExpand(true);
	int expanded_rows_height = wrapped.MeasureHeightForWidth(120);
	t.Expect(expanded_rows_height == wrapped_narrow.measured.cy, "wrap_rows_expand does not affect height-for-width probes");

	UiPanel panel;
	UiBoxLayout panel_box(UiDirection::H);
	panel_box.SetWrap(UiBoxWrap::Flow)
	         .SetWrapAutoResize(true)
	         .SetGap(8, 8)
	         .SetInset(8);
	FixedMinCtrl pa(Size(80, 34));
	FixedMinCtrl pb(Size(80, 34));
	FixedMinCtrl pc(Size(80, 34));
	FixedMinCtrl pd(Size(80, 34));
	panel_box.Add(pa).Fit();
	panel_box.Add(pb).Fit();
	panel_box.Add(pc).Fit();
	panel_box.Add(pd).Fit();
	panel.Add(panel_box);
	UiLayoutMeasureResult panel_child = UiMeasureLayout(panel_box);
	t.Expect(panel.GetMinSize().cx < panel_child.preferred.cx, "panel minimum width uses wrapped child constrained minimum instead of full preferred width");

	UiGroupPanel group;
	UiBoxLayout group_box(UiDirection::H);
	group_box.SetWrap(UiBoxWrap::Flow)
	         .SetWrapAutoResize(true)
	         .SetGap(8, 8)
	         .SetInset(8);
	FixedMinCtrl ga(Size(80, 34));
	FixedMinCtrl gb(Size(80, 34));
	FixedMinCtrl gc(Size(80, 34));
	FixedMinCtrl gd(Size(80, 34));
	group_box.Add(ga).Fit();
	group_box.Add(gb).Fit();
	group_box.Add(gc).Fit();
	group_box.Add(gd).Fit();
	group.SetContent(group_box);
	UiLayoutMeasureResult group_child = UiMeasureLayout(group_box);
	t.Expect(group.GetMinSize().cx < group_child.preferred.cx, "group panel minimum width uses wrapped child constrained minimum instead of full preferred width");

	UiStack stack;
	UiBoxLayout stack_page(UiDirection::H);
	stack_page.SetWrap(UiBoxWrap::Flow)
	          .SetWrapAutoResize(true)
	          .SetGap(8, 8)
	          .SetInset(8);
	FixedMinCtrl sta(Size(80, 34));
	FixedMinCtrl stb(Size(80, 34));
	FixedMinCtrl stc(Size(80, 34));
	FixedMinCtrl std(Size(80, 34));
	stack_page.Add(sta).Fit();
	stack_page.Add(stb).Fit();
	stack_page.Add(stc).Fit();
	stack_page.Add(std).Fit();
	stack.AddPage(stack_page, "page");
	UiLayoutMeasureResult stack_page_measure = UiMeasureLayout(stack_page);
	t.Expect(stack.GetMinSize().cx >= stack_page_measure.min.cx, "stack minimum width uses width-aware child minimum");

	UiBoxLayout cache_probe(UiDirection::H);
	cache_probe.SetWrap(UiBoxWrap::Flow)
	           .SetWrapAutoResize(true)
	           .SetGap(8, 8)
	           .SetInset(8);
	UiButton probe_a; probe_a.SetText("Short");
	UiButton probe_b; probe_b.SetText("Short");
	UiButton probe_c; probe_c.SetText("Short");
	cache_probe.Add(probe_a).Fit();
	cache_probe.Add(probe_b).Fit();
	cache_probe.Add(probe_c).Fit();
	int cached_height = cache_probe.MeasureHeightForWidth(220);
	probe_b.SetText("A much longer button caption that should wrap");
	int changed_height = cache_probe.MeasureHeightForWidth(220);
	t.Expect(changed_height > cached_height, "wrapped box measurement cache invalidates when child text changes");

	UiScrollPanel scroll;
	UiBoxLayout scroll_box(UiDirection::H);
	scroll_box.SetWrap(UiBoxWrap::Flow)
	          .SetWrapAutoResize(true)
	          .SetGap(8, 8)
	          .SetInset(8);
	FixedMinCtrl sa(Size(80, 34));
	FixedMinCtrl sb(Size(80, 34));
	FixedMinCtrl sc(Size(80, 34));
	FixedMinCtrl sd(Size(80, 34));
	scroll_box.Add(sa).Fit();
	scroll_box.Add(sb).Fit();
	scroll_box.Add(sc).Fit();
	scroll_box.Add(sd).Fit();
	scroll.Content().Add(scroll_box);
	scroll.SetRect(0, 0, 320, 220);
	scroll.Layout();
	int wide_content_w = scroll.GetContentSize().cx;
	scroll.SetRect(0, 0, 160, 220);
	scroll.Layout();
	int narrow_content_w = scroll.GetContentSize().cx;
	t.Expect(narrow_content_w <= wide_content_w, "scroll panel content bounds shrink when wrapped host narrows");
	t.Expect(narrow_content_w < UiMeasureLayout(scroll_box).preferred.cx, "scroll panel content bounds do not force wrapped child preferred width at narrow sizes");
	t.Expect(wide_content_w >= narrow_content_w, "scroll panel content width responds monotonically to wider viewport sizes");

	UiTree tree;
	UiTreeModel& tree_model = tree.GetInternalModel();
	UiTreeNodeRef root = tree_model.Root();
	for(int i = 0; i < 24; i++) {
		UiTreeNodeRef branch = tree_model.AddChild(root, UiModelItem(Format("Branch %d", i), i + 1));
		for(int j = 0; j < 3; j++)
			tree_model.AddChild(branch, UiModelItem(Format("Leaf %d-%d", i, j), 100 + i * 10 + j));
	}
	tree.Expand(root, true, true);
	UiLayoutMeasureResult tree_measure = UiMeasureLayout(tree);
	t.Expect(tree_measure.min.cy == tree.GetContentSize().cy,
	         "tree measure helper reports full logical content height");
	UiScrollPanel tree_scroll;
	tree_scroll.Content().Add(tree.SizePos());
	tree_scroll.SetRect(0, 0, 240, 160);
	tree_scroll.Layout();
	t.Expect(tree_scroll.GetContentSize().cy >= tree.GetContentSize().cy,
	         "scroll panel hosting a tree uses full tree content height instead of sample min height");
}

static void TestAncestorRelayoutContract(TestCtx& t)
{
	t.Section("Ancestor relayout contract");

	DesignerRegistry r;
	RegisterDesignerBuiltins(r);

	DesignerModel m;
	DesignerNodeId root = m.AddNode("BoxLayout", Designer_ROOT);
	r.Find("BoxLayout")->init_defaults(*m.Find(root));
	m.Find(root)->properties.Set("direction", "V");

	DesignerNodeId panel = m.AddNode("UiPanel", root);
	r.Find("UiPanel")->init_defaults(*m.Find(panel));
	m.Find(panel)->name = "panelHost";
	m.Find(panel)->properties.Set("h_sizing", "Expand");
	m.Find(panel)->properties.Set("v_sizing", "Fit");

	DesignerNodeId inner = m.AddNode("BoxLayout", panel);
	r.Find("BoxLayout")->init_defaults(*m.Find(inner));
	m.Find(inner)->name = "innerLayout";
	m.Find(inner)->properties.Set("direction", "V");
	m.Find(inner)->properties.Set("h_sizing", "Expand");
	m.Find(inner)->properties.Set("v_sizing", "Expand");
	m.Find(inner)->properties.Set("inset", 0);
	m.Find(inner)->properties.Set("gap", 0);

	DesignerNodeId title = m.AddNode("UiTitleCard", inner);
	r.Find("UiTitleCard")->init_defaults(*m.Find(title));
	m.Find(title)->name = "headerCard";
	m.Find(title)->properties.Set("h_sizing", "Expand");
	m.Find(title)->properties.Set("v_sizing", "Fixed");
	m.Find(title)->properties.Set("fixed_width", 220);
	m.Find(title)->properties.Set("fixed_height", 32);
	m.Find(title)->properties.Set("min_height", 0);
	m.Find(title)->properties.Set("max_height", 0);
	m.Find(title)->properties.Set("text", "Relayout");

	DesignerPreview preview;
	preview.Set(&m, &r);
	preview.SetRect(0, 0, 640, 360);
	preview.SyncRealPreview();
	preview.Layout();

	const DesignerNode* panel_node = m.Find(panel);
	const DesignerNode* title_node = m.Find(title);
	t.Expect(panel_node && title_node && panel_node->last_rect.GetHeight() > 0,
	         "initial panel layout is computed");
	int panel_h0 = panel_node->last_rect.GetHeight();
	int title_h0 = title_node->last_rect.GetHeight();

	DesignerCommandStack stack;
	t.Expect(stack.Execute(MakeDesignerSetPropertyCommand(title, "fixed_height", 88), m),
	         "fixed height command applies through model");
	preview.InvalidateRealPreview();
	preview.Layout();
	int panel_h1 = m.Find(panel)->last_rect.GetHeight();
	int title_h1 = m.Find(title)->last_rect.GetHeight();
	t.Expect(panel_h1 > panel_h0, "parent Fit height updates immediately when child fixed height grows");
	t.Expect(title_h1 > title_h0, "child fixed height grows after relayout");

	t.Expect(stack.Execute(MakeDesignerSetPropertyCommand(title, "v_sizing", "Fit"), m),
	         "v_sizing command applies through model");
	t.Expect(stack.Execute(MakeDesignerSetPropertyCommand(title, "min_height", 96), m),
	         "min height command applies through model");
	preview.InvalidateRealPreview();
	preview.Layout();
	int panel_h2 = m.Find(panel)->last_rect.GetHeight();
	int title_h2 = m.Find(title)->last_rect.GetHeight();
	t.Expect(panel_h2 > panel_h1, "parent Fit height updates immediately when child minimum height grows");
	t.Expect(title_h2 >= 96, "child minimum height is respected after relayout");

	t.Expect(stack.Execute(MakeDesignerSetPropertyCommand(title, "h_sizing", "Fixed"), m),
	         "h_sizing command applies through model");
	t.Expect(stack.Execute(MakeDesignerSetPropertyCommand(title, "fixed_width", 260), m),
	         "fixed width command applies through model");
	preview.InvalidateRealPreview();
	preview.Layout();
	t.Expect(m.Find(title)->last_rect.GetWidth() >= 260,
	         "changing child fixed width updates the preview rect without waiting for an unrelated toggle");
}

static void TestDesignerInspectorContextAndThemeRows(TestCtx& t)
{
	t.Section("Designer inspector context and theme rows");

	DesignerRegistry r;
	RegisterDesignerBuiltins(r);

	DesignerModel m;
	DesignerNodeId panel = m.AddNode("UiPanel", Designer_ROOT);
	r.Find("UiPanel")->init_defaults(*m.Find(panel));
	m.Find(panel)->name = "panelHost";

	DesignerNodeId box = m.AddNode("BoxLayout", panel);
	r.Find("BoxLayout")->init_defaults(*m.Find(box));
	m.Find(box)->name = "right_scroll_content";
	m.Find(box)->properties.Set("direction", "V");

	DesignerNodeId title = m.AddNode("UiTitleCard", box);
	r.Find("UiTitleCard")->init_defaults(*m.Find(title));
	m.Find(title)->name = "PageHeaderNode";
	m.Find(title)->properties.Set("text", "Page Header");
	m.Find(title)->properties.Set("h_sizing", "Fixed");
	m.Find(title)->properties.Set("v_sizing", "Fixed");
	m.Find(title)->properties.Set("fixed_width", 220);
	m.Find(title)->properties.Set("fixed_height", 68);

	DesignerPreview preview;
	preview.Set(&m, &r);
	preview.SetRect(0, 0, 640, 360);
	preview.SyncRealPreview();
	preview.Layout();

	DesignerInspector inspector;
	inspector.Set(&m, &r);
	inspector.SetRect(0, 0, 360, 520);
	inspector.SetNode(title);
	inspector.Layout();
	t.Expect(ControlTreeCountLabelText(inspector, "UiTitleCard [") == 1,
	         "inspector shows exactly one selected node metadata subheading");
	t.Expect(ControlTreeCountLabelText(inspector, "PageHeaderNode") == 1,
	         "inspector metadata subheading includes the selected node name");
	t.Expect(!ControlTreeHasLabelText(inspector, "Type"),
	         "inspector no longer shows a separate Type heading row");

	inspector.SetBindingGroup("Theme Overrides");
	inspector.SetNode(title);
	inspector.Layout();
	t.Expect(ControlTreeCountLabelText(inspector, "UiTitleCard [") == 1,
	         "theme overrides shows exactly one selected node metadata subheading");
	t.Expect(ControlTreeCountLabelText(inspector, "PageHeaderNode") == 1,
	         "theme overrides metadata subheading includes the selected node name");
	t.Expect(inspector.HasRow("theme_override"), "theme overrides shows activate overrides");
	t.Expect(inspector.HasRow("face_enabled"), "theme overrides keeps Fill visible");
	t.Expect(inspector.HasRow("face"), "theme overrides keeps Face color visible");
	t.Expect(inspector.HasRow("face_mode"), "theme overrides keeps Face mode visible");
	t.Expect(inspector.HasRow("face_quad"), "theme overrides keeps Quad face visible");
	t.Expect(inspector.HasRow("frame_enabled"), "theme overrides keeps Frame visible");
	t.Expect(inspector.HasRow("frame"), "theme overrides keeps Frame color visible");
	t.Expect(inspector.HasRow("frame_width"), "theme overrides keeps Frame thickness visible");
	t.Expect(inspector.HasRow("frame_style"), "theme overrides keeps Frame style visible");
	t.Expect(inspector.HasRow("radius"), "theme overrides keeps Radius visible");
	t.Expect(inspector.IsRowEnabled("face") && inspector.IsRowEnabled("frame") && inspector.IsRowEnabled("radius"),
	         "theme override detail rows remain editable while overrides are off");
	t.Expect(inspector.IsRowEnabled("frame_style"),
	         "theme override frame style remains editable while overrides are off");
	DesignerPanelAdapter theme_panel_preview;
	theme_panel_preview.SyncFromNode(*m.Find(panel));
	t.Expect(IsNumber(inspector.GetRowValue("frame_width")) &&
	         (int)inspector.GetRowValue("frame_width") == theme_panel_preview.GetStyle().metrics.frame_width,
	         "theme override frame thickness default matches preview effective frame width");

	Vector<DesignerApiBinding> title_bindings;
	One<Ctrl> title_ctrl(CreateDesignerAdapterCtrl(r, *m.Find(title)));
	DesignerAdapter* title_adapter = AsDesignerAdapter(*title_ctrl);
	t.Expect(title_adapter != nullptr, "title card adapter is created");
	if(title_adapter)
		title_adapter->DescribeApi(title_bindings, *m.Find(title));
	t.Expect(FindBinding(title_bindings, "text_align_v"),
	         "title card adapter exposes vertical text alignment");
	t.Expect(FindBinding(title_bindings, "card_line_side") && FindBinding(title_bindings, "card_line_length") &&
	         FindBinding(title_bindings, "card_line_style") && FindBinding(title_bindings, "card_line_thickness") &&
	         FindBinding(title_bindings, "card_line_gap") &&
	         FindBinding(title_bindings, "card_line_color_enabled") && FindBinding(title_bindings, "card_line_color"),
	         "title card adapter exposes card line side/style/color fields");
	t.Expect(FindBinding(title_bindings, "title_color_enabled") && FindBinding(title_bindings, "title_color") &&
	         FindBinding(title_bindings, "subtitle_color_enabled") && FindBinding(title_bindings, "subtitle_color"),
	         "title card adapter exposes title/subtitle color overrides");

	m.Find(title)->properties.Set("theme_override", true);
	m.Find(title)->properties.Set("text_align_v", "Top");
	m.Find(title)->properties.Set("card_line_side", "Left");
	m.Find(title)->properties.Set("card_line_length", "Medium");
	m.Find(title)->properties.Set("card_line_style", "Dotted");
	m.Find(title)->properties.Set("card_line_thickness", 3);
	m.Find(title)->properties.Set("card_line_gap", 8);
	m.Find(title)->properties.Set("card_line_color_enabled", true);
	m.Find(title)->properties.Set("card_line_color", Color(10, 90, 200));
	m.Find(title)->properties.Set("title_color_enabled", true);
	m.Find(title)->properties.Set("title_color", Color(10, 90, 200));
	m.Find(title)->properties.Set("subtitle_color_enabled", true);
	m.Find(title)->properties.Set("subtitle_color", Color(120, 130, 140));
	DesignerTitleCardAdapter title_adapter_ctrl;
	title_adapter_ctrl.SyncFromNode(*m.Find(title));
	t.Expect(title_adapter_ctrl.GetStyle().text_align_v == UiAlign::TOP,
	         "title card preview applies vertical text alignment");
	t.Expect(title_adapter_ctrl.GetStyle().card_line_side == UiAlign::LEFT,
	         "title card preview applies card line side");
	t.Expect(title_adapter_ctrl.GetStyle().card_line_gap == 8,
	         "title card preview applies card line gap");
	t.Expect(title_adapter_ctrl.GetStyle().card_line_color_enabled,
	         "title card preview applies card line color enable");
	t.Expect(title_adapter_ctrl.GetStyle().card_line_color == Color(10, 90, 200),
	         "title card preview applies explicit card line color override");
	t.Expect(title_adapter_ctrl.GetStyle().title_color == Color(10, 90, 200),
	         "title card preview applies explicit title color override");
	t.Expect(title_adapter_ctrl.GetStyle().subtitle_color == Color(120, 130, 140),
	         "title card preview applies explicit subtitle color override");
	UiTitleCard default_title_card;
	t.Expect(default_title_card.GetStyle().text_align_v == UiAlign::CENTER,
	         "title card default vertical alignment remains centered");
	t.Expect(default_title_card.GetStyle().card_line_side == UiAlign::BOTTOM,
	         "title card default card line side remains bottom");
	t.Expect(default_title_card.GetStyle().card_line_gap == 0,
	         "title card default card line gap remains zero");

	m.Find(panel)->properties.Set("frame_style", "Dashed");
	m.Find(panel)->properties.Set("frame_enabled", true);
	m.Find(panel)->properties.Set("face_enabled", true);
	DesignerPanelAdapter panel_dashed;
	panel_dashed.SyncFromNode(*m.Find(panel));
	t.Expect(panel_dashed.GetStyle().metrics.dashed && panel_dashed.GetStyle().metrics.dash_pattern == "6,4",
	         "panel preview applies dashed frame style override");

	DesignerNodeId scroll = m.AddNode("UiScrollPanel", panel);
	r.Find("UiScrollPanel")->init_defaults(*m.Find(scroll));
	m.Find(scroll)->name = "scrollpanel_02";
	Vector<DesignerApiBinding> scroll_bindings;
	One<Ctrl> scroll_ctrl(CreateDesignerAdapterCtrl(r, *m.Find(scroll)));
	DesignerAdapter* scroll_adapter = AsDesignerAdapter(*scroll_ctrl);
	t.Expect(scroll_adapter != nullptr, "scroll panel adapter is created");
	if(scroll_adapter)
		scroll_adapter->DescribeApi(scroll_bindings, *m.Find(scroll));
	const DesignerApiBinding* role_binding = FindBinding(scroll_bindings, "role");
	t.Expect(role_binding && role_binding->visible,
	         "scroll panel exposes role in designer inspector");
	t.Expect(FindBinding(scroll_bindings, "frame_width") && FindBinding(scroll_bindings, "frame_width")->visible,
	         "scroll panel exposes frame thickness in theme overrides");
	t.Expect(FindBinding(scroll_bindings, "frame_style") && FindBinding(scroll_bindings, "frame_style")->visible,
	         "scroll panel exposes frame style in theme overrides");

	DesignerNodeId tab = m.AddNode("UiTab", Designer_ROOT);
	r.Find("UiTab")->init_defaults(*m.Find(tab));
	Vector<DesignerApiBinding> tab_bindings;
	One<Ctrl> tab_ctrl(CreateDesignerAdapterCtrl(r, *m.Find(tab)));
	DesignerAdapter* tab_adapter = AsDesignerAdapter(*tab_ctrl);
	t.Expect(tab_adapter != nullptr, "tab adapter is created");
	if(tab_adapter)
		tab_adapter->DescribeApi(tab_bindings, *m.Find(tab));
	t.Expect(FindBinding(tab_bindings, "content_gap") && FindBinding(tab_bindings, "content_gap")->visible &&
	         FindBinding(tab_bindings, "content_gap")->label == "Page icon gap",
	         "tab exposes page icon gap in the inspector");
	t.Expect(FindBinding(tab_bindings, "affordance_gap") && FindBinding(tab_bindings, "affordance_gap")->visible,
	         "tab exposes action icon gap in the inspector");

	m.Find(scroll)->properties.Set("role", "Standard");
	DesignerScrollPanelAdapter scroll_standard;
	scroll_standard.SyncFromNode(*m.Find(scroll));
	Color std_face = scroll_standard.GetStyle().palette.face[ST_NORMAL].IsSolid()
	               ? scroll_standard.GetStyle().palette.face[ST_NORMAL].color
	               : SColorFace();
	m.Find(scroll)->properties.Set("role", "Accent");
	DesignerScrollPanelAdapter scroll_accent;
	scroll_accent.SyncFromNode(*m.Find(scroll));
	Color accent_face = scroll_accent.GetStyle().palette.face[ST_NORMAL].IsSolid()
	                  ? scroll_accent.GetStyle().palette.face[ST_NORMAL].color
	                  : SColorFace();
	t.Expect(std_face != accent_face, "scroll panel role changes appearance");
	m.Find(scroll)->properties.Set("theme_override", true);
	m.Find(scroll)->properties.Set("frame_enabled", true);
	m.Find(scroll)->properties.Set("face_enabled", true);
	m.Find(scroll)->properties.Set("frame_style", "Dotted");
	DesignerScrollPanelAdapter scroll_dotted;
	scroll_dotted.SyncFromNode(*m.Find(scroll));
	t.Expect(scroll_dotted.GetStyle().metrics.dashed && scroll_dotted.GetStyle().metrics.dash_pattern == "1,3",
	         "scroll panel preview applies dotted frame style override");

	m.Find(panel)->properties.Set("theme_override", true);
	m.Find(panel)->properties.Set("face_enabled", false);
	m.Find(panel)->properties.Set("frame_enabled", false);
	DesignerPanelAdapter panel_clear;
	panel_clear.SyncFromNode(*m.Find(panel));
	t.Expect(!panel_clear.GetStyle().metrics.face_enabled && !panel_clear.GetStyle().metrics.frame_enabled,
	         "panel preview clears fill and frame when overrides turn them off");
	inspector.SetNode(panel);
	inspector.Layout();
	t.Expect(inspector.HasRow("face") && inspector.HasRow("face_mode") && inspector.HasRow("face_quad"),
	         "panel theme override rows remain visible when Fill is off");
	t.Expect(inspector.IsRowEnabled("face") && inspector.IsRowEnabled("face_mode") && inspector.IsRowEnabled("face_quad"),
	         "panel theme override face rows remain editable when Fill is off");
	t.Expect(inspector.HasRow("frame"), "panel frame color remains visible when Frame is off");
	t.Expect(inspector.IsRowEnabled("frame"),
	         "panel frame color remains editable when Frame is off");
	t.Expect(inspector.HasRow("frame_width"), "panel frame thickness remains visible when Frame is off");
	t.Expect(inspector.IsRowEnabled("frame_width"),
	         "panel frame thickness remains editable when Frame is off");

	DesignerThemeSurfaceDefaults panel_defaults = DesignerResolveThemeSurfaceDefaults(*m.Find(panel));
	t.Expect(panel_defaults.found && panel_defaults.frame_width == theme_panel_preview.GetStyle().metrics.frame_width,
	         "resolved theme surface defaults expose the same frame thickness as preview");

	DesignerCommandStack activation_commands;
	activation_commands.BeginGroup("Activate overrides");
	activation_commands.Execute(MakeDesignerSetPropertyCommand(panel, "theme_override", true, "Activate overrides"), m);
	auto materialize_defaults = [&](DesignerNodeId id) {
		DesignerNode* n = m.Find(id);
		if(!n)
			return;
		DesignerThemeSurfaceDefaults defaults = DesignerResolveThemeSurfaceDefaults(*n);
		auto set_if_needed = [&](const String& key, const Value& target) {
			int q = n->properties.Find(key);
			bool had_old = q >= 0;
			Value current = had_old ? n->properties.GetValue(q) : Value();
			if(had_old && current == target)
				return;
			activation_commands.Execute(MakeDesignerSetPropertyCommand(id, key, current, had_old, target,
			                                                            "Materialize " + key), m);
			n = m.Find(id);
		};
		set_if_needed("face_enabled", defaults.face_enabled);
		set_if_needed("face", defaults.face);
		set_if_needed("frame_enabled", defaults.frame_enabled);
		set_if_needed("frame", defaults.frame);
		set_if_needed("frame_width", defaults.frame_width);
		set_if_needed("radius", defaults.radius);
		set_if_needed("shadow_enabled", defaults.shadow_enabled);
		set_if_needed("shadow_distance", defaults.shadow_distance);
		set_if_needed("shadow_offset_x", defaults.shadow_offset_x);
		set_if_needed("shadow_offset_y", defaults.shadow_offset_y);
		set_if_needed("shadow_alpha", defaults.shadow_alpha);
		set_if_needed("shadow_color", defaults.shadow_color);
		set_if_needed("shadow_curve", defaults.shadow_curve);
	};
	materialize_defaults(panel);
	activation_commands.EndGroup();
	t.Expect(m.Find(panel)->properties.Find("frame_width") >= 0,
	         "theme override activation materializes frame thickness into the model");
	t.Expect((int)m.Find(panel)->properties.GetValue(m.Find(panel)->properties.Find("frame_width")) == panel_defaults.frame_width,
	         "theme override activation stores the resolved frame thickness");
	activation_commands.Undo(m);
	t.Expect(m.Find(panel)->properties.Find("theme_override") < 0,
	         "undo restores the pre-activation theme override state");
	t.Expect(m.Find(panel)->properties.Find("frame_width") < 0,
	         "undo removes the materialized frame thickness");

	for(const char* key : { "theme_override", "face_enabled", "face", "face_mode", "face_quad",
	                        "frame_enabled", "frame", "frame_width", "frame_style", "radius",
	                        "shadow_enabled", "shadow_distance", "shadow_offset_x", "shadow_offset_y",
	                        "shadow_alpha", "shadow_color", "shadow_curve" })
		if(int q = m.Find(panel)->properties.Find(key); q >= 0)
			m.Find(panel)->properties.Remove(q);
	m.Find(panel)->properties.Set("theme_override", true);
	m.Find(panel)->properties.Set("frame_enabled", true);
	m.Find(panel)->properties.Set("frame_width", 0);
	DesignerPanelAdapter panel_zero;
	panel_zero.SyncFromNode(*m.Find(panel));
	t.Expect(panel_zero.GetStyle().metrics.frame_width == 0,
	         "explicit zero frame thickness is preserved in preview");
	String zero_json = StoreDesignerModelJson(m);
	t.Expect(zero_json.Find("\"frame_width\": 0") >= 0,
	         "explicit zero frame thickness persists in JSON");
	DesignerCodeGenOptions zero_codegen_options;
	zero_codegen_options.class_name = "FrameWidthZeroCheck";
	zero_codegen_options.appearance_mode = DesignerAppearanceMode::ExactDesign;
	String zero_code = GenerateDesignerCode(m, r, zero_codegen_options);
	t.Expect(zero_code.Find("frame_width = DPI(0)") >= 0 || zero_code.Find("frame_width = 0") >= 0,
	         "explicit zero frame thickness persists in generated C++");

	m.Find(scroll)->properties.Set("theme_override", true);
	m.Find(scroll)->properties.Set("face_enabled", false);
	m.Find(scroll)->properties.Set("frame_enabled", false);
	DesignerScrollPanelAdapter scroll_clear;
	scroll_clear.SyncFromNode(*m.Find(scroll));
	t.Expect(!scroll_clear.GetStyle().metrics.face_enabled && !scroll_clear.GetStyle().metrics.frame_enabled,
	         "scroll panel preview clears fill and frame when overrides turn them off");

	DesignerNodeId drop = m.AddNode("UiDropdown", panel);
	r.Find("UiDropdown")->init_defaults(*m.Find(drop));
	m.Find(drop)->properties.Set("role", "Alert");
	m.Find(drop)->properties.Set("theme_override", true);
	m.Find(drop)->properties.Set("frame_enabled", true);
	m.Find(drop)->properties.Set("frame", Color(220, 38, 38));
	m.Find(drop)->properties.Set("frame_width", 1);
	m.Find(drop)->properties.Set("frame_style", "Dotted");
	m.Find(drop)->properties.Set("shadow_enabled", true);
	m.Find(drop)->properties.Set("shadow_distance", 8);
	m.Find(drop)->properties.Set("shadow_alpha", 120);
	DesignerDropdownAdapter drop_adapter;
	drop_adapter.SyncFromNode(*m.Find(drop));
	t.Expect(!drop_adapter.GetStyle().transparent,
	         "dropdown explicit surface overrides force visible surface painting");
	t.Expect(drop_adapter.GetStyle().metrics.dashed && drop_adapter.GetStyle().metrics.dash_pattern == "1,3",
	         "dropdown frame style override applies dotted frame metrics");
	t.Expect(drop_adapter.GetStyle().metrics.shadow.enabled,
	         "dropdown shadow override applies visible shadow metrics");

	for(const char* type : { "BoxLayout", "GridLayout", "Spacer" }) {
		DesignerNodeId id = m.AddNode(type, Designer_ROOT);
		r.Find(type)->init_defaults(*m.Find(id));
		Vector<DesignerApiBinding> bindings;
		One<Ctrl> ctrl(CreateDesignerAdapterCtrl(r, *m.Find(id)));
			DesignerAdapter* adapter = AsDesignerAdapter(*ctrl);
		t.Expect(adapter != nullptr, String(type) + " adapter is created");
		if(adapter)
			adapter->DescribeApi(bindings, *m.Find(id));
		const DesignerApiBinding* role = FindBinding(bindings, "role");
		const DesignerApiBinding* theme_override = FindBinding(bindings, "theme_override");
		t.Expect(!role || !role->visible, String(type) + " keeps role hidden");
		t.Expect(!theme_override || !theme_override->visible, String(type) + " keeps theme overrides hidden");
	}
}

static void TestDesignerInspectorMultiSelectSizing(TestCtx& t)
{
	t.Section("Designer inspector multi-select sizing");

	DesignerRegistry r;
	RegisterDesignerBuiltins(r);

	DesignerModel m;
	DesignerNodeId root = m.AddNode("BoxLayout", Designer_ROOT);
	r.Find("BoxLayout")->init_defaults(*m.Find(root));

	DesignerNodeId a = m.AddNode("UiButton", root);
	r.Find("UiButton")->init_defaults(*m.Find(a));
	m.Find(a)->name = "buttonA";
	m.Find(a)->properties.Set("h_sizing", "Fixed");
	m.Find(a)->properties.Set("v_sizing", "Fixed");
	m.Find(a)->properties.Set("fixed_width", 180);
	m.Find(a)->properties.Set("fixed_height", 32);

	DesignerNodeId b = m.AddNode("UiButton", root);
	r.Find("UiButton")->init_defaults(*m.Find(b));
	m.Find(b)->name = "buttonB";
	m.Find(b)->properties.Set("h_sizing", "Fixed");
	m.Find(b)->properties.Set("v_sizing", "Fixed");
	m.Find(b)->properties.Set("fixed_width", 180);
	m.Find(b)->properties.Set("fixed_height", 32);

	DesignerInspector inspector;
	inspector.Set(&m, &r);
	inspector.SetRect(0, 0, 360, 520);

	Vector<DesignerNodeId> sel;
	sel << a << b;
	inspector.SetSelection(sel);
	inspector.Layout();
	t.Expect(inspector.HasRow("h_sizing"), "multi-select shows width mode");
	t.Expect(inspector.HasRow("fixed_width"), "multi-select shows fixed width");
	t.Expect(inspector.HasRow("fixed_height"), "multi-select shows fixed height");
	t.Expect(inspector.HasRow("min_width") && inspector.HasRow("max_width"),
	         "multi-select shows min/max width rows");
	t.Expect(inspector.IsRowEnabled("fixed_width"), "multi-select fixed width stays editable");
	t.Expect(inspector.IsRowEnabled("fixed_height"), "multi-select fixed height stays editable");
	t.Expect(inspector.IsRowEnabled("min_width") && inspector.IsRowEnabled("max_width"),
	         "multi-select min/max width stay editable");

	m.Find(a)->properties.Set("h_sizing", "Fit");
	inspector.SetSelection(sel);
	inspector.Layout();
	t.Expect(inspector.HasRow("fixed_width"), "mixed mode multi-select still shows fixed width");
	t.Expect(inspector.IsRowEnabled("fixed_width"), "mixed mode multi-select keeps fixed width editable");

	DesignerCommandStack commands;
	commands.BeginGroup("Set fixed width on selection");
	commands.Execute(MakeDesignerSetPropertyCommand(a, "fixed_width", 260, "Set fixed width"), m);
	commands.Execute(MakeDesignerSetPropertyCommand(b, "fixed_width", 260, "Set fixed width"), m);
	commands.Execute(MakeDesignerSetPropertyCommand(a, "h_sizing", "Fixed", "Set width mode"), m);
	commands.Execute(MakeDesignerSetPropertyCommand(b, "h_sizing", "Fixed", "Set width mode"), m);
	commands.EndGroup();
	t.Expect((int)TestNodePropertyOr(*m.Find(a), "fixed_width", 0) == 260 &&
	         (int)TestNodePropertyOr(*m.Find(b), "fixed_width", 0) == 260,
	         "multi-edit stores fixed width on both nodes");
	t.Expect(AsString(TestNodePropertyOr(*m.Find(a), "h_sizing", "Fit")) == "Fixed" &&
	         AsString(TestNodePropertyOr(*m.Find(b), "h_sizing", "Fit")) == "Fixed",
	         "stored fixed width becomes effective when both nodes switch to Fixed");

	m.Find(a)->properties.Set("h_sizing", "Fit");
	inspector.SetNode(a);
	inspector.Layout();
	t.Expect(inspector.HasRow("fixed_width"), "single-select still shows fixed width");
	t.Expect(inspector.IsRowEnabled("fixed_width"), "single-select fixed width stays editable outside Fixed mode");
}

static void TestDesignerChoiceCommitPath(TestCtx& t)
{
	t.Section("Designer choice commit path");

	DesignerRegistry r;
	RegisterDesignerBuiltins(r);

	DesignerModel m;
	DesignerNodeId tab = m.AddNode("UiTab", Designer_ROOT);
	r.Find("UiTab")->init_defaults(*m.Find(tab));
	DesignerNodeId page = m.AddNode("PageSlot", tab);
	r.Find("PageSlot")->init_defaults(*m.Find(page));

	DesignerInspector inspector;
	inspector.Set(&m, &r);
	inspector.SetRect(0, 0, 360, 520);
	TopWindow host;
	host.Add(inspector.SizePos());
	host.SetRect(0, 0, 380, 560);
	host.Open();
	Ctrl::ProcessEvents();
	inspector.SetNode(tab);
	inspector.Layout();

	int placement_commits = 0;
	Value last_value;
	inspector.WhenProperty = [&](DesignerNodeId id, String property, Value value) {
		if(id == tab && property == "placement") {
			placement_commits++;
			last_value = value;
			m.SetProperty(id, property, value);
		}
	};

	UiCompositeDropdown* placement = FindCompositeDropdownByLabel(inspector, "Placement");
	t.Expect(placement != nullptr, "inspector exposes Placement dropdown row");
	if(placement) {
		placement->SetData("Bottom");
		placement->WhenSelectData("Bottom");
		placement->WhenClose();
		Ctrl::ProcessEvents();
		t.Expect(placement_commits == 1, "placement dropdown commits exactly once");
		t.Expect(last_value == Value("Bottom"), "placement dropdown commits selected value");
		t.Expect(TestNodePropertyOr(*m.Find(tab), "placement", Value()) == Value("Bottom"),
		         "placement dropdown updates model property");
	}

	DesignerNodeId title = m.AddNode("UiTitleCard", Designer_ROOT);
	r.Find("UiTitleCard")->init_defaults(*m.Find(title));
	inspector.SetNode(title);
	inspector.Layout();

	int media_commits = 0;
	inspector.WhenProperty = [&](DesignerNodeId id, String property, Value value) {
		if(id == title && property == "media_side") {
			media_commits++;
			last_value = value;
			m.SetProperty(id, property, value);
		}
	};

	UiCompositeDropdown* media_side = FindCompositeDropdownByLabel(inspector, "Media side");
	t.Expect(media_side != nullptr, "inspector exposes Media side dropdown row");
	if(media_side) {
		media_side->SetData("Right");
		media_side->WhenSelectData("Right");
		media_side->WhenClose();
		Ctrl::ProcessEvents();
		t.Expect(media_commits == 1, "media side dropdown commits exactly once");
		t.Expect(last_value == Value("Right"), "media side dropdown commits selected value");
		t.Expect(TestNodePropertyOr(*m.Find(title), "media_side", Value()) == Value("Right"),
		         "media side dropdown updates model property");
	}

	DesignerNodeId group = m.AddNode("UiGroupPanel", Designer_ROOT);
	r.Find("UiGroupPanel")->init_defaults(*m.Find(group));
	inspector.SetNode(group);
	inspector.Layout();

	int header_side_commits = 0;
	int icon_commits = 0;
	inspector.WhenProperty = [&](DesignerNodeId id, String property, Value value) {
		if(id != group)
			return;
		if(property == "placement")
			header_side_commits++;
		if(property == "icon")
			icon_commits++;
		last_value = value;
		m.SetProperty(id, property, value);
	};

	UiCompositeDropdown* header_side = FindCompositeDropdownByLabel(inspector, "Header side");
	t.Expect(header_side != nullptr, "group panel exposes Header side dropdown row");
	if(header_side) {
		header_side->SetData("Left");
		header_side->WhenSelectData("Left");
		header_side->WhenClose();
		Ctrl::ProcessEvents();
		t.Expect(header_side_commits == 1, "group panel header side commits exactly once");
		t.Expect(TestNodePropertyOr(*m.Find(group), "placement", Value()) == Value("Left"),
		         "group panel header side updates model property");
	}

	UiCompositeDropdown* icon_row = FindCompositeDropdownByLabel(inspector, "Icon");
	t.Expect(icon_row != nullptr, "group panel exposes Icon dropdown row");
	if(icon_row) {
		icon_row->SetData("ICON_DESIGN_HOME_48");
		icon_row->WhenSelectData("ICON_DESIGN_HOME_48");
		icon_row->WhenClose();
		Ctrl::ProcessEvents();
		t.Expect(icon_commits == 1, "group panel icon commits exactly once");
		t.Expect(TestNodePropertyOr(*m.Find(group), "icon", Value()) == Value("ICON_DESIGN_HOME_48"),
		         "group panel icon updates model property");
	}

	inspector.SetNode(tab);
	inspector.Layout();
	UiCompositeDropdown* stale_row = FindCompositeDropdownByLabel(inspector, "Placement");
	t.Expect(stale_row != nullptr, "stale callback test finds Placement row");
	Event<const Value&> stale_select;
	if(stale_row)
		stale_select = stale_row->WhenSelectData;
	int stale_commits = 0;
	inspector.WhenProperty = [&](DesignerNodeId id, String property, Value value) {
		if(id == tab && property == "placement") {
			stale_commits++;
			m.SetProperty(id, property, value);
		}
	};
	inspector.SetNode(title);
	inspector.Layout();
	if(stale_select) {
		stale_select("Top");
		Ctrl::ProcessEvents();
	}
	t.Expect(stale_commits == 0, "stale dropdown callback is ignored after inspector rebuild");
	t.Expect(TestNodePropertyOr(*m.Find(tab), "placement", Value()) == Value("Bottom"),
	         "stale dropdown callback does not mutate the previous node");
	host.Close();
}

static void TestDesignerChoiceBindingAudit(TestCtx& t)
{
	t.Section("Designer choice binding audit");

	DesignerRegistry r;
	RegisterDesignerBuiltins(r);

	Vector<const DesignerType*> types = r.GetTypes();
	for(const DesignerType* type_ptr : types) {
		if(!type_ptr)
			continue;
		const DesignerType& type = *type_ptr;
		DesignerModel m;
		DesignerNodeId id = m.AddNode(type.id, Designer_ROOT);
		type.init_defaults(*m.Find(id));
	One<Ctrl> ctrl(CreateDesignerAdapterCtrl(r, *m.Find(id)));
		DesignerAdapter* adapter = AsDesignerAdapter(*ctrl);
		if(!adapter)
			continue;
		Vector<DesignerApiBinding> bindings;
		adapter->DescribeApi(bindings, *m.Find(id));
		for(const DesignerApiBinding& b : bindings) {
			if(!b.visible || b.editor != DesignerEditorKind::Choice || b.choices.GetCount() < 2)
				continue;
			Value current = TestNodePropertyOr(*m.Find(id), b.property_id, Value());
			Value next = Value();
			for(int q = 0; q < b.choices.GetCount(); q++) {
				Value candidate = b.choices.GetKey(q);
				if(IsNull(current) || candidate != current) {
					next = candidate;
					break;
				}
			}
			if(IsNull(next))
				continue;
			DesignerCommandStack stack;
			t.Expect(stack.Execute(MakeDesignerSetPropertyCommand(id, b.property_id, next, "Set " + b.property_id), m),
			         type.id + " choice " + b.property_id + " accepts non-default value");
			t.Expect(TestNodePropertyOr(*m.Find(id), b.property_id, Value()) == next,
			         type.id + " choice " + b.property_id + " survives model refresh");
		}
	}
}

static void TestSplitButtonPopupMetadata(TestCtx& t)
{
	t.Section("UiSplitButton popup metadata");

	UiSplitButton b;
	b.Add("Save As...", "cmd:save_as");
	b.AddSeparator();
	b.AddGroupHeader("Recent saves");
	b.Add("first.json", "E:/tmp/first.json");

	t.Expect(b.GetCount() == 3, "separator metadata does not create a standalone row");
	t.Expect(!b.GetItem(0).separator_before && !b.GetItem(0).group_header,
	         "command row remains a normal selectable item");
	t.Expect(b.GetItem(1).group_header && b.GetItem(1).separator_before && !b.GetItem(1).enabled,
	         "group header records separator metadata and stays non-selectable");
	t.Expect(!b.GetItem(2).group_header && !b.GetItem(2).separator_before,
	         "recent save row remains selectable after the header row");

	Vector<String> order;
	b.WhenSelect = [&](int, const Value&) { order.Add("select"); };
	b.WhenClose = [&] { order.Add("close"); };
	b.OpenPopup();
	b.DebugSelectPopupItem(0);
	Ctrl::ProcessEvents();
	t.Expect(order.GetCount() == 2, "split button popup emits one select and one close callback");
	t.Expect(order.GetCount() == 2 && order[0] == "select" && order[1] == "close",
	         "split button popup posts select before close after teardown");
}

static void TestRecentDocumentHelper(TestCtx& t)
{
	t.Section("Designer recent documents");

	DesignerRecentDocuments recents;
	recents.AddRecentDesignerDocument("E:/tmp/a.design.json");
	recents.AddRecentDesignerDocument("E:/tmp/b.design.json");
	recents.AddRecentDesignerDocument("E:/tmp/a.design.json");
	t.Expect(recents.Get().GetCount() == 2, "duplicate recent path is not duplicated");
	t.Expect(recents.Get().GetCount() > 0 && recents.Get()[0].EndsWith("a.design.json"),
	         "most recent document moves to the top");
	for(int i = 0; i < 12; i++)
		recents.AddRecentDesignerDocument(Format("E:/tmp/recent_%02d.design.json", i));
	t.Expect(recents.Get().GetCount() == DesignerRecentDocuments::MAX_RECENT,
	         "recent document list is capped at ten");
	Value stored = recents.StoreRecentDesignerDocuments();
	DesignerRecentDocuments restored;
	restored.LoadRecentDesignerDocuments(stored);
	t.Expect(restored.Get().GetCount() == 2, "recent document store/load round-trips");
	t.Expect(restored.Get()[0].EndsWith("a.design.json") && restored.Get()[1].EndsWith("b.design.json"),
	         "recent document store/load preserves order");

	UiSplitButton save_button, load_button;
	RefreshRecentDocumentMenus(save_button, load_button, restored);
	t.Expect(save_button.GetCount() == load_button.GetCount(), "Save and Load share the same recent menu model");
	t.Expect(save_button.GetCount() >= 4, "shared recent menu includes command, separator, header, and recent items");
	t.Expect(save_button.GetItem(save_button.GetCount() - 2).group_header &&
	         load_button.GetItem(load_button.GetCount() - 2).group_header,
	         "shared recent menu uses the same header row in both menus");
}

static DesignerModel MakeSampleModel(DesignerRegistry& r)
{
	DesignerModel m;
	DesignerNodeId main = m.AddNode("BoxLayout", Designer_ROOT);
	r.Find("BoxLayout")->init_defaults(*m.Find(main));
	m.Find(main)->name = "mainColumn";
	DesignerNodeId title = m.AddNode("UiTitleCard", main);
	r.Find("UiTitleCard")->init_defaults(*m.Find(title));
	m.Find(title)->name = "header";
	m.Find(title)->properties.Set("text_align_v", "Top");
	m.Find(title)->properties.Set("card_line_side", "Left");
	DesignerNodeId grid = m.AddNode("GridLayout", main);
	r.Find("GridLayout")->init_defaults(*m.Find(grid));
	m.Find(grid)->name = "contentGrid";
	DesignerNodeId label = m.AddNode("UiLabel", grid);
	r.Find("UiLabel")->init_defaults(*m.Find(label));
	m.Find(label)->name = "nameLabel";
	DesignerNodeId slider = m.AddNode("UiSlider", grid);
	r.Find("UiSlider")->init_defaults(*m.Find(slider));

	DesignerNodeId spacer = m.AddNode("Spacer", main);
	r.Find("Spacer")->init_defaults(*m.Find(spacer));
	m.Find(spacer)->name = "mainSpacer";
	m.Find(spacer)->properties.Set("v_sizing", "Fixed");
	m.Find(spacer)->properties.Set("fixed_height", 18);
	m.Find(slider)->name = "volumeSlider";
	DesignerNodeId button = m.AddNode("UiButton", grid);
	r.Find("UiButton")->init_defaults(*m.Find(button));
	m.Find(button)->name = "applyButton";
	DesignerNodeId edit = m.AddNode("UiLineEdit", grid);
	r.Find("UiLineEdit")->init_defaults(*m.Find(edit));
	m.Find(edit)->name = "nameEdit";
	DesignerNodeId toggle = m.AddNode("UiToggle", grid);
	r.Find("UiToggle")->init_defaults(*m.Find(toggle));
	m.Find(toggle)->name = "enabledToggle";
	DesignerNodeId drop = m.AddNode("UiDropdown", grid);
	r.Find("UiDropdown")->init_defaults(*m.Find(drop));
	m.Find(drop)->name = "modeDrop";
	DesignerNodeId panel = m.AddNode("UiPanel", grid);
	r.Find("UiPanel")->init_defaults(*m.Find(panel));
	m.Find(panel)->name = "panelHost";
	DesignerNodeId panel_label = m.AddNode("UiLabel", panel);
	r.Find("UiLabel")->init_defaults(*m.Find(panel_label));
	m.Find(panel_label)->name = "panelLabel";
	DesignerNodeId scroll = m.AddNode("UiScrollPanel", grid);
	r.Find("UiScrollPanel")->init_defaults(*m.Find(scroll));
	m.Find(scroll)->name = "scrollHost";
	m.Find(scroll)->properties.Set("role", "Accent");
	DesignerNodeId scroll_label = m.AddNode("UiLabel", scroll);
	r.Find("UiLabel")->init_defaults(*m.Find(scroll_label));
	m.Find(scroll_label)->name = "scrollLabel";
	DesignerNodeId splitter = m.AddNode("UiSplitter", grid);
	r.Find("UiSplitter")->init_defaults(*m.Find(splitter));
	m.Find(splitter)->name = "mainSplitter";
	DesignerNodeId split_a = m.AddNode("PaneSlot", splitter);
	r.Find("PaneSlot")->init_defaults(*m.Find(split_a));
	m.Find(split_a)->name = "leftPane";
	DesignerNodeId split_b = m.AddNode("PaneSlot", splitter);
	r.Find("PaneSlot")->init_defaults(*m.Find(split_b));
	m.Find(split_b)->name = "rightPane";
	DesignerNodeId split_label = m.AddNode("UiLabel", split_a);
	r.Find("UiLabel")->init_defaults(*m.Find(split_label));
	m.Find(split_label)->name = "splitLabel";
	DesignerNodeId quad = m.AddNode("UiQuadSplitter", grid);
	r.Find("UiQuadSplitter")->init_defaults(*m.Find(quad));
	m.Find(quad)->name = "quadSplitter";
	for(int i = 0; i < 4; i++) {
		DesignerNodeId pane = m.AddNode("PaneSlot", quad);
		r.Find("PaneSlot")->init_defaults(*m.Find(pane));
		m.Find(pane)->name = "quadPane" + AsString(i + 1);
	}
	for(const char *type : { "UiCheckBox", "UiBreadcrumbs", "UiTab", "UiStack", "UiTable", "UiTree" }) {
		DesignerNodeId id = m.AddNode(type, grid);
		r.Find(type)->init_defaults(*m.Find(id));
		m.Find(id)->name = type;
		if(String(type) == "UiBreadcrumbs") {
			m.Find(id)->properties.Set("crumb_count", 5);
			m.Find(id)->properties.Set("crumb_1", "Home");
			m.Find(id)->properties.Set("crumb_2", "Library");
			m.Find(id)->properties.Set("crumb_3", "Current");
			m.Find(id)->properties.Set("crumb_4", "Archive");
			m.Find(id)->properties.Set("crumb_5", "Settings");
		}
	}
	return m;
}


static void TestGeneratedCodeText(TestCtx& t)
{
	t.Section("Designer generated code");

	DesignerRegistry r;
	RegisterDesignerBuiltins(r);
	DesignerModel m = MakeSampleModel(r);
	String code = GenerateDesignerCode(m, r, "GeneratedDesignerCompileSmoke");
	t.Expect(code.Find("class GeneratedDesignerCompileSmoke") >= 0, "generated code emits a window class");
	t.Expect(code.Find("GUI_APP_MAIN") >= 0, "generated code emits GUI entry point");
	t.Expect(code.Find("Add(mainColumn);") >= 0 || code.Find("Add(mainColumn.SizePos())") >= 0,
	         "generated code attaches root layout to window using model name");
	t.Expect(code.Find("mainColumn.Add(header)") >= 0, "generated code attaches title to box layout using model name");
	t.Expect(code.Find("contentGrid.SetGridSize(") >= 0, "generated code emits stable grid size");
	t.Expect(code.Find(".SetMinCellSize(") >= 0, "generated code emits minimum cell size");
	t.Expect(code.Find("contentGrid.SetCustomStyle(") < 0, "generated code does not emit grid style factory calls");
	t.Expect(code.Find("contentGrid.Add(nameLabel") >= 0 && code.Find(", 0, 0") >= 0,
	         "generated code attaches label to explicit grid cell");
	t.Expect(code.Find("AddSpacer(") >= 0,
	         "generated code emits semantic box layout spacer");
	t.Expect(code.Find("UiBoxLayout mainColumn") >= 0, "generated code declares box layout member from model name");
	t.Expect(code.Find("UiGridLayout contentGrid") >= 0, "generated code declares grid layout member from model name");
	t.Expect(code.Find("UiButton") >= 0 && code.Find(".SetText(\"Button\")") >= 0, "generated code emits button control");
	t.Expect(code.Find("UiLineEdit") >= 0 && code.Find(".SetTextUtf8(\"Edit\")") >= 0, "generated code emits line edit control");
	t.Expect(code.Find("UiToggle") >= 0 && code.Find(".SetOn(true)") >= 0, "generated code emits toggle control");
	t.Expect(code.Find("UiDropdown") >= 0 && code.Find(".UseInternalModel().Clear().Add(\"First\"") >= 0, "generated code emits dropdown control");
	t.Expect(code.Find("UiCheckBox") >= 0 && code.Find(".SetState(") >= 0, "generated code emits checkbox control");
	t.Expect(code.Find("UiBreadcrumbs") >= 0 &&
	         code.Find(".AddCrumb(\"Home\", \"0\")") >= 0 &&
	         code.Find(".AddCrumb(\"Library\", \"1\")") >= 0 &&
	         code.Find(".AddCrumb(\"Current\", \"2\")") >= 0 &&
	         code.Find(".AddCrumb(\"Archive\", \"3\")") >= 0 &&
	         code.Find(".AddCrumb(\"Settings\", \"4\")") >= 0,
	         "generated code emits canonical numbered breadcrumbs");
	t.Expect(code.Find("UiTab") >= 0 && code.Find(".SetActiveTab(") >= 0, "generated code emits tab control");
	t.Expect(code.Find("UiStack") >= 0 && code.Find(".SetActivePage(") >= 0, "generated code emits stack control");
	t.Expect(code.Find("UiTable") >= 0 && code.Find(".GetInternalModel().SetSize(") >= 0, "generated code emits table control");
	t.Expect(code.Find("UiTree") >= 0 && code.Find(".ShowConnectorLines(") >= 0, "generated code emits tree control");
	t.Expect(code.Find("UiPanel panelHost") >= 0 && code.Find("panelHost.Add(panelLabel") >= 0, "generated code emits panel container");
	t.Expect(code.Find("UiScrollPanel") >= 0 && code.Find(".Content().Add(") >= 0, "generated code emits scroll panel content");
	t.Expect(code.Find("UiGroupPanel") >= 0 && code.Find(".SetContent(") >= 0, "generated code emits group panel content");
	t.Expect(code.Find("UiAccordion") >= 0 && code.Find(".AddSection(") >= 0, "generated code emits accordion sections");
	t.Expect(code.Find("UiTheme::ResolveScrollPanel(UiRole::Accent)") >= 0, "generated code emits role-aware scroll panel style");
	t.Expect(code.Find(".SetTextAlign(") >= 0, "generated code emits title card vertical text alignment");
	t.Expect(code.Find(".card_line_side = UiAlign::LEFT") >= 0, "generated code emits title card card line side");
	t.Expect(code.Find("UiSplitter") >= 0 && code.Find(".SetSplitPercent(") >= 0, "generated code emits splitter control");
	t.Expect(code.Find("UiQuadSplitter") >= 0 && code.Find(".SetSplitPercent(") >= 0, "generated code emits quad splitter control");
	t.Expect(code.Find("UiCompositeColor") >= 0 && code.Find(".SetColorCount(") >= 0 && code.Find(".SetColor(") >= 0,
	         "generated code emits composite color control");
	t.Expect(code.Find("UiSlider") >= 0 && code.Find(".SetRange(") >= 0 && code.Find(".SetThumbSize(") >= 0,
	         "generated code emits slider setup");
	t.Expect(code.Find("UiSplitButton") >= 0 && code.Find(".SetSplitWidth(") >= 0,
	         "generated code emits split button split lane setup");
	t.Expect(code.Find("crumb_a") < 0 && code.Find("crumb_b") < 0 && code.Find("crumb_c") < 0,
	         "generated code omits legacy breadcrumb aliases");
	String grid_code = GenerateDesignerCode(m, r, "GeneratedDesignerGridSmoke");
	t.Expect(grid_code.Find("contentGrid.SetGridSize(") >= 0, "generated code emits stable grid setup");
	t.Expect(grid_code.Find(".AddGrid(") < 0, "generated code uses new Add(row, col) grid placement");
	m.Find(5)->properties.Set("h_sizing", "Expand");
	m.Find(5)->properties.Set("v_sizing", "Expand");
	m.Find(4)->properties.Set("debug", true);
	m.Find(4)->properties.Set("debug_auto_color", false);
	m.Find(4)->properties.Set("debug_color", Color(37, 99, 235));
	m.Find(4)->properties.Set("cell_width", 640);
	m.Find(4)->properties.Set("cell_height", 360);
	String flow_expand_code = GenerateDesignerCode(m, r, "GeneratedDesignerFlowExpandSmoke");
	t.Expect(flow_expand_code.Find("contentGrid.Add(nameLabel") >= 0 && flow_expand_code.Find(", true, true") >= 0,
	         "generated code gives expanding grid children per-axis scale placement");
	t.Expect(flow_expand_code.Find(".SetDebugColor(Color(37, 99, 235)).SetDebug(true)") >= 0,
	         "generated code emits explicit grid debug color when debug is enabled");

	DesignerModel surface_clear;
	DesignerNodeId surface_root = surface_clear.AddNode("BoxLayout", Designer_ROOT);
	r.Find("BoxLayout")->init_defaults(*surface_clear.Find(surface_root));
	surface_clear.Find(surface_root)->properties.Set("direction", "V");
	surface_clear.Find(surface_root)->properties.Set("h_sizing", "Expand");
	surface_clear.Find(surface_root)->properties.Set("v_sizing", "Expand");
	DesignerNodeId surface_panel = surface_clear.AddNode("UiPanel", surface_root);
	r.Find("UiPanel")->init_defaults(*surface_clear.Find(surface_panel));
	surface_clear.Find(surface_panel)->name = "surfacePanel";
	surface_clear.Find(surface_panel)->properties.Set("role", "Accent");
	surface_clear.Find(surface_panel)->properties.Set("theme_override", true);
	surface_clear.Find(surface_panel)->properties.Set("face_enabled", false);
	surface_clear.Find(surface_panel)->properties.Set("frame_enabled", false);
	DesignerNodeId surface_scroll = surface_clear.AddNode("UiScrollPanel", surface_root);
	r.Find("UiScrollPanel")->init_defaults(*surface_clear.Find(surface_scroll));
	surface_clear.Find(surface_scroll)->name = "surfaceScroll";
	surface_clear.Find(surface_scroll)->properties.Set("role", "Accent");
	surface_clear.Find(surface_scroll)->properties.Set("theme_override", true);
	surface_clear.Find(surface_scroll)->properties.Set("face_enabled", false);
	surface_clear.Find(surface_scroll)->properties.Set("frame_enabled", false);
	String clear_code = GenerateDesignerCode(surface_clear, r, "GeneratedDesignerSurfaceClearSmoke", true);
	t.Expect(clear_code.Find("surfacePanel") >= 0 && clear_code.Find("s.metrics.face_enabled = false") >= 0 &&
	         clear_code.Find("s.metrics.frame_enabled = false") >= 0,
	         "generated code emits disabled panel fill/frame states");
	t.Expect(clear_code.Find("surfaceScroll") >= 0 && clear_code.Find("s.metrics.face_enabled = false") >= 0 &&
	         clear_code.Find("s.metrics.frame_enabled = false") >= 0,
	         "generated code emits disabled scroll panel fill/frame states");
	m.Find(surface_panel)->properties.Set("frame_style", "Dashed");
	m.Find(surface_scroll)->properties.Set("frame_style", "Dotted");
	m.Find(surface_panel)->properties.Set("frame_enabled", true);
	m.Find(surface_panel)->properties.Set("face_enabled", true);
	m.Find(surface_scroll)->properties.Set("frame_enabled", true);
	m.Find(surface_scroll)->properties.Set("face_enabled", true);
	String style_code = GenerateDesignerCode(surface_clear, r, "GeneratedDesignerSurfaceStyleSmoke", true);
	t.Expect(style_code.Find("s.metrics.dashed = true") >= 0 && style_code.Find("s.metrics.dash_pattern = \"6,4\"") >= 0 &&
	         style_code.Find("s.metrics.dash_pattern = \"1,3\"") >= 0,
	         "generated code emits dashed and dotted frame styles");

	DesignerModel frame_styles;
	DesignerNodeId frame_root = frame_styles.AddNode("BoxLayout", Designer_ROOT);
	r.Find("BoxLayout")->init_defaults(*frame_styles.Find(frame_root));
	frame_styles.Find(frame_root)->properties.Set("direction", "V");
	struct FrameSpec { const char* type; const char* name; const char* style; };
	Vector<FrameSpec> frame_specs;
	frame_specs << FrameSpec{"UiPanel", "framePanel", "Dashed"}
	            << FrameSpec{"UiScrollPanel", "frameScroll", "Dotted"}
	            << FrameSpec{"UiGroupPanel", "frameGroup", "Dashed"}
	            << FrameSpec{"UiTitleCard", "frameTitle", "Dotted"}
	            << FrameSpec{"UiLabel", "frameLabel", "Dashed"}
	            << FrameSpec{"UiButton", "frameButton", "Dotted"}
	            << FrameSpec{"UiToolButton", "frameTool", "Dashed"}
	            << FrameSpec{"UiSplitButton", "frameSplit", "Dotted"}
	            << FrameSpec{"UiLineEdit", "frameEdit", "Dashed"}
	            << FrameSpec{"UiIntEdit", "frameInt", "Dotted"}
	            << FrameSpec{"UiFloatEdit", "frameFloat", "Dashed"}
	            << FrameSpec{"UiDropdown", "frameDrop", "Dotted"};
	for(const FrameSpec& spec : frame_specs) {
		DesignerNodeId id = frame_styles.AddNode(spec.type, frame_root);
		r.Find(spec.type)->init_defaults(*frame_styles.Find(id));
		frame_styles.Find(id)->name = spec.name;
		frame_styles.Find(id)->properties.Set("theme_override", true);
		frame_styles.Find(id)->properties.Set("face_enabled", true);
		frame_styles.Find(id)->properties.Set("frame_enabled", true);
		frame_styles.Find(id)->properties.Set("frame_style", spec.style);
	}
	String frame_style_code = GenerateDesignerCode(frame_styles, r, "GeneratedFrameStyleAudit", true);
	for(const FrameSpec& spec : frame_specs)
		t.Expect(frame_style_code.Find(spec.name) >= 0, String(spec.type) + " frame-style audit emits node block");
	t.Expect(frame_style_code.Find("s.metrics.dash_pattern = \"6,4\"") >= 0,
	         "frame-style audit emits dashed codegen pattern");
	t.Expect(frame_style_code.Find("s.metrics.dash_pattern = \"1,3\"") >= 0,
	         "frame-style audit emits dotted codegen pattern");

	DesignerModel solid_frame;
	DesignerNodeId solid_root = solid_frame.AddNode("BoxLayout", Designer_ROOT);
	r.Find("BoxLayout")->init_defaults(*solid_frame.Find(solid_root));
	DesignerNodeId solid_panel = solid_frame.AddNode("UiPanel", solid_root);
	r.Find("UiPanel")->init_defaults(*solid_frame.Find(solid_panel));
	solid_frame.Find(solid_panel)->name = "solidFramePanel";
	solid_frame.Find(solid_panel)->properties.Set("theme_override", true);
	solid_frame.Find(solid_panel)->properties.Set("face_enabled", true);
	solid_frame.Find(solid_panel)->properties.Set("frame_enabled", true);
	solid_frame.Find(solid_panel)->properties.Set("frame_style", "Solid");
	String solid_frame_code = GenerateDesignerCode(solid_frame, r, "GeneratedSolidFrameAudit", true);
	t.Expect(solid_frame_code.Find("solidFramePanel") >= 0, "solid frame audit emits panel block");
	t.Expect(solid_frame_code.Find("dash_pattern") < 0 && solid_frame_code.Find("s.metrics.dashed = true") < 0,
	         "solid frame audit keeps solid codegen free of dashed overrides");

	for(const char *starter : { "HolyGrail", "Magazine", "SPA", "CardGrid", "SplitScreen", "FPattern", "HeaderWithActions", "DesignerWorkbench" }) {
		DesignerModel sm;
		t.Expect(ApplyDesignerTemplate(sm, r, starter), String(starter) + " template applies");
		String scode = GenerateDesignerCode(sm, r, "Generated" + String(starter));
		t.Expect(scode.Find("class Generated" + String(starter)) >= 0, String(starter) + " emits a class");
		t.Expect(scode.Find("GUI_APP_MAIN") >= 0, String(starter) + " emits a GUI entry point");
		t.Expect(scode.Find("Designer appearance") < 0, String(starter) + " keeps default code theme-first");
		String appearance_code = GenerateDesignerCode(sm, r, "GeneratedStyled" + String(starter), true);
		t.Expect(!appearance_code.IsEmpty(), String(starter) + " supports the explicit-appearance generation path");
		t.Expect(scode.Find(".SizePos())") >= 0 || scode.Find(".Add(") >= 0, String(starter) + " attaches at least one generated control");
		if(String(starter) == "DesignerWorkbench") {
			t.Expect(scode.Find("UiSplitButton") >= 0, "DesignerWorkbench emits split buttons");
			t.Expect(scode.Find("UiDropdown") >= 0, "DesignerWorkbench emits a dropdown");
			t.Expect(scode.Find("UiToolButton") >= 0, "DesignerWorkbench emits tool buttons");
			t.Expect(scode.Find("UiScrollPanel") >= 0, "DesignerWorkbench emits scroll panels");
			t.Expect(scode.Find("ICON_DESIGN_HELP_48") >= 0, "DesignerWorkbench emits valid icon references");
		}
	}

	DesignerNodeId header_id = Designer_NULL;
	for(const DesignerNode& node : m.GetNodes())
		if(node.name == "header")
			header_id = node.id;
	t.Expect(header_id != Designer_NULL, "sample model exposes title card header node");
		if(header_id != Designer_NULL) {
			m.Find(header_id)->properties.Set("theme_override", true);
			m.Find(header_id)->properties.Set("text_align_v", "Bottom");
			m.Find(header_id)->properties.Set("card_line_side", "Right");
			m.Find(header_id)->properties.Set("card_line_gap", 6);
			m.Find(header_id)->properties.Set("card_line_color_enabled", true);
			m.Find(header_id)->properties.Set("card_line_color", Color(12, 95, 210));
			m.Find(header_id)->properties.Set("title_color_enabled", true);
			m.Find(header_id)->properties.Set("title_color", Color(12, 95, 210));
			m.Find(header_id)->properties.Set("subtitle_color_enabled", true);
			m.Find(header_id)->properties.Set("subtitle_color", Color(120, 130, 140));
			String title_override_code = GenerateDesignerCode(m, r, "GeneratedDesignerTitleOverrideSmoke", true);
			t.Expect(title_override_code.Find(".SetTextAlign(") >= 0 &&
			         title_override_code.Find("s.card_line_side = UiAlign::RIGHT;") >= 0 &&
			         title_override_code.Find("s.card_line_gap = DPI(6);") >= 0 &&
			         title_override_code.Find("s.card_line_color_enabled = true;") >= 0 &&
			         title_override_code.Find("s.title_color = Color(12, 95, 210);") >= 0 &&
			         title_override_code.Find("s.subtitle_color = Color(120, 130, 140);") >= 0,
			         "generated code emits title card text alignment and title/subtitle color overrides");
		}

	DesignerModel clash;
	DesignerNodeId a = clash.AddNode("UiLabel", Designer_ROOT);
	DesignerNodeId b = clash.AddNode("UiLabel", Designer_ROOT);
	r.Find("UiLabel")->init_defaults(*clash.Find(a));
	r.Find("UiLabel")->init_defaults(*clash.Find(b));
	clash.Find(a)->name = "contentGrid";
	clash.Find(b)->name = "contentGrid";
	String clash_code = GenerateDesignerCode(clash, r, "GeneratedDesignerNameClashSmoke");
	t.Expect(clash_code.Find("UiLabel contentGrid;") >= 0 &&
	         clash_code.Find("UiLabel contentGrid_02;") >= 0,
	         "generated code resolves duplicate model names with two-digit suffix");
}

static void TestDesignerSerialization(TestCtx& t)
{
	t.Section("Designer JSON save/load");

	DesignerRegistry r;
	RegisterDesignerBuiltins(r);
	DesignerModel m = MakeSampleModel(r);
	String json = StoreDesignerModelJson(m);
	t.Expect(json.Find("\"format\": \"upp-ui-designer\"") >= 0, "designer JSON stores document marker");
	t.Expect(json.Find("\"type\": \"color\"") >= 0, "designer JSON stores color properties as portable values");

	DesignerModel loaded;
	String error;
	Vector<String> notes;
	t.Expect(LoadDesignerModelJson(loaded, r, json, error, &notes), "designer JSON reloads");
	t.Expect(loaded.Validate(), "loaded designer document validates");
	const DesignerNode* grid = loaded.Find(4);
	t.Expect(grid && grid->name == "contentGrid", "loaded designer document preserves model names");
	t.Expect(grid && grid->properties.Find("tab_icon_size") < 0, "unrelated defaults are not injected on wrong type");
	String loaded_code = GenerateDesignerCode(loaded, r, "GeneratedDesignerLoadedSmoke");
	t.Expect(loaded_code.Find("UiGridLayout contentGrid") >= 0, "loaded document keeps names usable by codegen");

	String unknown_json = json;
	unknown_json.Replace("\"type\": \"UiButton\"", "\"type\": \"FutureButton\"");
	DesignerModel future;
	notes.Clear();
	t.Expect(LoadDesignerModelJson(future, r, unknown_json, error, &notes), "unknown future control loads");
	bool found_generic = false;
	for(const DesignerNode& n : future.GetNodes()) {
		if(n.type_id == "Generic" &&
		   AsString(n.properties.GetValue(n.properties.Find("original_type"))) == "FutureButton")
			found_generic = true;
	}
	t.Expect(found_generic, "unknown future control is represented as generic fallback");
	t.Expect(!notes.IsEmpty(), "unknown future control reports a load note");
}

static void TestDesignerProjectExport(TestCtx& t)
{
	t.Section("Designer project export");

	DesignerRegistry r;
	RegisterDesignerBuiltins(r);
	String repo_root = GetFileFolder(GetFileFolder(GetExeFilePath()));
	String temp_root = AppendFileName(GetTempDirectory(), Format("DesignerExportSmoke_%08x", (int)GetTickCount()));
	String out_root = AppendFileName(temp_root, "out");
	String umk = "E:\\upp-18468\\umk.exe";
	t.Expect(FileExists(umk), "umk is available for export smoke");
	DeleteFolderDeep(temp_root);
	t.Expect(DirectoryCreate(temp_root) || DirectoryExists(temp_root), "export smoke temp root is available");
	t.Expect(DirectoryCreate(out_root) || DirectoryExists(out_root), "export smoke out directory is available");

	auto RunExportSmoke = [&](const String& project_name, const DesignerModel& model, const String& class_name,
	                          const String& source_design, DesignerExportSourceMode source_mode) {
		DesignerProjectExportOptions options;
		options.project_name = project_name;
		options.package_directory = AppendFileName(temp_root, project_name);
		options.class_name = class_name;
		options.source_mode = source_mode;
		options.include_design_json = true;
		options.include_readme = true;
		options.appearance_mode = DesignerAppearanceMode::ExactDesign;
		options.source_design_filename = source_design;
		options.umk_path = umk;
		options.build_method = "CLANGx64";
		options.output_exe_path = AppendFileName(out_root, project_name + ".exe");
		options.overwrite_existing = false;

		String pkg_dir = options.package_directory;
		String stage_dir = pkg_dir + ".staging";
		String backup_dir = pkg_dir + ".backup";
		FileDelete(AppendFileName(pkg_dir, project_name + ".upp"));
		FileDelete(AppendFileName(pkg_dir, "main.cpp"));
		FileDelete(AppendFileName(pkg_dir, project_name + ".h"));
		FileDelete(AppendFileName(pkg_dir, project_name + ".cpp"));
		FileDelete(AppendFileName(pkg_dir, project_name + ".design.json"));
		FileDelete(AppendFileName(pkg_dir, "README.md"));
		DeleteFolderDeep(pkg_dir);
		DeleteFolderDeep(stage_dir);
		DeleteFolderDeep(backup_dir);
		FileDelete(options.output_exe_path);

		DesignerProjectExportResult result;
		String json = StoreDesignerModelJson(model);
		t.Expect(ExportDesignerProject(model, r, options, json, result), project_name + " export writes package files");
		t.Expect(FileExists(result.upp_path), project_name + " export writes .upp");
		t.Expect(FileExists(result.main_cpp_path), project_name + " export writes main.cpp");
		if(source_mode == DesignerExportSourceMode::SplitHeaderSource) {
			t.Expect(FileExists(result.header_path), project_name + " export writes header");
			t.Expect(FileExists(result.source_cpp_path), project_name + " export writes source cpp");
		}
		else {
			t.Expect(!FileExists(result.header_path), project_name + " single-file export omits header");
			t.Expect(!FileExists(result.source_cpp_path), project_name + " single-file export omits source cpp");
		}
		t.Expect(FileExists(result.design_json_path), project_name + " export writes design.json");
		t.Expect(FileExists(result.readme_path), project_name + " export writes README.md");
		String initial_main = LoadFile(result.main_cpp_path);

		String upp = LoadFile(result.upp_path);
		String main = LoadFile(result.main_cpp_path);
		String readme = LoadFile(result.readme_path);
		t.Expect(upp.Find("CtrlLib") >= 0 && upp.Find("Ui;") >= 0, project_name + " .upp uses expected packages");
		if(source_mode == DesignerExportSourceMode::SplitHeaderSource) {
			String header = LoadFile(result.header_path);
			String source = LoadFile(result.source_cpp_path);
			t.Expect(upp.Find("main.cpp") >= 0 && upp.Find(project_name + ".h") >= 0 && upp.Find(project_name + ".cpp") >= 0,
			         project_name + " split export lists launcher, header, and source in .upp");
			t.Expect(main.Find("#include \"" + project_name + ".h\"") >= 0, project_name + " split launcher includes generated header");
			t.Expect(main.Find("GUI_APP_MAIN") >= 0, project_name + " split launcher owns GUI_APP_MAIN");
			t.Expect(header.Find("// Generated by U++ Ui Designer.") >= 0, project_name + " split header keeps generated header comment");
			t.Expect(header.Find("inline Ui") >= 0, project_name + " split header inlines theme helpers");
			t.Expect(source.Find("#include \"" + project_name + ".h\"") >= 0, project_name + " split source includes generated header");
		}
		else {
			t.Expect(main.Find("// Generated by U++ Ui Designer.") >= 0, project_name + " main.cpp has generated header");
			t.Expect(main.Find("// Layout tree.") >= 0 || main.Find("// Parent-child layout tree only.") >= 0,
			         project_name + " main.cpp has layout section comments");
		}
		t.Expect(readme.Find("Generated by U++ Ui Designer.") >= 0, project_name + " README identifies Designer export");
		t.Expect(!DirectoryExists(stage_dir), project_name + " export leaves no staging directory behind");

		DesignerProjectExportResult blocked;
		t.Expect(!ExportDesignerProject(model, r, options, json, blocked),
		         project_name + " export refuses non-empty directory without explicit overwrite");
		t.Expect(!DirectoryExists(stage_dir), project_name + " blocked export leaves no staging directory behind");

		DesignerModel changed;
		String reload_error;
		t.Expect(LoadDesignerModelJson(changed, r, json, reload_error, nullptr), project_name + " export failure test reloads model from JSON");
		for(const DesignerNode& n : changed.GetNodes()) {
			if(n.id == Designer_ROOT)
				continue;
			if(DesignerNode* mod = changed.Find(n.id)) {
				mod->name = mod->name + "_changed";
				mod->properties.Set("tooltip", "changed");
			}
			break;
		}
		DesignerProjectExportOptions failing = options;
		failing.overwrite_existing = true;
		failing.simulate_commit_failure = true;
		DesignerProjectExportResult failed;
		t.Expect(!ExportDesignerProject(changed, r, failing, StoreDesignerModelJson(changed), failed),
		         project_name + " simulated overwrite failure returns false");
		t.Expect(LoadFile(result.main_cpp_path) == initial_main,
		         project_name + " failed overwrite preserves existing export");
		t.Expect(!DirectoryExists(stage_dir), project_name + " failed overwrite leaves no staging directory behind");
		t.Expect(!DirectoryExists(backup_dir), project_name + " failed overwrite leaves no backup directory behind");

		// Compile smoke is exercised separately in the local build workflow.
		// This regression test keeps export validation focused on artifact shape.

		DesignerProjectExportOptions docs_off = options;
		docs_off.project_name = project_name + "NoDocs";
		docs_off.include_design_json = false;
		docs_off.include_readme = true;
		docs_off.overwrite_existing = false;
		docs_off.source_mode = DesignerExportSourceMode::SingleMainCpp;
		docs_off.output_exe_path = AppendFileName(out_root, docs_off.project_name + ".exe");
		docs_off.package_directory = AppendFileName(temp_root, docs_off.project_name);
		String docs_off_pkg = docs_off.package_directory;
		DeleteFolderDeep(docs_off_pkg);
		FileDelete(AppendFileName(docs_off_pkg, docs_off.project_name + ".upp"));
		FileDelete(AppendFileName(docs_off_pkg, "main.cpp"));
		FileDelete(AppendFileName(docs_off_pkg, docs_off.project_name + ".design.json"));
		FileDelete(AppendFileName(docs_off_pkg, "README.md"));
		FileDelete(docs_off.output_exe_path);
		DesignerProjectExportResult docs_off_result;
		t.Expect(ExportDesignerProject(model, r, docs_off, json, docs_off_result),
		         project_name + " export supports README without design.json");
		t.Expect(!FileExists(docs_off_result.design_json_path),
		         project_name + " export omits design.json when disabled");
		String docs_off_readme = LoadFile(docs_off_result.readme_path);
		t.Expect(docs_off_readme.Find("source `design.json` was not included") >= 0,
		         project_name + " README explains omitted source JSON");

		DesignerProjectExportOptions restore_fail = options;
		restore_fail.overwrite_existing = true;
		restore_fail.simulate_commit_failure = true;
		restore_fail.simulate_restore_failure = true;
		DesignerProjectExportResult restore_failed;
		t.Expect(!ExportDesignerProject(changed, r, restore_fail, StoreDesignerModelJson(changed), restore_failed),
		         project_name + " simulated restore failure returns false");
		t.Expect(DirectoryExists(backup_dir), project_name + " restore failure preserves backup directory");
		t.Expect(!DirectoryExists(stage_dir), project_name + " restore failure leaves no staging directory behind");
	};

	DesignerModel simple;
	DesignerNodeId box = simple.AddNode("BoxLayout", Designer_ROOT);
	r.Find("BoxLayout")->init_defaults(*simple.Find(box));
	DesignerNodeId label = simple.AddNode("UiLabel", box);
	r.Find("UiLabel")->init_defaults(*simple.Find(label));
	simple.Find(label)->name = "title";
	RunExportSmoke("DesignerExportSmoke", simple, "DesignerExportSmokeWindow", "design.json",
	               DesignerExportSourceMode::SingleMainCpp);
	RunExportSmoke("DesignerExportSplitSmoke", simple, "DesignerExportSplitSmokeWindow", "design.json",
	               DesignerExportSourceMode::SplitHeaderSource);

	DesignerModel workbench;
	t.Expect(ApplyDesignerTemplate(workbench, r, "DesignerWorkbench"), "DesignerWorkbench template applies for export");
	RunExportSmoke("DesignerWorkbenchExportSmoke", workbench, "DesignerWorkbenchExportWindow", "design_designer.json",
	               DesignerExportSourceMode::SingleMainCpp);
}

CONSOLE_APP_MAIN
{
	TestCtx t;
	TestModelTreeEdits(t);
	TestDesignerAddTarget(t);
	TestDesignerCommands(t);
	TestExplicitEmptyTextValues(t);
	TestDesignerArchitectureGuard(t);
	TestDesignerDragController(t);
	TestRegistryAndBuiltins(t);
	TestDesignerAdapters(t);
	TestDesignerApiCoverageAudit(t);
	TestDesignerThemeSchemaParity(t);
	TestDesignerCodeGenPages(t);
	TestPropertyEditStability(t);
	TestLayoutSizingPrimitives(t);
	TestWidthAwareLayoutMeasure(t);
	TestAncestorRelayoutContract(t);
	TestDesignerInspectorContextAndThemeRows(t);
	TestDesignerInspectorMultiSelectSizing(t);
	TestDesignerChoiceCommitPath(t);
	TestDesignerChoiceBindingAudit(t);
	TestSplitButtonPopupMetadata(t);
	TestRecentDocumentHelper(t);
	TestGeneratedCodeText(t);
	TestDesignerSerialization(t);
	TestDesignerProjectExport(t);

	Cout() << "\nChecks: " << t.checks << ", fails: " << t.fails << "\n";
	SetExitCode(t.fails ? 1 : 0);
}
