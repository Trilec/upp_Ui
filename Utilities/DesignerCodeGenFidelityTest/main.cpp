#include <Core/Core.h>
#include <Ui/Ui.h>

#include "../Designer/DesignerModel.h"
#include "../Designer/DesignerRegistry.h"
#include "../Designer/DesignerBuiltins.h"
#include "../Designer/DesignerSerialization.h"
#include "../Designer/DesignerCodeGen.h"

#include "../Designer/controls/DesignerControlFamilyShared.cpp"
#include "../Designer/controls/DesignerLayoutControls.cpp"
#include "../Designer/controls/DesignerContainerControls.cpp"
#include "../Designer/controls/DesignerButtonControls.cpp"
#include "../Designer/controls/DesignerEditControls.cpp"
#include "../Designer/controls/DesignerDisplayControls.cpp"
#include "../Designer/controls/DesignerCompositeControls.cpp"
#include "../Designer/controls/DesignerDataControls.cpp"

#include "../Designer/DesignerModel.cpp"
#include "../Designer/DesignerRegistry.cpp"
#include "../Designer/DesignerBuiltins.cpp"
#include "../Designer/DesignerAdapter.cpp"
#include "../Designer/DesignerSerialization.cpp"
#include "../Designer/DesignerCodeGen.cpp"

using namespace Upp;

struct FidelityCtx {
	int checks = 0;
	int fails = 0;

	void Check(bool cond, const String& msg)
	{
		++checks;
		if(!cond) {
			++fails;
			Cout() << "[FAIL] " << msg << "\n";
		}
	}
};

static String NormalizeEol(String s)
{
	s.Replace("\r\n", "\n");
	s.Replace("\r", "\n");
	return s;
}

static String FindRepoRoot()
{
	String dir = GetFileFolder(GetExeFilePath());
	for(int i = 0; i < 8; i++) {
		if(FileExists(AppendFileName(dir, "Utilities/DesignerWorkbenchExport/design.json")))
			return dir;
		String parent = GetFileFolder(dir);
		if(parent.IsEmpty() || parent == dir)
			break;
		dir = parent;
	}
	return String();
}

static DesignerModel LoadCanonicalModel(FidelityCtx& t, const DesignerRegistry& registry, const String& design_path)
{
	DesignerModel model;
	String error;
	Vector<String> notes;
	String json = LoadFile(design_path);
	t.Check(!json.IsEmpty(), "canonical design.json loads from disk");
	if(json.IsEmpty())
		return model;
	t.Check(LoadDesignerModelJson(model, registry, json, error, &notes), error.IsEmpty() ? "canonical design.json parses" : error);
	t.Check(model.Validate(), "canonical design model validates");
	return model;
}

static void CheckContains(FidelityCtx& t, const String& code, const String& needle, const String& msg)
{
	t.Check(code.Find(needle) >= 0, msg + " (" + needle + ")");
}

