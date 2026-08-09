/*
    UiLabelDemo
    ===========

    Purpose
    -------
    First reference shell for the next generation of self-contained Ui demos.
    It deliberately mirrors the visual language of UiDesigner without depending
    on UiDesigner or a shared demo framework.

    Current shell
    -------------
    - UiTitleCard identity header with theme, help, and exit actions.
    - Generous live preview containing the real UiLabel control.
    - Compact Inspector / Theme Overrides / Code tool buttons on the right.
    - One UiStack below those buttons; the tool buttons switch its active page.

    Next step
    ---------
    Once this shell is visually accepted, the Inspector and Theme Overrides
    placeholder pages will be replaced by the production PropertyEditor. Keep
    that integration explicit and readable in this package rather than hiding
    it behind a demo-specific application framework.

    Demo rule
    ---------
    This package should stay small, self-contained, and useful as readable U++
    example code. A developer should be able to understand the shell by opening
    this file alone.
*/

#include <Ui/Ui.h>

using namespace Upp;

namespace {

class UiLabelDemo : public TopWindow {
public:
    typedef UiLabelDemo CLASSNAME;

    UiLabelDemo()
    {
        Title("UiLabel Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1180), DPI(760));

        UiThemeContext context = UiTheme::GetContext();
        context.preset = UiThemePreset::Minimal;
        context.mode = UiThemeMode::Light;
        UiTheme::Set(context);

        BuildHeader();
        BuildPreview();
        BuildInspectorRail();
        BuildPages();
        ConnectEvents();
        ApplyTheme();
        SelectRightPage(0);
    }

    virtual void Layout() override
    {
        const Rect client = GetSize();
        const int pad = DPI(12);
        const int gap = DPI(10);
        const int header_h = DPI(72);
        const int action_w = DPI(114);
        const int right_min = DPI(300);
        const int right_max = DPI(390);

        const int right_w = min(right_max,
                                max(right_min, client.GetWidth() * 31 / 100));

        header_.SetRect(pad, pad,
                        max(0, client.GetWidth() - 3 * pad - action_w),
                        header_h);
        header_actions_.SetRect(client.right - pad - action_w, pad,
                                action_w, header_h);

        const int body_top = pad + header_h + gap;
        const int body_h = max(0, client.GetHeight() - body_top - pad);
        const int preview_w = max(0, client.GetWidth() - 3 * pad - right_w);

        preview_surface_.SetRect(pad, body_top, preview_w, body_h);
        right_surface_.SetRect(pad + preview_w + gap, body_top,
                               right_w, body_h);

        LayoutPreview();
        LayoutRightColumn();
    }

private:
    void BuildHeader()
    {
        Add(header_);
        Add(header_actions_);

        header_.SetTitle("UiLabel")
               .SetSubTitle("Styled text and icon presentation control")
               .ShowTitleLine(false)
               .ShowCardLine(false)
               .SetContentInset(DPI(8));

        header_actions_.SetDirection(UiDirection::H)
                       .SetGap(DPI(4))
                       .SetInset(DPI(0))
                       .SetAlignItems(UiCrossAlign::Center);

        theme_.SetIcon(ICON_ACTION_DARK_MODE_48())
              .SetIconSize(DPI(16), DPI(16))
              .Tip("Toggle light / dark theme");
        help_.SetIcon(ICON_DESIGN_HELP_48())
             .SetIconSize(DPI(16), DPI(16))
             .Tip("About this demo");
        exit_.SetIcon(ICON_DESIGN_MODE_OFF_ON_48())
             .SetIconSize(DPI(16), DPI(16))
             .Tip("Close demo");

        header_actions_.Add(theme_).Fixed(DPI(34)).MinCross(DPI(28));
        header_actions_.Add(help_).Fixed(DPI(34)).MinCross(DPI(28));
        header_actions_.Add(exit_).Fixed(DPI(34)).MinCross(DPI(28));
    }

    void BuildPreview()
    {
        Add(preview_surface_);
        preview_surface_.Add(preview_);
        preview_surface_.Add(preview_caption_);

        preview_.SetText("UiLabel preview")
                .SetIcon(ICON_DESIGN_WIDGETS_48(), UiIconRenderMode::MonoTint)
                .SetIconSize(DPI(28), DPI(28))
                .SetIconSide(UiAlign::TOP)
                .SetContentGap(DPI(10))
                .SetAlign(UiAlign::CENTER, UiAlign::CENTER)
                .SetSelectable(true);

        preview_caption_.SetText("Live preview")
                        .SetAlign(UiAlign::CENTER, UiAlign::CENTER);
    }

    void BuildInspectorRail()
    {
        Add(right_surface_);
        right_surface_.Add(right_tools_);
        right_surface_.Add(right_stack_);

        right_tools_.SetDirection(UiDirection::H)
                    .SetGap(DPI(4))
                    .SetInset(DPI(6))
                    .SetAlignItems(UiCrossAlign::Center);

        inspector_mode_.SetIcon(ICON_DESIGN_TUNE_48())
                       .SetIconSize(DPI(17), DPI(17))
                       .SetCheckable()
                       .Tip("Inspector");
        overrides_mode_.SetIcon(ICON_DESIGN_FORMAT_PAINT_48())
                       .SetIconSize(DPI(17), DPI(17))
                       .SetCheckable()
                       .Tip("Theme Overrides");
        code_mode_.SetIcon(ICON_DESIGN_CODE_BLOCKS_48())
                  .SetIconSize(DPI(17), DPI(17))
                  .SetCheckable()
                  .Tip("Code");

        right_tools_.Add(inspector_mode_).Fixed(DPI(38)).MinCross(DPI(30));
        right_tools_.Add(overrides_mode_).Fixed(DPI(38)).MinCross(DPI(30));
        right_tools_.Add(code_mode_).Fixed(DPI(38)).MinCross(DPI(30));
        right_tools_.AddSpacer(1).Expand(1);
    }

    void BuildPages()
    {
        right_stack_.Add(inspector_page_, "inspector");
        right_stack_.Add(overrides_page_, "overrides");
        right_stack_.Add(code_page_, "code");

        inspector_page_.Add(inspector_placeholder_.SizePos());
        overrides_page_.Add(overrides_placeholder_.SizePos());
        code_page_.Add(code_.SizePos());

        inspector_placeholder_.SetText(
            "Inspector\n\n"
            "The production PropertyEditor will expose the normal UiLabel API here "
            "after the shell and proportions are visually accepted.")
            .SetAlign(UiAlign::LEFT, UiAlign::TOP)
            .SetContentGap(DPI(6));

        overrides_placeholder_.SetText(
            "Theme Overrides\n\n"
            "The production PropertyEditor will expose explicit UiLabel style and "
            "theme overrides here, separate from normal control behaviour.")
            .SetAlign(UiAlign::LEFT, UiAlign::TOP)
            .SetContentGap(DPI(6));

        code_.SetReadOnly();
        code_.SetData(
            "UiLabel label;\n"
            "label.SetText(\"UiLabel preview\")\n"
            "     .SetIcon(ICON_DESIGN_WIDGETS_48(), UiIconRenderMode::MonoTint)\n"
            "     .SetIconSize(DPI(28), DPI(28))\n"
            "     .SetIconSide(UiAlign::TOP)\n"
            "     .SetContentGap(DPI(10))\n"
            "     .SetAlign(UiAlign::CENTER, UiAlign::CENTER)\n"
            "     .SetSelectable(true);\n");
    }

    void ConnectEvents()
    {
        inspector_mode_.WhenAction = [=] { SelectRightPage(0); };
        overrides_mode_.WhenAction = [=] { SelectRightPage(1); };
        code_mode_.WhenAction = [=] { SelectRightPage(2); };

        theme_.WhenAction = [=] { ToggleTheme(); };
        help_.WhenAction = [=] {
            PromptOK(
                "UiLabel demo shell\n\n"
                "The left side is a generous live preview. The three right-side "
                "icons switch one UiStack between Inspector, Theme Overrides, "
                "and Code. PropertyEditor integration is intentionally deferred "
                "until this shell is visually accepted.");
        };
        exit_.WhenAction = [=] { Close(); };
    }

    void SelectRightPage(int page)
    {
        page = minmax(page, 0, 2);
        right_stack_.SetActivePage(page);
        inspector_mode_.SetChecked(page == 0);
        overrides_mode_.SetChecked(page == 1);
        code_mode_.SetChecked(page == 2);
    }

    void ToggleTheme()
    {
        UiThemeContext context = UiTheme::GetContext();
        context.mode = context.mode == UiThemeMode::Dark
                     ? UiThemeMode::Light
                     : UiThemeMode::Dark;
        UiTheme::Set(context);
        Ctrl::SwapDarkLight();
        ApplyTheme();
        Refresh();
    }

    void ApplyTheme()
    {
        header_.ClearCustomStyle();

        preview_surface_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
        right_surface_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle));
        inspector_page_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle));
        overrides_page_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle));
        code_page_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle));

        preview_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Title));
        preview_caption_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
        inspector_placeholder_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Body));
        overrides_placeholder_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Body));

        theme_.ClearCustomStyle();
        help_.ClearCustomStyle();
        exit_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Alert));
        inspector_mode_.ClearCustomStyle();
        overrides_mode_.ClearCustomStyle();
        code_mode_.ClearCustomStyle();
    }

    void LayoutPreview()
    {
        const Rect r = preview_surface_.GetSize();
        const int inset = DPI(24);
        const int caption_h = DPI(30);
        const int available_h = max(0, r.GetHeight() - 2 * inset - caption_h);

        const int target_w = min(max(DPI(260), r.GetWidth() - 2 * inset), DPI(560));
        const int target_h = min(max(DPI(120), available_h / 2), DPI(220));
        const int x = (r.GetWidth() - target_w) / 2;
        const int y = max(inset, (available_h - target_h) / 2);

        preview_.SetRect(x, y, target_w, target_h);
        preview_caption_.SetRect(inset,
                                 max(0, r.GetHeight() - inset - caption_h),
                                 max(0, r.GetWidth() - 2 * inset),
                                 caption_h);
    }

    void LayoutRightColumn()
    {
        const Rect r = right_surface_.GetSize();
        const int inset = DPI(6);
        const int tools_h = DPI(42);

        right_tools_.SetRect(inset, inset,
                             max(0, r.GetWidth() - 2 * inset), tools_h);
        right_stack_.SetRect(inset, inset + tools_h + DPI(4),
                             max(0, r.GetWidth() - 2 * inset),
                             max(0, r.GetHeight() - 2 * inset - tools_h - DPI(4)));
    }

private:
    UiTitleCard header_;
    UiBoxLayout header_actions_ { UiDirection::H };
    UiToolButton theme_;
    UiToolButton help_;
    UiToolButton exit_;

    UiPanel preview_surface_;
    UiLabel preview_;
    UiLabel preview_caption_;

    UiPanel right_surface_;
    UiBoxLayout right_tools_ { UiDirection::H };
    UiToolButton inspector_mode_;
    UiToolButton overrides_mode_;
    UiToolButton code_mode_;
    UiStack right_stack_;

    UiPanel inspector_page_;
    UiPanel overrides_page_;
    UiPanel code_page_;
    UiLabel inspector_placeholder_;
    UiLabel overrides_placeholder_;
    UiMultiEdit code_;
};

} // namespace

GUI_APP_MAIN
{
    UiLabelDemo().Run();
}
