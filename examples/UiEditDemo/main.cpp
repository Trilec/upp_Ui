#include <CtrlLib/CtrlLib.h>
#include <Ui/Ui.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>

using namespace Upp;

namespace {

enum EditSample : int {
    EDIT_LINE = 0,
    EDIT_PASSWORD,
    EDIT_MASK,
    EDIT_MULTI,
    EDIT_COUNT,
};

String SampleName(EditSample sample)
{
    switch(sample) {
    case EDIT_PASSWORD: return "Password";
    case EDIT_MASK:     return "Mask";
    case EDIT_MULTI:    return "Multi-line";
    case EDIT_LINE:
    default:            return "Single-line";
    }
}

UiAlign ParseTextAlign(const String& value)
{
    if(value == "Center") return UiAlign::CENTER;
    if(value == "Right") return UiAlign::RIGHT;
    return UiAlign::LEFT;
}

struct EditConfig {
    String text;
    String placeholder;
    bool enabled = true;
    bool read_only = false;
    bool accepts_drop = true;
    bool overwrite = false;
    String text_align = "Left";

    Color face = White();
    Color frame = Color(203, 213, 225);
    Color ink = Color(30, 41, 59);
    Color placeholder_ink = Color(148, 163, 184);
    int frame_width = 1;
    int radius = 7;
    int font_height = 13;
    int margin_x = 10;
    int margin_y = 6;

    Color caret = Color(30, 41, 59);
    int caret_width = 1;
    bool block_caret = false;
    Color selection_face = Color(219, 234, 254);
    Color selection_ink = Color(30, 41, 59);

    bool underline_enabled = false;
    int underline_width = 1;
    Color underline = Color(100, 116, 139);

    int tab_size = 4;
    bool show_tabs = false;
    bool show_spaces = false;
    bool show_line_endings = false;

    String password_char = "Bullet";
    bool password_plain = false;
    bool password_eye = true;

    String mask = "##/##/####";
    String mask_prompt = "_";
    String mask_validator = "Date";
    String mask_formatter = "None";
    bool mask_show_error = true;

    bool multi_accept_tabs = true;
};

class UiEditDemoWindow : public TopWindow {
public:
    typedef UiEditDemoWindow CLASSNAME;