CONSOLE_APP_MAIN
{
	FidelityCtx t;

	String repo_root = FindRepoRoot();
	t.Check(!repo_root.IsEmpty(), "repository root is discoverable");
	if(repo_root.IsEmpty()) {
		Cout() << "Unable to locate repository root.\n";
		SetExitCode(1);
		return;
	}

	String design_path = AppendFileName(repo_root, "Utilities/DesignerWorkbenchExport/design.json");
	String canonical_cpp_path = AppendFileName(repo_root, "Utilities/DesignerWorkbenchExport/main.cpp");
	String canonical_cpp = LoadFile(canonical_cpp_path);

	DesignerRegistry registry;
	RegisterDesignerBuiltins(registry);

	DesignerModel model = LoadCanonicalModel(t, registry, design_path);

	DesignerCodeGenOptions exact;
	exact.class_name = "DesignerWorkbenchExportWindow";
	exact.appearance_mode = DesignerAppearanceMode::ExactDesign;
	String exact_code = GenerateDesignerCode(model, registry, exact);

	CheckContains(t, exact_code, "Makezoom_aspect_panelStyle()", "exact code emits zoom aspect style helper");
	CheckContains(t, exact_code, "zoom_aspect_panel.SetCustomStyle(Makezoom_aspect_panelStyle());", "exact code applies zoom aspect style helper");
	CheckContains(t, exact_code, "Makecenter_panelStyle()", "exact code emits center panel style helper");
	CheckContains(t, exact_code, "center_panel.SetCustomStyle(Makecenter_panelStyle());", "exact code applies center panel style helper");
	CheckContains(t, exact_code, "Makeleft_tool_button_panelStyle()", "exact code emits left tool panel style helper");
	CheckContains(t, exact_code, "left_tool_button_panel.SetCustomStyle(Makeleft_tool_button_panelStyle());", "exact code applies left tool panel style helper");
	CheckContains(t, exact_code, "Makeright_tool_button_panelStyle()", "exact code emits right tool panel style helper");
	CheckContains(t, exact_code, "right_tool_button_panel.SetCustomStyle(Makeright_tool_button_panelStyle());", "exact code applies right tool panel style helper");
	CheckContains(t, exact_code, "header_title_card.SetTitle(\"Designer\").SetSubTitle(\"\").SetContentInset(DPI(4)).SetMediaGap(DPI(9)).SetMediaReserve(", "exact code emits header title card chain");
	CheckContains(t, exact_code, ".ShowTitleLine(false)", "exact code explicitly disables the header title line");
	CheckContains(t, exact_code, ".ShowCardLine(false)", "exact code explicitly disables the header card line");
	CheckContains(t, exact_code, "version_label.SetIconScaleToContent(false);", "exact code keeps the version badge from auto-scaling");
	CheckContains(t, exact_code, "UiTheme::ResolvePanel(UiRole::Subtle)", "exact code keeps role-aware panel styling");
	CheckContains(t, exact_code, "s.metrics.face_enabled = true;", "exact code emits explicit fill enable");
	CheckContains(t, exact_code, "s.metrics.frame_enabled = true;", "exact code emits explicit frame enable");
	CheckContains(t, exact_code, "s.metrics.frame_width = DPI(1);", "exact code emits explicit frame width");
	CheckContains(t, exact_code, "s.metrics.radius = DPI(19);", "exact code emits explicit radius");
	CheckContains(t, exact_code, "s.metrics.shadow.enabled = true;", "exact code emits explicit shadow enable");
	CheckContains(t, exact_code, "s.metrics.shadow.distance = DPI(10);", "exact code emits explicit shadow distance");
	CheckContains(t, exact_code, "s.metrics.shadow.alpha = 38;", "exact code emits explicit shadow alpha");
	CheckContains(t, exact_code, "center_panel.SetSizeMin(DPI(0), DPI(0));", "exact code preserves center panel sizing");
	CheckContains(t, exact_code, "zoom_aspect_panel.SetSizeMin(DPI(0), DPI(0));", "exact code preserves zoom aspect sizing");
	CheckContains(t, exact_code, ".LineOrientation(UiSpacerLineOrientation::Vertical)", "exact code preserves vertical spacer line");
	CheckContains(t, exact_code, ".LineAlign(UiCrossAlign::Start)", "exact code preserves spacer line alignment");
	CheckContains(t, exact_code, ".LineThickness(DPI(2))", "exact code preserves spacer line thickness");
	CheckContains(t, exact_code, ".LineInset(DPI(2))", "exact code preserves spacer line inset");
	CheckContains(t, exact_code, "zoom_aspect_layout.AddSpacer(1);", "exact code emits zoom aspect spacer");
	CheckContains(t, exact_code, "LineOrientation(UiSpacerLineOrientation::Vertical)", "exact code keeps aspect spacer vertical");
	CheckContains(t, exact_code, "center_box_layout.Add(zoom_aspect_panel).Fixed(DPI(63))", "exact code preserves centered aspect strip height");
	CheckContains(t, exact_code, "MinMaxCross(DPI(278), DPI(278))", "exact code preserves centered aspect strip width");
	CheckContains(t, exact_code, "rightlayout.Add(right_tool_button_panel).Fixed(DPI(63))", "exact code preserves right tool strip height");
	CheckContains(t, exact_code, "MinMaxCross(DPI(346), DPI(346))", "exact code preserves right tool strip width");

	String theme_first_code = GenerateDesignerCode(model, registry, "DesignerWorkbenchExportWindow", DesignerAppearanceMode::ThemeFirst);
	t.Check(theme_first_code.Find("Makezoom_aspect_panelStyle()") < 0, "theme-first code omits exact appearance helpers");
	t.Check(theme_first_code.Find("SetCustomStyle(Makezoom_aspect_panelStyle())") < 0, "theme-first code does not apply omitted helpers");
	t.Check(theme_first_code.Find("Designer appearance export omitted in theme-first mode.") >= 0,
	        "theme-first code states that appearance export was omitted");

	if(!canonical_cpp.IsEmpty()) {
		String a = NormalizeEol(exact_code);
		String b = NormalizeEol(canonical_cpp);
		t.Check(a == b, "exact generated code matches canonical main.cpp");
	}

	bool write_canonical = false;
	for(const String& arg : CommandLine())
		if(ToLower(TrimBoth(arg)) == "--write-canonical")
			write_canonical = true;
	if(write_canonical) {
		if(!SaveFile(canonical_cpp_path, exact_code)) {
			Cout() << "[FAIL] unable to write canonical main.cpp\n";
			SetExitCode(1);
			return;
		}
		Cout() << "Canonical main.cpp refreshed.\n";
		return;
	}

	Cout() << Format("Checks: %d  Fails: %d\n", t.checks, t.fails);
	if(t.fails)
		SetExitCode(1);
}
