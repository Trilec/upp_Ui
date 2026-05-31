/*
    UiSplitButtonDemo
    =================

    Purpose
    - Visual and build smoke demo for UiSplitButton.

    Intent
    - Exercise primary-action clicks, split popup selection, popup width, icon
      rendering, disabled popup rows, and light/dark theme switching.
    - Keep the sample compact enough to act as a regression target for Designer
      header usage and command/history button layouts.

    Changelog
    - v0.1.0: Added first split-button demo with Save/Load-style history
      buttons and role/style variants.
*/

#include <Ui/Ui.h>

using namespace Upp;

namespace {

static const char* DEMO_VERSION = "v0.4.0";

Font DemoFont(int px, bool bold = false)
{
	Font f = SansSerifZ(px);
	if(Font::FindFaceNameIndex("Inter") >= 0)
		f.FaceName("Inter");
	if(bold)
		f.Bold();
	return f;
}

UiLabel::Style LabelStyle(UiRole role, int px, bool bold = false)
{
	UiLabel::Style s = UiTheme::ResolveLabel(role, UiTextSize::Body);
	s.font = DemoFont(px, bold);
	return s;
}

class SplitButtonDemo : public TopWindow {
public:
	typedef SplitButtonDemo CLASSNAME;

	SplitButtonDemo()
	{
		Title("UiSplitButton Demo " + String(DEMO_VERSION));
		Sizeable().Zoomable();
		SetRect(0, 0, DPI(760), DPI(430));

		Add(title_);
		Add(subtitle_);
		Add(mode_);
		Add(save_);
		Add(load_);
		Add(export_);
		Add(compact_);
		Add(disabled_);
		Add(status_);

		title_.SetText("UiSplitButton");
		subtitle_.SetText("Primary command on the left, related choices on the split arrow.");
		status_.SetText("Ready");

		mode_.SetText("Dark");
		mode_.WhenAction = [=] {
			dark_mode_ = !dark_mode_;
			UiThemeContext ctx = UiTheme::GetContext();
			ctx.mode = dark_mode_ ? UiThemeMode::Dark : UiThemeMode::Light;
			UiTheme::Set(ctx);
			ApplyTheme();
		};

		SetupButton(save_, "Save", CtrlImg::save(), UiRole::Accent, "Save clicked");
		save_.SetPopupMinWidth(DPI(360));
		save_.Add("E:\\apps\\github\\upp_Ui\\designs\\throughline.json", "save_a")
		     .Add("C:\\Users\\admin\\Downloads\\design-throughline2.json", "save_b")
		     .Add("No recent save", Value(), false);
		save_.SetItemDescription(0, "Recent save target");
		save_.SetItemDescription(1, "Imported designer document");

		SetupButton(load_, "Load", CtrlImg::open(), UiRole::Standard, "Load clicked");
		load_.SetPopupMinWidth(DPI(360));
		load_.Add("design.json", "load_a")
		     .Add("layout-sketch.json", "load_b")
		     .Add("empty-state.json", "load_c");
		load_.SetItemDescription(0, "Current workspace");
		load_.SetItemDescription(1, "Recent layout pass");

		SetupButton(export_, "Export", ICON_DESIGN_FOLDER_48(), UiRole::Subtle, "Export clicked");
		export_.Add("Export C++", "cpp")
		       .Add("Export JSON", "json")
		       .Add("Export PNG preview", "png");

		SetupButton(compact_, "", ICON_NAVIGATION_OUTLINED_MORE_VERT_48(), UiRole::Standard, "Compact clicked");
		compact_.SetSplitWidth(DPI(24)).SetPopupMinWidth(DPI(220));
		compact_.Add("Duplicate").Add("Rename").Add("Delete", "delete");

		SetupButton(disabled_, "Disabled", ICON_DESIGN_SETTINGS_48(), UiRole::Standard, "Disabled clicked");
		disabled_.Add("Unavailable", Value(), false);
		disabled_.Disable();

		ApplyTheme();
	}

	void SetupButton(UiSplitButton& b, const String& text, const Image& icon, UiRole role, const String& action)
	{
		b.SetText(text)
		 .SetIcon(icon)
		 .SetIconSize(DPI(16), DPI(16))
		 .SetIconRenderMode(UiIconRenderMode::MonoTint)
		 .SetSplitWidth(DPI(28))
		 .SetPopupMaxItems(6);
		b.WhenAction = [=] { status_.SetText(action); };
		b.WhenSelect = [=](int, const Value& v) {
			status_.SetText("Selected " + AsString(v));
		};
		roles_.Add(&b, role);
	}

	void ApplyTheme()
	{
		title_.SetCustomStyle(LabelStyle(UiRole::Accent, 18, true));
		subtitle_.SetCustomStyle(LabelStyle(UiRole::Subtle, 11));
		status_.SetCustomStyle(LabelStyle(UiRole::Subtle, 11));
		mode_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
		for(int i = 0; i < roles_.GetCount(); i++)
			roles_.GetKey(i)->SetCustomStyle(UiTheme::ResolveButton(roles_[i]));
		Refresh();
	}

	void Layout() override
	{
		Rect r = Rect(GetSize()).Deflated(DPI(20));
		title_.SetRect(r.left, r.top, DPI(260), DPI(26));
		subtitle_.SetRect(r.left, title_.GetRect().bottom + DPI(2), DPI(520), DPI(22));
		mode_.SetRect(r.right - DPI(92), r.top, DPI(92), DPI(28));

		int y = subtitle_.GetRect().bottom + DPI(24);
		save_.SetRect(r.left, y, DPI(108), DPI(34));
		load_.SetRect(save_.GetRect().right + DPI(12), y, DPI(108), DPI(34));
		export_.SetRect(load_.GetRect().right + DPI(12), y, DPI(128), DPI(34));
		compact_.SetRect(export_.GetRect().right + DPI(12), y, DPI(58), DPI(34));
		disabled_.SetRect(compact_.GetRect().right + DPI(12), y, DPI(120), DPI(34));
		status_.SetRect(r.left, y + DPI(58), r.GetWidth(), DPI(26));
	}

	void Paint(Draw& w) override
	{
		Rect r = GetSize();
		bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
		w.DrawRect(r, dark ? Color(24, 27, 32) : Color(250, 252, 255));
		Color dot = dark ? Color(45, 50, 60) : Color(232, 238, 247);
		for(int y = 0; y < r.bottom; y += DPI(12))
			for(int x = 0; x < r.right; x += DPI(12))
				w.DrawRect(x, y, 1, 1, dot);
	}

private:
	UiLabel title_;
	UiLabel subtitle_;
	UiButton mode_;
	UiSplitButton save_;
	UiSplitButton load_;
	UiSplitButton export_;
	UiSplitButton compact_;
	UiSplitButton disabled_;
	UiLabel status_;
	VectorMap<UiSplitButton*, UiRole> roles_;
	bool dark_mode_ = false;
};

}

GUI_APP_MAIN
{
	UiThemeContext ctx;
	ctx.preset = UiThemePreset::Minimal;
	ctx.mode = UiThemeMode::Light;
	UiTheme::Set(ctx);
	SplitButtonDemo().Run();
}