    UiEditDemoWindow()
    {
        Title("Ui Edit Family Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1260), DPI(800));

        UiThemeContext context = UiTheme::GetContext();
        context.preset = UiThemePreset::Minimal;
        context.mode = UiThemeMode::Light;
        UiTheme::Set(context);

        RegisterPropertyEditorV1Editors(factory_);
        SeedConfigs();

        Add(header_);
        Add(preview_panel_);
        Add(inspector_panel_);

        header_.SetTitle("Ui edit family")
               .SetSubTitle("Single-line, password, mask and multi-line controls share one selectable PropertyEditor reference")
               .SetMedia(ICON_DESIGN_DESCRIPTION_48())
               .SetMediaSide(UiAlign::LEFT)
               .SetMediaAlign(UiAlign::CENTER, UiAlign::CENTER)
               .SetMediaAutoFit(true)
               .ShowTitleLine(false)
               .SetContentInset(DPI(8))
               .SetContentCell(header_actions_);
        header_actions_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        header_actions_.AddSpacer(1).Expand(1);
        theme_button_.SetIcon(ICON_ACTION_DARK_MODE_48()).SetIconSize(DPI(16), DPI(16))
                     .Tip("Toggle light/dark theme");
        exit_button_.SetIcon(ICON_DESIGN_MODE_OFF_ON_48()).SetIconSize(DPI(16), DPI(16))
                    .Tip("Close demo");
        header_actions_.Add(theme_button_).Fixed(DPI(34));
        header_actions_.Add(exit_button_).Fixed(DPI(34));

        preview_panel_.Add(selector_);
        selector_.SetGap(DPI(6)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        selector_.Add(line_select_).Expand(1);
        selector_.Add(password_select_).Expand(1);
        selector_.Add(mask_select_).Expand(1);
        selector_.Add(multi_select_).Expand(1);
        line_select_.SetText("Single-line").SetCheckable();
        password_select_.SetText("Password").SetCheckable();
        mask_select_.SetText("Mask").SetCheckable();
        multi_select_.SetText("Multi-line").SetCheckable();

        preview_panel_.Add(line_label_);
        preview_panel_.Add(password_label_);
        preview_panel_.Add(mask_label_);
        preview_panel_.Add(multi_label_);
        preview_panel_.Add(line_);
        preview_panel_.Add(password_);
        preview_panel_.Add(mask_);
        preview_panel_.Add(multi_);
        preview_panel_.Add(status_);
        line_label_.SetText("UiLineEdit");
        password_label_.SetText("UiPasswordEdit");
        mask_label_.SetText("UiMaskEdit");
        multi_label_.SetText("UiMultiEdit");
        status_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);

        inspector_panel_.Add(properties_.SizePos());
        properties_.SetFactory(&factory_);
        properties_.SetModel(&model_);
        properties_.SetLabelRatio(38);
        PropertyEditorStyle pe_style = PropertyEditorStyle::System();
        pe_style.show_group_summaries = true;
        properties_.SetStyle(pe_style);

        Connect();
        ApplyTheme();
        SelectSample(EDIT_LINE);
        ApplyAllSamples();
    }

    virtual void Layout() override
    {
        Rect client = GetSize();
        const int pad = DPI(12);
        const int gap = DPI(10);
        const int header_h = DPI(72);
        const int right_w = min(DPI(455), max(DPI(360), client.GetWidth() * 37 / 100));

        header_.SetRect(pad, pad, max(0, client.GetWidth() - pad * 2), header_h);
        const int top = pad + header_h + gap;
        const int body_h = max(0, client.GetHeight() - top - pad);
        const int preview_w = max(0, client.GetWidth() - pad * 3 - right_w);
        preview_panel_.SetRect(pad, top, preview_w, body_h);
        inspector_panel_.SetRect(pad + preview_w + gap, top, right_w, body_h);

        Rect pr = preview_panel_.GetSize();
        const int inset = DPI(24);
        selector_.SetRect(inset, DPI(18), max(0, pr.GetWidth() - inset * 2), DPI(34));
        const int label_h = DPI(24);
        const int edit_h = DPI(36);
        const int gap_y = DPI(14);
        int y = DPI(78);
        const int edit_w = max(DPI(200), pr.GetWidth() - inset * 2);

        line_label_.SetRect(inset, y, edit_w, label_h); y += label_h;
        line_.SetRect(inset, y, edit_w, edit_h); y += edit_h + gap_y;
        password_label_.SetRect(inset, y, edit_w, label_h); y += label_h;
        password_.SetRect(inset, y, edit_w, edit_h); y += edit_h + gap_y;
        mask_label_.SetRect(inset, y, edit_w, label_h); y += label_h;
        mask_.SetRect(inset, y, edit_w, edit_h); y += edit_h + gap_y;
        multi_label_.SetRect(inset, y, edit_w, label_h); y += label_h;
        const int multi_h = max(DPI(120), pr.GetHeight() - y - DPI(70));
        multi_.SetRect(inset, y, edit_w, multi_h);
        status_.SetRect(inset, max(0, pr.bottom - DPI(42)), edit_w, DPI(26));
    }

private:
    PropertyEditorItem& Resettable(PropertyEditorItem& item)
    {
        item.SetDefault(item.value);
        return item;
    }

    Value Get(const char *id) const
    {
        const PropertyEditorItem *item = model_.Find(id);
        return item ? item->value : Value();
    }

    void SeedConfigs()
    {
        cfg_[EDIT_LINE].text = "Edit me";
        cfg_[EDIT_LINE].placeholder = "Single-line text";

        cfg_[EDIT_PASSWORD].text = "correct horse battery staple";
        cfg_[EDIT_PASSWORD].placeholder = "Password";

        cfg_[EDIT_MASK].text = "12/31/2026";
        cfg_[EDIT_MASK].placeholder = "MM/DD/YYYY";

        cfg_[EDIT_MULTI].text = "First line\nSecond line\nThird line";
        cfg_[EDIT_MULTI].placeholder = "Multi-line notes";
        cfg_[EDIT_MULTI].show_line_endings = false;
    }

    void BuildModel(EditSample sample)
    {
        const EditConfig& cfg = cfg_[sample];
        model_.Clear(false);

        if(sample == EDIT_MULTI)
            Resettable(model_.AddMultiline("text", "Text", cfg.text, "Content").SetExpandedRowSpan(3));
        else
            Resettable(model_.AddText("text", "Text", cfg.text, "Content"));
        Resettable(model_.AddText("placeholder", "Placeholder", cfg.placeholder, "Content"));

        Resettable(model_.AddBoolean("enabled", "Enabled", cfg.enabled, "Behaviour"));
        Resettable(model_.AddBoolean("read_only", "Read only", cfg.read_only, "Behaviour"));
        Resettable(model_.AddBoolean("accepts_drop", "Accept drop", cfg.accepts_drop, "Behaviour"));
        Resettable(model_.AddBoolean("overwrite", "Overwrite mode", cfg.overwrite, "Behaviour"));
        Resettable(model_.AddChoice("text_align", "Text alignment", cfg.text_align, "Behaviour")
            .AddChoice("Left", "Left").AddChoice("Center", "Center").AddChoice("Right", "Right"));

        Resettable(model_.AddColor("face", "Face", cfg.face, "Face"));
        Resettable(model_.AddColor("frame", "Frame", cfg.frame, "Frame"));
        Resettable(model_.AddNumericInt("frame_width", "Frame width", cfg.frame_width, 0, 12, 1, "Frame").SetUnit("px"));
        Resettable(model_.AddNumericInt("radius", "Radius", cfg.radius, 0, 40, 1, "Frame").SetUnit("px"));
        Resettable(model_.AddColor("ink", "Ink", cfg.ink, "Ink"));
        Resettable(model_.AddColor("placeholder_ink", "Placeholder", cfg.placeholder_ink, "Ink"));
        Resettable(model_.AddNumericInt("font_height", "Font height", cfg.font_height, 8, 40, 1, "Typography").SetUnit("px"));
        Resettable(model_.AddNumericInt("margin_x", "Horizontal", cfg.margin_x, 0, 40, 1, "Content Margin").SetUnit("px"));
        Resettable(model_.AddNumericInt("margin_y", "Vertical", cfg.margin_y, 0, 32, 1, "Content Margin").SetUnit("px"));

        Resettable(model_.AddColor("caret", "Caret colour", cfg.caret, "Editing"));
        Resettable(model_.AddNumericInt("caret_width", "Caret width", cfg.caret_width, 1, 8, 1, "Editing").SetUnit("px"));
        Resettable(model_.AddBoolean("block_caret", "Block caret", cfg.block_caret, "Editing"));
        Resettable(model_.AddColor("selection_face", "Selection face", cfg.selection_face, "Editing"));
        Resettable(model_.AddColor("selection_ink", "Selection ink", cfg.selection_ink, "Editing"));

        Resettable(model_.AddBoolean("underline_enabled", "Enabled", cfg.underline_enabled, "Underline"));
        Resettable(model_.AddNumericInt("underline_width", "Width", cfg.underline_width, 1, 8, 1, "Underline").SetUnit("px"));
        Resettable(model_.AddColor("underline", "Colour", cfg.underline, "Underline"));

        if(sample == EDIT_PASSWORD) {
            Resettable(model_.AddChoice("password_char", "Mask character", cfg.password_char, "Password")
                .AddChoice("Bullet", "Bullet •").AddChoice("Asterisk", "Asterisk *")
                .AddChoice("MiddleDot", "Middle dot ·"));
            Resettable(model_.AddBoolean("password_plain", "Show plain text", cfg.password_plain, "Password"));
            Resettable(model_.AddBoolean("password_eye", "Visibility button", cfg.password_eye, "Password"));
        }
        else if(sample == EDIT_MASK) {
            Resettable(model_.AddText("mask", "Mask", cfg.mask, "Mask"));
            Resettable(model_.AddText("mask_prompt", "Prompt character", cfg.mask_prompt, "Mask"));
            Resettable(model_.AddChoice("mask_validator", "Validator", cfg.mask_validator, "Mask")
                .AddChoice("None", "None").AddChoice("Date", "Date")
                .AddChoice("Time", "Time").AddChoice("NonEmpty", "Non-empty")
                .AddChoice("Alnum", "Alnum + underscore"));
            Resettable(model_.AddChoice("mask_formatter", "Formatter", cfg.mask_formatter, "Mask")
                .AddChoice("None", "None").AddChoice("Uppercase", "Uppercase")
                .AddChoice("Lowercase", "Lowercase").AddChoice("TitleCase", "Title case")
                .AddChoice("Username", "Username").AddChoice("SafeAlnum", "Safe alnum"));
            Resettable(model_.AddBoolean("mask_show_error", "Show invalid state", cfg.mask_show_error, "Mask"));
        }
        else if(sample == EDIT_MULTI) {
            Resettable(model_.AddBoolean("multi_accept_tabs", "Accept tabs", cfg.multi_accept_tabs, "Whitespace"));
            Resettable(model_.AddNumericInt("tab_size", "Tab size", cfg.tab_size, 1, 12, 1, "Whitespace"));
            Resettable(model_.AddBoolean("show_tabs", "Show tabs", cfg.show_tabs, "Whitespace"));
            Resettable(model_.AddBoolean("show_spaces", "Show spaces", cfg.show_spaces, "Whitespace"));
            Resettable(model_.AddBoolean("show_line_endings", "Show line endings", cfg.show_line_endings, "Whitespace"));
        }

        model_.SetGroupSubtitle("Content", SampleName(sample) + " sample content");
        model_.SetGroupSubtitle("Behaviour", "shared UiBaseEdit behaviour");
        model_.SetGroupSubtitle("Editing", "caret and selection");
        if(sample == EDIT_MULTI)
            model_.SetGroupSubtitle("Whitespace", "multi-line whitespace rendering");
        model_.StructureChanged();
        properties_.RefreshModel();
    }

    void PullConfig(EditSample sample)
    {
        EditConfig& cfg = cfg_[sample];
        cfg.text = AsString(Get("text"));
        cfg.placeholder = AsString(Get("placeholder"));
        cfg.enabled = (bool)Get("enabled");
        cfg.read_only = (bool)Get("read_only");
        cfg.accepts_drop = (bool)Get("accepts_drop");
        cfg.overwrite = (bool)Get("overwrite");
        cfg.text_align = AsString(Get("text_align"));
        cfg.face = Color(Get("face"));
        cfg.frame = Color(Get("frame"));
        cfg.frame_width = (int)Get("frame_width");
        cfg.radius = (int)Get("radius");
        cfg.ink = Color(Get("ink"));
        cfg.placeholder_ink = Color(Get("placeholder_ink"));
        cfg.font_height = (int)Get("font_height");
        cfg.margin_x = (int)Get("margin_x");
        cfg.margin_y = (int)Get("margin_y");
        cfg.caret = Color(Get("caret"));
        cfg.caret_width = (int)Get("caret_width");
        cfg.block_caret = (bool)Get("block_caret");
        cfg.selection_face = Color(Get("selection_face"));
        cfg.selection_ink = Color(Get("selection_ink"));
        cfg.underline_enabled = (bool)Get("underline_enabled");
        cfg.underline_width = (int)Get("underline_width");
        cfg.underline = Color(Get("underline"));

        if(sample == EDIT_PASSWORD) {
            cfg.password_char = AsString(Get("password_char"));
            cfg.password_plain = (bool)Get("password_plain");
            cfg.password_eye = (bool)Get("password_eye");
        }
        else if(sample == EDIT_MASK) {
            cfg.mask = AsString(Get("mask"));
            cfg.mask_prompt = AsString(Get("mask_prompt"));
            cfg.mask_validator = AsString(Get("mask_validator"));
            cfg.mask_formatter = AsString(Get("mask_formatter"));
            cfg.mask_show_error = (bool)Get("mask_show_error");
        }
        else if(sample == EDIT_MULTI) {
            cfg.multi_accept_tabs = (bool)Get("multi_accept_tabs");
            cfg.tab_size = (int)Get("tab_size");
            cfg.show_tabs = (bool)Get("show_tabs");
            cfg.show_spaces = (bool)Get("show_spaces");
            cfg.show_line_endings = (bool)Get("show_line_endings");
        }
    }

    UiBaseEdit::Style MakeStyle(const EditConfig& cfg) const
    {
        UiBaseEdit::Style style = UiTheme::ResolveEdit(UiRole::Standard);
        for(int i = 0; i < 4; i++) {
            style.palette.face[i] = UiFill::Solid(cfg.face);
            style.palette.frame[i] = cfg.frame;
            style.palette.ink[i] = cfg.ink;
            style.underline[i] = cfg.underline;
        }
        style.metrics.face_enabled = true;
        style.metrics.frame_enabled = cfg.frame_width > 0;
        style.metrics.frame_width = cfg.frame_width;
        style.metrics.radius = cfg.radius;
        style.metrics.content_margin = Rect(cfg.margin_x, cfg.margin_y, cfg.margin_x, cfg.margin_y);
        style.font.Height(cfg.font_height);
        style.text_align = ParseTextAlign(cfg.text_align);
        style.placeholder_ink = cfg.placeholder_ink;
        style.caret_color = cfg.caret;
        style.caret_width = cfg.caret_width;
        style.block_caret = cfg.block_caret;
        style.selection_color = cfg.selection_face;
        style.selection_ink = cfg.selection_ink;
        style.underline_enabled = cfg.underline_enabled;
        style.underline_width = cfg.underline_width;
        style.tab_size = cfg.tab_size;
        style.show_tabs = cfg.show_tabs;
        style.show_spaces = cfg.show_spaces;
        style.show_line_endings = cfg.show_line_endings;
        return style;
    }

    void ApplyCommon(UiBaseEdit& edit, const EditConfig& cfg)
    {
        edit.SetCustomStyle(MakeStyle(cfg));
        edit.SetPlaceholder(cfg.placeholder);
        edit.SetAcceptsDrop(cfg.accepts_drop);
        edit.SetOverwriteMode(cfg.overwrite);
        edit.SetTextAlign(ParseTextAlign(cfg.text_align));
        edit.SetEditable(!cfg.read_only);
        edit.Enable(cfg.enabled);
    }

    void ApplySample(EditSample sample)
    {
        EditConfig& cfg = cfg_[sample];
        switch(sample) {
        case EDIT_PASSWORD: {
            ApplyCommon(password_, cfg);
            wchar mask_char = cfg.password_char == "Asterisk" ? '*' :
                              cfg.password_char == "MiddleDot" ? 0x00B7 : 0x2022;
            password_.SetPasswordChar(mask_char)
                     .EnableVisibilityIcon(cfg.password_eye)
                     .SetPlainTextVisible(cfg.password_plain);
            if(password_.GetTextUtf8() != cfg.text)
                password_.SetTextUtf8(cfg.text);
            break;
        }
        case EDIT_MASK: {
            ApplyCommon(mask_, cfg);
            const char prompt = cfg.mask_prompt.IsEmpty() ? '_' : cfg.mask_prompt[0];
            mask_.SetMask(cfg.mask, prompt);
            Function<bool(const String&)> validator;
            if(cfg.mask_validator == "Date") validator = UiMaskEdit::DateValidator();
            else if(cfg.mask_validator == "Time") validator = UiMaskEdit::TimeValidator();
            else if(cfg.mask_validator == "NonEmpty") validator = UiMaskEdit::NonEmptyValidator();
            else if(cfg.mask_validator == "Alnum") validator = UiMaskEdit::AlnumUnderscoreValidator(true);
            mask_.SetValidator(validator);
            Function<String(const String&)> formatter;
            if(cfg.mask_formatter == "Uppercase") formatter = UiMaskEdit::UppercaseFormatter();
            else if(cfg.mask_formatter == "Lowercase") formatter = UiMaskEdit::LowercaseFormatter();
            else if(cfg.mask_formatter == "TitleCase") formatter = UiMaskEdit::TitleCaseFormatter();
            else if(cfg.mask_formatter == "Username") formatter = UiMaskEdit::UsernameFormatter();
            else if(cfg.mask_formatter == "SafeAlnum") formatter = UiMaskEdit::SafeAlnumFormatter();
            mask_.SetFormatter(formatter);
            if(mask_.GetTextUtf8() != cfg.text)
                mask_.SetTextUtf8(cfg.text);
            mask_.ShowError(cfg.mask_show_error && !mask_.IsValid());
            break;
        }
        case EDIT_MULTI:
            ApplyCommon(multi_, cfg);
            multi_.SetAcceptsTabs(cfg.multi_accept_tabs);
            if(multi_.GetTextUtf8() != cfg.text)
                multi_.SetTextUtf8(cfg.text);
            break;
        case EDIT_LINE:
        default:
            ApplyCommon(line_, cfg);
            if(line_.GetTextUtf8() != cfg.text)
                line_.SetTextUtf8(cfg.text);
            break;
        }
    }

    void ApplyAllSamples()
    {
        for(int i = 0; i < EDIT_COUNT; i++)
            ApplySample((EditSample)i);
        UpdateStatus();
        RefreshLayout();
        Refresh();
    }

    void SelectSample(EditSample sample)
    {
        selected_ = sample;
        line_select_.SetChecked(sample == EDIT_LINE);
        password_select_.SetChecked(sample == EDIT_PASSWORD);
        mask_select_.SetChecked(sample == EDIT_MASK);
        multi_select_.SetChecked(sample == EDIT_MULTI);
        BuildModel(sample);
        UpdateStatus();
    }

    void Connect()
    {
        line_select_.WhenAction = [=] { SelectSample(EDIT_LINE); };
        password_select_.WhenAction = [=] { SelectSample(EDIT_PASSWORD); };
        mask_select_.WhenAction = [=] { SelectSample(EDIT_MASK); };
        multi_select_.WhenAction = [=] { SelectSample(EDIT_MULTI); };
        theme_button_.WhenAction = [=] { ToggleTheme(); };
        exit_button_.WhenAction = [=] { Close(); };

        properties_.WhenPreview = [=](String, Value) {
            PullConfig(selected_);
            ApplySample(selected_);
            UpdateStatus();
        };
        properties_.WhenCommit = [=](String, Value) {
            PullConfig(selected_);
            ApplySample(selected_);
            UpdateStatus();
        };
        properties_.WhenReset = [=](String id) {
            PropertyEditorItem *item = model_.Find(id);
            if(item && item->resettable) {
                model_.SetValue(id, item->default_value);
                PullConfig(selected_);
                ApplySample(selected_);
                properties_.RefreshModel();
                UpdateStatus();
            }
        };

        line_.WhenChange = [=] { CaptureText(EDIT_LINE, line_); };
        password_.WhenChange = [=] { CaptureText(EDIT_PASSWORD, password_); };
        mask_.WhenChange = [=] { CaptureText(EDIT_MASK, mask_); };
        multi_.WhenChange = [=] { CaptureText(EDIT_MULTI, multi_); };
    }

    void CaptureText(EditSample sample, UiBaseEdit& edit)
    {
        cfg_[sample].text = edit.GetTextUtf8();
        if(selected_ == sample && model_.Find("text")) {
            model_.SetValue("text", cfg_[sample].text, false);
            properties_.RefreshValue("text");
        }
        if(sample == EDIT_MASK)
            mask_.ShowError(cfg_[EDIT_MASK].mask_show_error && !mask_.IsValid());
    }

    void UpdateStatus()
    {
        String detail = SampleName(selected_);
        if(selected_ == EDIT_MASK)
            detail << (mask_.IsValid() ? " · valid" : " · invalid");
        else if(selected_ == EDIT_PASSWORD)
            detail << (password_.IsPlainTextVisible() ? " · visible" : " · masked");
        else if(selected_ == EDIT_MULTI)
            detail << Format(" · %d chars", multi_.GetTextUtf8().GetCount());
        status_.SetText("Selected: " + detail);
    }

    void ToggleTheme()
    {
        UiThemeContext context = UiTheme::GetContext();
        context.mode = context.mode == UiThemeMode::Dark ? UiThemeMode::Light
                                                          : UiThemeMode::Dark;
        UiTheme::Set(context);
        Ctrl::SwapDarkLight();
        ApplyTheme();
        ApplyAllSamples();
    }

    void ApplyTheme()
    {
        UiTitleCard::Style header_style = UiTheme::ResolveTitleCard(UiRole::Accent);
        header_style.title_line = false;
        header_.SetCustomStyle(header_style);
        preview_panel_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
        inspector_panel_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle));
        line_label_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
        password_label_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
        mask_label_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
        multi_label_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
        status_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
        exit_button_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Alert));
        theme_button_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Standard));

        UiButton *selectors[] = { &line_select_, &password_select_, &mask_select_, &multi_select_ };
        for(int i = 0; i < EDIT_COUNT; i++)
            selectors[i]->SetCustomStyle(UiTheme::ResolveButton(i == selected_ ? UiRole::Accent
                                                                                : UiRole::Subtle));

        properties_.SetPaletteMode(UiTheme::GetContext().mode == UiThemeMode::Dark
            ? PropertyEditorPaletteMode::Dark : PropertyEditorPaletteMode::Light);
    }

private:
    EditConfig cfg_[EDIT_COUNT];
    EditSample selected_ = EDIT_LINE;

    UiTitleCard header_;
    UiBoxLayout header_actions_ { UiDirection::H };
    UiToolButton theme_button_, exit_button_;
    UiPanel preview_panel_, inspector_panel_;
    UiBoxLayout selector_ { UiDirection::H };
    UiButton line_select_, password_select_, mask_select_, multi_select_;
    UiLabel line_label_, password_label_, mask_label_, multi_label_, status_;
    UiLineEdit line_;
    UiPasswordEdit password_;
    UiMaskEdit mask_;
    UiMultiEdit multi_;
    PropertyEditor properties_;
    PropertyEditorFactory factory_;
    PropertyEditorModel model_;
};

} // namespace

GUI_APP_MAIN
{
    UiEditDemoWindow().Run();
}
